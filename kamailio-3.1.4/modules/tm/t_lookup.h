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
/*
 * History:
 * --------
 *  2003-02-24  s/T_NULL/T_NULL_CELL/ to avoid redefinition conflict w/
 *               nameser_compat.h (andrei)
 *  2004-02-11  FIFO/CANCEL + alignments (hash=f(callid,cseq)) (uli+jiri)
 *  2005-12-09  added t_set_fr()  (andrei)
 * 2009-06-24  changed set_t() to take also a branch parameter (andrei)
 */



#ifndef _T_LOOKUP_H
#define _T_LOOKUP_H

#include "defs.h"


#include "config.h"
#include "t_funcs.h"

#define T_UNDEFINED  ( (struct cell*) -1 )
#define T_NULL_CELL       ( (struct cell*) 0 )

#define T_BR_UNDEFINED (-1)

extern unsigned int     global_msg_id;



void init_t();
int init_rb( struct retr_buf *rb, struct sip_msg *msg );

typedef struct cell* (*tlookup_original_f)( struct sip_msg* p_msg );
struct cell* t_lookupOriginalT( struct sip_msg* p_msg );

int t_reply_matching( struct sip_msg* , int* );

typedef int (*tlookup_request_f)(struct sip_msg*, int, int*);

int t_lookup_request( struct sip_msg* p_msg, int leave_new_locked,
						int* canceled);
int t_newtran( struct sip_msg* p_msg );

int _add_branch_label( struct cell *trans,
    char *str, int *len, int branch );
int add_branch_label( struct cell *trans, 
	struct sip_msg *p_msg, int branch );

/* releases T-context */
int t_unref( struct sip_msg *p_msg);
typedef int (*tunref_f)( struct sip_msg *p_msg);

typedef int (*tcheck_f)(struct sip_msg*, int*);

/* old t_check version (no e2eack support) */
int t_check(struct sip_msg* , int *branch );
/* new version, e2eack and different return convention */
int t_check_msg(struct sip_msg* , int *branch );

typedef struct cell * (*tgett_f)(void);
struct cell *get_t();
int get_t_branch();

/* use carefully or better not at all -- current transaction is 
 * primarily set by lookup functions */
void set_t(struct cell *t, int branch);


#define T_GET_TI       "t_get_trans_ident"
#define T_LOOKUP_IDENT "t_lookup_ident"
#define T_IS_LOCAL     "t_is_local"

typedef int (*tislocal_f)(struct sip_msg*);
typedef int (*tnewtran_f)(struct sip_msg*);
typedef int (*tget_ti_f)(struct sip_msg*, unsigned int*, unsigned int*);
typedef int (*tlookup_ident_f)(struct cell**, unsigned int, unsigned int);
typedef int (*trelease_f)(struct sip_msg*);
typedef int (*tlookup_callid_f)(struct cell **, str, str);

int t_is_local(struct sip_msg*);
int t_get_trans_ident(struct sip_msg* p_msg, unsigned int* hash_index, unsigned int* label);
int t_lookup_ident(struct cell** trans, unsigned int hash_index, unsigned int label);
/* lookup a transaction by callid and cseq */
int t_lookup_callid(struct cell** trans, str callid, str cseq);

int t_set_fr(struct sip_msg* msg, unsigned int fr_inv_to, unsigned int fr_to );
int t_reset_fr();
#ifdef TM_DIFF_RT_TIMEOUT
int t_set_retr(struct sip_msg* msg, unsigned int t1_to, unsigned int t2_to);
int t_reset_retr();
#endif
int t_set_max_lifetime(struct sip_msg* msg, unsigned int eol_inv,
											unsigned int eol_noninv);
int t_reset_max_lifetime();

#ifdef WITH_AS_SUPPORT
/**
 * Returns the hash coordinates of the transaction current CANCEL is targeting.
 */
int t_get_canceled_ident(struct sip_msg *msg, unsigned int *hash_index, 
		unsigned int *label);
typedef int (*t_get_canceled_ident_f)(struct sip_msg *msg, 
		unsigned int *hash_index, unsigned int *label);
#endif /* WITH_AS_SUPPORT */

/**
 * required by TMX (K/O extensions)
 */
#define WITH_TM_CTX
#ifdef WITH_TM_CTX

typedef struct _tm_ctx {
	int branch_index;
} tm_ctx_t;

typedef tm_ctx_t* (*tm_ctx_get_f)(void);

tm_ctx_t* tm_ctx_get(void);
void tm_ctx_init(void);
void tm_ctx_set_branch_index(int v);

#else

#define tm_ctx_get() NULL
#define tm_ctx_init()
#define tm_ctx_set_branch_index(v)

#endif /* WITH_TM_CTX */

#endif
