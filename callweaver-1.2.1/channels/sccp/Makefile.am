AUTOMAKE_OPTS			= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES 		=

if WANT_CHAN_SCCP
cwmod_LTLIBRARIES		+= chan_sccp.la
chan_sccp_la_SOURCES		= chan_sccp.c sccp_actions.c sccp_channel.c \
				sccp_cli.c sccp_device.c sccp_indicate.c sccp_line.c \
				sccp_pbx.c sccp_socket.c sccp_softkeys.c sccp_utils.c

chan_sccp_la_CFLAGS		= $(AM_CFLAGS)
chan_sccp_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
chan_sccp_la_LDFLAGS            = -module -avoid-version -no-undefined
endif WANT_CHAN_SCCP

INCLUDES = -I${top_srcdir}/corelib -I$(top_srcdir)/include
