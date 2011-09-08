/*
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef tcp_init_h
#define tcp_init_h
#include "ip_addr.h"

#define DEFAULT_TCP_CONNECTION_LIFETIME_S 120 /* in  seconds */
/* maximum accepted lifetime in ticks (maximum possible is  ~ MAXINT/2) */
#define MAX_TCP_CON_LIFETIME	((1U<<(sizeof(ticks_t)*8-1))-1)

#define DEFAULT_TCP_SEND_TIMEOUT 10 /* if a send can't write for more then 10s,
									   timeout */
#define DEFAULT_TCP_CONNECT_TIMEOUT 10 /* if a connect doesn't complete in this
										  time, timeout */
#define DEFAULT_TCP_MAX_CONNECTIONS 2048 /* maximum connections */

#define DEFAULT_TCP_BUF_SIZE	4096  /* buffer size used for reads */

#define DEFAULT_TCP_WBUF_SIZE	2100 /*  after debugging switch to 4-16k */

struct tcp_child{
	pid_t pid;
	int proc_no; /* ser proc_no, for debugging */
	int unix_sock; /* unix "read child" sock fd */
	int busy;
	int n_reqs; /* number of requests serviced so far */
};

#define TCP_ALIAS_FORCE_ADD 1
#define TCP_ALIAS_REPLACE   2


int init_tcp();
void destroy_tcp();
int tcp_init(struct socket_info* sock_info);
int tcp_init_children();
void tcp_main_loop();
void tcp_receive_loop(int unix_sock);
int tcp_fix_child_sockets(int* fd);

/* sets source address used when opening new sockets and no source is specified
 *  (by default the address is choosen by the kernel)
 * Should be used only on init.
 * returns -1 on error */
int tcp_set_src_addr(struct ip_addr* ip);

#endif
