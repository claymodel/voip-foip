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
 *
 * \brief Various sorts of access control
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#include <fcntl.h>
#include <net/route.h>
#endif

#if defined(SOLARIS)
#include <sys/sockio.h>
#endif

/* netinet/ip.h may not define the following (See RFCs 791 and 1349) */
#if !defined(IPTOS_LOWCOST)
#define       IPTOS_LOWCOST           0x02
#endif

#if !defined(IPTOS_MINCOST)
#define       IPTOS_MINCOST           IPTOS_LOWCOST
#endif

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/acl.c $", "$Revision: 4723 $")

#include "callweaver/acl.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/options.h"
#include "callweaver/utils.h"
#include "callweaver/lock.h"
#include "callweaver/srv.h"

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
CW_MUTEX_DEFINE_STATIC(routeseq_lock);
#endif

struct cw_ha {
	/* Host access rule */
	struct in_addr netaddr;
	struct in_addr netmask;
	int sense;
	struct cw_ha *next;
};

/* Default IP - if not otherwise set, don't breathe garbage */
static struct in_addr __ourip = { 0x00000000 };

struct my_ifreq {
	char ifrn_name[IFNAMSIZ];	/* Interface name, e.g. "eth0", "ppp0", etc.  */
	struct sockaddr_in ifru_addr;
};

/* Free HA structure */
void cw_free_ha(struct cw_ha *ha)
{
	struct cw_ha *hal;
	while(ha) {
		hal = ha;
		ha = ha->next;
		free(hal);
	}
}

/* Copy HA structure */
static void cw_copy_ha(struct cw_ha *from, struct cw_ha *to)
{
	memcpy(&to->netaddr, &from->netaddr, sizeof(from->netaddr));
	memcpy(&to->netmask, &from->netmask, sizeof(from->netmask));
	to->sense = from->sense;
}

/* Create duplicate of ha structure */
static struct cw_ha *cw_duplicate_ha(struct cw_ha *original)
{
	struct cw_ha *new_ha = malloc(sizeof(struct cw_ha));
	/* Copy from original to new object */
	cw_copy_ha(original, new_ha); 

	return new_ha;
}

/* Create duplicate HA link list */
/*  Used in chan_sip2 templates */
struct cw_ha *cw_duplicate_ha_list(struct cw_ha *original)
{
	struct cw_ha *start=original;
	struct cw_ha *ret = NULL;
	struct cw_ha *link,*prev=NULL;

	while (start) {
		link = cw_duplicate_ha(start);  /* Create copy of this object */
		if (prev)
			prev->next = link;		/* Link previous to this object */

		if (!ret) 
			ret = link;		/* Save starting point */

		start = start->next;		/* Go to next object */
		prev = link;			/* Save pointer to this object */
	}
	return ret;    			/* Return start of list */
}

struct cw_ha *cw_append_ha(char *sense, char *stuff, struct cw_ha *path)
{
	struct cw_ha *ha = malloc(sizeof(struct cw_ha));
	char *nm = "255.255.255.255";
	char tmp[256];
	struct cw_ha *prev = NULL;
	struct cw_ha *ret;
	int x, z;
	unsigned int y;
	ret = path;
	while (path) {
		prev = path;
		path = path->next;
	}
	if (ha) {
		cw_copy_string(tmp, stuff, sizeof(tmp));
		nm = strchr(tmp, '/');
		if (!nm) {
			nm = "255.255.255.255";
		} else {
			*nm = '\0';
			nm++;
		}
		if (!strchr(nm, '.')) {
			if ((sscanf(nm, "%d", &x) == 1) && (x >= 0) && (x <= 32)) {
				y = 0;
				for (z=0;z<x;z++) {
					y >>= 1;
					y |= 0x80000000;
				}
				ha->netmask.s_addr = htonl(y);
			}
		} else if (!inet_aton(nm, &ha->netmask)) {
			cw_log(LOG_WARNING, "%s is not a valid netmask\n", nm);
			free(ha);
			return ret;
		}
		if (!inet_aton(tmp, &ha->netaddr)) {
			cw_log(LOG_WARNING, "%s is not a valid IP\n", tmp);
			free(ha);
			return ret;
		}
		ha->netaddr.s_addr &= ha->netmask.s_addr;
		if (!strncasecmp(sense, "p", 1)) {
			ha->sense = CW_SENSE_ALLOW;
		} else {
			ha->sense = CW_SENSE_DENY;
		}
		ha->next = NULL;
		if (prev) {
			prev->next = ha;
		} else {
			ret = ha;
		}
	}
	cw_log(LOG_DEBUG, "%s/%s appended to acl for peer\n", stuff, nm);
	return ret;
}

int cw_apply_ha(struct cw_ha *ha, struct sockaddr_in *sin)
{
	/* Start optimistic */
	int res = CW_SENSE_ALLOW;
	while (ha) {
		char iabuf[INET_ADDRSTRLEN];
		char iabuf2[INET_ADDRSTRLEN];

		/* For each rule, if this address and the netmask = the net address
		   apply the current rule */
		if ((sin->sin_addr.s_addr & ha->netmask.s_addr) == ha->netaddr.s_addr)
			res = ha->sense;
		/* DEBUG */
                if (option_debug > 5)
		cw_log(LOG_DEBUG,
			"##### Testing %s with %s. Result %d\n",
			cw_inet_ntoa(iabuf, sizeof(iabuf), sin->sin_addr),
			cw_inet_ntoa(iabuf2, sizeof(iabuf2), ha->netaddr),
			res
		);
		ha = ha->next;
	}
        if (option_debug > 5)
	    cw_log(LOG_DEBUG, "##### apply_ha Result: %d\n", res);
	return res;
}

int cw_get_ip_or_srv(struct sockaddr_in *sin, const char *value, const char *service)
{
	struct hostent *hp;
	struct cw_hostent ahp;
	char srv[256];
	char host[256];
	int tportno = ntohs(sin->sin_port);
	if (inet_aton(value, &sin->sin_addr))
		return 0;
	if (service) {
		snprintf(srv, sizeof(srv), "%s.%s", service, value);
		if (cw_get_srv(NULL, host, sizeof(host), &tportno, srv) > 0) {
			sin->sin_port = htons(tportno);
			value = host;
		}
	}
	hp = cw_gethostbyname(value, &ahp);
	if (hp) {
		memcpy(&sin->sin_addr, hp->h_addr, sizeof(sin->sin_addr));
		return 0;
	}

	return -1;
}

int cw_str2tos(const char *value, int *tos)
{
	int fval;
	if (sscanf(value, "%i", &fval) == 1)
		*tos = fval & 0xff;
	else if (!strcasecmp(value, "lowdelay"))
		*tos = IPTOS_LOWDELAY;
	else if (!strcasecmp(value, "throughput"))
		*tos = IPTOS_THROUGHPUT;
	else if (!strcasecmp(value, "reliability"))
		*tos = IPTOS_RELIABILITY;
	else if (!strcasecmp(value, "mincost"))
		*tos = IPTOS_MINCOST;
	else if (!strcasecmp(value, "none"))
		*tos = 0;
	else
		return -1;
	return 0;
}

int cw_get_ip(struct sockaddr_in *sin, const char *value)
{
	return cw_get_ip_or_srv(sin, value, NULL);
}

/* iface is the interface (e.g. eth0); address is the return value */
int cw_lookup_iface(char *iface, struct in_addr *address) 
{
	int mysock, res = 0;
	struct my_ifreq ifreq;

	memset(&ifreq, 0, sizeof(ifreq));
	cw_copy_string(ifreq.ifrn_name, iface, sizeof(ifreq.ifrn_name));

	mysock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	res = ioctl(mysock, SIOCGIFADDR, &ifreq);

	close(mysock);
	if (res < 0) {
		cw_log(LOG_WARNING, "Unable to get IP of %s: %s\n", iface, strerror(errno));
		memcpy((char *)address, (char *)&__ourip, sizeof(__ourip));
		return -1;
	} else {
		memcpy((char *)address, (char *)&ifreq.ifru_addr.sin_addr, sizeof(ifreq.ifru_addr.sin_addr));
		return 0;
	}
}

int cw_ouraddrfor(struct in_addr *them, struct in_addr *us)
{
	int s;
	struct sockaddr_in sin;
	socklen_t slen;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		cw_log(LOG_WARNING, "Cannot create socket\n");
		return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = 5060;
	sin.sin_addr = *them;
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin))) {
		cw_log(LOG_WARNING, "Cannot connect\n");
		close(s);
		return -1;
	}
	slen = sizeof(sin);
	if (getsockname(s, (struct sockaddr *)&sin, &slen)) {
		cw_log(LOG_WARNING, "Cannot get socket name\n");
		close(s);
		return -1;
	}
	close(s);
	*us = sin.sin_addr;
	return 0;
}

int cw_find_ourip(struct in_addr *ourip, struct sockaddr_in bindaddr)
{
	char ourhost[MAXHOSTNAMELEN] = "";
	struct cw_hostent ahp;
	struct hostent *hp;
	struct in_addr saddr;

	/* just use the bind address if it is nonzero */
	if (ntohl(bindaddr.sin_addr.s_addr)) {
		memcpy(ourip, &bindaddr.sin_addr, sizeof(*ourip));
		return 0;
	}
	/* try to use our hostname */
	if (gethostname(ourhost, sizeof(ourhost) - 1)) {
		cw_log(LOG_WARNING, "Unable to get hostname\n");
	} else {
		hp = cw_gethostbyname(ourhost, &ahp);
		if (hp) {
			memcpy(ourip, hp->h_addr, sizeof(*ourip));
			return 0;
		}
	}
	/* A.ROOT-SERVERS.NET. */
	if (inet_aton("198.41.0.4", &saddr) && !cw_ouraddrfor(&saddr, ourip))
		return 0;
	return -1;
}

