/*
 * udpfromto.c Helper functions to get/set addresses of UDP packets 
 *             based on recvfromto by Miquel van Smoorenburg
 *
 * recvfromto	Like recvfrom, but also stores the destination
 *		IP address. Useful on multihomed hosts.
 *
 *		Should work on Linux and BSD.
 *
 *		Copyright (C) 2002 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU Lesser General Public
 *		License as published by the Free Software Foundation; either
 *		version 2 of the License, or (at your option) any later version.
 *
 * sendfromto	added 18/08/2003, Jan Berkel <jan@sitadelle.com>
 *		Works on Linux and FreeBSD (5.x)
 *
 * sendfromto   modified to fallback to sendto if from hasn't been specified (is NULL)
 *		10/10/2005, Stefan Knoblich <stkn@gentoo.org>
 *
 * updfromto_init   don't return -1 if no support is available
 *		    10/10/2005, Stefan Knoblich <stkn@gentoo.org>
 *
 * Version: $Id$
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "callweaver/udpfromto.h"

int cw_udpfromto_init(int s)
{
#if defined(HAVE_IP_PKTINFO) || defined(HAVE_IP_RECVDSTADDR)
    static const int opt = 1;
	int err = -1;
    
#ifdef HAVE_IP_PKTINFO
	errno = ENOSYS;
	/* Set the IP_PKTINFO option (Linux). */
	err = setsockopt(s, SOL_IP, IP_PKTINFO, &opt, sizeof(opt));
#endif
#ifdef HAVE_IP_RECVDSTADDR
	/*
	 * Set the IP_RECVDSTADDR option (BSD). 
	 * Note: IP_RECVDSTADDR == IP_SENDSRCADDR 
	 */
	err = setsockopt(s, IPPROTO_IP, IP_RECVDSTADDR, &opt, sizeof(opt));
#endif
	return err;
#else
	return 0;
#endif
}
	
int cw_recvfromto(int s,
                    void *buf,
                    size_t len,
                    int flags,
                    struct sockaddr *from,
                    socklen_t *fromlen,
                    struct sockaddr *to,
                    socklen_t *tolen)
{
#if defined(HAVE_IP_PKTINFO) || defined(HAVE_IP_RECVDSTADDR)
	struct msghdr msgh;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char cbuf[256];
	int err;

	/*
	 *	If from or to are set, they must be big enough
	 *	to store a struct sockaddr_in.
	 */
	if ((from && (!fromlen || *fromlen < sizeof(struct sockaddr_in)))
        ||
	    (to   && (!tolen   || *tolen < sizeof(struct sockaddr_in))))
    {
		errno = EINVAL;
		return -1;
	}

	/*
	 *	IP_PKTINFO / IP_RECVDSTADDR don't provide sin_port so we have to
	 *	retrieve it using getsockname().
	 */
	if (to)
    {
		struct sockaddr_in si;
		socklen_t l = sizeof(si);

		((struct sockaddr_in *) to)->sin_family = AF_INET;
		((struct sockaddr_in *) to)->sin_port = 0;
		l = sizeof(si);
		if (getsockname(s, (struct sockaddr *) &si, &l) == 0)
        {
			((struct sockaddr_in *) to)->sin_port = si.sin_port;
			((struct sockaddr_in *) to)->sin_addr = si.sin_addr; 
		}
		if (tolen)
            *tolen = sizeof(struct sockaddr_in);
	}

	/* Set up iov and msgh structures. */
	memset(&msgh, 0, sizeof(struct msghdr));
	iov.iov_base = buf;
	iov.iov_len  = len;
	msgh.msg_control = cbuf;
	msgh.msg_controllen = sizeof(cbuf);
	msgh.msg_name = from;
	msgh.msg_namelen = fromlen  ?  *fromlen  :  0;
	msgh.msg_iov  = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_flags = 0;

	/* Receive one packet. */
	if ((err = recvmsg(s, &msgh, flags)) < 0)
		return err;
	if (fromlen)
        *fromlen = msgh.msg_namelen;

	/* Process auxiliary received data in msgh */
	for (cmsg = CMSG_FIRSTHDR(&msgh);
	     cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&msgh,cmsg))
    {

#ifdef HAVE_IP_PKTINFO
		if (cmsg->cmsg_level == SOL_IP
		    &&
            cmsg->cmsg_type == IP_PKTINFO)
        {
			struct in_pktinfo *i = (struct in_pktinfo *) CMSG_DATA(cmsg);
			if (to)
            {
				((struct sockaddr_in *) to)->sin_addr = i->ipi_addr;
				if (tolen)
                    *tolen = sizeof(struct sockaddr_in);
			}
			break;
		}
#endif

#ifdef HAVE_IP_RECVDSTADDR
		if (cmsg->cmsg_level == IPPROTO_IP
		    &&
            cmsg->cmsg_type == IP_RECVDSTADDR)
        {
			struct in_addr *i = (struct in_addr *)CMSG_DATA(cmsg);
			if (to)
            {
				((struct sockaddr_in *)to)->sin_addr = *i;
				if (tolen)
                    *tolen = sizeof(struct sockaddr_in);
			}
			break;
		}
#endif
	}
	return err;
#else 
	/* fallback: call recvfrom */
	return recvfrom(s, buf, len, flags, from, fromlen);
#endif
}

int cw_sendfromto(int s,
                    void *buf,
                    size_t len,
                    int flags,
                    struct sockaddr *from,
                    socklen_t fromlen,
                    struct sockaddr *to,
                    socklen_t tolen)
{
#if defined(HAVE_IP_PKTINFO) || defined(HAVE_IP_SENDSRCADDR)
	if (from  &&  fromlen)
	{
		struct msghdr msgh;
		struct cmsghdr *cmsg;
		struct iovec iov;
#ifdef HAVE_IP_PKTINFO
		char cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];
		struct in_pktinfo pktinfo, *pktinfo_ptr;
		memset(&pktinfo, 0, sizeof(struct in_pktinfo));
#endif

#ifdef HAVE_IP_SENDSRCADDR
		char cmsgbuf[CMSG_SPACE(sizeof(struct in_addr))];
#endif

		/* Set up iov and msgh structures. */
		memset(&msgh, 0, sizeof(struct msghdr));
		iov.iov_base = buf;
		iov.iov_len = len;
		msgh.msg_iov = &iov;
		msgh.msg_iovlen = 1;
		msgh.msg_control = cmsgbuf;
		msgh.msg_controllen = sizeof(cmsgbuf);
		msgh.msg_name = to;
		msgh.msg_namelen = tolen;
	
		cmsg = CMSG_FIRSTHDR(&msgh);

#ifdef HAVE_IP_PKTINFO
		cmsg->cmsg_level = SOL_IP;
		cmsg->cmsg_type = IP_PKTINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
		pktinfo.ipi_spec_dst = ((struct sockaddr_in *)from)->sin_addr;
		pktinfo_ptr = (struct in_pktinfo *)CMSG_DATA(cmsg);
		memcpy(pktinfo_ptr, &pktinfo, sizeof(struct in_pktinfo));
#endif
#ifdef HAVE_IP_SENDSRCADDR
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_SENDSRCADDR;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
		memcpy((struct in_addr *) CMSG_DATA(cmsg), 
               &((struct sockaddr_in *) from)->sin_addr,
               sizeof(struct in_addr));
#endif

		return sendmsg(s, &msgh, flags);
	}
    /* from is NULL, use good ol' sendto */
    return sendto(s, buf, len, flags, to, tolen);
#else
	/* fallback: call sendto() */
	return sendto(s, buf, len, flags, to, tolen);
#endif
}
