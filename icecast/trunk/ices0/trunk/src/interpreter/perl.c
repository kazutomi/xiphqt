/* perl.c
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
#include <EXTERN.h>
#include <perl.h>

static PerlInterpreter *my_perl;

static void
interpreter_perl_init ()
{
	static char *my_argv[] = {"", "ices.pl"};
	
	ices_log_debug ("Importing Perl module...");

	char *args[] = { NULL };
	if((my_perl = perl_alloc()) == NULL) {
		ices_log_debug ("perl_alloc() error: (no memory!*");
		return;
	}
	perl_construct(my_perl);

	if (perl_parse(my_perl, NULL, 2, my_argv, NULL)) {
		ices_log_debug ("perl_parse() error: parse problem");
	}
}

static void
interpreter_perl_setup_path ()
{
	char *perlpath = getenv ("PERLPATH");
	char newpath[1024];

	if (perlpath && (strlen (perlpath) > 900)) {
		ices_log ("Environment variable PERLPATH is too long, please rectify!");
		exit (0);
	}

	if (perlpath) {
		sprintf (newpath, "%s:%s:.", perlpath, ICES_MODULEDIR);
	} else {
		sprintf (newpath, "%s:.", ICES_MODULEDIR);
	}

	putenv (newpath);
}

static void
interpreter_perl_shutdown ()
{
	perl_destruct(my_perl);
	perl_free(my_perl);
}

void *
interpreter_perl_eval_function (char *functionname)
{
	dSP;
	int retcount = 0;
	char *retstr;
	
	ices_log_debug ("Interpreting [%s]", functionname);
	
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	PUTBACK;
	
	retcount = perl_call_pv(functionname, G_SCALAR);
	
	retstr = ices_util_strdup (POPs);
	
	ices_log_debug ("perl [%s] returned %d values, last [%s]", functionname, retcount, retstr);
	
	SPAGAIN;
	FREETMPS;
	LEAVE;
	
	ices_log_debug ("Done interpreting [%s]", functionname);
	
	return retstr;
}






