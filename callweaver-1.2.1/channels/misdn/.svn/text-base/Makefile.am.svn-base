AUTOMAKE_OPTS			= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES 		=

if WANT_CHAN_MISDN
cwmod_LTLIBRARIES		+= chan_misdn.la
chan_misdn_la_SOURCES		= chan_misdn.c chan_misdn_config.h asn1.c \
				  asn1.h fac.c fac.h isdn_lib.c isdn_lib.h \
				  isdn_lib_intern.h isdn_msg_parser.c misdn_config.c


chan_misdn_la_CFLAGS		= $(AM_CFLAGS) @MISDN_CFLAGS@ -D_GNU_SOURCE -DCHAN_MISDN_VERSION=\"0.4.0-rc28\"
chan_misdn_la_LIBADD		= @MISDN_LDFLAGS@ -lisdnnet -lmISDN ${top_builddir}/corelib/libcallweaver.la
chan_misdn_la_LDFLAGS            = -module -avoid-version -no-undefined
endif WANT_CHAN_MISDN

INCLUDES = -I${top_srcdir}/corelib -I$(top_srcdir)/include
