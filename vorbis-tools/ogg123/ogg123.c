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

 last mod: $Id: ogg123.c,v 1.39.2.30.2.2 2001/10/15 05:56:55 volsung Exp $

 ********************************************************************/

#define _GNU_SOURCE

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
unsigned char convbuffer[BUFFER_CHUNK_SIZE];
int convsize = BUFFER_CHUNK_SIZE;

/* take big options structure from the data segment */
ogg123_options_t Options;

char skipfile_requested;
char exit_requested;
char pause_requested;

pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

struct {
    const char *key;			/* includes the '=' for programming convenience */
    const char *formatstr;		/* formatted output */
} ogg_comment_keys[] = {
  {"ARTIST=", "Artist: %s"},
  {"ALBUM=", "Album: %s"},
  {"TITLE=", "Title: %s"},
  {"VERSION=", "Version: %s"},
  {"TRACKNUMBER=", "Track number: %s"},
  {"ORGANIZATION=", "Organization: %s"},
  {"GENRE=", "Genre: %s"},
  {"DESCRIPTION=", "Description: %s"},
  {"DATE=", "Date: %s"},
  {"LOCATION=", "Location: %s"},
  {"COPYRIGHT=", "Copyright %s"},
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

/* configuration interface */
int ConfigErrorFunc (void *arg, ParseCode pcode, int lineno, const char *filename, char *line)
{
  if (pcode == parse_syserr)
    {
      if (errno != EEXIST && errno != ENOENT)
	perror ("System error");
      return -1;
    }
  else
    {
      Error ("=== Parse error: %s on line %d of %s (%s)\n", ParseErr(pcode), lineno, filename, line);
      return 0;
    }
}

ParseCode ReadConfig (Option_t opts[], const char *filename)
{
  return ParseFile (opts, filename, ConfigErrorFunc, NULL);
}

void ReadStdConfigs (Option_t opts[])
{
  char filename[FILENAME_MAX];
  char *homedir = getenv("HOME");

  /* Read config from files in same order as original parser */
  ReadConfig (opts, "/etc/ogg123rc");
  if (homedir && strlen(homedir) < FILENAME_MAX - 10) {
    /* Try ~/.ogg123 */
    strncpy (filename, homedir, FILENAME_MAX);
    strcat (filename, "/.ogg123rc");
    ReadConfig (opts, filename);
  }
}
/* /configuration interface */

/* status interface */
/* string temporary data (from the data segment) */
char TwentyBytes1[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char TwentyBytes2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char TwentyBytes3[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void InitOgg123Stats (Stat_t stats[])
{
  Stat_t *cur;

  cur = &stats[0]; /* currently playing file / stream */
  cur->prio = 3; cur->enabled = 0; cur->formatstr = "File: %s"; cur->type = stat_stringarg;
  cur = &stats[1]; /* current playback time (preformatted) */
  cur->prio = 1; cur->enabled = 1; cur->formatstr = "Time: %s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes1;
  cur = &stats[2]; /* remaining playback time (preformatted) */
  cur->prio = 1; cur->enabled = 0; cur->formatstr = "%s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes2;
  cur = &stats[3]; /* total playback time (preformatted) */
  cur->prio = 1; cur->enabled = 0; cur->formatstr = "of %s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes3;
  cur = &stats[4]; /* instantaneous bitrate */
  cur->prio = 2; cur->enabled = 1; cur->formatstr = "Bitrate: %5.1f"; cur->type = stat_doublearg;
  cur = &stats[5]; /* average bitrate (not yet implemented) */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = "Avg bitrate: %5.1f"; cur->type = stat_doublearg;
  cur = &stats[6]; /* input buffer fill % */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = " Input Buffer %5.1f%%"; cur->type = stat_doublearg;
  cur = &stats[7]; /* input buffer status */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = "(%s)"; cur->type = stat_stringarg; cur->arg.stringarg = NULL;
  cur = &stats[8]; /* output buffer fill % */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = " Output Buffer %5.1f%%"; cur->type = stat_doublearg;
  cur = &stats[9]; /* output buffer status */
  cur->prio = 1; cur->enabled = 0; cur->formatstr = "%s"; cur->type = stat_stringarg; cur->arg.stringarg = NULL;
}

void SetTime (Stat_t stats[], ogg_int64_t sample)
{
  double CurTime = (double) sample / (double) Options.outputOpts.rate;
  long c_min = (long) CurTime / (long) 60;
  double c_sec = CurTime - 60.0f * c_min;
  long r_min, t_min;
  double r_sec, t_sec;

  if (stats[2].enabled && Options.inputOpts.seekable) {
    if (sample > Options.inputOpts.totalSamples) {
      /* file probably grew while playing; update total time */
      Options.inputOpts.totalSamples = sample;
      Options.inputOpts.totalTime = CurTime;
      stats[3].arg.stringarg[0] = '\0';
      r_min = 0;
      r_sec = 0.0f;
    } else {
      r_min = (long) (Options.inputOpts.totalTime - CurTime) / (long) 60;
      r_sec = ((double) Options.inputOpts.totalTime - CurTime) - 60.0f * (double) r_min;
    }
    sprintf (stats[2].arg.stringarg, "[%02li:%05.2f]", r_min, r_sec);
    if (stats[3].arg.stringarg[0] == '\0') {
      t_min = (long) Options.inputOpts.totalTime / (long) 60;
      t_sec = Options.inputOpts.totalTime - 60.0f * t_min;
      sprintf (stats[3].arg.stringarg, "%02li:%05.2f", t_min, t_sec);
    }    
  }
  sprintf (stats[1].arg.stringarg, "%02li:%05.2f", c_min, c_sec);
}

void SetBufferStats (buf_t *buf, char *strbuf)
{
  char *cur = strbuf;
  char *comma = ", ";
  char *sep = "(";

  if (buf->prebuffering) {
    cur += sprintf (cur, "%sPrebuffering", sep);
    sep = comma;
  }
  if (buf->paused) {
    cur += sprintf (cur, "%sPaused", sep);
    sep = comma;
  }
  if (buf->eos) {
    cur += sprintf (cur, "%sEOS", sep);
    sep = comma;
  }
  if (cur != strbuf)
    cur += sprintf (cur, ")");
}

void SetBuffersStats ()
{
  char strbuf[80];
  Stat_t *stats = Options.statOpts.stats;

  memset (strbuf, 0, 80);
  if (Options.inputOpts.buffer) {
    SetBufferStats (Options.inputOpts.buffer, strbuf);
    stats[6].arg.doublearg = (double) buffer_full(Options.inputOpts.buffer) / (double) Options.inputOpts.buffer->size * 100.0f;
  }
  if (stats[7].arg.stringarg)
    free (stats[7].arg.stringarg);
  stats[7].arg.stringarg = strdup (strbuf);

  memset (strbuf, 0, 80);
  if (Options.outputOpts.buffer) {
    SetBufferStats (Options.outputOpts.buffer, strbuf);
    stats[8].arg.doublearg = (double) buffer_full(Options.outputOpts.buffer) / (double) Options.outputOpts.buffer->size * 100.0f;
  }
  if (stats[9].arg.stringarg)
    free (stats[9].arg.stringarg);
  stats[9].arg.stringarg = strdup (strbuf);
}

void Ogg123UpdateStats (void)
{
  if (pthread_mutex_trylock(&stats_lock) == 0) {
    SetBuffersStats ();
    UpdateStats (Options.statOpts.stats);
    pthread_mutex_unlock(&stats_lock);
  }
}

/* /status interface */

/* buffer interface */
size_t OutBufferWrite(void *ptr, size_t size, size_t nmemb, void *arg, char iseos)
{
  static ogg_int64_t cursample = 0;
  static unsigned char RechunkBuffer[BUFFER_CHUNK_SIZE];
  static size_t curBuffered = 0;
  size_t origSize;
  unsigned char *data = ptr;

  origSize = size;
  size *= nmemb;
  
  SetTime (Options.statOpts.stats, cursample);
  Ogg123UpdateStats();
  cursample += Options.playOpts.nth * size / Options.outputOpts.channels / 2 / Options.playOpts.ntimes; /* locked to 16-bit */
  //  fprintf(stderr, "nmemb = %d, size = %d, cursample = %lld\n", nmemb, size,
  //  cursample);

  /* optimized fast path */
  if (curBuffered == BUFFER_CHUNK_SIZE && curBuffered == 0)
    devices_write (ptr, size, 1, Options.outputOpts.devices);
  else
    /* don't actually write until we have a full chunk, or of course EOS */
    while (size) {
      size_t toChunkNow = BUFFER_CHUNK_SIZE - curBuffered <= size ? BUFFER_CHUNK_SIZE - curBuffered : size;
      memmove (RechunkBuffer + curBuffered, data, toChunkNow);
      size -= toChunkNow;
      data += toChunkNow;
      curBuffered += toChunkNow;
      if (curBuffered == BUFFER_CHUNK_SIZE) {
	devices_write (RechunkBuffer, curBuffered, 1, Options.outputOpts.devices);
	curBuffered = 0;
      }
    }

  if (iseos) {
    cursample = 0;
    devices_write (RechunkBuffer, curBuffered, 1, Options.outputOpts.devices);
    curBuffered = 0;
  }
  return origSize;
}
/* /buffer interface */


void usage(void)
{
  int i, driver_count;
  ao_info **devices = ao_driver_info_list(&driver_count);

  printf ( 
         "Ogg123 from " PACKAGE " " VERSION "\n"
	 " by Kenneth Arnold <kcarnold@arnoldnet.net> and others\n\n"
	 "Usage: ogg123 [<options>] <input file> ...\n\n"
	 "  -h, --help     this help\n"
	 "  -V, --version  display Ogg123 version\n"
	 "  -d, --device=d uses 'd' as an output device\n"
	 "      Possible devices are ('*'=live, '@'=file):\n"
	 "        ");
  
  for(i = 0; i < driver_count; i++) {
    printf ("%s", devices[i]->short_name);
    if (devices[i]->type == AO_TYPE_LIVE)
      printf ("*");
    else if (devices[i]->type == AO_TYPE_FILE)
      printf ("@");
    printf (" ");
  }

  printf ("\n");
  
  printf (
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

#define INIT(type, value) type type##_##value = value
char char_n = 'n';
float float_50f = 50.0f;
float float_0f = 0.0f;
INIT(int, 10000);
INIT(int, 1);
INIT(int, 0);

int main(int argc, char **argv)
{
  int ret;
  int option_index = 1;
  ao_option *temp_options = NULL;
  ao_option ** current_options = &temp_options;
  ao_info *info;
  int temp_driver_id = -1;
  devices_t *current;

  /* *INDENT-OFF* */
  Option_t opts[] = {
    /* found, name, description, type, ptr, default */
    {0, "default_device", "default output device", opt_type_string, &Options.outputOpts.default_device, NULL},
    {0, "shuffle",        "shuffle playlist",      opt_type_char,   &Options.playOpts.shuffle,        &char_n},
    {0, "verbose",        "verbosity level",       opt_type_int,    &Options.statOpts.verbose,        &int_1},
    {0, "outbuffer",      "out buffer size (kB)",  opt_type_int,    &Options.outputOpts.BufferSize, &int_0},
    {0, "outprebuffer",   "out prebuffer (%)",     opt_type_float,  &Options.outputOpts.Prebuffer,   &float_0f},
    {0, "inbuffer",       "in buffer size (kB)",   opt_type_int,    &Options.inputOpts.BufferSize, &int_10000},
    {0, "inprebuffer",    "in prebuffer (%)",      opt_type_float,  &Options.inputOpts.Prebuffer, &float_50f},
    {0, "save_stream",    "append stream to file", opt_type_string, &Options.inputOpts.SaveStream, NULL},
    {0, "delay",          "skip file delay (sec)", opt_type_int,    &Options.playOpts.delay,          &int_1},
    {0, NULL,             NULL,                    0,               NULL,                NULL}
  };
  /* *INDENT-ON* */

  Options.playOpts.nth = 1;
  Options.playOpts.ntimes = 1;
  Options.statOpts.verbose = 1;

  on_exit (ogg123_onexit, &Options);
  signal (SIGINT, SigHandler);
  signal (SIGTSTP, SigHandler);
  signal (SIGCONT, SigHandler);
  ao_initialize();
  
  InitOgg123Stats (Options.statOpts.stats);

  InitOpts(opts);
  ReadStdConfigs (opts);

    while (-1 != (ret = getopt_long(argc, argv, "b:c::d:f:hl:k:o:p:qvVx:y:z",
				    long_options, &option_index))) {
	switch (ret) {
	case 0:
	  Error ("Internal error: long option given when none expected.\n");
	  exit(1);
	case 'b':
	  Options.outputOpts.BufferSize = atoi(optarg);
	  break;
	case 'c':
	  if (optarg)
	    {
	      char *tmp = strdup (optarg);
	      ParseCode pcode = ParseLine (opts, tmp);
	      if (pcode != parse_ok)
		Error ("=== Error \"%s\" while parsing config option from command line.\n"
		       "=== Option was: %s\n", ParseErr (pcode), optarg);
	      free (tmp);
	    }
	  else {
	    /* not using the status interface here */
	    fprintf (stdout, "Available options:\n");
	    DescribeOptions (opts, stdout);
	    exit (0);
	  }
	  break;
	case 'd':
	    temp_driver_id = ao_driver_id(optarg);
	    if (temp_driver_id < 0) {
		Error ("=== No such device %s.\n", optarg);
		exit(1);
	    }
	    current = append_device(Options.outputOpts.devices,
				    temp_driver_id, 
				    NULL, NULL);
	    if(Options.outputOpts.devices == NULL)
		    Options.outputOpts.devices = current;
	    current_options = &current->options;
	    break;
	case 'f':
	  if (temp_driver_id >= 0) {
	    info = ao_driver_info(temp_driver_id);
	    if (info->type == AO_TYPE_FILE) {
	      free(current->filename);
	      current->filename = strdup(optarg);
	    } else {
	      Error ("=== Driver %s is not a file output driver.\n",
		     info->short_name);
	      exit(1);
	    }
	  } else {
	    Error ("=== Cannot specify output file without specifying a driver.\n");
	    exit (1);
	  }
	  break;
	case 'k':
	    Options.playOpts.seekpos = atof(optarg);
	    break;
	case 'l':
	    Options.playOpts.delay = atoi(optarg);
	    break;
	case 'o':
	    if (optarg && !add_option(current_options, optarg)) {
		Error ("=== Incorrect option format: %s.\n", optarg);
		exit(1);
	    }
	    break;
	case 'h':
	    usage();
	    exit(0);
	case 'p':
	  Options.outputOpts.Prebuffer = atof (optarg);
	  if (Options.outputOpts.Prebuffer < 0.0f || Options.outputOpts.Prebuffer > 100.0f)
	    {
	      Error ("--- Prebuffer value invalid. Range is 0-100, using nearest value.\n");
	      Options.outputOpts.Prebuffer = Options.outputOpts.Prebuffer < 0.0f ? 0.0f : 100.0f;
	    }
	  break;
	case 'q':
	  Options.statOpts.verbose = 0;
	  break;
	case 'v':
	  Options.statOpts.verbose++;
	  break;
	case 'V':
	  Error ("Ogg123 from " PACKAGE " " VERSION "\n");
	  exit(0);
	case 'x':
	  Options.playOpts.nth = atoi (optarg);
	  if (Options.playOpts.nth == 0) {
	    Error ("--- Cannot play every 0th chunk!\n");
	    Options.playOpts.nth = 1;
	  }
	  break;
	case 'y':
	  Options.playOpts.ntimes = atoi (optarg);
	  if (Options.playOpts.ntimes == 0) {
	    Error ("--- Cannot play every chunk 0 times.\n"
		   "--- To do a test decode, use the null output driver.\n");
	    Options.playOpts.ntimes = 1;
	  }
	  break;
	case 'z':
	    Options.playOpts.shuffle = 1;
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

    SetPriority (Options.statOpts.verbose);

    /* Add last device to device list or use the default device */
    if (temp_driver_id < 0) {
      if (Options.outputOpts.default_device) {
	temp_driver_id = ao_driver_id (Options.outputOpts.default_device);
	if (temp_driver_id < 0)
	  Error ("--- Driver %s specified in configuration file invalid.\n", Options.outputOpts.default_device);
      }
      
      if (temp_driver_id < 0) {
	temp_driver_id = ao_default_driver_id();
      }
      
      if (temp_driver_id < 0) {
	Error ("=== Could not load default driver and no driver specified in config file. Exiting.\n");
	exit(1);
      }

      Options.outputOpts.devices = append_device(Options.outputOpts.devices, temp_driver_id, 
						 temp_options, NULL);
    }

    Options.inputOpts.BufferSize *= 1024;

    if (Options.outputOpts.BufferSize)
      {
	Options.outputOpts.BufferSize *= 1024;
	Options.outputOpts.Prebuffer = Options.outputOpts.Prebuffer * 
	  (float) Options.outputOpts.BufferSize / 100.0f;
	Options.outputOpts.buffer = buffer_create(Options.outputOpts.BufferSize, (int) Options.outputOpts.Prebuffer, NULL, (pWriteFunc) OutBufferWrite, NULL, NULL, 4096);
	Options.statOpts.stats[8].enabled = 1;
	Options.statOpts.stats[9].enabled = 1;
      }
    
    if (Options.playOpts.shuffle == 'n' || Options.playOpts.shuffle == 'N')
      Options.playOpts.shuffle = 0;
    else if (Options.playOpts.shuffle == 'y' || Options.playOpts.shuffle == 'Y')
      Options.playOpts.shuffle = 1;

    if (Options.playOpts.shuffle) {
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
	Options.playOpts.read_file = argv[optind];
	play_file();
	optind++;
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

  signal (SIGINT, SigHandler);
}

void signal_activate_skipfile(int ignored)
{
  signal(SIGINT,signal_skipfile);
}

void SigHandler (int signo)
{
  switch (signo) {
  case SIGINT:
    exit_requested = 1;
  case SIGTSTP:
    pause_requested = 1;
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
    break;
  default:
    psignal (signo, "Unknown signal caught");
  }
}

#if 0
/* from vorbisfile.c */
static int _fseek64_wrap(FILE *f,ogg_int64_t off,int whence)
{
  if(f==NULL)return(-1);
  return fseek(f,(int)off,whence);
}
#endif

void play_file()
{
  OggVorbis_File vf;
  int current_section = -1, eof = 0, eos = 0, ret;
  int old_section = -1;
  int is_big_endian = ao_is_big_endian();
  double realseekpos = Options.playOpts.seekpos;
  int nthc = 0, ntimesc = 0;
  int tmp;
  ov_callbacks VorbisfileCallbacks;
  
  tmp = strchr(Options.playOpts.read_file, ':') - Options.playOpts.read_file;
  if (tmp < 10 && tmp + 2 < strlen(Options.playOpts.read_file) && !strncmp(Options.playOpts.read_file + tmp, "://", 3))
    {
      /* let's call this a URL. */
      ShowMessage (1, 0, 1, "Stream: %s", Options.playOpts.read_file);
      VorbisfileCallbacks.read_func = StreamBufferRead;
      VorbisfileCallbacks.seek_func = StreamBufferSeek;
      VorbisfileCallbacks.close_func = StreamBufferClose;
      VorbisfileCallbacks.tell_func = StreamBufferTell;
      
      Options.inputOpts.URL = Options.playOpts.read_file;
      Options.inputOpts.buffer = InitStream (Options.inputOpts);
      if ((ov_open_callbacks (Options.inputOpts.buffer->data, &vf, NULL, 0, VorbisfileCallbacks)) < 0) {
	Error ("=== Input not an Ogg Vorbis audio stream.\n");
	return;
      }
      Options.statOpts.stats[6].enabled = 1;
      Options.statOpts.stats[7].enabled = 1;
    }
  else
    {
      FILE *InStream;
#if 0
      VorbisfileCallbacks.read_func = fread;
      VorbisfileCallbacks.seek_func = _fseek64_wrap;
      VorbisfileCallbacks.close_func = fclose;
      VorbisfileCallbacks.tell_func = ftell;
#endif
      if (strcmp(Options.playOpts.read_file, "-"))
	{
	  ShowMessage (1, 0, 1, "File: %s", Options.playOpts.read_file);
	  /* Open the file. */
	  if ((InStream = fopen(Options.playOpts.read_file, "rb")) == NULL) {
	    perror ("=== Error opening input file");
	    return;
	  }
	}
      else
	{
	  ShowMessage (1, 0, 1, "-=( Standard Input )=- ");
	  InStream = stdin;
	}
      if ((ov_open (InStream, &vf, NULL, 0)) < 0) {
	Error ("=== Input not an Ogg Vorbis audio stream.\n");
	return;
      }
    }
    
    /* Setup so that pressing ^C in the first second of playback
     * interrupts the program, but after the first second, skips
     * the song.  This functionality is similar to mpg123's abilities. */
    
    if (Options.playOpts.delay > 0) {
      skipfile_requested = 0;
      signal(SIGALRM,signal_activate_skipfile);
      alarm(Options.playOpts.delay);
    }
        
    exit_requested = 0;
    pause_requested = 0;

    while (!eof && !exit_requested) {
      int i;
      vorbis_comment *vc = ov_comment(&vf, -1);
      vorbis_info *vi = ov_info(&vf, -1);
      
      Options.outputOpts.rate = vi->rate;
      Options.outputOpts.channels = vi->channels;
      if(open_audio_devices() < 0)
	exit(1);
      
      for (i = 0; i < vc->comments; i++) {
	char *cc = vc->user_comments[i];	/* current comment */
	int j;
	
	for (j = 0; ogg_comment_keys[j].key != NULL; j++)
	  if (!strncasecmp
	      (ogg_comment_keys[j].key, cc,
	       strlen(ogg_comment_keys[j].key))) {
	    ShowMessage (1, 0, 1, ogg_comment_keys[j].formatstr,
			 cc + strlen(ogg_comment_keys[j].key));
	    break;
	  }
	if (ogg_comment_keys[j].key == NULL)
	  ShowMessage (1, 0, 1, "Unrecognized comment: '%s'", cc);
      }
      
      ShowMessage (3, 0, 1, "Version is %d", vi->version);
      ShowMessage (3, 0, 1, "Bitrate Hints: upper=%ld nominal=%ld lower=%ld window=%ld",
		   vi->bitrate_upper, vi->bitrate_nominal, vi->bitrate_lower, vi->bitrate_window);
      ShowMessage (2, 0, 1, "Bitstream is %d channel, %ldHz",
		   vi->channels, vi->rate);
      ShowMessage (2, 0, 1, "Encoded by: %s", vc->vendor);
      
      if (ov_seekable (&vf)) {
	if ((realseekpos > ov_time_total(&vf, -1)) || (realseekpos < 0))
	  /* If we're out of range set it to right before the end. If we set it
	   * right to the end when we seek it will go to the beginning of the song */
	  realseekpos = ov_time_total(&vf, -1) - 0.01;
	
	Options.inputOpts.seekable = 1;
	Options.inputOpts.totalTime = ov_time_total(&vf, -1);
	Options.inputOpts.totalSamples = ov_pcm_total(&vf, -1);
	Options.statOpts.stats[2].enabled = 1;
	Options.statOpts.stats[3].enabled = 1;
	if (Options.statOpts.verbose > 0)
	  SetTime (Options.statOpts.stats, realseekpos / vi->rate);
	
	if (realseekpos > 0)
	  ov_time_seek(&vf, realseekpos);
      }
      else
	Options.inputOpts.seekable = 0;

      /* Ready to start sending data to the audio playback thread */

      if (Options.outputOpts.buffer)
	buffer_thread_start (Options.outputOpts.buffer);

      eos = 0;
      
      while (!eos && !exit_requested) {
	
	if (skipfile_requested) {
	  eof = eos = 1;
	  signal(SIGALRM,signal_activate_skipfile);
	  alarm(Options.playOpts.delay);
	  break;
	}

	if (pause_requested) {
	  buffer_thread_pause (Options.outputOpts.buffer);
	  kill (getpid(), SIGSTOP); /* We stall here */
	  
	  /* Done pausing */
	  buffer_thread_unpause (Options.outputOpts.buffer);
	  pause_requested = 0;
	}


	old_section = current_section;
	ret =
	  ov_read(&vf, (char *) convbuffer, sizeof(convbuffer), is_big_endian,
		  2, 1, &current_section);
	if (ret == 0) {
	  /* End of file */
	  eof = eos = 1;
	} else if (ret == OV_HOLE) {
	  if (Options.statOpts.verbose > 1) 
	    /* we should be able to resync silently; if not there are 
	       bigger problems. */
	    Error ("--- Hole in the stream; probably harmless\n");
	} else if (ret < 0) {
	  /* Stream error */
	  Error ("=== Vorbis library reported a stream error.\n");
	} else {
	  /* did we enter a new logical bitstream */
	  if (old_section != current_section && old_section != -1)
	    eos = 1;
	  
	  Options.statOpts.stats[4].arg.doublearg = (double) ov_bitrate_instant (&vf) / 1000.0f;

	  do {
	    if (nthc-- == 0) {
	      if (Options.outputOpts.buffer) {
		buffer_submit_data (Options.outputOpts.buffer, 
				    convbuffer, ret, 1);
		Ogg123UpdateStats();
	      }
	      else
		OutBufferWrite (convbuffer, ret, 1, &Options, 0);
	      nthc = Options.playOpts.nth - 1;
	    }
	  } while (++ntimesc < Options.playOpts.ntimes);
	  ntimesc = 0;
	  
	}
      }

      /* Done playing this logical bitstream.  Now we cleanup. */
      if (Options.outputOpts.buffer) {
	fprintf(stderr, "exit requested = %d", exit_requested);

	if (!exit_requested && !skipfile_requested) {
	  buffer_mark_eos (Options.outputOpts.buffer);
	  buffer_wait_for_empty (Options.outputOpts.buffer);
	}

	buffer_thread_kill (Options.outputOpts.buffer);
      }

    }
    
    alarm(0);
    signal(SIGALRM,SIG_DFL);
    signal(SIGINT,SigHandler);
    
    ov_clear(&vf);
    
    ShowMessage (1, 1, 1, "Done.");
    
    if (exit_requested)
      exit (0);
}

int open_audio_devices()
{
  static int prevrate=0, prevchan=0;
  devices_t *current;
  ao_sample_format format;

  if(prevrate == Options.outputOpts.rate && prevchan == Options.outputOpts.channels && Options.outputOpts.devicesOpen)
    return 0;
  
  if(prevrate !=0 && prevchan!=0 && Options.outputOpts.devicesOpen)
    {
      close_audio_devices (Options.outputOpts.devices);
      Options.outputOpts.devicesOpen = 0;
    }
  
  format.rate = prevrate = Options.outputOpts.rate;
  format.channels = prevchan = Options.outputOpts.channels;
  format.bits = 16;
  format.byte_format = AO_FMT_NATIVE;

  current = Options.outputOpts.devices;
  while (current != NULL) {
    ao_info *info = ao_driver_info(current->driver_id);
    
    if (Options.statOpts.verbose > 0) {
      ShowMessage (1, 0, 1, "Device:   %s", info->name);
      ShowMessage (1, 0, 1, "Author:   %s", info->author);
      ShowMessage (1, 0, 1, "Comments: %s\n", info->comment);
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
	Error ("Error: Device not available.\n");
	break;
      case AO_ENOTLIVE:
	Error ("Error: %s requires an output filename to be specified with -f.\n", info->short_name);
	break;
      case AO_EBADOPTION:
	Error ("Error: Unsupported option value to %s device.\n",
		info->short_name);
	break;
      case AO_EOPENDEVICE:
	Error ("Error: Cannot open device %s.\n",
		info->short_name);
	break;
      case AO_EFAIL:
	Error ("Error: Device failure.\n");
	break;
      case AO_ENOTFILE:
	Error ("Error: An output file cannot be given for %s device.\n", info->short_name);
	break;
      case AO_EOPENFILE:
	Error ("Error: Cannot open file %s for writing.\n",
		current->filename);
	break;
      case AO_EFILEEXISTS:
	Error ("Error: File %s already exists.\n", current->filename);
	break;
      default:
	Error ("Error: This error should never happen.  Panic!\n");
	break;
      }
	
      return -1;
    }
    current = current->next_device;
  }
  
  Options.outputOpts.devicesOpen = 1;
  return 0;
}

void ogg123_onexit (int exitcode, void *arg)
{
  if (Options.inputOpts.buffer) {
    StreamInputCleanup (Options.inputOpts.buffer);
    Options.inputOpts.buffer = NULL;
  }
      
  if (Options.outputOpts.buffer) {
    buffer_destroy (Options.outputOpts.buffer);
    Options.outputOpts.buffer = NULL;
  }

  ao_onexit (exitcode, Options.outputOpts.devices);
  Options.outputOpts.devicesOpen = 0;
}
