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
 
 last mod: $Id: ao_interface.h,v 1.1.2.3.2.1 2001/10/14 05:42:51 volsung Exp $
 
********************************************************************/

/* ogg123's interface to libao */

#ifndef __AO_INTERFACE_H
#define __AO_INTERFACE_H

#include <ao/ao.h>

/* For facilitating output to multiple devices */
typedef struct devices_s {
  int driver_id;
  ao_device *device;
  ao_option *options;
  char *filename;
  struct devices_s *next_device;
} devices_t;

devices_t *append_device(devices_t * devices_list, int driver_id,
                         ao_option * options, char *filename);
int devices_write(void *ptr, size_t size, size_t nmemb, devices_t * d);
int add_option(ao_option ** op_h, const char *optstring);
void ao_onexit (int exitcode, void *devices);
void close_audio_devices (devices_t *devices);
void free_audio_devices (devices_t *devices);

#endif /* __AO_INTERFACE_H */
