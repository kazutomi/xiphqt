/*
 *
 *  squishyio
 *
 *      Copyright (C) 2010 Xiph.Org
 *
 *  squishyball is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  squishyball is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with rtrecord; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <ao/ao.h>
#include <vorbis/vorbisfile.h>
#include <FLAC/stream_decoder.h>
#include <unistd.h>
#include "squishyio.h"

#define rbsize 4096
static unsigned char rawbuf[rbsize];

static inline int host_is_big_endian() {
  union {
    int32_t pattern;
    unsigned char bytewise[4];
  } m;
  m.pattern = 0xfeedface; /* deadbeef */
  if (m.bytewise[0] == 0xfe) return 1;
  return 0;
}

static char *trim_path(const char *in){
  /* search back to first /, \, or : */
  if(in){
    char *a = strrchr(in,'/');
    char *b = strrchr(in,'\\');
    char *c = strrchr(in,':');
    int posa = (a ? a-in+1 : 0);
    int posb = (b ? b-in+1 : 0);
    int posc = (c ? c-in+1 : 0);
    if(posb>posa)posa=posb;
    if(posc>posa)posa=posc;
    return (char *)in+posa;
  }
  return NULL;
}

typedef struct{
  int (*id_func)(const char *path,unsigned char *buf);
  pcm_t *(*load_func)(const char *path, FILE *in);
  char *format;
} input_format;

/* steal/simplify/expand file ID/load code from oggenc */

/* Macros to read header data */
#define READ_U32_LE(buf) \
    (((buf)[3]<<24)|((buf)[2]<<16)|((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U16_LE(buf) \
    (((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U32_BE(buf) \
    (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|((buf)[3]&0xff))

#define READ_U16_BE(buf) \
    (((buf)[0]<<8)|((buf)[1]&0xff))

static int wav_id(const char *path,unsigned char *buf){
  if(memcmp(buf, "RIFF", 4))
    return 0; /* Not wave */
  if(memcmp(buf+8, "WAVE",4))
    return 0; /* RIFF, but not wave */
  return 1;
}

static int aiff_id(const char *path,unsigned char *buf){
  if(memcmp(buf, "FORM", 4))
    return 0;
  if(memcmp(buf+8, "AIF",3))
    return 0;
  if(buf[11]!='C' && buf[11]!='F')
    return 0;
  return 1;
}

static int flac_id(const char *path,unsigned char *buf){
  return memcmp(buf, "fLaC", 4) == 0;
}

static int oggflac_id(const char *path,unsigned char *buf){
  return memcmp(buf, "OggS", 4) == 0 &&
    (memcmp (buf+28, "\177FLAC", 5) == 0 ||
     flac_id(path,buf+28));
}

static int vorbis_id(const char *path,unsigned char *buf){
  return memcmp(buf, "OggS", 4) == 0 &&
    memcmp (buf+28, "\x01vorbis", 7) == 0;
}

static int sw_id(const char *path,unsigned char *buf){
  /* if all else fails, look for JM's favorite extension */
  return memcmp(path+strlen(path)-3,".sw",3)==0;
}

static int read_raw_samples(pcm_t *pcm, FILE *in, int s8, int bep){
  union {
    float f;
    unsigned char c[4];
  } m;

  int i;
  int bytesper,samplesper;
  off_t sofar=0;
  int bps=(abs(pcm->savebits)+7)>>3;
  int cpf=pcm->ch;
  int bpf=bps*cpf;

  pcm->data = calloc(pcm->ch,sizeof(*pcm->data));
  if(pcm->data == NULL){
    fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
    goto err;
  }
  for(i=0;i<pcm->ch;i++){
    pcm->data[i] = calloc(pcm->samples,sizeof(**pcm->data));
    if(pcm->data[i] == NULL){
      fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
      goto err;
    }
  }

  samplesper = rbsize/bpf;
  bytesper = samplesper*bpf;

  while(sofar<pcm->samples){
    unsigned char *d=rawbuf;
    off_t bytes, samples=samplesper;
    int i,j;
    if((pcm->samples-sofar)<samples)samples=pcm->samples-sofar;
    bytes = samples*bpf;

    bytes=fread(d,1,bytes,in);
    if(bytes==0)break;

    switch(pcm->savebits){
    case 8:
      if(s8){
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = ((ogg_int16_t)(*d)<<8)*(1.f/32768.f);
            d++;
          }
          sofar++;
        }
      }else{
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = (*d-128)*(1.f/128.f);
            d++;
          }
          sofar++;
        }
      }
      break;
    case 16:
      if(bep){
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = ((ogg_int16_t)((d[0]<<8)|(d[1])))*(1.f/32768.f);
            d+=2;
          }
          sofar++;
        }
      }else{
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = ((ogg_int16_t)((d[1]<<8)|(d[0])))*(1.f/32768.f);
            d+=2;
          }
          sofar++;
        }
      }
      break;
    case 24:
      if(bep){
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = ((int)((d[0]<<24)|(d[1]<<16)|(d[2]<<8))>>8)*(1.f/8388608.f);
            d+=3;
          }
          sofar++;
        }
      }else{
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            pcm->data[j][sofar] = ((int)((d[2]<<24)|(d[1]<<16)|(d[0]<<8))>>8)*(1.f/8388608.f);
            d+=3;
          }
          sofar++;
        }
      }
      break;
    case -32:
      if((!bep)==(!host_is_big_endian())){
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            m.c[0]=d[0];
            m.c[1]=d[1];
            m.c[2]=d[2];
            m.c[3]=d[3];
            d+=4;
            pcm->data[j][sofar] = m.f;
          }
          sofar++;
        }
      }else{
        for(i=0;i<samples;i++){
          for(j=0;j<cpf;j++){
            m.c[0]=d[3];
            m.c[1]=d[2];
            m.c[2]=d[1];
            m.c[3]=d[0];
            d+=4;
            pcm->data[j][sofar] = m.f;
          }
          sofar++;
        }
      }
      break;
    default:
      fprintf(stderr,"Unsupported input bit depth\n");
      goto err;
    }
  }

  if(sofar<pcm->samples){
    fprintf(stderr,"Input file ended before declared length (%ld < %ld samples); continuing...\n",(long)sofar,(long)pcm->samples);
    pcm->samples=sofar;
  }

  if(pcm->savebits==8)pcm->savebits=16;

  return 0;
 err:
  free_pcm(pcm);
  return 1;
}



/* WAV file support ***********************************************************/

static int find_wav_chunk(FILE *in, const char *path, char *type, unsigned int *len){
  unsigned char buf[8];

  while(1){
    if(fread(buf,1,8,in) < 8){
      fprintf(stderr, "%s: Unexpected EOF in reading WAV header\n",path);
      return 0; /* EOF before reaching the appropriate chunk */
    }

    if(memcmp(buf, type, 4)){
      *len = READ_U32_LE(buf+4);
      if(fseek(in, *len, SEEK_CUR))
        return 0;

      buf[4] = 0;
    }else{
      *len = READ_U32_LE(buf+4);
      return 1;
    }
  }
}

static pcm_t *wav_load(const char *path, FILE *in){
  unsigned char buf[40];
  unsigned int len;
  pcm_t *pcm = NULL;
  int i;

  if(fseek(in,12,SEEK_SET)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }

  pcm = calloc(1,sizeof(pcm_t));
  pcm->name=strdup(trim_path(path));

  if(!find_wav_chunk(in, path, "fmt ", &len)){
    fprintf(stderr,"%s: Failed to find fmt chunk in WAV file\n",path);
    goto err;
  }

  if(len < 16){
    fprintf(stderr, "%s: Unrecognised format chunk in WAV header\n",path);
    goto err;
  }

  /* A common error is to have a format chunk that is not 16, 18 or
   * 40 bytes in size.  This is incorrect, but not fatal, so we only
   * warn about it instead of refusing to work with the file.
   * Please, if you have a program that's creating format chunks of
   * sizes other than 16 or 18 bytes in size, report a bug to the
   * author.
   */
  if(len!=16 && len!=18 && len!=40)
    fprintf(stderr,
            "%s: INVALID format chunk in WAV header.\n"
            " Trying to read anyway (may not work)...\n",path);

  if(len>40)len=40;

  if(fread(buf,1,len,in) < len){
    fprintf(stderr,"%s: Unexpected EOF in reading WAV header\n",path);
    goto err;
  }

  unsigned int mask = 0;
  unsigned int format =      READ_U16_LE(buf);
  unsigned int channels =    READ_U16_LE(buf+2);
  unsigned int samplerate =  READ_U32_LE(buf+4);
  //unsigned int bytespersec = READ_U32_LE(buf+8);
  unsigned int align =       READ_U16_LE(buf+12);
  unsigned int samplesize =  READ_U16_LE(buf+14);
  const char *mask_map[32]={
    "L","R","C","LFE", "BL","BR","CL","CR",
    "BC","SL","SR","X", "X","X","X","X",
    "X","X","X","X", "X","X","X","X",
    "X","X","X","X", "X","X","X","X"};

  if(format == 0xfffe){ /* WAVE_FORMAT_EXTENSIBLE */

    if(len<40){
      fprintf(stderr,"%s: Extended WAV format header invalid (too small)\n",path);
      goto err;
    }

    mask = READ_U32_LE(buf+20);
    format = READ_U16_LE(buf+24);
  }

  if(mask==0){
    switch(channels){
    case 1:
      pcm->matrix = strdup("M");
      break;
    case 2:
      pcm->matrix = strdup("L,R");
      break;
    case 3:
      pcm->matrix = strdup("L,R,C");
      break;
    case 4:
      pcm->matrix = strdup("L,R,BL,BR");
      break;
    case 5:
      pcm->matrix = strdup("L,R,C,BL,BR");
      break;
    case 6:
      pcm->matrix = strdup("L,R,C,LFE,BL,BR");
      break;
    case 7:
      pcm->matrix = strdup("L,R,C,LFE,BC,SL,SR");
      break;
    default:
      pcm->matrix = strdup("L,R,C,LFE,BL,BR,SL,SR");
      break;
    }
  }else{
    pcm->matrix = calloc(32*4,sizeof(char));
    for(i=0;i<32;i++){
      if(mask&(1<<i)){
        strcat(pcm->matrix,mask_map[i]);
        strcat(pcm->matrix,",");
      }
    }
    pcm->matrix[strlen(pcm->matrix)-1]=0;
  }

  if(!find_wav_chunk(in, path, "data", &len)){
    fprintf(stderr,"%s: Failed to find fmt chunk in WAV file\n",path);
    goto err;
  }

  if(align != channels * ((samplesize+7)/8)) {
    /* This is incorrect according to the spec. Warn loudly, then ignore
     * this value.
     */
    fprintf(stderr, "%s: WAV 'block alignment' value is incorrect, "
            "ignoring.\n"
            "The software that created this file is incorrect.\n",path);
  }

  if((format==1 && (samplesize == 24 || samplesize == 16 || samplesize == 8)) ||
     (samplesize == 32 && format == 3)){
    /* OK, good - we have a supported format,
       now we want to find the size of the file */
    pcm->rate = samplerate;
    pcm->ch = channels;
    pcm->savebits = (format==3 ? -samplesize : samplesize);

    if(len){
      pcm->samples = len;
    }else{
      long pos;
      pos = ftell(in);
      if(fseek(in, 0, SEEK_END) == -1){
        fprintf(stderr,"%s failed to seek: %s\n",path,strerror(errno));
        goto err;
      }else{
        pcm->samples = ftell(in) - pos;
        fseek(in,pos, SEEK_SET);
      }
    }

  }else{
    fprintf(stderr,
            "%s: Wav file is unsupported subformat (must be 8,16, or 24-bit PCM\n"
            "or floating point PCM\n",path);
    goto err;
  }

  /* read the samples into memory */
  pcm->samples/=pcm->ch;
  pcm->samples/=((abs(pcm->savebits)+7)>>3);
  if(read_raw_samples(pcm,in,0,0))
    goto err;
  return pcm;
 err:
  free_pcm(pcm);
  return NULL;
}

/* AIFF file support ***********************************************************/

static int find_aiff_chunk(FILE *in, const char *path, char *type, unsigned int *len){
  unsigned char buf[8];
  int restarted = 0;

  while(1){
    if(fread(buf,1,8,in)<8){
      if(!restarted) {
        /* Handle out of order chunks by seeking back to the start
         * to retry */
        restarted = 1;
        fseek(in, 12, SEEK_SET);
        continue;
      }
      fprintf(stderr,"%s: Unexpected EOF in AIFF chunk\n",path);
      return 0;
    }

    *len = READ_U32_BE(buf+4);

    if(memcmp(buf,type,4)){
      if((*len) & 0x1)
        (*len)++;

      if(fseek(in,*len,SEEK_CUR))
        return 0;
    }else
      return 1;
  }
}

static double read_IEEE80(unsigned char *buf){
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

static inline void swap(unsigned char *a, unsigned char *b){
  unsigned char temp=*a;
  *a=*b;
  *b=temp;
}

static pcm_t *aiff_load(const char *path, FILE *in){
  pcm_t *pcm = NULL;
  int aifc; /* AIFC or AIFF? */
  unsigned int len;
  unsigned char *buffer;
  unsigned char buf2[12];
  int bend = 1;

  if(fseek(in,0,SEEK_SET)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }
  if(fread(buf2,1,12,in)!=12){
    fprintf(stderr,"%s: Failed to read AIFF header\n",path);
    goto err;
  }

  pcm = calloc(1,sizeof(pcm_t));
  pcm->name=strdup(trim_path(path));

  if(buf2[11]=='C')
    aifc=1;
  else
    aifc=0;

  if(!find_aiff_chunk(in, path, "COMM", &len)){
    fprintf(stderr,"%s: No common chunk found in AIFF file\n",path);
    goto err;
  }

  if(len < 18){
    fprintf(stderr, "%s: Truncated common chunk in AIFF header\n",path);
    goto err;
  }

  buffer = alloca(len);

  if(fread(buffer,1,len,in) < len){
    fprintf(stderr, "%s: Unexpected EOF in reading AIFF header\n",path);
    goto err;
  }

  pcm->ch = READ_U16_BE(buffer);
  pcm->rate = (int)read_IEEE80(buffer+8);
  pcm->savebits = READ_U16_BE(buffer+6);
  pcm->samples = READ_U32_BE(buffer+2)*pcm->ch*((pcm->savebits+7)/8);

  switch(pcm->ch){
  case 1:
    pcm->matrix = strdup("M");
    break;
  case 2:
    pcm->matrix = strdup("L,R");
    break;
  case 3:
    pcm->matrix = strdup("L,R,C");
    break;
  default:
    pcm->matrix = strdup("L,R,BL,BR");
    break;
  }

  if(aifc){
    if(len < 22){
      fprintf(stderr, "%s: AIFF-C header truncated.\n",path);
      goto err;
    }

    if(!memcmp(buffer+18, "NONE", 4)){
      bend = 1;
    }else if(!memcmp(buffer+18, "sowt", 4)){
      bend = 0;
    }else{
      fprintf(stderr, "%s: Can't handle compressed AIFF-C (%c%c%c%c)\n", path,
              *(buffer+18), *(buffer+19), *(buffer+20), *(buffer+21));
      goto err;
    }
  }

  if(!find_aiff_chunk(in, path, "SSND", &len)){
    fprintf(stderr, "%s: No SSND chunk found in AIFF file\n",path);
    goto err;
  }
  if(len < 8) {
    fprintf(stderr,"%s: Corrupted SSND chunk in AIFF header\n",path);
    goto err;
  }

  if(fread(buf2,1,8, in) < 8){
    fprintf(stderr, "%s: Unexpected EOF reading AIFF header\n",path);
    goto err;
  }

  int offset = READ_U32_BE(buf2);
  int blocksize = READ_U32_BE(buf2+4);

  if( blocksize != 0 ||
      !(pcm->savebits==24 || pcm->savebits == 16 || pcm->savebits == 8)){
    fprintf(stderr,
            "%s: Unsupported type of AIFF/AIFC file\n"
            " Must be 8-, 16- or 24-bit integer PCM.\n",path);
    goto err;
  }

  fseek(in, offset, SEEK_CUR); /* Swallow some data */

  /* read the samples into memory */
  pcm->samples/=pcm->ch;
  pcm->samples/=((abs(pcm->savebits)+7)>>3);
  if(read_raw_samples(pcm,in,1,bend))
    goto err;
  return pcm;
 err:
  free_pcm(pcm);
  return NULL;
}

/* SW loading to make JM happy *******************************************************/

static pcm_t *sw_load(const char *path, FILE *in){

  pcm_t *pcm = calloc(1,sizeof(pcm_t));
  pcm->name=strdup(trim_path(path));
  pcm->savebits=16;
  pcm->ch=1;
  pcm->rate=48000;
  pcm->matrix=strdup("M");

  if(fseek(in,0,SEEK_END)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }
  pcm->samples=ftell(in);
  if(pcm->samples==-1 || fseek(in,0,SEEK_SET)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }

  pcm->samples/=2;
  if(read_raw_samples(pcm,in,0,0))
    goto err;
  return pcm;
 err:
  free_pcm(pcm);
  return NULL;
}

/* FLAC and OggFLAC load support *****************************************************************/

typedef struct {
  FILE *in;
  pcm_t *pcm;
  off_t fill;
} flac_callback_arg;

/* glorified fread wrapper */
static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder,
                                            FLAC__byte buffer[],
                                            size_t *bytes,
                                            void *client_data){
  flac_callback_arg *flac = (flac_callback_arg *)client_data;

  if(feof(flac->in)){
    *bytes = 0;
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
  }else if(ferror(flac->in)){
    *bytes = 0;
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }

  *bytes = fread(buffer, sizeof(FLAC__byte), *bytes, flac->in);

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder,
                                              const FLAC__Frame *frame,
                                              const FLAC__int32 *const buffer[],
                                              void *client_data){
  flac_callback_arg *flac = (flac_callback_arg *)client_data;
  pcm_t *pcm = flac->pcm;
  int samples = frame->header.blocksize;
  int channels = frame->header.channels;
  int bits_per_sample = frame->header.bits_per_sample;
  off_t sofar = flac->fill;
  int i, j;

  if(pcm->data == NULL){
    /* lazy initialization */
    pcm->ch = channels;
    pcm->savebits = (bits_per_sample+7)/8*8;

    pcm->data = calloc(pcm->ch,sizeof(*pcm->data));
    if(pcm->data == NULL){
      fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    for(i=0;i<pcm->ch;i++){
      pcm->data[i] = calloc(pcm->samples,sizeof(**pcm->data));
      if(pcm->data[i] == NULL){
        fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
      }
    }
  }

  if(channels != pcm->ch){
    fprintf(stderr,"\r%s: number of channels changes part way through file\n",pcm->name);
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }
  if(pcm->savebits != (bits_per_sample+7)/8*8){
    fprintf(stderr,"\r%s: bit depth changes part way through file\n",pcm->name);
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }

  {
    int shift = pcm->savebits - bits_per_sample;
    off_t sofar=flac->fill;
    switch(pcm->savebits){
    case 16:
      for (j = 0; j < samples; j++){
        for (i = 0; i < channels; i++)
          pcm->data[i][sofar] = (buffer[i][j]<<shift)*(1.f/32768.f);
        sofar++;
      }
      break;
    case 24:
      for (j = 0; j < samples; j++){
        for (i = 0; i < channels; i++)
          pcm->data[i][sofar] = (buffer[i][j]<<shift)*(1.f/8388608.f);
        sofar++;
      }
      break;
    default:
      fprintf(stderr,"\r%s: Only 16- and 24-bit FLACs are supported for decode right now.\n",pcm->name);
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
  }
  flac->fill=sofar;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__StreamDecoder *decoder,
                       const FLAC__StreamMetadata *metadata,
                       void *client_data){
  flac_callback_arg *flac = (flac_callback_arg *)client_data;
  pcm_t *pcm = flac->pcm;

  switch (metadata->type){
  case FLAC__METADATA_TYPE_STREAMINFO:
    pcm->samples = metadata->data.stream_info.total_samples; 
    pcm->rate = metadata->data.stream_info.sample_rate;
    break;
  default:
    break;
  }
}

static void error_callback(const FLAC__StreamDecoder *decoder,
                    FLAC__StreamDecoderErrorStatus status,
                    void *client_data){

  flac_callback_arg *flac = (flac_callback_arg *)client_data;
  pcm_t *pcm = flac->pcm;
  fprintf(stderr,"\r%s: Error decoding file.\n",pcm->name);
}

static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder,
                        void *client_data){
  flac_callback_arg *flac = (flac_callback_arg *)client_data;
  return feof(flac->in)? true : false;
}

static pcm_t *flac_load_i(const char *path, FILE *in, int oggp){
  pcm_t *pcm;
  flac_callback_arg *flac;
  FLAC__StreamDecoder *decoder;
  FLAC__bool ret;

  if(fseek(in,0,SEEK_SET)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }

  pcm = calloc(1,sizeof(pcm_t));
  flac = calloc(1,sizeof(flac_callback_arg));
  decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_set_md5_checking(decoder, true);
  FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);

  pcm->name=strdup(trim_path(path));
  flac->in=in;
  flac->pcm=pcm;

  if(oggp)
    FLAC__stream_decoder_init_ogg_stream(decoder,
                                         read_callback,
                                         /*seek_callback=*/0,
                                         /*tell_callback=*/0,
                                         /*length_callback=*/0,
                                         eof_callback,
                                         write_callback,
                                         metadata_callback,
                                         error_callback,
                                         flac);
  else
    FLAC__stream_decoder_init_stream(decoder,
                                     read_callback,
                                     /*seek_callback=*/0,
                                     /*tell_callback=*/0,
                                     /*length_callback=*/0,
                                     eof_callback,
                                     write_callback,
                                     metadata_callback,
                                     error_callback,
                                     flac);

  /* setup and sample reading handled by configured callbacks */
  ret=FLAC__stream_decoder_process_until_end_of_stream(decoder);
  FLAC__stream_decoder_finish(decoder);
  FLAC__stream_decoder_delete(decoder);
  free(flac);
  if(!ret){
    free_pcm(pcm);
    return NULL;
  }

  /* set channel matrix */
  switch(pcm->ch){
  case 1:
    pcm->matrix = strdup("M");
    break;
  case 2:
    pcm->matrix = strdup("L,R");
    break;
  case 3:
    pcm->matrix = strdup("L,R,C");
    break;
  case 4:
    pcm->matrix = strdup("L,R,BL,BR");
    break;
  case 5:
    pcm->matrix = strdup("L,R,C,BL,BR");
    break;
  case 6:
    pcm->matrix = strdup("L,R,C,LFE,BL,BR");
    break;
  case 7:
    pcm->matrix = strdup("L,R,C,LFE,BC,SL,SR");
    break;
  default:
    pcm->matrix = strdup("L,R,C,LFE,BL,BR,SL,SR");
    break;
  }

  return pcm;
 err:
  return NULL;
}

static pcm_t *flac_load(const char *path, FILE *in){
  return flac_load_i(path,in,0);
}

static pcm_t *oggflac_load(const char *path, FILE *in){
  return flac_load_i(path,in,1);
}

/* Vorbis load support **************************************************************************/
static pcm_t *vorbis_load(const char *path, FILE *in){
  OggVorbis_File vf;
  vorbis_info *vi=NULL;
  pcm_t *pcm=NULL;
  off_t fill=0;
  int last_section=-1;
  int i;

  memset(&vf,0,sizeof(vf));

  if(fseek(in,0,SEEK_SET)==-1){
    fprintf(stderr,"%s: Failed to seek\n",path);
    goto err;
  }

  if(ov_open_callbacks(in, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
    fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
    goto err;
  }

  vi=ov_info(&vf,-1);
  pcm = calloc(1,sizeof(pcm_t));
  pcm->name=strdup(trim_path(path));
  pcm->savebits=16;
  pcm->ch=vi->channels;
  pcm->rate=vi->rate;
  pcm->samples=ov_pcm_total(&vf,-1);

  pcm->data = calloc(pcm->ch,sizeof(*pcm->data));
  if(pcm->data == NULL){
    fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
    goto err;
  }
  for(i=0;i<pcm->ch;i++){
    pcm->data[i] = calloc(pcm->samples,sizeof(**pcm->data));
    if(pcm->data[i] == NULL){
      fprintf(stderr,"Unable to allocate enough memory to load sample into memory\n");
      goto err;
    }
  }

  switch(pcm->ch){
  case 1:
    pcm->matrix = strdup("M");
    break;
  case 2:
    pcm->matrix = strdup("L,R");
    break;
  case 3:
    pcm->matrix = strdup("L,C,R");
    break;
  case 4:
    pcm->matrix = strdup("L,R,BL,BR");
    break;
  case 5:
    pcm->matrix = strdup("L,C,R,BL,BR");
    break;
  case 6:
    pcm->matrix = strdup("L,C,R,BL,BR,LFE");
    break;
  case 7:
    pcm->matrix = strdup("L,C,R,SL,SR,BC,LFE");
    break;
  default:
    pcm->matrix = strdup("L,C,R,SL,SR,BL,BR,LFE");
    break;
  }

  while(fill<pcm->samples){
    int current_section;
    int i;
    float **pcmout;
    long ret=ov_read_float(&vf,&pcmout,4096,&current_section);

    if(current_section!=last_section){
      last_section=current_section;
      vi=ov_info(&vf,-1);
      if(vi->channels != pcm->ch || vi->rate!=pcm->rate){
        fprintf(stderr,"%s: Chained file changes channel count/sample rate\n",path);
        goto err;
      }
    }

    if(ret<0){
      fprintf(stderr,"%s: Error while decoding file\n",path);
      goto err;
    }
    if(ret==0){
      fprintf(stderr,"%s: Audio data ended prematurely\n",path);
      goto err;
    }

    for(i=0;i<pcm->ch;i++)
      memcpy(pcm->data[i]+fill,pcmout[i],ret*sizeof(**pcm->data));
    fill+=ret;
  }
  ov_clear(&vf);

  return pcm;
 err:
  ov_clear(&vf);
  free_pcm(pcm);
  return NULL;
}

#define MAX_ID_LEN 35
unsigned char buf[MAX_ID_LEN];

/* Define the supported formats here */
static input_format formats[] = {
  {wav_id,     wav_load,    "wav"},
  {aiff_id,    aiff_load,   "aiff"},
  {flac_id,    flac_load,   "flac"},
  {oggflac_id, oggflac_load,"oggflac"},
  {vorbis_id,  vorbis_load, "oggvorbis"},
  {sw_id,      sw_load,     "sw"},
  {NULL,       NULL,        NULL}
};

pcm_t *squishyio_load_file(const const char *path){
  FILE *f = fopen(path,"rb");
  int j=0;
  int fill;

  if(!f){
    fprintf(stderr,"Unable to open file %s: %s\n",path,strerror(errno));
    return NULL;
  }

  fill = fread(buf, 1, MAX_ID_LEN, f);
  if(fill<MAX_ID_LEN){
    fprintf(stderr,"%s: Input file truncated or NULL\n",path);
    fclose(f);
    return NULL;
  }

  while(formats[j].id_func){
    if(formats[j].id_func(path,buf)){
      pcm_t *ret=formats[j].load_func(path,f);
      fclose(f);
      return ret;
    }
    j++;
  }
  fprintf(stderr,"%s: Unrecognized file format\n",path);
  return NULL;
}

void free_pcm(pcm_t *pcm){
  if(pcm){
    if(pcm->name)free(pcm->name);
    if(pcm->matrix)free(pcm->matrix);
    if(pcm->data)free(pcm->data);
    memset(pcm,0,sizeof(pcm));
    free(pcm);
  }
}

static inline float triangle_ditherval(float *save){
  float r = rand()/(float)RAND_MAX-.5f;
  float ret = *save-r;
  *save = r;
  return ret;
}

int squishyio_save_file(const char *path, pcm_t *pcm, int overwrite){
  ao_sample_format format;
  ao_initialize();

  format.bits = pcm->savebits;
  format.rate = pcm->rate;
  format.channels = pcm->ch;
  format.byte_format = AO_FMT_LITTLE;
  format.matrix = pcm->matrix;

  {
    float t[pcm->ch];
    int ch=0;
    int samplesper, bytesper;
    off_t sofar=0;
    int bps=(abs(pcm->savebits)+7)>>3;
    int cpf=pcm->ch;
    int bpf=bps*cpf;
    int id = ao_driver_id("wav");
    ao_device *a=ao_open_file(id, path, overwrite, &format, NULL);
    if(!a){
      fprintf(stderr,"Failed to open output file: %s",path);
      return 1;
    }

    samplesper = rbsize/bpf;
    bytesper = samplesper*bpf;
    memset(t,0,sizeof(t));

    while(sofar<pcm->samples){
      unsigned char *d=rawbuf;
      off_t bytes, samples=samplesper;
      int i,j,val;
      if((pcm->samples-sofar)<samples)samples=pcm->samples-sofar;
      bytes = samples*bpf;

      switch(pcm->savebits){
      case 16:
        for(i=0;i<samples;i++){
          for(j=0;j<pcm->ch;j++){
            if(pcm->savedither){
              val = rint(pcm->data[j][sofar]*32768.f + triangle_ditherval(t+ch));
              ch++;
              if(ch>pcm->ch)ch=0;
            }else{
              val = rint(pcm->data[j][sofar]*32768.f);
            }

            if(val>=32767.f){
              d[0]=0xff;
              d[1]=0x7f;
            }else if(val<=-32768.f){
              d[0]=0x00;
              d[1]=0x80;
            }else{
              int iv = (int)val;
              d[0]=iv&0xff;
              d[1]=(iv>>8)&0xff;
            }
            d+=2;
          }
          sofar++;
        }
        break;

      case 24:
        for(i=0;i<samples;i++){
          for(j=0;j<pcm->ch;j++){
            val = rint(pcm->data[j][sofar]*8388608);

            if(val>=8388607.f){
              d[0]=0xff;
              d[1]=0x7f;
              d[2]=0x7f;
            }else if(val<=-8388608.f){
              d[0]=0x00;
              d[1]=0x00;
              d[2]=0x80;
            }else{
              int iv = (int)val;
              d[0]=iv&0xff;
              d[1]=(iv>>8)&0xff;
              d[2]=(iv>>16)&0xff;
            }
            d+=3;
          }
          sofar++;
        }
        break;
      default:
        fprintf(stderr,"Unsupported output bit depth :-(\n");
        goto err;
      }

      if(!ao_play(a,(char *)rawbuf,bytes))
        goto err;

    }

    ao_close(a);
    return 0;

  err:
    ao_close(a);
    return 1;
  }
}

