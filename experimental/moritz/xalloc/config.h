#if !defined(WIN32)

# define HAVE_SYS_TIME_H 	1
# define HAVE_SYS_TREE_H	1
# define HAVE_SYS_TYPES_H	1
# define HAVE_SYS_WAIT_H 	1
# define HAVE_GETTIMEOFDAY	1
# define HAVE_SETPROCTITLE	1
# define HAVE_SIGNAL_H		1
# define HAVE_STDINT_H		1
# define HAVE_UNISTD_H		1

#else

# define HAVE_SYS_TIMEB_H	1
# define HAVE_FTIME		1
# define HAVE_SIGNAL_H		1
# define HAVE_WINDOWS_H 	1

#endif /* !WIN32 */
