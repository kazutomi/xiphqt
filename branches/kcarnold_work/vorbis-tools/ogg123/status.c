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

 last mod: $Id: status.c,v 1.1.2.7 2001/08/31 18:01:12 kcarnold Exp $

 ********************************************************************/

/* status interface */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "status.h"

/* a few globals */
/* stderr is thread-global, so status vars should be also; caller must
 * ensure that status functions are not called concurrently */
size_t buflen = 100; /* guess max length to be 100 */
char *tmpbuf = NULL; /* global so updating quick after size determined */
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

int AppendString (int len, const char *fmt, ...) {
  va_list ap;
  int n = -1;
  size_t size;

  while (n == -1) {
    size = buflen - len - 1;
    va_start (ap, fmt);
    n = vsnprintf (tmpbuf + len, size, fmt, ap);
    va_end (ap);

    if (n > -1 && n < size)
      return n;
    /* otherwise, double the size (likely more efficient) */
    buflen *= 2;
    if (!(tmpbuf = realloc (tmpbuf, buflen))) {
      ClearLine();
      perror ("malloc");
      exit (1);
    }
    n = -1;
  }
  return 0; /* makes dumb compilers happy */
}

void UpdateStats (Stat_t stats[])
{
  int len = 0, left;
  
  if (tmpbuf == NULL) {
    tmpbuf = malloc (buflen);
    if (!tmpbuf) {
      ClearLine();
      perror ("malloc");
      exit (1);
    }
    *tmpbuf = '\0';
  }
  
  while (stats->formatstr != NULL)
    {
      if (stats->prio > MaxPrio || !stats->enabled) {
	stats++;
	continue;
      }
      if (len != 0)
	len += AppendString(len, " ");
      else
	fputc ('\r', stderr);
      switch (stats->type) {
      case stat_noarg:
	len += AppendString (len, stats->formatstr);
	break;
      case stat_intarg:
	len += AppendString (len, stats->formatstr, stats->arg.intarg);
	break;
      case stat_stringarg:
	len += AppendString (len, stats->formatstr, stats->arg.stringarg);
	break;
      case stat_floatarg:
	len += AppendString (len, stats->formatstr, stats->arg.floatarg);
	break;
      case stat_doublearg:
	len += AppendString (len, stats->formatstr, stats->arg.doublearg);
	break;
      }
      stats++;
    }
  left = LastLineLen - fprintf (stderr, "%s", tmpbuf);
  while (left-- > 0)
    fputc (' ', stderr);
  if (LastLineLen || len)
    fputc ('\r', stderr);
  LastLineLen = len;
}

void ShowMessage (int prio, char keepLastLine, char addNewline, const char *fmt, ...)
{
  va_list ap;

  if (prio > MaxPrio)
    return;
  if (!keepLastLine)
    ClearLine();
  else
    if (LastLineLen)
      fputc ('\n', stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  if (addNewline)
    fputc ('\n', stderr);
  LastLineLen = 0;
}

/* a degenerate ShowMessage specifically for spitting out an error. fmt has a newline. */
void Error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  ClearLine();
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  LastLineLen = 0;
}

void SetPriority (int prio)
{
  MaxPrio = prio;
}
