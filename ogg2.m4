# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl XIPH_PATH_OGG2([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG2_CFLAGS and OGG2_LIBS
dnl
AC_DEFUN(XIPH_PATH_OGG2,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg2,[  --with-ogg2=PFX   Prefix where libogg2 is installed (optional)], ogg2_prefix="$withval", ogg2_prefix="")
AC_ARG_WITH(ogg2-libraries,[  --with-ogg2-libraries=DIR   Directory where libogg2 library is installed (optional)], ogg2_libraries="$withval", ogg2_libraries="")
AC_ARG_WITH(ogg2-includes,[  --with-ogg2-includes=DIR   Directory where libogg2 header files are installed (optional)], ogg2_includes="$withval", ogg2_includes="")
AC_ARG_ENABLE(ogg2test, [  --disable-ogg2test       Do not try to compile and run a test Ogg program],, enable_ogg2test=yes)

  if test "x$ogg2_libraries" != "x" ; then
    OGG2_LIBS="-L$ogg2_libraries"
  elif test "x$ogg2_prefix" != "x" ; then
    OGG2_LIBS="-L$ogg2_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    OGG2_LIBS="-L$prefix/lib"
  fi

  OGG2_LIBS="$OGG2_LIBS -logg2"

  if test "x$ogg2_includes" != "x" ; then
    OGG_CFLAGS="-I$ogg2_includes"
  elif test "x$ogg_prefix" != "x" ; then
    OGG2_CFLAGS="-I$ogg2_prefix/include"
  elif test "$prefix" != "xNONE"; then
    OGG2_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for Ogg2)
  no_ogg2=""


  if test "x$enable_ogg2test" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG2_CFLAGS"
    LIBS="$LIBS $OGG2_LIBS"
dnl
dnl Now check if the installed Ogg2 is sufficiently new.
dnl
      rm -f conf.ogg2test
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg2.h>

int main ()
{
  system("touch conf.ogg2test");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg2" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.ogg2test ; then
       :
     else
       echo "*** Could not run Ogg2 test program, checking why..."
       CFLAGS="$CFLAGS $OGG2_CFLAGS"
       LIBS="$LIBS $OGG2_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg2.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg2 or finding the wrong"
       echo "*** version of Ogg2. If it is not finding Ogg2, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg2 was incorrectly installed"
       echo "*** or that you have moved Ogg2 since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG2_CFLAGS=""
     OGG2_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG2_CFLAGS)
  AC_SUBST(OGG2_LIBS)
  rm -f conf.ogg2test
])
