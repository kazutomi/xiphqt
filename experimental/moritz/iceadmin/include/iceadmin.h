#ifndef __ICEADMIN_H__
#define __ICEADMIN_H__

/* The function completed successfully: */
#define ICEADMIN_SUCCESS	0
/* A request succeeded, but the command could not be fulfilled: */
#define ICEADMIN_FAILURE	1
/* An error occured: */
#define ICEADMIN_ERROR		2
/* Libiceadmin is being abused with bad function arguments, e.g. NULL where
 * not permitted: */
#define ICEADMIN_ABUSE		3
/* Libiceadmin is insane and something unexpected happened. This is a bug: */
#define ICEADMIN_INSANE 	4
/* Stale handle that wasn't initialized properly: */
#define ICEADMIN_BADHANDLE	5

typedef struct iceadmin 	*iceadmin_t;

#if defined(__cplusplus) && (!defined(__BEGIN_DECLS) || !defined(__END_DECLS))
# undef __BEGIN_DECLS
# undef __END_DECLS
# define __BEGIN_DECLS		extern "C" {
# define __END_DECLS		}
#endif /* !__BEGIN_DECLS || !_END_DECLS */

__BEGIN_DECLS
void	iceadmin_initialize(void);
void	iceadmin_shutdown(void);

void	iceadmin_get_error(iceadmin_t /* handle */, char * /* buffer */,
			   size_t /* buffer size */);

/* Set the user-agent string to "name/version" or NULL for the default. */
void	iceadmin_set_useragent(const char * /* string */);

int	iceadmin_new(iceadmin_t * /* ptr to new handle */,
		     const char * /* mountpoint, may be NULL for certain *
				   * _get_/_list_ functions */,
		     const char * /* hostname */,
		     const char * /* service/port */,
		     const char * /* username */,
		     const char * /* password */);
#define iceadmin_delete(i)						\
	iceadmin_delete_c(&(i))
int	iceadmin_delete_c(iceadmin_t * /* handle ptr */);
#define iceadmin_free(p)						\
	iceadmin_free_c((void *)&(p))
int	iceadmin_free_c(void ** /* buffer allocated by iceadmin */);

/*
 * Commands that work without a mountpoint.
 * A buffer is allocated and its size stored. The buffer needs to be freed by
 * the caller with iceadmin_free().
 */
int	iceadmin_cmd_get_stats(iceadmin_t /* handle */,
			       char ** /* buffer */,
			       size_t * /* buffer size */);
int	iceadmin_cmd_get_xmlstats(iceadmin_t /* handle */,
				  char ** /* buffer */,
				  size_t * /* buffer size */);
int	iceadmin_cmd_list_mounts(iceadmin_t /* handle */,
				 char ** /* buffer */,
				 size_t * /* buffer size */);
int	iceadmin_cmd_list_xmlmounts(iceadmin_t /* handle */,
				    char ** /* buffer */,
				    size_t * /* buffer size */);

/*
 * Commands that require a mountpoint.
 * A buffer is allocated and its size stored. The buffer needs to be freed by
 * the caller with iceadmin_free().
 */
int	iceadmin_cmd_list_clients(iceadmin_t /* handle */,
				  char ** /* buffer */,
				  size_t * /* buffer size */);
int	iceadmin_cmd_list_xmlclients(iceadmin_t /* handle */,
				     char ** /* buffer */,
				     size_t * /* buffer size */);
int	iceadmin_cmd_auth_list_users(iceadmin_t /* handle */,
				     char ** /* buffer */,
				     size_t * /* buffer size */);
int	iceadmin_cmd_auth_list_xmlusers(iceadmin_t /* handle */,
					char ** /* buffer */,
					size_t * /* buffer size */);

/*
 * Miscellaneous commands that work on a mountpoint.
 */
int	iceadmin_cmd_kill_source(iceadmin_t /* handle */);
int	iceadmin_cmd_kill_client(iceadmin_t /* handle */,
				 unsigned int /* client ID */);
int	iceadmin_cmd_move_clients(iceadmin_t /* handle */,
				  const char * /* destination */);
int	iceadmin_cmd_update_fallback(iceadmin_t /* handle */,
				     const char * /* mountpoint */);
int	iceadmin_cmd_update_metadata(iceadmin_t /* handle */,
				     const char * /* metadata string */);
int	iceadmin_cmd_update_artist_title(iceadmin_t /* handle */,
					 const char * /* artist */,
					 const char * /* title */);
int	iceadmin_cmd_auth_adduser(iceadmin_t /* handle */,
				  const char * /* username */,
				  const char * /* password */);
int	iceadmin_cmd_auth_deluser(iceadmin_t /* handle */,
				  const char * /* username */);
__END_DECLS

#endif /* __ICEADMIN_H__ */
