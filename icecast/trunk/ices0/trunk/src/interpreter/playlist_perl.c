/* playlist_perl.c
 * - Interpreter functions for perl
 * Copyright (c) 2000 Chad Armstrong, Alexander Haväng
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

int
interpreter_playlist_perl_get_current_lineno ()
{
 	char *str;
	int ret=0;
	
	str = interpreter_perl_eval_function ("ices_perl_get_current_lineno");
	ret = atoi(str);
	ices_util_free(str);
	if (!ret) ices_log_error ("Execution of 'ices_perl_get_current_lineno()' in ices.pl failed");
	return ret;
}

char *
interpreter_playlist_perl_get_next ()
{
	return interpreter_perl_eval_function ("ices_perl_get_next");
	// implied free(str)
}

int
interpreter_playlist_perl_initialize (ices_config_t *ices_config)
{
        char *str;
        int ret=0;
			
        str = interpreter_perl_eval_function ("ices_perl_initialize");
        ret = atoi(str);
	ices_util_free(str);
		
        if (!ret) ices_log_error ("Execution of 'ices_perl_initialize()' in ices.pl failed");
        return ret;

}


int
interpreter_playlist_perl_shutdown (ices_config_t *ices_config)
{
        char *str;
        int ret=0;
			                  
        str = interpreter_perl_eval_function ("ices_perl_shutdown");
	ret = atoi(str);
	ices_util_free(str);

        if (!ret) ices_log_error ("Execution of 'ices_perl_shutdown()' in ices.pl failed");
        return ret;
}

