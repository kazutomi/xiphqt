/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: transport.h,v 1.1.2.1 2001/12/08 23:59:25 volsung Exp $

 ********************************************************************/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include <sys/types.h>
#include <unistd.h>

struct transport_t;

typedef struct data_source_t {
  char *source_string;
  struct transport_t *transport;
  void *private;
} data_source_t;

typedef struct transport_t {
  char *name;
  int (* can_transport)(char *source_string);
  data_source_t* (* open) (char *source_string);
  int (* peek) (data_source_t *source, void *ptr, size_t size, size_t nmemb);
  int (* read) (data_source_t *source, void *ptr, size_t size, size_t nmemb);
  int (* seek) (data_source_t *source, long offset, int whence);
  long (* tell) (data_source_t *source);
  void (* close) (data_source_t *source);
} transport_t;

transport_t *get_transport_by_name (char *name);
transport_t *select_transport (char *source);

#endif /* __TRANSPORT_H__ */
