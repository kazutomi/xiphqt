/* log.c
 * - Functions for logging in ices
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

extern ices_config_t ices_config;

/* Private function declarations */
static void ices_log_string (char *format, char *string);
static int ices_log_open_logfile ();
static int ices_log_close_logfile ();

static char lasterror[BUFSIZE];
/* Public function definitions */

/* Initialize the log module, creates log file */
void
ices_log_initialize ()
{
	if (!ices_log_open_logfile ()) {
		ices_log ("%s", ices_log_get_error ());
	}
	ices_log ("Logfile opened");
}

/* Shutdown the log module, close the logfile */
void
ices_log_shutdown ()
{
	if (!ices_log_close_logfile ()) {
		ices_log ("%s", ices_log_get_error ());
	}
}

/* Close everything, start up with clean slate when 
 * run as a daemon */
void
ices_log_daemonize ()
{
	close (0); 
	close (1); 
	close (2);

	freopen ("/dev/null", "r", stdin);
	freopen ("/dev/null", "w", stdout);
	freopen ("/dev/null", "w", stderr);

	ices_log_reopen_logfile ();
}

/* Cycle the logfile, usually called from the SIGHUP handler */
int
ices_log_reopen_logfile ()
{
	ices_log_close_logfile ();
	return ices_log_open_logfile ();
}

/* Log only if verbose mode is set. Prepend output with DEBUG:  */
void
ices_log_debug (const char *fmt, ...)
{
	char buff[BUFSIZE];
	va_list ap;

	if (!ices_config.verbose)
		return;

	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buff, BUFSIZE, fmt, ap);
#else
	vsprintf(buff, fmt, ap);
#endif
	va_end(ap);

	ices_log_string ("DEBUG: %s\n", buff);
}

/* Log to console and file */
void
ices_log (const char *fmt, ...)
{
	char buff[BUFSIZE];
	va_list ap;

	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buff, BUFSIZE, fmt, ap);
#else
	vsprintf(buff, fmt, ap);
#endif
	va_end(ap);

	ices_log_string ("%s\n", buff);
}

/* Store error information in module memory */
void
ices_log_error (const char *fmt, ...)
{
	char buff[BUFSIZE];
	va_list ap;

	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buff, BUFSIZE, fmt, ap);
#else
	vsprintf(buff, fmt, ap);
#endif
	va_end(ap);

	strncpy (lasterror, buff, BUFSIZE);
}

/* Get last error from log module */
char *
ices_log_get_error ()
{
	return lasterror;
}

/* Specific log method for thread info */
void
thread_log (char *type, int level, char *fmt, ...)
{
	char buff[BUFSIZE];
	va_list ap;

	if (!ices_config.verbose)
		return;
	
	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buff, BUFSIZE, fmt, ap);
#else
	vsprintf(buff, fmt, ap);
#endif
	va_end(ap);

	ices_log_string ("DEBUG: %s\n", buff);
}

/* Private function definitions */

/* Function to log string to both console and file */
static void
ices_log_string (char *format, char *string)
{
	if (ices_config.logfile) {
		fprintf (ices_config.logfile, format, string);
#ifndef HAVE_SETLINEBUF
		fflush (ices_config.logfile);
#endif
	}
	
	/* Don't log to console when daemonized */
	if (!ices_config.daemon) {
		fprintf (stdout, format, string);
	}
}

/* Open the ices logfile, create it if needed */
static int
ices_log_open_logfile ()
{
	char namespace[1024], errorspace[1024];
	char *filename;
	FILE *logfp;

	filename = ices_util_get_random_filename (namespace, "log");
	
	logfp = ices_util_fopen_for_writing (filename);

	if (!logfp) {
		ices_log_error ("Error while opening %s, error: %s", ices_util_strerror (errno, errorspace, 1024));
		return 0;
	}

	ices_config.logfile = logfp;
#ifdef HAVE_SETLINEBUF
	setlinebuf (ices_config.logfile);
#endif

	return 1;
}

/* Close ices' logfile */
static int
ices_log_close_logfile ()
{
	if (ices_config.logfile) {
		ices_util_fclose (ices_config.logfile);
	}

	return 1;
}
