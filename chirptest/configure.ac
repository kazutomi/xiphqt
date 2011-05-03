dnl Process this file with autoconf to produce a configure script. -*-m4-*-

cflags_save="$CFLAGS"
AC_PREREQ(2.57)
AC_INIT(chirptest.c)
AM_INIT_AUTOMAKE(chirp, 20110503, [ghost-dev@xiph.org])
AC_CONFIG_FILES([Makefile])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [sincos])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([cairo], [cairo_create])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

if test -z "$GCC"; then
        case $host in 
        *-*-irix*)
                DEBUG="-g -signed" 
                CFLAGS="-O2 -w -signed"
                PROFILE="-p -g3 -O2 -signed"
                ;;
        sparc-sun-solaris*)
                DEBUG="-v -g"
                CFLAGS="-xO4 -fast -w -fsimple -native -xcg92"
                PROFILE="-v -xpg -g -xO4 -fast -native -fsimple -xcg92 -Dsuncc"
                ;;
        *)
                DEBUG="-g"
                CFLAGS="-O"
                PROFILE="-g -p" 
                ;;
        esac
else
        case $host in 
        *-*-linux*)
                DEBUG="-g -Wall -fsigned-char"
                CFLAGS="-O2 -fsigned-char -ffast-math"
                PROFILE="-Wall -W -pg -g -O2 -fsigned-char -ffast-math"
                ;;
        sparc-sun-*)
                DEBUG="-g -Wall -fsigned-char"
                CFLAGS="-O2 -fsigned-char"
                PROFILE="-pg -g -O2 -fsigned-char" 
                ;;
        *-*-darwin*)
                DEBUG="-fno-common -g -Wall -fsigned-char"
                CFLAGS="-fno-common -O2 -Wall -fsigned-char -ffast-math"
                PROFILE="-fno-common -O2 -Wall -pg -g -fsigned-char -ffast-math"
                ;;
        *)
                DEBUG="-g -Wall -fsigned-char"
                CFLAGS="-O2 -fsigned-char -ffast-math"
                PROFILE="-O2 -g -pg -fsigned-char -ffast-math" 
                ;;
        esac
fi

CFLAGS="$CFLAGS -DVERSION='\"$VERSION\"' $COMMON_FLAGS"
DEBUG="$DEBUG -DVERSION='\\\"$VERSION\\\"' $COMMON_FLAGS"
PROFILE="$PROFILE -DVERSION='\\\"$VERSION\\\"' $COMMON_FLAGS"
LIBS="$LIBS"

AC_SUBST(DEBUG)
AC_SUBST(PROFILE)
AC_OUTPUT

echo "Type \"make\" to compile";
