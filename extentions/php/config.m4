dnl $Id$
dnl config.m4 for extension tdtinyurl

PHP_ARG_WITH(tduqid, include tduqid library,
Make sure that the comment is aligned:
[  --with-tduqid      Include tduqid library])

PHP_ARG_WITH(hiredis, include tduqid library,
Make sure that the comment is aligned:
[  --with-hiredis   Include hiredis library])

PHP_ARG_WITH(sqlite3, include tduqid library,
Make sure that the comment is aligned:
[  --with-sqlite3	Include sqlite3 library])


# tduqid
if test "$PHP_tduqid" != "no"; then
  SEARCH_PATH="$PHP_tduqid /usr/local /usr"
  SEARCH_FOR="/include/tduqid.h"
  AC_MSG_CHECKING([for tduqid files in default path])
  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR; then
      TDUQID_DIR=$i
      AC_MSG_RESULT(found in $i)
    fi
  done
  
  if test -z "$TDUQID_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the tduqid distribution])
  fi


  LIBNAME=tduqid
  LIBSYMBOL=tduqid_get_identity_grp_no

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY(tduqid,, TDTINYURL_SHARED_LIBADD)
        AC_DEFINE(HAVE_TDUQIDLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong tduqid lib version or lib not found])
  ],[
    -L$TDUQID_DIR/lib/  -ltduqid
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

# sqlite3
if test "$PHP_SQLITE3" != "no"; then
  SEARCH_PATH="$PHP_SQLITE3 /usr/local /usr"
  SEARCH_FOR="/include/sqlite3.h"
  AC_MSG_CHECKING([for sqlite3 files in default path])
  for i in $SEARCH_PATH ; do
    if test -r $i/$SEARCH_FOR; then
      SQLITE3_DIR=$i
      AC_MSG_RESULT(found in $i)
    fi
  done
  
  if test -z "$SQLITE3_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the sqlite3 distribution])
  fi


  LIBNAME=sqlite3
  LIBSYMBOL=sqlite3_exec

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY(sqlite3,, TDTINYURL_SHARED_LIBADD)
        AC_DEFINE(HAVE_SQLITE3LIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong sqlite3 lib version or lib not found])
  ],[
    -L$SQLITE3_DIR/lib/ -lsqlite3
  ])

fi


PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $HIREDIS_DIR/lib/, TDTINYURL_SHARED_LIBADD)
PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SQLITE3_DIR/lib/, TDTINYURL_SHARED_LIBADD)
PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $TDUQID_DIR/lib/, TDTINYURL_SHARED_LIBADD)
PHP_ADD_INCLUDE($HIREDIS_DIR/include)
PHP_ADD_INCLUDE($SQLITE3_DIR/include)
PHP_ADD_INCLUDE($TDUQID_DIR/include)

PHP_SUBST(TDTINYURL_SHARED_LIBADD)
PHP_NEW_EXTENSION(tdtinyurl, tdtinyurl.c, $ext_shared)
