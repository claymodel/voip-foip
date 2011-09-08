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



#ifndef _T_REPLY_H
#define _T_REPLY_H

/* CANCEL_REASON_SUPPORT on by default */
#ifndef NO_CANCEL_REASON_SUPPORT
#define CANCEL_REASON_SUPPORT
#endif /* NO_CANCEL_REASON_SUPPORT */

#include "defs.h"
#include "../../rpc.h"
#include "../../tags.h"

#include "h_table.h"


/* reply processing status */
enum rps {
	/* something bad happened */
	RPS_ERROR=0,	
	/* transaction completed but we still accept the reply */
	RPS_PUSHED_AFTER_COMPLETION,
	/* reply discarded */
	RPS_DISCARDED,
	/* reply stored for later processing */
	RPS_STORE,
	/* transaction completed */
	RPS_COMPLETED,
	/* provisional reply not affecting transaction state */
	RPS_PROVISIONAL
};

extern char tm_tags[TOTAG_VALUE_LEN];
extern char *tm_tag_suffix;

extern int goto_on_sl_reply;

extern int failure_reply_mode;

/* has this to-tag been never seen in previous 200/INVs? */
int unmatched_totag(struct cell *t, struct sip_msg *ack);

/* branch bitmap type */
typedef unsigned int branch_bm_t;

#ifdef CANCEL_REASON_SUPPORT

/* reason building blocks (see rfc3326) */
#define REASON_PREFIX "Reason: SIP;cause="
#define REASON_PREFIX_LEN (sizeof(REASON_PREFIX)-1)
#define REASON_TEXT ";text="
#define REASON_TEXT_LEN (sizeof(REASON_TEXT)-1)

#define CANCEL_REAS_UNKNOWN 0
#define CANCEL_REAS_PACKED_HDRS -1
#define CANCEL_REAS_RCVD_CANCEL -2
#define CANCEL_REAS_FINAL_REPLY(x) (x)
#define CANCEL_REAS_MIN CANCEL_REAS_RCVD_CANCEL


/** cancel reason structure.*/
struct cancel_reason {
	short cause; /**< 0 = unknown, -1 =  cancel, > 0 final reply code. */
	union{
		str text; /**< reason text if reason is final reply .*/
		struct sip_msg* e2e_cancel; /**< cancel msg if reason is cancel. */
		str packed_hdrs; /**< complete reason headers. */
	}u;
};

struct cancel_info {
	branch_bm_t cancel_bitmap; /**< cancel branch bitmap */
	struct cancel_reason reason;
};


#define init_cancel_reason(cr) \
	do {\
		(cr)->cause=0; \
		(cr)->u.e2e_cancel=0; \
	} while(0)

#define init_cancel_info(ci) \
	do {\
		(ci)->cancel_bitmap=0; \
		init_cancel_reason(&(ci)->reason); \
	}while (0);

#else /* ! CANCEL_REASON_SUPPORT */

struct cancel_info {
	branch_bm_t cancel_bitmap; /**< cancel branch bitmap */
};

#define init_cancel_info(ci) \
	do {\
		(ci)->cancel_bitmap=0; \
	}while (0);

#endif /* CANCEL_REASON_SUPPORT */


/* reply export types */
typedef int (*treply_f)(struct sip_msg * , unsigned int , char * );
typedef int (*treply_wb_f)( struct cell* trans,
	unsigned int code, str *text, str *body,
	str *new_header, str *to_tag);
typedef int (*treply_trans_f)(struct cell *t, struct sip_msg* p_msg, unsigned int code,
	char * text);

/* wrapper function needed after changes in w_t_reply */
int w_t_reply_wrp(struct sip_msg *m, unsigned int code, char *txt);

typedef int (*tget_reply_totag_f)(struct sip_msg *, str *);
int t_get_reply_totag(struct sip_msg *msg, str *totag);

#define LOCK_REPLIES(_t) lock(&(_t)->reply_mutex )
#define UNLOCK_REPLIES(_t) unlock(&(_t)->reply_mutex )

/* This function is called whenever a reply for our module is received;
 * we need to register this function on module initialization;
 * Returns :   0 - core router stops
 *             1 - core router relay statelessly
 */
int reply_received( struct sip_msg  *p_msg ) ;

/* return 1 if a failure_route processes */
int run_failure_handlers(struct cell *t, struct sip_msg *rpl,
					int code, int extra_flags);
typedef int (*run_failure_handlers_f)(struct cell*, struct sip_msg*, int, int);


/* Retransmits the last sent inbound reply.
 * Returns  -1 - error
 *           1 - OK
 */
int t_retransmit_reply( /* struct sip_msg * */  );


/* send a UAS reply
 * Warning: 'buf' and 'len' should already have been build.
 * returns 1 if everything was OK or -1 for error
 */


int t_reply_with_body(struct cell *trans, unsigned int code,
		str *text, str *body, str *new_header, str *to_tag);


/* send a UAS reply
 * returns 1 if everything was OK or -1 for error
 */
int t_reply( struct cell *t, struct sip_msg * , unsigned int , char * );
/* the same as t_reply, except it does not claim
   REPLY_LOCK -- useful to be called within reply
   processing
*/
int t_reply_unsafe( struct cell *t, struct sip_msg * , unsigned int , char * );


enum rps relay_reply( struct cell *t, struct sip_msg *p_msg, int branch,
	unsigned int msg_status, struct cancel_info *cancel_data,
	int do_put_on_wait );

enum rps local_reply( struct cell *t, struct sip_msg *p_msg, int branch,
	unsigned int msg_status, struct cancel_info *cancel_data );

void set_final_timer( /* struct s_table *h_table,*/ struct cell *t );

void cleanup_uac_timers( struct cell *t );

void on_negative_reply( struct cell* t, struct sip_msg* msg,
	int code, void *param  );

/* set which 'reply' structure to take if only negative
   replies arrive 
*/
void t_on_negative( unsigned int go_to );
unsigned int get_on_negative();
void t_on_reply( unsigned int go_to );
unsigned int get_on_reply();

int t_retransmit_reply( struct cell *t );

void tm_init_tags();

/* selects the branch for fwd-ing the reply */
int t_pick_branch(int inc_branch, int inc_code, struct cell *t, int *res_code);
/* checks the selected branch from failure_route */
int t_pick_branch_blind(struct cell *t, int *res_code);

/* drops all the replies to make sure
 * that none of them is picked up again
 */
void t_drop_replies(int v);

void rpc_reply(rpc_t* rpc, void* c);

void faked_env( struct cell *t,struct sip_msg *msg);
int fake_req(struct sip_msg *faked_req,
		struct sip_msg *shmem_msg, int extra_flags, struct ua_client *uac);

void free_faked_req(struct sip_msg *faked_req, struct cell *t);

typedef int (*tget_picked_f)(void);
int t_get_picked_branch(void);

#endif
