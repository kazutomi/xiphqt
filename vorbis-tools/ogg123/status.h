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

 last mod: $Id: status.h,v 1.1.2.4.2.1 2001/10/31 05:38:56 volsung Exp $

 ********************************************************************/

#ifndef __STATUS_H
#define __STATUS_H

#include <stdarg.h>

/* status interface */

typedef struct {
  int prio;
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
} Stat_t;

void PrintStatsLine (Stat_t stats[]);
void ShowMessage (int prio, char keepStatusLine, char addNewline, 
		  const char *fmt, ...);
void Error (const char *fmt, ...);
void SetPriority (int prio);

#endif /* __STATUS_H */
