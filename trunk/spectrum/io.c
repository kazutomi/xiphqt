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

#include "io.h"

int bits[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int bigendian[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int channels[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int rate[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int signedp[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};

extern sig_atomic_t acc_loop;
extern int blocksize;
int blockslice[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};

float **blockbuffer=0;
int blockbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

static unsigned char readbuffer[MAX_FILES][readbuffersize];
static int readbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static int readbufferptr[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

static FILE *f[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t offset[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t length[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
static off_t bytesleft[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int seekable[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int global_seekable=0;
int total_ch=0;

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

          /* Add cooked WAVE_FORMAT_EXTENSIBLE support */
          if(ltype == 65534){
            int cbSize = READ_U16_LE(buf+16);
            if(cbSize>=22)
              ltype = READ_U16_LE(buf + 24); 
	  }

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

      if(length[fi]!=-1)
	bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
      total_ch += channels[fi];

    }else{
      fprintf(stderr,"Unable to open %s: %s\n",inputname[fi],strerror(errno));
      exit(1);
    }
  }

  return 0;
}

/* Convert new data from readbuffer into the blockbuffers until the
   blockbuffer is full */
static void LBEconvert(){
  float scale=1./2147483648.;
  int ch=0,fi;

  for(fi=0;fi<inputs;fi++){
    int bytes=(bits[fi]+7)/8;
    int j;
    int32_t xor=(signedp[fi]?0:0x80000000UL);
    
    int readlimit=(readbufferfill[fi]-readbufferptr[fi])/
      channels[fi]/bytes*channels[fi]*bytes+readbufferptr[fi];

    int bfill = blockbufferfill[fi];
    int tosize = blocksize + blockslice[fi];
    int rptr = readbufferptr[fi];
    unsigned char *rbuf = readbuffer[fi];

    if(readlimit){
      
      switch(bytes){
      case 1:
	
	while(bfill<tosize && rptr<readlimit){
	  for(j=ch;j<channels[fi]+ch;j++)
	    blockbuffer[j][bfill]=((rbuf[rptr++]<<24)^xor)*scale;
	  bfill++;
	}
	break;
	
      case 2:
      
	if(bigendian[fi]){
	  while(bfill<tosize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+1]<<16)| (rbuf[rptr]<<24))^xor)*scale;
	      rptr+=2;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<tosize && rptr<readlimit){
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
	  while(bfill<tosize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+2]<<8)|(rbuf[rptr+1]<<16)|(rbuf[rptr]<<24))^xor)*scale;
	      rptr+=3;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<tosize && rptr<readlimit){
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
	  while(bfill<tosize && rptr<readlimit){
	    for(j=ch;j<channels[fi]+ch;j++){
	      blockbuffer[j][bfill]=
		(((rbuf[rptr+3])|(rbuf[rptr+2]<<8)|(rbuf[rptr+1]<<16)|(rbuf[rptr+3]<<24))^xor)*scale;
	      rptr+=4;
	    }
	    bfill++;
	  }
	}else{
	  while(bfill<tosize && rptr<readlimit){
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

int input_read(int loop, int partialok){
  int i,j,fi,ch=0;
  int eof=1;
  int notdone=1;
  int rewound[total_ch];
  memset(rewound,0,sizeof(rewound));

  if(blockbuffer==0){
    blockbuffer=malloc(total_ch*sizeof(*blockbuffer));

    for(i=0;i<total_ch;i++){
      blockbuffer[i]=calloc(blocksize*2,sizeof(**blockbuffer));
    }
  }

  /* if this is first frame, do we allow a single slice or fully
     fill the buffer */
  for(fi=0;fi<inputs;fi++){
    if(blockbufferfill[fi]==0){
      for(i=ch;i<channels[fi]+ch;i++)
        for(j=0;j<blocksize;j++)
          blockbuffer[i][j]=NAN;
      if(partialok){
        blockbufferfill[fi]=blocksize;
      }else{
        blockbufferfill[fi]=blockslice[fi];
      }
    }
  }

  /* try to fill buffer to blocksize+slice before performing shift */

  while(notdone){
    notdone=0;

    /* if there's data left to be pulled out of a readbuffer, do that */
    LBEconvert();

    ch=0;
    for(fi=0;fi<inputs;fi++){
      if(blockbufferfill[fi]!=blocksize+blockslice[fi]){

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

	  if(actually_readbytes<=0 && ferror(f[fi])){
	    fprintf(stderr,"Input read error from %s: %s\n",
                    inputname[fi],strerror(ferror(f[fi])));
	  }else if (actually_readbytes==0){
            /* real, hard EOF/error in a partially filled block */
            if(!partialok){
              /* partial frame is *not* ok.  zero it out. */
              memset(readbuffer[fi],0,readbuffersize);
              bytesleft[fi]=0;
              readbufferfill[fi]=0;
              readbufferptr[fi]=0;
              blockbufferfill[fi]=0;
            }
            if(loop && (!rewound[fi] || (partialok && blockbufferfill[fi]))){
              /* rewind this file and continue */
              fseek(f[fi],offset[fi],SEEK_SET);
              if(length[fi]!=-1)
                bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
              notdone=1;
              rewound[fi]=1;
            }
	  }else{
            /* got a read */
	    bytesleft[fi]-=actually_readbytes;
	    readbufferfill[fi]+=actually_readbytes;
            notdone=1;
	  }
	}
      }else{
        eof=0;
      }
      ch += channels[fi];
    }
  }

  /* shift */
  for(fi=0,ch=0;fi<inputs;fi++){
    if(blockbufferfill[fi]>blocksize){
      for(i=ch;i<channels[fi]+ch;i++)
        memmove(blockbuffer[i],blockbuffer[i]+(blockbufferfill[fi]-blocksize),
                blocksize*sizeof(**blockbuffer));
      blockbufferfill[fi]=blocksize;
    }
    for(i=ch;i<channels[fi]+ch;i++)
      for(j=blockbufferfill[fi];j<blocksize;j++)
        blockbuffer[i][j]=NAN;
    ch+=channels[fi];
  }

  return eof;
}

int rewind_files(){
  int fi;
  for(fi=0;fi<inputs;fi++){
    if(!seekable[fi])
      return 1;
  }

  for(fi=0;fi<inputs;fi++){
    blockbufferfill[fi]=0;
    readbufferptr[fi]=0;
    readbufferfill[fi]=0;
    fseek(f[fi],offset[fi],SEEK_SET);
    if(length[fi]!=-1)bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
  }
  return 0;
}
