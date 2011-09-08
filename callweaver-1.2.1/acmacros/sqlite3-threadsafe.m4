dnl -*- autoconf -*-
dnl $id$
dnl Check for libsqlite3, based on version found at libdbi-drivers.sf.net (GPLv2-licensed)

AC_DEFUN([AC_FIND_FILE], [
  $3=no
  for i in $2; do
      for j in $1; do
          if test -r "$i/$j"; then
              $3=$i
              break 2
          fi
      done
  done ])dnl

AC_DEFUN([AC_CHECK_SQLITE3_THREADSAFE], [
  LIBS_OLD=$LIBS
  unset LIBS
  have_sqlite3="no"
  ac_sqlite3="no"
  ac_sqlite3_incdir="no"
  ac_sqlite3_libdir="no"

  # exported variables
  SQLITE3_THREADSAFE_LIBS=""
  SQLITE3_THREADSAFE_CFLAGS=""

  AC_ARG_WITH(sqlite3,
      AC_HELP_STRING( [--with-sqlite3[=dir]] , [Compile with libsqlite3 at given dir, has to be a threadsafe version] ),
      [ ac_sqlite3="$withval" 
        if test "x$withval" != "xno" -a test "x$withval" != "xyes"; then
            ac_sqlite3="yes"
            ac_sqlite3_incdir="$withval"/include
            ac_sqlite3_libdir="$withval"/lib
        fi ],
      [ ac_sqlite3="auto" ] )
  AC_ARG_WITH(sqlite3-incdir,
      AC_HELP_STRING( [--with-sqlite3-incdir],
                      [Specifies where the SQLite3 include files are.] ),
      [  ac_sqlite3_incdir="$withval" ] )
  AC_ARG_WITH(sqlite3-libdir,
      AC_HELP_STRING( [--with-sqlite3-libdir],
                      [Specifies where the SQLite3 libraries are.] ),
      [  ac_sqlite3_libdir="$withval" ] )

  # Try to automagically find SQLite, either with pkg-config, or without.
  if test "x$ac_sqlite3" = "xauto"; then
      if test "x$PKG_CONFIG" != "xno"; then
          AC_MSG_CHECKING([for Threadsafe SQLite3])
	   PKG_CHECK_MODULES([SQLITE3_THREADSAFE],sqlite3 >= 3.2,[AC_DEFINE([HAVE_PKG_SQLITE3],1,[system sqlite3 is there])],[echo "no sqlite3 package found"])
          if test "x$SQLITE3_THREADSAFE_LIBS" = "x" -a "x$SQLITE3_THREADSAFE_CFLAGS" = "x"; then
              AC_CHECK_LIB([sqlite3], [pthread_create], [ac_sqlite3="yes"], [ac_sqlite3="no"])
          else
              AC_CHECK_LIB([sqlite3], [pthread_create], [ac_sqlite3="yes"], [ac_sqlite3="no"],"${SQLITE3_THREADSAFE_LIBS}")
          fi
          AC_MSG_RESULT([$ac_sqlite3])
      else
          AC_CHECK_LIB([sqlite3], [pthread_create], [ac_sqlite3="yes"], [ac_sqlite3="no"])
      fi
  fi

  if test "x$ac_sqlite3" = "xyes"; then
      if test "$ac_sqlite3$_incdir" = "no" || test "$ac_sqlite333_libs" = "no"; then
          sqlite3$_incdirs="/usr/include /usr/local/include /usr/include/sqlite /usr/local/include/sqlite /usr/local/sqlite/include /opt/sqlite/include"
          AC_FIND_FILE(sqlite3.h, $sqlite3$_incdirs, ac_sqlite3_incdir)
          sqlite3_libdirs="/usr/lib /usr/local/lib /usr/lib/sqlite /usr/local/lib/sqlite /usr/local/sqlite/lib /opt/sqlite/lib"
          sqlite3_libs="libsqlite3.so libsqlite3.a"
          AC_FIND_FILE($sqlite3_libs, $sqlite3_libdirs, ac_sqlite3_libdir)
          if test "$ac_sqlite3_incdir" = "no"; then
              AC_MSG_ERROR([Invalid SQLite directory - include files not found.])
          fi
          if test "$ac_sqlite3_libdir" = "no"; then
              AC_MSG_ERROR([Invalid SQLite directory - libraries not found.])
          fi
      fi
      have_sqlite3="yes"

      if test x"$ac_sqlite3_libdir" = xno; then
          test "x$SQLITE3_THREADSAFE_LIBS" = "x" && SQLITE3_THREADSAFE_LIBS="-lsqlite3"
      else
          test "x$SQLITE_THREADSAFE_LIBS" = "x" && SQLITE3_THREADSAFE_LIBS="-L$ac_sqlite3_libdir -lsqlite3"
      fi
      test x"$ac_sqlite3_incdir" = xno && test "x$SQLITE3_THREADSAFE_CFLAGS" = "x" && SQLITE3_THREADSAFE_INCLUDE="-I$ac_sqlite3_incdir"

      AC_SUBST([SQLITE3_THREADSAFE_LIBS])
      AC_SUBST([SQLITE3_THREADSAFE_CFLAGS])
  fi
  LIBS=$OLD_LIBS
])dnl
