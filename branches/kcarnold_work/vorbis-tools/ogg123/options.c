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

 last mod: $Id: options.c,v 1.1.2.8 2001/08/31 18:01:12 kcarnold Exp $

 ********************************************************************/

#define _GNU_SOURCE

/* if strcasecmp is giving you problems, switch to strcmp or the appropriate
 * function for your platform / compiler.
 */

#include "options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> /* for LONG_MAX / LONG_MIN */
#include <errno.h>
#include <math.h> /* for HUGE_VAL */

/* InitOpts - initialize opts[] - sets all pointers to their default values */
void InitOpts (Option_t opts[])
{
  while (opts && opts->name)
    {
      opts->found = 0;
      if (opts->dfl) {
	switch (opts->type) {
	case opt_type_none:
	  /* do nothing */
	  break;
	  
	case opt_type_char:
	  *(char *) opts->ptr = *(char*) opts->dfl;
	  break;
	  
	case opt_type_string:
	  *(char **) opts->ptr = *(char **) opts->dfl;
	  break;
	  
	case opt_type_int:
	  *(long int *) opts->ptr = *(int *) opts->dfl;
	  break;
	  
	case opt_type_float:
	  *(float *) opts->ptr = *(float *) opts->dfl;
	  break;
	  
	case opt_type_double:
	*(double *) opts->ptr = *(double *) opts->dfl;
	break;
	}
      }
      opts++;
    }
}

/* ParseLine - parses line of config file stored in line
 *  Input: opts[] is an array of Option_t, initialized.
 *         line is a null-terminated line of input (s/\n/\0/)
 *  Output: opts is updated with the option(s) in line. 
 *          line may be modified.
 *  Return value: A ParseCode
 */
ParseCode ParseLine (Option_t opts[], char *line)
{
  char *equals, *value = "";
  Option_t *opt;
  int len;

  /* skip leading whitespace */
  while (line[0] == ' ')
    line++;

  /* remove comments */
  equals = strchr (line, '#');
  if (equals)
    *equals = '\0';

  /* return if only whitespace on line */
  if (!line[0] || line[0] == '#')
    return parse_ok;

  /* check for an '=' and set to \0 */
  equals = strchr (line, '=');
  if (equals) {
    value = equals + 1;
    *equals = '\0';
  }

  /* cut trailing whitespace from key (line = key now) */
  while ((equals = strrchr(line, ' ')))
    *equals = '\0';

  /* remove this if you want a zero-length key */
  if (strlen(line) == 0)
    return parse_nokey;
  
  if (value) {
    /* cut leading whitespace from value */
    while (*value == ' ')
      value++;
    
    /* cut trailing whitespace from value */
    len = strlen (value);
    while (len > 0 && value[len-1] == ' ') {
      len--;
      value[len] = '\0';
    }
  }

  /* now key is in line and value is in value. Search for a matching option. */
  opt = opts;
  while (opt->name) {
    if (!strcasecmp (opt->name, line)) {
      long tmpl;
      char *endptr;

      /* found the key. now set the value. */
      switch (opt->type) {
      case opt_type_none:
	if (value != NULL || strlen(value) > 0)
	  return parse_badvalue;
	opt->found++;
	break;

      case opt_type_char:
	if (strlen(value) != 1)
	  return parse_badvalue;
	opt->found++;
	*(char *) opt->ptr = value[0];
	break;

      case opt_type_string:
	opt->found++;
	if (*(char **)opt->ptr) free(*(char **)opt->ptr);
	*(char **) opt->ptr = strdup (value);
	break;

      case opt_type_int:
	if (!value || *value == '\0')
	  return parse_badvalue;
	tmpl = strtol (value, &endptr, 0);
	if (((tmpl == LONG_MIN || tmpl == LONG_MAX) && errno == ERANGE)
	    || (*endptr != '\0'))
	  return parse_badvalue;
	opt->found++;
	*(long int *) opt->ptr = tmpl;
	break;
	
      case opt_type_float:
	if (!value || *value == '\0')
	  return parse_badvalue;
	opt->found++;
	*(float *) opt->ptr = atof (value);
	break;

      case opt_type_double:
	if (!value || *value == '\0')
	  return parse_badvalue;
	opt->found++;
	*(double *) opt->ptr = atof (value);
	break;

      default:
	return parse_badtype;
      }
      return parse_ok;
    }
    opt++;
  }
  return parse_keynotfound;
}

/* ParseFile - open, read, and parse a file
 *  Input: opts[] is an array of Option_t, initialized
 *         filename is the name of a line to open
 *         errfunc is a function to call on error, passing arg and the error code.
 *         - errfunc should return 0 to continue or 1 to fail with the
 *           original error code. Regardless of errfunc, system errors
 *           always cause ParseFile to return.
 *  Output: opts[] is updated with the options in the file.
 *  Return: a ParseCode
 */
ParseCode ParseFile (Option_t opts[], const char *filename, int (*errfunc) (void *, ParseCode, int, const char*, char*), void *arg)
{
  unsigned int len=80;
  char *line = malloc (len);
  int readoffset, thischar, lineno;
  FILE *file;
  ParseCode pcode;
  char empty[] = "";

  if (!line)
    {
      errfunc (arg, parse_syserr, 0, empty, empty);
      return parse_syserr;
    }

  file = fopen (filename, "r");
  if (!file)
    {
      errfunc (arg, parse_syserr, 0, empty, empty);
      free (line);
      return parse_syserr;
    }

  lineno = 0;
  while (!feof (file)) {
    lineno++;
    readoffset = 0;
    memset (line, 0, len);
    while ((thischar = fgetc(file)) != EOF) {
      if (readoffset + 1 > len) {
	len *= 2;
	line = realloc (line, len);
	if (!line)
	  {
	    errfunc (arg, parse_syserr, 0, empty, empty);
	    fclose (file);
	    return parse_syserr;
	  }
      }
      if (thischar == '\n') {
	line[readoffset] = '\0';
	break;
      }
      else
	line[readoffset] = (unsigned char) thischar;
      readoffset++;
    }
    pcode = ParseLine (opts, line);
    if (pcode != parse_ok)
      if (!errfunc (arg, pcode, lineno, filename, line)) {
	free (line);
	return pcode;
      }
  }
  free (line);
  return parse_ok;
}

/* ParseErr - returns a string corresponding to parse code pcode */
const char *ParseErr (ParseCode pcode)
{
  switch (pcode) {
  case parse_ok:
    return "Success";
  case parse_syserr:
    return strerror(errno);
  case parse_keynotfound:
    return "Key not found";
  case parse_nokey:
    return "No key";
  case parse_badvalue:
    return "Bad value";
  case parse_badtype:
    return "Bad type in options list";
  default:
    return "Unknown error";
  }
}

int PrintSpace (FILE *f, int s, int c)
{
  int tmp = 0;
  do {
    fputc (c, f);
    tmp++;
  } while (--s > 0);
  return tmp;
}

/* DescribeOptions - describe available options to outfile */
void DescribeOptions (Option_t opts[], FILE *f)
{
  /* name | description | type | default */
  int colWidths[] = {0, 0, 7, 7};
  int totalWidth = 0;
  Option_t *opt = opts;

  while (opt->name)
    {
      int len = strlen (opt->name) + 1;
      if (len  > colWidths[0])
	colWidths[0] = len;
      opt++;
    }

  opt = opts;
  while (opt->name)
    {
      int len = strlen (opt->desc) + 1;
      if (len > colWidths[1])
	colWidths[1] = len;
      opt++;
    }

  /* Column headers */
  /* Name */
  totalWidth += fprintf (f, "Name");
  totalWidth += PrintSpace (f, (colWidths[0] - 4), ' ');
  
  /* Description */
  totalWidth += fprintf (f, "Description");
  totalWidth += PrintSpace (f, (colWidths[1] - 11), ' ');
  
  /* Type */
  totalWidth += fprintf (f, "Type");
  totalWidth += PrintSpace (f, (colWidths[2] - 4), ' ');
  
  /* Default */
  totalWidth += fprintf (f, "Default");
  totalWidth += PrintSpace (f, (colWidths[3] - 7), ' ');

  fputc ('\n', f);
  
  /* Divider */
  PrintSpace (f, totalWidth, '-');

  fputc ('\n', f);

  opt = opts;
  while (opt->name)
    {
      /* name */
      int w = colWidths[0];
      w -= fprintf (f, "%s", opt->name);
      PrintSpace (f, w, ' ');

      /* description */
      w = colWidths[1];
      w -= fprintf (f, "%s", opt->desc);
      PrintSpace (f, w, ' ');

      /* type */
      w = colWidths[2];
      switch (opt->type) {
      case opt_type_none:
	w -= fprintf (f, "none");
	break;
      case opt_type_char:
	w -= fprintf (f, "char");
	break;
      case opt_type_string:
	w -= fprintf (f, "string");
	break;
      case opt_type_int:
	w -= fprintf (f, "int");
	break;
      case opt_type_float:
	w -= fprintf (f, "float");
	break;
      case opt_type_double:
	w -= fprintf (f, "double");
	break;
      default:
	w -= fprintf (f, "other");
      }
      PrintSpace (f, w, ' ');

      /* default */
      if (opt->dfl == NULL)
	fputs ("(NULL)", f);
      else {
	switch (opt->type) {
	case opt_type_none:
	  fputs ("(none)", f);
	  break;
	case opt_type_char:
	  fputc (*(char *) opt->dfl, f);
	  break;
	case opt_type_string:
	  fputs (*(char **) opt->dfl, f);
	  break;
	case opt_type_int:
	  fprintf (f, "%ld", *(long int *) opt->dfl);
	  break;
	case opt_type_float:
	  fprintf (f, "%f", (double) (*(float *) opt->dfl));
	  break;
	case opt_type_double:
	  fprintf (f, "%f", *(double *) opt->dfl);
	  break;
	}
      }
      fputc ('\n', f);
      opt++;
    }
}
