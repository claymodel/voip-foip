#
# Main Automake file for CallWeaver.org
#
# Author: Michael "cypromis" Bielicki <michal.bielickie at halo2.pl>
# Copyright (c) 2005, 2006, 2007 Halo Kwadrat Sp. z o.o.
#
# This program is free software and it comes WITHOUT any warranty.
# You may redistribute it and/or modify it under the terms of the
# GNU General Public License (GPL) version 2 as published by the
# Free Software Foundation, but not any later version of the GPL.
#
# $Id$
#
###########
# GENERAL #
###########

AUTOMAKE_OPTS = gnu foreign
ACLOCAL_AMFLAGS = -I acmacros
SH = @SH@
SHELL = @SHELL@
AWK = @AWK@
AR = @AR@
GREP = @GREP@
SED = @SED@
M4 = @M4@
SVN = @SVN@

## LDFLAGS		= --as-needed --enable-new-dtags

##########
# DAEMON #
##########

libexecdir = ${cwexecdir}
libexec_PROGRAMS = callweaver

callweaver_SOURCES = main.c

callweaver_LDADD = ${top_builddir}/corelib/libcallweaver.la $(LIBLTDL)
callweaver_LDADD += -lreadline

callweaver_CFLAGS =  -fomit-frame-pointer -Wall -Wstrict-prototypes 
callweaver_CFLAGS += -Wmissing-prototypes -Wmissing-declarations -I$(top_srcdir)/include $(AM_CFLAGS)

#############
# UTILITIES #
#############

EXTRA_DIST = substitute.mak

utilitydir = ${cwutilsdir}
utility_SCRIPTS = callweaver_cli safe_callweaver
CLEANFILES = $(utility_SCRIPTS)

@substitute@

callweaver_cli:
	$(substitute) $(top_srcdir)/callweaver_cli.in > callweaver_cli
	chmod +x callweaver_cli

safe_callweaver:
	$(substitute) $(top_srcdir)/safe_callweaver.in > safe_callweaver
	chmod +x safe_callweaver

#############
# MAN PAGES #
#############

install-man-pages:
if WANT_MAN_PAGES
	if test ! -d $(DESTDIR)${cwmandir}/man8; then \
		mkdir -p $(DESTDIR)${cwmandir}/man8 ; \
	fi ; \
	cp $(top_srcdir)/callweaver.8 $(DESTDIR)${cwmandir}/man8/. ; \
	cp $(top_srcdir)/safe_callweaver.8 $(DESTDIR)${cwmandir}/man8/. ;
endif

uninstall-man-pages:
	if test -d $(DESTDIR)${cwmandir}/man8; then \
		rm -rf $(DESTDIR)${cwmandir}/man8/*callweaver.8 ; \
	fi ;

##############
# OTHER DOCS #
##############

install-docs: install-readme install-license install-credits install-sgml-docs

uninstall-docs:
	if test -d $(DESTDIR)${cwdocdir}; then \
		rm -rf $(DESTDIR)${cwdocdir} ; \
	fi ;

# README file
install-readme:
if WANT_README
	if test ! -d $(DESTDIR)${cwdocdir}; then \
		mkdir -p $(DESTDIR)${cwdocdir} ; \
	fi ; \
	cp $(top_srcdir)/README $(DESTDIR)${cwdocdir}/. ;
endif

uninstall-readme:
	if test -f $(DESTDIR)${cwdocdir}/README; then \
		if test ! -d $(DESTDIR)${cwdocdir}; then \
			mkdir -p $(DESTDIR)${cwdocdir} ; \
		fi ; \
		rm -f $(DESTDIR)${cwdocdir}/README ; \
	fi ;

# LICENSE file
install-license:
if WANT_LICENSE
	if test ! -d $(DESTDIR)${cwdocdir}; then \
		mkdir -p $(DESTDIR)${cwdocdir} ; \
	fi ; \
	cp $(top_srcdir)/LICENSE $(DESTDIR)${cwdocdir}/. ;
endif

uninstall-license:
	if test -f $(DESTDIR)${cwdocdir}/LICENSE; then \
		rm -f $(DESTDIR)${cwdocdir}/LICENSE ; \
	fi ;

# CREDITS file
install-credits:
if WANT_CREDITS
	if test ! -d $(DESTDIR)${cwdocdir}; then \
		mkdir -p $(DESTDIR)${cwdocdir} ; \
	fi ; \
	cp $(top_srcdir)/CREDITS $(DESTDIR)${cwdocdir}/. ;
endif

uninstall-credits:
	if test -f $(DESTDIR)${cwdocdir}/CREDITS; then \
		rm -f $(DESTDIR)${cwdocdir}/CREDITS ; \
	fi ;

# SGML docs
install-sgml-docs:
if WANT_SGML_DOCS
	if test ! -d $(DESTDIR)${cwdocdir}; then \
		mkdir -p $(DESTDIR)${cwdocdir} ; \
	fi ; \
	cp $(top_srcdir)/callweaver.sgml $(DESTDIR)${cwdocdir}/. ;
endif

uninstall-sgml-docs:
	if test -f $(DESTDIR)${cwdocdir}/callweaver.sgml; then \
		rm -f $(DESTDIR)${cwdocdir}/callweaver.sgml ; \
	fi ;

###################
# SUB DIRECTORIES #
###################

if WANT_SIP_TCP_SUPPORT
       MAYBE_KEYS = keys
endif

SUBDIRS =   libltdl libs
SUBDIRS +=  stdtime @SQLITE3_SUBDIR@ include corelib . pbx funcs
SUBDIRS +=  cdr apps codecs formats channels ogi utils res configs ${MAYBE_KEYS}

############################
# INSTALLATION DIRECTORIES #
############################

# mutable data
INSTALL_DIRS =	$(cwvardir) $(cwvardir)/core

# built-in database
INSTALL_DIRS +=	$(cwdbdir) $(cwsqlitedir)

# loadable modules
INSTALL_DIRS +=	$(cwmoddir) $(cwicdmoddir)

# CDRs and log files
INSTALL_DIRS +=	$(cwlogdir) $(cwlogdir)/cdr-custom $(cwlogdir)/cdr-csv

# pid file and console socket
INSTALL_DIRS +=	$(cwrundir)

# spool files
INSTALL_DIRS +=	$(cwspooldir) $(cwspooldir)/outgoing $(cwspooldir)/voicemail

# on-hold music
INSTALL_DIRS +=	$(cwmohdir)

# crypto keys
INSTALL_DIRS +=	$(cwkeydir)

# images
INSTALL_DIRS +=	$(cwimagesdir)

# create directories, set ownership and permissions
install-data-hook:
	for install_dir in $(INSTALL_DIRS); \
	do \
		if test ! -d "$(DESTDIR)$${install_dir}"; \
		then \
			mkdir -p $(DESTDIR)$${install_dir}; \
			chmod 0750 $(DESTDIR)$${install_dir}; \
		fi; \
		if test -z "$(DESTDIR)"; \
		then \
			chown ${cwrunuser}:${cwrungroup} $(DESTDIR)$${install_dir}; \
		fi; \
	done

# END OF FILE
