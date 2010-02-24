/*
 *
 *  y4oi:
 *     A utility for doing several simple but essential operations
 *     on yuv4ogg interchange streams.
 *
 *     y4oi copyright (C) 2010 Monty <monty@xiph.org>
 *
 *  y4oi is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  y4oi is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#define _FILE_OFFSET_BITS 64
#define EPSILON 1e-6
#define _GNU_SOURCE

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <limits.h>

typedef struct {
  double begin;
  double end;
} interval;

typedef struct {
  double num;
  double den;
} ratio;

typedef enum {
  Cmono=0,
  C411ntscdv=1,
  C420jpeg=2,
  C420mpeg2=3,
  C420paldv=4,
  C420unknown=5,
  C422jpeg=6,
  C422smpte=7,
  C422unknown=8,
  C444=9,
} chromafmt;

extern char *chromaformat[];
extern unsigned char chromabits[];
extern char *chromaformat_long[];
extern ratio force_fps;
extern int force_no_sync;
extern int force_non_interlaced;

typedef enum {
  STREAM_INVALID=0,
  STREAM_VIDEO=1,
  STREAM_AUDIO=2
} stream_type;

typedef enum {
  INVALID=-1,
  PROGRESSIVE=0,
  TOP_FIRST=1,
  BOTTOM_FIRST=2
} interlace_type;

typedef struct stream_s stream_t;
typedef struct frame_s frame_t;


typedef struct {
  FILE *f;
  int eof;
  stream_t **streams;
  int num_streams;
  int synced;
  int seekable;
  frame_t *f_head;
  frame_t *f_tail;
} y4o_in_t;

struct frame_s {
  double pts;
  size_t len;
  int streamno;
  int presync;
  int ticks;
  int bos;
  double duration;

  unsigned char *data;
  FILE *swap;
  off_t swap_pos;

  frame_t *next;
  frame_t *prev;

  frame_t *f_next;
  frame_t *f_prev;

  stream_t *s;
};

typedef struct {

  FILE *f[2];
  off_t head[2];
  off_t tail[2];
  int write;
} swap_t;

typedef struct {
  int rate;
  int ch;
} audio_param_t;

typedef struct {
  int fps_n;
  int fps_d;
  int pa_n;
  int pa_d;
  int frame_n;
  int frame_d;
  int format;
  int w;
  int h;
  interlace_type i;
} video_param_t;

typedef union {
  audio_param_t audio;
  video_param_t video;
} stream_param_t;

struct stream_s{
  stream_type type;
  int stream_num;
  double tickduration;
  long bytes_per_tick;
  double tolerance;
  int bos;

  stream_param_t m;

  frame_t  *inq_head;
  frame_t  *inq_tail;
  swap_t    inswap;
  long      tick_depth;

  stream_t *next;
  stream_t *prev;
  y4o_in_t *y;
};

extern y4o_in_t *y4o_open_in(FILE *f);
extern void y4o_close_in(y4o_in_t *y);
extern frame_t *y4o_read_frame(y4o_in_t *y);
extern int y4o_lock_frame(frame_t *p);
extern void y4o_pull_frame(frame_t *p);
extern void y4o_free_frame(frame_t *p);

typedef struct filter_queue_frame fq_frame_t;
typedef struct filter_queue fq_t;

struct filter_queue_frame {
  unsigned char *body;
  int body_size;

  int synced;
  int sn;
  int sno;
  stream_type st;
  stream_param_t sp;
  double pts;
  int bos;
  int eos;

  fq_frame_t *prev;
  fq_frame_t *next;
} queue_frame_t;

typedef struct {
  char *name;
  char *desc;
  void (*process)(fq_t *);
  int is_type_output_p;
} filter_t;

typedef enum {
  QUEUE_INIT,
  QUEUE_STARTUP,
  QUEUE_PROCESS,
  QUEUE_FLUSH,
  QUEUE_FINISHED
} fq_state;

struct filter_queue {
  int streams;
  int *stream_depth;
  fq_state state;
  fq_frame_t *head;
  fq_frame_t *tail;
  fq_t *next;
  filter_t *filter;
  char **option_keys;
  char **option_vals;
  void *internal;
};

extern void filter_forward_frame(fq_t *fq);
extern void filter_drop_frame(fq_t *fq);
extern void filter_submit_frame(stream_t *s, unsigned char *b, int len, double pts);
extern void filter_flush();
extern int filter_append(char *cmd);
extern int filter_is_last_output();

extern int verbose;
#ifndef NAME
#define NAME queue->filter->name
#endif

extern char *lastprint;

#define yprint(level, format, args...) {                                \
    if(strcmp(format,"\n")){                                            \
      if((lastprint&&!NAME) || (!lastprint&&NAME) ||                    \
         (lastprint&&NAME&&strcmp(lastprint,NAME?NAME:"")))             \
        fprintf(stderr,"\n");                                           \
      lastprint=(char *)NAME;                                           \
      if(NAME){                                                         \
        if(verbose){                                                    \
          fprintf(stderr,"%s [%8s]: " format,level?level:"       ",     \
                  NAME?NAME:"",## args);                                \
        }else{                                                          \
          fprintf(stderr,"%s%s" format,level?level:"",level?": ":"",## args); \
        }                                                               \
      }else{                                                            \
        fprintf(stderr,"%s%s" format,level?level:"",                    \
                level?": ":"",## args);                                 \
      }                                                                 \
    }else{                                                              \
      fprintf(stderr,"\n");                                             \
    }                                                                   \
  }

#define ydebug(format, args...) {                                       \
    if(verbose==2)                                                      \
      yprint("  debug",format,##args);                                  \
  }

#define yverbose(format, args...) {                                     \
    if(verbose>0)                                                       \
      yprint(NULL,format,##args);                                       \
  }

#define yinfo(format, args...) {                                        \
    if(verbose>=0)                                                      \
      yprint(NULL,format,##args);                                       \
  }

#define ywarn(format, args...) {                                        \
    if(verbose>=0)                                                      \
      yprint("WARNING",format,##args);                                  \
  }

#define yerror(format, args...) {                                       \
    if(verbose>=0)                                                      \
      yprint("  ERROR",format,##args);                                  \
  }
