dnl Process this file with autoconf to produce a configure script
AC_INIT(theora-exp, [0.1], [theora-dev@xiph.org])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

V_LIB_CURRENT=0
V_LIB_REVISION=1
V_LIB_AGE=0
AC_SUBST(V_LIB_CURRENT)
AC_SUBST(V_LIB_REVISION)
AC_SUBST(V_LIB_AGE)

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CPP
CFLAGS="$cflags_save"

AM_PROG_LIBTOOL

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Check for sdl-config
AC_CHECK_PROG(HAVE_SDL_CONFIG, sdl-config, true, false)
AM_CONDITIONAL(HAVE_SDL_CONFIG,$HAVE_SDL_CONFIG)

dnl check for Ogg
HAVE_OGG=no

dnl first check through pkg-config since it's more flexible

dnl check for pkg-config itself so we don't try the m4 macro without pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(OGG, ogg >= 1.1, HAVE_OGG=yes, HAVE_OGG=no)
fi
if test "x$HAVE_OGG" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_OGG(, AC_MSG_ERROR([
    libogg is required to build this package!
    please see http://www.xiph.org/ for how to
    obtain a copy.
  ]))
  cflags_save=$CFLAGS
  libs_save=$LIBS
  CFLAGS="$CFLAGS $OGG_CFLAGS"
  LIBS="$LIBS $OGG_LIBS"
  AC_CHECK_FUNC(oggpackB_read, , [
    AC_MSG_ERROR([newer libogg version (>1.0) required])
  ])
  CFLAGS=$cflags_save
  LIBS=$libs_save
fi

dnl check for Vorbis
HAVE_VORBIS=no

dnl first check through pkg-config since it's more flexible

if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(VORBIS, vorbis >= 1.0.1, HAVE_VORBIS=yes, HAVE_VORBIS=no)
  dnl also set VORBISENC_LIBS since an examples needs it
  dnl the old .m4 sets this to a value to use on top of VORBIS_LIBS,
  dnl so we do the same here.
  VORBISENC_LIBS="-lvorbisenc"
  AC_SUBST(VORBISENC_LIBS)
fi
if test "x$HAVE_VORBIS" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_VORBIS(HAVE_VORBIS=yes, HAVE_VORBIS=no)
fi

AM_PATH_SDL(,[
	HAVE_SDL=yes
	SDL_LIBS=`$SDL_CONFIG --libs`
],AC_MSG_WARN([*** Unable to find SDL -- Not compiling example players ***]))

AM_PATH_SDL(,[
	HAVE_SDL=yes
	SDL_LIBS=`$SDL_CONFIG --libs`
],AC_MSG_WARN([*** Unable to find SDL -- Not compiling example players ***]))


AC_CHECK_HEADERS([sys/soundcard.h soundcard.h machine/soundcard.h],[
  HAVE_OSS=yes
  break
])
if test x$HAVE_OSS != xyes; then
  AC_MSG_WARN([OSS audio support not found -- not compiling player_example])
fi

if test x$HAVE_SDL = xyes -a x$HAVE_OSS = xyes; then
  BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES player_example"
fi
AC_SUBST(BUILDABLE_EXAMPLES)

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/time.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(gettimeofday select strstr)

dnl Check for optimization
ac_enable_x86asm=yes
AC_ARG_ENABLE(x86asm,
    AS_HELP_STRING([--enable-x86asm],
    [enable x86 assambler optimization, default=autodetect]),
    []
)

if test "$enable_x86asm" != "no"; then
    case "$target_cpu" in
        i[[3456]]86)
            AC_DEFINE([OC_X86ASM], [], [enable x86 assambler optimization])
            AM_CONDITIONAL(OC_X86ASM,true)
            ;;
        x86_64) 
            AC_DEFINE([OC_X86ASM], [], [enable x86 assambler optimization])
            AM_CONDITIONAL(OC_X86ASM,true)
            ;;
        *) 
            AM_CONDITIONAL(OC_X86ASM,false)
            ;;
    esac
else
    AM_CONDITIONAL(OC_X86ASM,false)
    ac_enable_x86asm=no
fi

dnl enable experimental encoder
ac_enable_encode=no
AC_ARG_ENABLE(encode,
     [  --enable-encode        enable encoding support(experimental) ],
     [ ac_enable_encode=$enableval ], [ ac_enable_encode=no] )

if test "x${ac_enable_encode}" != xno ; then
    AC_DEFINE([TH_ENCODER], [], [ enable experimental encoder])
    BUILDABLE_EXAMPLES="$BUILDABLE_EXAMPLES encoder_example"
fi
AM_CONDITIONAL(TH_ENCODER, [test "x${ac_enable_encode}" != xno])


dnl Enable some opzimization for gcc
OPTIMIZATION_CFLAGS="-O3 -fforce-addr -fomit-frame-pointer"
AC_SUBST(OPTIMIZATION_CFLAGS)

AC_OUTPUT([
  Makefile lib/Makefile 
  include/Makefile include/theora/Makefile
  examples/Makefile
  theoradec.pc
  theoraenc.pc
  theoracompat.pc
])

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${datadir}/doc)

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    x86 assambler optimization: ...... ${ac_enable_x86asm}
    Encoding support(experimental): .. ${ac_enable_encode}


  Installation paths:

    libtheora: ....................... ${LIBDIR}
    C header files: .................. ${INCLUDEDIR}/theora
    Documentation: ................... ${DOCDIR}/$PACKAGE

  Building:

    Type 'make' to compile $PACKAGE.

    Type 'make install' to install $PACKAGE.

  Example programs will be built but not installed.
------------------------------------------------------------------------
])

