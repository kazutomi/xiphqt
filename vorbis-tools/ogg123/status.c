/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Kenneth C. Arnold <ogg@arnoldnet.net> AND OTHER CONTRIBUTORS  *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: status.c,v 1.1.2.7.2.3 2001/12/09 03:45:26 volsung Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "buffer.h"
#include "status.h"


int last_line_len = 0;
int max_verbosity = 0;

pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;


/* ------------------- Private functions ------------------ */

void set_buffer_state_string (buf_t *buf, char *strbuf)
{
  char *cur = strbuf;
  char *comma = ", ";
  char *sep = "(";

  if (buf->prebuffering) {
    cur += sprintf (cur, "%sPrebuf", sep);
    sep = comma;
  }
  if (buf->paused) {
    cur += sprintf (cur, "%sPaused", sep);
    sep = comma;
  }
  if (buf->eos) {
    cur += sprintf (cur, "%sEOS", sep);
    sep = comma;
  }
  if (cur != strbuf)
    cur += sprintf (cur, ")");
  else
    *cur = '\0';
}


#if 0
void SetTime (stat_t stats[], ogg_int64_t sample)
{
  double CurTime = (double) sample / (double) Options.outputOpts.rate;
  long c_min = (long) CurTime / (long) 60;
  double c_sec = CurTime - 60.0f * c_min;
  long r_min, t_min;
  double r_sec, t_sec;

  if (stats[2].enabled && Options.inputOpts.seekable) {
    if (sample > Options.inputOpts.totalSamples) {
      /* file probably grew while playing; update total time */
      Options.inputOpts.totalSamples = sample;
      Options.inputOpts.totalTime = CurTime;
      stats[3].arg.stringarg[0] = '\0';
      r_min = 0;
      r_sec = 0.0f;
    } else {
      r_min = (long) (Options.inputOpts.totalTime - CurTime) / (long) 60;
      r_sec = ((double) Options.inputOpts.totalTime - CurTime) - 60.0f * (double) r_min;
    }
    sprintf (stats[2].arg.stringarg, "[%02li:%05.2f]", r_min, r_sec);
    if (stats[3].arg.stringarg[0] == '\0') {
      t_min = (long) Options.inputOpts.totalTime / (long) 60;
      t_sec = Options.inputOpts.totalTime - 60.0f * t_min;
      sprintf (stats[3].arg.stringarg, "%02li:%05.2f", t_min, t_sec);
    }    
  }
  sprintf (stats[1].arg.stringarg, "%02li:%05.2f", c_min, c_sec);
}
#endif


void clear_line ()
{
  int len;

  len = last_line_len;

  fputc('\r', stderr);

  while (len > 0) {
    fputc (' ', stderr);
    len--;
  }

  fputc ('\r', stderr);
}


void print_statistics_line (stat_t stats[])
{
  int len = 0;
  
  status_clear_line(last_line_len);
  
  while (stats->formatstr != NULL) {
    
    if (stats->verbosity > max_verbosity || !stats->enabled) {
      stats++;
      continue;
    }

    if (len != 0)
      len += fprintf(stderr, " ");

    switch (stats->type) {
    case stat_noarg:
      len += fprintf(stderr, stats->formatstr);
      break;
    case stat_intarg:
      len += fprintf(stderr, stats->formatstr, stats->arg.intarg);
      break;
    case stat_stringarg:
      len += fprintf(stderr, stats->formatstr, stats->arg.stringarg);
      break;
    case stat_floatarg:
      len += fprintf(stderr, stats->formatstr, stats->arg.floatarg);
      break;
    case stat_doublearg:
      len += fprintf(stderr, stats->formatstr, stats->arg.doublearg);
      break;
    }

    stats++;
  }

  last_line_len = len;
}


void vstatus_print_nolock (const char *fmt, va_list ap)
{
  if (last_line_len != 0)
    fputc ('\n', stderr);

  vfprintf (stderr, fmt, ap);

  fputc ('\n', stderr);

  last_line_len = 0;
}


/* ------------------- Public interface -------------------- */

#define TIME_STR_SIZE 20
#define STATE_STR_SIZE 25
#define NUM_STATS 10

stat_t *stats_create ()
{
  stat_t *stats;
  stat_t *cur;

  stats = calloc(NUM_STATS + 1, sizeof(stat_t));  /* One extra for end flag */
  if (stats == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }

  cur = stats + 0; /* currently playing file / stream */
  cur->verbosity = 3; 
  cur->enabled = 0;
  cur->formatstr = "File: %s"; 
  cur->type = stat_stringarg;
  
  cur = stats + 1; /* current playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = "Time: %s"; 
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }

    
  cur = stats + 2; /* remaining playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 3; /* total playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 0;
  cur->formatstr = "of %s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 4; /* instantaneous bitrate */
  cur->verbosity = 2;
  cur->enabled = 1;
  cur->formatstr = "Bitrate: %5.1f";
  cur->type = stat_doublearg;

  cur = stats + 5; /* average bitrate (not yet implemented) */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = "Avg bitrate: %5.1f";
  cur->type = stat_doublearg;

  cur = stats + 6; /* input buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = " Input Buffer %5.1f%%";
  cur->type = stat_doublearg;

  cur = stats + 7; /* input buffer status */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 8; /* output buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = " Output Buffer %5.1f%%"; 
  cur->type = stat_doublearg;

  cur = stats + 9; /* output buffer status */
  cur->verbosity = 1;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 10; /* End flag */
  cur->formatstr = NULL;

  return stats;
}


void stats_cleanup (stat_t *stats)
{
  free(stats[1].arg.stringarg);
  free(stats[2].arg.stringarg);
  free(stats[3].arg.stringarg);
  free(stats[7].arg.stringarg);
  free(stats[9].arg.stringarg);
  free(stats);
}


void status_set_verbosity (int verbosity)
{
  max_verbosity = verbosity;
}


void status_reset_output_lock ()
{
  pthread_mutex_unlock(&output_lock);
}


void status_clear_line ()
{
  pthread_mutex_lock(&output_lock);

  clear_line();

  pthread_mutex_unlock(&output_lock);
}

void status_print_statistics (stat_t *stats)
{

  /* Updating statistics is not critical.  If another thread is
     already doing output, we skip it. */
  if (pthread_mutex_trylock(&output_lock) == 0) {

#if 0
    if () {
      set_buffer_state_string(Options.inputOpts.data->buf,
			      stats[7].arg.stringarg);
      stats[6].arg.doublearg = 
	(double) buffer_full(Options.inputOpts.data->buf)
	/ (double) Options.inputOpts.data->buf->size * 100.0f;
    }

    if (Options.outputOpts.buffer) {
      set_buffer_state_string(Options.inputOpts.data->buf,
			      stats[9].arg.stringarg);
      
      stats[8].arg.doublearg =
	(double) buffer_full(Options.outputOpts.buffer) 
	/ (double) Options.outputOpts.buffer->size * 100.0f;
    }

    
    print_statistics_line(stats);
#endif

    clear_line();
    last_line_len = fprintf(stderr, "Boing!");
    
    pthread_mutex_unlock(&output_lock);
  }
}


void status_message (int verbosity, const char *fmt, ...)
{
  va_list ap;

  if (verbosity > max_verbosity)
    return;

  pthread_mutex_lock(&output_lock);

  va_start (ap, fmt);
  vstatus_print_nolock(fmt, ap);
  va_end (ap);

  pthread_mutex_unlock(&output_lock);
}


void vstatus_message (int verbosity, const char *fmt, va_list ap)
{
  if (verbosity > max_verbosity)
    return;

  pthread_mutex_lock(&output_lock);

  vstatus_print_nolock(fmt, ap);

  pthread_mutex_unlock(&output_lock);
}


void status_error (const char *fmt, ...)
{
  va_list ap;

  pthread_mutex_lock(&output_lock);

  va_start (ap, fmt);
  vstatus_print_nolock (fmt, ap);
  va_end (ap);

  pthread_mutex_unlock(&output_lock);
}


void vstatus_error (const char *fmt, va_list ap)
{
  pthread_mutex_lock(&output_lock);

  vstatus_print_nolock (fmt, ap);

  pthread_mutex_unlock(&output_lock);
}

