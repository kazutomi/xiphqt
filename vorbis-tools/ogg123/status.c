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

 last mod: $Id: status.c,v 1.1.2.3 2001/08/13 00:59:43 kcarnold Exp $

 ********************************************************************/

/* status interface */

#include <stdio.h>
#include "status.h"

/* a few globals */
int LastLineLen = 0;
int MaxPrio = 0;

void ClearLine ()
{
  int c = LastLineLen;
  fputc('\r', stderr);
  while (c-- > 0)
    fputc (' ', stderr);
  fputc ('\r', stderr);
}

void UpdateStats (Stat_t stats[])
{
  int len = 0, left;

  while (stats->formatstr != NULL)
    {
      if (stats->prio > MaxPrio || !stats->enabled) {
	stats++;
	continue;
      }
      if (len != 0)
	len += fprintf (stderr, " ");
      else
	fputc ('\r', stderr);
      switch (stats->type) {
      case stat_noarg:
	len += fprintf (stderr, stats->formatstr);
	break;
      case stat_intarg:
	len += fprintf (stderr, stats->formatstr, stats->arg.intarg);
	break;
      case stat_stringarg:
	len += fprintf (stderr, stats->formatstr, stats->arg.stringarg);
	break;
      case stat_floatarg:
	len += fprintf (stderr, stats->formatstr, stats->arg.floatarg);
	break;
      case stat_doublearg:
	len += fprintf (stderr, stats->formatstr, stats->arg.doublearg);
	break;
      }
      stats++;
    }
  left = LastLineLen - len;
  while (left-- > 0)
    fputc (' ', stderr);
  LastLineLen = len;
  fputc ('\r', stderr);
}

/* msg has no final \n and no formatting */
void ShowMessage (int prio, char keepLastLine, char *msg)
{
  if (prio > MaxPrio)
    return;
  if (!keepLastLine)
    ClearLine();
  else
    if (LastLineLen)
      fputc ('\n', stderr);
  fprintf (stderr, msg);
  fputc ('\n', stderr);
  LastLineLen = 0;
}

void SetPriority (int prio)
{
  MaxPrio = prio;
}
