/* playlist.c
 * - Functions for playlist handling
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

/* Global function definitions */

/* Wrapper function for the specific playlist handler's current line number.
 * This might not be available if your playlist is a database or something
 * weird, but it's just for the cue file so it doesn't matter much */
int
ices_playlist_get_current_lineno ()
{
	switch (ices_config.playlist_type) {
		case ices_playlist_builtin_e:
			return ices_playlist_builtin_get_current_lineno ();
			break;
#ifdef HAVE_LIBPYTHON
		case ices_playlist_python_e:
			return interpreter_playlist_python_get_current_lineno ();
			break;
#endif
#ifdef HAVE_LIBPERL
		case ices_playlist_perl_e:
			return interpreter_playlist_perl_get_current_lineno ();
			break;
#endif
		default:
			ices_log_error ("Unknown playlist module!");
			return -1;
	}
}

/* Wrapper for the playlist handler's next file function.
 * Remember that if this returns non-NULL then the return
 * value is free()ed by the caller. */
char *
ices_playlist_get_next ()
{
	switch (ices_config.playlist_type) {
		case ices_playlist_builtin_e:
			return ices_playlist_builtin_get_next ();
			break;
#ifdef HAVE_LIBPYTHON
		case ices_playlist_python_e:
			return interpreter_playlist_python_get_next ();
			break;
#endif
#ifdef HAVE_LIBPERL
		case ices_playlist_perl_e:
			return interpreter_playlist_perl_get_next ();
			break;
#endif
		default:
			ices_log_error ("Unknown playlist module!");
			return NULL;
	}
}

/* Initialize the toplevel playlist handler */
int
ices_playlist_initialize ()
{
	int res = 0;
	ices_log_debug ("Initializing playlist handler...");

	switch (ices_config.playlist_type) {
		case ices_playlist_builtin_e:
			res = ices_playlist_builtin_initialize (&ices_config);
			break;
		case ices_playlist_python_e:
#ifdef HAVE_LIBPYTHON
			res = interpreter_playlist_python_initialize (&ices_config);
#else
			ices_log_error ("This binary has no support for embedded python");
#endif
			break;

		case ices_playlist_perl_e:
#ifdef HAVE_LIBPERL
			res = interpreter_playlist_perl_initialize (&ices_config);
#else
			ices_log_error ("This binary has no support for embedded perl");
#endif
			break;
		default:
			ices_log_error ("Unknown playlist module!");
			break;
	}

	if (res == 0) {
		ices_log ("Initialization of playlist handler failed. [%s]", ices_log_get_error ());
		ices_setup_shutdown ();
	}

	return res;
}

/* Shutdown the playlist handler */
int
ices_playlist_shutdown ()
{	
	switch (ices_config.playlist_type) {
		case ices_playlist_builtin_e:
			return ices_playlist_builtin_shutdown (&ices_config);
			break;
#ifdef HAVE_LIBPYTHON
		case ices_playlist_python_e:
			return interpreter_playlist_python_shutdown (&ices_config);
			break;
#endif
#ifdef HAVE_LIBPERL
		case ices_playlist_perl_e:
			return interpreter_playlist_perl_shutdown (&ices_config);
			break;
#endif
		default:
			ices_log_error ("Unknown playlist module!");
			return 0;
	}
}
