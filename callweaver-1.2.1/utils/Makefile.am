AUTOMAKE_OPTS = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwutils_PROGRAMS = streamplayer # check_expr 
streamplayer_SOURCES = streamplayer.c ${top_srcdir}/corelib/strcompat.c
# check_expr_SOURCES = check_expr.c ../cw_expr2.c ../cw_expr2f.c
# check_expr_CFLAGS  = -DNO_OPX_MM -D_GNU_SOURCE -DSTANDALONE $(AM_CFLAGS)

if USE_NEWT
    cwutils_PROGRAMS += cwman
    cwman_CFLAGS = $(AM_CFLAGS) @SSL_CFLAGS@
    cwman_SOURCES = cwman.c ${top_srcdir}/corelib/utils.c
    cwman_LDADD = -lnewt @SSL_LIBS@
endif USE_NEWT

if WANT_SMSQ
    cwutils_PROGRAMS += smsq
    smsq_CFLAGS = $(AM_CFLAGS) -DSPOOLDIR=\"$(cwspooldir)\"
    smsq_SOURCES = smsq.c
    smsq_LDADD = -lpopt
endif WANT_SMSQ
