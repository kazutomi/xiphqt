#ifndef __ERROR_H__
#define __ERROR_H__

#define ERRMSG_MAX	256

void	_set_error(struct iceadmin *, int, int, const char *, ...);
void	_clear_error(struct iceadmin *);
void	_get_error_string(struct iceadmin *, char *, size_t);

#endif /* __ERROR_H__ */
