/* ogg123.c by Kenneth Arnold <ogg123@arnoldnet.net> */
/* Maintained by Stan Seibert <volsung@xiph.org> */

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

 last mod: $Id: ogg123.c,v 1.39.2.30.2.16 2001/12/11 18:46:23 volsung Exp $

 ********************************************************************/

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "audio.h"
#include "buffer.h"
#include "callbacks.h"
#include "cfgfile_options.h"
#include "cmdline_options.h"
#include "format.h"
#include "transport.h"
#include "status.h"

#include "ogg123.h"

void exit_cleanup ();
void play (char *source_string);

/* take buffer out of the data segment, not the stack */
#define AUDIO_CHUNK_SIZE 4096
unsigned char convbuffer[AUDIO_CHUNK_SIZE];
int convsize = AUDIO_CHUNK_SIZE;

ogg123_options_t options;
stat_format_t *stat_format;
buf_t *audio_buffer;

audio_play_arg_t audio_play_arg;


/* ------------------------- config file options -------------------------- */

/* This macro is used to create some dummy variables to hold default values
   for the options. */
#define INIT(type, value) type type##_##value = value
char char_n = 'n';
float float_50f = 50.0f;
float float_0f = 0.0f;
INIT(int, 10000);
INIT(int, 1);
INIT(int, 0);

file_option_t file_opts[] = {
  /* found, name, description, type, ptr, default */
  {0, "default_device", "default output device", opt_type_string,
   &options.default_device, NULL},
  {0, "shuffle",        "shuffle playlist",      opt_type_bool,
   &options.shuffle,        &int_0},
  {0, "verbose",        "verbosity level",       opt_type_int,
   &options.verbosity,        &int_1},
  {0, "outbuffer",      "out buffer size (kB)",  opt_type_int,
   &options.buffer_size, &int_0},
  {0, "outprebuffer",   "out prebuffer (%)",     opt_type_float,
   &options.prebuffer,   &float_0f},
  {0, NULL,             NULL,                    0,               NULL,                NULL}
};


/* Flags set by the signal handler to control the threads */
signal_request_t sig_request = {0, 0, 0, 0};


/* ------------------------------- signal handler ------------------------- */


void signal_handler (int signo)
{
  switch (signo) {
  case SIGALRM:
    sig_request.ticks++;
    if (sig_request.ticks < options.delay)
      alarm(1);
    break;

  case SIGINT:
    if (sig_request.ticks < options.delay)
      sig_request.exit = 1;
    else
      sig_request.skipfile = 1;
    break;

  case SIGTSTP:
    sig_request.pause = 1;
    /* buffer_Pause (Options.outputOpts.buffer);
       buffer_WaitForPaused (Options.outputOpts.buffer);
       }
       if (Options.outputOpts.devicesOpen == 0) {
       close_audio_devices (Options.outputOpts.devices);
       Options.outputOpts.devicesOpen = 0;
       }
    */
    /* open_audio_devices();
       if (Options.outputOpts.buffer) {
       buffer_Unpause (Options.outputOpts.buffer);
       }
    */
    break;

  case SIGCONT:
    break;  /* Don't need to do anything special to resume */

  default:
    psignal (signo, "Unknown signal caught");
  }
}

/* -------------------------- util functions ---------------------------- */

void options_init (ogg123_options_t *opts)
{
  opts->verbosity = 1;
  opts->shuffle = 0;
  opts->delay = 2;
  opts->nth = 1;
  opts->ntimes = 1;
  opts->seekpos = 0.0;
  opts->buffer_size = 0;
  opts->prebuffer = 0.0f;
  opts->default_device = NULL;

  opts->status_freq = 4;
}

/* ----------------------------- callbacks ------------------------------ */

void decoder_error_callback (void *arg, int severity, char *message, ...)
{
  va_list ap;

  va_start(ap, message);
  vstatus_error(message, ap);
  va_end(ap);
}

void decoder_metadata_callback (void *arg, int verbosity, char *message, ...)
{
  va_list ap;

  va_start(ap, message);
  vstatus_message(verbosity, message, ap);
  va_end(ap);
}


/* --------------------------- main code -------------------------------- */



int main(int argc, char **argv)
{
  int optind;

  ao_initialize();
  stat_format = stat_format_create();
  options_init(&options);
  file_options_init(file_opts);

  parse_std_configs(file_opts);
  optind = parse_cmdline_options(argc, argv, &options, file_opts);

  audio_play_arg.devices = options.devices;
  audio_play_arg.stat_format = stat_format;


  status_set_verbosity(options.verbosity);

  /* Setup signal handlers and callbacks */

  ATEXIT (exit_cleanup);
  signal (SIGINT, signal_handler);
  signal (SIGTSTP, signal_handler);
  signal (SIGCONT, signal_handler);
  signal (SIGALRM, signal_handler);

  /* Do we have anything left to play? */
  if (optind == argc) {
    cmdline_usage();
    exit(1);
  }
  
  /* Setup buffer */ 
  if (options.buffer_size > 0) {
    audio_buffer = buffer_create(options.buffer_size,
				 options.buffer_size * options.prebuffer,
				 audio_play_callback, &audio_play_arg,
				 AUDIO_CHUNK_SIZE);
    if (audio_buffer == NULL) {
      status_error("Error: Could not create audio buffer.\n");
      exit(1);
    }
  } else
    audio_buffer = NULL;


  /* Shuffle playlist */
  if (options.shuffle) {
    int i;
    
    srandom(time(NULL));
    
    for (i = optind; i < argc; i++) {
      int j = optind + random() % (argc - i);
      char *temp = argv[i];
      argv[i] = argv[j];
      argv[j] = temp;
    }
  }
  

  /* Play the files/streams */

  while (optind < argc) {
    play(argv[optind]);
    optind++;
  }


  exit (0);
}


void play (char *source_string)
{
  transport_t *transport;
  format_t *format;
  data_source_t *source;
  decoder_t *decoder;
  decoder_callbacks_t decoder_callbacks = { &decoder_error_callback,
					    &decoder_metadata_callback };
  print_statistics_arg_t *pstats_arg;
  buffer_stats_t *buffer_stats;

  /* Preserve between calls so we only open the audio device when we 
     have to */
  static audio_format_t old_audio_fmt = { 0, 0, 0, 0, 0 };
  audio_format_t new_audio_fmt;
  audio_reopen_arg_t *reopen_arg;

  int eof = 0, eos = 0, ret;
  int nthc = 0, ntimesc = 0;
  int next_status = 0;
  int status_interval = 0;

  new_audio_fmt.big_endian = ao_is_big_endian();
  new_audio_fmt.signed_sample = 1;
  new_audio_fmt.word_size = 2;

  /* Locate and use transport for this data source */  
  if ( (transport = select_transport(source_string)) == NULL ) {
    status_error("No module could be found to read from %s.\n", source_string);
    return;
  }
  
  if ( (source = transport->open(source_string, &options)) == NULL ) {
    status_error("Cannot open %s.\n", source_string);
    return;
  }

  /* Detect the file format and initialize a decoder */
  if ( (format = select_format(source)) == NULL ) {
    status_error("The file format of %s is not supported.\n", source_string);
    return;
  }
  
  if ( (decoder = format->init(source, &options, &new_audio_fmt, 
			       &decoder_callbacks, NULL)) == NULL ) {
    status_error("Error opening %s using the %s module."
		 "  The file may be corrupted.\n", source_string,
		 format->name);
    return;
  }


  /* Reset all of the signal flags and setup the timer */
  sig_request.skipfile = 0;
  sig_request.exit     = 0;
  sig_request.pause    = 0;
  sig_request.ticks    = 0;
  alarm(1); /* Count seconds */


  /* Start the audio playback thread before we begin sending data */    
  if (audio_buffer != NULL) {
    
    /* First reset mutexes and other synchronization variables */
    buffer_reset (audio_buffer);
    buffer_thread_start (audio_buffer);
  }


  /* Main loop:  Iterates over all of the logical bitstreams in the file */
  while (!eof && !sig_request.exit) {
    
    /* Loop through data within a logical bitstream */
    eos = 0;    
    while (!eos && !sig_request.exit) {
      
      /* Check signals */
      if (sig_request.skipfile) {
	eof = eos = 1;
	break;
      }

      if (sig_request.pause) {
	buffer_thread_pause (audio_buffer);
	kill (getpid(), SIGSTOP); /* We block here until we unpause */
	
	/* Done pausing */
	buffer_thread_unpause (audio_buffer);
	sig_request.pause = 0;
      }


      /* Read another block of audio data */
      ret = format->read(decoder, convbuffer, convsize, &eos, &new_audio_fmt);

      /* Bail if we need to */
      if (ret == 0) {
	eof = eos = 1;
	break;
      } else if (ret < 0) {
	status_error("Error: Decoding failure.\n");
	break;
      }

      
      /* Check to see if the audio format has changed */
      if (!audio_format_equal(&new_audio_fmt, &old_audio_fmt)) {
	old_audio_fmt = new_audio_fmt;
	
	/* Update our status printing interval */
	status_interval = new_audio_fmt.word_size * new_audio_fmt.channels * 
	  new_audio_fmt.rate / options.status_freq;
	next_status = 0;

	reopen_arg = new_audio_reopen_arg(options.devices, &new_audio_fmt);

	if (audio_buffer)	  
	  buffer_insert_action_at_end(audio_buffer, &audio_reopen_callback,
				      reopen_arg);
	else
	  audio_reopen_callback(NULL, reopen_arg);
      }
      

      /* Update statistics display if needed */
      if (next_status <= 0) {
	
	pstats_arg = new_print_statistics_arg(stat_format,
					      transport->statistics(source),
					      format->statistics(decoder));
	if (audio_buffer) {
	  /* Place a status update into the buffer */
	  buffer_append_action_at_end(audio_buffer,
				      &print_statistics_callback,
				      pstats_arg);

	  /* And if we are not playing right now, do an immediate
	     update just the output buffer */
	  buffer_stats = buffer_statistics(audio_buffer);
	  if (buffer_stats->paused || buffer_stats->prebuffering) {
	    pstats_arg = new_print_statistics_arg(stat_format,
						  NULL,
						  NULL);
	    print_statistics_callback(audio_buffer, pstats_arg);
	  }

	} else
	  print_statistics_callback(NULL, pstats_arg);
	
	next_status = status_interval;
      } else
	next_status -= ret;

      /* Write audio data block to output, skipping or repeating chunks
	 as needed */
      do {
	
	if (nthc-- == 0) {
	  if (audio_buffer)
	    buffer_submit_data(audio_buffer, convbuffer, ret);
	  else
	    audio_play_callback(convbuffer, ret, eos, &audio_play_arg);
	  
	  nthc = options.nth - 1;
	}
	
      } while (++ntimesc < options.ntimes);

      ntimesc = 0;
            
    } /* End of data loop */
    
  
    if (audio_buffer != NULL)
      buffer_mark_eos(audio_buffer);
  } /* End of logical bitstream loop */
  

  /* Done playing this logical bitstream.  Now we cleanup output buffer. */
  if (audio_buffer) {
    
    if (!sig_request.exit && !sig_request.skipfile)
      buffer_wait_for_empty(audio_buffer);
    
    
    buffer_thread_kill(audio_buffer);
  }
  

  alarm(0);  
  format->cleanup(decoder);
  transport->close(source);
  
  status_message(1, "Done.");
  
  if (sig_request.exit)
    exit (0);
}


void exit_cleanup ()
{
      
  if (audio_buffer != NULL) {
    buffer_destroy (audio_buffer);
    audio_buffer = NULL;
  }

  ao_onexit (options.devices);
}
