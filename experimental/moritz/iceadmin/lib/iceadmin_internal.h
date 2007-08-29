#ifndef __ICEADMIN_INTERNAL_H__
#define __ICEADMIN_INTERNAL_H__

#include <libxml/parser.h>

/* Args: None */
#define PFX_GET_STATS		"/admin/stats"
/* Args: None */
#define PFX_LIST_MOUNTS 	"/admin/listmounts"
/* Args: mount= */
#define PFX_LIST_CLIENTS	"/admin/listclients?"
/* Args: (charset=), mount=, (song= || artist=, title=) */
#define PFX_UPD_METADATA	"/admin/metadata?charset=UTF-8&mode=updinfo&"
/* Args: mount=, fallback= */
#define PFX_UPD_FALLBACK	"/admin/fallbacks?"
/* Args: mount=, destination= */
#define PFX_MOVE_CLIENTS	"/admin/moveclients?"
/* Args: mount=, id= */
#define PFX_KILL_CLIENT 	"/admin/killclient?"
/* Args: mount= */
#define PFX_KILL_SOURCE 	"/admin/killsource?"
/* Args: mount= */
#define PFX_AUTH_LISTUSERS	"/admin/manageauth?"
/* Args: mount=, username=, password= */
#define PFX_AUTH_ADDUSER	"/admin/manageauth?action=add&"
/* Args: mount=, username= */
#define PFX_AUTH_DELUSER	"/admin/manageauth?action=delete&"

#ifndef BUFSIZ
# define BUFSIZ 		1024
#endif /* !BUFSIZ */

struct iceadmin {
	char		*hostname;
	char		*service;
	struct sockaddr *address;
	int		 domain;
	int		 type;
	int		 protocol;
	socklen_t	 addrsize;
	char		*mountpoint;
	char		*username;
	char		*password;
	char		*errmsg;
	int		 errnum;
	int		 gai_err;
	int		 initialized;
};

char *	_iceadmin_url_encode(const char *);
char *	_iceadmin_url_decode(const char *);
char *	_iceadmin_auth_basic_string(const char *, const char *);
int	_iceadmin_base64_encode(const char *, size_t, char **, size_t *);
int	_iceadmin_read_http_reply(struct iceadmin *, int, unsigned int *);
char *	_iceadmin_strnstr(const char *, const char *, size_t, size_t *);
int	_iceadmin_read_xml_data(struct iceadmin *, int, char **, size_t *);
int	_iceadmin_connect(struct iceadmin *, int *);
int	_iceadmin_format_xmltree(struct iceadmin *, char **, size_t *,
				 xmlDocPtr);
void	_iceadmin_walk_xmltree(char **, size_t *, xmlDocPtr, xmlNodePtr,
			       char **, size_t, unsigned int);
int	_iceadmin_get_xml_reply(char *, size_t, char *, size_t);

#ifndef HAVE_STRLCAT
# define strlcat	_iceadmin_strlcat
#endif /* !HAVE_STRLCAT */
size_t	_iceadmin_strlcat(char *, const char *, size_t);

#endif /* __ICEADMIN_INTERNAL_H__   */
