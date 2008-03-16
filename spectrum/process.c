/*
 *
 *  gtk2 spectrum analyzer
 *    
 *      Copyright (C) 2004 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  The analyzer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include "analyzer.h"

static int blockslice[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};

static float **blockbuffer=0;
static int blockbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static float *window;
static float *freqbuffer=0;
static fftwf_plan plan;

static unsigned char readbuffer[MAX_FILES][readbuffersize];
static int readbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static int readbufferptr[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

static FILE *f[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t offset[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t length[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
static off_t bytesleft[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int seekable[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int global_seekable=0;

pthread_mutex_t feedback_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
int feedback_increment=0;

float *feedback_count;
float **feedback_work;
float *process_work;

float **feedback_acc;
float **feedback_max;
float **feedback_instant;

float **ph_acc;
float **ph_max;
float **ph_instant;

float **xmappingL;
float **xmappingH;
int metascale = -1;
int metawidth = -1;
int metares = -1;

sig_atomic_t acc_clear=0;
sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

static int host_is_big_endian() {
  int32_t pattern = 0xfeedface; /* deadbeef */
  unsigned char *bytewise = (unsigned char *)&pattern;
  if (bytewise[0] == 0xfe) return 1;
  return 0;
}

/* Macros to read header data */
#define READ_U32_LE(buf) \
        (((buf)[3]<<24)|((buf)[2]<<16)|((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U16_LE(buf) \
        (((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U32_BE(buf) \
        (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|((buf)[3]&0xff))

#define READ_U16_BE(buf) \
        (((buf)[0]<<8)|((buf)[1]&0xff))

double read_IEEE80(unsigned char *buf){
  int s=buf[0]&0xff;
  int e=((buf[0]&0x7f)<<8)|(buf[1]&0xff);
  double f=((unsigned long)(buf[2]&0xff)<<24)|
    ((buf[3]&0xff)<<16)|
    ((buf[4]&0xff)<<8) |
    (buf[5]&0xff);
  
  if(e==32767){
    if(buf[2]&0x80)
      return HUGE_VAL; /* Really NaN, but this won't happen in reality */
    else{
      if(s)
	return -HUGE_VAL;
      else
	return HUGE_VAL;
    }
  }
  
  f=ldexp(f,32);
  f+= ((buf[6]&0xff)<<24)|
    ((buf[7]&0xff)<<16)|
    ((buf[8]&0xff)<<8) |
    (buf[9]&0xff);
  
  return ldexp(f, e-16446);
}

static int find_chunk(FILE *in, char *type, unsigned int *len, int endian){
  unsigned int i;
  unsigned char buf[8];

  while(1){
    if(fread(buf,1,8,in) <8)return 0;

    if(endian)
      *len = READ_U32_BE(buf+4);
    else
      *len = READ_U32_LE(buf+4);

    if(memcmp(buf,type,4)){

      if((*len) & 0x1)(*len)++;
      
      for(i=0;i<*len;i++)
	if(fgetc(in)==EOF)return 0;

    }else return 1;
  }
}

int input_load(void){

  int stdinp=0,i,fi;
  if(inputs==0){
    /* look at stdin... is it a file, pipe, tty...? */
    if(isatty(STDIN_FILENO)){
      fprintf(stderr,
	      "Spectrum requires either an input file on the command line\n"
	      "or stream data piped|redirected to stdin. spectrum -h will\n"
	      "give more details.\n");
      return 1;
    }
    stdinp=1;    /* file coming in via stdin */
    inputname[0]=strdup("stdin");
    inputs++;
  }

  for(fi=0;fi<inputs;fi++){

    if(stdinp && fi==0){
      int newfd=dup(STDIN_FILENO);
      f[fi]=fdopen(newfd,"rb");
    }else{
      f[fi]=fopen(inputname[fi],"rb");
    }
    seekable[fi]=0;

    /* Crappy! Use a lib to do this for pete's sake! */
    if(f[fi]){
      char headerid[12];
      off_t filelength;
	
      /* parse header (well, sort of) and get file size */
      seekable[fi]=(fseek(f[fi],0,SEEK_CUR)?0:1);

      if(!seekable[fi]){
	filelength=-1;
      }else{
	fseek(f[fi],0,SEEK_END);
	filelength=ftello(f[fi]);
	fseek(f[fi],0,SEEK_SET);
	global_seekable=1;
      }
      
      fread(headerid,1,12,f[fi]);
      if(!strncmp(headerid,"RIFF",4) && !strncmp(headerid+8,"WAVE",4)){
	unsigned int chunklen;
      
	if(find_chunk(f[fi],"fmt ",&chunklen,0)){
	  int ltype;
	  int lch;
	  int lrate;
	  int lbits;
	  unsigned char *buf=alloca(chunklen);
	
	  fread(buf,1,chunklen,f[fi]);
	
	  ltype = READ_U16_LE(buf); 
	  lch =   READ_U16_LE(buf+2); 
	  lrate = READ_U32_LE(buf+4);
	  lbits = READ_U16_LE(buf+14);
	  
	  if(ltype!=1){
	    fprintf(stderr,"%s:\n\tWAVE file not PCM.\n",inputname[fi]);
	    return 1;
	  }
	      
	  if(bits[fi]==-1)bits[fi]=lbits;
	  if(channels[fi]==-1)channels[fi]=lch;
	  if(signedp[fi]==-1){
	    signedp[fi]=0;
	    if(bits[fi]>8)signedp[fi]=1;
	  }
	  if(bigendian[fi]==-1)bigendian[fi]=0;
	  if(rate[fi]==-1){
	    if(lrate<4000 || lrate>192000){
	      fprintf(stderr,"%s:\n\tSampling rate out of bounds\n",inputname[fi]);
	      return 1;
	    }
	    rate[fi]=lrate;
	  }

	  if(find_chunk(f[fi],"data",&chunklen,0)){
	    off_t pos=ftello(f[fi]);
	    int bytes=(bits[fi]+7)/8;
	    if(seekable[fi])
	      filelength=
		(filelength-pos)/
		(channels[fi]*bytes)*
		(channels[fi]*bytes)+pos;
	    
	    if(chunklen==0UL ||
	       chunklen==0x7fffffffUL || 
	       chunklen==0xffffffffUL){
	      if(filelength==-1){
		length[fi]=-1;
		fprintf(stderr,"%s: Incomplete header; assuming stream.\n",inputname[fi]);
	      }else{
		length[fi]=(filelength-pos)/(channels[fi]*bytes);
		fprintf(stderr,"%s: Incomplete header; using actual file size.\n",inputname[fi]);
	      }
	    }else if(filelength==-1 || chunklen+pos<=filelength){
	      length[fi]=(chunklen/(channels[fi]*bytes));
	      fprintf(stderr,"%s: Using declared file size (%ld).\n",
		      inputname[fi],(long)length[fi]*channels[fi]*bytes);
	      
	    }else{
	      
	      length[fi]=(filelength-pos)/(channels[fi]*bytes);
	      fprintf(stderr,"%s: File truncated; Using actual file size.\n",inputname[fi]);
	    }
	    offset[fi]=ftello(f[fi]);
	  } else {
	    fprintf(stderr,"%s: WAVE file has no \"data\" chunk following \"fmt \".\n",inputname[fi]);
	    return 1;
	  }
	}else{
	  fprintf(stderr,"%s: WAVE file has no \"fmt \" chunk.\n",inputname[fi]);
	  return 1;
	}
      
      }else if(!strncmp(headerid,"FORM",4) && !strncmp(headerid+8,"AIF",3)){
	unsigned int len;
	int aifc=0;
	if(headerid[11]=='C')aifc=1;
	unsigned char *buffer;
	char buf2[8];
	
	int lch;
	int lbits;
	int lrate;
	int bytes;
	
	/* look for COMM */
	if(!find_chunk(f[fi], "COMM", &len,1)){
	  fprintf(stderr,"%s: AIFF file has no \"COMM\" chunk.\n",inputname[fi]);
	  return 1;
	}
	
	if(len < 18 || (aifc && len<22)) {
	  fprintf(stderr,"%s: AIFF COMM chunk is truncated.\n",inputname[fi]);
	  return 1;
	}
	
	buffer = alloca(len);
	
	if(fread(buffer,1,len,f[fi]) < len){
	  fprintf(stderr, "%s: Unexpected EOF in reading AIFF header\n",inputname[fi]);
	  return 1;
	}
	
	lch = READ_U16_BE(buffer);
	lbits = READ_U16_BE(buffer+6);
	lrate = (int)read_IEEE80(buffer+8);
      
	if(bits[fi]==-1)bits[fi]=lbits;
	bytes=(bits[fi]+7)/8;
	if(signedp[fi]==-1)signedp[fi]=1;
	if(rate[fi]==-1){
	  if(lrate<4000 || lrate>192000){
	    fprintf(stderr,"%s:\n\tSampling rate out of bounds\n",inputname[fi]);
	    return 1;
	  }
	  rate[fi]=lrate;
	}
	if(channels[fi]==-1)channels[fi]=lch;
	
	if(bigendian[fi]==-1){
	  if(aifc){
	    if(!memcmp(buffer+18, "NONE", 4)) {
	      bigendian[fi] = 1;
	    }else if(!memcmp(buffer+18, "sowt", 4)) {
	      bigendian[fi] = 0;
	    }else{
	      fprintf(stderr, "%s: Spectrum supports only linear PCM AIFF-C files.\n",inputname[fi]);
	      return 1;
	    }
	  }else
	    bigendian[fi] = 1;
	}
	if(!find_chunk(f[fi], "SSND", &len, 1)){
	  fprintf(stderr,"%s: AIFF file has no \"SSND\" chunk.\n",inputname[fi]);
	  return 1;
	}
	
	if(fread(buf2,1,8,f[fi]) < 8){
	  fprintf(stderr,"%s: Unexpected EOF reading AIFF header\n",inputname[fi]);
	  return 1;
	}
	
	{
	  int loffset = READ_U32_BE(buf2);
	  int lblocksize = READ_U32_BE(buf2+4);
	  
	  /* swallow some data */
	  for(i=0;i<loffset;i++)
	    if(fgetc(f[fi])==EOF)break;
	  
	  if( lblocksize == 0 && (bits[fi] == 24 || bits[fi] == 16 || bits[fi] == 8)){
	    
	    off_t pos=ftello(f[fi]);
	    
	    if(seekable[fi])
	      filelength=
		(filelength-pos)/
		(channels[fi]*bytes)*
		(channels[fi]*bytes)+pos;
	  
	    if(len==0UL ||
	       len==0x7fffffffUL || 
	       len==0xffffffffUL){
	      if(filelength==-1){
		length[fi]=-1;
		fprintf(stderr,"%s: Incomplete header; assuming stream.\n",inputname[fi]);
	      }else{
		length[fi]=(filelength-pos)/(channels[fi]*bytes);
		fprintf(stderr,"%s: Incomplete header; using actual file size.\n",inputname[fi]);
	      }
	    }else if(filelength==-1 || (len+pos-loffset-8)<=filelength){
	      length[fi]=((len-loffset-8)/(channels[fi]*bytes));
	      fprintf(stderr,"%s: Using declared file size.\n",inputname[fi]);
	      
	    }else{
	      length[fi]=(filelength-pos)/(channels[fi]*bytes);
	      fprintf(stderr,"%s: File truncated; Using actual file size.\n",inputname[fi]);
	    }
	    offset[fi]=pos;
	  }else{
	    fprintf(stderr, "%s: Spectrum supports only linear PCM AIFF-C files.\n",inputname[fi]);
	    return 1;
	  }
	}
      } else {
	/* must be raw input */
	fprintf(stderr,"Input has no header; assuming raw stream/file.\n");
      
	if(channels[fi]==-1)channels[fi]=1;
	if(rate[fi]==-1)rate[fi]=44100;
	if(bits[fi]==-1)bits[fi]=16;
	if(signedp[fi]==-1)signedp[fi]=1;
	if(bigendian[fi]==-1)bigendian[fi]=host_is_big_endian();
      
	offset[fi]=0;
	length[fi]=-1;
	if(seekable[fi])length[fi]=filelength/(channels[fi]*((bits[fi]+7)/8));
	
	memcpy(readbuffer[fi],headerid,12);
	readbufferfill[fi]=12;
	
      }

      /* select the full-block slice size: ~10fps */
      blockslice[fi]=rate[fi]/10;
      while(blockslice[fi]>blocksize/2)blockslice[fi]/=2;
      total_ch += channels[fi];

      if(length[fi]!=-1)
	bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
      
    }else{
      fprintf(stderr,"Unable to open %s: %s\n",inputname[fi],strerror(errno));
      exit(1);
    }
  }

  blockbuffer=malloc(total_ch*sizeof(*blockbuffer));
  process_work=calloc(blocksize+2,sizeof(*process_work));
  feedback_count=calloc(total_ch,sizeof(*feedback_count));
  feedback_work=calloc(total_ch,sizeof(*feedback_work));

  feedback_acc=malloc(total_ch*sizeof(*feedback_acc));
  feedback_max=malloc(total_ch*sizeof(*feedback_max));
  feedback_instant=malloc(total_ch*sizeof(*feedback_instant));

  ph_acc=malloc(total_ch*sizeof(*ph_acc));
  ph_max=malloc(total_ch*sizeof(*ph_max));
  ph_instant=malloc(total_ch*sizeof(*ph_instant));
  
  freqbuffer=fftwf_malloc((blocksize+2)*sizeof(*freqbuffer));
  for(i=0;i<total_ch;i++){
    blockbuffer[i]=calloc(blocksize,sizeof(**blockbuffer));

    feedback_acc[i]=calloc(blocksize/2+1,sizeof(**feedback_acc));
    feedback_max[i]=calloc(blocksize/2+1,sizeof(**feedback_max));
    feedback_instant[i]=calloc(blocksize/2+1,sizeof(**feedback_instant));

    ph_acc[i]=calloc(blocksize+2,sizeof(**ph_acc));
    ph_max[i]=calloc(blocksize+2,sizeof(**ph_max));
    ph_instant[i]=calloc(blocksize+2,sizeof(**ph_instant));
  }
  
  plan=fftwf_plan_dft_r2c_1d(blocksize,freqbuffer,
			     (fftwf_complex *)freqbuffer,
			     FFTW_ESTIMATE);
  
  /* construct proper window (sin^4 I'd think) */
  window = calloc(blocksize,sizeof(*window));
  for(i=0;i<blocksize;i++)window[i]=sin(M_PIl*i/blocksize);
  for(i=0;i<blocksize;i++)window[i]*=window[i];
  for(i=0;i<blocksize;i++)window[i]=sin(window[i]*M_PIl*.5);
  for(i=0;i<blocksize;i++)window[i]*=window[i]/(blocksize/4)*.778;
    
  return 0;

}

/* Convert new data from readbuffer into the blockbuffers until the
   blockbuffer is full */
static void LBEconvert(void){
  float scale=1./2147483648.;
  int ch=0,fi;

  for(fi=0;fi<inputs;fi++){
    int bytes=(bits[fi]+7)/8;
    int j;
    int32_t xor=(signedp[fi]?0:0x80000000UL);
    
    int readlimit=(readbufferfill[fi]-readbufferptr[fi])/
      channels[fi]/bytes*channels[fi]*bytes+readbufferptr[fi];

    int bfill = blockbufferfill[fi];
    int rptr = readbufferptr[fi];
    unsigned char *rbuf = readbuffer[fi];

    if(readlimit){
      
      switch(bytes){
      case 1:
	
	while(bfill<blocksize && rptr<readlimit){
	  for(j=ch;j<channels[fi]+ch;j++)
	    blockbuffer[j][bfill]=((rbuf[rptr++]<<24)^xor)*scale;
	  bfill++;
	}
	break;
	
      case 2:
      
	if(bigendian[fi]){
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+1]<<16)| (rbuf[rptr]<<24))^xor)*scale;
	      rptr+=2;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr]<<16)| (rbuf[rptr+1]<<24))^xor)*scale;
	      rptr+=2;
	    }
	    bfill++;
	  }
	}
	break;
	
      case 3:
	
	if(bigendian[fi]){
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+2]<<8)|(rbuf[rptr+1]<<16)|(rbuf[rptr]<<24))^xor)*scale;
	      rptr+=3;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr]<<8)|(rbuf[rptr+1]<<16)|(rbuf[rptr+2]<<24))^xor)*scale;
	      rptr+=3;
	    }
	    bfill++;
	  }
	}
	break;
      case 4:
	
	if(bigendian[fi]){
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+3])|(rbuf[rptr+2]<<8)|(rbuf[rptr+1]<<16)|(rbuf[rptr+3]<<24))^xor)*scale;
	      rptr+=4;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<blocksize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr])|(rbuf[rptr+1]<<8)|(rbuf[rptr+2]<<16)|(rbuf[rptr+3]<<24))^xor)*scale;
	      rptr+=4;
	    }
	    bfill++;
	  }
	}
	break;
      }
    }
    ch+=channels[fi];
    blockbufferfill[fi]=bfill;
    readbufferptr[fi]=rptr;    
  }
}

/* when we return, the blockbuffer is full or we're at EOF */
/* EOF cases: 
     loop set: return EOF if all seekable streams have hit EOF
     loop unset: return EOF if all streams have hit EOF
   pad individual EOF streams out with zeroes until global EOF is hit  */

static int input_read(void){
  int i,fi,ch=0;
  int eof=1;
  int notdone=1;

  for(fi=0;fi<inputs;fi++){
    
    /* shift according to slice */
    if(blockbufferfill[fi]==blocksize){
      if(blockslice[fi]<blocksize){
	for(i=0;i<channels[fi];i++)
	  memmove(blockbuffer[i+ch],blockbuffer[i+ch]+blockslice[fi],
		  (blocksize-blockslice[fi])*sizeof(**blockbuffer));
	blockbufferfill[fi]-=blockslice[fi];
      }else
	blockbufferfill[fi]=0;
    }
    ch+=channels[fi];
  }

  while(notdone){
    notdone=0;

    /* if there's data left to be pulled out of a readbuffer, do that */
    LBEconvert();
    
    ch=0;
    for(fi=0;fi<inputs;fi++){
      if(blockbufferfill[fi]!=blocksize){
	
	/* shift the read buffer before fill if there's a fractional
	   frame in it */
	if(readbufferptr[fi]!=readbufferfill[fi] && readbufferptr[fi]>0){
	  memmove(readbuffer[fi],readbuffer[fi]+readbufferptr[fi],
		  (readbufferfill[fi]-readbufferptr[fi])*sizeof(**readbuffer));
	  readbufferfill[fi]-=readbufferptr[fi];
	  readbufferptr[fi]=0;
	}else{
	  readbufferfill[fi]=0;
	  readbufferptr[fi]=0;
	}
	
	/* attempt to top off the readbuffer */
	{
	  long actually_readbytes=0,readbytes=readbuffersize-readbufferfill[fi];

	  if(readbytes>0)
	    actually_readbytes=fread(readbuffer[fi]+readbufferfill[fi],1,readbytes,f[fi]);
	    
	  if(actually_readbytes<0){
	    fprintf(stderr,"Input read error from %s: %s\n",inputname[fi],strerror(errno));
	  }else if (actually_readbytes==0){
	    /* don't process any partially-filled blocks; the
	       stairstep at the end could pollute results badly */
	    
	    memset(readbuffer[fi],0,readbuffersize);
	    bytesleft[fi]=0;
	    readbufferfill[fi]=0;
	    readbufferptr[fi]=0;
	    blockbufferfill[fi]=0;
	  
	  }else{
	    bytesleft[fi]-=actually_readbytes;
	    readbufferfill[fi]+=actually_readbytes;
	    
	    /* conditionally clear global EOF */
	    if(acc_loop){
	      if(seekable[fi])eof=0;
	    }else{
	      eof=0;
	    }
	    notdone=1;
	  }
	}
      }
      ch += channels[fi];
    }
  }
  return eof;
}

void rundata_clear(){
  int i,j;
  for(i=0;i<total_ch;i++){
    feedback_count[i]=0;
    memset(feedback_acc[i],0,(blocksize/2+1)*sizeof(**feedback_acc));
    memset(feedback_max[i],0,(blocksize/2+1)*sizeof(**feedback_max));
    memset(feedback_instant[i],0,(blocksize/2+1)*sizeof(**feedback_instant));

    for(j=0;j<blocksize+2;j++){
      ph_acc[i][j]=0;
      ph_max[i][j]=0;
      ph_instant[i][j]=0;
    }
  }
  acc_clear=0;
}

/* return 0 on EOF, 1 otherwise */
static int process(){
  int fi,i,j,ch;
  int eof_all;

  /* for each file, FOR SCIENCE! */
  for(fi=0;fi<inputs;fi++){
    if(acc_rewind && seekable[fi]){

      blockbufferfill[fi]=0;
      readbufferptr[fi]=0;
      readbufferfill[fi]=0;
      fseek(f[fi],offset[fi],SEEK_SET);
      if(length[fi]!=-1)bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
    }
  }

  eof_all=input_read();

  if(eof_all){
    if(acc_loop && !acc_rewind){
      acc_rewind=1;
      return process();
    } else {
      acc_rewind=0;
      return 0;
    }
  }
  acc_rewind=0;

  if(acc_clear)
    rundata_clear();

  /* by channel */
  ch=0;
  for(fi=0;fi<inputs;fi++){
    if(blockbufferfill[fi]){
      for(i=ch;i<ch+channels[fi];i++){
	
	float *data=blockbuffer[i];

	/* window the blockbuffer into the FFT buffer */
	for(j=0;j<blocksize;j++){
	  freqbuffer[j]=data[j]*window[j];
	}
	
	/* transform */
	fftwf_execute(plan);
	
	pthread_mutex_lock(&feedback_mutex);

	/* perform desired accumulations */
	for(j=0;j<blocksize+2;j+=2){
	  float R = freqbuffer[j];
	  float I = freqbuffer[j+1];
	  float sqR = R*R;
	  float sqI = I*I;
	  float sqM = sqR+sqI;

	  /* deal with phase accumulate/rotate */
	  if(i==ch){
	    /* normalize/store ref for later rotation */
	    process_work[j] = R;
	    process_work[j+1] = -I;

	  }else{
	    /* rotate signed square phase according to ref for phase calculation */
	    float pR;
	    float pI;
	    float rR = process_work[j];
	    float rI = process_work[j+1];
	    pR = (rR*R - rI*I);
	    pI = (rR*I + rI*R);

	    ph_instant[i][j]=pR;
	    ph_instant[i][j+1]=pI;

	    ph_acc[i][j]+=pR;
	    ph_acc[i][j+1]+=pI;
	    
	    if(feedback_max[i][j>>1]<sqM){
	      ph_max[i][j]=pR;
	      ph_max[i][j+1]=pI;
	    }
	  }
	  
	  feedback_instant[i][j>>1]=sqM;
	  feedback_acc[i][j>>1]+=sqM;
	  
	  if(feedback_max[i][j>>1]<sqM)
	    feedback_max[i][j>>1]=sqM;
	  
	}
	feedback_count[i]++;
       
	pthread_mutex_unlock(&feedback_mutex);
      }
    }
    ch+=channels[fi];
  }
  feedback_increment++;
  write(eventpipe[1],"",1);
  return 1;
}

void *process_thread(void *dummy){
  while(!process_exit && process());
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

void process_dump(int mode){
  int fi,i,j,ch;
  FILE *out;

  {   
    out=fopen("accumulate.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_acc[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("max.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_max[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("instant.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_instant[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("accphase.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){

      /* phase */ 
      for(i=0;i<blocksize+2;i+=2){
	fprintf(out,"%f ",(double)i*.5*rate[fi]/blocksize);
	fprintf(out,"%f ",atan2(ph_acc[ch+1][i+1],ph_acc[ch+1][i])*57.29);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

}

/* how many bins to 'trim' off the edge of calculated data when we
   know we've hit a boundary of marginal measurement */
#define binspan 5

float **process_fetch(int res, int scale, int mode, int link, 
		      int *active, int width, 
		      float *ymax, float *pmax, float *pmin){
  int ch,ci,i,j,fi;
  float **data;
  float **ph;

  /* are our scale mappings up to date? */
  if(res != metares || scale != metascale || width != metawidth){
    if(!xmappingL) xmappingL = calloc(inputs, sizeof(*xmappingL));
    if(!xmappingH) xmappingH = calloc(inputs, sizeof(*xmappingH));

    for(fi=0;fi<inputs;fi++){

      /* if mapping preexists, resize it */
      if(xmappingL[fi]){
	xmappingL[fi] = realloc(xmappingL[fi],(width+1)*sizeof(**xmappingL));
      }else{
	xmappingL[fi] = malloc((width+1)*sizeof(**xmappingL));
      }
      if(xmappingH[fi]){
	xmappingH[fi] = realloc(xmappingH[fi],(width+1)*sizeof(**xmappingH));
      }else{
	xmappingH[fi] = malloc((width+1)*sizeof(**xmappingH));
      }

      metascale = scale;
      metawidth = width;
      metares = res;

      
      /* generate new numbers */
      for(i=0;i<width;i++){
	float off=0;
	float loff=1.;
	float hoff=1.;
	float lfreq,hfreq;

	switch(res){
	case 0: /* screen-resolution */
	  off=1.;
	  break;
	case 1: /* 1/24th octave */
	  loff = .95918945710913818816;
	  hoff = 1.04254690518999138632;
	  break;
	case 2: /* 1/12th octave */
	  loff = .94387431268169349664;
	  hoff = 1.05946309435929526455;
	  break;
	case 3: /* 1/3th octave */
	  loff = .79370052598409973738;
	  hoff = 1.25992104989487316475;
	  break;
	}

	switch(scale){
	case 0: /* log */
	  lfreq= pow(10.,(i-off)/(width-1)
		     * (log10(100000.)-log10(5.))
		     + log10(5.)) * loff;
	  hfreq= pow(10.,(i+off)/(width-1)
		     * (log10(100000.)-log10(5.))
		     + log10(5.)) * hoff;
	  break;
	case 1: /* ISO */
	  lfreq= pow(2.,(i-off)/(width-1)
		     * (log2(20000.)-log2(25.))
		     + log2(25.)) * loff;
	  hfreq= pow(2.,(i+off)/(width-1)
		     * (log2(20000.)-log2(25.))
		     + log2(25.)) *hoff;
	  break;
	case 2: /* screen-resolution linear */
	  lfreq=(i-off)*20000./(width-1)*loff;
	  hfreq=(i+off)*20000./(width-1)*hoff;
	  break;
	}

	xmappingL[fi][i]=lfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingH[fi][i]=hfreq/(rate[fi]*.5)*(blocksize/2);

      }
      
      for(i=0;i<width;i++){
	if(xmappingL[fi][i]<0.)xmappingL[fi][i]=0.;
	if(xmappingL[fi][i]>blocksize/2.)xmappingL[fi][i]=blocksize/2.;
	if(xmappingH[fi][i]<0.)xmappingH[fi][i]=0.;
	if(xmappingH[fi][i]>blocksize/2.)xmappingH[fi][i]=blocksize/2.;
      }
    }

    for(i=0;i<total_ch;i++)
      if(feedback_work[i]){
	feedback_work[i] = realloc(feedback_work[i],(width+1)*sizeof(**feedback_work));
      }else{
	feedback_work[i] = malloc((width+1)*sizeof(**feedback_work));
      }
  }
      
  /* mode selects the base data set */
  switch(mode){    
  case 0: /* independent / instant */
    data=feedback_instant;
    ph=ph_instant;
    break;
  case 1: /* independent / max */
    data=feedback_max;
    ph=ph_max;
    break;
  case 2:
    data=feedback_acc;
    ph=ph_acc;
    break;
  }

  ch=0;
  *ymax = -150.;
  *pmax = -180.;
  *pmin = 180.;
  for(fi=0;fi<inputs;fi++){
    float *L = xmappingL[fi];
    float *H = xmappingH[fi];

    switch(link){
    case LINK_INDEPENDENT:
      
      for(ci=0;ci<channels[fi];ci++){
	float *y = feedback_work[ci+ch];
	float *m = data[ci+ch];
	if(active[ch+ci]){
	  for(i=0;i<width;i++){
	    int first=floor(L[i]);
	    int last=floor(H[i]);
	    float sum;
	    
	    if(first==last){
	      float del=H[i]-L[i];
	      sum=m[first]*del;
	    }else{
	      float del=1.-(L[i]-first);
	      sum=m[first]*del;
	      
	      for(j=first+1;j<last;j++)
		sum+=m[j];
	      
	      del=(H[i]-last);
	      sum+=m[last]*del;
	    }

	    sum=todB_a(&sum)*.5;
	    if(sum>*ymax)*ymax=sum;
	    y[i]=sum;	  
	  }
	}
      }
      break;

    case LINK_SUMMED:
      {
	float *y = feedback_work[ch];
	memset(y,0,(width+1)*sizeof(*y));
      
	for(ci=0;ci<channels[fi];ci++){
	  float *m = data[ci+ch];
	  if(active[ch+ci]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      
	      if(first==last){
		float del=H[i]-L[i];
		y[i]+=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		y[i]+=m[first]*del;
		
		for(j=first+1;j<last;j++)
		  y[i]+=m[j];
		
		del=(H[i]-last);
		y[i]+=m[last]*del;
	      }
	    }
	  }
	}
      
	for(i=0;i<width;i++){
	  float sum=todB_a(y+i)*.5;
	  if(sum>*ymax)*ymax=sum;
	  y[i]=sum;	  
	}
      }
      break;
      
    case LINK_IMPEDENCE_p1:
    case LINK_IMPEDENCE_1:
    case LINK_IMPEDENCE_10:
      {
	float shunt = (link == LINK_IMPEDENCE_p1?.1:(link == LINK_IMPEDENCE_1?1:10));
	float *r = feedback_work[ch];

	for(ci=0;ci<channels[fi];ci++){
	  float *y = feedback_work[ci+ch];
	  float *m = data[ch+ci];
	  
	  if(ci==0 || active[ch+ci]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sum;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sum=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		sum=m[first]*del;
		
		for(j=first+1;j<last;j++)
		  sum+=m[j];
		
		del=(H[i]-last);
		sum+=m[last]*del;
	      }

	      if(ci==0){
		/* stash the reference in the work vector */
		r[i]=sum;
	      }else{
		/* the shunt */
		/* 'r' collected at source, 'sum' across the shunt */
		float V=sqrt(r[i]);
		float S=sqrt(sum);
		
		if(S>(1e-5) && V>S){
		  y[i] = shunt*(V-S)/S;
		}else{
		  y[i] = NAN;
		}
	      }
	    }
	  }
	}
	    
	/* scan the resulting buffers for marginal data that would
	   produce spurious output. Specifically we look for sharp
	   falloffs of > 40dB or an original test magnitude under
	   -70dB. */
	{
	  float max = -140;
	  for(i=0;i<width;i++){
	    float v = r[i] = todB_a(r+i)*.5;
	    if(v>max)max=v;
	  }

	  for(ci=1;ci<channels[fi];ci++){
	    if(active[ch+ci]){
	      float *y = feedback_work[ci+ch];	      
	      for(i=0;i<width;i++){
		if(r[i]<max-40 || r[i]<-70){
		  int j=i-binspan;
		  if(j<0)j=0;
		  for(;j<i;j++)
		    y[j]=NAN;
		  for(;j<width;j++){
		    if(r[j]>max-40 && r[j]>-70)break;
		    y[j]=NAN;
		  }
		  i=j+3;
		  for(;j<i && j<width;j++){
		    y[j]=NAN;
		  }
		}
		if(!isnan(y[i]) && y[i]>*ymax)*ymax = y[i];
	      }
	    }
 	  }
	  fprintf(stderr,"ymax=%f\n",*ymax);

	}
      }
      break;

    case LINK_PHASE: /* response/phase */

      if(channels[fi]>=2){
	float *om = feedback_work[ch];
	float *op = feedback_work[ch+1];

	float *r = data[ch];
	float *m = data[ch+1];
	float *p = ph[ch+1];
	float mag[width];

	if(feedback_count[ch]==0){
	  memset(om,0,width*sizeof(*om));
	  memset(op,0,width*sizeof(*op));
	}else{
	  /* two vectors only; response and phase */
	  /* response */
	  if(active[ch] || active[ch+1]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sumR,sumM;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sumR=r[first]*del;
		sumM=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		sumR=r[first]*del;
		sumM=m[first]*del;
		
		for(j=first+1;j<last;j++){
		  sumR+=r[j];
		  sumM+=m[j];
		}

		del=(H[i]-last);
		sumR+=r[last]*del;
		sumM+=m[last]*del;
	      }

	      mag[i] = todB_a(&sumR)*.5;
	      if(active[ch] && sumR > 1e-8){
		sumM /= sumR;
		om[i] = todB_a(&sumM)*.5;
	      }else{
		om[i] = NAN;
	      }
	    }
	  }
	  
	  /* phase */
	  if(active[ch+1]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sumR,sumI;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sumR=p[(first<<1)]*del;
		sumI=p[(first<<1)+1]*del;
	      }else{
		float del=1.-(L[i]-first);
		sumR=p[(first<<1)]*del;
		sumI=p[(first<<1)+1]*del;
		
		for(j=first+1;j<last;j++){
		  sumR+=p[(j<<1)];
		  sumI+=p[(j<<1)+1];
		}

		del=(H[i]-last);
		sumR+=p[(last<<1)]*del;
		sumI+=p[(last<<1)+1]*del;
	      }

	      if(sumR*sumR+sumI*sumI > 1e-16){
		op[i] = atan2(sumI,sumR)*57.29;
	      }else{
		op[i]=NAN;
	      }
	    }
	  }

	  /* scan the resulting buffers for marginal data that would
	     produce spurious output. Specifically we look for sharp
	     falloffs of > 40dB or an original test magnitude under
	     -70dB. */
	  if(active[ch] || active[ch+1]){
	    int max = -140;
	    for(i=0;i<width;i++)
	      if(!isnan(mag[i]) && mag[i]>max)max=mag[i];
	    
	    for(i=0;i<width;i++){
	      if(!isnan(mag[i])){
		if(mag[i]<max-40 || mag[i]<-70){
		  int j=i-binspan;
		  if(j<0)j=0;
		  for(;j<i;j++){
		    om[j]=NAN;
		    op[j]=NAN;
		  }
		  for(;j<width;j++){
		    if(mag[j]>max-40 && mag[j]>-70)break;
		    om[j]=NAN;
		    op[j]=NAN;
		  }
		  i=j+3;
		  for(;j<i && j<width;j++){
		    om[j]=NAN;
		    op[j]=NAN;
		  }
		}
		if(om[i]>*ymax)*ymax = om[i];
		if(!isnan(op[i])){
		  if(op[i]>*pmax)*pmax = op[i];
		  if(op[i]<*pmin)*pmin = op[i];
		}
	      }
	    }
	  }
	}
      }
      break;

    case LINK_THD: /* THD */
      break;
      
    case LINK_THDN: /* THD+N */
      break;
      
    }
    ch+=channels[fi];
  }

  return feedback_work;

}

