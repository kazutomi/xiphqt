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

/* Stupid automake and STUPID perl */
#ifdef PACKAGE
#undef PACKAGE
#endif

#include <EXTERN.h> 
#include <perl.h> 

static PerlInterpreter *my_perl;

/* most of the following is almost ripped straight out of 'man perlcall' or 'man perlembed' 
 *
 * my_perl is left resident, and we do not reload the perl module file when it changes.
 * shutdown() will clean up anything allocated at this point
 */

static void
interpreter_perl_initialize ()
{
	static char *my_argv[] = {"", "ices.pm"}; 	/* dummy arguments, hardcoded pearl module */
	
	ices_log_debug ("Importing Perl module ices.pm:");

	if((my_perl = perl_alloc()) == NULL) {
		ices_log_debug ("perl_alloc() error: (no memory!*");
		ices_setup_shutdown();
		return;
	}
	perl_construct(my_perl);

	if (perl_parse(my_perl, NULL, 2, my_argv, NULL)) {
		ices_log_debug ("perl_parse() error: parse problem");
		ices_setup_shutdown();
	}
}

/* cleanup time! */
static void
interpreter_perl_shutdown ()
{
	if (my_perl != NULL){
		perl_destruct(my_perl);
		perl_free(my_perl);
	}
}


/*
 *  Here be magic...
 *  man perlcall gave me the following steps, to be
 *  able to handle getting return values from the
 *  embedded perl calls.
 */	
void *
interpreter_perl_eval_function (char *functionname)
{
	dSP;				/* initialize stack pointer      */
	int retcount = 0;
	char *retstr;
	
	ices_log_debug ("Interpreting [%s]", functionname);
	
	ENTER;				/* everything created after here */
	SAVETMPS;			/* ...is a temporary variable.   */
	PUSHMARK(SP);			/* remember the stack pointer    */
	PUTBACK;			/* make local stack pointer global */
	
	retcount = perl_call_pv(functionname, G_SCALAR);	/* make the call */
	
	SPAGAIN;			/* refresh stack pointer         */
	
	/* we're calling strdup here, free() this later! */
	retstr = ices_util_strdup (POPp);/* pop the return value from stack */        
	ices_log_debug ("perl [%s] returned %d values, last [%s]", functionname, retcount, retstr);
	
	FREETMPS;			/* free that return value        */
	LEAVE;				/* ...and the XPUSHed "mortal" args.*/
	
	ices_log_debug ("Done interpreting [%s]", functionname);
	
	return retstr;
}
