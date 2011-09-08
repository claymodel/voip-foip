AUTOMAKE_OPTS			= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES 		=

if WANT_CHAN_FAX
cwmod_LTLIBRARIES		+= chan_fax.la
chan_fax_la_SOURCES		= chan_fax.c faxmodem.c faxmodem.h

chan_fax_la_CFLAGS		= $(AM_CFLAGS)
chan_fax_la_LIBADD		= @CHAN_FAX_LIBS@ ${top_builddir}/corelib/libcallweaver.la
chan_fax_la_LDFLAGS            = -module -avoid-version -no-undefined
endif WANT_CHAN_FAX

INCLUDES = -I${top_srcdir}/corelib -I$(top_srcdir)/include
