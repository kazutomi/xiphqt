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

/* Most people will think this is very ugly.. what the hell is this
 * guy thinking, including .c files and god know's what else :)
 * It's actually not all that bad, cause it's a nice way to do
 * conditional compiling.
 * Now, say some chmoboe wants to add support for a new interpreted
 * language, what does he have to add?
 * First, in configure.in, check for your interpreters CFLAGS, LIBS,
 * header files and all that junk. Do what we do for perl and python
 * cause that works fine.
 * Make sure configure defines HAVE_LANGUAGE_H and HAVE_LIBLANGUAGE,
 * or whatever the suitable header/lib defines will be.
 * Then, in this file, do like we do with python and perl, #ifdef
 * around a .c include, and statically declare the initialization
 * and shutdown functions. Then further down in this file, add
 * #ifdefs and if()s to call the initialization and shutdown
 * functions. Can't get much easier than that can it? 
 * The file $language.c in this directory (#included here), should
 * contain the api to setup, shutdown, and call functions in
 * your language.
 * The file playlist_$language.c in this directory should contain
 * 4 function definitions, interpreter_playlist_$language_initialize(),
 * interpreter_playlist_$language_shutdown(), 
 * interpreter_playlist_$language_get_next(), and 
 * interpreter_playlist_$language_get_current_lineno().
 * The declarations to these functions must be added to interpreter.h,
 * along with whatever api function you need to call in language.c.
 * That should be pretty much it, just don't forget to add your .c files
 * to Makefile.am in the directory (the EXTRA_DIST stuff), or your
 * .c files won't make it to the distribution.
 * And you have got 2 fully working examples to work with, you can't
 * fail :)
 */ 

#if defined(HAVE_PYTHON_H) && defined(HAVE_LIBPYTHON)
static void interpreter_python_initialize ();
static void interpreter_python_shutdown ();
/* python defines _REENTRANT */
#undef _REENTRANT
#include "python.c"
#endif

#if defined(HAVE_LIBPERL)
# ifdef ANY
#  undef ANY
# endif
static void interpreter_perl_initialize ();
static void interpreter_perl_shutdown ();
#include "perl.c"
#endif

/* Initialize the specified interpreter */
void
interpreter_initialize ()
{
#ifdef HAVE_LIBPYTHON
	if (ices_config.playlist_type == ices_playlist_python_e)
		interpreter_python_initialize();
#endif
#ifdef HAVE_LIBPERL
	if (ices_config.playlist_type == ices_playlist_perl_e)
		interpreter_perl_initialize();
#endif
}

/* Shutdown the specified interpreter */
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
