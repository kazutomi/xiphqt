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

int
ices_playlist_initialize ()
{
	ices_log_debug ("Initializing playlist handler...");

	switch (ices_config.playlist_type) {
		case ices_playlist_builtin_e:
			return ices_playlist_builtin_initialize (&ices_config);
			break;
#ifdef HAVE_LIBPYTHON
		case ices_playlist_python_e:
			return interpreter_playlist_python_initialize (&ices_config);
			break;
#endif
#ifdef HAVE_LIBPERL
		case ices_playlist_perl_e:
			return interpreter_playlist_perl_initialize (&ices_config);
			break;
#endif
		default:
			ices_log_error ("Unknown playlist module!");
			return 0;
	}
}

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
	




