#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(src/vorbose.c)
AM_INIT_AUTOMAKE(vorbose, 0.1, [vorbis-dev@xiph.org])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile src/Makefile])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AC_CHECK_LIB(ogg2, ogg2pack_eop, [], [
  AC_MSG_ERROR([$PACKAGE requires libogg2])
])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADERS([ogg2/ogg.h])

dnl we cheat with the vorbis headers, using our own copies since we
dnl access internals

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])
AC_OUTPUT
