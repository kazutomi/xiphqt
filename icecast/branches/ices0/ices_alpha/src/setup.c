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
static void ices_setup_parse_config_file (ices_config_t *ices_config, const char *configfile);
static void ices_setup_parse_command_line (ices_config_t *ices_config, char **argv, int argc);
static void ices_setup_activate_changes (ices_conn_t *conn, const ices_config_t *ices_config);
static void ices_setup_usage ();
static void ices_setup_update_pidfile (int icespid);
static void ices_setup_daemonize ();
static void ices_setup_run_mode_select (ices_config_t *ices_config);

/* Global function definitions */
void
ices_setup_init ()
{
	ices_config_t *ices_config = ices_util_get_config ();
	ices_conn_t *conn = ices_util_get_conn ();
	
	/* Initialize the libices structure */
	ices_init_connection (conn);
	
	/* Parse the options in the config file, and the command line */
	ices_setup_parse_options (ices_config);
	
	/* Copy those options to the libices structure */
	ices_setup_activate_changes (conn, ices_config);

	/* Initialize the interpreters */
	interpreter_init ();

	/* Initialize the thread library */
	thread_initialize ();

	/* Setup signal handlers */
	ices_signals_setup ();

	/* Initialize the playlist handler */
	ices_playlist_initialize ();

	/* Go into daemon mode if requested */
	ices_setup_run_mode_select (ices_config);
}

void
ices_setup_shutdown ()
{
	ices_conn_t *conn = ices_util_get_conn ();

	ices_disconnect (conn);

	ices_playlist_shutdown ();

	thread_shutdown ();
}

/* Local function definitions */
static void
ices_setup_parse_options (ices_config_t *ices_config)
{
	/* Get default values for the settings */
	ices_setup_parse_defaults (ices_config);

	/* Parse the configfile */
	ices_setup_parse_config_file (ices_config, ices_config->configfile);
	
	/* Parse the commandline */
	ices_setup_parse_command_line (ices_config, ices_util_get_argv(), ices_util_get_argc());
}

static void
ices_setup_parse_defaults (ices_config_t *ices_config)
{
	ices_config->host = ices_util_strdup (ICES_DEFAULT_HOST);
	ices_config->port = ICES_DEFAULT_PORT;
	ices_config->mount = ices_util_strdup (ICES_DEFAULT_MOUNT);
	ices_config->password = ices_util_strdup (ICES_DEFAULT_PASSWORD);
	ices_config->header_protocol = ICES_DEFAULT_HEADER_PROTOCOL;
	ices_config->name = ices_util_strdup (ICES_DEFAULT_NAME);
	ices_config->genre = ices_util_strdup (ICES_DEFAULT_GENRE);
	ices_config->description = ices_util_strdup (ICES_DEFAULT_DESCRIPTION);
	ices_config->url = ices_util_strdup (ICES_DEFAULT_URL);
	ices_config->bitrate = ICES_DEFAULT_BITRATE;
	ices_config->ispublic = ICES_DEFAULT_ISPUBLIC;
	ices_config->dumpfile = NULL; /* No dumpfile by default */
	ices_config->configfile = ices_util_strdup (ICES_DEFAULT_CONFIGFILE);
	ices_config->playlist_file = ices_util_strdup (ICES_DEFAULT_PLAYLIST_FILE);
	ices_config->randomize_playlist = ICES_DEFAULT_RANDOMIZE_PLAYLIST;
	ices_config->daemon = ICES_DEFAULT_DAEMON;
	ices_config->pre_dj = ICES_DEFAULT_PRE_DJ;
	ices_config->post_dj = ICES_DEFAULT_POST_DJ;
	ices_config->base_directory = ices_util_strdup (ICES_DEFAULT_BASE_DIRECTORY);
	ices_config->playlist_type = ICES_DEFAULT_PLAYLIST_TYPE;
	ices_config->verbose = ICES_DEFAULT_VERBOSE;
}

static void
ices_setup_parse_config_file (ices_config_t *ices_config, const char *configfile)
{
	int ret = ices_xml_parse_config_file (ices_config, configfile);

	if (ret == -1) {
		/* ret == -1 means we have no libxml support */
		ices_log_debug ("%s", ices_log_get_error ());
	} else if (ret == 0) {
		ices_log ("%s", ices_log_get_error ());
	}
}

static void
ices_setup_parse_command_line (ices_config_t *ices_config, char **argv, int argc)
{
	int arg;
	char *s;

        arg = 1;
	
        while (arg < argc) {
                s = argv[arg];
		
                if (s[0] == '-') {
			
			if ((strchr ("rivBzx", s[1]) == NULL) && arg >= (argc - 1)) {
				ices_log ("Option %c requires an argument!\n", s[1]);
				ices_setup_usage ();
				exit (1);
			}
			
			switch (s[1]) {
				case 'B':
					ices_config->daemon = 1;
					break;
				case 'b':
					arg++;
					ices_config->bitrate = atoi (argv[arg]);
					break;
				case 'c':
					arg++;
					ices_config->configfile = ices_util_strdup (argv[arg]);
					ices_setup_parse_config_file (ices_config, ices_config->configfile);
					break;
				case 'd':
					arg++;
					ices_config->description = ices_util_strdup (argv[arg]);
					break;
				case 'D':
					arg++;
					ices_config->base_directory = ices_util_strdup (argv[arg]);
					break;
				case 'F':
					arg++;
					ices_config->playlist_file = ices_util_strdup (argv[arg]);
					break;
				case 'f':
					arg++;
					ices_config->dumpfile = ices_util_strdup (argv[arg]);
					break;
				case 'g':
					arg++;
					ices_config->genre = ices_util_strdup (argv[arg]);
					break;
				case 'h':
					arg++;
					ices_config->host = ices_util_strdup (argv[arg]);
					break;
				case 'i':
					ices_config->header_protocol = icy_header_protocol_e;
					break;
				case 'm':
					arg++;
					ices_config->mount = ices_util_strdup (argv[arg]);
					break;
				case 'n':
					arg++;
					ices_config->name = ices_util_strdup (argv[arg]);
					break;
				case 'P':
					arg++;
					ices_config->password = ices_util_strdup (argv[arg]);
					break;
				case 'p':
					arg++;
					ices_config->port = atoi (argv[arg]);
					break;
				case 'r':
					ices_config->randomize_playlist = 1;
					break;
				case 'S':
					arg++;

					if (strcmp (argv[arg], "python") == 0)
						ices_config->playlist_type = ices_playlist_python_e;
					else if (strcmp (argv[arg], "perl") == 0)
						ices_config->playlist_type = ices_playlist_perl_e;
					else 
						ices_config->playlist_type = ices_playlist_builtin_e;
					break;
				case 's':
					arg++;
					ices_config->ispublic = 0;
					break;
				case 'u':
					arg++;
					ices_config->url = ices_util_strdup (argv[arg]);
					break;
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

static void
ices_setup_activate_changes (ices_conn_t *conn, const ices_config_t *ices_config)
{
	conn->port = ices_config->port;
	conn->ip = ices_config->host;
	conn->password = ices_config->password;
	conn->icy_compat = ices_config->header_protocol == icy_header_protocol_e;
	conn->dumpfile = ices_config->dumpfile;
	conn->name = ices_config->name;
	conn->url = ices_config->url;
	conn->genre = ices_config->genre;
	conn->description = ices_config->description;
	conn->bitrate = ices_config->bitrate;
	conn->ispublic = ices_config->ispublic;
	conn->mount = ices_config->mount;

	ices_log_debug ("Sending following information to libshout:");
	ices_log_debug ("Host: %s\tPort: %d", conn->ip, conn->port);
	ices_log_debug ("Password: %s\tIcy Compat: %d", conn->password, conn->icy_compat);
	ices_log_debug ("Name: %s\tURL: %s", conn->name, conn->url);
	ices_log_debug ("Genre: %s\tDesc: %s", conn->genre, conn->description);
	ices_log_debug ("Bitrate: %d\tPublic: %d", conn->bitrate, conn->ispublic);
	ices_log_debug ("Mount: %s\tDumpfile: %s", ices_util_nullcheck (conn->mount), ices_util_nullcheck (conn->dumpfile)); 
}

static void
ices_setup_usage ()
{
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
	ices_log ("\t-m <mountpoint>");
	ices_log ("\t-n <stream name>");
	ices_log ("\t-p <port>");
	ices_log ("\t-P <password>");
	ices_log ("\t-r (randomize playlist)");
	ices_log ("\t-s (private stream)");
	ices_log ("\t-S <perl|python|builtin>");
	ices_log ("\t-u <stream url>");
}


static void
ices_setup_run_mode_select (ices_config_t *ices_config)
{
	if (ices_config->daemon) 
		ices_setup_daemonize ();
}

static void
ices_setup_daemonize ()
{
	int icespid = fork ();
	
	if (icespid == -1) {
		ices_log ("ERROR: Cannot fork(), that means no daemon, sorry!");
		return;
	}

	if (icespid != 0) {
		ices_log ("Into the land of the dreaded daemons we go... (pid: %d)\n", icespid);
#ifdef HAVE_SETPGID
		setpgid (icespid, icespid);
#endif
		exit (0);
	}
#ifdef HAVE_SETPGID
	setpgid (0, 0);
#endif
	freopen ("/dev/null", "r", stdin);
	freopen ("/dev/null", "w", stdout);
	freopen ("/dev/null", "w", stderr);
	close (0);
	close (1);
	close (2);

	ices_setup_update_pidfile (icespid);
}

static void
ices_setup_update_pidfile (int icespid)
{
	FILE *pidfd = ices_util_fopen_for_writing ("ices.pid");
	
	fprintf (pidfd, "%d", icespid);
	
	ices_util_fclose (pidfd);
}

		
