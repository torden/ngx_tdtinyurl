dnl $Id$
dnl config.m4 for extension tdtinyurl

PHP_ARG_WITH(uqid, include uqid library,
Make sure that the comment is aligned:
[  --with-uqid      Include uqid library])

PHP_ARG_WITH(hiredis, include uqid library,
Make sure that the comment is aligned:
[  --with-hiredis   Include hiredis library])

# UQID
if test "$PHP_UQID" != "no"; then
  SEARCH_PATH="$PHP_UQID /usr/local /usr"
  SEARCH_FOR="/include/uqid.h"
  AC_MSG_CHECKING([for uqid files in default path])
  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR; then
      UQID_DIR=$i
      AC_MSG_RESULT(found in $i)
    fi
  done
  
  if test -z "$UQID_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the uqid distribution])
  fi


  LIBNAME=uqid
  LIBSYMBOL=uqid_get_identity_grp_no

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY(uqid,, TDTINYURL_SHARED_LIBADD)
        AC_DEFINE(HAVE_UQIDLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong uqid lib version or lib not found])
  ],[
    -L$UQID_DIR/lib64/  -luqid
  ])
  
fi

# HIREDIS
if test "$PHP_HIREDIS" != "no"; then
  SEARCH_PATH="$PHP_HIREDIS /usr/local /usr"
  SEARCH_FOR="/include/hiredis/hiredis.h"
  AC_MSG_CHECKING([for hiredis files in default path])
  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR; then
      HIREDIS_DIR=$i
      AC_MSG_RESULT(found in $i)
    fi
  done
  
  if test -z "$HIREDIS_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the hiredis distribution])
  fi


  LIBNAME=hiredis
  LIBSYMBOL=redisConnectUnixNonBlock

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY(hiredis,, TDTINYURL_SHARED_LIBADD)
        AC_DEFINE(HAVE_HIREDISLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong hiredis lib version or lib not found])
  ],[
    -L$HIREDIS_DIR/lib/ -lhiredis
  ])

fi

PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $UQID_DIR/lib/, TDTINYURL_SHARED_LIBADD)
PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $HIREDIS_DIR/lib/, TDTINYURL_SHARED_LIBADD)
PHP_ADD_INCLUDE($HIREDIS_DIR/include)
PHP_ADD_INCLUDE($UQID_DIR/include)

PHP_SUBST(TDTINYURL_SHARED_LIBADD)
PHP_NEW_EXTENSION(tdtinyurl, tdtinyurl.c, $ext_shared)
