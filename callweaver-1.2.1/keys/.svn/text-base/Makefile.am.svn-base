AUTOMAKE_OPTS = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

KEYS = dh1024.pem \
	 dh512.pem \
	 servercert.pem \
	 serverkey.pem \
	 trustcerts.pem

install-data-local:
	if [ ! -d "$(DESTDIR)${cwkeydir}" ]; \
	then \
	    mkdir -p $(DESTDIR)${cwkeydir}; \
	    chmod 0750 $(DESTDIR)${cwkeydir}; \
	fi; \
	if test -z "$(DESTDIR)"; \
	then \
		chown ${cwrunuser}:${cwrungroup} $(DESTDIR)${cwkeydir}; \
	fi; \
	for keyfile in $(KEYS); \
	do \
		if test ! -f $(DESTDIR)${cwkeydir}/$${keyfile}; \
		then \
			$(INSTALL) -m 0640 $${keyfile} $(DESTDIR)${cwkeydir}/$${keyfile}; \
			if test -z "$(DESTDIR)"; \
			then \
				chown ${cwrunuser}:${cwrungroup} $(DESTDIR)${cwkeydir}/$${keyfile}; \
			fi; \
		fi; \
	done

