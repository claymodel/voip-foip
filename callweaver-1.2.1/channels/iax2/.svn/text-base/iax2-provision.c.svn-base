/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 * \brief IAX Provisioning Protocol 
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL$", "$Revision$")

#if defined(__linux__)
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include "iax2.h"
#include "iax2-provision.h"
#include "iax2-parser.h"

static struct iax_flag {
	char *name;
	int value;
} iax_flags[] = {
	{ "register", PROV_FLAG_REGISTER },
	{ "secure", PROV_FLAG_SECURE },
	{ "heartbeat", PROV_FLAG_HEARTBEAT },
	{ "debug", PROV_FLAG_DEBUG },
	{ "disablecid", PROV_FLAG_DIS_CALLERID },
	{ "disablecw", PROV_FLAG_DIS_CALLWAIT },
	{ "disablecidcw", PROV_FLAG_DIS_CIDCW },
	{ "disable3way", PROV_FLAG_DIS_THREEWAY },
};

char *iax_provflags2str(char *buf, int buflen, unsigned int flags)
{
	int x;
	if (!buf || buflen < 1) {
		return(NULL);
	}
	buf[0] = '\0';
	for (x=0;x<sizeof(iax_flags) / sizeof(iax_flags[0]); x++) {
		if (flags & iax_flags[x].value){
			strncat(buf, iax_flags[x].name, buflen - strlen(buf) - 1);
			strncat(buf, ",", buflen - strlen(buf) - 1);
		}
	}
	if (strlen(buf)) 
		buf[strlen(buf) - 1] = '\0';
	else
		strncpy(buf, "none", buflen - 1);
	return buf;
}
