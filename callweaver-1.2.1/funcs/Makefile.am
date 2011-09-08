AUTOMAKE_OPTS = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES		=

#pbx_functions_la_SOURCES	= pbx_functions.c pbx_functions.h
#pbx_functions_la_LDFLAGS             = -module -avoid-version -no-undefined
#pbx_functions_la_CFLAGS              = $(AM_CFLAGS)
#pbx_functions_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la

if WANT_FUNC_CALLERID
cwmod_LTLIBRARIES		+= func_callerid.la
func_callerid_la_SOURCES	= func_callerid.c
func_callerid_la_LDFLAGS	= -module -avoid-version -no-undefined
func_callerid_la_CFLAGS		= $(AM_CFLAGS)
func_callerid_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_CALLERID

if WANT_FUNC_CDR
cwmod_LTLIBRARIES		+= func_cdr.la
func_cdr_la_SOURCES		= func_cdr.c
func_cdr_la_LDFLAGS		= -module -avoid-version -no-undefined
func_cdr_la_CFLAGS		= $(AM_CFLAGS)
func_cdr_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_CDR

if WANT_FUNC_CONFIG
cwmod_LTLIBRARIES		+= func_config.la
func_config_la_SOURCES		= func_config.c
func_config_la_LDFLAGS		= -module -avoid-version -no-undefined
func_config_la_CFLAGS		= $(AM_CFLAGS)
func_config_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_CONFIG

if WANT_FUNC_DB
cwmod_LTLIBRARIES		+= func_db.la
func_db_la_SOURCES		= func_db.c
func_db_la_LDFLAGS		= -module -avoid-version -no-undefined
func_db_la_CFLAGS		= $(AM_CFLAGS)
func_db_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_DB

if WANT_FUNC_ENUM
cwmod_LTLIBRARIES		+= func_enum.la
func_enum_la_SOURCES		= func_enum.c
func_enum_la_LDFLAGS             = -module -avoid-version -no-undefined
func_enum_la_CFLAGS              = $(AM_CFLAGS)
func_enum_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_ENUM

if WANT_FUNC_ENV
cwmod_LTLIBRARIES		+= func_env.la
func_env_la_SOURCES		= func_env.c
func_env_la_LDFLAGS		= -module -avoid-version -no-undefined
func_env_la_CFLAGS		= $(AM_CFLAGS)
func_env_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_ENV

if WANT_FUNC_FILEEXISTS
cwmod_LTLIBRARIES		+= func_fileexists.la
func_fileexists_la_SOURCES	= func_fileexists.c
func_fileexists_la_LDFLAGS	= -module -avoid-version -no-undefined
func_fileexists_la_CFLAGS	= $(AM_CFLAGS)
func_fileexists_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_FILEEXISTS

if WANT_FUNC_GROUPCOUNT
cwmod_LTLIBRARIES		+= func_groupcount.la
func_groupcount_la_SOURCES	= func_groupcount.c
func_groupcount_la_LDFLAGS	= -module -avoid-version -no-undefined
func_groupcount_la_CFLAGS	= $(AM_CFLAGS)
func_groupcount_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_GROUPCOUNT

if WANT_FUNC_LANGUAGE
cwmod_LTLIBRARIES		+= func_language.la
func_language_la_SOURCES	= func_language.c
func_language_la_LDFLAGS	= -module -avoid-version -no-undefined
func_language_la_CFLAGS		= $(AM_CFLAGS)
func_language_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_LANGUAGE

if WANT_FUNC_LOGIC
cwmod_LTLIBRARIES		+= func_logic.la
func_logic_la_SOURCES		= func_logic.c
func_logic_la_LDFLAGS		= -module -avoid-version -no-undefined
func_logic_la_CFLAGS		= $(AM_CFLAGS)
func_logic_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_LOGIC

if WANT_FUNC_MATH
cwmod_LTLIBRARIES		+= func_math.la
func_math_la_SOURCES		= func_math.c
func_math_la_LDFLAGS		= -module -avoid-version -no-undefined
func_math_la_CFLAGS		= $(AM_CFLAGS)
func_math_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_MATH

if WANT_FUNC_MD5
cwmod_LTLIBRARIES		+= func_md5.la
func_md5_la_SOURCES		= func_md5.c
func_md5_la_LDFLAGS		= -module -avoid-version -no-undefined
func_md5_la_CFLAGS		= $(AM_CFLAGS)
func_md5_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_MD5

if WANT_FUNC_MOH
cwmod_LTLIBRARIES		+= func_moh.la
func_moh_la_SOURCES		= func_moh.c
func_moh_la_LDFLAGS		= -module -avoid-version -no-undefined
func_moh_la_CFLAGS		= $(AM_CFLAGS)
func_moh_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_MOH

if WANT_FUNC_STRINGS
cwmod_LTLIBRARIES		+= func_strings.la
func_strings_la_SOURCES		= func_strings.c
func_strings_la_LDFLAGS		= -module -avoid-version -no-undefined
func_strings_la_CFLAGS		= $(AM_CFLAGS)
func_strings_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_STRINGS

if WANT_FUNC_TIMEOUT
cwmod_LTLIBRARIES		+= func_timeout.la
func_timeout_la_SOURCES		= func_timeout.c
func_timeout_la_LDFLAGS		= -module -avoid-version -no-undefined
func_timeout_la_CFLAGS		= $(AM_CFLAGS)
func_timeout_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_TIMEOUT

if WANT_FUNC_URI
cwmod_LTLIBRARIES		+= func_uri.la
func_uri_la_SOURCES		= func_uri.c
func_uri_la_LDFLAGS		= -module -avoid-version -no-undefined
func_uri_la_CFLAGS		= $(AM_CFLAGS)
func_uri_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_FUNC_URI

INCLUDES = -I$(top_srcdir)/include
