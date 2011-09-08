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
 *
 * History:
 * --------
 * 2003-03-06  voicemail changes accepted
 * 2009-07-14  renamed which_cancel* to prepare_to_cancel* (andrei)
 *
 */


#ifndef _TM_BIND_H
#define _TM_BIND_H

#include "defs.h"


#include "../../sr_module.h"
#include "t_hooks.h"
#include "uac.h"
#include "t_fwd.h"
#include "t_lookup.h"
#include "t_reply.h"
#include "dlg.h"
#include "callid.h"
#include "t_cancel.h"
#include "t_suspend.h"
#include "t_stats.h"

/* export not usable from scripts */
#define NO_SCRIPT	-1

struct tm_binds {
	register_tmcb_f  register_tmcb;
	cmd_function     t_relay_to_udp; /* WARNING: failure_route unsafe */
	cmd_function     t_relay_to_tcp; /* WARNING: failure_route unsafe */ 
	cmd_function     t_relay;        /* WARNING: failure_route unsafe */
	tnewtran_f       t_newtran;
	treply_f         t_reply;
	treply_wb_f      t_reply_with_body;
	tislocal_f       t_is_local;
	tget_ti_f        t_get_trans_ident;
	tlookup_ident_f  t_lookup_ident;
	taddblind_f      t_addblind;
	treply_f         t_reply_unsafe;
	treply_trans_f   t_reply_trans;
	tfwd_f           t_forward_nonack;
	reqwith_t        t_request_within;
	reqout_t         t_request_outside;
	req_t            t_request;
	new_dlg_uac_f      new_dlg_uac;
	dlg_response_uac_f dlg_response_uac;
	new_dlg_uas_f      new_dlg_uas;
	update_dlg_uas_f   update_dlg_uas;
	dlg_request_uas_f  dlg_request_uas;
	set_dlg_target_f   set_dlg_target;
	free_dlg_f         free_dlg;
	print_dlg_f        print_dlg;
	tgett_f            t_gett;
	calculate_hooks_f  calculate_hooks;
	t_uac_t            t_uac;
	t_uac_with_ids_t   t_uac_with_ids;
	trelease_f         t_release;
	tunref_f           t_unref;
	run_failure_handlers_f run_failure_handlers;
	cancel_uacs_f      cancel_uacs;
	cancel_all_uacs_f  cancel_all_uacs;
	prepare_request_within_f  prepare_request_within;
	send_prepared_request_f   send_prepared_request;
	enum route_mode*   route_mode;
#ifdef DIALOG_CALLBACKS
	register_new_dlg_cb_f register_new_dlg_cb;
	register_dlg_tmcb_f   register_dlg_tmcb;
#else
	void* reserved1; /* make sure the structure has the same size even
	                    if no dlg callbacks are used/defined*/
	void* reserved2;
#endif
#ifdef WITH_AS_SUPPORT
	ack_local_uac_f           ack_local_uac;
	t_get_canceled_ident_f    t_get_canceled_ident;
#else
	void* reserved3;
	void* reserved4;
#endif
	t_suspend_f	t_suspend;
	t_continue_f	t_continue;
	t_cancel_suspend_f	t_cancel_suspend;
	tget_reply_totag_f t_get_reply_totag;
	tget_picked_f t_get_picked_branch;
	tlookup_callid_f t_lookup_callid;
	generate_callid_f generate_callid;
	generate_fromtag_f generate_fromtag;
	tlookup_request_f t_lookup_request;
	tlookup_original_f t_lookup_original;
	tcheck_f t_check;
	unref_cell_f unref_cell;
	prepare_to_cancel_f prepare_to_cancel;
	tm_get_stats_f get_stats;
	tm_get_table_f get_table;
	dlg_add_extra_f dlg_add_extra;
	tuaccancel_f    t_cancel_uac;
#ifdef WITH_TM_CTX
	tm_ctx_get_f tm_ctx_get;
#else
	void* reserved5;
#endif
};

extern int tm_init;

typedef int(*load_tm_f)( struct tm_binds *tmb );
int load_tm( struct tm_binds *tmb);


static inline int load_tm_api(struct tm_binds* tmb)
{
	load_tm_f load_tm;

	/* import the TM auto-loading function */
	load_tm = (load_tm_f)find_export("load_tm", NO_SCRIPT, 0);
	
	if (load_tm == NULL) {
		LOG(L_WARN, "Cannot import load_tm function from tm module\n");
		return -1;
	}
	
	/* let the auto-loading function load all TM stuff */
	if (load_tm(tmb) == -1) {
		return -1;
	}
	return 0;
}

#endif
