/*
 *
 *  gtk2 spectrum analyzer
 *
 *      Copyright (C) 2004-2012 Monty
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "io.h"

#define readsize 512

/* locks access to file metadata and blockbuffer data */
pthread_mutex_t blockbuffer_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

int bits[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int bigendian[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int channels[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int rate[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int signedp[MAX_FILES] = {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};

int total_ch=0;

float **blockbuffer=0;
int blockbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int blockbuffernew[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
/* end list of locked buffers */

int bits_force[MAX_FILES] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int bigendian_force[MAX_FILES] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int channels_force[MAX_FILES] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int rate_force[MAX_FILES] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int signed_force[MAX_FILES] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

extern int blocksize; /* set only at startup */
sig_atomic_t blockslice_frac;
int blockslice_count=0;
static int blockslice_started=0;
static int blockslices[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

/* used to determine the slight sample timing offsets between the
   blockbuffer heads of inputs with different, non-interger-ratio
   sampling rates, or an input that's not a multiple of the requested
   blockslice fraction.  It lists the sample position within this
   second of one-past the last sample read */
int blockslice_cursor[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
/* used to indicate which inputs have reached eof */
int blockslice_eof[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
/* how much the blockbuffer was advanced last read for this input */
int blockslice_adv[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

static unsigned char readbuffer[MAX_FILES][readsize];
static int readbufferfill[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static int readbufferptr[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

static FILE *f[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t offset[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
static off_t length[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
static off_t bytesleft[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};
int seekable[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int isapipe[MAX_FILES]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};
int global_seekable=0;

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

static void close_one_input(int fi){
  int i;
  if(f[fi]){
    fclose(f[fi]);
    f[fi]=NULL;
    blockbufferfill[fi]=0;
    blockbuffernew[fi]=0;
    readbufferptr[fi]=0;
    readbufferfill[fi]=0;
    if(blockbuffer){
      for(i=0;i<total_ch;i++)
        if(blockbuffer[i])free(blockbuffer[i]);
      free(blockbuffer);
      blockbuffer=NULL;
    }
    total_ch -= channels[fi];
    channels[fi]=0;
  }
}

/* used to load or reload an input */
static int load_one_input(int fi){
  int i;

  if(!strcmp(inputname[fi],"/dev/stdin") && fi==0){
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

    if(fread(headerid,1,12,f[fi])==0 && feof(f[fi])){
      close_one_input(fi);
      return 1;
    }
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

        if(!bits_force[fi])bits[fi]=lbits;
        if(!channels_force[fi])channels[fi]=lch;
        if(!signed_force[fi]){
          signedp[fi]=0;
          if(bits[fi]>8)signedp[fi]=1;
        }
        if(!bigendian_force[fi])bigendian[fi]=0;
        if(!rate_force[fi]){
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

      if(!bits_force[fi])bits[fi]=lbits;
      bytes=(bits[fi]+7)/8;
      if(!signed_force[fi])signedp[fi]=1;
      if(!rate_force[fi]){
        if(lrate<4000 || lrate>192000){
          fprintf(stderr,"%s:\n\tSampling rate out of bounds\n",inputname[fi]);
          return 1;
        }
        rate[fi]=lrate;
      }
      if(!channels_force[fi])channels[fi]=lch;

      if(!bigendian_force[fi]){
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

      if(!channels_force[fi])channels[fi]=1;
      if(!rate_force[fi])rate[fi]=44100;
      if(!bits_force[fi])bits[fi]=16;
      if(!signed_force[fi])signedp[fi]=1;
      if(!bigendian_force[fi])bigendian[fi]=host_is_big_endian();

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
    return 1;
  }

  return 0;
}

int input_load(void){
  int fi;
  if(inputs==0){
    /* look at stdin... is it a file, pipe, tty...? */
    if(isatty(STDIN_FILENO)){
      fprintf(stderr,
	      "Input file on the command line or stream data\n"
              "piped|redirected to stdin. -h will give more details.\n");
      return 1;
    }
    inputname[0]=strdup("/dev/stdin");
    inputs++;
  }

  for(fi=0;fi<inputs;fi++){
    struct stat sb;
    isapipe[fi]=0;
    if(!stat(inputname[fi],&sb))
      isapipe[fi]=S_ISFIFO(sb.st_mode);

    if(load_one_input(fi))
      exit(1);
  }

  return 0;
}

/* attempts to reopen a pipe at EOF.  Ignores non-fifo inputs.
   Returns nonzero if a pipe reopens, 0 otherwise */

int pipe_reload(){
  int fi;
  int ret=0;
  for(fi=0;fi<inputs;fi++)
    if(!seekable[fi] && f[fi]==NULL && isapipe[fi] &&
       !load_one_input(fi)){
      fprintf(stderr,"reopened input %d\n",fi);
      ret=1;
    }
  return ret;
}

/* Convert new data from readbuffer into the blockbuffers until the
   blockbuffer is full or the readbuffer is empty */
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
    int tosize = blocksize + blockslices[fi];
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

static void blockslice_init(void){

  /* strict determinism is nice */
  if(!blockslice_started){
    int frac = blockslice_frac;
    int fi;

    blockslice_count=0;
    memset(blockslice_cursor,0,sizeof(blockslice_cursor));
    memset(blockslice_eof,0,sizeof(blockslice_eof));
    blockslice_started = 1;

    for(fi=0;fi<inputs;fi++)
      blockslices[fi] = rate[fi]/frac;
  }
}

/* blockslices are tracked/locked over a one second period */
static void blockslice_advance(void){
  int fi;
  int frac = blockslice_frac;
  int count = blockslice_count + (1000000/frac);

  blockslice_count = count;
  count += (1000000/frac);

  for(fi=0;fi<inputs;fi++){
    int nextsample = rint((double)rate[fi]*count/1000000);

    blockslice_cursor[fi] += blockslice_adv[fi];
    if(blockslice_adv[fi] < blockslices[fi])
      blockslice_eof[fi]=1;
    else
      blockslices[fi] = nextsample - blockslice_cursor[fi];
    if(blockslice_cursor[fi] >= rate[fi]) blockslice_cursor[fi]-=rate[fi];

  }
  if(blockslice_count>=1000000)blockslice_count-=1000000;
}

/* input_read returns:
   -1 if a pipe hits EOF
    0 if all at EOF and nothing to process
    1 if new data ready to process

   All access to blockbuffer[0,blocksize) is locked, as is the
   blockbuffer metadata
*/

int input_read(int loop, int partialok){
  int i,j,fi,ch;
  int notdone=1;
  int ret=0;
  int rewound[total_ch];
  memset(rewound,0,sizeof(rewound));

  blockslice_init();

  pthread_mutex_lock(&blockbuffer_mutex);
  if(blockbuffer==0){
    blockbuffer=malloc(total_ch*sizeof(*blockbuffer));

    for(i=0;i<total_ch;i++){
      blockbuffer[i]=calloc(blocksize*2,sizeof(**blockbuffer));
    }
  }

  /* if this is first frame, do we allow a single slice or fully
     fill the buffer */
  ch=0;
  for(fi=0,ch=0;fi<inputs;fi++){
    if(blockbufferfill[fi]==0){
      for(i=ch;i<channels[fi]+ch;i++)
        for(j=0;j<blocksize;j++)
          blockbuffer[i][j]=NAN;
      if(partialok){
        blockbufferfill[fi]=blocksize;
      }else{
        blockbufferfill[fi]=blockslices[fi];
      }
    }
  }
  pthread_mutex_unlock(&blockbuffer_mutex);

  while(notdone){
    notdone=0;

    /* if there's data left to be pulled out of a readbuffer, do
       that */
    /* not locked: new data is either fed into a yet-inactive
       buffer or to the end (past the blocksize) of an active
       buffer */
    LBEconvert();

    /* fill readbuffers */
    for(fi=0;fi<inputs;fi++){

      /* drain what's already been processed before fill */
      if(readbufferptr[fi]>0){
        if(readbufferptr[fi]!=readbufferfill[fi]){
          /* partial frame; shift it */
          memmove(readbuffer[fi],readbuffer[fi]+readbufferptr[fi],
                  (readbufferfill[fi]-readbufferptr[fi]));
          readbufferfill[fi]-=readbufferptr[fi];
          readbufferptr[fi]=0;
        }else{
          /* all processed, just empty */
          readbufferfill[fi]=0;
          readbufferptr[fi]=0;
        }
      }

      /* attempt to top off the readbuffer if the given blockbuffer
         isn't full */
      if(readbufferfill[fi]<readsize &&
         blockbufferfill[fi]<blocksize+blockslices[fi]){
        int readbytes=readsize-readbufferfill[fi];
        int actually_readbytes =
          fread(readbuffer[fi]+readbufferfill[fi],1,readbytes,f[fi]);

        if(actually_readbytes<=0 && ferror(f[fi])){
          fprintf(stderr,"Input read error from %s: %s\n",
                  inputname[fi],strerror(ferror(f[fi])));
        }else if (actually_readbytes==0){
          if(isapipe[fi] && !seekable[fi]){
            /* attempt to reopen a pipe immediately; kick out */
            close_one_input(fi);
            return -1;
          }
          if(loop && seekable[fi] && (!rewound[fi] || readbufferfill[fi])){
            /* real, hard EOF/error in a partially filled block */
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
    }
  }

  /* at this point, all buffers are fully topped off or at eof */
  pthread_mutex_lock(&blockbuffer_mutex);

  /* shift any updated blockbuffers and mark them new */
  for(fi=0,ch=0;fi<inputs;fi++){
    if(blockbufferfill[fi]>=blocksize+(partialok?1:blockslices[fi])){
      for(i=ch;i<channels[fi]+ch;i++)
        memmove(blockbuffer[i],blockbuffer[i]+
                (blockbufferfill[fi]-blocksize),
                blocksize*sizeof(**blockbuffer));

      blockslice_adv[fi] = blockbufferfill[fi]-blocksize;

      blockbufferfill[fi]=blocksize;
      blockbuffernew[fi]=1;
      ret=1;
    }else{
      blockslice_adv[fi]=0;
    }
  }
  blockslice_advance();

  return ret;
}

int rewind_files(){
  int fi;

  for(fi=0;fi<inputs;fi++){
    if(seekable[fi]){
      blockbufferfill[fi]=0;
      blockbuffernew[fi]=0;
      readbufferptr[fi]=0;
      readbufferfill[fi]=0;
      fseek(f[fi],offset[fi],SEEK_SET);
      if(length[fi]!=-1)bytesleft[fi]=length[fi]*channels[fi]*((bits[fi]+7)/8);
    }
  }

  blockslice_started=0;
  return 0;
}
