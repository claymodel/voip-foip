/*
 *  $Id$
 *
 * Copyright (C) 2005 iptelorg GmbH
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
/*
 * History:
 * -------
 *  2005-12-11 created by andrei
 *  2009-06-01 Pre- and post-script callbacks of onsend route are executed (Miklos)
 */


#ifndef onsend_h
#define onsend_h


#include "ip_addr.h"
#include "action.h"
#include "route.h"
#include "script_cb.h"
#include "sr_compat.h"

struct onsend_info{
	union sockaddr_union* to;
	struct socket_info* send_sock;
	char* buf;
	int len;
};

extern struct onsend_info* p_onsend;


#define get_onsend_info()	(p_onsend)

/*
 * returns: 0 drop the message, >= ok, <0 error (but forward the message)
 * it also migh change dst->send_flags!
 * WARNING: buf must be 0 terminated (to allow regex matches on it) */
static inline int run_onsend(struct sip_msg* orig_msg, struct dest_info* dst,
								char* buf, int len)
{
	struct onsend_info onsnd_info;
	int ret;
	struct run_act_ctx ra_ctx;
	int backup_route_type;
	snd_flags_t fwd_snd_flags_bak;
	snd_flags_t rpl_snd_flags_bak;
	
	ret=1;
	if (onsend_rt.rlist[DEFAULT_RT]){
		onsnd_info.to=&dst->to;
		onsnd_info.send_sock=dst->send_sock;
		onsnd_info.buf=buf;
		onsnd_info.len=len;
		p_onsend=&onsnd_info;
		backup_route_type=get_route_type();
		set_route_type(ONSEND_ROUTE);
		if (exec_pre_script_cb(orig_msg, ONSEND_CB_TYPE)>0) {
			/* backup orig_msg send flags */
			fwd_snd_flags_bak=orig_msg->fwd_send_flags;
			rpl_snd_flags_bak=orig_msg->rpl_send_flags;
			orig_msg->fwd_send_flags=dst->send_flags; /* intial value */
			init_run_actions_ctx(&ra_ctx);
			ret=run_actions(&ra_ctx, onsend_rt.rlist[DEFAULT_RT], orig_msg);
			/* update dst send_flags */
			dst->send_flags=orig_msg->fwd_send_flags;
			/* restore orig_msg flags */
			orig_msg->fwd_send_flags=fwd_snd_flags_bak;
			orig_msg->rpl_send_flags=rpl_snd_flags_bak;
			exec_post_script_cb(orig_msg, ONSEND_CB_TYPE);
			if((ret==0) && !(ra_ctx.run_flags&DROP_R_F)){
				ret = 1;
			}
		} else {
			ret=0; /* drop the message */
		}
		set_route_type(backup_route_type);
		p_onsend=0; /* reset it */
	}
	return ret;
}


#endif
