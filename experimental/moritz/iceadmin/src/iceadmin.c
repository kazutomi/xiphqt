#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <iceadmin.h>

#include "cfg.h"
#include "xalloc.h"

#ifdef HAVE___PROGNAME
extern char		*__progname;
#else
char			*__progname;
#endif /* HAVE___PROGNAME */

void	init_self(void);
int	shutdown_self(int);
char *  get_progname(const char *);

void
init_self(void)
{
	xalloc_initialize_debug(2, stderr);
	iceadmin_initialize();
	cfg_initialize();
}

int
shutdown_self(int ret)
{
	cfg_shutdown();
	iceadmin_shutdown();
	xalloc_shutdown();

	return (ret);
}

char *
get_progname(const char *argv0)
{
#ifdef HAVE___PROGNAME
	return (strdup(__progname));
#else
	char	*p;

	if (argv0 == NULL)
		return ((char *)"iceadmin");
	p = strrchr(argv0, PATH_SEPARATOR);
	if (p == NULL)
		p = (char *)argv0;
	else
		p++;

	return (strdup(p));
#endif /* HAVE___PROGNAME */
}

int
main(int argc, char *argv[])
{
	iceadmin_t	 iceadmin;
	char		 errbuf[256];
	char		*buf;
	size_t		 bufsize;
	int		 ret;

	__progname = get_progname(argv[0]);

	init_self();

	if ((ret = cfg_parse_cmdline(argc, argv)) != CONFIG_OKAY)
		return (shutdown_self(ret));

	return (shutdown_self(0));
}
