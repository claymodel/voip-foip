/*
 * $Id$
 *
 * cloning a message into shared memory (TM keeps a snapshot
 * of messages in memory); note that many operations, which
 * allocate pkg memory (such as parsing) cannot be used with
 * a cloned message -- it would result in linking pkg structures
 * to shmem msg and eventually in a memory error
 *
 * the cloned message is stored in a single memory fragment to
 * save too many shm_mallocs -- these are expensive as they
 * not only take lookup in fragment table but also a shmem lock
 * operation (the same for shm_free)
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
 *
 * History:
 * --------
 *  2003-01-23 - msg_cloner clones msg->from->parsed too (janakj)
 *  2003-01-29 - scratchpad removed (jiri)
 *  2003-02-25 - auth_body cloner added (janakj)
 *  2003-02-28  scratchpad compatibility abandoned (jiri)
 *  2003-03-31  removed msg->repl_add_rm (andrei)
 *  2003-04-04  parsed uris are recalculated on cloning (jiri)
 *  2003-05-07  received, rport & i via shortcuts are also translated (andrei)
 *  2003-11-11  updated cloning of lump_rpl (bogdan)
 *  2004-03-31  alias shortcuts are also translated (andrei)
 *  2006-04-20  via->comp is also translated (andrei)
 *  2006-10-16  HDR_{PROXY,WWW}_AUTHENTICATE_T cloned (andrei)
 *  2007-01-26  HDR_DATE_T, HDR_IDENTITY_T, HDR_IDENTITY_INFO_T added (gergo)
 *  2007-09-05  A separate memory block is allocated for the lumps
 *              in case of requests in order to allow cloning them
 *              later than the SIP msg. (Miklos)
 * 2009-07-22  moved most of the functions to core sip_msg_clone.c  (andrei)*/

#include "defs.h"


#include "sip_msg.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../ut.h"
#include "../../sip_msg_clone.h"

#ifdef POSTPONE_MSG_CLONING
#include "../../fix_lumps.h"
#endif

/* Warning: Cloner does not clone all hdr_field headers (From, To, etc.). Pointers will reference pkg memory. Dereferencing will crash ser!!! */

struct sip_msg*  sip_msg_cloner( struct sip_msg *org_msg, int *sip_msg_len )
{
#ifdef POSTPONE_MSG_CLONING
	/* take care of the lumps only for replies if the msg cloning is 
	   postponed */
	if (org_msg->first_line.type==SIP_REPLY)
#endif /* POSTPONE_MSG_CLONING */
		/*cloning all the lumps*/
		return sip_msg_shm_clone(org_msg, sip_msg_len, 1);
#ifdef POSTPONE_MSG_CLONING
	/* don't clone the lumps */
	return sip_msg_shm_clone(org_msg, sip_msg_len, 0);
#endif /* POSTPONE_MSG_CLONING */
}

#ifdef POSTPONE_MSG_CLONING
/* indicates wheter we have already cloned the msg lumps or not */
unsigned char lumps_are_cloned = 0;



/* wrapper function for msg_lump_cloner() with some additional sanity checks */
int save_msg_lumps( struct sip_msg *shm_msg, struct sip_msg *pkg_msg)
{
	int ret;
	struct lump* add_rm;
	struct lump* body_lumps;
	struct lump_rpl* reply_lump;
	
	/* make sure that we do not clone the lumps twice */
	if (lumps_are_cloned) {
		LOG(L_DBG, "DEBUG: save_msg_lumps: lumps have been already cloned\n" );
		return 0;
	}
	/* sanity checks */
	if (unlikely(!shm_msg || ((shm_msg->msg_flags & FL_SHM_CLONE)==0))) {
		LOG(L_ERR, "ERROR: save_msg_lumps: BUG, there is no shmem-ized message"
			" (shm_msg=%p)\n", shm_msg);
		return -1;
	}
	if (unlikely(shm_msg->first_line.type!=SIP_REQUEST)) {
		LOG(L_ERR, "ERROR: save_msg_lumps: BUG, the function should be called only for requests\n" );
		return -1;
	}

#ifdef EXTRA_DEBUG
	membar_depends();
	if (shm_msg->add_rm || shm_msg->body_lumps || shm_msg->reply_lump) {
		LOG(L_ERR, "ERROR: save_msg_lumps: BUG, trying to overwrite the already cloned lumps\n");
		return -1;
	}
#endif

	/* needless to clone the lumps for ACK, they will not be used again */
	if (shm_msg->REQ_METHOD == METHOD_ACK)
		return 0;

	/* clean possible previous added vias/clen header or else they would 
	 * get propagated in the failure routes */
	free_via_clen_lump(&pkg_msg->add_rm);

	lumps_are_cloned = 1;
	ret=msg_lump_cloner(pkg_msg, &add_rm, &body_lumps, &reply_lump);
	if (likely(ret==0)){
		/* make sure the lumps are fully written before adding them to
		   shm_msg (in case someone accesses it in the same time) */
		membar_write();
		shm_msg->add_rm = add_rm;
		shm_msg->body_lumps = body_lumps;
		shm_msg->reply_lump = reply_lump;
	}
	return ret<0?-1:0;
}
#endif /* POSTPONE_MSG_CLONING */
