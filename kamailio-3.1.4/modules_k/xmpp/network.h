/*
 * $Id$
 *
 * XMPP Module
 * This file is part of Kamailio, a free SIP server.
 *
 * Copyright (C) 2006 Voice Sistem S.R.L.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andreea Spirea
 *
 */

/*! \file
 * \ingroup xmpp
 */


#ifndef _NETWORK_H
#define _NETWORK_H

extern int net_listen(char *server, int port);
extern int net_connect(char *server, int port);
extern int net_send(int fd, const char *buf, int len);
extern int net_printf(int fd, char *format, ...) __attribute__ ((format (printf, 2, 3)));
extern char *net_read_chunk(int fd, int size);
extern char *net_read_static(int fd);

#endif /* _NETWORK_H */
