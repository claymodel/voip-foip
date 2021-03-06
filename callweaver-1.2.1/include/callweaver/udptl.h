/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * UDPTL support for T.38
 * 
 * Copyright (C) 2005, Steve Underood, partly based on RTP code which is
 * Copyright (C) 1999-2004, Digium, Inc.
 *
 * Steve Underood <steveu@coppice.org>
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
 *
 * $HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/include/callweaver/udptl.h $
 * $Revision: 4723 $
 */

#ifndef _CALLWEAVER_UDPTL_H
#define _CALLWEAVER_UDPTL_H

#include "callweaver/frame.h"
#include "callweaver/io.h"
#include "callweaver/sched.h"
#include "callweaver/channel.h"

#include <netinet/in.h>

enum
{
    UDPTL_ERROR_CORRECTION_NONE,
    UDPTL_ERROR_CORRECTION_FEC,
    UDPTL_ERROR_CORRECTION_REDUNDANCY
};

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct cw_udptl_protocol
{
	const char * const type;
	/* Get UDPTL struct, or NULL if unwilling to transfer */
	struct cw_udptl *(*get_udptl_info)(struct cw_channel *chan);
	/* Set UDPTL peer */
	int (* const set_udptl_peer)(struct cw_channel *chan, struct cw_udptl *peer);
	struct cw_udptl_protocol *next;
};

struct cw_udptl;

typedef int (*cw_udptl_callback)(struct cw_udptl *udptl, struct cw_frame *f, void *data);

struct cw_udptl *cw_udptl_new_with_sock_info(struct sched_context *sched,
                                                 struct io_context *io,
                                                 int callbackmode,
                                                 udp_socket_info_t *sock_info);

int cw_udptl_set_active(struct cw_udptl *udptl, int active);

void cw_udptl_set_peer(struct cw_udptl *udptl, struct sockaddr_in *them);

void cw_udptl_get_peer(struct cw_udptl *udptl, struct sockaddr_in *them);

void cw_udptl_get_us(struct cw_udptl *udptl, struct sockaddr_in *us);

int cw_udptl_get_stunstate(struct cw_udptl *udptl);

void cw_udptl_destroy(struct cw_udptl *udptl);

void cw_udptl_reset(struct cw_udptl *udptl);

void cw_udptl_set_callback(struct cw_udptl *udptl, cw_udptl_callback callback);

void cw_udptl_set_data(struct cw_udptl *udptl, void *data);

int cw_udptl_write(struct cw_udptl *udptl, struct cw_frame *f);

struct cw_frame *cw_udptl_read(struct cw_udptl *udptl);

int cw_udptl_fd(struct cw_udptl *udptl);

udp_socket_info_t *cw_udptl_udp_socket(struct cw_udptl *udptl,
                                         udp_socket_info_t *sock_info);

int cw_udptl_settos(struct cw_udptl *udptl, int tos);

void cw_udptl_set_m_type(struct cw_udptl* udptl, int pt);

void cw_udptl_set_udptlmap_type(struct cw_udptl *udptl, int pt,
									char *mimeType, char *mimeSubtype);

int cw_udptl_lookup_code(struct cw_udptl* udptl, int is_cw_format, int code);

void cw_udptl_offered_from_local(struct cw_udptl *udptl, int local);

int cw_udptl_get_preferred_error_correction_scheme(struct cw_udptl* udptl);

int cw_udptl_get_current_error_correction_scheme(struct cw_udptl* udptl);

void cw_udptl_set_error_correction_scheme(struct cw_udptl* udptl, int ec);

int cw_udptl_get_local_max_datagram(struct cw_udptl* udptl);

void cw_udptl_set_local_max_datagram(struct cw_udptl* udptl, int max_datagram);

int cw_udptl_get_far_max_datagram(struct cw_udptl* udptl);

void cw_udptl_set_far_max_datagram(struct cw_udptl* udptl, int max_datagram);

void cw_udptl_get_current_formats(struct cw_udptl *udptl,
									int *cw_formats, int *non_cw_formats);

void cw_udptl_setnat(struct cw_udptl *udptl, int nat);

enum cw_bridge_result cw_udptl_bridge(struct cw_channel *c0, struct cw_channel *c1, int flags, struct cw_frame **fo, struct cw_channel **rc);

int cw_udptl_proto_register(struct cw_udptl_protocol *proto);

void cw_udptl_proto_unregister(struct cw_udptl_protocol *proto);

void cw_udptl_stop(struct cw_udptl *udptl);

void cw_udptl_init(void);

void cw_udptl_reload(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
