/**
 * $Id$
 *
 * Copyright (C) 2008 Daniel-Constantin Mierla (asipto.com)
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../sr_module.h"
#include "../../pvar.h"
#include "../../mod_fix.h"
#include "../../lib/kmi/mi.h"

#include "pv_branch.h"
#include "pv_core.h"
#include "pv_stats.h"
#include "pv_shv.h"
#include "pv_time.h"
#include "pv_trans.h"
#include "pv_select.h"
#ifdef WITH_XAVP
#include "pv_xavp.h"
#endif

MODULE_VERSION

static tr_export_t mod_trans[] = {
	{ {"s", sizeof("s")-1}, /* string class */
		tr_parse_string },
	{ {"nameaddr", sizeof("nameaddr")-1}, /* nameaddr class */
		tr_parse_nameaddr },
	{ {"uri", sizeof("uri")-1}, /* uri class */
		tr_parse_uri },
	{ {"param", sizeof("param")-1}, /* param class */
		tr_parse_paramlist },
	{ {"tobody", sizeof("tobody")-1}, /* param class */
		tr_parse_tobody },

	{ { 0, 0 }, 0 }
};

static pv_export_t mod_pvs[] = {
	{ {"branch", sizeof("branch")-1}, /* branch attributes */
		PVT_CONTEXT, pv_get_branchx, pv_set_branchx,
		pv_parse_branchx_name, pv_parse_index, 0, 0 },
	{ {"mi", (sizeof("mi")-1)}, /* message id */
		PVT_OTHER, pv_get_msgid, 0,
		0, 0, 0, 0},
	{ {"stat", sizeof("stat")-1}, /* statistics */
		PVT_OTHER, pv_get_stat, 0,
		pv_parse_stat_name, 0, 0, 0 },
	{ {"sel", sizeof("sel")-1}, /* select */
		PVT_OTHER, pv_get_select, 0,
		pv_parse_select_name, 0, 0, 0 },
	{ {"snd", (sizeof("snd")-1)}, PVT_OTHER, pv_get_sndto, 0,
		pv_parse_snd_name, 0, 0, 0 },
	{ {"sndto", (sizeof("sndto")-1)}, PVT_OTHER, pv_get_sndto, 0,
		pv_parse_snd_name, 0, 0, 0 },
	{ {"sndfrom", (sizeof("sndfrom")-1)}, PVT_OTHER, pv_get_sndfrom, 0,
		pv_parse_snd_name, 0, 0, 0 },
#ifdef WITH_XAVP
	{ {"xavp", sizeof("xavp")-1}, /* xavp */
		PVT_OTHER, pv_get_xavp, pv_set_xavp,
		pv_parse_xavp_name, 0, 0, 0 },
#endif

	{{"avp", (sizeof("avp")-1)}, PVT_AVP, pv_get_avp, pv_set_avp,
		pv_parse_avp_name, pv_parse_index, 0, 0},
	{{"hdr", (sizeof("hdr")-1)}, PVT_HDR, pv_get_hdr, 0, pv_parse_hdr_name,
		pv_parse_index, 0, 0},
	{{"var", (sizeof("var")-1)}, PVT_SCRIPTVAR, pv_get_scriptvar,
		pv_set_scriptvar, pv_parse_scriptvar_name, 0, 0, 0},
	{{"ai", (sizeof("ai")-1)}, /* */
		PVT_OTHER, pv_get_pai, 0,
		0, 0, 0, 0},
	{{"adu", (sizeof("adu")-1)}, /* auth digest uri */
		PVT_OTHER, pv_get_authattr, 0,
		0, 0, pv_init_iname, 3},
	{{"ar", (sizeof("ar")-1)}, /* auth realm */
		PVT_OTHER, pv_get_authattr, 0,
		0, 0, pv_init_iname, 2},
	{{"au", (sizeof("au")-1)}, /* */
		PVT_OTHER, pv_get_authattr, 0,
		0, 0, pv_init_iname, 1},
	{{"ad", (sizeof("ad")-1)}, /* */
		PVT_OTHER, pv_get_authattr, 0,
		0, 0, pv_init_iname, 4},
	{{"aU", (sizeof("aU")-1)}, /* */
		PVT_OTHER, pv_get_authattr, 0,
		0, 0, pv_init_iname, 5},
	{{"Au", (sizeof("Au")-1)}, /* */
		PVT_OTHER, pv_get_acc_username, 0,
		0, 0, pv_init_iname, 1},
	{{"bf", (sizeof("bf")-1)}, /* */
		PVT_CONTEXT, pv_get_bflags, pv_set_bflags,
		0, 0, 0, 0},
	{{"bF", (sizeof("bF")-1)}, /* */
		PVT_CONTEXT, pv_get_hexbflags, pv_set_bflags,
		0, 0, 0, 0},
	{{"br", (sizeof("br")-1)}, /* */
		PVT_BRANCH, pv_get_branch, pv_set_branch,
		0, 0, 0, 0},
	{{"bR", (sizeof("bR")-1)}, /* */
		PVT_CONTEXT, pv_get_branches, 0,
		0, 0, 0, 0},
	{{"bs", (sizeof("bs")-1)}, /* */
		PVT_OTHER, pv_get_body_size, 0,
		0, 0, 0, 0},
	{{"ci", (sizeof("ci")-1)}, /* */
		PVT_OTHER, pv_get_callid, 0,
		0, 0, 0, 0},
	{{"cl", (sizeof("cl")-1)}, /* */
		PVT_OTHER, pv_get_content_length, 0,
		0, 0, 0, 0},
	{{"cs", (sizeof("cs")-1)}, /* */
		PVT_OTHER, pv_get_cseq, 0,
		0, 0, 0, 0},
	{{"ct", (sizeof("ct")-1)}, /* */
		PVT_OTHER, pv_get_contact, 0,
		0, 0, 0, 0},
	{{"cT", (sizeof("cT")-1)}, /* */
		PVT_OTHER, pv_get_content_type, 0,
		0, 0, 0, 0},
	{{"dd", (sizeof("dd")-1)}, /* */
		PVT_OTHER, pv_get_dsturi_attr, 0,
		0, 0, pv_init_iname, 1},
	{{"di", (sizeof("di")-1)}, /* */
		PVT_OTHER, pv_get_diversion, 0,
		0, 0, pv_init_iname, 1},
	{{"dir", (sizeof("dir")-1)}, /* */
		PVT_OTHER, pv_get_diversion, 0,
		0, 0, pv_init_iname, 2},
	{{"dip", (sizeof("dis")-1)}, /* */
		PVT_OTHER, pv_get_diversion, 0,
		0, 0, pv_init_iname, 3},
	{{"dp", (sizeof("dp")-1)}, /* */
		PVT_OTHER, pv_get_dsturi_attr, 0,
		0, 0, pv_init_iname, 2},
	{{"dP", (sizeof("dP")-1)}, /* */
		PVT_OTHER, pv_get_dsturi_attr, 0,
		0, 0, pv_init_iname, 3},
	{{"ds", (sizeof("ds")-1)}, /* */
		PVT_CONTEXT, pv_get_dset, 0,
		0, 0, 0, 0},
	{{"du", (sizeof("du")-1)}, /* */
		PVT_DSTURI, pv_get_dsturi, pv_set_dsturi,
		0, 0, 0, 0},
	{{"duri", (sizeof("duri")-1)}, /* */
		PVT_DSTURI, pv_get_dsturi, pv_set_dsturi,
		0, 0, 0, 0},
	{{"err.class", (sizeof("err.class")-1)}, /* */
		PVT_OTHER, pv_get_errinfo_attr, 0,
		0, 0, 0, 0},
	{{"err.level", (sizeof("err.level")-1)}, /* */
		PVT_OTHER, pv_get_errinfo_attr, 0,
		0, 0, pv_init_iname, 1},
	{{"err.info", (sizeof("err.info")-1)}, /* */
		PVT_OTHER, pv_get_errinfo_attr, 0,
		0, 0, pv_init_iname, 2},
	{{"err.rcode", (sizeof("err.rcode")-1)}, /* */
		PVT_OTHER, pv_get_errinfo_attr, 0,
		0, 0, pv_init_iname, 3},
	{{"err.rreason", (sizeof("err.rreason")-1)}, /* */
		PVT_OTHER, pv_get_errinfo_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"fd", (sizeof("fd")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, pv_set_from_domain,
		0, 0, pv_init_iname, 3},
	{{"from.domain", (sizeof("from.domain")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, pv_set_from_domain,
		0, 0, pv_init_iname, 3},
	{{"fn", (sizeof("fn")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, pv_set_from_display,
		0, 0, pv_init_iname, 5},
	{{"fs", (sizeof("fs")-1)}, /* */
		PVT_OTHER, pv_get_force_sock, pv_set_force_sock,
		0, 0, 0, 0},
	{{"ft", (sizeof("ft")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"fu", (sizeof("fu")-1)}, /* */
		PVT_FROM, pv_get_from_attr, pv_set_from_uri,
		0, 0, pv_init_iname, 1},
	{{"from", (sizeof("from")-1)}, /* */
		PVT_FROM, pv_get_from_attr, pv_set_from_uri,
		0, 0, pv_init_iname, 1},
	{{"fU", (sizeof("fU")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, pv_set_from_username,
		0, 0, pv_init_iname, 2},
	{{"from.user", (sizeof("from.user")-1)}, /* */
		PVT_OTHER, pv_get_from_attr, pv_set_from_username,
		0, 0, pv_init_iname, 2},
	{{"mb", (sizeof("mb")-1)}, /* */
		PVT_OTHER, pv_get_msg_buf, 0,
		0, 0, 0, 0},
	{{"mf", (sizeof("mf")-1)}, /* */
		PVT_OTHER, pv_get_flags, pv_set_mflags,
		0, 0, 0, 0},
	{{"mF", (sizeof("mF")-1)}, /* */
		PVT_OTHER, pv_get_hexflags, pv_set_mflags,
		0, 0, 0, 0},
	{{"ml", (sizeof("ml")-1)}, /* */
		PVT_OTHER, pv_get_msg_len, 0,
		0, 0, 0, 0},
	{{"od", (sizeof("od")-1)}, /* */
		PVT_OTHER, pv_get_ouri_attr, 0,
		0, 0, pv_init_iname, 2},
	{{"op", (sizeof("op")-1)}, /* */
		PVT_OTHER, pv_get_ouri_attr, 0,
		0, 0, pv_init_iname, 3},
	{{"oP", (sizeof("oP")-1)}, /* */
		PVT_OTHER, pv_get_ouri_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"ou", (sizeof("ou")-1)}, /* */
		PVT_OURI, pv_get_ouri, 0,
		0, 0, 0, 0},
	{{"ouri", (sizeof("ouri")-1)}, /* */
		PVT_OURI, pv_get_ouri, 0,
		0, 0, 0, 0},
	{{"oU", (sizeof("oU")-1)}, /* */
		PVT_OTHER, pv_get_ouri_attr, 0,
		0, 0, pv_init_iname, 1},
	{{"pd", (sizeof("pd")-1)}, /* */
		PVT_OTHER, pv_get_ppi_attr, 0,
		0, 0, pv_init_iname, 3},
	{{"pn", (sizeof("pn")-1)}, /* */
		PVT_OTHER, pv_get_ppi_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"pp", (sizeof("pp")-1)}, /* */
		PVT_OTHER, pv_get_pid, 0,
		0, 0, 0, 0},
	{{"pr", (sizeof("pr")-1)}, /* */
		PVT_OTHER, pv_get_proto, 0,
		0, 0, 0, 0},
	{{"proto", (sizeof("proto")-1)}, /* */
		PVT_OTHER, pv_get_proto, 0,
		0, 0, 0, 0},
	{{"pu", (sizeof("pu")-1)}, /* */
		PVT_OTHER, pv_get_ppi_attr, 0,
		0, 0, pv_init_iname, 1},
	{{"pU", (sizeof("pU")-1)}, /* */
		PVT_OTHER, pv_get_ppi_attr, 0,
		0, 0, pv_init_iname, 2},
	{{"rb", (sizeof("rb")-1)}, /* */
		PVT_MSG_BODY, pv_get_msg_body, 0,
		0, 0, 0, 0},
	/* {{"rc", (sizeof("rc")-1)},
		PVT_OTHER, pv_get_return_code, 0,
		0, 0, 0, 0},
	{{"retcode", (sizeof("retcode")-1)},
		PVT_OTHER, pv_get_return_code, 0,
		0, 0, 0, 0}, */
	{{"rd", (sizeof("rd")-1)}, /* */
		PVT_RURI_DOMAIN, pv_get_ruri_attr, pv_set_ruri_host,
		0, 0, pv_init_iname, 2},
	{{"ruri.domain", (sizeof("ruri.domain")-1)}, /* */
		PVT_RURI_DOMAIN, pv_get_ruri_attr, pv_set_ruri_host,
		0, 0, pv_init_iname, 2},
	{{"re", (sizeof("re")-1)}, /* */
		PVT_OTHER, pv_get_rpid, 0,
		0, 0, 0, 0},
	{{"rm", (sizeof("rm")-1)}, /* */
		PVT_OTHER, pv_get_method, 0,
		0, 0, 0, 0},
	{{"rp", (sizeof("rp")-1)}, /* */
		PVT_OTHER, pv_get_ruri_attr, pv_set_ruri_port,
		0, 0, pv_init_iname, 3},
	{{"rP", (sizeof("rP")-1)}, /* */
		PVT_OTHER, pv_get_ruri_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"rr", (sizeof("rr")-1)}, /* */
		PVT_OTHER, pv_get_reason, 0,
		0, 0, 0, 0},
	{{"rs", (sizeof("rs")-1)}, /* */
		PVT_OTHER, pv_get_status, 0,
		0, 0, 0, 0},
	{{"rt", (sizeof("rt")-1)}, /* */
		PVT_OTHER, pv_get_refer_to, 0,
		0, 0, 0, 0},
	{{"ru", (sizeof("ru")-1)}, /* */
		PVT_RURI, pv_get_ruri, pv_set_ruri,
		0, 0, 0, 0},
	{{"ruri", (sizeof("ruri")-1)}, /* */
		PVT_RURI, pv_get_ruri, pv_set_ruri,
		0, 0, 0, 0},
	{{"rU", (sizeof("rU")-1)}, /* */
		PVT_RURI_USERNAME, pv_get_ruri_attr, pv_set_ruri_user,
		0, 0, pv_init_iname, 1},
	{{"ruri.user", (sizeof("ruri.user")-1)}, /* */
		PVT_RURI_USERNAME, pv_get_ruri_attr, pv_set_ruri_user,
		0, 0, pv_init_iname, 1},
	{{"rv", (sizeof("rv")-1)}, /* */
		PVT_OTHER, pv_get_version, 0,
		0, 0, 0, 0},
	{{"Ri", (sizeof("Ri")-1)}, /* */
		PVT_OTHER, pv_get_rcvip, 0,
		0, 0, 0, 0},
	{{"Rp", (sizeof("Rp")-1)}, /* */
		PVT_OTHER, pv_get_rcvport, 0,
		0, 0, 0, 0},
	{{"sf", (sizeof("sf")-1)}, /* */
		PVT_OTHER, pv_get_sflags, pv_set_sflags,
		0, 0, 0, 0},
	{{"sF", (sizeof("sF")-1)}, /* */
		PVT_OTHER, pv_get_hexsflags, pv_set_sflags,
		0, 0, 0, 0},
	{{"src_ip", (sizeof("src_ip")-1)}, /* */
		PVT_OTHER, pv_get_srcip, 0,
		0, 0, 0, 0},
	{{"si", (sizeof("si")-1)}, /* */
		PVT_OTHER, pv_get_srcip, 0,
		0, 0, 0, 0},
	{{"sp", (sizeof("sp")-1)}, /* */
		PVT_OTHER, pv_get_srcport, 0,
		0, 0, 0, 0},
	{{"td", (sizeof("td")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, pv_set_to_domain,
		0, 0, pv_init_iname, 3},
	{{"to.domain", (sizeof("to.domain")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, pv_set_to_domain,
		0, 0, pv_init_iname, 3},
	{{"tn", (sizeof("tn")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, pv_set_to_display,
		0, 0, pv_init_iname, 5},
	{{"tt", (sizeof("tt")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, 0,
		0, 0, pv_init_iname, 4},
	{{"tu", (sizeof("tu")-1)}, /* */
		PVT_TO, pv_get_to_attr, pv_set_to_uri,
		0, 0, pv_init_iname, 1},
	{{"to", (sizeof("to")-1)}, /* */
		PVT_TO, pv_get_to_attr, pv_set_to_uri,
		0, 0, pv_init_iname, 1},
	{{"tU", (sizeof("tU")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, pv_set_to_username,
		0, 0, pv_init_iname, 2},
	{{"to.user", (sizeof("to.user")-1)}, /* */
		PVT_OTHER, pv_get_to_attr, pv_set_to_username,
		0, 0, pv_init_iname, 2},
	{{"true", (sizeof("true")-1)}, /* */
		PVT_OTHER, pv_get_true, 0,
		0, 0, 0, 0},
	{{"Tf", (sizeof("Tf")-1)}, /* */
		PVT_CONTEXT, pv_get_timef, 0,
		0, 0, 0, 0},
	{{"TF", (sizeof("TF")-1)}, /* */
		PVT_OTHER, pv_get_timenowf, 0,
		0, 0, 0, 0},
	{{"Ts", (sizeof("Ts")-1)}, /* */
		PVT_CONTEXT, pv_get_times, 0,
		0, 0, 0, 0},
	{{"TS", (sizeof("TS")-1)}, /* */
		PVT_OTHER, pv_get_timenows, 0,
		0, 0, 0, 0},
	{{"ua", (sizeof("ua")-1)}, /* */
		PVT_OTHER, pv_get_useragent, 0,
		0, 0, 0, 0},

	{ {"shv", (sizeof("shv")-1)}, PVT_OTHER, pv_get_shvar,
		pv_set_shvar, pv_parse_shvar_name, 0, 0, 0},
	{ {"time", (sizeof("time")-1)}, PVT_CONTEXT, pv_get_time,
		0, pv_parse_time_name, 0, 0, 0},
	{ {"TV", (sizeof("TV")-1)}, PVT_OTHER, pv_get_timeval,
		0, pv_parse_timeval_name, 0, 0, 0},
	{ {"nh", (sizeof("nh")-1)}, PVT_OTHER, pv_get_nh,
		0, pv_parse_nh_name, 0, 0, 0},

	{ {0, 0}, 0, 0, 0, 0, 0, 0, 0 }
};

static int add_avp_aliases(modparam_t type, void* val);

static param_export_t params[]={ 
	{"shvset",              STR_PARAM|USE_FUNC_PARAM, (void*)param_set_shvar },
	{"varset",              STR_PARAM|USE_FUNC_PARAM, (void*)param_set_var },
	{"avp_aliases",         STR_PARAM|USE_FUNC_PARAM, (void*)add_avp_aliases },
	{0,0,0}
};

static mi_export_t mi_cmds[] = {
	{ "shv_get",       mi_shvar_get,  0,                 0,  0 },
	{ "shv_set" ,      mi_shvar_set,  0,                 0,  0 },
	{ 0, 0, 0, 0, 0}
};

static int mod_init(void);
static void mod_destroy(void);
static int pv_isset(struct sip_msg* msg, char* pvid, char *foo);
static int pv_unset(struct sip_msg* msg, char* pvid, char *foo);

static cmd_export_t cmds[]={
	{"pv_isset",  (cmd_function)pv_isset,  1, fixup_pvar_null, 0, 
		ANY_ROUTE },
	{"pv_unset",  (cmd_function)pv_unset,  1, fixup_pvar_null, 0, 
		ANY_ROUTE },
#ifdef WITH_XAVP
	{"pv_xavp_print",  (cmd_function)pv_xavp_print,  0, 0, 0, 
		ANY_ROUTE },
#endif
	{0,0,0,0,0,0}
};



/** module exports */
struct module_exports exports= {
	"pv",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,
	0,          /* exported statistics */
	mi_cmds,    /* exported MI functions */
	mod_pvs,    /* exported pseudo-variables */
	0,          /* extra processes */
	mod_init,   /* module initialization function */
	0,
	mod_destroy,
	0           /* per-child init function */
};

static int mod_init(void)
{
	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	return 0;
}

static void mod_destroy(void)
{
	shvar_destroy_locks();
	destroy_shvars();
}

int mod_register(char *path, int *dlflags, void *p1, void *p2)
{
	return register_trans_mod(path, mod_trans);
}

static int pv_isset(struct sip_msg* msg, char* pvid, char *foo)
{
	pv_spec_t *sp;
	pv_value_t value;
	int ret;

	sp = (pv_spec_t*)pvid;
	if(pv_get_spec_value(msg, sp, &value)!=0)
		return -1;
	ret =1;
	if(value.flags & (PV_VAL_EMPTY|PV_VAL_NULL))
		ret = -1;
	pv_value_destroy(&value);
	return ret;
}

static int pv_unset(struct sip_msg* msg, char* pvid, char *foo)
{
	pv_spec_t *sp;
	
	sp = (pv_spec_t*)pvid;
	pv_set_spec_value(msg, sp, 0, NULL);

	return 1;
}

static int add_avp_aliases(modparam_t type, void* val)
{
	if (val!=0 && ((char*)val)[0]!=0)
	{
		if ( add_avp_galias_str((char*)val)!=0 )
			return -1;
	}

	return 0;
}

