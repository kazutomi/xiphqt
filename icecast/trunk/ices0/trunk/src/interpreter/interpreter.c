/* interpreter.c
 * - Interpreter functions
 * Copyright (c) 2000 Alexander Havang
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

#if defined(HAVE_PYTHON_H) && defined(HAVE_LIBPYTHON)
static void interpreter_python_init ();
static void interpreter_python_shutdown ();
#include "python.c"
#endif

#if defined(HAVE_PERL_H) && defined(HAVE_LIBPERL)
static void interpreter_perl_init ();
static void interpreter_perl_shutdown ();
#include "perl.c"
#endif

void
interpreter_init ()
{
#ifdef HAVE_LIBPYTHON
	if (ices_config.playlist_type == ices_playlist_python_e)
		interpreter_python_init();
#endif
#ifdef HAVE_LIBPERL
	if (ices_config.playlist_type == ices_playlist_perl_e)
		interpreter_perl_init();
#endif
}

void
interpreter_shutdown ()
{
#ifdef HAVE_LIBPYTHON
	if (ices_config.playlist_type == ices_playlist_python_e)
		interpreter_python_shutdown ();
#endif
#ifdef HAVE_LIBPERL
	if (ices_config.playlist_type == ices_playlist_perl_e)
		interpreter_perl_shutdown ();
#endif
}


