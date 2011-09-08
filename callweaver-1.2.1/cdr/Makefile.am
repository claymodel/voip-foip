AUTOMAKE_OPTS	= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES	= cdr_csv.la cdr_custom.la

cdr_csv_la_SOURCES	= cdr_csv.c
cdr_csv_la_LDFLAGS 	= -module -avoid-version -no-undefined
cdr_csv_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la

cdr_custom_la_SOURCES	= cdr_custom.c
cdr_custom_la_LDFLAGS	= -module -avoid-version -no-undefined
cdr_custom_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la

if WANT_CDR_MYSQL
cwmod_LTLIBRARIES	+= cdr_mysql.la
cdr_mysql_la_SOURCES	= cdr_mysql.c
cdr_mysql_la_CFLAGS	= $(AM_CFLAGS) @MYSQL_CFLAGS@ 
cdr_mysql_la_LDFLAGS	= -module -avoid-version -no-undefined
cdr_mysql_la_LIBADD	= @MYSQL_LIBS@ ${top_builddir}/corelib/libcallweaver.la
endif WANT_CDR_MYSQL

if WANT_CDR_PGSQL
cwmod_LTLIBRARIES	+= cdr_pgsql.la
cdr_pgsql_la_SOURCES	= cdr_pgsql.c
cdr_pgsql_la_CFLAGS	= $(AM_CFLAGS) @PQINCPATH@ 
cdr_pgsql_la_LDFLAGS	= -module -avoid-version -no-undefined
cdr_pgsql_la_LIBADD	= @PQLIBPATH@ -lpq ${top_builddir}/corelib/libcallweaver.la
endif WANT_CDR_PGSQL

if WANT_CDR_PGSQL_CUSTOM
cwmod_LTLIBRARIES     += cdr_pgsql_custom.la
cdr_pgsql_custom_la_SOURCES    = cdr_pgsql_custom.c
cdr_pgsql_custom_la_CFLAGS     = $(AM_CFLAGS) @PQINCPATH@
cdr_pgsql_custom_la_LDFLAGS    = -module -avoid-version -no-undefined
cdr_pgsql_custom_la_LIBADD     = @PQLIBPATH@ -lpq ${top_builddir}/corelib/libcallweaver.la
endif WANT_CDR_PGSQL_CUSTOM

if WANT_CDR_MANAGER
cwmod_LTLIBRARIES     += cdr_manager.la
cdr_manager_la_SOURCES    = cdr_manager.c
cdr_manager_la_CFLAGS     = $(AM_CFLAGS) @PQINCPATH@
cdr_manager_la_LDFLAGS    = -module -avoid-version -no-undefined
cdr_manager_la_LIBADD     = ${top_builddir}/corelib/libcallweaver.la
endif WANT_CDR_MANAGER

cwmod_LTLIBRARIES	+= cdr_sqlite3.la
cdr_sqlite3_la_SOURCES	= cdr_sqlite3.c
cdr_sqlite3_la_LDFLAGS	= -module -avoid-version -no-undefined
cdr_sqlite3_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
cdr_sqlite3_la_LIBADD	+= @SQLITE3_THREADSAFE_LIBS@
cdr_sqlite3_la_CFLAGS	= @SQLITE3_THREADSAFE_CFLAGS@ 

if WANT_CDR_SQLITE3_CUSTOM
cwmod_LTLIBRARIES	+= cdr_sqlite3_custom.la
cdr_sqlite3_custom_la_SOURCES	= cdr_sqlite3_custom.c
cdr_sqlite3_custom_la_LDFLAGS	= -module -avoid-version -no-undefined
cdr_sqlite3_custom_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
cdr_sqlite3_custom_la_LIBADD	+= @SQLITE3_THREADSAFE_LIBS@
cdr_sqlite3_custom_la_CFLAGS	= @SQLITE3_THREADSAFE_CFLAGS@ 
endif WANT_CDR_SQLITE3_CUSTOM

#if WANT_CDR_ODBC
#cwmod_LTLIBRARIES	+= cdr_odbc.la
#cdr_odbc_la_SOURCES	= cdr_odbc.c
#cdr_odbc_la_CFLAGS	= $(AM_CFLAGS) @ODBC_CFLAGS@
#cdr_odbc_la_LDFLAGS	= -module -avoid-version -no-undefined
#cdr_odbc_la_LIBADD	= @ODBC_LIBS@ ${top_builddir}/corelib/libcallweaver.la
#endif WANT_CDR_ODBC



INCLUDES = -I$(top_srcdir)/include
