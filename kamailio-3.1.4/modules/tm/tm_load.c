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
 *
 * History:
 * --------
 * 2003-03-06  voicemail changes accepted (jiri)
 * 2003-04-14  t_relay_to split in udp and tcp (jiri)
 */

#include "defs.h"


#include "tm_load.h"
#include "uac.h"

#define LOAD_ERROR "ERROR: tm_bind: TM module function "

int tm_init = 0;

int load_tm( struct tm_binds *tmb)
{
	if (!tm_init) {
		LOG(L_ERR, "tm:load_tm: Module not initialized yet,"
			" make sure that all modules that need"
		    " tm module are loaded after tm in the configuration file\n");
		return -1;
	}

	memset(tmb, 0, sizeof(struct tm_binds));

	/* exported to cfg */
	if (!( tmb->t_newtran=(tnewtran_f)find_export("t_newtran", 0, 0)) ) {
		LOG(L_ERR, LOAD_ERROR "'t_newtran' not found\n");
		return -1;
	}
#ifdef USE_TCP
	if (!( tmb->t_relay_to_tcp=find_export("t_relay_to_tcp", 2, 0)) ) {
		LOG(L_ERR, LOAD_ERROR "'t_relay_to_tcp' not found\n");
		return -1;
	}
#endif
	if (!( tmb->t_relay_to_udp=find_export("t_relay_to_udp", 2, 0)) ) {
		LOG(L_ERR, LOAD_ERROR "'t_relay_to_udp' not found\n");
		return -1;
	}
	if (!( tmb->t_relay=find_export("t_relay", 0, 0)) ) {
		LOG(L_ERR, LOAD_ERROR "'t_relay' not found\n");
		return -1;
	}
	if (!(tmb->t_forward_nonack=(tfwd_f)find_export("t_forward_nonack",2,0))) {
		LOG( L_ERR, LOAD_ERROR "'t_forward_nonack' not found\n");
		return -1;
	}
	if (!(tmb->t_release=(trelease_f)find_export("t_release",0,0))) {
		LOG( L_ERR, LOAD_ERROR "'t_release' not found\n");
		return -1;
	}
/*	if (!(tmb->t_reply=(treply_f)find_export(T_REPLY, 2, 0)) ) {
		LOG( L_ERR, LOAD_ERROR "'t_reply' not found\n");
		return -1;
	}*/

	/* non-cfg API */
	tmb->register_tmcb =register_tmcb;
	tmb->t_reply = w_t_reply_wrp;
	tmb->t_reply_with_body = t_reply_with_body;
	tmb->t_reply_trans = t_reply;
	tmb->t_is_local = t_is_local;
	tmb->t_get_trans_ident = t_get_trans_ident;
	tmb->t_lookup_ident = t_lookup_ident;
	tmb->t_addblind = add_blind_uac;
	tmb->t_request_within = req_within;
	tmb->t_request_outside = req_outside;
	tmb->t_request = request;
	tmb->new_dlg_uac = new_dlg_uac;
	tmb->dlg_response_uac = dlg_response_uac;
	tmb->new_dlg_uas = new_dlg_uas;
	tmb->update_dlg_uas = update_dlg_uas;
	tmb->dlg_request_uas = dlg_request_uas;
	tmb->set_dlg_target = set_dlg_target;
	tmb->free_dlg = free_dlg;
	tmb->print_dlg = print_dlg;
	tmb->t_gett = get_t;
	tmb->calculate_hooks = w_calculate_hooks;
	tmb->t_uac = t_uac;
	tmb->t_uac_with_ids = t_uac_with_ids;
	tmb->t_unref = t_unref;
	tmb->run_failure_handlers = run_failure_handlers;
	tmb->cancel_uacs = cancel_uacs;
	tmb->cancel_all_uacs = cancel_all_uacs;
	tmb->prepare_request_within = prepare_req_within;
	tmb->send_prepared_request = send_prepared_request;
	tmb->dlg_add_extra = dlg_add_extra;
	tmb->t_cancel_uac = t_uac_cancel;

#ifdef DIALOG_CALLBACKS
	tmb->register_new_dlg_cb=register_new_dlg_cb;
	tmb->register_dlg_tmcb=register_dlg_tmcb;
#endif
#ifdef WITH_AS_SUPPORT
	tmb->ack_local_uac = ack_local_uac;
	tmb->t_get_canceled_ident = t_get_canceled_ident;
#endif
	tmb->t_suspend = t_suspend;
	tmb->t_continue = t_continue;
	tmb->t_cancel_suspend = t_cancel_suspend;
	tmb->t_get_reply_totag = t_get_reply_totag;
	tmb->t_get_picked_branch = t_get_picked_branch;
	tmb->t_lookup_callid = t_lookup_callid;
	tmb->generate_callid = generate_callid;
	tmb->generate_fromtag = generate_fromtag;
	tmb->t_lookup_request = t_lookup_request;
	tmb->t_lookup_original = t_lookupOriginalT;
	tmb->t_check = t_check;
	tmb->unref_cell = unref_cell;
	tmb->prepare_to_cancel = prepare_to_cancel;
	tmb->get_stats = tm_get_stats;
	tmb->get_table = tm_get_table;

#ifdef WITH_TM_CTX
	tmb->tm_ctx_get = tm_ctx_get;
#endif
	return 1;
}
