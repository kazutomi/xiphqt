#ifndef __CFG_H__
#define __CFG_H__

#define DEFAULT_CONFIG_FILE	".iceadminrc"

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

void	cfg_initialize(void);
void	cfg_shutdown(void);

int	cfg_parse_cmdline(int, char *[]);
void	cfg_print_help(void);

#endif /* __CFG_H__ */
