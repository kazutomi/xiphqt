/* playlist_perl.c
 * - Interpreter functions for perl
 * Copyright (c) 2000 Chad Armstrong, Alexander Haväng
 * Copyright (c) 2001 Brendan Cully
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

/* Stupid automake and STUPID perl */
#ifdef PACKAGE
#undef PACKAGE
#endif

#include <EXTERN.h>
#include <perl.h>

/* -- local prototypes -- */
static int playlist_perl_get_lineno (void);
static char* playlist_perl_get_next (void);
static char* playlist_perl_get_metadata (void);
static void playlist_perl_shutdown (void);

int
interpreter_playlist_perl_initialize (playlist_module_t* pm)
{
  char *str;
  int ret = 0;

  pm->get_next = playlist_perl_get_next;
  pm->get_metadata = playlist_perl_get_metadata;
  pm->get_lineno = playlist_perl_get_lineno;
  pm->shutdown = playlist_perl_shutdown;

  str = interpreter_perl_eval_function ("ices_perl_initialize");
  ret = atoi (str);	/* allocated in perl.c */
  ices_util_free (str);	/* clean up after yourself! */
		
  if (!ret) 
    ices_log_error ("Execution of 'ices_perl_initialize()' in ices.pm failed");

  return ret;
}

static int
playlist_perl_get_lineno (void)
{
 	char *str;
	int ret = 0;
	
	str = interpreter_perl_eval_function ("ices_perl_get_current_lineno");
	ret = atoi (str); 	/* allocated in perl.c */
	ices_util_free (str);	/* clean up after yourself! */

	if (!ret) 
		ices_log_error ("Execution of 'ices_perl_get_current_lineno()' in ices.pm failed");

	return ret;
}

static char *
playlist_perl_get_next (void)
{
	return interpreter_perl_eval_function ("ices_perl_get_next");
	/* implied free(str), this is called higher up */
}

static char*
playlist_perl_get_metadata (void)
{
  return interpreter_perl_eval_function ("ices_perl_get_metadata");
}

static void
playlist_perl_shutdown (void)
{
        char *str;
        int ret = 0;
			                  
        str = interpreter_perl_eval_function ("ices_perl_shutdown");
	ret = atoi (str);
	ices_util_free (str);

        if (!ret) 
		ices_log_error ("Execution of 'ices_perl_shutdown()' in ices.pm failed");

        return;
}
