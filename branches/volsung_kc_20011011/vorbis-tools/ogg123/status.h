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

 last mod: $Id: status.h,v 1.1.2.4.2.3 2001/12/09 03:45:26 volsung Exp $

 ********************************************************************/

#ifndef __STATUS_H__
#define __STATUS_H__

#include <stdarg.h>

typedef struct {
  int verbosity;
  char enabled;
  const char *formatstr;
  enum {
    stat_noarg = 0,
    stat_intarg,
    stat_stringarg,
    stat_floatarg,
    stat_doublearg
  } type;
  union {
    int intarg;
    char *stringarg;
    float floatarg;
    double doublearg;
  } arg;
} stat_t;


/* Status options:
 * stats[0] - currently playing file / stream
 * stats[1] - current playback time
 * stats[2] - remaining playback time
 * stats[3] - total playback time
 * stats[4] - instantaneous bitrate
 * stats[5] - average bitrate (not yet implemented)
 * stats[6] - input buffer fill %
 * stats[7] - input buffer status
 * stats[8] - output buffer fill %
 * stats[9] - output buffer status
 * stats[10] - Null format string to mark end of array
 */

stat_t *stats_create ();
void stats_cleanup (stat_t *stats);

void status_set_verbosity (int verbosity);
void status_reset_output_lock ();
void status_clear_line ();
void status_print_statistics (stat_t *stats);
void status_message (int verbosity, const char *fmt, ...);
void vstatus_message (int verbosity, const char *fmt, va_list ap);
void status_error (const char *fmt, ...);
void vstatus_error (const char *fmt, va_list);

#endif /* __STATUS_H__ */
