/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 * Kevin P. Fleming <kpfleming@digium.com>
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
 * \brief Network socket handling
 */

#ifndef _CALLWEAVER_NETSOCK_H
#define _CALLWEAVER_NETSOCK_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <netinet/in.h>
#include "callweaver/io.h"
#include "callweaver/cwobj.h"

struct cw_netsock;

struct cw_netsock_list;

struct cw_netsock_list *cw_netsock_list_alloc(void);

int cw_netsock_init(struct cw_netsock_list *list);

struct cw_netsock *cw_netsock_bind(struct cw_netsock_list *list, struct io_context *ioc,
				     const char *bindinfo, int defaultport, int tos, cw_io_cb callback, void *data);

struct cw_netsock *cw_netsock_bindaddr(struct cw_netsock_list *list, struct io_context *ioc,
					 struct sockaddr_in *bindaddr, int tos, cw_io_cb callback, void *data);

int cw_netsock_free(struct cw_netsock_list *list, struct cw_netsock *netsock);

int cw_netsock_release(struct cw_netsock_list *list);

struct cw_netsock *cw_netsock_find(struct cw_netsock_list *list,
				     struct sockaddr_in *sa);

int cw_netsock_sockfd(const struct cw_netsock *ns);

const struct sockaddr_in *cw_netsock_boundaddr(const struct cw_netsock *ns);

void *cw_netsock_data(const struct cw_netsock *ns);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_NETSOCK_H */
