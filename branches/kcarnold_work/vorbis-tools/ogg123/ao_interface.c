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

 last mod: $Id: ao_interface.c,v 1.5.2.5 2001/08/12 03:59:31 kcarnold Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ogg123.h"

devices_t *append_device(devices_t * devices_list, int driver_id,
			 ao_option * options, char *filename)
{
    devices_t *head = devices_list;

    if (devices_list != NULL) {
	while (devices_list->next_device != NULL)
	    devices_list = devices_list->next_device;
	devices_list = devices_list->next_device =
	    malloc(sizeof(devices_t));
    } else {
	head = devices_list = (devices_t *) malloc(sizeof(devices_t));
    }
    devices_list->driver_id = driver_id;
    devices_list->options = options;
    devices_list->filename = filename;
    devices_list->device = NULL;
    devices_list->next_device = NULL;

    return devices_list;
}

size_t devices_write(void *ptr, size_t size, size_t nmemb, devices_t * d, char iseos)
{
  size_t i, total = 0;
  devices_t * start = d;
  for (i=0; i < nmemb; i++) {
    d = start;
    while (d != NULL) {
      int ret = ao_play(d->device, ptr, size);
      if (ret < size)
	return total + ret;
      total += ret;
      d = d->next_device;
    }
  }
  return total;
}

int add_option(ao_option ** op_h, const char *optstring)
{
    char *key, *value;
    int result;

    key = strdup(optstring);
    if (key == NULL)
	return 0;

    value = strchr(key, ':');
    if (value == NULL) {
	free(key);
	return 0;
    }

    /* split by replacing the separator with a null */
    *value++ = '\0';

    result = ao_append_option(op_h, key, value);
    free(key);

    return (result);
}

int get_default_device(void)
{
    FILE *fp;
    char filename[FILENAME_MAX];
    char line[100];
    char *device = NULL;
    char *homedir = getenv("HOME");
    int i;

    /* Maybe I'm being extremely paranoid, but if ogg123 is ever suid
       root (to access audio devices), this is a possible buffer overflow. */
    if (homedir == NULL || strlen(homedir) >= FILENAME_MAX - 10)
	return -1;

    strncpy(filename, homedir, FILENAME_MAX);
    strcat(filename, "/.ogg123rc");

    fp = fopen(filename, "r");
    /* if no ~/.ogg123rc can be found, try /etc/ogg123rc instead */
    if (!fp) fp = fopen("/etc/ogg123rc", "r");

    /* This is a very simplistic parser. If more options are ever added,
       it will need a serious overhaul. */
    if (fp) {
      while (fgets(line, 100, fp)) {
	if (strncmp(line, "default_device=", 15) == 0) {
	  device = &line[15];
	  for (i = 0; i < strlen(device); i++)
	    if (device[i] == '\n' || device[i] == '\r')
	      device[i] = 0;
	}
      }
      fclose(fp);
    }
    
    if (device)
      return ao_driver_id(device);
    
    return -1;
}

void close_audio_devices (devices_t *devices)
{
  devices_t *current = devices;
  while (current != NULL) {
    if (current->device)
      ao_close(current->device);
    current->device = NULL;
    current = current->next_device;
  }
}

void free_audio_devices (devices_t *devices)
{
  devices_t *current;
  while (devices != NULL) {
    current = devices->next_device;
    free (devices);
    devices = current;
  }
}

void ao_onexit (int exitcode, void *arg)
{
  devices_t *devices = (devices_t *) arg;

  close_audio_devices (devices);
  free_audio_devices (devices);

  ao_shutdown();
}
