/*
 * $Id$
 *
 * dialog module - basic support for dialog tracking
 *
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
 * --------
 *  2006-04-14 initial version (bogdan)
 *  2006-11-28 Added statistic support for the number of early and failed
 *              dialogs. (Jeffrey Magder - SOMA Networks) 
 *  2007-04-30 added dialog matching without DID (dialog ID), but based only
 *              on RFC3261 elements - based on an original patch submitted 
 *              by Michel Bensoussan <michel@extricom.com> (bogdan)
 *  2007-05-15 added saving dialogs' information to database (ancuta)
 *  2007-07-04 added saving dialog cseq, contact, record route 
 *              and bind_addresses(sock_info) for caller and callee (ancuta)
 *  2008-04-14 added new type of callback to be triggered when dialogs are 
 *              loaded from DB (bogdan)
 *  2010-06-16 added sip-router rpc interface (osas)
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "../../sr_module.h"
#include "../../lib/srdb1/db.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../pvar.h"
#include "../../mod_fix.h"
#include "../../script_cb.h"
#include "../../lib/kcore/faked_msg.h"
#include "../../lib/kcore/hash_func.h"
#include "../../lib/kcore/kstats_wrapper.h"
#include "../../mem/mem.h"
#include "../../lib/kmi/mi.h"
#include "../../lvalue.h"
#include "../../parser/parse_to.h"
#include "../../modules/tm/tm_load.h"
#include "../../rpc_lookup.h"
#include "../rr/api.h"
#include "dlg_hash.h"
#include "dlg_timer.h"
#include "dlg_handlers.h"
#include "dlg_load.h"
#include "dlg_cb.h"
#include "dlg_db_handler.h"
#include "dlg_req_within.h"
#include "dlg_profile.h"
#include "dlg_var.h"
#include "dlg_transfer.h"

MODULE_VERSION


static int mod_init(void);
static int child_init(int rank);
static void mod_destroy(void);

/* module parameter */
static int dlg_hash_size = 4096;
static char* rr_param = "did";
static int dlg_flag = -1;
static str timeout_spec = {NULL, 0};
static int default_timeout = 60 * 60 * 12;  /* 12 hours */
static char* profiles_wv_s = NULL;
static char* profiles_nv_s = NULL;
str dlg_extra_hdrs = {NULL,0};
static int db_fetch_rows = 200;

int seq_match_mode = SEQ_MATCH_STRICT_ID;
str dlg_bridge_controller = {"sip:controller@kamailio.org", 27};

str ruri_pvar_param = {"$ru", 3};
pv_elem_t * ruri_param_model = NULL;

/* statistic variables */
int dlg_enable_stats = 1;
int active_dlgs_cnt = 0;
int early_dlgs_cnt = 0;
int detect_spirals = 1;
stat_var *active_dlgs = 0;
stat_var *processed_dlgs = 0;
stat_var *expired_dlgs = 0;
stat_var *failed_dlgs = 0;
stat_var *early_dlgs  = 0;

struct tm_binds d_tmb;
struct rr_binds d_rrb;
pv_spec_t timeout_avp;

int dlg_db_mode_param = DB_MODE_NONE;

/* db stuff */
static str db_url = str_init(DEFAULT_DB_URL);
static unsigned int db_update_period = DB_DEFAULT_UPDATE_PERIOD;

static int pv_get_dlg_count( struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res);

/* commands wrappers and fixups */
static int fixup_profile(void** param, int param_no);
static int fixup_get_profile2(void** param, int param_no);
static int fixup_get_profile3(void** param, int param_no);
static int w_set_dlg_profile(struct sip_msg*, char*, char*);
static int w_unset_dlg_profile(struct sip_msg*, char*, char*);
static int w_is_in_profile(struct sip_msg*, char*, char*);
static int w_get_profile_size2(struct sip_msg*, char*, char*);
static int w_get_profile_size3(struct sip_msg*, char*, char*, char*);
static int w_dlg_isflagset(struct sip_msg *msg, char *flag, str *s2);
static int w_dlg_resetflag(struct sip_msg *msg, char *flag, str *s2);
static int w_dlg_setflag(struct sip_msg *msg, char *flag, char *s2);
static int w_dlg_manage(struct sip_msg*, char*, char*);
static int w_dlg_bye(struct sip_msg*, char*, char*);
static int w_dlg_refer(struct sip_msg*, char*, char*);
static int w_dlg_bridge(struct sip_msg*, char*, char*, char*);
static int fixup_dlg_bye(void** param, int param_no);
static int fixup_dlg_refer(void** param, int param_no);
static int fixup_dlg_bridge(void** param, int param_no);
static int w_dlg_get(struct sip_msg*, char*, char*, char*);
static int w_is_known_dlg(struct sip_msg *);

static cmd_export_t cmds[]={
	{"dlg_manage", (cmd_function)w_dlg_manage,            0,0,
			0, REQUEST_ROUTE },
	{"set_dlg_profile", (cmd_function)w_set_dlg_profile,  1,fixup_profile,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"set_dlg_profile", (cmd_function)w_set_dlg_profile,  2,fixup_profile,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"unset_dlg_profile", (cmd_function)w_unset_dlg_profile,  1,fixup_profile,
			0, FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"unset_dlg_profile", (cmd_function)w_unset_dlg_profile,  2,fixup_profile,
			0, FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"is_in_profile", (cmd_function)w_is_in_profile,      1,fixup_profile,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"is_in_profile", (cmd_function)w_is_in_profile,      2,fixup_profile,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"get_profile_size",(cmd_function)w_get_profile_size2, 2,fixup_get_profile2,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"get_profile_size",(cmd_function)w_get_profile_size3, 3,fixup_get_profile3,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_setflag", (cmd_function)w_dlg_setflag,          1,fixup_igp_null,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_resetflag", (cmd_function)w_dlg_resetflag,      1,fixup_igp_null,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_isflagset", (cmd_function)w_dlg_isflagset,      1,fixup_igp_null,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_bye",(cmd_function)w_dlg_bye,                   1,fixup_dlg_bye,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_refer",(cmd_function)w_dlg_refer,               2,fixup_dlg_refer,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_bridge",(cmd_function)w_dlg_bridge,             3,fixup_dlg_bridge,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"dlg_get",(cmd_function)w_dlg_get,                   3,fixup_dlg_bridge,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"is_known_dlg", (cmd_function)w_is_known_dlg,        0, NULL,
			0, REQUEST_ROUTE| FAILURE_ROUTE | ONREPLY_ROUTE | BRANCH_ROUTE },
	{"load_dlg",  (cmd_function)load_dlg,   0, 0, 0, 0},
	{0,0,0,0,0,0}
};

static param_export_t mod_params[]={
	{ "enable_stats",          INT_PARAM, &dlg_enable_stats         },
	{ "hash_size",             INT_PARAM, &dlg_hash_size            },
	{ "rr_param",              STR_PARAM, &rr_param                 },
	{ "dlg_flag",              INT_PARAM, &dlg_flag                 },
	{ "timeout_avp",           STR_PARAM, &timeout_spec.s           },
	{ "default_timeout",       INT_PARAM, &default_timeout          },
	{ "dlg_extra_hdrs",        STR_PARAM, &dlg_extra_hdrs.s         },
	{ "dlg_match_mode",        INT_PARAM, &seq_match_mode           },
	{ "detect_spirals",        INT_PARAM, &detect_spirals,          },
	{ "db_url",                STR_PARAM, &db_url.s                 },
	{ "db_mode",               INT_PARAM, &dlg_db_mode_param        },
	{ "table_name",            STR_PARAM, &dialog_table_name        },
	{ "call_id_column",        STR_PARAM, &call_id_column.s         },
	{ "from_uri_column",       STR_PARAM, &from_uri_column.s        },
	{ "from_tag_column",       STR_PARAM, &from_tag_column.s        },
	{ "to_uri_column",         STR_PARAM, &to_uri_column.s          },
	{ "to_tag_column",         STR_PARAM, &to_tag_column.s          },
	{ "h_id_column",           STR_PARAM, &h_id_column.s            },
	{ "h_entry_column",        STR_PARAM, &h_entry_column.s         },
	{ "state_column",          STR_PARAM, &state_column.s           },
	{ "start_time_column",     STR_PARAM, &start_time_column.s      },
	{ "timeout_column",        STR_PARAM, &timeout_column.s         },
	{ "to_cseq_column",        STR_PARAM, &to_cseq_column.s         },
	{ "from_cseq_column",      STR_PARAM, &from_cseq_column.s       },
	{ "to_route_column",       STR_PARAM, &to_route_column.s        },
	{ "from_route_column",     STR_PARAM, &from_route_column.s      },
	{ "to_contact_column",     STR_PARAM, &to_contact_column.s      },
	{ "from_contact_column",   STR_PARAM, &from_contact_column.s    },
	{ "to_sock_column",        STR_PARAM, &to_sock_column.s         },
	{ "from_sock_column",      STR_PARAM, &from_sock_column.s       },
	{ "sflags_column",         STR_PARAM, &sflags_column.s          },
	{ "toroute_name_column",   STR_PARAM, &toroute_name_column.s    },
	{ "db_update_period",      INT_PARAM, &db_update_period         },
	{ "db_fetch_rows",         INT_PARAM, &db_fetch_rows            },
	{ "profiles_with_value",   STR_PARAM, &profiles_wv_s            },
	{ "profiles_no_value",     STR_PARAM, &profiles_nv_s            },
	{ "bridge_controller",     STR_PARAM, &dlg_bridge_controller.s  },
	{ "ruri_pvar",             STR_PARAM, &ruri_pvar_param.s        },
	{ 0,0,0 }
};


static stat_export_t mod_stats[] = {
	{"active_dialogs" ,     STAT_NO_RESET,  &active_dlgs       },
	{"early_dialogs",       STAT_NO_RESET,  &early_dlgs        },
	{"processed_dialogs" ,  0,              &processed_dlgs    },
	{"expired_dialogs" ,    0,              &expired_dlgs      },
	{"failed_dialogs",      0,              &failed_dlgs       },
	{0,0,0}
};

struct mi_root * mi_dlg_bridge(struct mi_root *cmd_tree, void *param);

static mi_export_t mi_cmds[] = {
	{ "dlg_list",           mi_print_dlgs,       0,  0,  0},
	{ "dlg_list_ctx",       mi_print_dlgs_ctx,   0,  0,  0},
	{ "dlg_end_dlg",        mi_terminate_dlg,    0,  0,  0},
	{ "dlg_terminate_dlg",  mi_terminate_dlgs,   0,  0,  0},
	{ "profile_get_size",   mi_get_profile,      0,  0,  0},
	{ "profile_list_dlgs",  mi_profile_list,     0,  0,  0},
	{ "dlg_bridge",         mi_dlg_bridge,       0,  0,  0},
	{ 0, 0, 0, 0, 0}
};

static rpc_export_t rpc_methods[];

static pv_export_t mod_items[] = {
	{ {"DLG_count",  sizeof("DLG_count")-1}, PVT_OTHER,  pv_get_dlg_count,    0,
		0, 0, 0, 0 },
	{ {"DLG_lifetime",sizeof("DLG_lifetime")-1}, PVT_OTHER, pv_get_dlg_lifetime, 0,
		0, 0, 0, 0 },
	{ {"DLG_status",  sizeof("DLG_status")-1}, PVT_OTHER, pv_get_dlg_status, 0,
		0, 0, 0, 0 },
	{ {"dlg_ctx",  sizeof("dlg_ctx")-1}, PVT_OTHER, pv_get_dlg_ctx,
		pv_set_dlg_ctx, pv_parse_dlg_ctx_name, 0, 0, 0 },
	{ {"dlg",  sizeof("dlg")-1}, PVT_OTHER, pv_get_dlg,
		0, pv_parse_dlg_name, 0, 0, 0 },
	{ {0, 0}, 0, 0, 0, 0, 0, 0, 0 }
};

struct module_exports exports= {
	"dialog",        /* module's name */
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,            /* exported functions */
	mod_params,      /* param exports */
	mod_stats,       /* exported statistics */
	mi_cmds,         /* exported MI functions */
	mod_items,       /* exported pseudo-variables */
	0,               /* extra processes */
	mod_init,        /* module initialization function */
	0,               /* reply processing function */
	mod_destroy,
	child_init       /* per-child init function */
};


static int fixup_profile(void** param, int param_no)
{
	struct dlg_profile_table *profile;
	pv_elem_t *model=NULL;
	str s;

	s.s = (char*)(*param);
	s.len = strlen(s.s);
	if(s.len==0) {
		LM_ERR("param %d is empty string!\n", param_no);
		return E_CFG;
	}

	if (param_no==1) {
		profile = search_dlg_profile( &s );
		if (profile==NULL) {
			LM_CRIT("profile <%s> not definited\n",s.s);
			return E_CFG;
		}
		pkg_free(*param);
		*param = (void*)profile;
		return 0;
	} else if (param_no==2) {
		if(pv_parse_format(&s ,&model) || model==NULL) {
			LM_ERR("wrong format [%s] for value param!\n", s.s);
			return E_CFG;
		}
		*param = (void*)model;
	}
	return 0;
}


static int fixup_get_profile2(void** param, int param_no)
{
	pv_spec_t *sp;
	int ret;

	if (param_no==1) {
		return fixup_profile(param, 1);
	} else if (param_no==2) {
		ret = fixup_pvar_null(param, 1);
		if (ret<0) return ret;
		sp = (pv_spec_t*)(*param);
		if (sp->type!=PVT_AVP && sp->type!=PVT_SCRIPTVAR) {
			LM_ERR("return must be an AVP or SCRIPT VAR!\n");
			return E_SCRIPT;
		}
	}
	return 0;
}


static int fixup_get_profile3(void** param, int param_no)
{
	if (param_no==1) {
		return fixup_profile(param, 1);
	} else if (param_no==2) {
		return fixup_profile(param, 2);
	} else if (param_no==3) {
		return fixup_get_profile2( param, 2);
	}
	return 0;
}



int load_dlg( struct dlg_binds *dlgb )
{
	dlgb->register_dlgcb = register_dlgcb;
	return 1;
}


static int pv_get_dlg_count(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	int n;
	int l;
	char *ch;

	if(msg==NULL || res==NULL)
		return -1;

	n = active_dlgs ? (int)get_stat_val(active_dlgs) : 0;
	l = 0;
	ch = int2str( n, &l);

	res->rs.s = ch;
	res->rs.len = l;

	res->ri = n;
	res->flags = PV_VAL_STR|PV_VAL_INT|PV_TYPE_INT;

	return 0;
}


static int mod_init(void)
{
	unsigned int n;


#ifdef STATISTICS
	/* register statistics */
	if (register_module_stats( exports.name, mod_stats)!=0 ) {
		LM_ERR("failed to register core statistics\n");
		return -1;
	}
#endif

	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	if (rpc_register_array(rpc_methods)!=0) {
		LM_ERR("failed to register RPC commands\n");
		return -1;
	}

	if(faked_msg_init()<0)
		return -1;

	if (timeout_spec.s)
		timeout_spec.len = strlen(timeout_spec.s);

	dlg_bridge_controller.len = strlen(dlg_bridge_controller.s);
	db_url.len = strlen(db_url.s);
	call_id_column.len = strlen(call_id_column.s);
	from_uri_column.len = strlen(from_uri_column.s);
	from_tag_column.len = strlen(from_tag_column.s);
	to_uri_column.len = strlen(to_uri_column.s);
	to_tag_column.len = strlen(to_tag_column.s);
	h_id_column.len = strlen(h_id_column.s);
	h_entry_column.len = strlen(h_entry_column.s);
	state_column.len = strlen(state_column.s);
	start_time_column.len = strlen(start_time_column.s);
	timeout_column.len = strlen(timeout_column.s);
	to_cseq_column.len = strlen(to_cseq_column.s);
	from_cseq_column.len = strlen(from_cseq_column.s);
	to_route_column.len = strlen(to_route_column.s);
	from_route_column.len = strlen(from_route_column.s);
	to_contact_column.len = strlen(to_contact_column.s);
	from_contact_column.len = strlen(from_contact_column.s);
	to_sock_column.len = strlen(to_sock_column.s);
	from_sock_column.len = strlen(from_sock_column.s);
	sflags_column.len = strlen(sflags_column.s);
	toroute_name_column.len = strlen(toroute_name_column.s);
	dialog_table_name.len = strlen(dialog_table_name.s);

	/* param checkings */
	if (dlg_flag==-1) {
		LM_ERR("no dlg flag set!!\n");
		return -1;
	} else if (dlg_flag>MAX_FLAG) {
		LM_ERR("invalid dlg flag %d!!\n",dlg_flag);
		return -1;
	}

	if (rr_param==0 || rr_param[0]==0) {
		LM_ERR("empty rr_param!!\n");
		return -1;
	} else if (strlen(rr_param)>MAX_DLG_RR_PARAM_NAME) {
		LM_ERR("rr_param too long (max=%d)!!\n", MAX_DLG_RR_PARAM_NAME);
		return -1;
	}

	if (timeout_spec.s) {
		if ( pv_parse_spec(&timeout_spec, &timeout_avp)==0 
				&& (timeout_avp.type!=PVT_AVP)){
			LM_ERR("malformed or non AVP timeout "
				"AVP definition in '%.*s'\n", timeout_spec.len,timeout_spec.s);
			return -1;
		}
	}

	if (default_timeout<=0) {
		LM_ERR("0 default_timeout not accepted!!\n");
		return -1;
	}

	if (ruri_pvar_param.s==NULL || *ruri_pvar_param.s=='\0') {
		LM_ERR("invalid r-uri PV string\n");
		return -1;
	}
	ruri_pvar_param.len = strlen(ruri_pvar_param.s);
	if(pv_parse_format(&ruri_pvar_param, &ruri_param_model) < 0
				|| ruri_param_model==NULL) {
		LM_ERR("malformed r-uri PV string: %s\n", ruri_pvar_param.s);
		return -1;
	}

	/* update the len of the extra headers */
	if (dlg_extra_hdrs.s)
		dlg_extra_hdrs.len = strlen(dlg_extra_hdrs.s);

	if (seq_match_mode!=SEQ_MATCH_NO_ID &&
	seq_match_mode!=SEQ_MATCH_FALLBACK &&
	seq_match_mode!=SEQ_MATCH_STRICT_ID ) {
		LM_ERR("invalid value %d for seq_match_mode param!!\n",seq_match_mode);
		return -1;
	}

	if (detect_spirals != 0 && detect_spirals != 1) {
		LM_ERR("invalid value %d for detect_spirals param!!\n",detect_spirals);
		return -1;
	}

	/* if statistics are disabled, prevent their registration to core */
	if (dlg_enable_stats==0)
		exports.stats = 0;

	/* create profile hashes */
	if (add_profile_definitions( profiles_nv_s, 0)!=0 ) {
		LM_ERR("failed to add profiles without value\n");
		return -1;
	}
	if (add_profile_definitions( profiles_wv_s, 1)!=0 ) {
		LM_ERR("failed to add profiles with value\n");
		return -1;
	}

	/* load the TM API */
	if (load_tm_api(&d_tmb)!=0) {
		LM_ERR("can't load TM API\n");
		return -1;
	}

	/* load RR API also */
	if (load_rr_api(&d_rrb)!=0) {
		LM_ERR("can't load RR API\n");
		return -1;
	}

	/* register callbacks*/
	/* listen for all incoming requests  */
	if ( d_tmb.register_tmcb( 0, 0, TMCB_REQUEST_IN, dlg_onreq, 0, 0 ) <=0 ) {
		LM_ERR("cannot register TMCB_REQUEST_IN callback\n");
		return -1;
	}

	/* listen for all routed requests  */
	if ( d_rrb.register_rrcb( dlg_onroute, 0 ) <0 ) {
		LM_ERR("cannot register RR callback\n");
		return -1;
	}

	if (register_script_cb( profile_cleanup, POST_SCRIPT_CB|REQUEST_CB,0)<0) {
		LM_ERR("cannot regsiter script callback");
		return -1;
	}
	if (register_script_cb(dlg_cfg_cb,
				PRE_SCRIPT_CB|REQUEST_CB,0)<0)
	{
		LM_ERR("cannot regsiter pre-script ctx callback\n");
		return -1;
	}
	if (register_script_cb(dlg_cfg_cb,
				POST_SCRIPT_CB|REQUEST_CB,0)<0)
	{
		LM_ERR("cannot regsiter post-script ctx callback\n");
		return -1;
	}

	if ( register_timer( dlg_timer_routine, 0, 1)<0 ) {
		LM_ERR("failed to register timer \n");
		return -1;
	}

	/* init handlers */
	init_dlg_handlers( rr_param, dlg_flag,
		timeout_spec.s?&timeout_avp:0, default_timeout);

	/* init timer */
	if (init_dlg_timer(dlg_ontimeout)!=0) {
		LM_ERR("cannot init timer list\n");
		return -1;
	}

	/* initialized the hash table */
	for( n=0 ; n<(8*sizeof(n)) ; n++) {
		if (dlg_hash_size==(1<<n))
			break;
		if (dlg_hash_size<(1<<n)) {
			LM_WARN("hash_size is not a power "
				"of 2 as it should be -> rounding from %d to %d\n",
				dlg_hash_size, 1<<(n-1));
			dlg_hash_size = 1<<(n-1);
		}
	}

	if ( init_dlg_table(dlg_hash_size)<0 ) {
		LM_ERR("failed to create hash table\n");
		return -1;
	}

	/* if a database should be used to store the dialogs' information */
	dlg_db_mode = dlg_db_mode_param;
	if (dlg_db_mode==DB_MODE_NONE) {
		db_url.s = 0; db_url.len = 0;
	} else {
		if (dlg_db_mode!=DB_MODE_REALTIME &&
		dlg_db_mode!=DB_MODE_DELAYED && dlg_db_mode!=DB_MODE_SHUTDOWN ) {
			LM_ERR("unsupported db_mode %d\n", dlg_db_mode);
			return -1;
		}
		if ( !db_url.s || db_url.len==0 ) {
			LM_ERR("db_url not configured for db_mode %d\n", dlg_db_mode);
			return -1;
		}
		if (init_dlg_db(&db_url, dlg_hash_size, db_update_period,db_fetch_rows)!=0) {
			LM_ERR("failed to initialize the DB support\n");
			return -1;
		}
		run_load_callbacks();
	}

	destroy_dlg_callbacks( DLGCB_LOADED );

	return 0;
}


static int child_init(int rank)
{
	dlg_db_mode = dlg_db_mode_param;

	if (rank==1) {
		if_update_stat(dlg_enable_stats, active_dlgs, active_dlgs_cnt);
		if_update_stat(dlg_enable_stats, early_dlgs, early_dlgs_cnt);
	}

	if ( ((dlg_db_mode==DB_MODE_REALTIME || dlg_db_mode==DB_MODE_DELAYED)
				&& (rank>0 || rank==PROC_TIMER || rank==PROC_MAIN))
			|| (dlg_db_mode==DB_MODE_SHUTDOWN && (rank==PROC_MAIN)) )
	{
		if ( dlg_connect_db(&db_url) ) {
			LM_ERR("failed to connect to database (rank=%d)\n",rank);
			return -1;
		}
	}

	/* in DB_MODE_SHUTDOWN only PROC_MAIN will do a DB dump at the end, so
	 * for the rest of the processes will be the same as DB_MODE_NONE */
	if (dlg_db_mode==DB_MODE_SHUTDOWN && rank!=PROC_MAIN)
		dlg_db_mode = DB_MODE_NONE;
	/* in DB_MODE_REALTIME and DB_MODE_DELAYED the PROC_MAIN have no DB handle */
	if ( (dlg_db_mode==DB_MODE_REALTIME || dlg_db_mode==DB_MODE_DELAYED) &&
			rank==PROC_MAIN)
		dlg_db_mode = DB_MODE_NONE;

	return 0;
}


static void mod_destroy(void)
{
	if(dlg_db_mode == DB_MODE_DELAYED || dlg_db_mode == DB_MODE_SHUTDOWN) {
		dialog_update_db(0, 0);
		destroy_dlg_db();
	}
	/* no DB interaction from now on */
	dlg_db_mode = DB_MODE_NONE;
	destroy_dlg_table();
	destroy_dlg_timer();
	destroy_dlg_callbacks( DLGCB_CREATED|DLGCB_LOADED );
	destroy_dlg_handlers();
	destroy_dlg_profiles();
}



static int w_set_dlg_profile(struct sip_msg *msg, char *profile, char *value)
{
	pv_elem_t *pve;
	str val_s;

	pve = (pv_elem_t *)value;

	if (((struct dlg_profile_table*)profile)->has_value) {
		if ( pve==NULL || pv_printf_s(msg, pve, &val_s)!=0 || 
		val_s.len == 0 || val_s.s == NULL) {
			LM_WARN("cannot get string for value\n");
			return -1;
		}
		if ( set_dlg_profile( msg, &val_s,
		(struct dlg_profile_table*)profile) < 0 ) {
			LM_ERR("failed to set profile");
			return -1;
		}
	} else {
		if ( set_dlg_profile( msg, NULL,
		(struct dlg_profile_table*)profile) < 0 ) {
			LM_ERR("failed to set profile");
			return -1;
		}
	}
	return 1;
}



static int w_unset_dlg_profile(struct sip_msg *msg, char *profile, char *value)
{
	pv_elem_t *pve;
	str val_s;

	pve = (pv_elem_t *)value;

	if (((struct dlg_profile_table*)profile)->has_value) {
		if ( pve==NULL || pv_printf_s(msg, pve, &val_s)!=0 || 
		val_s.len == 0 || val_s.s == NULL) {
			LM_WARN("cannot get string for value\n");
			return -1;
		}
		if ( unset_dlg_profile( msg, &val_s,
		(struct dlg_profile_table*)profile) < 0 ) {
			LM_ERR("failed to unset profile");
			return -1;
		}
	} else {
		if ( unset_dlg_profile( msg, NULL,
		(struct dlg_profile_table*)profile) < 0 ) {
			LM_ERR("failed to unset profile");
			return -1;
		}
	}
	return 1;
}



static int w_is_in_profile(struct sip_msg *msg, char *profile, char *value)
{
	pv_elem_t *pve;
	str val_s;

	pve = (pv_elem_t *)value;

	if ( pve!=NULL && ((struct dlg_profile_table*)profile)->has_value) {
		if ( pv_printf_s(msg, pve, &val_s)!=0 || 
		val_s.len == 0 || val_s.s == NULL) {
			LM_WARN("cannot get string for value\n");
			return -1;
		}
		return is_dlg_in_profile( msg, (struct dlg_profile_table*)profile,
			&val_s);
	} else {
		return is_dlg_in_profile( msg, (struct dlg_profile_table*)profile,
			NULL);
	}
}


/**
 * get dynamic name profile size
 */
static int w_get_profile_size3(struct sip_msg *msg, char *profile,
		char *value, char *result)
{
	pv_elem_t *pve;
	str val_s;
	pv_spec_t *sp_dest;
	unsigned int size;
	pv_value_t val;

	if(result!=NULL)
	{
		pve = (pv_elem_t *)value;
		sp_dest = (pv_spec_t *)result;
	} else {
		pve = NULL;
		sp_dest = (pv_spec_t *)value;
	}
	if ( pve!=NULL && ((struct dlg_profile_table*)profile)->has_value) {
		if ( pv_printf_s(msg, pve, &val_s)!=0 || 
		val_s.len == 0 || val_s.s == NULL) {
			LM_WARN("cannot get string for value\n");
			return -1;
		}
		size = get_profile_size( (struct dlg_profile_table*)profile, &val_s );
	} else {
		size = get_profile_size( (struct dlg_profile_table*)profile, NULL );
	}

	memset(&val, 0, sizeof(pv_value_t));
	val.flags = PV_VAL_INT|PV_TYPE_INT;
	val.ri = (int)size;

	if(sp_dest->setf(msg, &sp_dest->pvp, (int)EQ_T, &val)<0)
	{
		LM_ERR("setting profile PV failed\n");
		return -1;
	}

	return 1;
}


/**
 * get static name profile size
 */
static int w_get_profile_size2(struct sip_msg *msg, char *profile, char *result)
{
	return w_get_profile_size3(msg, profile, result, NULL);
}


static int w_dlg_setflag(struct sip_msg *msg, char *flag, char *s2)
{
	dlg_ctx_t *dctx;
	int val;

	if(fixup_get_ivalue(msg, (gparam_p)flag, &val)!=0)
	{
		LM_ERR("no flag value\n");
		return -1;
	}
	if(val<0 || val>31)
		return -1;
	if ( (dctx=dlg_get_dlg_ctx())==NULL )
		return -1;

	dctx->flags |= 1<<val;
	if(dctx->dlg)
		dctx->dlg->sflags |= 1<<val;
	return 1;
}


static int w_dlg_resetflag(struct sip_msg *msg, char *flag, str *s2)
{
	dlg_ctx_t *dctx;
	int val;

	if(fixup_get_ivalue(msg, (gparam_p)flag, &val)!=0)
	{
		LM_ERR("no flag value\n");
		return -1;
	}
	if(val<0 || val>31)
		return -1;

	if ( (dctx=dlg_get_dlg_ctx())==NULL )
		return -1;

	dctx->flags &= ~(1<<val);
	if(dctx->dlg)
		dctx->dlg->sflags &= ~(1<<val);
	return 1;
}


static int w_dlg_isflagset(struct sip_msg *msg, char *flag, str *s2)
{
	dlg_ctx_t *dctx;
	int val;

	if(fixup_get_ivalue(msg, (gparam_p)flag, &val)!=0)
	{
		LM_ERR("no flag value\n");
		return -1;
	}
	if(val<0 || val>31)
		return -1;

	if ( (dctx=dlg_get_dlg_ctx())==NULL )
		return -1;

	if(dctx->dlg)
		return (dctx->dlg->sflags&(1<<val))?1:-1;
	return (dctx->flags&(1<<val))?1:-1;
}

static int w_dlg_manage(struct sip_msg *msg, char *s1, char *s2)
{
	str tag;
	int backup_mode;

	if( (msg->to==NULL && parse_headers(msg, HDR_TO_F,0)<0) || msg->to==NULL )
	{
		LM_ERR("bad TO header\n");
		return -1;
	}
	tag = get_to(msg)->tag_value;
	if(tag.s!=0 && tag.len!=0)
	{
		backup_mode = seq_match_mode;
		seq_match_mode = SEQ_MATCH_NO_ID;
		dlg_onroute(msg, NULL, NULL);
		seq_match_mode = backup_mode;
	} else {
		if(dlg_new_dialog(msg, 0)!=0)
			return -1;
	}
	return 1;
}

static int w_dlg_bye(struct sip_msg *msg, char *side, char *s2)
{
	struct dlg_cell *dlg;
	int n;

	dlg = dlg_get_ctx_dialog();
	if(dlg==NULL)
		return -1;
	
	n = (int)(long)side;
	if(n==1)
	{
		if(dlg_bye(dlg, NULL, DLG_CALLER_LEG)!=0)
			return -1;
		return 1;
	} else if(n==2) {
		if(dlg_bye(dlg, NULL, DLG_CALLEE_LEG)!=0)
			return -1;
		return 1;
	} else {
		if(dlg_bye_all(dlg, NULL)!=0)
			return -1;
		return 1;
	}
}

static int w_dlg_refer(struct sip_msg *msg, char *side, char *to)
{
	struct dlg_cell *dlg;
	int n;
	str st = {0,0};

	dlg = dlg_get_ctx_dialog();
	if(dlg==NULL)
		return -1;
	
	n = (int)(long)side;

	if(fixup_get_svalue(msg, (gparam_p)to, &st)!=0)
	{
		LM_ERR("unable to get To\n");
		return -1;
	}
	if(st.s==NULL || st.len == 0)
	{
		LM_ERR("invalid To parameter\n");
		return -1;
	}
	if(n==1)
	{
		if(dlg_transfer(dlg, &st, DLG_CALLER_LEG)!=0)
			return -1;
	} else {
		if(dlg_transfer(dlg, &st, DLG_CALLEE_LEG)!=0)
			return -1;
	}
	return 1;
}

static int w_dlg_bridge(struct sip_msg *msg, char *from, char *to, char *op)
{
	str sf = {0,0};
	str st = {0,0};
	str so = {0,0};

	if(from==0 || to==0 || op==0)
	{
		LM_ERR("invalid parameters\n");
		return -1;
	}

	if(fixup_get_svalue(msg, (gparam_p)from, &sf)!=0)
	{
		LM_ERR("unable to get From\n");
		return -1;
	}
	if(sf.s==NULL || sf.len == 0)
	{
		LM_ERR("invalid From parameter\n");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)to, &st)!=0)
	{
		LM_ERR("unable to get To\n");
		return -1;
	}
	if(st.s==NULL || st.len == 0)
	{
		LM_ERR("invalid To parameter\n");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)op, &so)!=0)
	{
		LM_ERR("unable to get OP\n");
		return -1;
	}

	if(dlg_bridge(&sf, &st, &so)!=0)
		return -1;
	return 1;
}


static int fixup_dlg_bye(void** param, int param_no)
{
	char *val;
	int n = 0;

	if (param_no==1) {
		val = (char*)*param;
		if (strcasecmp(val,"all")==0) {
			n = 0;
		} else if (strcasecmp(val,"caller")==0) {
			n = 1;
		} else if (strcasecmp(val,"callee")==0) {
			n = 2;
		} else {
			LM_ERR("invalid param \"%s\"\n", val);
			return E_CFG;
		}
		pkg_free(*param);
		*param=(void*)(long)n;
	} else {
		LM_ERR("called with parameter != 1\n");
		return E_BUG;
	}
	return 0;
}

static int fixup_dlg_refer(void** param, int param_no)
{
	char *val;
	int n = 0;

	if (param_no==1) {
		val = (char*)*param;
		if (strcasecmp(val,"caller")==0) {
			n = 1;
		} else if (strcasecmp(val,"callee")==0) {
			n = 2;
		} else {
			LM_ERR("invalid param \"%s\"\n", val);
			return E_CFG;
		}
		pkg_free(*param);
		*param=(void*)(long)n;
	} else if (param_no==2) {
		return fixup_spve_null(param, 1);
	} else {
		LM_ERR("called with parameter idx %d\n", param_no);
		return E_BUG;
	}
	return 0;
}

static int fixup_dlg_bridge(void** param, int param_no)
{
	if (param_no>=1 && param_no<=3) {
		return fixup_spve_null(param, 1);
	} else {
		LM_ERR("called with parameter idx %d\n", param_no);
		return E_BUG;
	}
	return 0;
}

static int w_dlg_get(struct sip_msg *msg, char *ci, char *ft, char *tt)
{
	struct dlg_cell *dlg = NULL;
	str sc = {0,0};
	str sf = {0,0};
	str st = {0,0};
	unsigned int dir = 0;

	if(ci==0 || ft==0 || tt==0)
	{
		LM_ERR("invalid parameters\n");
		return -1;
	}

	if(fixup_get_svalue(msg, (gparam_p)ci, &sc)!=0)
	{
		LM_ERR("unable to get Call-ID\n");
		return -1;
	}
	if(sc.s==NULL || sc.len == 0)
	{
		LM_ERR("invalid Call-ID parameter\n");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)ft, &sf)!=0)
	{
		LM_ERR("unable to get From tag\n");
		return -1;
	}
	if(sf.s==NULL || sf.len == 0)
	{
		LM_ERR("invalid From tag parameter\n");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)tt, &st)!=0)
	{
		LM_ERR("unable to get To Tag\n");
		return -1;
	}
	if(st.s==NULL || st.len == 0)
	{
		LM_ERR("invalid To tag parameter\n");
		return -1;
	}

	dlg = get_dlg(&sc, &sf, &st, &dir, NULL);
	if(dlg==NULL)
		return -1;
	current_dlg_pointer = dlg;
	_dlg_ctx.dlg = dlg;
	_dlg_ctx.dir = dir;
	return 1;
}

struct mi_root * mi_dlg_bridge(struct mi_root *cmd_tree, void *param)
{
	str from = {0,0};
	str to = {0,0};
	str op = {0,0};
	struct mi_node* node;

	node = cmd_tree->node.kids;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);
	from = node->value;
	if(from.len<=0 || from.s==NULL)
	{
		LM_ERR("bad From value\n");
		return init_mi_tree( 500, "Bad From value", 14);
	}

	node = node->next;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);
	to = node->value;
	if(to.len<=0 || to.s == NULL)
	{
		return init_mi_tree(500, "Bad To value", 12);
	}

	node= node->next;
	if(node != NULL)
	{
		op = node->value;
		if(op.len<=0 || op.s==NULL)
		{
			return init_mi_tree(500, "Bad OP value", 12);
		}
	}

	if(dlg_bridge(&from, &to, &op)!=0)
		return init_mi_tree(500, MI_INTERNAL_ERR_S,  MI_INTERNAL_ERR_LEN);

	return init_mi_tree(200, MI_OK_S, MI_OK_LEN);
}

/**************************** RPC functions ******************************/
/*!
 * \brief Helper method that outputs a dialog via the RPC interface
 * \see rpc_print_dlg
 * \param rpc RPC node that should be filled
 * \param c RPC void pointer
 * \param dlg printed dialog
 * \param with_context if 1 then the dialog context will be also printed
 * \return 0 on success, -1 on failure
 */
static inline void internal_rpc_print_dlg(rpc_t *rpc, void *c, struct dlg_cell *dlg, int with_context)
{
	rpc_cb_ctx_t rpc_cb;

	rpc->printf(c, "hash:%u:%u state:%u timestart:%u timeout:%u",
		dlg->h_entry, dlg->h_id, dlg->state, dlg->start_ts, dlg->tl.timeout);
	rpc->printf(c, "\tcallid:%.*s from_tag:%.*s to_tag:%.*s",
		dlg->callid.len, dlg->callid.s,
		dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
		dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
	rpc->printf(c, "\tfrom_uri:%.*s to_uri:%.*s",
		dlg->from_uri.len, dlg->from_uri.s, dlg->to_uri.len, dlg->to_uri.s);
	rpc->printf(c, "\tcaller_contact:%.*s caller_cseq:%.*s",
		dlg->contact[DLG_CALLER_LEG].len, dlg->contact[DLG_CALLER_LEG].s,
		dlg->cseq[DLG_CALLER_LEG].len, dlg->cseq[DLG_CALLER_LEG].s);
	rpc->printf(c, "\tcaller_route_set: %.*s",
		dlg->route_set[DLG_CALLER_LEG].len, dlg->route_set[DLG_CALLER_LEG].s);
	rpc->printf(c, "\tcallee_contact:%.*s callee_cseq:%.*s",
		dlg->contact[DLG_CALLEE_LEG].len, dlg->contact[DLG_CALLEE_LEG].s,
		dlg->cseq[DLG_CALLEE_LEG].len, dlg->cseq[DLG_CALLEE_LEG].s);
	rpc->printf(c, "\tcallee_route_set: %.*s",
		dlg->route_set[DLG_CALLEE_LEG].len, dlg->route_set[DLG_CALLEE_LEG].s);
	if (dlg->bind_addr[DLG_CALLEE_LEG]) {
		rpc->printf(c, "\tcaller_bind_addr:%.*s callee_bind_addr:%.*s",
			dlg->bind_addr[DLG_CALLER_LEG]->sock_str.len, dlg->bind_addr[DLG_CALLER_LEG]->sock_str.s,
			dlg->bind_addr[DLG_CALLEE_LEG]->sock_str.len, dlg->bind_addr[DLG_CALLEE_LEG]->sock_str.s);
	} else {
		rpc->printf(c, "\tcaller_bind_addr:%.*s callee_bind_addr:",
			dlg->bind_addr[DLG_CALLER_LEG]->sock_str.len, dlg->bind_addr[DLG_CALLER_LEG]->sock_str.s);
	}
	if (with_context) {
		rpc_cb.rpc = rpc;
		rpc_cb.c = c;
		run_dlg_callbacks( DLGCB_RPC_CONTEXT, dlg, NULL, DLG_DIR_NONE, (void *)&rpc_cb);
	}
}

/*!
 * \brief Helper function that outputs all dialogs via the RPC interface
 * \see rpc_print_dlgs
 * \param rpc RPC node that should be filled
 * \param c RPC void pointer
 * \param with_context if 1 then the dialog context will be also printed
 */
static void internal_rpc_print_dlgs(rpc_t *rpc, void *c, int with_context)
{
	struct dlg_cell *dlg;
	unsigned int i;

	for( i=0 ; i<d_table->size ; i++ ) {
		dlg_lock( d_table, &(d_table->entries[i]) );

		for( dlg=d_table->entries[i].first ; dlg ; dlg=dlg->next ) {
			internal_rpc_print_dlg(rpc, c, dlg, with_context);
		}
		dlg_unlock( d_table, &(d_table->entries[i]) );
	}
}

/*!
 * \brief Helper function that outputs a dialog via the RPC interface
 * \see rpc_print_dlgs
 * \param rpc RPC node that should be filled
 * \param c RPC void pointer
 * \param with_context if 1 then the dialog context will be also printed
 */
static void internal_rpc_print_single_dlg(rpc_t *rpc, void *c, int with_context) {
	str callid, from_tag;
	struct dlg_entry *d_entry;
	struct dlg_cell *dlg;
	unsigned int h_entry;

	if (rpc->scan(c, ".S.S", &callid, &from_tag) < 2) return;

	h_entry = core_hash( &callid, 0, d_table->size);
	d_entry = &(d_table->entries[h_entry]);
	dlg_lock( d_table, d_entry);
	for( dlg = d_entry->first ; dlg ; dlg = dlg->next ) {
		if (match_downstream_dialog( dlg, &callid, &from_tag)==1) {
			internal_rpc_print_dlg(rpc, c, dlg, with_context);
		}
	}
	dlg_unlock( d_table, d_entry);
}

/*!
 * \brief Helper function that outputs the size of a given profile via the RPC interface
 * \see rpc_profile_get_size
 * \see rpc_profile_w_value_get_size
 * \param rpc RPC node that should be filled
 * \param c RPC void pointer
 * \param profile_name the given profile
 * \param value the given profile value
 */
static void internal_rpc_profile_get_size(rpc_t *rpc, void *c, str *profile_name, str *value) {
	unsigned int size;
	struct dlg_profile_table *profile;

	profile = search_dlg_profile( profile_name );
	if (!profile) {
		rpc->printf(c, "Non existing profile:%.*s",
			profile_name->len, profile_name->s);
		return;
	}
	size = get_profile_size(profile, value);
	if (value) {
		rpc->printf(c, "Profile:%.*s => profile:%.*s value:%.*s count:%u",
			profile_name->len, profile_name->s,
			profile->name.len, profile->name.s,
			value->len, value->s, size);
		return;
	} else {
		rpc->printf(c, "Profile:%.*s => profile:%.*s value: count:%u",
			profile_name->len, profile_name->s,
			profile->name.len, profile->name.s, size);
		return;
	}
	return;
}

/*!
 * \brief Helper function that outputs the dialogs belonging to a given profile via the RPC interface
 * \see rpc_profile_print_dlgs
 * \see rpc_profile_w_value_print_dlgs
 * \param rpc RPC node that should be filled
 * \param c RPC void pointer
 * \param profile_name the given profile
 * \param value the given profile value
 * \param with_context if 1 then the dialog context will be also printed
 */
static void internal_rpc_profile_print_dlgs(rpc_t *rpc, void *c, str *profile_name, str *value) {
	struct dlg_profile_table *profile;
	struct dlg_profile_hash *ph;
	unsigned int i;

	profile = search_dlg_profile( profile_name );
	if (!profile) {
		rpc->printf(c, "Non existing profile:%.*s",
			profile_name->len, profile_name->s);
		return;
	}

	/* go through the hash and print the dialogs */
	if (profile->has_value==0 || value==NULL) {
		/* no value */
		lock_get( &profile->lock );
		for ( i=0 ; i< profile->size ; i++ ) {
			ph = profile->entries[i].first;
			if(ph) {
				do {
					/* print dialog */
					internal_rpc_print_dlg(rpc, c, ph->dlg, 0);
					/* next */
					ph=ph->next;
				}while(ph!=profile->entries[i].first);
			}
			lock_release(&profile->lock);
		}
	} else {
		/* check for value also */
		lock_get( &profile->lock );
		for ( i=0 ; i< profile->size ; i++ ) {
			ph = profile->entries[i].first;
			if(ph) {
				do {
					if ( value->len==ph->value.len &&
						memcmp(value->s,ph->value.s,value->len)==0 ) {
						/* print dialog */
						internal_rpc_print_dlg(rpc, c, ph->dlg, 0);
					}
					/* next */
					ph=ph->next;
				}while(ph!=profile->entries[i].first);
			}
			lock_release(&profile->lock);
		}
	}
}

/*
 * Wrapper around is_known_dlg().
 */

static int w_is_known_dlg(struct sip_msg *msg) {
	return	is_known_dlg(msg);
}

static const char *rpc_print_dlgs_doc[2] = {
	"Print all dialogs", 0
};
static const char *rpc_print_dlgs_ctx_doc[2] = {
	"Print all dialogs with associated context", 0
};
static const char *rpc_print_dlg_doc[2] = {
	"Print dialog based on callid and fromtag", 0
};
static const char *rpc_print_dlg_ctx_doc[2] = {
	"Print dialog with associated context based on callid and fromtag", 0
};
static const char *rpc_end_dlg_entry_id_doc[2] = {
	"End a given dialog based on [h_entry] [h_id]", 0
};
static const char *rpc_profile_get_size_doc[2] = {
	"Returns the number of dialogs belonging to a profile", 0
};
static const char *rpc_profile_print_dlgs_doc[2] = {
	"Lists all the dialogs belonging to a profile", 0
};
static const char *rpc_dlg_bridge_doc[2] = {
	"Bridge two SIP addresses in a call using INVITE(hold)-REFER-BYE mechanism:\
 to, from, [outbound SIP proxy]", 0
};


static void rpc_print_dlgs(rpc_t *rpc, void *c) {
	internal_rpc_print_dlgs(rpc, c, 0);
}
static void rpc_print_dlgs_ctx(rpc_t *rpc, void *c) {
	internal_rpc_print_dlgs(rpc, c, 1);
}
static void rpc_print_dlg(rpc_t *rpc, void *c) {
	internal_rpc_print_single_dlg(rpc, c, 0);
}
static void rpc_print_dlg_ctx(rpc_t *rpc, void *c) {
	internal_rpc_print_single_dlg(rpc, c, 1);
}
static void rpc_end_dlg_entry_id(rpc_t *rpc, void *c) {
	unsigned int h_entry, h_id;
	struct dlg_cell * dlg = NULL;
	str rpc_extra_hdrs = {NULL,0};

	if (rpc->scan(c, "ddS", &h_entry, &h_id, &rpc_extra_hdrs) < 2) return;

	dlg = lookup_dlg(h_entry, h_id, NULL);
	if(dlg){
		dlg_bye_all(dlg, (rpc_extra_hdrs.len>0)?&rpc_extra_hdrs:NULL);
		unref_dlg(dlg, 1);
	}
}
static void rpc_profile_get_size(rpc_t *rpc, void *c) {
	str profile_name = {NULL,0};
	str value = {NULL,0};

	if (rpc->scan(c, ".S", &profile_name) < 1) return;
	if (rpc->scan(c, "*.S", &value) > 0) {
		internal_rpc_profile_get_size(rpc, c, &profile_name, &value);
	} else {
		internal_rpc_profile_get_size(rpc, c, &profile_name, NULL);
	}
	return;
}
static void rpc_profile_print_dlgs(rpc_t *rpc, void *c) {
	str profile_name = {NULL,0};
	str value = {NULL,0};

	if (rpc->scan(c, ".S", &profile_name) < 1) return;
	if (rpc->scan(c, "*.S", &value) > 0) {
		internal_rpc_profile_print_dlgs(rpc, c, &profile_name, &value);
	} else {
		internal_rpc_profile_print_dlgs(rpc, c, &profile_name, NULL);
	}
	return;
}
static void rpc_dlg_bridge(rpc_t *rpc, void *c) {
	str from = {NULL,0};
	str to = {NULL,0};
	str op = {NULL,0};

	if (rpc->scan(c, "SSS", &from, &to, &op) < 2) return;

	dlg_bridge(&from, &to, &op);
}

static rpc_export_t rpc_methods[] = {
	{"dlg.list", rpc_print_dlgs, rpc_print_dlgs_doc, 0},
	{"dlg.list_ctx", rpc_print_dlgs_ctx, rpc_print_dlgs_ctx_doc, 0},
	{"dlg.dlg_list", rpc_print_dlg, rpc_print_dlg_doc, 0},
	{"dlg.dlg_list_ctx", rpc_print_dlg_ctx, rpc_print_dlg_ctx_doc, 0},
	{"dlg.end_dlg", rpc_end_dlg_entry_id, rpc_end_dlg_entry_id_doc, 0},
	{"dlg.profile_get_size", rpc_profile_get_size, rpc_profile_get_size_doc, 0},
	{"dlg.profile_list", rpc_profile_print_dlgs, rpc_profile_print_dlgs_doc, 0},
	{"dlg.bridge_dlg", rpc_dlg_bridge, rpc_dlg_bridge_doc, 0},
	{0, 0, 0, 0}
};
