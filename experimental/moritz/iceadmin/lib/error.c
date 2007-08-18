#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iceadmin_internal.h"
#include "error.h"
#include "xalloc.h"

void
_set_error(struct iceadmin *iceadmin, int from_netdb, int errnum, const char *fmt, ...)
{
	va_list ap;
	char	errmsg[ERRMSG_MAX];

	if (iceadmin->errmsg != NULL)
		xfree(iceadmin->errmsg);

	va_start(ap, fmt);
	vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);

	iceadmin->errmsg = xstrdup(errmsg);
	iceadmin->errnum = errnum;
	iceadmin->gai_err = from_netdb;
}

void
_clear_error(struct iceadmin *iceadmin)
{
	if (iceadmin->errmsg != NULL) {
		xfree(iceadmin->errmsg);
		iceadmin->errmsg = NULL;
	}
	iceadmin->errnum = 0;
}

void
_get_error_string(struct iceadmin *iceadmin, char *buf, size_t bufsize)
{
	if (iceadmin->errmsg == NULL && iceadmin->errnum == 0) {
		snprintf(buf, bufsize, "No error\n");
		return;
	}

	if (iceadmin->errmsg != NULL) {
		if (iceadmin->errnum != 0) {
			snprintf(buf, bufsize, "%s: %s\n", iceadmin->errmsg,
				 iceadmin->gai_err ?
				 gai_strerror(iceadmin->errnum) :
				 strerror(iceadmin->errnum));
		} else {
			snprintf(buf, bufsize, "%s\n", iceadmin->errmsg);
		}
	} else {
		snprintf(buf, bufsize, "%s\n",
			 iceadmin->gai_err ?
			 gai_strerror(iceadmin->errnum) :
			 strerror(iceadmin->errnum));
	}
}
