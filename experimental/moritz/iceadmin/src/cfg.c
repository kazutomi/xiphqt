#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <sys/tree.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "cfg.h"
#include "xalloc.h"

#ifndef PATH_MAX
# define PATH_MAX	256
#endif /* !PATH_MAX */

extern char		*__progname;

struct server_pref {
	RB_ENTRY(server_pref)	 entry;
	char			*token;
	unsigned int		 cfg_line;
	char			*hostname;
	char			*service;
	char			*username;
	char			*password;
	char			*default_mount;
};
RB_HEAD(server_pref_tree, server_pref) server_pref_tree_head = RB_INITIALIZER(&server_pref_tree_head);
int	                _cfg_serverpref_cmp(void *, void *);
RB_PROTOTYPE(server_pref_tree, server_pref, entry, _cfg_serverpref_cmp)
RB_GENERATE(server_pref_tree, server_pref, entry, _cfg_serverpref_cmp)

void			_cfg_print_usage(void);
void			_cfg_print_usagehelp(void);
struct server_pref *	_cfg_serverpref_create(const char *, const char *,
					       const char *, const char *,
					       const char *, const char *);
void			_cfg_serverpref_free(struct server_pref **);
int			_cfg_parse_cfgfile(FILE *, const char *);
char *			_cfg_parse_token(const char *);
char *			_cfg_parse_keyvalue(const char *, const char *,
					    const char *, unsigned int);

static const char *optstring = "c:h";

void
_cfg_print_usage(void)
{
	fprintf(stderr, "usage: %s [-h] [-c configfile]\n",
		__progname);
}

void
_cfg_print_usagehelp(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "  -c configfile  use configuration defaults in configfile\n");
	fprintf(stderr, "  -h             display this additional help and exit\n");
	fprintf(stderr, "\n");
}

int
cfg_parse_cmdopts(int *argc, char **argv[])
{
	extern char	*optarg;
	extern int	 optind;
	int		 ch;
	FILE		*cfgfile;
	char		*home;
	char		 cfgfile_name[PATH_MAX];

	cfgfile = NULL;
	cfgfile_name[0] = '\0';

	while ((ch = getopt(*argc, *argv, optstring)) != -1) {
#ifdef S_IROTH
		struct stat	st;
#endif /* S_IROTH */

		switch (ch) {
		case 'c':
			if (cfgfile != NULL) {
				fprintf(stderr, "%s: Error: Multiple -c arguments given\n",
					__progname);
				fclose(cfgfile);
				return (2);
			}
			snprintf(cfgfile_name, sizeof(cfgfile_name), "%s", optarg);
#ifdef S_IROTH
			if (stat(cfgfile_name, &st) != -1 &&
			    (st.st_mode & S_IROTH)) {
				fprintf(stderr, "WARNING: %s is world-readable!\n",
					cfgfile_name);
			}
#endif /* S_IROTH */
			if ((cfgfile = fopen(cfgfile_name, "r")) == NULL) {
				fprintf(stderr, "%s: %s: %s\n", __progname,
					cfgfile_name, strerror(errno));
				return (2);
			}
			break;
		case 'h':
			_cfg_print_usage();
			_cfg_print_usagehelp();
			return (0);
		default:
			_cfg_print_usage();
			return (2);
		}
	}
	*argc -= optind;
	*argv += optind;

	if (cfgfile == NULL && (home = getenv("HOME")) != NULL) {
#ifdef S_IROTH
		struct stat	st;
#endif /* S_IROTH */

		snprintf(cfgfile_name, sizeof(cfgfile_name), "%s/%s", home, CONFIG_FILENAME_HOME);
#ifdef S_IROTH
		if (stat(cfgfile_name, &st) != -1 && (st.st_mode & S_IROTH)) {
			fprintf(stderr, "WARNING: %s is world-readable!\n",
				cfgfile_name);
		}
#endif /* S_IROTH */
		if ((cfgfile = fopen(cfgfile_name, "r")) == NULL && errno != ENOENT)
			fprintf(stderr, "%s: %s: %s\n", __progname,
				cfgfile_name, strerror(errno));
	}
	if (cfgfile == NULL) {
#ifdef S_IROTH
		struct stat	st;
#endif /* S_IROTH */

		snprintf(cfgfile_name, sizeof(cfgfile_name), "%s/%s", SYSCONF_DIR, CONFIG_FILENAME);
#ifdef S_IROTH
		if (stat(cfgfile_name, &st) != -1 && (st.st_mode & S_IROTH)) {
			fprintf(stderr, "WARNING: %s is world-readable!\n",
				cfgfile_name);
		}
#endif /* S_IROTH */
		if ((cfgfile = fopen(cfgfile_name, "r")) == NULL && errno != ENOENT)
			fprintf(stderr, "%s: %s: %s\n", __progname,
				cfgfile_name, strerror(errno));
	}

	if (_cfg_parse_cfgfile(cfgfile, cfgfile_name) != 0) {
		fclose(cfgfile);
		return (1);
	}

	if (cfgfile != NULL)
		fclose(cfgfile);

	return (CONFIG_OKAY);
}

void
cfg_initialize(void)
{
}

void
cfg_shutdown(void)
{
	struct server_pref	*cur, *next;

	for (cur = RB_MIN(server_pref_tree, &server_pref_tree_head);
	     cur != NULL; cur = next) {
		next = RB_NEXT(server_pref_tree, &server_pref_tree_head, cur);
		RB_REMOVE(server_pref_tree, &server_pref_tree_head, cur);
		_cfg_serverpref_free(&cur);
	}
}

void
cfg_print_help(void)
{
	_cfg_print_usage();
	_cfg_print_usagehelp();
}

struct server_pref *
cfg_get_server_pref(const char *token)
{
	struct server_pref	*srvp, find;

	if (token == NULL || token[0] == '\0')
		return (NULL);

	find.token = xstrdup(token);
	srvp = RB_FIND(server_pref_tree, &server_pref_tree_head, &find);
	xfree(find.token);

	return (srvp);
}

const char *
cfg_sp_hostname(struct server_pref *srvp)
{
	return ((const char *)srvp->hostname);
}

const char *
cfg_sp_service(struct server_pref *srvp)
{
	return ((const char *)srvp->service);
}

const char *
cfg_sp_username(struct server_pref *srvp)
{
	return ((const char *)srvp->username);
}

const char *
cfg_sp_password(struct server_pref *srvp)
{
	return ((const char *)srvp->password);
}

const char *
cfg_sp_default_mount(struct server_pref *srvp)
{
	return ((const char *)srvp->default_mount);
}

int
_cfg_serverpref_cmp(void *arg_a, void *arg_b)
{
	struct server_pref	*a = (struct server_pref *)arg_a;
	struct server_pref	*b = (struct server_pref *)arg_b;

	return (strcmp(a->token, b->token));
}

struct server_pref *
_cfg_serverpref_create(const char *token, const char *hostname,
		       const char *service, const char *username,
		       const char *password, const char *default_mount)
{
	struct server_pref      *srvp;

	srvp = xcalloc(1, sizeof(struct server_pref));
	srvp->token = xstrdup(token);
	if (hostname != NULL)
		srvp->hostname = xstrdup(hostname);
	if (service != NULL)
		srvp->service = xstrdup(service);
	if (username != NULL)
		srvp->username = xstrdup(username);
	if (password != NULL)
		srvp->password = xstrdup(password);
	if (default_mount != NULL)
		srvp->default_mount = xstrdup(default_mount);

	return (srvp);
}

void
_cfg_serverpref_free(struct server_pref **srvp_p)
{
	struct server_pref      *srvp = *srvp_p;

	if (srvp->token != NULL)
		xfree(srvp->token);
	if (srvp->hostname != NULL)
		xfree(srvp->hostname);
	if (srvp->service != NULL)
		xfree(srvp->service);
	if (srvp->username != NULL)
		xfree(srvp->username);
	if (srvp->password != NULL)
		xfree(srvp->password);
	if (srvp->default_mount != NULL)
		xfree(srvp->default_mount);
	memset(srvp, 0, sizeof(struct server_pref));
	xfree(*srvp_p);
	*srvp_p = NULL;
}

int
_cfg_parse_cfgfile(FILE *filep, const char *filename)
{
	char		 buf[BUFSIZ];
	char		*bp;
	unsigned int	 line;
	char		*token;
	char		*hostname, *service, *username, *password,
				*default_mount;

	if (filep == NULL)
		return (0);

	line = 0;
	token = NULL;
	hostname = service = username = password = default_mount = NULL;
	while (fgets(buf, sizeof(buf), filep) != NULL) {
		int	cont;

		line++;

		if (strlen(buf) == sizeof(buf) - 1) {
			fprintf(stderr, "%s: %s[%u]: Line too long\n",
				__progname, filename, line);
			return (1);
		}

		/* Trim trailing newlines and carriage returns: */
		buf[strcspn(buf, "\n")] = '\0';
		buf[strcspn(buf, "\r")] = '\0';
		if (buf[0] == '\0')
			continue;

		/* Skip whitespaces at the beginning of a line: */
		for (bp = buf; bp != '\0'; bp++) {
			if (*bp != ' ' && *bp != '\t')
				break;
		}

		/* Skip comments and lines with whitespaces only: */
		if (*bp == '\0' || *bp == '#')
			continue;

		if (*bp == '[') {
			if (token != NULL) {
				/* Add whatever we've got to the tree: */
				struct server_pref *srvp, *exists;

				srvp = _cfg_serverpref_create(token, hostname,
							      service, username,
							      password,
							      default_mount);
				srvp->cfg_line = line;
				exists = RB_INSERT(server_pref_tree, &server_pref_tree_head, srvp);
				xfree(token);
				token = NULL;
				if (hostname != NULL) {
					xfree(hostname);
					hostname = NULL;
				}
				if (service != NULL) {
					xfree(service);
					service = NULL;
				}
				if (username != NULL) {
					xfree(username);
					username = NULL;
				}
				if (password != NULL) {
					xfree(password);
					password = NULL;
				}
				if (default_mount != NULL) {
					xfree(default_mount);
					default_mount = NULL;
				}
				if (exists != NULL) {
					fprintf(stderr, "%s: %s[%u]: Duplicate server token '%s' (already defined in line %u)\n",
						__progname, filename, line,
						exists->token,
						exists->cfg_line);
					return (1);
				}
			}
			if ((token = _cfg_parse_token(bp)) == NULL) {
				fprintf(stderr, "%s: %s[%u]: Invalid server token\n",
					__progname, filename, line);
				return (1);
			}
			continue;
		}

		cont = 1;
		if (token == NULL) {
			fprintf(stderr, "%s: %s[%u]: Missing server token\n",
				__progname, filename, line);
			cont = 0;
		} else if (strncasecmp(bp, CFG_KEY_HOSTNAME,
				       strlen(CFG_KEY_HOSTNAME)) == 0) {
			if (hostname != NULL) {
				fprintf(stderr, "%s: %s[%u]: Duplicate '%s' key in server configuration '%s'\n",
					__progname, filename, line,
					CFG_KEY_HOSTNAME, token);
				cont = 0;
			} else if ((hostname = _cfg_parse_keyvalue(bp, CFG_KEY_HOSTNAME, filename, line))
				   == NULL) {
				cont = 0;
			}
		} else if (strncasecmp(bp, CFG_KEY_SERVICE,
				       strlen(CFG_KEY_SERVICE)) == 0) {
			if (service != NULL) {
				fprintf(stderr, "%s: %s[%u]: Duplicate '%s' key in server configuration '%s'\n",
					__progname, filename, line,
					CFG_KEY_SERVICE, token);
				cont = 0;
			} else if ((service = _cfg_parse_keyvalue(bp, CFG_KEY_SERVICE, filename, line))
				   == NULL) {
				cont = 0;
			}
		} else if (strncasecmp(bp, CFG_KEY_USERNAME,
				       strlen(CFG_KEY_USERNAME)) == 0) {
			if (username != NULL) {
				fprintf(stderr, "%s: %s[%u]: Duplicate '%s' key in server configuration '%s'\n",
					__progname, filename, line,
					CFG_KEY_USERNAME, token);
				cont = 0;
			} else if ((username = _cfg_parse_keyvalue(bp, CFG_KEY_USERNAME, filename, line))
				   == NULL) {
				cont = 0;
			}
		} else if (strncasecmp(bp, CFG_KEY_PASSWORD,
				       strlen(CFG_KEY_PASSWORD)) == 0) {
			if (password != NULL) {
				fprintf(stderr, "%s: %s[%u]: Duplicate '%s' key in server configuration '%s'\n",
					__progname, filename, line,
					CFG_KEY_PASSWORD, token);
				cont = 0;
			} else if ((password = _cfg_parse_keyvalue(bp, CFG_KEY_PASSWORD, filename, line))
				   == NULL) {
				cont = 0;
			}
		} else if (strncasecmp(bp, CFG_KEY_DEFAULT_MOUNT,
				       strlen(CFG_KEY_DEFAULT_MOUNT)) == 0) {
			if (default_mount != NULL) {
				fprintf(stderr, "%s: %s[%u]: Duplicate '%s' key in server configuration '%s'\n",
					__progname, filename, line,
					CFG_KEY_DEFAULT_MOUNT, token);
				cont = 0;
			} else if ((default_mount = _cfg_parse_keyvalue(bp, CFG_KEY_DEFAULT_MOUNT, filename, line))
				   == NULL) {
				cont = 0;
			}
		} else {
			fprintf(stderr, "%s: %s[%u]: Unknown configuration key/value pair '%s'\n",
				__progname, filename, line, bp);
			cont = 0;
		}

		if (!cont) {
			if (token != NULL) {
				xfree(token);
				token = NULL;
			}
			if (hostname != NULL) {
				xfree(hostname);
				hostname = NULL;
			}
			if (service != NULL) {
				xfree(service);
				service = NULL;
			}
			if (username != NULL) {
				xfree(username);
				username = NULL;
			}
			if (password != NULL) {
				xfree(password);
				password = NULL;
			}
			if (default_mount != NULL) {
				xfree(default_mount);
				default_mount = NULL;
			}
			return (1);
		}
	}
	if (ferror(filep)) {
		fprintf(stderr, "%s: %s: %s\n", __progname, filename, strerror(errno));
		return (1);
	}
	if (token != NULL) {
		/* Add the final server preference to the tree: */
		struct server_pref *srvp;

		srvp = _cfg_serverpref_create(token, hostname,
					      service, username,
					      password,
					      default_mount);
		RB_INSERT(server_pref_tree, &server_pref_tree_head, srvp);
		xfree(token);
		token = NULL;
		if (hostname != NULL) {
			xfree(hostname);
			hostname = NULL;
		}
		if (service != NULL) {
			xfree(service);
			service = NULL;
		}
		if (username != NULL) {
			xfree(username);
			username = NULL;
		}
		if (password != NULL) {
			xfree(password);
			password = NULL;
		}
		if (default_mount != NULL) {
			xfree(default_mount);
			default_mount = NULL;
		}
	}

	return (0);
}

char *
_cfg_parse_token(const char *buf)
{
	char		 tbuf[BUFSIZ];
	const char	*bp;
	unsigned int	 i, okay;
	char		*ret;
	size_t		 retsize;

	ret = NULL;

	okay = 0;
	bp = buf + 1;
	for (i = 0; i < (sizeof(tbuf) - 1) && *bp != '\0'; i++, bp++) {
		if (*bp == ']') {
			okay = 1;
			break;
		}
		tbuf[i] = *bp;
	}
	tbuf[i] = '\0';

	if (!okay)
		return (ret);

	retsize = strlen(tbuf) + 1;
	ret = xcalloc(retsize, sizeof(char));
	snprintf(ret, retsize, "%s", tbuf);

	return (ret);
}

char *
_cfg_parse_keyvalue(const char *buf, const char *key, const char *filename,
		    unsigned int line)
{
	char		 tbuf[BUFSIZ];
	const char	*bp;
	unsigned int	 i, okay;
	char		*ret;
	size_t		 retsize;

	ret = NULL;

	bp = buf;
	/* Skip the key, we already have it. */
	bp += strlen(key);
	/* Skip any whitespaces after the key. */
	while (*bp != '\0' && (*bp == ' ' || *bp == '\t'))
		bp++;
	/* An equal sign is now expected. */
	if (*bp++ != '=') {
		fprintf(stderr, "%s: %s[%u]: Malformed ``key = \"value\"'': Bad key or missing '='\n",
			__progname, filename, line);
		return (ret);
	}
	/* Skip any whitespaces after the equal sign. */
	while (*bp != '\0' && (*bp == ' ' || *bp == '\t'))
		bp++;
	/* Double-quotes around the key value are required. */
	if (*bp++ != '"') {
		fprintf(stderr, "%s: %s[%u]: Malformed ``key = \"value\"'': Missing opening quotes\n",
			__progname, filename, line);
		return (ret);
	}

	/* Read value until the second quote. '\\' is the escape character. */
	okay = 0;
	for (i = 0; i < (sizeof(tbuf) - 1) && *bp != '\0'; i++, bp++) {
		if (*bp == '"') {
			okay = 1;
			break;
		}
		if (*bp == '\\')
			bp++;
		tbuf[i] = *bp;
	}
	tbuf[i] = '\0';
	if (!okay) {
		if (i >= sizeof(tbuf) - 1)
			fprintf(stderr, "%s: %s[%u]: Malformed ``key = \"value\"'': Value too long\n",
				__progname, filename, line);
		else
			fprintf(stderr, "%s: %s[%u]: Malformed ``key = \"value\"'': Missing closing quotes\n",
				__progname, filename, line);
		return (ret);
	}

	/* Skip closing quote. */
	bp++;
	/* Whitespaces and comments behind the value are ok, the rest isn't. */
	while (*bp != '\0' && (*bp == ' ' || *bp == '\t'))
		bp++;
	if (*bp != '\0' && *bp != '#')  {
		fprintf(stderr, "%s: %s[%u]: Malformed ``key = \"value\"'': Garbage after \"%s\"\n",
			__progname, filename, line, tbuf);
		return (ret);
	}

	retsize = strlen(tbuf) + 1;
	ret = xcalloc(retsize, sizeof(char));
	snprintf(ret, retsize, "%s", tbuf);

	return (ret);
}
