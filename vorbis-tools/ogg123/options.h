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

 last mod: $Id: options.h,v 1.1.2.1 2001/08/12 03:59:31 kcarnold Exp $

 ********************************************************************/

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <stdio.h>

/* options.h
 *  Header to ogg123 configuration options interface
 */

typedef enum {
  opt_type_none = 0,
  opt_type_char,
  opt_type_string,
  opt_type_int, /* long int */
  opt_type_float,
  opt_type_double
} OptionType;

typedef enum {
  parse_ok = 0,
  parse_syserr,
  parse_keynotfound,
  parse_nokey,
  parse_badvalue,
  parse_badtype
} ParseCode;

typedef struct Option_s {
  char found;
  char *name;
  char *desc;
  OptionType type;
  void *ptr;
  void *dfl;
} Option_t;

void InitOpts (Option_t opts[]);
ParseCode ParseLine (Option_t opts[], char *line);
ParseCode ParseFile (Option_t opts[], char *filename, int (*errfunc) (void *, ParseCode, int, char*, char*), void *arg);
const char *ParseErr (ParseCode pcode);
void DescribeOptions (Option_t opts[], FILE *outfile);

#endif /* __OPTIONS_H */
