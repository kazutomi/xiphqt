# configure script for ices
# $Id: configure.ac,v 1.5 2003/03/13 20:51:36 brendan Exp $

m4_define(ICES_MAJOR, 0)
m4_define(ICES_MINOR, 2)
m4_define(ICES_MICRO, 3)
m4_define(ICES_VERSION,
  m4_if(ICES_MICRO, 0, ICES_MAJOR.ICES_MINOR, ICES_MAJOR.ICES_MINOR.ICES_MICRO))

AC_INIT([ices], ICES_VERSION, [icecast@xiph.org])
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([src/ices.c])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile src/playlist/Makefile
  conf/Makefile conf/ices.conf.dist doc/Makefile doc/ices.1])

VERSION=ICES_VERSION

AC_ARG_WITH(docdir,
  [[  --with-docdir=DIR       put documentation in DIR [PREFIX/doc/ices]]],
  [ices_cv_docdir="$with_docdir"],
  [ices_cv_docdir='${prefix}/doc/ices'])

docdir="$ices_cv_docdir"
AC_SUBST(docdir)

AC_ARG_WITH(moddir,
  [[  --with-moddir=DIR       put playlist modules in DIR [SYSCONFDIR/modules]]],
  [ices_cv_moddir="$with_moddir"],
  [ices_cv_moddir='${sysconfdir}/modules'])

moddir="$ices_cv_moddir"
AC_SUBST(moddir)

dnl -- Finally, actual autoconf stuff --

AC_ARG_PROGRAM

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

if test "$GCC" = yes
then
  CFLAGS="$CFLAGS -Wall"
fi

dnl -- System check --

AC_CANONICAL_HOST
AC_AIX
AC_ISC_POSIX
AC_MINIX

IRIX=no
case $host in
  *hpux*)
    CFLAGS="$CFLAGS -D_HPUX_SOURCE -DHPUX"
    if test "$CC" = cc
    then
      CFLAGS="$CFLAGS -Aa"
    fi
  ;;
  *irix*)
    IRIX=yes
  ;;
esac

dnl -- Compiler feature check --

AC_C_CONST
AC_C_BIGENDIAN

dnl -- System library check --

if test "$IRIX" = "yes"
then
  AC_MSG_WARN(Skipping library tests because they confuse Irix)
else
  AC_SEARCH_LIBS(gethostbyname, dns nsl resolv)
fi

dnl -- System header check --

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([errno.h fcntl.h signal.h sys/signal.h sys/socket.h \
  sys/stat.h sys/time.h sys/types.h unistd.h])
AC_HEADER_TIME

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL

dnl -- System function check --

AC_FUNC_STRFTIME
AC_CHECK_FUNCS([vsnprintf setsid setlinebuf])

dnl -- Build system init --

AM_INIT_AUTOMAKE(ices, $VERSION)
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

dnl -- Required extra libraries --

AC_ARG_WITH([shout-config],
  [[  --with-shout-config[=DIR]   use shout-config in PATH to find libshout]])

have_libshout="no"
if test "$with_shout_config" != "no"
then
  if test -n "$with_shout_config" -a "$with_shout_config" != "yes"
  then
    SHOUTCONFIG="$with_shout_config"
  fi
  AC_PATH_PROG(SHOUTCONFIG, [shout-config])

  if test -x "$SHOUTCONFIG"
  then
    LIBS="$LIBS `$SHOUTCONFIG --libs`"
    CFLAGS="$CFLAGS `$SHOUTCONFIG --cflags`"
  elif test -n "$with_shout_config"
  then
    AC_MSG_ERROR([Could not run shout-config])
  fi
fi

AC_CHECK_HEADER([shout/shout.h], [
  AC_DEFINE(HAVE_SHOUT_SHOUT_H, 1, [Define if you have <shout/shout.h>])
  have_libshout="maybe"
  ])
if test "$have_libshout" != "no"
then
  dnl shout_new was introduced in libshout2 so it serves as a version check.
  AC_CHECK_LIB([shout], [shout_new], [
    have_libshout="yes"
    if test -z "$SHOUTCONFIG"
    then
      LIBS="$LIBS -lshout"
    fi
  ],
  [AC_MSG_ERROR([Could not find a usable installed libshout])])
fi

dnl -- Optional features --
AC_CACHE_SAVE

AC_ARG_WITH(xml-config,
  [  --with-xml-config=PATH  use xml-config in PATH to find libxml ])

have_xml="no"
if test "$with_xml_config" != "no"
then
  if test -n "$with_xml_config" -a "$with_xml_config" != "yes"
  then
    XMLCONFIG="$with_xml_config"
    if ! test -x "$XMLCONFIG"
    then
      AC_MSG_ERROR([$with_xml_config cannot be run])
    fi
  else
    AC_CHECK_PROGS(XMLCONFIG, [xml2-config xml-config])
  fi

  if test -n "$XMLCONFIG"
  then
    AC_DEFINE(HAVE_LIBXML, 1, [Define if you have the GNOME XML library])
    LIBS="$LIBS `$XMLCONFIG --libs`"
    CPPFLAGS="$CPPFLAGS `$XMLCONFIG --cflags`"
    have_xml="yes"
  else
    AC_MSG_RESULT([Compiling without libxml support - no configfiles])
  fi
fi
AM_CONDITIONAL(USE_XML, test "$have_xml" = "yes")

AC_ARG_WITH([python],
  [[  --with-python[=PATH]    include python scripting support]],
  [enable_python="$with_python"],
  [enable_python="yes"])

have_python="no"
if test "$enable_python" != "no"
then
  XIPH_PATH_PYTHON([$enable_python])
  if test -n "$PYTHON_LIBS"
  then
    have_python="yes"
    CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
    LDFLAGS="$LDFLAGS $PYTHON_LDFLAGS"
    LIBS="$LIBS $PYTHON_LIBS"
    AC_DEFINE(HAVE_LIBPYTHON, 1, [Define if you have the python library])
  fi
fi
AM_CONDITIONAL(USE_PYTHON, test "$have_python" = "yes")

if test "$with_python" != "no" -a "$have_python" != "yes"
then
  if test -n "$with_python"
  then
    AC_MSG_ERROR([Could not link to python])
  else
    AC_MSG_RESULT([Could not link to python, python scripting disabled])
  fi
fi

AC_ARG_WITH(perl,
  [[  --with-perl[=PATH]      include perl scripting support]])

have_perl="no"
PERL=perl
if test "$with_perl" != "no"
then
  if test -n "$with_perl" -a "$with_perl" != "yes"
  then
    PERL="$with_perl"
    if ! test -x "$PERL"
    then
      AC_MSG_ERROR([Perl not found in the specified location])
    fi
  fi

  PERLCFLAGS=`$PERL -MExtUtils::Embed -e ccopts`
  PERLLIBS=`$PERL -MExtUtils::Embed -e ldopts`

  saved_CPPFLAGS="$CPPFLAGS"
  saved_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS $PERLCFLAGS"
  LIBS="$LIBS $PERLLIBS"

  AC_CHECK_FUNCS(perl_construct)
  if test "$ac_cv_func_perl_construct" != "no"
  then
    have_perl="yes"
    AC_DEFINE(HAVE_LIBPERL, 1, [Define if you have perl libraries])
  else
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"
  fi
fi
AM_CONDITIONAL(USE_PERL, test "$have_perl" = "yes")

if test "$with_perl" != "no" -a "$have_perl" != "yes"
then
  if test -n "$with_perl"
  then
    AC_MSG_ERROR([Could not link to perl])
  else
    AC_MSG_RESULT([Could not link to perl, perl scripting disabled])
  fi
fi

AC_ARG_WITH(lame,
  [[  --with-lame[=DIR]       enable support for reencoding with lame [in DIR]]])

have_LAME="no"
if test "$with_lame" != "no"
then
  if test -n "$with_lame" -a "$with_lame" != "yes"
  then
    CPPFLAGS="$CPPFLAGS -I$with_lame/include"
    LDFLAGS="$LDFLAGS -L$with_lame/lib"
  fi

  AC_CHECK_HEADER(lame/lame.h, [
    have_LAME="maybe",
    AC_DEFINE(HAVE_LAME_LAME_H, 1, [Use <lame/lame.h> instead of <lame.h>])
    ], [
    AC_CHECK_HEADER(lame.h, [
    have_LAME="maybe"])])

  if test "$have_LAME" != "no"
  then
    AC_CHECK_LIB(mp3lame, lame_encode_flush_nogap, [
      have_LAME="yes"
      LIBS="$LIBS -lmp3lame"
      LIBM="-lm"
      AC_DEFINE(HAVE_LIBLAME, 1, [Define if you have the LAME MP3 library])
      
      AC_CHECK_FUNCS([lame_decode_exit])
      ], [have_LAME="no"], -lm)
  fi
fi
AM_CONDITIONAL(USE_LAME, test "$have_LAME" = "yes")

if test "$with_lame" != "no" -a "$have_LAME" != "yes"
then
  if test -n "$with_lame"
  then
    AC_MSG_ERROR([Could not find a valid LAME library installation])
  else
    AC_MSG_RESULT([Could not find a valid LAME library, reencoding disabled])
  fi
fi

AC_ARG_WITH(vorbis,
  [[  --with-vorbis[=DIR]     support for vorbis using libvorbisfile [in DIR]]])

if test "$have_LAME" != "yes"
then
  if test -n "$with_vorbis" -a "$with_vorbis" != "no"
  then
    AC_MSG_ERROR([Vorbis cannot be enabled without LAME])
  elif test "$with_vorbis" != "no"
  then
    AC_MSG_RESULT([Vorbis is disabled because LAME is not enabled])
    with_vorbis="no"
  fi
fi

have_vorbis="no"
if test "$with_vorbis" != "no"
then
  if test -n "$with_vorbis" -a "$with_vorbis" != "yes"
  then
    CPPFLAGS="$CPPFLAGS -I$with_vorbis/include"
    LDFLAGS="$LDFLAGS -L$with_vorbis/lib"
  fi

  AC_CHECK_HEADER(vorbis/vorbisfile.h, have_vorbis="maybe")
  if test "$have_vorbis" != "no"
  then
    AC_CHECK_LIB(vorbisfile, ov_open, [
      LIBS="$LIBS -lvorbisfile -lvorbis -logg"
      AC_DEFINE(HAVE_LIBVORBISFILE, 1, [Define if you have libvorbisfile])
      have_vorbis="yes"
    ],[have_vorbis="no"],-lvorbis -logg)
  fi
fi
AM_CONDITIONAL(USE_VORBIS, test "$have_vorbis" = "yes")

if test "$with_vorbis" != "no" -a "$have_vorbis" != "yes"
then
  if test -n "$with_vorbis"
  then
    AC_MSG_ERROR([Could not find libvorbisfile])
  else
    AC_MSG_RESULT([Could not find libvorbisfile, vorbis support disabled])
  fi
fi

dnl -- and finish up --

LIBS="$LIBS $LIBM $LIBDL"

AC_OUTPUT

AC_MSG_RESULT([Compiling with: $CPPFLAGS $CFLAGS])
AC_MSG_RESULT([Linking with: $LDFLAGS $LIBS])
AC_MSG_RESULT([Features:])
AC_MSG_RESULT([  LAME    : $have_LAME])
AC_MSG_RESULT([  Vorbis  : $have_vorbis])
AC_MSG_RESULT([  Perl    : $have_perl])
AC_MSG_RESULT([  Python  : $have_python])
AC_MSG_RESULT([  XML     : $have_xml])
