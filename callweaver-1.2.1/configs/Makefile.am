AUTOMAKE_OPTS = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

SAMPLES = adsi.conf.sample \
	adtranvofr.conf.sample \
	db-memcached.conf.sample \
	cdr.conf.sample \
	cdr_custom.conf.sample \
	cdr_manager.conf.sample \
	cdr_tds.conf.sample \
	codecs.conf.sample \
	dnsmgr.conf.sample \
	enum.conf.sample \
	extconfig.conf.sample \
	extensions.conf.sample \
	extensions.ael.sample \
	indications.conf.sample \
	logger.conf.sample \
	manager.conf.sample \
	meetme.conf.sample \
	modem.conf.sample \
	modules.conf.sample \
	musiconhold.conf.sample \
	muted.conf.sample \
	callweaver.adsi.sample \
	callweaver.conf.sample \
	osp.conf.sample \
	privacy.conf.sample \
	res_snmp.conf.sample \
	rtp.conf.sample \
	udptl.conf.sample \
	amd.conf.sample

if WANT_MEMCACHE
SAMPLES += db-memcached.conf.sample
endif WANT_MEMCACHE

if WANT_APP_QUEUE
SAMPLES += queues.conf.sample
endif WANT_APP_QUEUE

if WANT_APP_VOICEMAIL
SAMPLES += voicemail.conf.sample
endif WANT_APP_VOICEMAIL

if WANT_CDR_MYSQL
SAMPLES	+= cdr_mysql.conf.sample
endif WANT_CDR_MYSQL

#if WANT_CDR_ODBC
#SAMPLES	+= cdr_odbc.conf.sample
#endif WANT_CDR_ODBC

if WANT_CDR_PGSQL
SAMPLES	+= cdr_pgsql.conf.sample
endif WANT_CDR_PGSQL

if WANT_CDR_PGSQL_CUSTOM
SAMPLES += cdr_pgsql_custom.conf.sample
endif WANT_CDR_PGSQL_CUSTOM

if WANT_CHAN_AGENT
SAMPLES += agents.conf.sample
endif WANT_CHAN_AGENT

if WANT_CHAN_ALSA
SAMPLES	+= alsa.conf.sample
endif WANT_CHAN_ALSA

if WANT_CHAN_CAPI
SAMPLES += capi.conf.sample
endif WANT_CHAN_CAPI

if WANT_CHAN_FAX
SAMPLES += chan_fax.conf.sample
endif WANT_CHAN_FAX

if WANT_CHAN_FEATURES
SAMPLES += features.conf.sample
endif WANT_CHAN_FEATURES

if WANT_CHAN_IAX2
SAMPLES += iax.conf.sample
endif WANT_CHAN_IAX2

if WANT_CHAN_MGCP
SAMPLES += mgcp.conf.sample
endif WANT_CHAN_MGCP

if WANT_CHAN_MISDN
SAMPLES += misdn.conf.sample
endif WANT_CHAN_MISDN

if WANT_CHAN_SIP
SAMPLES += sip.conf.sample sip_notify.conf.sample
endif WANT_CHAN_SIP

if WANT_CHAN_SCCP
SAMPLES	+= sccp.conf.sample
endif WANT_CHAN_SCCP

if WANT_CHAN_UNICALL
SAMPLES += unicall.conf.sample
endif WANT_CHAN_UNICALL

if WANT_CHAN_VISDN
SAMPLES += chan_visdn.conf.sample
endif WANT_CHAN_VISDN

if WANT_CHAN_WOOMERA
SAMPLES += woomera.conf.sample
endif WANT_CHAN_WOOMERA

if WANT_CHAN_ZAP
SAMPLES += chan_dahdi.conf.sample
endif WANT_CHAN_ZAP

if WANT_PBX_DUNDI
SAMPLES += dundi.conf.sample
endif WANT_PBX_DUNDI

if WANT_RES_JABBER
SAMPLES += res_jabber.conf.sample
endif WANT_RES_JABBER

#if WANT_RES_ODBC
#SAMPLES += res_odbc.conf.sample
#endif WANT_RES_ODBC

if WANT_RES_CONFIG_MYSQL
SAMPLES += res_mysql.conf.sample
endif WANT_RES_CONFIG_MYSQL

if WANT_RES_CONFIG_PGSQL
SAMPLES += res_pgsql.conf.sample
endif WANT_RES_CONFIG_PGSQL

if WANT_RES_CONFIG_CURL
SAMPLES += curl.conf.sample
endif WANT_RES_CONFIG_CURL

if WANT_RES_SQLITE
SAMPLES += res_sqlite.conf.sample
endif WANT_RES_SQLITE

if WANT_CHAN_BLUETOOTH
SAMPLES += chan_bluetooth.conf.sample
endif WANT_CHAN_BLUETOOTH

noinst_DATA = $(SAMPLES)

@substitute@

EXTRA_DIST = $(SAMPLES) callweaver.conf.sample.in logger.conf.sample.in
CLEANFILES = callweaver.conf.sample logger.conf.sample musiconhold.conf.sample

callweaver.conf.sample: callweaver.conf.sample.in Makefile
	rm -f callweaver.conf.sample.tmp
	$(substitute) $(srcdir)/callweaver.conf.sample.in > callweaver.conf.sample.tmp
	mv callweaver.conf.sample.tmp callweaver.conf.sample

logger.conf.sample: logger.conf.sample.in Makefile
	rm -f logger.conf.sample.tmp
	$(substitute) $(srcdir)/logger.conf.sample.in > logger.conf.sample.tmp
	mv logger.conf.sample.tmp logger.conf.sample

musiconhold.conf.sample: musiconhold.conf.sample.in Makefile
	rm -f musiconhold.conf.sample.tmp
	$(substitute) $(srcdir)/musiconhold.conf.sample.in > musiconhold.conf.sample.tmp
	mv musiconhold.conf.sample.tmp musiconhold.conf.sample

install-data-local:
	if [ ! -d "$(DESTDIR)${cwconfdir}" ]; \
	then \
	    mkdir -p $(DESTDIR)${cwconfdir}; \
	    chmod 0750 $(DESTDIR)${cwconfdir}; \
	fi; \
	if test -z "$(DESTDIR)"; \
	then \
		chown ${cwrunuser}:${cwrungroup} $(DESTDIR)${cwconfdir}; \
	fi; \
	for samplefile in $(SAMPLES); \
	do \
		configfile=`basename $${samplefile} .sample`; \
		if test ! -f $(DESTDIR)${cwconfdir}/$${configfile}; \
		then \
			$(INSTALL) -m 0640 $${samplefile} $(DESTDIR)${cwconfdir}/$${configfile}; \
			if test -z "$(DESTDIR)"; \
			then \
				chown ${cwrunuser}:${cwrungroup} $(DESTDIR)${cwconfdir}/$${configfile}; \
			fi; \
		fi; \
	done
