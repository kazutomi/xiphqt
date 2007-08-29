#ifndef __CFG_H__
#define __CFG_H__

#ifndef SYSCONF_DIR
# define SYSCONF_DIR		"/etc"
#endif
#ifndef CONFIG_FILENAME
# define CONFIG_FILENAME 	"iceadmin.conf"
#endif
#ifndef CONFIG_FILENAME_HOME
# define CONFIG_FILENAME_HOME	".iceadminrc"
#endif

#define CFG_KEY_HOSTNAME	"hostname"
#define CFG_KEY_SERVICE 	"service"
#define CFG_KEY_USERNAME	"username"
#define CFG_KEY_PASSWORD	"password"
#define CFG_KEY_DEFAULT_MOUNT	"default_mount"

#define CONFIG_OKAY		0x7A69

#ifndef PATH_SEPARATOR
# ifdef WIN32
#  define PATH_SEPARATOR	'\\'
# else
#  define PATH_SEPARATOR	'/'
# endif /* WIN32 */
#endif /* !PATH_SEPARATOR */
#ifndef BUFSIZ
# define BUFSIZ 		1024
#endif /* !BUFSIZ */

typedef struct server_pref *server_pref_t;

void		cfg_initialize(void);
void		cfg_shutdown(void);

int		cfg_parse_cmdopts(int *, char **[]);
void		cfg_print_help(void);

server_pref_t	cfg_get_server_pref(const char *);
const char *	cfg_sp_hostname(server_pref_t);
const char *	cfg_sp_service(server_pref_t);
const char *	cfg_sp_username(server_pref_t);
const char *	cfg_sp_password(server_pref_t);
const char *	cfg_sp_default_mount(server_pref_t);

#endif /* __CFG_H__ */
