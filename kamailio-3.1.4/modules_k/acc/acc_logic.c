/*
 * $Id$
 * 
 * Accounting module logic
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2006 Voice Sistem SRL
 *
 * This file is part of Kamailio, a free SIP server.
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
 * History:
 * -------
 * 2006-09-19  forked from the acc_mod.c file during a big re-structuring
 *             of acc module (bogdan)
 */

/*! \file
 * \ingroup acc
 * \brief Acc:: Logic
 *
 * - Module: \ref acc
 */

#include <stdio.h>
#include <string.h>

#include "../../dprint.h"
#include "../../parser/parse_from.h"
#include "../../parser/parse_content.h"
#include "../../modules/tm/tm_load.h"
#include "../rr/api.h"
#include "../../lib/kcore/km_ut.h"
#include "../../flags.h"
#include "acc.h"
#include "acc_api.h"
#include "acc_mod.h"
#include "acc_logic.h"

extern struct tm_binds tmb;
extern struct rr_binds rrb;

struct acc_enviroment acc_env;


#define is_acc_flag_set(_rq,_flag)  (((_flag) != -1) && (isflagset((_rq), (_flag)) == 1))
#define reset_acc_flag(_rq,_flag)   (resetflag((_rq), (_flag)))

#define is_failed_acc_on(_rq)  is_acc_flag_set(_rq,failed_transaction_flag)

#define is_log_acc_on(_rq)     is_acc_flag_set(_rq,log_flag)
#define is_log_mc_on(_rq)      is_acc_flag_set(_rq,log_missed_flag)

#ifdef SQL_ACC
	#define is_db_acc_on(_rq)     is_acc_flag_set(_rq,db_flag)
	#define is_db_mc_on(_rq)      is_acc_flag_set(_rq,db_missed_flag)
#else
	#define is_db_acc_on(_rq)     (0)
	#define is_db_mc_on(_rq)      (0)
#endif

#ifdef RAD_ACC
	#define is_rad_acc_on(_rq)     is_acc_flag_set(_rq,radius_flag)
	#define is_rad_mc_on(_rq)      is_acc_flag_set(_rq,radius_missed_flag)
#else
	#define is_rad_acc_on(_rq)     (0)
	#define is_rad_mc_on(_rq)      (0)
#endif


#ifdef DIAM_ACC
	#define is_diam_acc_on(_rq)     is_acc_flag_set(_rq,diameter_flag)
	#define is_diam_mc_on(_rq)      is_acc_flag_set(_rq,diameter_missed_flag)
#else
	#define is_diam_acc_on(_rq)     (0)
	#define is_diam_mc_on(_rq)      (0)
#endif

#define is_acc_on(_rq) \
	( (is_log_acc_on(_rq)) || (is_db_acc_on(_rq)) \
	|| (is_rad_acc_on(_rq)) || (is_diam_acc_on(_rq)) )

#define is_mc_on(_rq) \
	( (is_log_mc_on(_rq)) || (is_db_mc_on(_rq)) \
	|| (is_rad_mc_on(_rq)) || (is_diam_mc_on(_rq)) )

#define skip_cancel(_rq) \
	(((_rq)->REQ_METHOD==METHOD_CANCEL) && report_cancels==0)




static void tmcb_func( struct cell* t, int type, struct tmcb_params *ps );


static inline struct hdr_field* get_rpl_to( struct cell *t,
														struct sip_msg *reply)
{
	if (reply==FAKED_REPLY || !reply || !reply->to)
		return t->uas.request->to;
	else
		return reply->to;
}


static inline void env_set_to(struct hdr_field *to)
{
	acc_env.to = to;
}


static inline void env_set_text(char *p, int len)
{
	acc_env.text.s = p;
	acc_env.text.len = len;
}


static inline void env_set_code_status( int code, struct sip_msg *reply)
{
	static char code_buf[INT2STR_MAX_LEN];

	acc_env.code = code;
	if (reply==FAKED_REPLY || reply==NULL) {
		/* code */
		acc_env.code_s.s =
			int2bstr((unsigned long)code, code_buf, &acc_env.code_s.len);
		/* reason */
		acc_env.reason.s = error_text(code);
		acc_env.reason.len = strlen(acc_env.reason.s);
	} else {
		acc_env.code_s = reply->first_line.u.reply.status;
		acc_env.reason = reply->first_line.u.reply.reason;
	}
}


static inline void env_set_comment(struct acc_param *accp)
{
	acc_env.code = accp->code;
	acc_env.code_s = accp->code_s;
	acc_env.reason = accp->reason;
}


static inline int acc_preparse_req(struct sip_msg *req)
{
	if ( (parse_headers(req,HDR_CALLID_F|HDR_CSEQ_F|HDR_FROM_F|HDR_TO_F,0)<0)
	|| (parse_from_header(req)<0 ) ) {
		LM_ERR("failed to preparse request\n");
		return -1;
	}
	return 0;
}



int w_acc_log_request(struct sip_msg *rq, char *comment, char *foo)
{
	if (acc_preparse_req(rq)<0)
		return -1;
	env_set_to( rq->to );
	env_set_comment((struct acc_param*)comment);
	env_set_text( ACC_REQUEST, ACC_REQUEST_LEN);
	return acc_log_request(rq);
}


#ifdef SQL_ACC
int w_acc_db_request(struct sip_msg *rq, char *comment, char *table)
{
	if (!table) {
		LM_ERR("db support not configured\n");
		return -1;
	}
	if (acc_preparse_req(rq)<0)
		return -1;
	env_set_to( rq->to );
	env_set_comment((struct acc_param*)comment);
	env_set_text(table, strlen(table));
	return acc_db_request(rq);
}
#endif


#ifdef RAD_ACC
int w_acc_rad_request(struct sip_msg *rq, char *comment, char *foo)
{
	if (acc_preparse_req(rq)<0)
		return -1;
	env_set_to( rq->to );
	env_set_comment((struct acc_param*)comment);
	return acc_rad_request(rq);
}
#endif


#ifdef DIAM_ACC
int w_acc_diam_request(struct sip_msg *rq, char *comment, char *foo)
{
	if (acc_preparse_req(rq)<0)
		return -1;
	env_set_to( rq->to );
	env_set_comment((struct acc_param*)comment);
	return acc_diam_request(rq);
}
#endif



/* prepare message and transaction context for later accounting */
void acc_onreq( struct cell* t, int type, struct tmcb_params *ps )
{
	int tmcb_types;
	int is_invite;

	if ( ps->req && !skip_cancel(ps->req) &&
	(is_acc_on(ps->req) || is_mc_on(ps->req)) ) {
		/* do some parsing in advance */
		if (acc_preparse_req(ps->req)<0)
			return;
		is_invite = (ps->req->REQ_METHOD==METHOD_INVITE)?1:0;
		/* install additional handlers */
		tmcb_types =
			/* report on completed transactions */
			TMCB_RESPONSE_OUT |
			/* account e2e acks if configured to do so */
			((report_ack && is_acc_on(ps->req))?TMCB_E2EACK_IN:0) |
			/* get incoming replies ready for processing */
			TMCB_RESPONSE_IN |
			/* report on missed calls */
			((is_invite && is_mc_on(ps->req))?TMCB_ON_FAILURE:0) ;
		if (tmb.register_tmcb( 0, t, tmcb_types, tmcb_func, 0, 0 )<=0) {
			LM_ERR("cannot register additional callbacks\n");
			return;
		}
		/* if required, determine request direction */
		if( detect_direction && !rrb.is_direction(ps->req,RR_FLOW_UPSTREAM) ) {
			LM_DBG("detected an UPSTREAM req -> flaging it\n");
			ps->req->msg_flags |= FL_REQ_UPSTREAM;
		}
	}
}



/* is this reply of interest for accounting ? */
static inline int should_acc_reply(struct sip_msg *req,struct sip_msg *rpl,
																	int code)
{
	/* negative transactions reported otherwise only if explicitly 
	 * demanded */
	if ( !is_failed_acc_on(req) && code >=300 )
		return 0;
	if ( !is_acc_on(req) )
		return 0;
	if ( code<200 && !(early_media &&
	parse_headers(rpl,HDR_CONTENTLENGTH_F, 0)==0 && rpl->content_length &&
	get_content_length(rpl)>0 ) )
		return 0;

	return 1; /* seed is through, we will account this reply */
}



/* parse incoming replies before cloning */
static inline void acc_onreply_in(struct cell *t, struct sip_msg *req,
											struct sip_msg *reply, int code)
{
	/* don't parse replies in which we are not interested */
	/* missed calls enabled ? */
	if ( (reply && reply!=FAKED_REPLY) && (should_acc_reply(req,reply,code)
	|| (is_invite(t) && code>=300 && is_mc_on(req))) ) {
		parse_headers(reply, HDR_TO_F, 0 );
	}
}



/* initiate a report if we previously enabled MC accounting for this t */
static inline void on_missed(struct cell *t, struct sip_msg *req,
											struct sip_msg *reply, int code)
{
	str new_uri_bk = {0, 0};
	int flags_to_reset = 0;

	/* set as new_uri the last branch */
	if (t->relayed_reply_branch>=0) {
		new_uri_bk = req->new_uri;
		req->new_uri = t->uac[t->relayed_reply_branch].uri;
		req->parsed_uri_ok = 0;
	} else {
		new_uri_bk.len = -1;
		new_uri_bk.s = 0;
	}

	/* set env variables */
	env_set_to( get_rpl_to(t,reply) );
	env_set_code_status( code, reply);

	/* we report on missed calls when the first
	 * forwarding attempt fails; we do not wish to
	 * report on every attempt; so we clear the flags; 
	 */

	if (is_log_mc_on(req)) {
		env_set_text( ACC_MISSED, ACC_MISSED_LEN);
		acc_log_request( req );
		flags_to_reset |= log_missed_flag;
	}
#ifdef SQL_ACC
	if (is_db_mc_on(req)) {
		env_set_text(db_table_mc.s, db_table_mc.len);
		acc_db_request( req );
		flags_to_reset |= db_missed_flag;
	}
#endif
#ifdef RAD_ACC
	if (is_rad_mc_on(req)) {
		acc_rad_request( req );
		flags_to_reset |= radius_missed_flag;
	}
#endif
/* DIAMETER */
#ifdef DIAM_ACC
	if (is_diam_mc_on(req)) {
		acc_diam_request( req );
		flags_to_reset |= diameter_missed_flag;
	}
#endif

	/* run extra acc engines */
	acc_run_engines(req, 1, &flags_to_reset);

	/* Reset the accounting missed_flags
	 * These can't be reset in the blocks above, because
	 * it would skip accounting if the flags are identical
	 */
	reset_acc_flag( req, flags_to_reset );

	if (new_uri_bk.len>=0) {
		req->new_uri = new_uri_bk;
		req->parsed_uri_ok = 0;
	}

}



/* initiate a report if we previously enabled accounting for this t */
static inline void acc_onreply( struct cell* t, struct sip_msg *req,
											struct sip_msg *reply, int code)
{
	str new_uri_bk;

	/* acc_onreply is bound to TMCB_REPLY which may be called
	   from _reply, like when FR hits; we should not miss this
	   event for missed calls either */
	if (is_invite(t) && code>=300 && is_mc_on(req) )
		on_missed(t, req, reply, code);

	if (!should_acc_reply(req, reply, code))
		return;

	/* for reply processing, set as new_uri the winning branch */
	if (t->relayed_reply_branch>=0) {
		new_uri_bk = req->new_uri;
		req->new_uri = t->uac[t->relayed_reply_branch].uri;
		req->parsed_uri_ok = 0;
	} else {
		new_uri_bk.len = -1;
		new_uri_bk.s = 0;
	}
	/* set env variables */
	env_set_to( get_rpl_to(t,reply) );
	env_set_code_status( code, reply);

	if ( is_log_acc_on(req) ) {
		env_set_text( ACC_ANSWERED, ACC_ANSWERED_LEN);
		acc_log_request(req);
	}
#ifdef SQL_ACC
	if (is_db_acc_on(req)) {
		env_set_text( db_table_acc.s, db_table_acc.len);
		acc_db_request(req);
	}
#endif
#ifdef RAD_ACC
	if (is_rad_acc_on(req))
		acc_rad_request(req);
#endif
/* DIAMETER */
#ifdef DIAM_ACC
	if (is_diam_acc_on(req))
		acc_diam_request(req);
#endif

	/* run extra acc engines */
	acc_run_engines(req, 0, NULL);

	if (new_uri_bk.len>=0) {
		req->new_uri = new_uri_bk;
		req->parsed_uri_ok = 0;
	}
}



static inline void acc_onack( struct cell* t, struct sip_msg *req,
		struct sip_msg *ack, int code)
{
	if (acc_preparse_req(ack)<0)
		return;

	/* set env variables */
	env_set_to( ack->to?ack->to:req->to );
	env_set_code_status( t->uas.status, 0 );

	if (is_log_acc_on(req)) {
		env_set_text( ACC_ACKED, ACC_ACKED_LEN);
		acc_log_request( ack );
	}
#ifdef SQL_ACC
	if (is_db_acc_on(req)) {
		env_set_text( db_table_acc.s, db_table_acc.len);
		acc_db_request( ack );
	}
#endif
#ifdef RAD_ACC
	if (is_rad_acc_on(req)) {
		acc_rad_request(ack);
	}
#endif
/* DIAMETER */
#ifdef DIAM_ACC
	if (is_diam_acc_on(req)) {
		acc_diam_request(ack);
	}
#endif

	/* run extra acc engines */
	acc_run_engines(req, 0, NULL);
	
}


/**
 * @brief execute an acc event via a specific engine
 */
int acc_api_exec(struct sip_msg *rq, acc_engine_t *eng,
		acc_param_t* comment)
{
	acc_info_t inf;
	if (acc_preparse_req(rq)<0)
		return -1;
	env_set_to(rq->to);
	env_set_comment(comment);
	memset(&inf, 0, sizeof(acc_info_t));
	inf.env  = &acc_env;
	acc_api_set_arrays(&inf);
	return eng->acc_req(rq, &inf);
}


static void tmcb_func( struct cell* t, int type, struct tmcb_params *ps )
{
	if (type&TMCB_RESPONSE_OUT) {
		acc_onreply( t, ps->req, ps->rpl, ps->code);
	} else if (type&TMCB_E2EACK_IN) {
		acc_onack( t, t->uas.request, ps->req, ps->code);
	} else if (type&TMCB_ON_FAILURE) {
		on_missed( t, ps->req, ps->rpl, ps->code);
	} else if (type&TMCB_RESPONSE_IN) {
		acc_onreply_in( t, ps->req, ps->rpl, ps->code);
	}
}

