/*
 * $Id$
 *
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
/* History:
 * --------
 *
 *  2003-04-12  FORCE_RPORT_T added (andrei)
 *  2003-04-22  strip_tail added (jiri)
 *  2003-10-10  >,<,>=,<=, != and MSGLEN_O added (andrei)
 *  2003-10-28  FORCE_TCP_ALIAS added (andrei)
 *  2004-02-24  added LOAD_AVP_T and AVP_TO_URI_T (bogdan)
 *  2005-12-11  added SND{IP,PORT,PROTO,AF}_O & TO{IP,PORT}_O (andrei)
 *  2005-12-19  select framework added SELECT_O and SELECT_ST (mma)
 *  2008-12-17  added UDP_MTU_TRY_PROTO_T (andrei)
 */


#ifndef route_struct_h
#define route_struct_h

#include <sys/types.h>
#include <regex.h>
#include "select.h"
#include "usr_avp.h"

#define EXPR_DROP -127  /* used only by the expression and if evaluator */
/*
 * Other important values (no macros for them yet):
 * expr true = 1
 * expr false = 0 (used only inside the expression and if evaluator)
 *
 * action continue  or if used in condition true = 1
 * action drop/quit/stop script processing = 0
 * action error or if used in condition false = -1 (<0 and !=EXPR_DROP)
 *
 */


/* expression type */
enum expr_type { EXP_T=1, ELEM_T };
enum expr_op {
	/* expression operator if type==EXP_T */
	LOGAND_OP=1, LOGOR_OP, NOT_OP, BINAND_OP, BINOR_OP,
	/* expression operator if type==ELEM_T */
	EQUAL_OP=10, MATCH_OP, GT_OP, LT_OP, GTE_OP, LTE_OP, DIFF_OP, NO_OP
};
/* expression left member "special" type (if type==ELEM_T)
  (start at 51 for debugging purposes: it's better to not overlap with 
   expr_r_type)
*/
enum _expr_l_type{
	   METHOD_O=51, URI_O, FROM_URI_O, TO_URI_O, SRCIP_O, SRCPORT_O,
	   DSTIP_O, DSTPORT_O, PROTO_O, AF_O, MSGLEN_O, ACTION_O,
	   NUMBER_O, AVP_O, SNDIP_O, SNDPORT_O, TOIP_O, TOPORT_O, SNDPROTO_O,
	   SNDAF_O, RETCODE_O, SELECT_O, PVAR_O, RVEXP_O, SELECT_UNFIXED_O};
/* action types */
enum action_type{
		FORWARD_T=1, SEND_T, DROP_T, LOG_T, ERROR_T, ROUTE_T, EXEC_T,
		SET_HOST_T, SET_HOSTPORT_T, SET_USER_T, SET_USERPASS_T,
		SET_PORT_T, SET_URI_T, SET_HOSTPORTTRANS_T, SET_HOSTALL_T,
		SET_USERPHONE_T,
		IF_T, SWITCH_T /* only until fixup*/,
		BLOCK_T, EVAL_T, SWITCH_JT_T, SWITCH_COND_T, MATCH_COND_T, WHILE_T,
		/* module function calls with string only parameters */
		MODULE0_T, MODULE1_T, MODULE2_T, MODULE3_T, MODULE4_T, MODULE5_T,
		MODULE6_T, MODULEX_T,
		/* module function calls, that have some RVE parameters */
		MODULE1_RVE_T, MODULE2_RVE_T, MODULE3_RVE_T,
		MODULE4_RVE_T, MODULE5_RVE_T, MODULE6_RVE_T, MODULEX_RVE_T,
		SETFLAG_T, RESETFLAG_T, ISFLAGSET_T ,
		AVPFLAG_OPER_T,
		LEN_GT_T, PREFIX_T, STRIP_T,STRIP_TAIL_T,
		APPEND_BRANCH_T,
		REVERT_URI_T,
		FORWARD_TCP_T,
		FORWARD_UDP_T,
		FORWARD_TLS_T,
		FORWARD_SCTP_T,
		SEND_TCP_T,
		FORCE_RPORT_T,
		ADD_LOCAL_RPORT_T,
		SET_ADV_ADDR_T,
		SET_ADV_PORT_T,
		FORCE_TCP_ALIAS_T,
		LOAD_AVP_T,
		AVP_TO_URI_T,
		FORCE_SEND_SOCKET_T,
		ASSIGN_T,
		ADD_T,
		UDP_MTU_TRY_PROTO_T,
		SET_FWD_NO_CONNECT_T,
		SET_RPL_NO_CONNECT_T,
		SET_FWD_CLOSE_T,
		SET_RPL_CLOSE_T
};
/* parameter types for actions or types for expression right operands
   (WARNING right operands only, not working for left operands) */
enum _operand_subtype{
		NOSUBTYPE=0, STRING_ST, NET_ST, NUMBER_ST, IP_ST, RE_ST, PROXY_ST,
		EXPR_ST, ACTIONS_ST, MODEXP_ST, MODFIXUP_ST, URIHOST_ST, URIPORT_ST,
		MYSELF_ST, STR_ST, SOCKID_ST, SOCKETINFO_ST, ACTION_ST, AVP_ST,
		SELECT_ST, PVAR_ST,
		LVAL_ST,  RVE_ST,
		RETCODE_ST, CASE_ST,
		BLOCK_ST, JUMPTABLE_ST, CONDTABLE_ST, MATCH_CONDTABLE_ST,
		SELECT_UNFIXED_ST,
		STRING_RVE_ST /* RVE converted to a string (fparam hack) */,
		RVE_FREE_FIXUP_ST /* (str)RVE fixed up by a reversable fixup */,
		FPARAM_DYN_ST /* temporary only (fparam hack) */
};

typedef enum _expr_l_type expr_l_type;
typedef enum _operand_subtype expr_r_type;
typedef enum _operand_subtype action_param_type;

/* run flags */
#define EXIT_R_F   1
#define RETURN_R_F 2
#define BREAK_R_F  4
#define DROP_R_F   8
#define IGNORE_ON_BREAK_R_F 256


struct cfg_pos{
	int s_line;
	int e_line;
	unsigned short s_col;
	unsigned short e_col;
	char *fname;
};


/* Expression operand */
union exp_op {
	void* param;
	long numval; /* must have the same size as a void*/
	struct expr* expr;
	char* string;
	avp_spec_t* attr;
	select_t* select;
	regex_t* re;
	struct net* net;
	struct _str str;
};

struct expr{
	enum expr_type type; /* exp, exp_elem */
	enum expr_op op; /* and, or, not | ==,  =~ */
	expr_l_type l_type;
	expr_r_type r_type;
	union exp_op l;
	union exp_op r;
};

typedef struct {
	action_param_type type;
	union {
		long number;
		char* string;
		struct _str str;
		void* data;
		avp_spec_t* attr;
		select_t* select;
	} u;
} action_u_t;

/* maximum internal array/params
 * for module function calls val[0] and val[1] store a pointer to the
 * function and the number of params, the rest are the function params 
 */
#define MAX_ACTIONS (2+6)

struct action{
	int cline;
	char *cfile;
	enum action_type type;  /* forward, drop, log, send ...*/
	int count;
	struct action* next;
	action_u_t val[MAX_ACTIONS];
};

struct expr* mk_exp(int op, struct expr* left, struct expr* right);
struct expr* mk_elem(int op, expr_l_type ltype, void* lparam,
							 expr_r_type rtype, void* rparam);

/* @param type - type of the action
   @param count - count of couples {type,val} (variable number of parameters)*/
struct action* mk_action(enum action_type type, int count, ...);

struct action* append_action(struct action* a, struct action* b);

void print_action(struct action* a);
void print_actions(struct action* a);
void print_expr(struct expr* exp);

/** joins to cfg file positions into a new one. */
void cfg_pos_join(struct cfg_pos* res,
							struct cfg_pos* pos1, struct cfg_pos* pos2);
#endif

