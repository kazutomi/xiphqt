#include <stdio.h>
#include <string.h>

/* simple WAVE header writer */

#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_EXTENSIBLE  0xfffe
#define WAV_HEADER_LEN 68

#define WRITE_U32(buf, x) *(buf)     = (unsigned char)(x&0xff);\
  *((buf)+1) = (unsigned char)((x>>8)&0xff);                            \
  *((buf)+2) = (unsigned char)((x>>16)&0xff);                           \
  *((buf)+3) = (unsigned char)((x>>24)&0xff);

#define WRITE_U16(buf, x) *(buf)     = (unsigned char)(x&0xff);\
  *((buf)+1) = (unsigned char)((x>>8)&0xff);

struct riff_struct {
  char id[4];   /* RIFF */
  unsigned int len;
  char wave_id[4]; /* WAVE */
};

struct chunk_struct {
  char id[4];
  unsigned int len;
};

struct common_struct
{
  unsigned short wFormatTag;
  unsigned short wChannels;
  unsigned int dwSamplesPerSec;
  unsigned int dwAvgBytesPerSec;
  unsigned short wBlockAlign;
  unsigned short wBitsPerSample;
  unsigned short cbSize;
  unsigned short wValidBitsPerSample;
  unsigned int   dwChannelMask;
  unsigned short subFormat;
};

struct wave_header
{
  struct riff_struct   riff;
  struct chunk_struct  format;
  struct common_struct common;
  struct chunk_struct  data;
};

int header_out(FILE *out, int rate, int bits, int channels){
  struct wave_header wave;
  char buf[WAV_HEADER_LEN];

  /* Store information */
  wave.common.wChannels = channels;
  wave.common.wBitsPerSample = ((bits+7)>>3)<<3;
  wave.common.wValidBitsPerSample = bits;
  wave.common.dwSamplesPerSec = rate;

  memset(buf, 0, WAV_HEADER_LEN);

  /* Fill out our wav-header with some information. */
  strncpy(wave.riff.id, "RIFF",4);
  wave.riff.len = 0xffffffff; //size - 8; Use a bogus size for streaming */
  strncpy(wave.riff.wave_id, "WAVE",4);
  strncpy(wave.format.id, "fmt ",4);
  wave.format.len = 40;

  wave.common.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  wave.common.dwAvgBytesPerSec =
    wave.common.wChannels *
    wave.common.dwSamplesPerSec *
    (wave.common.wBitsPerSample >> 3);

  wave.common.wBlockAlign =
    wave.common.wChannels *
    (wave.common.wBitsPerSample >> 3);
  wave.common.cbSize = 22;
  wave.common.subFormat = WAVE_FORMAT_PCM;
  wave.common.dwChannelMask = 0;

  strncpy(wave.data.id, "data",4);

  wave.data.len = 0xffffffff; // size - WAV_HEADER_LEN; Use a bogus size for streaming */

  strncpy(buf, wave.riff.id, 4);
  WRITE_U32(buf+4, wave.riff.len);
  strncpy(buf+8, wave.riff.wave_id, 4);
  strncpy(buf+12, wave.format.id,4);
  WRITE_U32(buf+16, wave.format.len);
  WRITE_U16(buf+20, wave.common.wFormatTag);
  WRITE_U16(buf+22, wave.common.wChannels);
  WRITE_U32(buf+24, wave.common.dwSamplesPerSec);
  WRITE_U32(buf+28, wave.common.dwAvgBytesPerSec);
  WRITE_U16(buf+32, wave.common.wBlockAlign);
  WRITE_U16(buf+34, wave.common.wBitsPerSample);
  WRITE_U16(buf+36, wave.common.cbSize);
  WRITE_U16(buf+38, wave.common.wValidBitsPerSample);
  WRITE_U32(buf+40, wave.common.dwChannelMask);
  WRITE_U16(buf+44, wave.common.subFormat);
  memcpy(buf+46,"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71",14);
  strncpy(buf+60, wave.data.id, 4);
  WRITE_U32(buf+64, wave.data.len);

  if (fwrite(buf, sizeof(char), WAV_HEADER_LEN, out)
      != WAV_HEADER_LEN) {
    return 0; /* Could not write wav header */
  }

  return 1;
}

