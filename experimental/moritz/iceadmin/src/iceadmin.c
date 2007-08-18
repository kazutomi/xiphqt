#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <iceadmin.h>
#include <xalloc.h>

void	init_self(void);
int	shutdown_self(int);

void
init_self(void)
{
	xalloc_initialize_debug(2, stderr);
	iceadmin_initialize();
}

int
shutdown_self(int ret)
{
	iceadmin_shutdown();
	xalloc_shutdown();

	return (ret);
}

int
main(void)
{
	iceadmin_t	 iceadmin;
	char		 errbuf[256];
	char		*buf;
	size_t		 bufsize;
	int		 ret;

	init_self();

	if (iceadmin_new(&iceadmin, "/stream.ogg", "example.domain", "8000", "admin", "hackme") != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "%s", errbuf);
		iceadmin_delete(iceadmin);
		return (shutdown_self(1));
	}

	if ((ret = iceadmin_cmd_get_stats(iceadmin, &buf, &bufsize)) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "Get status: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}

	printf("%s\n", buf);
	iceadmin_free(buf);

	if ((ret = iceadmin_cmd_list_mounts(iceadmin, &buf, &bufsize)) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "List mounts: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}

	printf("%s\n", buf);
	iceadmin_free(buf);

	if ((ret = iceadmin_cmd_list_clients(iceadmin, &buf, &bufsize)) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "List clients: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}

	printf("%s\n", buf);
	iceadmin_free(buf);

	/*
	if ((ret = iceadmin_cmd_kill_source(iceadmin)) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "Kill source: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}
	*/

	if ((ret = iceadmin_cmd_kill_client(iceadmin, 354)) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "Kill client: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}

	if ((ret = iceadmin_cmd_update_metadata(iceadmin, "Yay! Test!?!?!")) != ICEADMIN_SUCCESS) {
		iceadmin_get_error(iceadmin, errbuf, sizeof(errbuf));
		fprintf(stderr, "Update metadata: %s", errbuf);
		if (ret != ICEADMIN_FAILURE) {
			iceadmin_delete(iceadmin);
			return (shutdown_self(1));
		}
	}

	iceadmin_delete(iceadmin);

	return (shutdown_self(0));
}
