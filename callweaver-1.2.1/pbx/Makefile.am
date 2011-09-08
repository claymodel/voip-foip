AUTOCONF_OPTS 		= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

BUILT_SOURCES =

cwmod_LTLIBRARIES 	= pbx_spool.la pbx_loopback.la pbx_config.la

pbx_config_la_SOURCES 	= pbx_config.c
pbx_config_la_LDFLAGS	= -module -avoid-version -no-undefined
pbx_config_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
pbx_spool_la_SOURCES 	= pbx_spool.c
pbx_spool_la_LDFLAGS	= -module -avoid-version -no-undefined
pbx_spool_la_LIBADD    = ${top_builddir}/corelib/libcallweaver.la
pbx_loopback_la_SOURCES = pbx_loopback.c
pbx_loopback_la_LDFLAGS = -module -avoid-version -no-undefined
pbx_loopback_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la

if WANT_PBX_DUNDI
cwmod_LTLIBRARIES	+= pbx_dundi.la
pbx_dundi_la_SOURCES 	= pbx_dundi.c dundi-parser.c dundi-parser.h
pbx_dundi_la_LIBADD 	= -lz ${top_builddir}/corelib/libcallweaver.la
pbx_dundi_la_LDFLAGS 	= -module -avoid-version -no-undefined
endif WANT_PBX_DUNDI

if WANT_PBX_REALTIME
cwmod_LTLIBRARIES	+= pbx_realtime.la
pbx_realtime_la_SOURCES = pbx_realtime.c
pbx_realtime_la_LDFLAGS = -module -avoid-version -no-undefined
pbx_realtime_la_LIBADD  = ${top_builddir}/corelib/libcallweaver.la
endif WANT_PBX_REALTIME

if WANT_PBX_AEL
cwmod_LTLIBRARIES	+= pbx_ael.la
pbx_ael_la_SOURCES	= pbx_ael.c ael/ael_lex.c ael/ael_tab.c
pbx_ael_la_LDFLAGS	= -module -avoid-version -no-undefined
pbx_ael_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
pbx_ael_la_CFLAGS	= $(AM_CFLAGS) -D_GNU_SOURCE=1

BUILT_SOURCES += ael/ael_tab.c ael/ael_tab.h ael/ael_lex.c

ael/ael_tab.c ael/ael_tab.h: ael/ael.y
	$(YACC) -d -o ael/ael_tab.c ael/ael.y

ael/ael_lex.c: ael/ael.l
	$(LEX) -t ael/ael.l > ael/ael_lex.c

endif WANT_PBX_AEL


INCLUDES = -I$(top_srcdir)/include 
