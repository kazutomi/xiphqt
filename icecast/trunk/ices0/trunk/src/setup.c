/* setup.c
 * - Functions for initialization in ices
 * Copyright (c) 2000 Alexander Haväng
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "definitions.h"

#include <thread.h>

/* Local function declarations */
static void ices_setup_parse_options (ices_config_t *ices_config);
static void ices_setup_parse_defaults (ices_config_t *ices_config);
#ifdef HAVE_LIBXML
static void ices_setup_parse_config_file (ices_config_t *ices_config, const char *configfile);
#endif
static void ices_setup_parse_command_line (ices_config_t *ices_config, char **argv, int argc);
static void ices_setup_parse_command_line_for_new_configfile (ices_config_t *ices_config, char **argv, int argc);
static void ices_setup_activate_libshout_changes (const ices_config_t *ices_config);
static void ices_setup_usage (void);
static void ices_setup_version (void);
static void ices_setup_update_pidfile (int icespid);
static void ices_setup_daemonize (void);
static void ices_setup_run_mode_select (ices_config_t *ices_config);
static int  ices_setup_shutting_down = 0;
static void ices_setup_free_all_allocations (ices_config_t *ices_config);

extern ices_config_t ices_config;

static int ThreadsInitialised = 0;

/* Global function definitions */

/* Top level initialization function for ices.
 * It will parse options, initialize modules,
 * and if requested, become a daemon. */
void
ices_setup_initialize (void)
{
  ices_stream_config_t* stream;
  
  /* Setup signal handlers */
  ices_signals_setup ();

  /* Parse the options in the config file, and the command line */
  ices_setup_parse_options (&ices_config);

  /* Go into daemon mode if requested */
  ices_setup_run_mode_select (&ices_config);

  /* Initialize the libshout structure */
  for (stream = ices_config.streams; stream; stream = stream->next) {
    shout_init_connection (&stream->conn);
  }

  ices_setup_activate_libshout_changes (&ices_config);

  /* Open logfiles */
  ices_log_initialize ();

  /* Initialize the thread library */
  thread_initialize ();
  ThreadsInitialised = 1;

  /* Initialize the playlist handler */
  ices_playlist_initialize ();

  /* Initialize id3 stuff */
  ices_id3_initialize ();

#ifdef HAVE_LIBLAME
  /* Initialize liblame for reeencoding */
  ices_reencode_initialize ();
#endif
}

/* Top level ices shutdown function.
 * This is the _only_ way out of here */
void
ices_setup_shutdown (void)
{
  ices_stream_config_t* stream;

  /* Protection for multiple threads calling shutdown.
   * Remember that this is can be called from many places,
   * including the SIGING signal handler */
  if (ThreadsInitialised) {
    thread_library_lock ();

    if (ices_setup_shutting_down) {
      thread_library_unlock ();
      return;
    }

    ices_setup_shutting_down = 1;

    thread_library_unlock ();
  }

  /* Tell libshout to disconnect from server */
  for (stream = ices_config.streams; stream; stream = stream->next)
    shout_disconnect (&stream->conn);

#ifdef HAVE_LIBLAME
  /* Order the reencoding engine to shutdown */
  ices_reencode_shutdown ();
#endif

  /* Tell the playlist module to shutdown and cleanup */
  ices_playlist_shutdown ();

  /* Shutdown id3 module */
  ices_id3_shutdown ();

  /* Cleanup the cue file (the cue module has no init yet) */
  ices_cue_shutdown ();

  /* Shutdown the thread library */
  thread_shutdown ();

  /* Make sure we're not leaving any memory allocated around when
   * we exit. This makes it easier to find memory leaks, and 
   * some systems actually don't clean up that well */
  ices_setup_free_all_allocations (&ices_config);
	
  /* Let the log and console know we wen't down ok */
  ices_log ("Ices Exiting...");

  /* Close logfiles */
  ices_log_shutdown ();
	
  /* Down and down we go... */
  exit (1);
}

/* Local function definitions */

/* Top level option parsing function.
 * Sets of options object (ices_config), with:
 * - Hardcoded defaults
 * - Configfile settings
 * - Command line options */
static void
ices_setup_parse_options (ices_config_t *ices_config)
{
	/* Get default values for the settings */
	ices_setup_parse_defaults (ices_config);

	/* Look for given configfile on the commandline */
	ices_setup_parse_command_line_for_new_configfile (ices_config, ices_util_get_argv(), ices_util_get_argc ());

#ifdef HAVE_LIBXML
	/* Parse the configfile */
	ices_setup_parse_config_file (ices_config, ices_config->configfile);
#endif
	
	/* Parse the commandline */
	ices_setup_parse_command_line (ices_config, ices_util_get_argv(), ices_util_get_argc());
}

/* Function for placing hardcoded defaults in the 
 * options object (ices_config) */
static void
ices_setup_parse_defaults (ices_config_t *ices_config)
{
  ices_config->host = ices_util_strdup (ICES_DEFAULT_HOST);
  ices_config->port = ICES_DEFAULT_PORT;
  ices_config->password = ices_util_strdup (ICES_DEFAULT_PASSWORD);
  ices_config->header_protocol = ICES_DEFAULT_HEADER_PROTOCOL;
  ices_config->dumpfile = NULL; /* No dumpfile by default */
  ices_config->configfile = ices_util_strdup (ICES_DEFAULT_CONFIGFILE);
  ices_config->daemon = ICES_DEFAULT_DAEMON;
  ices_config->pre_dj = ICES_DEFAULT_PRE_DJ;
  ices_config->post_dj = ICES_DEFAULT_POST_DJ;
  ices_config->base_directory = ices_util_strdup (ICES_DEFAULT_BASE_DIRECTORY);
  ices_config->verbose = ICES_DEFAULT_VERBOSE;
  ices_config->reencode = 0;

  ices_config->pm.playlist_file =
    ices_util_strdup (ICES_DEFAULT_PLAYLIST_FILE);
  ices_config->pm.module = NULL; /* Default to the hardcoded default */
  ices_config->pm.randomize = ICES_DEFAULT_RANDOMIZE_PLAYLIST;
  ices_config->pm.playlist_type = ICES_DEFAULT_PLAYLIST_TYPE;

  ices_config->streams =
    (ices_stream_config_t*) malloc (sizeof (ices_stream_config_t));

  ices_setup_parse_stream_defaults (ices_config->streams);
}

/* Place hardcoded defaults into an ices_stream_config object */
void
ices_setup_parse_stream_defaults (ices_stream_config_t* ices_stream_config)
{
  ices_stream_config->mount = ices_util_strdup (ICES_DEFAULT_MOUNT);
  
  ices_stream_config->name = ices_util_strdup (ICES_DEFAULT_NAME);
  ices_stream_config->genre = ices_util_strdup (ICES_DEFAULT_GENRE);
  ices_stream_config->description = ices_util_strdup (ICES_DEFAULT_DESCRIPTION);
  ices_stream_config->url = ices_util_strdup (ICES_DEFAULT_URL);
  ices_stream_config->ispublic = ICES_DEFAULT_ISPUBLIC;

  ices_stream_config->bitrate = ICES_DEFAULT_BITRATE;
  ices_stream_config->reencode = ICES_DEFAULT_REENCODE;
  ices_stream_config->out_numchannels = -1;
  ices_stream_config->out_samplerate = -1;

  ices_stream_config->encoder_state = NULL;
  ices_stream_config->encoder_initialised = 0;

  ices_stream_config->next = NULL;
}

/* Frees ices_stream_config_t data (but not the object itself) */
static void
ices_setup_free_stream (ices_stream_config_t* stream)
{
  ices_util_free (stream->mount);

  ices_util_free (stream->name);
  ices_util_free (stream->genre);
  ices_util_free (stream->description);
  ices_util_free (stream->url);
}

/* Function to free() all allocated memory when ices shuts down. */
static void
ices_setup_free_all_allocations (ices_config_t *ices_config)
{
  ices_stream_config_t *stream, *next;

  ices_util_free (ices_config->host);
  ices_util_free (ices_config->password);
  ices_util_free (ices_config->dumpfile);
  ices_util_free (ices_config->configfile);
  ices_util_free (ices_config->base_directory);

  ices_util_free (ices_config->pm.playlist_file);
  ices_util_free (ices_config->pm.module);

  for (stream = ices_config->streams; stream; stream = next)
  {
    next = stream->next;

    ices_setup_free_stream (stream);
    ices_util_free (stream);
  }
}

#ifdef HAVE_LIBXML
/* Tell the xml module to parse the config file. */
static void
ices_setup_parse_config_file (ices_config_t *ices_config, const char *configfile)
{
	char namespace[1024];
	const char *realname = NULL;
	int ret;

	if (ices_util_verify_file (configfile)) {
		realname = configfile;
	} else {
		sprintf (namespace, "%s/%s", ICES_ETCDIR, configfile);
		if (ices_util_verify_file (namespace))
			realname = &namespace[0];
	}
	
	if (realname) {
		ret = ices_xml_parse_config_file (ices_config, realname);

		if (ret == -1) {
			/* ret == -1 means we have no libxml support */
			ices_log_debug ("%s", ices_log_get_error ());
		} else if (ret == 0) {
			/* A real error */
			ices_log ("%s", ices_log_get_error ());
		}
	}
}
#endif

/* This function looks through the command line options for a new
 * configfile. */
static void
ices_setup_parse_command_line_for_new_configfile (ices_config_t *ices_config, char **argv, int argc)
{
	int arg;
	char *s;

        arg = 1;
	
        while (arg < argc) {
                s = argv[arg];
		
                if (s[0] == '-') {
			
			switch (s[1]) {
				case 'c':
					arg++;
					if (ices_config->configfile)
						ices_util_free (ices_config->configfile);
					ices_config->configfile = ices_util_strdup (argv[arg]);
#ifndef HAVE_LIBXML
					ices_log ("You have no libxml support. The config file you just specified cannot be used.");
					ices_setup_shutdown ();
#endif
					break;
			}
		}
		arg++;
	}
}

/* This function parses the command line options */
static void
ices_setup_parse_command_line (ices_config_t *ices_config, char **argv, int argc)
{
	int arg;
	char *s;

        arg = 1;
	
        while (arg < argc) {
                s = argv[arg];
		
                if (s[0] == '-') {
			
			if ((strchr ("RriVvBzxHN", s[1]) == NULL) && arg >= (argc - 1)) {
				ices_log ("Option %c requires an argument!\n", s[1]);
				ices_setup_usage ();
				ices_setup_shutdown ();
				return;
			}
			
			switch (s[1]) {
				case 'B':
					ices_config->daemon = 1;
					break;
				case 'b':
					arg++;
					ices_config->streams->bitrate = atoi (argv[arg]);
					break;
				case 'c':
					arg++;
					break;
				case 'd':
					arg++;
					ices_util_free (ices_config->streams->description);
					ices_config->streams->description = ices_util_strdup (argv[arg]);
					break;
				case 'D':
					arg++;
					if (ices_config->base_directory)
						ices_util_free (ices_config->base_directory);
					ices_config->base_directory = ices_util_strdup (argv[arg]);
					break;
				case 'F':
					arg++;
					ices_util_free (ices_config->pm.playlist_file);
					ices_config->pm.playlist_file = ices_util_strdup (argv[arg]);
					break;
				case 'f':
					arg++;
					if (ices_config->dumpfile)
						ices_util_free (ices_config->dumpfile);
					ices_config->dumpfile = ices_util_strdup (argv[arg]);
					break;
				case 'g':
					arg++;
					ices_util_free (ices_config->streams->genre);
					ices_config->streams->genre = ices_util_strdup (argv[arg]);
					break;
				case 'h':
					arg++;
					if (ices_config->host)
						ices_util_free (ices_config->host);
					ices_config->host = ices_util_strdup (argv[arg]);
					break;
				case 'H':
					arg++;
					ices_config->streams->out_samplerate = atoi (argv[arg]);
					break;
				case 'i':
					ices_config->header_protocol = icy_header_protocol_e;
					break;
				case 'M':
					arg++;
					ices_util_free (ices_config->pm.module);
					ices_config->pm.module = ices_util_strdup (argv[arg]);
					break;
				case 'm':
					arg++;
					ices_util_free (ices_config->streams->mount);
					ices_config->streams->mount = ices_util_strdup (argv[arg]);
					break;
				case 'N':
					arg++;
					ices_config->streams->out_numchannels = atoi (argv[arg]);
					break;
				case 'n':
					arg++;
					ices_util_free (ices_config->streams->name);
					ices_config->streams->name = ices_util_strdup (argv[arg]);
					break;
				case 'P':
					arg++;
					if (ices_config->password)
						ices_util_free (ices_config->password);
					ices_config->password = ices_util_strdup (argv[arg]);
					break;
				case 'p':
					arg++;
					ices_config->port = atoi (argv[arg]);
					break;
				case 'R':
#ifdef HAVE_LIBLAME
					ices_config->streams->reencode = 1;
#else
					ices_log ("Support for reencoding with liblame was not found. You can't reencode this.");
					ices_setup_shutdown ();
#endif
					break;
				case 'r':
					ices_config->pm.randomize = 1;
					break;
				case 'S':
					arg++;

					if (strcmp (argv[arg], "python") == 0)
						ices_config->pm.playlist_type = ices_playlist_python_e;
					else if (strcmp (argv[arg], "perl") == 0)
						ices_config->pm.playlist_type = ices_playlist_perl_e;
					else 
						ices_config->pm.playlist_type = ices_playlist_builtin_e;
					break;
				case 's':
					arg++;
					ices_config->streams->ispublic = 0;
					break;
				case 'u':
					arg++;
					ices_util_free (ices_config->streams->url);
					ices_config->streams->url = ices_util_strdup (argv[arg]);
					break;
			        case 'V':
				  ices_setup_version ();
				  exit (0);
				case 'v':
					ices_config->verbose = 1;
					break;
				case 'z':
					ices_config->pre_dj = 1;
					break;
				case 'x':
					ices_config->post_dj = 1;
					break;
				default:
					ices_setup_usage ();
					break;
			}
		}
		arg++;
	}
}

/* This function takes all the new configuration and copies it to the
   libshout object. */
static void
ices_setup_activate_libshout_changes (const ices_config_t *ices_config)
{
  ices_stream_config_t* stream;
  int streamno = 0;

  for (stream = ices_config->streams; stream; stream = stream->next) {
    stream->conn.port = ices_config->port;
    stream->conn.ip = ices_config->host;
    stream->conn.password = ices_config->password;
    stream->conn.icy_compat =
      ices_config->header_protocol == icy_header_protocol_e;
    stream->conn.dumpfile = ices_config->dumpfile;
    stream->conn.name = stream->name;
    stream->conn.url = stream->url;
    stream->conn.genre = stream->genre;
    stream->conn.description = stream->description;
    stream->conn.bitrate = stream->bitrate;
    stream->conn.ispublic = stream->ispublic;
    stream->conn.mount = stream->mount;

    ices_log_debug ("Sending following information to libshout:");
    ices_log_debug ("Stream: %d", streamno);
    ices_log_debug ("Host: %s\tPort: %d", stream->conn.ip, stream->conn.port);
    ices_log_debug ("Password: %s\tIcy Compat: %d", stream->conn.password,
		    stream->conn.icy_compat);
    ices_log_debug ("Name: %s\tURL: %s", stream->conn.name, stream->conn.url);
    ices_log_debug ("Genre: %s\tDesc: %s", stream->conn.genre,
		    stream->conn.description);
    ices_log_debug ("Bitrate: %d\tPublic: %d", stream->conn.bitrate,
		    stream->conn.ispublic);
    ices_log_debug ("Mount: %s\tDumpfile: %s",
		    ices_util_nullcheck (stream->conn.mount),
		    ices_util_nullcheck (stream->conn.dumpfile));

    streamno++;
  }
}

/* Display all command line options for ices */
static void
ices_setup_usage (void)
{
  ices_log ("This is ices " VERSION);
	ices_log ("ices <options>");
	ices_log ("Options:");
	ices_log ("\t-B (Background (daemon mode))");
	ices_log ("\t-b <stream bitrate>");
	ices_log ("\t-c <configfile>");
	ices_log ("\t-D <base directory>");
	ices_log ("\t-d <stream description>");
	ices_log ("\t-f <dumpfile on server>");
	ices_log ("\t-F <playlist>");
	ices_log ("\t-g <stream genre>");
	ices_log ("\t-h <host>");
	ices_log ("\t-i (use icy headers)");
	ices_log ("\t-M <interpreter module>");
	ices_log ("\t-m <mountpoint>");
	ices_log ("\t-n <stream name>");
	ices_log ("\t-p <port>");
	ices_log ("\t-P <password>");
	ices_log ("\t-R (activate reencoding)");
	ices_log ("\t-r (randomize playlist)");
	ices_log ("\t-s (private stream)");
	ices_log ("\t-S <perl|python|builtin>");
	ices_log ("\t-u <stream url>");
	ices_log ("\t-v (verbose output)");
	ices_log ("\t-H <reencoded sample rate>");
	ices_log ("\t-N <reencoded number of channels>");
}

/* display version information */
static void
ices_setup_version (void)
{
  ices_log ("ices " VERSION "\nFeatures: "

#ifdef HAVE_LIBLAME
  "LAME "
#endif

#ifdef HAVE_LIBPERL
  "PERL "
#endif

#ifdef HAVE_LIBPYTHON
  "python "
#endif

#ifdef HAVE_LIBXML
  "libxml "
#endif
    );
}

/* This function makes ices run in the selected way.
 * If requested, this is in the background */
static void
ices_setup_run_mode_select (ices_config_t *ices_config)
{
	if (ices_config->daemon) 
		ices_setup_daemonize ();
}

/* Put ices in the background, as a daemon */
static void
ices_setup_daemonize (void)
{
	int icespid = fork ();
	
	if (icespid == -1) {
		ices_log ("ERROR: Cannot fork(), that means no daemon, sorry!");
		return;
	}

	if (icespid != 0) {
#ifdef HAVE_SETPGID
		setpgid (icespid, icespid);
#endif
		/* Update the pidfile (so external applications know what pid
		   ices is running with. */
		printf ("Into the land of the dreaded daemons we go... (pid: %d)\n", icespid);
		ices_setup_shutdown ();
	}
#ifdef HAVE_SETPGID
	setpgid (0, 0);
#endif
	ices_log_daemonize ();
	ices_setup_update_pidfile (getpid());
}

/* Update a file called ices.pid with the given process id */
static void
ices_setup_update_pidfile (int icespid)
{
  char buf[1024];
  FILE* pidfd;

  if (! ices_config.base_directory) {
    ices_log_error ("Base directory is invalid");
    return;
  }

  snprintf (buf, sizeof (buf), "%s/ices.pid", ices_config.base_directory);

  pidfd = ices_util_fopen_for_writing (buf);
	
  if (pidfd) {
    fprintf (pidfd, "%d", icespid);
    ices_util_fclose (pidfd);
  }
}

		
