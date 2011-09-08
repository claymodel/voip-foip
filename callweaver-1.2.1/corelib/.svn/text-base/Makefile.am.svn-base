DEFS += -include $(top_builddir)/include/confdefs.h

SUBDIRS=jitterbuffer

cwlib_LTLIBRARIES = libcallweaver.la
libcallweaver_la_SOURCES = version.c io.c sched.c logger.c frame.c config.c channel.c \
	generator.c translate.c file.c say.c pbx.c cli.c term.c \
	ulaw.c alaw.c phone_no_utils.c callerid.c image.c app.c \
	cdr.c acl.c rtp.c manager.c callweaver_hash.c\
	dsp.c chanvars.c indications.c autoservice.c db.c privacy.c \
	callweaver_mm.c enum.c srv.c dns.c aescrypt.c aestab.c aeskey.c \
	malloc.c utils.c dnsmgr.c devicestate.c \
	netsock.c slinfactory.c callweaver_expr2.c \
	callweaver_expr2f.c strcompat.c loader.c callweaver.c \
	stubfunctions-adsi.c stubfunctions-crypto.c stubfunctions-features.c \
	stubfunctions-monitor.c udp.c udptl.c udpfromto.c stun.c coef_in.h coef_out.h \
	ecdisa.h aesopt.h callweaver_expr2.h jitterbuffer/libcwjb.la

nodist_libcallweaver_la_SOURCES = defaults.h

if WANT_DEBUG
libcallweaver_la_CFLAGS 	=  -D_REENTRANT -Wall -Wstrict-prototypes 
else
libcallweaver_la_CFLAGS 	=  -fomit-frame-pointer -D_REENTRANT -Wall -Wstrict-prototypes 
endif
libcallweaver_la_CFLAGS	+= -Wmissing-prototypes -Wmissing-declarations @SQLITE3_THREADSAFE_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/include $(AM_CFLAGS) 
#libcallweaver_la_CFLAGS	+= -DDETECT_DEADLOCKS -DCW_MUTEX_INIT_W_CONSTRUCTORS -DTHREAD_CRASH

libcallweaver_la_LIBADD	=  -lspandsp -ltiff $(LIBLTDL)
libcallweaver_la_LIBADD	+= @SQLITE3_THREADSAFE_LIBS@
libcallweaver_la_LIBADD += ${top_builddir}/stdtime/libtime.la 
libcallweaver_la_LIBADD += ${top_builddir}/corelib/jitterbuffer/libcwjb.la
libcallweaver_la_LIBADD	+= -lreadline
libcallweaver_la_LIBADD	+= -lssl -lcrypto @SSL_LIBS@
if WANT_MEMCACHE
libcallweaver_la_LIBADD	+= -lmemcache
endif WANT_MEMCACHE

libcallweaver_la_CFLAGS	+= @SSL_CFLAGS@

if WANT_SRTP
libcallweaver_la_LIBADD	+= -lsrtp
endif WANT_SRTP

libcallweaver_la_LDFLAGS   = -no-undefined

BUILT_SOURCES = defaults.h version.sh version #callweaver_expr2.c callweaver_expr2.h callweaver_expr2f.c
EXTRA_DIST = defaults.h.in version.sh.in
CLEANFILES = defaults.h defaults.h.tmp version.sh version.c version.c.tmp

@substitute@

callweaver_expr2.c callweaver_expr2.h: callweaver_expr2.y
	$(YACC) -d --name-prefix=cw_yy -o callweaver_expr2.c callweaver_expr2.y

callweaver_expr2f.c: callweaver_expr2.l
	$(LEX) --full -t callweaver_expr2.l > callweaver_expr2f.c

defaults.h: defaults.h.in Makefile
	rm -f defaults.h.tmp
	$(substitute) $(srcdir)/defaults.h.in > defaults.h.tmp
	mv defaults.h.tmp defaults.h

.PHONY: version
version:
	(p=`pwd`; cd "${top_srcdir}"; sh "$${p}/version.sh") > version.c.tmp
	if cmp version.c.tmp version.c > /dev/null 2>&1; then rm -f version.c.tmp; else mv version.c.tmp version.c; fi
