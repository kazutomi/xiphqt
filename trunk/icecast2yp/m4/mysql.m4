# =========================================================================
# AM_PATH_MYSQL : MySQL library

dnl AM_PATH_MYSQL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for MYSQL, and define MYSQL_CFLAGS and MYSQL_LIBS
dnl
AC_DEFUN(AM_PATH_MYSQL,
[dnl
dnl Get the cflags and libraries from the mysql_config script
dnl
AC_ARG_WITH(mysql-prefix,[  --with-mysql-prefix=PFX   Prefix where MYSQL is installed (optional)],
            mysql_prefix="$withval", mysql_prefix="")
AC_ARG_WITH(mysql-exec-prefix,[  --with-mysql-exec-prefix=PFX Exec prefix where MYSQL is installed (optional)],
            mysql_exec_prefix="$withval", mysql_exec_prefix="")

  if test x$mysql_exec_prefix != x ; then
     mysql_args="$mysql_args --exec-prefix=$mysql_exec_prefix"
     if test x${MYSQL_CONFIG+set} != xset ; then
        MYSQL_CONFIG=$mysql_exec_prefix/bin/mysql_config
     fi
  fi
  if test x$mysql_prefix != x ; then
     mysql_args="$mysql_args --prefix=$mysql_prefix"
     if test x${MYSQL_CONFIG+set} != xset ; then
        MYSQL_CONFIG=$mysql_prefix/bin/mysql_config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no)
  AC_MSG_CHECKING(for MYSQL)
  no_mysql=""
  if test "$MYSQL_CONFIG" = "no" ; then
    MYSQL_CFLAGS=""
    MYSQL_LIBS=""
    AC_MSG_RESULT(no)
     ifelse([$2], , :, [$2])
  else
    MYSQL_CFLAGS=`$MYSQL_CONFIG $mysqlconf_args --cflags | sed -e "s/'//g"`
    MYSQL_LIBS=`$MYSQL_CONFIG $mysqlconf_args --libs | sed -e "s/'//g"`
    AC_MSG_RESULT(yes)
    ifelse([$1], , :, [$1])
  fi
  AC_SUBST(MYSQL_CFLAGS)
  AC_SUBST(MYSQL_LIBS)
])
