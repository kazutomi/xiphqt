dnl Process this file with autoconf to produce a configure script

dnl ------------------------------------------------
dnl Initialization and Versioning
dnl ------------------------------------------------

AC_INIT(theora-tools,[unreleased-snapshot])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

dnl no shared linking for us
AM_DISABLE_SHARED
AM_ENABLE_STATIC
AM_PROG_LIBTOOL

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

AC_CANONICAL_HOST

dnl Set some target options

cflags_save="$CFLAGS"
ldflags_save="$LDFLAGS"
if test -z "$GCC"; then
        case $host in 
        *)
                DEBUG="-g"
                CFLAGS="-O"
                PROFILE="-g -p" ;;
        esac
else

        case $host in 
        *)
                DEBUG="-g -W -D__NO_MATH_INLINES"
                CFLAGS="-O2 -Wall"
                PROFILE="-W -pg -g -O2 -fno-inline-functions";;
        esac
fi
CFLAGS="$CFLAGS $cflags_save"
LDFLAGS="$LDFLAGS $ldflags_save"

dnl --------------------------------------------------
dnl Check for headers
dnl --------------------------------------------------

dnl none here

dnl --------------------------------------------------
dnl Check for typedefs, structures, etc
dnl --------------------------------------------------

dnl none

dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

dnl substitute the included getopt if the system doesn't support long options
AC_CHECK_FUNC(getopt_long,
              [GETOPT_OBJS=''],
              [GETOPT_OBJS='getopt.$(OBJEXT) getopt1.$(OBJEXT)'])
AC_SUBST(GETOPT_OBJS)

AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
PKG_CHECK_MODULES(XIPH,ogg >= 1.1 vorbis theoraenc >= 1.1 theoradec >= 1.1)
CFLAGS="$CFLGS $XIPH_CFLAGS"
LIBS="$LIBS $XIPH_LIBS"

PKG_CHECK_MODULES(VORBISENC, vorbisenc)
AC_SUBST(VORBISENC_CFLAGS)
AC_SUBST(VORBISENC_LIBS)

dnl check for libpng
HAVE_PNG=no
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(PNG, libpng, HAVE_PNG=yes, HAVE_PNG=no)
fi
AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)

if test x$HAVE_PNG = xyes; then
  PNG2THEORA="png2theora"
  AC_SUBST(PNG2THEORA)
fi

dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

LIBS="$LIBS"

AC_SUBST(LIBS)
AC_SUBST(DEBUG)
AC_SUBST(PROFILE)

AC_CONFIG_FILES([
  Makefile
  theoraenc/Makefile
  theoracomment/Makefile
  png2theora/Makefile
  vp32theora/Makefile
])
AC_OUTPUT
