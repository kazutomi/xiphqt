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

 last mod: $Id: callbacks.c,v 1.1.2.2 2001/12/12 15:52:25 volsung Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>

#include "callbacks.h"

/* Audio callbacks */

int audio_play_callback (void *ptr, int nbytes, int eos, void *arg)
{
  audio_play_arg_t *play_arg = (audio_play_arg_t *) arg;
  int ret;

  ret = audio_devices_write(play_arg->devices, ptr, nbytes);

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
	status_error("Error: Device %s failure.\n", info->short_name);
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
  *arg->format = *fmt;

  return arg;
}


/* Statistics callbacks */

void print_statistics_callback (buf_t *buf, void *arg)
{
  print_statistics_arg_t *stats_arg = (print_statistics_arg_t *) arg;
  buffer_stats_t *buffer_stats;

  if (buf != NULL)
    buffer_stats = buffer_statistics(buf);
  else
    buffer_stats = NULL;

  status_print_statistics(stats_arg->stat_format,
			  buffer_stats,
			  stats_arg->transport_statistics,
			  stats_arg->decoder_statistics);

  free(stats_arg->transport_statistics);
  free(stats_arg->decoder_statistics);
  free(stats_arg);
}


print_statistics_arg_t *new_print_statistics_arg (
			       stat_format_t *stat_format,
			       data_source_stats_t *transport_statistics,
			       decoder_stats_t *decoder_statistics)
{
  print_statistics_arg_t *arg;

  if ( (arg = malloc(sizeof(print_statistics_arg_t))) == NULL ) {
    status_error("Error: Out of memory in new_print_statistics_arg().\n");
    exit(1);
  }  
  
  arg->stat_format = stat_format;
  arg->transport_statistics = transport_statistics;
  arg->decoder_statistics = decoder_statistics;

  return arg;
}
