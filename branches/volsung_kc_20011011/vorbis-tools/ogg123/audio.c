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

 last mod: $Id: audio.c,v 1.1.2.2 2001/12/09 03:45:26 volsung Exp $

 ********************************************************************/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "audio.h"


audio_device_t *append_audio_device(audio_device_t *devices_list,
				     int driver_id,
				     ao_option *options, char *filename)
{
  audio_device_t *head = devices_list;
  
  if (devices_list != NULL) {
    while (devices_list->next_device != NULL)
      devices_list = devices_list->next_device;
    devices_list = devices_list->next_device =
      malloc(sizeof(audio_device_t));
  } else {
    head = devices_list = (audio_device_t *) malloc(sizeof(audio_device_t));
  }
  devices_list->driver_id = driver_id;
  devices_list->options = options;
  devices_list->filename = filename;
  devices_list->device = NULL;
  devices_list->next_device = NULL;
  
  return devices_list;
}


int audio_devices_write(audio_device_t *d, void *ptr, int nbytes)
{
  audio_device_t *start = d;

  d = start;
  while (d != NULL) {
    if (ao_play(d->device, ptr, nbytes) == 0)
      return 0; /* error occurred */
    d = d->next_device;
  }

  return 1;
}

int add_ao_option(ao_option **op_h, const char *optstring)
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

void close_audio_devices (audio_device_t *devices)
{
  audio_device_t *current = devices;

  while (current != NULL) {
    if (current->device)
      ao_close(current->device);
    current->device = NULL;
    current = current->next_device;
  }
}

void free_audio_devices (audio_device_t *devices)
{
  audio_device_t *current;

  while (devices != NULL) {
    current = devices->next_device;
    free (devices);
    devices = current;
  }
}

void ao_onexit (void *arg)
{
  audio_device_t *devices = (audio_device_t *) arg;

  close_audio_devices (devices);
  free_audio_devices (devices);

  ao_shutdown();
}


int audio_play_callback (void *ptr, int nbytes, int eos, void *arg)
{
  audio_play_arg_t *play_arg = (audio_play_arg_t *) arg;
  int ret;

  ret = audio_devices_write(play_arg->devices, ptr, nbytes);

  status_print_statistics(play_arg->stats);

  return ret ? nbytes : 0;
}

void audio_reopen_callback (buf_t *buf, void *arg)
{
  audio_reopen_arg_t *reopen_arg = (audio_reopen_arg_t *) arg;
  audio_device_t *current;
  ao_sample_format format;

  /* We DO NOT want to get cancelled part way through this and have our
     audio devices in an unknown state */
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);


  close_audio_devices (reopen_arg->devices);
  
  /* Record audio device settings and open the devices */
  format.rate = reopen_arg->format->rate;
  format.channels = reopen_arg->format->channels;
  format.bits = reopen_arg->format->word_size * 8;
  format.byte_format = reopen_arg->format->big_endian ? 
    AO_FMT_BIG : AO_FMT_LITTLE;

  current = reopen_arg->devices;

  while (current != NULL) {
    ao_info *info = ao_driver_info(current->driver_id);
    
    status_message(1, "\nDevice:   %s", info->name);
    status_message(1, "Author:   %s", info->author);
    status_message(1, "Comments: %s\n", info->comment);
    
    if (current->filename == NULL)
      current->device = ao_open_live(current->driver_id, &format,
				     current->options);
    else
      current->device = ao_open_file(current->driver_id, current->filename,
				     0, &format, current->options);
    
    /* Report errors */
    if (current->device == NULL) {
      switch (errno) {
      case AO_ENODRIVER:
        status_error("Error: Device not available.\n");
	break;
      case AO_ENOTLIVE:
	status_error("Error: %s requires an output filename to be specified with -f.\n", info->short_name);
	break;
      case AO_EBADOPTION:
	status_error("Error: Unsupported option value to %s device.\n",
		     info->short_name);
	break;
      case AO_EOPENDEVICE:
	status_error("Error: Cannot open device %s.\n",
		     info->short_name);
	break;
      case AO_EFAIL:
	status_error("Error: Device failure.\n");
	break;
      case AO_ENOTFILE:
	status_error("Error: An output file cannot be given for %s device.\n", info->short_name);
	break;
      case AO_EOPENFILE:
	status_error("Error: Cannot open file %s for writing.\n",
		     current->filename);
	break;
      case AO_EFILEEXISTS:
	status_error("Error: File %s already exists.\n", current->filename);
	break;
      default:
	status_error("Error: This error should never happen.  Panic!\n");
	break;
      }
	 
      /* We cannot recover from any of these errors */
      exit(1);      
    }
    
    current = current->next_device;
  }

  /* Cleanup argument */
  free(reopen_arg->format);
  free(reopen_arg);
  
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);  
}


audio_reopen_arg_t *new_audio_reopen_arg (audio_device_t *devices,
					  audio_format_t *fmt)
{
  audio_reopen_arg_t *arg;

  if ( (arg = malloc(sizeof(audio_reopen_arg_t))) == NULL ) {
    status_error("Error: Out of memory in new_audio_reopen_arg().\n");
    exit(1);
  }  
  
  if ( (arg->format = malloc(sizeof(audio_format_t))) == NULL ) {
    status_error("Error: Out of memory in new_audio_reopen_arg().\n");
    exit(1);
  }  
  
  arg->devices = devices;
  /* Copy format in case fmt is recycled later */
  audio_format_copy(arg->format, fmt);

  return arg;
}
