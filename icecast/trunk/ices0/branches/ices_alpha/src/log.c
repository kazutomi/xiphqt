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

char lasterror[BUFSIZE];
extern ices_config_t ices_config;

/* Public function definitions */
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

	fprintf (stderr, "DEBUG: %s\n", buff);
}

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

	fprintf (stderr, "%s\n", buff);
}

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

char *
ices_log_get_error ()
{
	return lasterror;
}

void
_log (char *type, int level, char *fmt, ...)
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
	
	fprintf (stderr, "DEBUG: %s:%d %s\n", type, level, buff);
}

