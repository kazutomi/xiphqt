/* Some systems don't have assert.h */
#undef HAVE_ASSERT_H

/* We might be the silly hpux */
#undef hpux

/* Are we sysv? */
#undef SYSV

/* Fucked up IRIX */
#undef IRIX

/* Or svr4 perhaps? */
#undef SVR4

/* Some kind of Linux */
#undef LINUX

/* Or perhaps some bsd variant? */
#undef __SOMEBSD__

/* UNIX98 and others want socklen_t */
#undef HAVE_SOCKLEN_T

/* The complete version of shout */
#undef VERSION

/* Definately Solaris */
#undef SOLARIS

/* directories that we use... blah blah blah */
#undef ICES_ETCDIR
#undef ICES_LOGDIR
#undef ICES_MODULEDIR

/* What the hell is this? */
#undef PACKAGE

/* DAMN I HATE HATE HATE AUTOCONF */
#undef HAVE_SOCKET
#undef HAVE_CONNECT
#undef HAVE_GETHOSTBYNAME
#undef HAVE_LIBPYTHON
#undef HAVE_LIBXML
