#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <errno.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <libxml/parser.h>

#include "iceadmin.h"
#include "iceadmin_internal.h"
#include "error.h"
#include "xalloc.h"

#ifndef SA_LEN
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
#  define SA_LEN(x)	((x)->sa_len)
# else
#  define SA_LEN(x)	((x)->sa_family == AF_INET6 ? \
			sizeof(struct sockaddr_in6) : \
			sizeof(struct sockaddr_in))
# endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */
#endif /* !SA_LEN */


char	*_iceadmin_useragent = NULL;

void
iceadmin_initialize(void)
{
	xalloc_initialize_debug(2, stderr);
	iceadmin_set_useragent(NULL);
	xmlInitParser();
}

void
iceadmin_shutdown(void)
{
	xmlCleanupParser();
	if (_iceadmin_useragent != NULL)
		xfree(_iceadmin_useragent);
	xalloc_shutdown();
}

void
iceadmin_get_error(struct iceadmin *iceadmin, char *buf, size_t bufsize)
{
	if (iceadmin == NULL || buf == NULL || bufsize == 0)
		return;

	_get_error_string(iceadmin, buf, bufsize);
}

void
iceadmin_set_useragent(const char *agent)
{
	size_t	size;

	if (_iceadmin_useragent != NULL)
		xfree(_iceadmin_useragent);

	if (agent == NULL) {
		size = strlen(PACKAGE_NAME) + 1 + strlen(PACKAGE_VERSION) + 1;
		_iceadmin_useragent = xmalloc(size);
		snprintf(_iceadmin_useragent, size, "%s/%s",
			 PACKAGE_NAME, PACKAGE_VERSION);
	} else {
		size = strlen(agent) + 1;
		_iceadmin_useragent = xmalloc(size);
		snprintf(_iceadmin_useragent, size, "%s", agent);
	}
}

int
iceadmin_new(struct iceadmin **iceadmin_p, const char *mountpoint,
	     const char *hostname, const char *service, const char *username,
	     const char *password)
{
	struct iceadmin *iceadmin = NULL;
	int		 error, success;
	struct addrinfo *res0, *res, hints;
	const char	*last_error;
	char		 connect_error[BUFSIZ];

	if (iceadmin_p == NULL || hostname == NULL || service == NULL ||
	    username == NULL || password == NULL)
		return (ICEADMIN_ABUSE);

	*iceadmin_p = xcalloc(1, sizeof(struct iceadmin));
	iceadmin = *iceadmin_p;

	iceadmin->hostname = xstrdup(hostname);
	iceadmin->service = xstrdup(service);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((error = getaddrinfo(hostname, service, &hints, &res0)) != 0) {
		_set_error(iceadmin, 1, error, "getaddrinfo: [%s]:%s",
			   hostname, service);
		return (ICEADMIN_ERROR);
	}

	snprintf(connect_error, sizeof(connect_error), "connect: [%s]:%s",
		 hostname, service);

	success = 0;
	last_error = NULL;
	for (res = res0; res != NULL; res = res->ai_next) {
		int	sock;

		if ((sock = socket(res->ai_family, res->ai_socktype,
				   res->ai_protocol)) == -1) {
			last_error = "socket";
			error = errno;
			continue;
		}

		if ((connect(sock, res->ai_addr, res->ai_addrlen)) == -1) {
			last_error = connect_error;
			error = errno;
			close(sock);
			continue;
		}

		close(sock);
		iceadmin->addrsize = res->ai_addrlen;
		iceadmin->address = xcalloc(1, iceadmin->addrsize);
		memcpy(iceadmin->address, res->ai_addr, iceadmin->addrsize);
		iceadmin->domain = res->ai_family;
		iceadmin->type = res->ai_socktype;
		iceadmin->protocol = res->ai_protocol;
		success = 1;
		error = 0;
		last_error = NULL;

		break;
	}
	freeaddrinfo(res0);

	if (!success) {
		_set_error(iceadmin, 0, error, last_error);
		return (ICEADMIN_ERROR);
	}

	if (mountpoint == NULL)
		iceadmin->mountpoint = xstrdup("");
	else
		iceadmin->mountpoint = xstrdup(mountpoint);
	iceadmin->username = xstrdup(username);
	iceadmin->password = xstrdup(password);

	iceadmin->initialized = 1;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_delete_c(struct iceadmin **iceadmin_p)
{
	struct iceadmin *iceadmin;

	if (iceadmin_p == NULL || (iceadmin = *iceadmin_p) == NULL ||
	    iceadmin == NULL)
		return (ICEADMIN_ABUSE);

	_clear_error(iceadmin);

	if (iceadmin->hostname != NULL)
		xfree(iceadmin->hostname);
	if (iceadmin->service != NULL)
		xfree(iceadmin->service);
	if (iceadmin->address != NULL)
		xfree(iceadmin->address);
	if (iceadmin->mountpoint != NULL)
		xfree(iceadmin->mountpoint);
	if (iceadmin->username != NULL)
		xfree(iceadmin->username);
	if (iceadmin->password != NULL)
		xfree(iceadmin->password);

	memset(iceadmin, 0, sizeof(struct iceadmin));
	xfree(*iceadmin_p);
	*iceadmin_p = NULL;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_free_c(void **buf_p)
{
	char	*buf;

	if (buf_p == NULL)
		return (ICEADMIN_ERROR);

	if ((buf = (char *)*buf_p) != NULL) {
		xfree(buf);
		*buf_p = NULL;
	}

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_get_stats(struct iceadmin *iceadmin,
		       char **buf_p, size_t *bufsize_p)
{
	char		*buf;
	size_t		 bufsize;
	xmlDocPtr	 doc;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);

	if (iceadmin_cmd_get_xmlstats(iceadmin, &buf, &bufsize) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	if ((doc = xmlParseMemory(buf, bufsize)) == NULL) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: XML parse error (not well-formed)",
			   iceadmin->hostname, iceadmin->service);
		xfree(buf);
		return (ICEADMIN_ERROR);
	}
	xfree(buf);

	if (_iceadmin_format_xmltree(iceadmin, &buf, &bufsize, doc) != ICEADMIN_SUCCESS) {
		xmlFreeDoc(doc);
		return (ICEADMIN_ERROR);
	}
	xmlFreeDoc(doc);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_get_xmlstats(struct iceadmin *iceadmin,
			  char **buf_p, size_t *bufsize_p)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_GET_STATS, _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_list_mounts(struct iceadmin *iceadmin,
			 char **buf_p, size_t *bufsize_p)
{
	char		*buf;
	size_t		 bufsize;
	xmlDocPtr	 doc;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);

	if (iceadmin_cmd_list_xmlmounts(iceadmin, &buf, &bufsize) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	if ((doc = xmlParseMemory(buf, bufsize)) == NULL) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: XML parse error (not well-formed)",
			   iceadmin->hostname, iceadmin->service);
		xfree(buf);
		return (ICEADMIN_ERROR);
	}
	xfree(buf);

	if (_iceadmin_format_xmltree(iceadmin, &buf, &bufsize, doc) != ICEADMIN_SUCCESS) {
		xmlFreeDoc(doc);
		return (ICEADMIN_ERROR);
	}
	xmlFreeDoc(doc);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_list_xmlmounts(struct iceadmin *iceadmin,
			    char **buf_p, size_t *bufsize_p)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_LIST_MOUNTS, _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_list_clients(struct iceadmin *iceadmin,
			  char **buf_p, size_t *bufsize_p)
{
	char		*buf;
	size_t		 bufsize;
	xmlDocPtr	 doc;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (iceadmin_cmd_list_xmlclients(iceadmin, &buf, &bufsize) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	if ((doc = xmlParseMemory(buf, bufsize)) == NULL) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: XML parse error (not well-formed)",
			   iceadmin->hostname, iceadmin->service);
		xfree(buf);
		return (ICEADMIN_ERROR);
	}
	xfree(buf);

	if (_iceadmin_format_xmltree(iceadmin, &buf, &bufsize, doc) != ICEADMIN_SUCCESS) {
		xmlFreeDoc(doc);
		return (ICEADMIN_ERROR);
	}
	xmlFreeDoc(doc);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_list_xmlclients(struct iceadmin *iceadmin,
			     char **buf_p, size_t *bufsize_p)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount;

	if (iceadmin == NULL || buf_p == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_LIST_CLIENTS, enc_mount, _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_kill_source(struct iceadmin *iceadmin)
{
	char		*auth;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount;

	if (iceadmin == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_KILL_SOURCE, enc_mount, _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_kill_client(struct iceadmin *iceadmin, unsigned int client_id)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount;

	if (iceadmin == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s&id=%u HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_KILL_CLIENT, enc_mount, client_id,
		       _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	ret = _iceadmin_get_xml_reply(buf, bufsize, static_buf, sizeof(static_buf));
	xfree(buf);

	switch (ret) {
	case 1:
		/* Okay */
		break;
	case 0:
		_set_error(iceadmin, 0, 0, "[%s]:%s: %s",
			   iceadmin->hostname, iceadmin->service,
			   static_buf);
		return (ICEADMIN_FAILURE);
	default:
		_set_error(iceadmin, 0, 0, "[%s]:%s: Bad XML reply",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_move_clients(struct iceadmin *iceadmin, const char *destination)
{
	char		*auth;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char            *enc_mount, *enc_dest;

	if (iceadmin == NULL || destination == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (strcmp(iceadmin->mountpoint, destination) == 0) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' and destination '%s' are identical",
			   iceadmin->hostname, iceadmin->service,
			   iceadmin->mountpoint, destination);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
        enc_dest = _iceadmin_url_encode(destination);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s&destination=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_MOVE_CLIENTS, enc_mount, enc_dest,
		       _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_dest);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_dest);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' or destination '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint, destination);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_update_fallback(struct iceadmin *iceadmin, const char *fallback)
{
	char		*auth;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount, *enc_fallback;

	if (iceadmin == NULL || fallback == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	enc_fallback = _iceadmin_url_encode(fallback);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s&fallback=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_UPD_FALLBACK, enc_mount, enc_fallback,
		       _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_fallback);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_fallback);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_update_metadata(struct iceadmin *iceadmin, const char *metadata)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount, *enc_meta;

	if (iceadmin == NULL || metadata == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	enc_meta = _iceadmin_url_encode(metadata);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s&song=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_UPD_METADATA, enc_mount, enc_meta,
		       _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_meta);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_meta);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	ret = _iceadmin_get_xml_reply(buf, bufsize, static_buf, sizeof(static_buf));
	xfree(buf);

	switch (ret) {
	case 1:
		/* Okay */
		break;
	case 0:
		_set_error(iceadmin, 0, 0, "[%s]:%s: %s",
			   iceadmin->hostname, iceadmin->service,
			   static_buf);
		return (ICEADMIN_FAILURE);
	default:
		_set_error(iceadmin, 0, 0, "[%s]:%s: Bad XML reply",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	return (ICEADMIN_SUCCESS);
}

int
iceadmin_cmd_update_artist_title(struct iceadmin *iceadmin,
				 const char *artist, const char *title)
{
	char		*buf, *auth;
	size_t		 bufsize;
	int		 sock;
	char		 static_buf[BUFSIZ];
	unsigned int	 httpret;
	int		 ret;
	char		*enc_mount, *enc_artist, *enc_title;

	if (iceadmin == NULL || artist == NULL || title == NULL)
		return (ICEADMIN_ABUSE);
	if (iceadmin->initialized != 1)
		return (ICEADMIN_BADHANDLE);
	if (iceadmin->mountpoint[0] == '\0')
		return (ICEADMIN_ERROR);

	if (_iceadmin_connect(iceadmin, &sock) != ICEADMIN_SUCCESS)
		return (ICEADMIN_ERROR);

	enc_mount = _iceadmin_url_encode(iceadmin->mountpoint);
	enc_artist = _iceadmin_url_encode(artist);
	enc_title = _iceadmin_url_encode(title);
	auth = _iceadmin_auth_basic_string(iceadmin->username,
					   iceadmin->password);
	ret = snprintf(static_buf, sizeof(static_buf), "GET %smount=%s&artist=%s&title=%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
		       PFX_UPD_METADATA, enc_mount, enc_artist, enc_title,
		       _iceadmin_useragent, auth);
	if (ret < 0 || ret >= (int)sizeof(static_buf)) {
		_set_error(iceadmin, 0, 0, "Bad request -- too long");
		xfree(auth);
		xfree(enc_title);
		xfree(enc_artist);
		xfree(enc_mount);
		return (ICEADMIN_ERROR);
	}
	xfree(auth);
	xfree(enc_title);
	xfree(enc_artist);
	xfree(enc_mount);

	if (write(sock, static_buf, strlen(static_buf)) == -1) {
		_set_error(iceadmin, 0, errno, "write: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(sock);
		return (ICEADMIN_ERROR);
	}

	httpret = 0;
	if (_iceadmin_read_http_reply(iceadmin, sock, &httpret)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		if (httpret == 400)
			_set_error(iceadmin, 0, 0, "[%s]:%s: Mountpoint '%s' not found",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->mountpoint);
		return (ICEADMIN_ERROR);
	}

	if (_iceadmin_read_xml_data(iceadmin, sock, &buf, &bufsize)
	    != ICEADMIN_SUCCESS) {
		close(sock);
		return (ICEADMIN_ERROR);
	}
	close(sock);

	ret = _iceadmin_get_xml_reply(buf, bufsize, static_buf, sizeof(static_buf));
	xfree(buf);

	switch (ret) {
	case 1:
		/* Okay */
		break;
	case 0:
		_set_error(iceadmin, 0, 0, "[%s]:%s: %s",
			   iceadmin->hostname, iceadmin->service,
			   static_buf);
		return (ICEADMIN_FAILURE);
	default:
		_set_error(iceadmin, 0, 0, "[%s]:%s: Bad XML reply",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	return (ICEADMIN_SUCCESS);
}
