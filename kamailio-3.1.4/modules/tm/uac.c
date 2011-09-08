/*
 * $Id$
 *
 * simple UAC for things such as SUBSCRIBE or SMS gateway;
 * no authentication and other UAC features -- just send
 * a message, retransmit and await a reply; forking is not
 * supported during client generation, in all other places
 * it is -- adding it should be simple
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
 *  2003-01-23  t_uac_dlg now uses get_out_socket (jiri)
 *  2003-01-27  fifo:t_uac_dlg completed (jiri)
 *  2003-01-29  scratchpad removed (jiri)
 *  2003-02-13  t_uac, t _uac_dlg, gethfblock, uri2proxy changed to use 
 *               proto & rb->dst (andrei)
 *  2003-02-27  FIFO/UAC now dumps reply -- good for CTD (jiri)
 *  2003-02-28  scratchpad compatibility abandoned (jiri)
 *  2003-03-01  kr set through a function now (jiri)
 *  2003-03-19  replaced all mallocs/frees w/ pkg_malloc/pkg_free (andrei)
 *  2003-04-02  port_no_str does not contain a leading ':' anymore (andrei)
 *  2003-07-08  appropriate log messages in check_params(...), 
 *               call calculate_hooks if next_hop==NULL in t_uac (dcm) 
 *  2003-10-24  updated to the new socket_info lists (andrei)
 *  2003-12-03  completion filed removed from transaction and uac callbacks
 *              merged in transaction callbacks as LOCAL_COMPLETED (bogdan)
 *  2004-02-11  FIFO/CANCEL + alignments (hash=f(callid,cseq)) (uli+jiri)
 *  2004-02-13  t->is_invite, t->local, t->noisy_ctimer replaced (bogdan)
 *  2004-08-23  avp support in t_uac (bogdan)
 *  2005-12-16  t_uac will set the new_cell timers to the default values,
 *               fixes 0 fr_timer bug (andrei)
 *  2006-08-11  t_uac uses dns failover until it finds a send socket (andrei)
 *  2007-03-15  TMCB_ONSEND callbacks support added (andrei)
 *  2007-03-23  TMCB_LOCAL_REQUEST_IN callbacks support (andrei)
 *  2007-04-23  per dialog callbacks support (andrei)
 *  2007-06-01  support for per transaction different retransmissions intervals
 *              (andrei)
 */

#include <string.h>
#include "../../mem/shm_mem.h"
#include "../../dprint.h"
#include "../../globals.h"
#include "../../md5.h"
#include "../../crc.h"
#include "../../ip_addr.h"
#include "../../socket_info.h"
#include "../../compiler_opt.h"
#include "config.h"
#include "ut.h"
#include "h_table.h"
#include "t_hooks.h"
#include "t_funcs.h"
#include "t_msgbuilder.h"
#include "callid.h"
#include "uac.h"
#include "t_stats.h"
#ifdef USE_DNS_FAILOVER
#include "../../dns_cache.h"
#include "../../cfg_core.h" /* cfg_get(core, core_cfg, use_dns_failover) */
#endif
#ifdef WITH_EVENT_LOCAL_REQUEST
#include "../../receive.h"
#include "../../route.h"
#include "../../action.h"
#include "t_lookup.h"
#endif

#define FROM_TAG_LEN (MD5_LEN + 1 /* - */ + CRC16_LEN) /* length of FROM tags */

#ifdef WITH_EVENT_LOCAL_REQUEST
/* where to go for the local request route ("tm:local-request") */
int goto_on_local_req=-1; /* default disabled */
#endif /* WITH_EVEN_LOCAL_REQuEST */

static char from_tag[FROM_TAG_LEN + 1];

/*
 * Initialize UAC
 */
int uac_init(void) 
{
	str src[3];
	struct socket_info *si;

	if (RAND_MAX < TABLE_ENTRIES) {
		LOG(L_WARN, "Warning: uac does not spread "
		    "across the whole hash table\n");
	}
	/* on tcp/tls bind_address is 0 so try to get the first address we listen
	 * on no matter the protocol */
	si=bind_address?bind_address:get_first_socket();
	if (si==0){
		LOG(L_CRIT, "BUG: uac_init: null socket list\n");
		return -1;
	}

	/* calculate the initial From tag */
	src[0].s = "Long live SER server";
	src[0].len = strlen(src[0].s);
	src[1].s = si->address_str.s;
	src[1].len = strlen(src[1].s);
	src[2].s = si->port_no_str.s;
	src[2].len = strlen(src[2].s);

	MD5StringArray(from_tag, src, 3);
	from_tag[MD5_LEN] = '-';
	return 1;
}


/*
 * Generate a From tag
 */
void generate_fromtag(str* tag, str* callid)
{
	     /* calculate from tag from callid */
	crcitt_string_array(&from_tag[MD5_LEN + 1], callid, 1);
	tag->s = from_tag; 
	tag->len = FROM_TAG_LEN;
}


/*
 * Check value of parameters
 */
static inline int check_params(uac_req_t *uac_r, str* to, str* from)
{
	if (!uac_r || !uac_r->method || !to || !from) {
		LOG(L_ERR, "check_params(): Invalid parameter value\n");
		return -1;
	}

	if (!uac_r->method->s || !uac_r->method->len) {
		LOG(L_ERR, "check_params(): Invalid request method\n");
		return -2;
	}

	if (!to->s || !to->len) {
		LOG(L_ERR, "check_params(): Invalid To URI\n");
		return -4;
	}

	if (!from->s || !from->len) {
		LOG(L_ERR, "check_params(): Invalid From URI\n");
		return -5;
	}
	return 0;
}

static inline unsigned int dlg2hash( dlg_t* dlg )
{
	str cseq_nr;
	unsigned int hashid;

	cseq_nr.s=int2str(dlg->loc_seq.value, &cseq_nr.len);
	hashid=hash(dlg->id.call_id, cseq_nr);
	DBG("DEBUG: dlg2hash: %d\n", hashid);
	return hashid;
}


/* WARNING: - dst_cell contains the created cell, but it is un-referenced
 *            (before using it make sure you REF() it first)
 *          - if  ACK (method==ACK), a cell will be created but it will not
 *            be added in the hash table (should be either deleted by the 
 *            caller) 
 */
static inline int t_uac_prepare(uac_req_t *uac_r, 
		struct retr_buf **dst_req,
		struct cell **dst_cell)
{
	struct dest_info dst;
	struct cell *new_cell;
	struct retr_buf *request;
	char* buf;
	int buf_len, ret;
	unsigned int hi;
	int is_ack;
	ticks_t lifetime;
#ifdef USE_DNS_FAILOVER
	struct dns_srv_handle dns_h;
#endif
	long nhtype;
#ifdef WITH_EVENT_LOCAL_REQUEST
	struct cell *backup_t;
	int backup_branch;
	unsigned int backup_msgid;
	static struct sip_msg lreq;
	char *buf1;
	int buf_len1;
	int sflag_bk;
	int backup_route_type;
#endif
	snd_flags_t snd_flags;

	ret=-1;
	hi=0; /* make gcc happy */
	/*if (dst_req) *dst_req = NULL;*/
	is_ack = (((uac_r->method->len == 3) && (memcmp("ACK", uac_r->method->s, 3)==0)) ? 1 : 0);
	
	/*** added by dcm 
	 * - needed by external ua to send a request within a dlg
	 */
	if ((nhtype = w_calculate_hooks(uac_r->dialog)) < 0)
		/* if err's returned, the message is incorrect */
		goto error2;

	if (!uac_r->dialog->loc_seq.is_set) {
		/* this is the first request in the dialog,
		set cseq to default value now - Miklos */
		uac_r->dialog->loc_seq.value = DEFAULT_CSEQ;
		uac_r->dialog->loc_seq.is_set = 1;
	}

	DBG("DEBUG:tm:t_uac: next_hop=<%.*s>\n",uac_r->dialog->hooks.next_hop->len,
			uac_r->dialog->hooks.next_hop->s);
	/* new message => take the dialog send_socket if set, or the default
	  send_socket if not*/
	SND_FLAGS_INIT(&snd_flags);
#ifdef USE_DNS_FAILOVER
	if (cfg_get(core, core_cfg, use_dns_failover)){
		dns_srv_handle_init(&dns_h);
		if ((uri2dst2(&dns_h, &dst, uac_r->dialog->send_sock, snd_flags,
							uac_r->dialog->hooks.next_hop, PROTO_NONE)==0)
				|| (dst.send_sock==0)){
			dns_srv_handle_put(&dns_h);
			ser_error = E_NO_SOCKET;
			ret=ser_error;
			LOG(L_ERR, "t_uac: no socket found\n");
			goto error2;
		}
		dns_srv_handle_put(&dns_h); /* not needed anymore */
	}else{
		if ((uri2dst2(0, &dst, uac_r->dialog->send_sock, snd_flags,
						uac_r->dialog->hooks.next_hop, PROTO_NONE)==0) ||
				(dst.send_sock==0)){
			ser_error = E_NO_SOCKET;
			ret=ser_error;
			LOG(L_ERR, "t_uac: no socket found\n");
			goto error2;
		}
	}
#else /* USE_DNS_FAILOVER */
	if ((uri2dst2(&dst, uac_r->dialog->send_sock, snd_flags,
					uac_r->dialog->hooks.next_hop, PROTO_NONE)==0) ||
			(dst.send_sock==0)){
		ser_error = E_NO_SOCKET;
		ret=ser_error;
		LOG(L_ERR, "t_uac: no socket found\n");
		goto error2;
	}
#endif /* USE_DNS_FAILOVER */

	new_cell = build_cell(0); 
	if (!new_cell) {
		ret=E_OUT_OF_MEM;
		LOG(L_ERR, "t_uac: short of cell shmem\n");
		goto error2;
	}
	if (uac_r->method->len==INVITE_LEN && memcmp(uac_r->method->s, INVITE, INVITE_LEN)==0){
		new_cell->flags |= T_IS_INVITE_FLAG;
		new_cell->flags|=T_AUTO_INV_100 &
				(!cfg_get(tm, tm_cfg, tm_auto_inv_100) -1);
#ifdef WITH_AS_SUPPORT
		if (uac_r->cb_flags & TMCB_DONT_ACK)
			new_cell->flags |= T_NO_AUTO_ACK;
#endif
		lifetime=cfg_get(tm, tm_cfg, tm_max_inv_lifetime);
	}else
		lifetime=cfg_get(tm, tm_cfg, tm_max_noninv_lifetime);
	new_cell->flags |= T_IS_LOCAL_FLAG;
	/* init timers hack, new_cell->fr_timer and new_cell->fr_inv_timer
	 * must be set, or else the fr will happen immediately
	 * we can't call init_new_t() because we don't have a sip msg
	 * => we'll ignore t_set_fr() or avp timer value and will use directly the
	 * module params fr_inv_timer and fr_timer -- andrei */
	new_cell->fr_timeout=cfg_get(tm, tm_cfg, fr_timeout);
	new_cell->fr_inv_timeout=cfg_get(tm, tm_cfg, fr_inv_timeout);
	new_cell->end_of_life=get_ticks_raw()+lifetime;
#ifdef TM_DIFF_RT_TIMEOUT
	/* same as above for retransmission intervals */
	new_cell->rt_t1_timeout=cfg_get(tm, tm_cfg, rt_t1_timeout);
	new_cell->rt_t2_timeout=cfg_get(tm, tm_cfg, rt_t2_timeout);
#endif

	set_kr(REQ_FWDED);

	request = &new_cell->uac[0].request;
	request->dst = dst;
	request->flags |= nhtype;

	if (!is_ack) {
#ifdef TM_DEL_UNREF
		INIT_REF(new_cell, 1); /* ref'ed only from the hash */
#endif
		hi=dlg2hash(uac_r->dialog);
		LOCK_HASH(hi);
		insert_into_hash_table_unsafe(new_cell, hi);
		UNLOCK_HASH(hi);
	}

	buf = build_uac_req(uac_r->method, uac_r->headers, uac_r->body, uac_r->dialog, 0, new_cell,
		&buf_len, &dst);
	if (!buf) {
		LOG(L_ERR, "t_uac: Error while building message\n");
		ret=E_OUT_OF_MEM;
		goto error1;
	}

#ifdef WITH_EVENT_LOCAL_REQUEST
	if (unlikely(goto_on_local_req>=0)) {
		DBG("executing event_route[tm:local-request]\n");
		if(likely(build_sip_msg_from_buf(&lreq, buf, buf_len, inc_msg_no())
					== 0)) {
			/* fill some field in sip_msg */
			if (unlikely(set_dst_uri(&lreq, uac_r->dialog->hooks.next_hop))) {
				LM_ERR("failed to set dst_uri");
				free_sip_msg(&lreq);
			} else {
				lreq.force_send_socket = uac_r->dialog->send_sock;
				lreq.rcv.proto = dst.send_sock->proto;
				lreq.rcv.src_ip = dst.send_sock->address;
				lreq.rcv.src_port = dst.send_sock->port_no;
				lreq.rcv.dst_port = su_getport(&dst.to);
				su2ip_addr(&lreq.rcv.dst_ip, &dst.to);
				lreq.rcv.src_su=dst.send_sock->su;
				lreq.rcv.bind_address=dst.send_sock;
			#ifdef USE_COMP
				lreq.rcv.comp=dst.comp;
			#endif /* USE_COMP */
				/* AVPs are reset anyway afterwards, so no need to 
				   backup/restore them*/
				sflag_bk = getsflags();

				/* run the route */
				backup_route_type = get_route_type();
				set_route_type(LOCAL_ROUTE);
				/* set T to the current transaction */
				backup_t=get_t();
				backup_branch=get_t_branch();
				backup_msgid=global_msg_id;
				/* fake transaction and message id */
				global_msg_id=lreq.id;
				set_t(new_cell, T_BR_UNDEFINED);
				run_top_route(event_rt.rlist[goto_on_local_req], &lreq, 0);
				/* restore original environment */
				set_t(backup_t, backup_branch);
				global_msg_id=backup_msgid;
				set_route_type( backup_route_type );

				/* restore original environment */
				setsflagsval(sflag_bk);

				if (unlikely(lreq.new_uri.s))
				{
					pkg_free(lreq.new_uri.s);
					lreq.new_uri.s=0;
					lreq.new_uri.len=0;
				}
				if (unlikely(lreq.dst_uri.s))
				{
					pkg_free(lreq.dst_uri.s);
					lreq.dst_uri.s=0;
					lreq.dst_uri.len=0;
				}

				if (unlikely(lreq.add_rm || lreq.body_lumps)) {
					LM_DBG("apply new updates to sip msg\n");
					buf1 = build_req_buf_from_sip_req(&lreq,
							(unsigned int*)&buf_len1,
							&dst, BUILD_NO_LOCAL_VIA|BUILD_NO_VIA1_UPDATE|
							BUILD_IN_SHM);
					if (likely(buf1)){
						shm_free(buf);
						buf = buf1;
						buf_len = buf_len1;
						/* a possible change of the method is not handled! */
					}
				}
				lreq.buf=0; /* covers the obsolete DYN_BUF */
				free_sip_msg(&lreq);
			}
		}
	}
#endif

	/* better reset avp list now - anyhow, it's useless from
	 * this point (bogdan) */
	reset_avps();
#ifdef WITH_XAVP
	xavp_reset_list();
#endif

	new_cell->method.s = buf;
	new_cell->method.len = uac_r->method->len;

	request->buffer = buf;
	request->buffer_len = buf_len;
	new_cell->nr_of_outgoings++;

	/* Register the callbacks after everything is successful and nothing can fail.
	Otherwise the callback parameter would be freed twise, once from TMCB_DESTROY,
	and again because of the negative return code. */
	if(uac_r->cb && insert_tmcb(&(new_cell->tmcb_hl), uac_r->cb_flags, 
								*(uac_r->cb), uac_r->cbp, NULL)!=1){
		ret=E_OUT_OF_MEM; 
		LOG(L_ERR, "t_uac: short of tmcb shmem\n");
		goto error1;
	}
	if (has_local_reqin_tmcbs())
			run_local_reqin_callbacks(new_cell, 0, 0);
#ifdef DIALOG_CALLBACKS
	run_trans_dlg_callbacks(uac_r->dialog, new_cell, request);
#endif /* DIALOG_CALLBACKS */
	if (dst_req) *dst_req = request;
	if (dst_cell) *dst_cell = new_cell;
	else if(is_ack && dst_req==0){
		free_cell(new_cell);
	}
	
	return 1;

 error1:
 	if (!is_ack) {
		LOCK_HASH(hi);
		remove_from_hash_table_unsafe(new_cell);
		UNLOCK_HASH(hi);
#ifdef TM_DEL_UNREF
		UNREF_FREE(new_cell);
	}else
#else
	}
#endif
		free_cell(new_cell);
error2:
	return ret;
}

/*
 * Prepare a message within a dialog
 */
int prepare_req_within(uac_req_t *uac_r,
		struct retr_buf **dst_req)
{
	if (!uac_r || !uac_r->method || !uac_r->dialog) {
		LOG(L_ERR, "req_within: Invalid parameter value\n");
		goto err;
	}

	if (uac_r->dialog->state != DLG_CONFIRMED) {
		LOG(L_ERR, "req_within: Dialog is not confirmed yet\n");
		goto err;
	}

	if ((uac_r->method->len == 3) && (!memcmp("ACK", uac_r->method->s, 3))) goto send;
	if ((uac_r->method->len == 6) && (!memcmp("CANCEL", uac_r->method->s, 6))) goto send;
	uac_r->dialog->loc_seq.value++; /* Increment CSeq */
 send:
	return t_uac_prepare(uac_r, dst_req, 0);

 err:
	/* if (cbp) shm_free(cbp); */
	/* !! never free cbp here because if t_uac_prepare fails, cbp is not freed
	 * and thus caller has no chance to discover if it is freed or not !! */
	return -1;
}

static inline void send_prepared_request_impl(struct retr_buf *request, int retransmit)
{
	if (SEND_BUFFER(request) == -1) {
		LOG(L_ERR, "t_uac: Attempt to send to precreated request failed\n");
	}
#ifdef TMCB_ONSEND
	else if (unlikely(has_tran_tmcbs(request->my_T, TMCB_REQUEST_SENT)))
		/* we don't know the method here */
			run_onsend_callbacks(TMCB_REQUEST_SENT, request, 0, 0,
									TMCB_LOCAL_F);
#endif
	
	if (retransmit && (start_retr(request)!=0))
		LOG(L_CRIT, "BUG: t_uac: failed to start retr. for %p\n", request);
}

void send_prepared_request(struct retr_buf *request)
{
	send_prepared_request_impl(request, 1 /* retransmit */);
}

/*
 * Send a request using data from the dialog structure
 */
int t_uac(uac_req_t *uac_r)
{
	return t_uac_with_ids(uac_r, NULL, NULL);
}

/*
 * Send a request using data from the dialog structure
 * ret_index and ret_label will identify the new cell
 */
int t_uac_with_ids(uac_req_t *uac_r,
	unsigned int *ret_index, unsigned int *ret_label)
{
	struct retr_buf *request;
	struct cell *cell;
	int ret;
	int is_ack;

	ret = t_uac_prepare(uac_r, &request, &cell);
	if (ret < 0) return ret;
	is_ack = (uac_r->method->len == 3) && (memcmp("ACK", uac_r->method->s, 3)==0) ? 1 : 0;
	send_prepared_request_impl(request, !is_ack /* retransmit */);
	if (is_ack) {
		if (cell) free_cell(cell);
		if (ret_index && ret_label)
			*ret_index = *ret_label = 0;
	} else {
		if (ret_index && ret_label) {
			*ret_index = cell->hash_index;
			*ret_label = cell->label;
		}
	}
	return ret;
}

#ifdef WITH_AS_SUPPORT
struct retr_buf *local_ack_rb(sip_msg_t *rpl_2xx, struct cell *trans,
					unsigned int branch, str *hdrs, str *body)
{
	struct retr_buf *lack;
	unsigned int buf_len;
	char *buffer;
	struct dest_info dst;

	buf_len = (unsigned)sizeof(struct retr_buf);
	if (! (buffer = build_dlg_ack(rpl_2xx, trans, branch, hdrs, body, 
			&buf_len, &dst))) {
		return 0;
	} else {
		/* 'buffer' now points into a contiguous chunk of memory with enough
		 * room to hold both the retr. buffer and the string raw buffer: it
		 * points to the begining of the string buffer; we iterate back to get
		 * the begining of the space for the retr. buffer. */
		lack = &((struct retr_buf *)buffer)[-1];
		lack->buffer = buffer;
		lack->buffer_len = buf_len;
		lack->dst = dst;
	}

	/* TODO: need next 2? */
	lack->activ_type = TYPE_LOCAL_ACK;
	lack->my_T = trans;

	return lack;
}

void free_local_ack(struct retr_buf *lack)
{
	shm_free(lack);
}

void free_local_ack_unsafe(struct retr_buf *lack)
{
	shm_free_unsafe(lack);
}

/**
 * @return: 
 * 	0: success
 * 	-1: internal error
 * 	-2: insane call :)
 */
int ack_local_uac(struct cell *trans, str *hdrs, str *body)
{
	struct retr_buf *local_ack, *old_lack;
	int ret;
#ifdef	TMCB_ONSEND
	struct tmcb_params onsend_params;
#endif

	/* sanity checks */

#ifdef EXTRA_DEBUG
	if (! trans) {
		BUG("no transaction to ACK.\n");
		abort();
	}
#endif

#define RET_INVALID \
		ret = -2; \
		goto fin

	if (! is_local(trans)) {
		ERR("trying to ACK non local transaction (T@%p).\n", trans);
		RET_INVALID;
	}
	if (! is_invite(trans)) {
		ERR("trying to ACK non INVITE local transaction (T@%p).\n", trans);
		RET_INVALID;
	}
	if (! trans->uac[0].reply) {
		ERR("trying to ACK un-completed INVITE transaction (T@%p).\n", trans);
		RET_INVALID;
	}

	if (! (trans->flags & T_NO_AUTO_ACK)) {
		ERR("trying to ACK an auto-ACK transaction (T@%p).\n", trans);
		RET_INVALID;
	}
	if (trans->uac[0].local_ack) {
		ERR("trying to rebuild ACK retransmission buffer (T@%p).\n", trans);
		RET_INVALID;
	}

	/* looks sane: build the retransmission buffer */

	if (! (local_ack = local_ack_rb(trans->uac[0].reply, trans, /*branch*/0, 
			hdrs, body))) {
		ERR("failed to build ACK retransmission buffer");
		RET_INVALID;
	} else {
		/* set the new buffer, but only if not already set (conc. invok.) */
		if ((old_lack = (struct retr_buf *)atomic_cmpxchg_long(
				(void *)&trans->uac[0].local_ack, 0, (long)local_ack))) {
			/* buffer already set: deny current attempt */
			ERR("concurrent ACKing for local INVITE detected (T@%p).\n",trans);
			free_local_ack(local_ack);
			RET_INVALID;
		}
	}

	if (msg_send(&local_ack->dst, local_ack->buffer, local_ack->buffer_len)<0){
		/* hopefully will succeed on next 2xx retransmission */
		ERR("failed to send local ACK (T@%p).\n", trans);
		ret = -1;
		goto fin;
	}
#ifdef	TMCB_ONSEND
	else {
		INIT_TMCB_ONSEND_PARAMS(onsend_params, 0, 0, &trans->uac[0].request,
								&local_ack->dst,
								local_ack->buffer, local_ack->buffer_len,
								TMCB_LOCAL_F, 0 /* branch */, TYPE_LOCAL_ACK);
		run_onsend_callbacks2(TMCB_REQUEST_SENT, trans, &onsend_params);
	}
#endif

	ret = 0;
fin:
	/* TODO: ugly! */
	/* FIXME: the T had been obtain by t_lookup_ident()'ing for it, so, it is
	 * ref-counted. The t_unref() can not be used, as it requests a valid SIP
	 * message (all available might be the reply, but if AS goes wrong and
	 * tries to ACK before the final reply is received, we still have to
	 * lookup the T to find this out). */
	UNREF( trans );
	return ret;

#undef RET_INVALID
}
#endif /* WITH_AS_SUPPORT */


/*
 * Send a message within a dialog
 */
int req_within(uac_req_t *uac_r)
{
	if (!uac_r || !uac_r->method || !uac_r->dialog) {
		LOG(L_ERR, "req_within: Invalid parameter value\n");
		goto err;
	}

	if ((uac_r->method->len == 3) && (!memcmp("ACK", uac_r->method->s, 3))) goto send;
	if ((uac_r->method->len == 6) && (!memcmp("CANCEL", uac_r->method->s, 6))) goto send;
	uac_r->dialog->loc_seq.value++; /* Increment CSeq */
 send:
	return t_uac(uac_r);

 err:
	/* callback parameter must be freed outside of tm module
	if (cbp) shm_free(cbp); */
	return -1;
}


/*
 * Send an initial request that will start a dialog
 * WARNING: writes uac_r->dialog
 */
int req_outside(uac_req_t *uac_r, str* to, str* from)
{
	str callid, fromtag;

	if (check_params(uac_r, to, from) < 0) goto err;
	
	generate_callid(&callid);
	generate_fromtag(&fromtag, &callid);

	if (new_dlg_uac(&callid, &fromtag, DEFAULT_CSEQ, from, to, &uac_r->dialog) < 0) {
		LOG(L_ERR, "req_outside(): Error while creating new dialog\n");
		goto err;
	}

	return t_uac(uac_r);

 err:
	/* callback parameter must be freed outside of tm module
	if (cbp) shm_free(cbp); */
	return -1;
}


/*
 * Send a transactional request, no dialogs involved
 * WARNING: writes uac_r->dialog
 */
int request(uac_req_t *uac_r, str* ruri, str* to, str* from, str *next_hop)
{
	str callid, fromtag;
	dlg_t* dialog;
	int res;

	if (check_params(uac_r, to, from) < 0) goto err;

	generate_callid(&callid);
	generate_fromtag(&fromtag, &callid);

	if (new_dlg_uac(&callid, &fromtag, DEFAULT_CSEQ, from, to, &dialog) < 0) {
		LOG(L_ERR, "request(): Error while creating temporary dialog\n");
		goto err;
	}

	if (ruri) {
		dialog->rem_target.s = ruri->s;
		dialog->rem_target.len = ruri->len;
		/* hooks will be set from w_calculate_hooks */
	}

	if (next_hop) dialog->dst_uri = *next_hop;
	w_calculate_hooks(dialog);

	/* WARNING:
	 * to be clean it should be called 
	 *   set_dlg_target(dialog, ruri, next_hop);
	 * which sets both uris if given [but it duplicates them in shm!]
	 *
	 * but in this case the _ruri parameter in set_dlg_target
	 * must be optional (it is needed now) and following hacks
	 *   dialog->rem_target.s = 0;
	 *   dialog->dst_uri.s = 0;
	 * before freeing dialog here must be removed
	 */
	uac_r->dialog = dialog;
	res = t_uac(uac_r);
	dialog->rem_target.s = 0;
	dialog->dst_uri.s = 0;
	free_dlg(dialog);
	uac_r->dialog = 0;
	return res;

 err:
	/* callback parameter must be freed outside of tm module
	if (cp) shm_free(cp); */
	return -1;
}
