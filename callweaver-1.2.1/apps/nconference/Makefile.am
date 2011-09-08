#
# CallWeaver.org
#
#
# $Id$
#
AUTOMAKE_OPTS                   = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES             =

cwmod_LTLIBRARIES                     += app_nconference.la
app_nconference_la_SOURCES              = frame.c conference.c member.c sounds.c dtmf.c vad.c cli.c \
					  jitterbuffer.c \
					  app_nconference.c 
app_nconference_la_LDFLAGS              = -module -avoid-version -no-undefined
app_nconference_la_LIBADD               = ${top_builddir}/corelib/libcallweaver.la
app_nconference_la_CFLAGS		= $(AM_CFLAGS) 
#app_nconference_la_CFLAGS		+= -DDETECT_DEADLOCKS -DTHREAD_CRASH -DDEBUG_THREADS

INCLUDES = -I$(top_srcdir)/include
