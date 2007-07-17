/*
 * Copyright (c) 2007 Moritz Grimm <mdgrimm@gmx.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "compat.h"

/*
 * Modified basename() implementation from OpenBSD, based on:
 * $OpenBSD: basename.c,v 1.14 2005/08/08 08:05:33 espie Exp $
 * Copyright (c) 1997, 2004 Todd C. Miller <Todd.Miller@courtesan.com>
 */
char *
local_basename(const char *path)
{
	static char	 bname[PATH_MAX];
	size_t		 len;
	const char	*startp, *endp;

	if (path == NULL || *path == '\0') {
		bname[0] = '.';
		bname[1] = '\0';
		return (bname);
	}

	/* Strip any trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == PATH_SEPARATOR)
		endp--;

	/* All slashes become "\" */
	if (endp == path && *endp == PATH_SEPARATOR) {
		bname[0] = PATH_SEPARATOR;
		bname[1] = '\0';
		return (bname);
	}

	/* Find the start of the base */
	startp = endp;
	while (startp > path && *(startp - 1) != PATH_SEPARATOR)
		startp--;

	len = endp - startp + 1;
	if (len >= sizeof(bname)) {
		errno = ENAMETOOLONG;
		return (NULL);
	}
	memcpy(bname, startp, len);
	bname[len] = '\0';

	return (bname);
}
