/* ogg123.c by Kenneth Arnold <ogg123@arnoldnet.net> */
/* Modified to use libao by Stan Seibert <volsung@asu.edu> */

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

 last mod: $Id: ogg123.c,v 1.39.2.13 2001/08/12 03:59:31 kcarnold Exp $

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

#include "ogg123.h"
#include "ao_interface.h"
#include "curl_interface.h"
#include "buffer.h"
#include "options.h"
#include "status.h"

/* take buffer out of the data segment, not the stack */
#define BUFFER_CHUNK_SIZE 4096
char convbuffer[BUFFER_CHUNK_SIZE];
int convsize = BUFFER_CHUNK_SIZE;
buf_t * InBuffer = NULL;
buf_t * OutBuffer = NULL;

static char skipfile_requested;
static char exit_requested;

struct {
    char *key;			/* includes the '=' for programming convenience */
    char *formatstr;		/* formatted output */
} ogg_comment_keys[] = {
  {"ARTIST=", "Artist: %s\n"},
  {"ALBUM=", "Album: %s\n"},
  {"TITLE=", "Title: %s\n"},
  {"VERSION=", "Version: %s\n"},
  {"TRACKNUMBER=", "Track number: %s\n"},
  {"ORGANIZATION=", "Organization: %s\n"},
  {"GENRE=", "Genre: %s\n"},
  {"DESCRIPTION=", "Description: %s\n"},
  {"DATE=", "Date: %s\n"},
  {"LOCATION=", "Location: %s\n"},
  {"COPYRIGHT=", "Copyright %s\n"},
  {NULL, NULL}
};

struct option long_options[] = {
  /* GNU standard options */
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    /* ogg123-specific options */
    {"buffer", required_argument, 0, 'b'},
    {"config", optional_argument, 0, 'c'},
    {"device", required_argument, 0, 'd'},
    {"file", required_argument, 0, 'f'},
    {"skip", required_argument, 0, 'k'},
    {"delay", required_argument, 0, 'l'},
    {"device-option", required_argument, 0, 'o'},
    {"prebuffer", required_argument, 0, 'p'},
    {"quiet", no_argument, 0, 'q'},
    {"save", required_argument, 0, 's'},
    {"verbose", no_argument, 0, 'v'},
    {"nth", required_argument, 0, 'x'},
    {"ntimes", required_argument, 0, 'y'},
    {"shuffle", no_argument, 0, 'z'},
    {0, 0, 0, 0}
};

int ConfigErrorFunc (void *arg, ParseCode pcode, int lineno, char *filename, char *line)
{
  if (pcode == parse_syserr)
    {
      if (errno != EEXIST && errno != ENOENT)
	perror ("System error");
      return -1;
    }
  else
    {
      fprintf (stderr, "Parse error: %s on line %d of %s (%s)\n", ParseErr(pcode), lineno, filename, line);
      return 0;
    }
}

ParseCode ReadConfig (Option_t opts[], char *filename)
{
  return ParseFile (opts, filename, ConfigErrorFunc, NULL);
}

void ReadStdConfigs (Option_t opts[])
{
  char filename[FILENAME_MAX];
  char *homedir = getenv("HOME");

  /* Read config from files in same order as original parser */
  if (homedir && strlen(homedir) < FILENAME_MAX - 10) {
    /* Try ~/.ogg123 */
    strncpy (filename, homedir, FILENAME_MAX);
    strcat (filename, "/.ogg123rc");
    ReadConfig (opts, filename);
  }
  ReadConfig (opts, "/etc/ogg123rc");
}

void usage(void)
{
  FILE *o = stderr;
  int i, driver_count;
  ao_info **devices = ao_driver_info_list(&driver_count);
  
  fprintf(o,
	  "Ogg123 from " PACKAGE " " VERSION "\n"
	  " by Kenneth Arnold <kcarnold@arnoldnet.net> and others\n\n"
	  "Usage: ogg123 [<options>] <input file> ...\n\n"
	  "  -h, --help     this help\n"
	  "  -V, --version  display Ogg123 version\n"
	  "  -d, --device=d uses 'd' as an output device\n"
	  "      Possible devices are:\n"
	  "        ");
  
  for(i = 0; i < driver_count; i++)
    fprintf(o,"%s ",devices[i]->short_name);
  
  fprintf(o,"\n");
  
  fprintf(o,
	  "  -f, --file=filename  Set the output filename for a previously\n"
	  "      specified file device (with -d).\n"
	  "  -k n, --skip n  Skip the first 'n' seconds\n"
	  "  -o, --device-option=k:v passes special option k with value\n"
	  "      v to previously specified device (with -d).  See\n"
	  "      man page for more info.\n"
	  "  -b n, --buffer n  use a buffer of approximately 'n' kilobytes\n"
	  "  -p n, --prebuffer n  prebuffer n%% of the buffer before playing\n"
	  "  -v, --verbose  display progress and other status information\n"
	  "  -q, --quiet    don't display anything (no title)\n"
	  "  -z, --shuffle  shuffle play\n"
	  "\n"
	  "ogg123 will skip to the next song on SIGINT (Ctrl-C) after s seconds after\n"
	  "song start.\n"
	  "  -l, --delay=s  set s (default 1). If s=-1, disable song skip.\n");
}

int main(int argc, char **argv)
{
  ogg123_options_t opt;
  int ret;
  int option_index = 1;
  ao_option *temp_options = NULL;
  ao_option ** current_options = &temp_options;
  ao_info *info;
  int temp_driver_id = -1;
  devices_t *current;

  /* data used just to initialize the pointers */
  char char_n = 'n';
  long int_10000 = 10000;
  float float_50f = 50.0f;
  float float_0f = 0.0f;
  long int_1 = 1;
  long int_0 = 0;

  /* *INDENT-OFF* */
  Option_t opts[] = {
    /* found, name, description, type, ptr, default */
    {0, "default_device", "default output device", opt_type_string, &opt.default_device, NULL},
    {0, "shuffle",        "shuffle playlist",      opt_type_char,   &opt.shuffle,        &char_n},
    {0, "verbose",        "be verbose",            opt_type_int,    &opt.verbose,        &int_0},
    {0, "quiet",          "be quiet",              opt_type_int,    &opt.quiet,          &int_0},
    {0, "outbuffer",      "out buffer size (kB)",  opt_type_int,    &opt.outbuffer_size, &int_0},
    {0, "outprebuffer",   "out prebuffer (%)",     opt_type_float,  &opt.outprebuffer,   &float_0f},
    {0, "inbuffer",       "in buffer size (kB)",   opt_type_int,    &opt.inputOpts.BufferSize, &int_10000},
    {0, "inprebuffer",    "in prebuffer (%)",      opt_type_float,  &opt.inputOpts.Prebuffer, &float_50f},
    {0, "save_stream",    "append stream to file", opt_type_string, &opt.inputOpts.SaveStream, NULL},
    {0, "delay",          "skip file delay (sec)", opt_type_int,    &opt.delay,          &int_1},
    {0, NULL,             NULL,                    0,               NULL,                NULL}
  };
  /* *INDENT-ON* */

  memset (&opt, 0, sizeof(opt));
  
  opt.delay = 1;
  opt.nth = 1;
  opt.ntimes = 1;

  on_exit (ogg123_onexit, &opt);
  signal (SIGINT, signal_quit);
  ao_initialize();
  
  InitOpts(opts);
  ReadStdConfigs (opts);

    while (-1 != (ret = getopt_long(argc, argv, "b:c::d:f:hl:k:o:p:qvVx:y:z",
				    long_options, &option_index))) {
	switch (ret) {
	case 0:
	    fprintf(stderr,
		    "Internal error: long option given when none expected.\n");
	    exit(1);
	case 'b':
	  opt.outbuffer_size = atoi(optarg);
	  break;
	case 'c':
	  if (optarg)
	    {
	      char *tmp = strdup (optarg);
	      ParseCode pcode = ParseLine (opts, tmp);
	      if (pcode != parse_ok)
		fprintf (stderr,
			 "Error parsing config option from command line.\n"
			 "Error: %s\n"
			 "Option was: %s\n", ParseErr (pcode), optarg);
	      free (tmp);
	    }
	  else {
	    fprintf (stdout, "Available options:\n");
	    DescribeOptions (opts, stdout);
	    exit (0);
	  }
	  break;
	case 'd':
	    temp_driver_id = ao_driver_id(optarg);
	    if (temp_driver_id < 0) {
		fprintf(stderr, "No such device %s.\n", optarg);
		exit(1);
	    }
	    current = append_device(opt.outdevices, temp_driver_id, 
				    NULL, NULL);
	    if(opt.outdevices == NULL)
		    opt.outdevices = current;
	    current_options = &current->options;
	    break;
	case 'f':
	    info = ao_driver_info(temp_driver_id);
	    if (info->type == AO_TYPE_FILE) {
	        free(current->filename);
		current->filename = strdup(optarg);
	    } else {
	        fprintf(stderr, "Driver %s is not a file output driver.\n",
			info->short_name);
	        exit(1);
	    }
	    break;
	case 'k':
	    opt.seekpos = atof(optarg);
	    break;
	case 'l':
	    opt.delay = atoi(optarg);
	    break;
	case 'o':
	    if (optarg && !add_option(current_options, optarg)) {
		fprintf(stderr, "Incorrect option format: %s.\n", optarg);
		exit(1);
	    }
	    break;
	case 'h':
	    usage();
	    exit(0);
	case 'p':
	  opt.outprebuffer = atof (optarg);
	  if (opt.outprebuffer < 0.0f || opt.outprebuffer > 100.0f)
	    {
	      fprintf (stderr, "Prebuffer value invalid. Range is 0-100, using nearest value.\n");
	      opt.outprebuffer = opt.outprebuffer < 0.0f ? 0.0f : 100.0f;
	    }
	  break;
	case 'q':
	    opt.quiet++;
	    break;
	case 'v':
	    opt.verbose++;
	    break;
	case 'V':
	    fprintf(stderr, "Ogg123 from " PACKAGE " " VERSION "\n");
	    exit(0);
	case 'x':
	  opt.nth = atoi (optarg);
	  break;
	case 'y':
	  opt.ntimes = atoi (optarg);
	  break;
	case 'z':
	    opt.shuffle = 1;
	    break;
	case '?':
	    break;
	default:
	    usage();
	    exit(1);
	}
    }

    if (optind == argc) {
	usage();
	exit(1);
    }

    /* Add last device to device list or use the default device */
    if (temp_driver_id < 0) {
      if (opt.default_device) {
	temp_driver_id = ao_driver_id (opt.default_device);
	if (temp_driver_id < 0 && opt.quiet < 2)
	  fprintf (stderr, "Warning: driver %s specified in configuration file invalid.\n", opt.default_device);
      }
    }

    if (temp_driver_id < 0) {
      temp_driver_id = ao_default_driver_id();
    }

    if (temp_driver_id < 0) {
      fprintf(stderr,
	      "Could not load default driver and no driver specified in config file. Exiting.\n");
      exit(1);
    }
    opt.outdevices = append_device(opt.outdevices, temp_driver_id, 
				   temp_options, NULL);

    opt.inputOpts.BufferSize *= 1024;

    if (opt.outbuffer_size)
      {
	opt.outbuffer_size *= 1024;
	opt.outprebuffer = (opt.outprebuffer * (float) opt.outbuffer_size / 100.0f);
	OutBuffer = StartBuffer (opt.outbuffer_size, (int) opt.outprebuffer,
				 opt.outdevices, (pWriteFunc) devices_write,
				 NULL, NULL, 4096);
      }
    
    if (opt.shuffle == 'n' || opt.shuffle == 'N')
      opt.shuffle = 0;
    else if (opt.shuffle == 'y' || opt.shuffle == 'Y')
      opt.shuffle = 1;

    if (opt.shuffle) {
	int i;
	int range = argc - optind;
	
	srandom(time(NULL));

	for (i = optind; i < argc; i++) {
	  int j = optind + random() % range;
	  char *temp = argv[i];
	  argv[i] = argv[j];
	  argv[j] = temp;
	}
    }

    while (optind < argc) {
	opt.read_file = argv[optind];
	play_file(&opt);
	optind++;
    }

    if (OutBuffer != NULL) {
      buffer_WaitForEmpty (OutBuffer);
    }
    
    exit (0);
}

/* Two signal handlers, one for SIGINT, and the second for
 * SIGALRM.  They are de/activated on an as-needed basis by the
 * player to allow the user to stop ogg123 or skip songs.
 */

void signal_skipfile(int which_signal)
{
  skipfile_requested = 1;

  /* libao, when writing wav's, traps SIGINT so it correctly
   * closes things down in the event of an interrupt.  We
   * honour this.   libao will re-raise SIGINT once it cleans
   * up properly, causing the application to exit.  This is 
   * desired since we would otherwise re-open output.wav 
   * and blow away existing "output.wav" file.
   */

  signal (SIGINT, signal_quit);
}

void signal_activate_skipfile(int ignored)
{
  signal(SIGINT,signal_skipfile);
}

void signal_quit(int ignored)
{
  exit_requested = 1;
  if (OutBuffer)
    buffer_flush (OutBuffer);
}

#if 0
/* from vorbisfile.c */
static int _fseek64_wrap(FILE *f,ogg_int64_t off,int whence)
{
  if(f==NULL)return(-1);
  return fseek(f,(int)off,whence);
}
#endif

void play_file(ogg123_options_t *opt)
{
  OggVorbis_File vf;
  int current_section = -1, eof = 0, eos = 0, ret;
  int old_section = -1;
  long t_min = 0, c_min = 0, r_min = 0;
  double t_sec = 0, c_sec = 0, r_sec = 0;
  int is_big_endian = ao_is_big_endian();
  double realseekpos = opt->seekpos;
  int nthc = 0, ntimesc = 0;
  double u_time, u_pos;
  int tmp;
  ov_callbacks VorbisfileCallbacks;
  
  tmp = strchr(opt->read_file, ':') - opt->read_file;
  if (tmp < 10 && tmp + 2 < strlen(opt->read_file) && !strncmp(opt->read_file + tmp, "://", 3))
    {
      /* let's call this a URL. */
      if (opt->quiet < 1)
	fprintf (stderr, "Playing from stream %s\n", opt->read_file);
      VorbisfileCallbacks.read_func = StreamBufferRead;
      VorbisfileCallbacks.seek_func = StreamBufferSeek;
      VorbisfileCallbacks.close_func = StreamBufferClose;
      VorbisfileCallbacks.tell_func = StreamBufferTell;
      
      opt->inputOpts.URL = opt->read_file;
      InBuffer = InitStream (opt->inputOpts);
      if ((ov_open_callbacks (InBuffer->data, &vf, NULL, 0, VorbisfileCallbacks)) < 0) {
	fprintf(stderr, "Error: input not an Ogg Vorbis audio stream.\n");
	return;
      }
      
    }
  else
    {
#if 0
      VorbisfileCallbacks.read_func = fread;
      VorbisfileCallbacks.seek_func = _fseek64_wrap;
      VorbisfileCallbacks.close_func = fclose;
      VorbisfileCallbacks.tell_func = ftell;
#endif
      if (strcmp(opt->read_file, "-"))
	{
	  if (opt->quiet < 1)
	    fprintf(stderr, "Playing from file %s.\n", opt->read_file);
	  /* Open the file. */
	  if ((opt->instream = fopen(opt->read_file, "rb")) == NULL) {
	    fprintf(stderr, "Error opening input file.\n");
	    exit(1);
	  }
	}
      else
	{
	  if (opt->quiet < 1)
	    fprintf(stderr, "Playing from standard input.\n");
	  opt->instream = stdin;
	}
      if ((ov_open (opt->instream, &vf, NULL, 0)) < 0) {
	fprintf(stderr, "Error: input not an Ogg Vorbis audio stream.\n");
	return;
      }
    }
    
    /* Setup so that pressing ^C in the first second of playback
     * interrupts the program, but after the first second, skips
     * the song.  This functionality is similar to mpg123's abilities. */
    
    if (opt->delay > 0) {
      skipfile_requested = 0;
      signal(SIGALRM,signal_activate_skipfile);
      alarm(opt->delay);
    }
    
    exit_requested = 0;
    
    while (!eof && !exit_requested) {
      int i;
      vorbis_comment *vc = ov_comment(&vf, -1);
      vorbis_info *vi = ov_info(&vf, -1);
      
      if(open_audio_devices(opt, vi->rate, vi->channels) < 0)
	exit(1);

	if (opt->quiet < 1) {
	    if (eos && opt->verbose) fprintf (stderr, "\r                                                                          \r\n");
	    for (i = 0; i < vc->comments; i++) {
		char *cc = vc->user_comments[i];	/* current comment */
		int i;

		for (i = 0; ogg_comment_keys[i].key != NULL; i++)
		    if (!strncasecmp
			(ogg_comment_keys[i].key, cc,
			 strlen(ogg_comment_keys[i].key))) {
			fprintf(stderr, ogg_comment_keys[i].formatstr,
				cc + strlen(ogg_comment_keys[i].key));
			break;
		    }
		if (ogg_comment_keys[i].key == NULL)
		    fprintf(stderr, "Unrecognized comment: '%s'\n", cc);
	    }

	    fprintf(stderr, "\nBitstream is %d channel, %ldHz\n",
		    vi->channels, vi->rate);
	    if (opt->verbose > 1)
	      fprintf(stderr, "Encoded by: %s\n\n", vc->vendor);
	}

	if (opt->verbose > 0 && ov_seekable(&vf)) {
	    /* Seconds with double precision */
	    u_time = ov_time_total(&vf, -1);
	    t_min = (long) u_time / (long) 60;
	    t_sec = u_time - 60 * t_min;
	}

	if ((realseekpos > ov_time_total(&vf, -1)) || (realseekpos < 0))
	    /* If we're out of range set it to right before the end. If we set it
	     * right to the end when we seek it will go to the beginning of the song */
	    realseekpos = ov_time_total(&vf, -1) - 0.01;

	if (realseekpos > 0)
	    ov_time_seek(&vf, realseekpos);

	eos = 0;

	while (!eos && !exit_requested) {

	    if (skipfile_requested) {
	      eof = eos = 1;
	      skipfile_requested = 0;
	      signal(SIGALRM,signal_activate_skipfile);
	      alarm(opt->delay);
	      if (OutBuffer)
		buffer_flush (OutBuffer);
	      break;
  	    }

	    old_section = current_section;
	    ret =
		ov_read(&vf, convbuffer, sizeof(convbuffer), is_big_endian,
			2, 1, &current_section);
	    if (ret == 0) {
		/* End of file */
		eof = eos = 1;
	    } else if (ret == OV_HOLE) {
	      if (opt->verbose > 1) 
		/* we should be able to resync silently; if not there are 
		   bigger problems. */
		fprintf (stderr, "Warning: hole in the stream; probably harmless\n");
	    } else if (ret < 0) {
	      /* Stream error */
	      fprintf(stderr, "Error: libvorbis reported a stream error.\n");
	    } else {
		/* did we enter a new logical bitstream */
		if (old_section != current_section && old_section != -1)
		    eos = 1;

		do {
		  if (nthc-- == 0) {
		    if (OutBuffer)
			SubmitData (OutBuffer, convbuffer, ret, 1);
		    else
		      devices_write(convbuffer, ret, 1, opt->outdevices, 0);
		    nthc = opt->nth - 1;
		  }
		} while (++ntimesc < opt->ntimes);
		ntimesc = 0;

		if (opt->verbose > 0) {
		    if (ov_seekable (&vf)) {
		      u_pos = ov_time_tell(&vf);
		      c_min = (long) u_pos / (long) 60;
		      c_sec = u_pos - 60 * c_min;
		      r_min = (long) (u_time - u_pos) / (long) 60;
		      r_sec = (u_time - u_pos) - 60 * r_min;
		      if (OutBuffer)
			fprintf(stderr,
				"\rTime: %02li:%05.2f [%02li:%05.2f] of %02li:%05.2f, Bitrate: %5.1f, Buffer fill: %3.0f%%   \r",
				c_min, c_sec, r_min, r_sec, t_min, t_sec,
				(double) ov_bitrate_instant(&vf) / 1000.0F,
				(double) buffer_full(OutBuffer) / (double) OutBuffer->size * 100.0F);
		      else
			fprintf(stderr,
				"\rTime: %02li:%05.2f [%02li:%05.2f] of %02li:%05.2f, Bitrate: %5.1f   \r",
				c_min, c_sec, r_min, r_sec, t_min, t_sec,
				(double) ov_bitrate_instant(&vf) / 1000.0F);
		    } else {
		      /* working around a bug in vorbisfile */
		      /* I don't think that bug is there anymore -ken */
		      u_pos = (double) ov_pcm_tell(&vf) / (double) vi->rate;
		      c_min = (long) u_pos / (long) 60;
		      c_sec = u_pos - 60 * c_min;
		      if (OutBuffer)
			fprintf(stderr,
				"\rTime: %02li:%05.2f, Bitrate: %5.1f, Buffer fill: %3.0f%%   \r",
				c_min, c_sec,
				(float) ov_bitrate_instant (&vf) / 1000.0F,
				(double) buffer_full(OutBuffer) / (double) OutBuffer->size * 100.0F);
		      else
			fprintf(stderr,
				"\rTime: %02li:%05.2f, Bitrate: %5.1f   \r",
				c_min, c_sec,
				(float) ov_bitrate_instant (&vf) / 1000.0F);
		    }
		}
	    }
	}
    }
    
    alarm(0);
    signal(SIGALRM,SIG_DFL);
    signal(SIGINT,signal_quit);
    
    ov_clear(&vf);

    if (opt->quiet < 1)
	fprintf(stderr, "\nDone.\n");

    if (exit_requested)
      {
	exit (0);
      }
}

/* if not for the two lines involving the buffer, this would go in
 * ao_interface.c. */
int open_audio_devices(ogg123_options_t *opt, int rate, int channels)
{
  static int prevrate=0, prevchan=0;
  devices_t *current;
  ao_sample_format format;

  if(prevrate == rate && prevchan == channels)
    return 0;
  
  if(prevrate !=0 && prevchan!=0)
	{
	  if (OutBuffer)
	    buffer_WaitForEmpty (OutBuffer);

	  close_audio_devices (opt->outdevices);
	}
  
  format.rate = prevrate = rate;
  format.channels = prevchan = channels;
  format.bits = 16;
  format.byte_format = AO_FMT_NATIVE;

  current = opt->outdevices;
  while (current != NULL) {
    ao_info *info = ao_driver_info(current->driver_id);
    
    if (opt->verbose > 0) {
      fprintf(stderr, "Device:   %s\n", info->name);
      fprintf(stderr, "Author:   %s\n", info->author);
      fprintf(stderr, "Comments: %s\n", info->comment);
      fprintf(stderr, "\n");	
    }
    
    if (current->filename == NULL)
      current->device = ao_open_live(current->driver_id, &format,
				     current->options);
    else
      current->device = ao_open_file(current->driver_id, current->filename,
				     0, &format, current->options);

    if (current->device == NULL) {
      switch (errno) {
      case AO_ENODRIVER:
	fprintf(stderr, "Error: Device not available.\n");
	break;
      case AO_ENOTLIVE:
	fprintf(stderr, "Error: %s requires an output filename to be specified with -f.\n", info->short_name);
	break;
      case AO_EBADOPTION:
	fprintf(stderr, "Error: Unsupported option value to %s device.\n",
		info->short_name);
	break;
      case AO_EOPENDEVICE:
	fprintf(stderr, "Error: Cannot open device %s.\n",
		info->short_name);
	break;
      case AO_EFAIL:
	fprintf(stderr, "Error: Device failure.\n");
	break;
      case AO_ENOTFILE:
	fprintf(stderr, "Error: An output file cannot be given for %s device.\n", info->short_name);
	break;
      case AO_EOPENFILE:
	fprintf(stderr, "Error: Cannot open file %s for writing.\n",
		current->filename);
	break;
      case AO_EFILEEXISTS:
	fprintf(stderr, "Error: File %s already exists.\n", current->filename);
	break;
      default:
	fprintf(stderr, "Error: This error should never happen.  Panic!\n");
	break;
      }
	
      return -1;
    }
    current = current->next_device;
  }
  
  return 0;
}

void ogg123_onexit (int exitcode, void *arg)
{
  ogg123_options_t *opt = (ogg123_options_t*) arg;

  if (InBuffer) {
    StreamInputCleanup (InBuffer);
    InBuffer = NULL;
  }
      
  if (OutBuffer) {
    buffer_cleanup (OutBuffer);
    OutBuffer = NULL;
  }

  ao_onexit (exitcode, opt->outdevices);
}
