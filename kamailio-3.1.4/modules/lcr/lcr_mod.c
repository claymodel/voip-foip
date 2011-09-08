/*
 * Least Cost Routing module
 *
 * Copyright (C) 2005-2010 Juha Heinanen
 * Copyright (C) 2006 Voice Sistem SRL
 *
 * This file is part of SIP Router, a free SIP server.
 *
 * SIP Router is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * SIP Router is distributed in the hope that it will be useful,
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
 *  2005-02-14: Introduced lcr module (jh)
 *  2005-02-20: Added sequential forking functions (jh)
 *  2005-02-25: Added support for int AVP names, combined addr and port
 *              AVPs (jh)
 *  2005-07-28: Added support for gw URI scheme and transport, 
 *              backport from ser (kd)
 *  2005-08-20: Added support for gw prefixes (jh)
 *  2005-09-03: Request-URI user part can be modified between load_gws()
 *              and first next_gw() calls.
 *  2008-10-10: Database values are now checked and from/to_gw functions
 *              execute in O(logN) time.
 *  2008-11-26: Added timer based check of gateways (shurik)
 *  2009-05-12  added RPC support (andrei)
 *  2009-06-21  Added support for more than one lcr instance and
                gw defunct capability (jh)
 */
/*!
 * \file
 * \brief SIP-router LCR :: Module interface
 * \ingroup lcr
 * Module: \ref lcr
 */

/*! \defgroup lcr SIP-router Least Cost Routing Module
 *
 * The Least Cost Routing (LCR) module implements capability to serially
 * forward a request to one or more gateways so that the order in which
 * the gateways is tried is based on admin defined "least cost" rules.

 * The LCR module supports many independent LCR instances (gateways and
 * least cost rules). Each such instance has its own LCR identifier.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pcre.h>
#include "../../locking.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../lib/srdb1/db.h"
#include "../../lib/kcore/km_ut.h"
#include "../../usr_avp.h"
#include "../../parser/parse_from.h"
#include "../../parser/msg_parser.h"
#include "../../action.h"
#include "../../qvalue.h"
#include "../../dset.h"
#include "../../ip_addr.h"
#include "../../resolve.h"
#include "../../mod_fix.h"
#include "../../socket_info.h"
#include "../../pvar.h"
#include "../../mod_fix.h"
#include "hash.h"
#include "lcr_rpc.h"
#include "../../rpc_lookup.h"

MODULE_VERSION

/*
 * versions of database tables required by the module.
 */
#define LCR_RULE_TABLE_VERSION 1
#define LCR_RULE_TARGET_TABLE_VERSION 1
#define LCR_GW_TABLE_VERSION 1

/* database defaults */

#define LCR_RULE_TABLE "lcr_rule"
#define LCR_RULE_TARGET_TABLE "lcr_rule_target"
#define LCR_GW_TABLE "lcr_gw"

#define ID_COL "id"
#define LCR_ID_COL "lcr_id"
#define PREFIX_COL "prefix"
#define FROM_URI_COL "from_uri"
#define STOPPER_COL "stopper"
#define ENABLED_COL "enabled"
#define RULE_ID_COL "rule_id"
#define PRIORITY_COL "priority"
#define GW_ID_COL "gw_id"
#define WEIGHT_COL "weight"
#define GW_NAME_COL "gw_name"
#define IP_ADDR_COL "ip_addr"
#define PORT_COL "port"
#define URI_SCHEME_COL "uri_scheme"
#define TRANSPORT_COL "transport"
#define PARAMS_COL "params"
#define HOSTNAME_COL "hostname"
#define STRIP_COL "strip"
#define TAG_COL "tag"
#define FLAGS_COL "flags"
#define DEFUNCT_COL "defunct"

/* Default module parameter values */
#define DEF_LCR_COUNT 1
#define DEF_LCR_RULE_HASH_SIZE 128
#define DEF_LCR_GW_COUNT 128
#define DEF_FETCH_ROWS 1024

/*
 * Type definitions
 */

struct matched_gw_info {
    unsigned short gw_index;
    unsigned short prefix_len;
    unsigned short priority;
    unsigned int weight;
    unsigned short duplicate;
};

/*
 * Database variables
 */
static db1_con_t* dbh = 0;   /* Database connection handle */
static db_func_t lcr_dbf;

/*
 * Locking variables
 */
gen_lock_t *reload_lock;

/*
 * Module parameter variables
 */

/* database variables */
static str db_url           = str_init(DEFAULT_RODB_URL);
static str lcr_rule_table   = str_init(LCR_RULE_TABLE);
static str lcr_rule_target_table = str_init(LCR_RULE_TARGET_TABLE);
static str lcr_gw_table     = str_init(LCR_GW_TABLE);
static str id_col           = str_init(ID_COL);
static str lcr_id_col       = str_init(LCR_ID_COL);
static str prefix_col       = str_init(PREFIX_COL);
static str from_uri_col     = str_init(FROM_URI_COL);
static str stopper_col      = str_init(STOPPER_COL);
static str enabled_col      = str_init(ENABLED_COL);
static str rule_id_col      = str_init(RULE_ID_COL);
static str priority_col     = str_init(PRIORITY_COL);
static str gw_id_col        = str_init(GW_ID_COL);
static str weight_col       = str_init(WEIGHT_COL);
static str gw_name_col      = str_init(GW_NAME_COL);
static str ip_addr_col      = str_init(IP_ADDR_COL);
static str port_col         = str_init(PORT_COL);
static str uri_scheme_col   = str_init(URI_SCHEME_COL);
static str transport_col    = str_init(TRANSPORT_COL);
static str params_col       = str_init(PARAMS_COL);
static str hostname_col     = str_init(HOSTNAME_COL);
static str strip_col        = str_init(STRIP_COL);
static str tag_col          = str_init(TAG_COL);
static str flags_col        = str_init(FLAGS_COL);
static str defunct_col      = str_init(DEFUNCT_COL);

/* number of rows to fetch at a shot */
static int fetch_rows_param = DEF_FETCH_ROWS;

/* avps */
static char *gw_uri_avp_param = NULL;
static char *ruri_user_avp_param = NULL;
static char *flags_avp_param = NULL;
static char *defunct_gw_avp_param = NULL;
static char *lcr_id_avp_param = NULL;

/* max number of lcr instances */
unsigned int lcr_count_param = DEF_LCR_COUNT;

/* size of lcr rules hash table */
unsigned int lcr_rule_hash_size_param = DEF_LCR_RULE_HASH_SIZE;

/* max no of gws */
unsigned int lcr_gw_count_param = DEF_LCR_GW_COUNT;

/* can gws be defuncted */
static unsigned int defunct_capability_param = 0;

/* dont strip or tag param */
static int dont_strip_or_tag_flag_param = -1;


/*
 * Other module types and variables
 */

static int     gw_uri_avp_type;
static int_str gw_uri_avp;
static int     ruri_user_avp_type;
static int_str ruri_user_avp;
static int     flags_avp_type;
static int_str flags_avp;
static int     defunct_gw_avp_type;
static int_str defunct_gw_avp;
static int     lcr_id_avp_type;
static int_str lcr_id_avp;

/* Pointer to rule hash table pointer table */
struct rule_info ***rule_pt = (struct rule_info ***)NULL;

/* Pointer to gw table pointer table */
struct gw_info **gw_pt = (struct gw_info **)NULL;

/*
 * Functions that are defined later
 */
static void destroy(void);
static int mod_init(void);
static int child_init(int rank);
static void free_shared_memory(void);

static int load_gws_1(struct sip_msg* _m, char* _s1, char* _s2);
static int load_gws_2(struct sip_msg* _m, char* _s1, char* _s2);
static int next_gw(struct sip_msg* _m, char* _s1, char* _s2);
static int defunct_gw(struct sip_msg* _m, char* _s1, char* _s2);
static int from_gw_1(struct sip_msg* _m, char* _s1, char* _s2);
static int from_gw_2(struct sip_msg* _m, char* _s1, char* _s2);
static int from_any_gw_0(struct sip_msg* _m, char* _s1, char* _s2);
static int from_any_gw_1(struct sip_msg* _m, char* _s1, char* _s2);
static int to_gw_1(struct sip_msg* _m, char* _s1, char* _s2);
static int to_gw_2(struct sip_msg* _m, char* _s1, char* _s2);
static int to_any_gw_0(struct sip_msg* _m, char* _s1, char* _s2);
static int to_any_gw_1(struct sip_msg* _m, char* _s1, char* _s2);

/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
    {"load_gws", (cmd_function)load_gws_1, 1, fixup_igp_null, 0,
     REQUEST_ROUTE | FAILURE_ROUTE},
    {"load_gws", (cmd_function)load_gws_2, 2, fixup_igp_pvar,
     fixup_free_igp_pvar, REQUEST_ROUTE | FAILURE_ROUTE},
    {"next_gw", (cmd_function)next_gw, 0, 0, 0,
     REQUEST_ROUTE | FAILURE_ROUTE},
    {"defunct_gw", (cmd_function)defunct_gw, 1, fixup_igp_null, 0,
     REQUEST_ROUTE | FAILURE_ROUTE},
    {"from_gw", (cmd_function)from_gw_1, 1, fixup_igp_null, 0,
     REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"from_gw", (cmd_function)from_gw_2, 2, fixup_igp_pvar,
     fixup_free_igp_pvar, REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"from_any_gw", (cmd_function)from_any_gw_0, 0, 0, 0,
     REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"from_any_gw", (cmd_function)from_any_gw_1, 1, fixup_pvar_null,
     fixup_free_pvar_null, REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"to_gw", (cmd_function)to_gw_1, 1, fixup_igp_null, 0,
     REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"to_gw", (cmd_function)to_gw_2, 2, fixup_igp_pvar,
     fixup_free_igp_pvar, REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"to_any_gw", (cmd_function)to_any_gw_0, 0, 0, 0,
     REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {"to_any_gw", (cmd_function)to_any_gw_1, 1, fixup_pvar_null,
     fixup_free_pvar_null, REQUEST_ROUTE | FAILURE_ROUTE | ONREPLY_ROUTE},
    {0, 0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
    {"db_url",                   STR_PARAM, &db_url.s},
    {"lcr_rule_table",           STR_PARAM, &lcr_rule_table.s},
    {"lcr_rule_target_table",    STR_PARAM, &lcr_rule_target_table.s},
    {"lcr_gw_table",             STR_PARAM, &lcr_gw_table.s},
    {"lcr_id_column",            STR_PARAM, &lcr_id_col.s},
    {"id_column",                STR_PARAM, &id_col.s},
    {"prefix_column",            STR_PARAM, &prefix_col.s},
    {"from_uri_column",          STR_PARAM, &from_uri_col.s},
    {"stopper_column",           STR_PARAM, &stopper_col.s},
    {"enabled_column",           STR_PARAM, &enabled_col.s},
    {"rule_id_column",           STR_PARAM, &rule_id_col.s},
    {"priority_column",          STR_PARAM, &priority_col.s},
    {"gw_id_column",             STR_PARAM, &gw_id_col.s},
    {"weight_column",            STR_PARAM, &weight_col.s},
    {"gw_name_column",           STR_PARAM, &gw_name_col.s},
    {"ip_addr_column",           STR_PARAM, &ip_addr_col.s},
    {"port_column",              STR_PARAM, &port_col.s},
    {"uri_scheme_column",        STR_PARAM, &uri_scheme_col.s},
    {"transport_column",         STR_PARAM, &transport_col.s},
    {"params_column",            STR_PARAM, &params_col.s},
    {"hostname_column",          STR_PARAM, &hostname_col.s},
    {"strip_column",             STR_PARAM, &strip_col.s},
    {"tag_column",               STR_PARAM, &tag_col.s},
    {"flags_column",             STR_PARAM, &flags_col.s},
    {"defunct_column",           STR_PARAM, &defunct_col.s},
    {"gw_uri_avp",               STR_PARAM, &gw_uri_avp_param},
    {"ruri_user_avp",            STR_PARAM, &ruri_user_avp_param},
    {"flags_avp",                STR_PARAM, &flags_avp_param},
    {"defunct_capability",       INT_PARAM, &defunct_capability_param},
    {"defunct_gw_avp",           STR_PARAM, &defunct_gw_avp_param},
    {"lcr_id_avp",               STR_PARAM, &lcr_id_avp_param},
    {"lcr_count",                INT_PARAM, &lcr_count_param},
    {"lcr_rule_hash_size",       INT_PARAM, &lcr_rule_hash_size_param},
    {"lcr_gw_count",             INT_PARAM, &lcr_gw_count_param},
    {"dont_strip_or_tag_flag",   INT_PARAM, &dont_strip_or_tag_flag_param},
    {"fetch_rows",               INT_PARAM, &fetch_rows_param},
    {0, 0, 0}
};

/*
 * Module interface
 */
struct module_exports exports = {
	"lcr", 
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,      /* Exported functions */
	params,    /* Exported parameters */
	0,         /* exported statistics */
	0,         /* exported MI functions */
	0,         /* exported pseudo-variables */
	0,         /* extra processes */
	mod_init,  /* module initialization function */
	0,         /* response function */
	destroy,   /* destroy function */
	child_init /* child initialization function */
};


static int lcr_db_init(const str* db_url)
{	
	if (lcr_dbf.init==0){
		LM_CRIT("null lcr_dbf\n");
		goto error;
	}
	if (dbh) {
	    LM_ERR("database is already connected\n");
	    goto error;
	}
	dbh=lcr_dbf.init(db_url);
	if (dbh==0){
		LM_ERR("unable to connect to the database\n");
		goto error;
	}
	return 0;
error:
	return -1;
}



static int lcr_db_bind(const str* db_url)
{
    if (db_bind_mod(db_url, &lcr_dbf)<0){
	LM_ERR("unable to bind to the database module\n");
	return -1;
    }

    if (!DB_CAPABILITY(lcr_dbf, DB_CAP_QUERY)) {
	LM_ERR("database module does not implement 'query' function\n");
	return -1;
    }

    return 0;
}


static void lcr_db_close(void)
{
	if (dbh && lcr_dbf.close){
		lcr_dbf.close(dbh);
		dbh=0;
	}
}


/*
 * Module initialization function that is called before the main process forks
 */
static int mod_init(void)
{
    pv_spec_t avp_spec;
    str s;
    unsigned short avp_flags;
    unsigned int i;

    /* Register RPC commands */
    if (rpc_register_array(lcr_rpc)!=0) {
	LM_ERR("failed to register RPC commands\n");
	return -1;
    }

    /* Update length of module variables */
    db_url.len = strlen(db_url.s);
    lcr_rule_table.len = strlen(lcr_rule_table.s);
    lcr_rule_target_table.len = strlen(lcr_rule_target_table.s);
    lcr_gw_table.len = strlen(lcr_gw_table.s);
    id_col.len = strlen(id_col.s);
    lcr_id_col.len = strlen(lcr_id_col.s);
    prefix_col.len = strlen(prefix_col.s);
    from_uri_col.len = strlen(from_uri_col.s);
    stopper_col.len = strlen(stopper_col.s);
    enabled_col.len = strlen(enabled_col.s);
    rule_id_col.len = strlen(rule_id_col.s);
    priority_col.len = strlen(priority_col.s);
    gw_id_col.len = strlen(gw_id_col.s);
    weight_col.len = strlen(weight_col.s);
    gw_name_col.len = strlen(gw_name_col.s);
    ip_addr_col.len = strlen(ip_addr_col.s);
    port_col.len = strlen(port_col.s);
    uri_scheme_col.len = strlen(uri_scheme_col.s);
    transport_col.len = strlen(transport_col.s);
    params_col.len = strlen(params_col.s);
    hostname_col.len = strlen(hostname_col.s);
    strip_col.len = strlen(strip_col.s);
    tag_col.len = strlen(tag_col.s);
    flags_col.len = strlen(flags_col.s);
    defunct_col.len = strlen(defunct_col.s);

    /* Bind database */
    if (lcr_db_bind(&db_url)) {
	LM_ERR("no database module found\n");
	return -1;
    }

    /* Check values of some params */
    if (lcr_count_param < 1) {
	LM_ERR("invalid lcr_count module parameter value <%d>\n",
	       lcr_count_param);
	return -1;
    }
    if (lcr_rule_hash_size_param < 1) {
	LM_ERR("invalid lcr_rule_hash_size module parameter value <%d>\n",
	       lcr_rule_hash_size_param);
	return -1;
    }
    if (lcr_gw_count_param < 1) {
	LM_ERR("invalid lcr_gw_count module parameter value <%d>\n",
	       lcr_gw_count_param);
	return -1;
    }
    if ((dont_strip_or_tag_flag_param != -1) &&
	!flag_in_range(dont_strip_or_tag_flag_param)) {
	LM_ERR("invalid dont_strip_or_tag_flag value <%d>\n",
	       dont_strip_or_tag_flag_param);
	return -1;
    }

    /* Process AVP params */

    if (gw_uri_avp_param && *gw_uri_avp_param) {
	s.s = gw_uri_avp_param; s.len = strlen(s.s);
	if (pv_parse_spec(&s, &avp_spec)==0
	    || avp_spec.type!=PVT_AVP) {
	    LM_ERR("malformed or non AVP definition <%s>\n", gw_uri_avp_param);
	    return -1;
	}
	
	if (pv_get_avp_name(0, &(avp_spec.pvp), &gw_uri_avp, &avp_flags) != 0) {
	    LM_ERR("invalid AVP definition <%s>\n", gw_uri_avp_param);
	    return -1;
	}
	gw_uri_avp_type = avp_flags;
    } else {
	LM_ERR("AVP gw_uri_avp has not been defined\n");
	return -1;
    }

    if (ruri_user_avp_param && *ruri_user_avp_param) {
	s.s = ruri_user_avp_param; s.len = strlen(s.s);
	if (pv_parse_spec(&s, &avp_spec)==0
	    || avp_spec.type!=PVT_AVP) {
	    LM_ERR("malformed or non AVP definition <%s>\n",
		   ruri_user_avp_param);
	    return -1;
	}
	
	if (pv_get_avp_name(0, &(avp_spec.pvp), &ruri_user_avp, &avp_flags)
	    != 0) {
	    LM_ERR("invalid AVP definition <%s>\n", ruri_user_avp_param);
	    return -1;
	}
	ruri_user_avp_type = avp_flags;
    } else {
	LM_ERR("AVP ruri_user_avp has not been defined\n");
	return -1;
    }

    if (flags_avp_param && *flags_avp_param) {
	s.s = flags_avp_param; s.len = strlen(s.s);
	if (pv_parse_spec(&s, &avp_spec)==0
	    || avp_spec.type!=PVT_AVP) {
	    LM_ERR("malformed or non AVP definition <%s>\n", flags_avp_param);
	    return -1;
	}
	
	if (pv_get_avp_name(0, &(avp_spec.pvp), &flags_avp, &avp_flags) != 0) {
	    LM_ERR("invalid AVP definition <%s>\n", flags_avp_param);
	    return -1;
	}
	flags_avp_type = avp_flags;
    } else {
	LM_ERR("AVP flags_avp has not been defined\n");
	return -1;
    }

    if (defunct_capability_param > 0) {
	if (defunct_gw_avp_param && *defunct_gw_avp_param) {
	    s.s = defunct_gw_avp_param; s.len = strlen(s.s);
	    if ((pv_parse_spec(&s, &avp_spec) == 0) ||
		(avp_spec.type != PVT_AVP)) {
		LM_ERR("malformed or non AVP definition <%s>\n",
		       defunct_gw_avp_param);
		return -1;
	    }
	    if (pv_get_avp_name(0, &(avp_spec.pvp), &defunct_gw_avp,
				&avp_flags) != 0) {
		LM_ERR("invalid AVP definition <%s>\n", defunct_gw_avp_param);
		return -1;
	    }
	    defunct_gw_avp_type = avp_flags;
	} else {
	    LM_ERR("AVP defunct_gw_avp has not been defined\n");
	    return -1;
	}
	if (lcr_id_avp_param && *lcr_id_avp_param) {
	    s.s = lcr_id_avp_param; s.len = strlen(s.s);
	    if ((pv_parse_spec(&s, &avp_spec) == 0) ||
		(avp_spec.type != PVT_AVP)) {
		LM_ERR("malformed or non AVP definition <%s>\n",
		       lcr_id_avp_param);
		return -1;
	    }
	    if (pv_get_avp_name(0, &(avp_spec.pvp), &lcr_id_avp,
				&avp_flags) != 0) {
		LM_ERR("invalid AVP definition <%s>\n", lcr_id_avp_param);
		return -1;
	    }
	    lcr_id_avp_type = avp_flags;
	} else {
	    LM_ERR("AVP lcr_id_avp has not been defined\n");
	    return -1;
	}
    }

    if (fetch_rows_param < 1) {
	LM_ERR("invalid fetch_rows module parameter value <%d>\n",
	       fetch_rows_param);
	return -1;
    }

    /* Check table version */
    if (lcr_db_init(&db_url) < 0) {
	LM_ERR("unable to open database connection\n");
	return -1;
    }
    if ((db_check_table_version(&lcr_dbf, dbh, &lcr_rule_table,
				LCR_RULE_TABLE_VERSION) < 0) ||
	(db_check_table_version(&lcr_dbf, dbh, &lcr_rule_target_table,
				LCR_RULE_TARGET_TABLE_VERSION) < 0) ||
	(db_check_table_version(&lcr_dbf, dbh, &lcr_gw_table,
				LCR_GW_TABLE_VERSION) < 0)) {
	LM_ERR("error during table version check\n");
	lcr_db_close();
	goto err;
    }
    lcr_db_close();

    /* rule shared memory */

    /* rule hash table pointer table */
    /* pointer at index 0 points to temp rule hash table */
    rule_pt = (struct rule_info ***)shm_malloc(sizeof(struct rule_info **) *
					       (lcr_count_param + 1));
    if (rule_pt == 0) {
	LM_ERR("no memory for rule hash table pointer table\n");
	goto err;
    }
    memset(rule_pt, 0, sizeof(struct rule_info **) * (lcr_count_param + 1));

    /* rules hash tables */
    /* last entry in hash table contains list of different prefix lengths */
    for (i = 0; i <= lcr_count_param; i++) {
	rule_pt[i] = (struct rule_info **)
	    shm_malloc(sizeof(struct rule_info *) *
		       (lcr_rule_hash_size_param + 1));
	if (rule_pt[i] == 0) {
	    LM_ERR("no memory for rules hash table\n");
	    goto err;
	}
	memset(rule_pt[i], 0, sizeof(struct rule_info *) *
	       (lcr_rule_hash_size_param + 1));
    }
    /* gw shared memory */

    /* gw table pointer table */
    /* pointer at index 0 points to temp rule hash table */
    gw_pt = (struct gw_info **)shm_malloc(sizeof(struct gw_info *) *
					  (lcr_count_param + 1));
    if (gw_pt == 0) {
	LM_ERR("no memory for gw table pointer table\n");
	goto err;
    }
    memset(gw_pt, 0, sizeof(struct gw_info *) * (lcr_count_param + 1));

    /* gw tables themselves */
    /* ordered by <ip_addr, port> for from_gw/to_gw functions */
    /* in each table i, (gw_pt[i])[0].ip_addr contains number of
       gateways in the table and (gw_pt[i])[0].port as value 1
       if some gateways in the table have null ip addr */
    for (i = 0; i <= lcr_count_param; i++) {
	gw_pt[i] = (struct gw_info *)shm_malloc(sizeof(struct gw_info) *
						(lcr_gw_count_param + 1));
	if (gw_pt[i] == 0) {
	    LM_ERR("no memory for gw table\n");
	    goto err;
	}
	memset(gw_pt[i], 0, sizeof(struct gw_info *) *
	       (lcr_gw_count_param + 1));
    }

    /* Allocate and initialize locks */
    reload_lock = lock_alloc();
    if (reload_lock == NULL) {
	LM_ERR("cannot allocate reload_lock\n");
	goto err;
    }
    if (lock_init(reload_lock) == NULL) {
	LM_ERR("cannot init reload_lock\n");
	goto err;
    }

    /* First reload */
    lock_get(reload_lock);
    if (reload_tables() < 0) {
	lock_release(reload_lock);
	LM_CRIT("failed to reload lcr tables\n");
	goto err;
    }
    lock_release(reload_lock);

    return 0;

err:
    free_shared_memory();
    return -1;
}


/* Module initialization function called in each child separately */
static int child_init(int rank)
{
    return 0;
}


static void destroy(void)
{
    lcr_db_close();

    free_shared_memory();
}


/* Free shared memory */
static void free_shared_memory(void)
{
    int i;
    for (i = 0; i <= lcr_count_param; i++) {
	if (rule_pt && rule_pt[i]) {
	    rule_hash_table_contents_free(rule_pt[i]);
	    shm_free(rule_pt[i]);
	    rule_pt[i] = 0;
	}
    }
    if (rule_pt) {
	shm_free(rule_pt);
	rule_pt = 0;
    }
    for (i = 0; i <= lcr_count_param; i++) {
	if (gw_pt && gw_pt[i]) {
	    shm_free(gw_pt[i]);
	    gw_pt[i] = 0;
	}
    }
    if (gw_pt) {
	shm_free(gw_pt);
	gw_pt = 0;
    }
    if (reload_lock) {
	lock_destroy(reload_lock);
	lock_dealloc(reload_lock);
	reload_lock=0;
    }
}
   
/*
 * Compare matched gateways based on prefix_len, priority, and randomized
 * weight.
 */
static int comp_matched(const void *m1, const void *m2)
{
    struct matched_gw_info *mi1 = (struct matched_gw_info *) m1;
    struct matched_gw_info *mi2 = (struct matched_gw_info *) m2;

    /* Sort by prefix_len */
    if (mi1->prefix_len > mi2->prefix_len) return 1;
    if (mi1->prefix_len == mi2->prefix_len) {
	/* Sort by priority */
	if (mi1->priority < mi2->priority) return 1;
	if (mi1->priority == mi2->priority) {
	    /* Sort by randomized weigth */
	    if (mi1->weight > mi2->weight) return 1;
	    if (mi1->weight == mi2->weight) return 0;
	    return -1;
	}
	return -1;
    }
    return -1;
}


/* Compile pattern into shared memory and return pointer to it. */
static pcre *reg_ex_comp(const char *pattern)
{
    pcre *re, *result;
    const char *error;
    int rc, size, err_offset;

    re = pcre_compile(pattern, 0, &error, &err_offset, NULL);
    if (re == NULL) {
	LM_ERR("pcre compilation of '%s' failed at offset %d: %s\n",
	       pattern, err_offset, error);
	return (pcre *)0;
    }
    rc = pcre_fullinfo(re, NULL, PCRE_INFO_SIZE, &size);
    if (rc != 0) {
	LM_ERR("pcre_fullinfo on compiled pattern '%s' yielded error: %d\n",
	       pattern, rc);
	return (pcre *)0;
    }
    result = (pcre *)shm_malloc(size);
    if (result == NULL) {
	pcre_free(re);
	LM_ERR("not enough shared memory for compiled PCRE pattern\n");
	return (pcre *)0;
    }
    memcpy(result, re, size);
    pcre_free(re);
    return result;
}


/*
 * Compare gateways based on their IP address
 */
static int comp_gws(const void *_g1, const void *_g2)
{
    struct gw_info *g1 = (struct gw_info *)_g1;
    struct gw_info *g2 = (struct gw_info *)_g2;

    if (g1->ip_addr < g2->ip_addr) return -1;
    if (g1->ip_addr > g2->ip_addr) return 1;

    return 0;
}


/*
 * Check if ip_addr/grp_id of gateway is unique.
 */
static int gw_unique(const struct gw_info *gws, const unsigned int count,
		     const unsigned int ip_addr, const unsigned int port,
		     char *hostname, unsigned int hostname_len)
{
    unsigned int i;

    for (i = 1; i <= count; i++) {
	if ((gws[i].ip_addr == ip_addr) &&
	    (gws[i].port == port) &&
	    (gws[i].hostname_len == hostname_len) &&
	    (strncasecmp(gws[i].hostname, hostname, hostname_len) == 0)) {
	    return 0;
	}
    }
    return 1;
}

static int insert_gw(struct gw_info *gws, unsigned int i, unsigned int gw_id,
		     char *gw_name, unsigned int gw_name_len,
		     unsigned int scheme, unsigned int ip_addr,
		     unsigned int port, unsigned int transport,
		     char *params, unsigned int params_len,
		     char *hostname, unsigned int hostname_len,
		     char *ip_string, unsigned int strip, char *tag,
		     unsigned int tag_len, unsigned int flags,
		     unsigned int defunct_until)
{
    if (gw_unique(gws, i - 1, ip_addr, port, hostname, hostname_len) == 0) {
	LM_ERR("gw <%s, %u, %.*s> is not unique\n", ip_string, port,
	       hostname_len, hostname);
	return 0;
    }
    gws[i].gw_id = gw_id;
    if (gw_name_len) memcpy(&(gws[i].gw_name[0]), gw_name, gw_name_len);
    gws[i].gw_name_len = gw_name_len;
    gws[i].scheme = scheme;
    gws[i].ip_addr = ip_addr;
    gws[i].port = port;
    gws[i].transport = transport;
    if (params_len) memcpy(&(gws[i].params[0]), params, params_len);
    gws[i].params_len = params_len;
    if (hostname_len) memcpy(&(gws[i].hostname[0]), hostname, hostname_len);
    gws[i].hostname_len = hostname_len;
    gws[i].strip = strip;
    gws[i].tag_len = tag_len;
    if (tag_len) memcpy(&(gws[i].tag[0]), tag, tag_len);
    gws[i].flags = flags;
    gws[i].defunct_until = defunct_until;
    LM_DBG("inserted gw <%u, %.*s, %s, %u, %.*s> at index %u\n", gw_id,
	   gw_name_len, gw_name, ip_string, port, hostname_len, hostname, i);
    return 1;
}


/*
 * Insert prefix_len into list pointed by last rule hash table entry 
 * if not there already. Keep list in decending prefix_len order.
 */
static int prefix_len_insert(struct rule_info **table,
			     unsigned short prefix_len)
{
    struct rule_info *lcr_rec, **previous, *this;
    
    previous = &(table[lcr_rule_hash_size_param]);
    this = table[lcr_rule_hash_size_param];

    while (this) {
	if (this->prefix_len == prefix_len)
	    return 1;
	if (this->prefix_len < prefix_len) {
	    lcr_rec = shm_malloc(sizeof(struct rule_info));
	    if (lcr_rec == NULL) {
		LM_ERR("no shared memory for rule_info\n");
		return 0;
	    }
	    memset(lcr_rec, 0, sizeof(struct rule_info));
	    lcr_rec->prefix_len = prefix_len;
	    lcr_rec->next = this;
	    *previous = lcr_rec;
	    return 1;
	}
	previous = &(this->next);
	this = this->next;
    }

    lcr_rec = shm_malloc(sizeof(struct rule_info));
    if (lcr_rec == NULL) {
	LM_ERR("no shared memory for rule_info\n");
	return 0;
    }
    memset(lcr_rec, 0, sizeof(struct rule_info));
    lcr_rec->prefix_len = prefix_len;
    lcr_rec->next = NULL;
    *previous = lcr_rec;
    return 1;
}


/*
 * Reload gws to unused gw table, rules to unused lcr hash table, and
 * prefix lens to a new prefix_len list.  When done, make these tables
 * and list the current ones.
 */
int reload_tables()
{
    unsigned int i, n, lcr_id, rule_id, gw_id, gw_name_len, port, strip,
	tag_len, prefix_len, from_uri_len, stopper, enabled, flags, gw_cnt,
	hostname_len, params_len, defunct_until, null_gw_ip_addr, priority,
	weight, tmp;
    struct in_addr ip_addr;
    uri_type scheme;
    uri_transport transport;
    char *gw_name, *ip_string, *hostname, *tag, *prefix, *from_uri, *params;
    db1_res_t* res = NULL;
    db_row_t* row;
    db_key_t key_cols[1];
    db_op_t op[1];
    db_val_t vals[1];
    db_key_t gw_cols[12];
    db_key_t rule_cols[5];
    db_key_t target_cols[4];
    pcre *from_uri_re;
    struct gw_info *gws, *gw_pt_tmp;
    struct rule_info **rules, **rule_pt_tmp;

    key_cols[0] = &lcr_id_col;
    op[0] = OP_EQ;
    VAL_TYPE(vals) = DB1_INT;
    VAL_NULL(vals) = 0;

    rule_cols[0] = &id_col;
    rule_cols[1] = &prefix_col;
    rule_cols[2] = &from_uri_col;
    rule_cols[3] = &stopper_col;
    rule_cols[4] = &enabled_col;
	
    gw_cols[0] = &gw_name_col;
    gw_cols[1] = &ip_addr_col;
    gw_cols[2] = &port_col;
    gw_cols[3] = &uri_scheme_col;
    gw_cols[4] = &transport_col;
    gw_cols[5] = &params_col;
    gw_cols[6] = &hostname_col;
    gw_cols[7] = &strip_col;
    gw_cols[8] = &tag_col;
    gw_cols[9] = &flags_col;
    gw_cols[10] = &defunct_col;
    gw_cols[11] = &id_col;

    target_cols[0] = &rule_id_col;
    target_cols[1] = &gw_id_col;
    target_cols[2] = &priority_col;
    target_cols[3] = &weight_col;

    from_uri_re = 0;

    if (lcr_db_init(&db_url) < 0) {
	LM_ERR("unable to open database connection\n");
	return -1;
    }

    for (lcr_id = 1; lcr_id <= lcr_count_param; lcr_id++) {

	/* reload rules */

	rules = rule_pt[0];
	rule_hash_table_contents_free(rules);

	if (lcr_dbf.use_table(dbh, &lcr_rule_table) < 0) {
	    LM_ERR("error while trying to use lcr_rule table\n");
	    goto err;
	}

	VAL_INT(vals) = lcr_id;
	if (DB_CAPABILITY(lcr_dbf, DB_CAP_FETCH)) {
	    if (lcr_dbf.query(dbh, key_cols, op, vals, rule_cols, 1, 5, 0, 0)
		< 0) {
		LM_ERR("db query on lcr_rule table failed\n");
		goto err;
	    }
	    if (lcr_dbf.fetch_result(dbh, &res, fetch_rows_param) < 0) {
		LM_ERR("failed to fetch rows from lcr_rule table\n");
		goto err;
	    }
	} else {
	    if (lcr_dbf.query(dbh, key_cols, op, vals, rule_cols, 1, 5, 0, &res)
		< 0) {
		LM_ERR("db query on lcr_rule table failed\n");
		goto err;
	    }
	}

	n = 0;
	from_uri_re = 0;
    
	do {
	    LM_DBG("loading, cycle %d with <%d> rows", n++, RES_ROW_N(res));
	    for (i = 0; i < RES_ROW_N(res); i++) {

		from_uri_re = 0;
		row = RES_ROWS(res) + i;

		if ((VAL_NULL(ROW_VALUES(row)) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row)) != DB1_INT)) {
		    LM_ERR("lcr rule id at row <%u> is null or not int\n", i);
		    goto err;
		}
		rule_id = (unsigned int)VAL_INT(ROW_VALUES(row));

		enabled = (unsigned int)VAL_INT(ROW_VALUES(row) + 4);
		if ((enabled != 0) && (enabled != 1)) {
		    LM_ERR("lcr rule <%u> enabled is not 0 or 1\n", rule_id);
		    goto err;
		}
		if (enabled == 0) {
		    LM_DBG("skipping disabled lcr rule <%u>\n", rule_id);
		    continue;
		}

		if (VAL_NULL(ROW_VALUES(row) + 1) == 1) {
		    prefix_len = 0;
		    prefix = 0;
		} else {
		    if (VAL_TYPE(ROW_VALUES(row) + 1) != DB1_STRING) {
			LM_ERR("lcr rule <%u> prefix is not string\n", rule_id);
			goto err;
		    }
		    prefix = (char *)VAL_STRING(ROW_VALUES(row) + 1);
		    prefix_len = strlen(prefix);
		}
		if (prefix_len > MAX_PREFIX_LEN) {
		    LM_ERR("lcr rule <%u> prefix is too long\n", rule_id);
		    goto err;
		}

		if ((VAL_NULL(ROW_VALUES(row) + 3) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row) + 3) != DB1_INT)) {
		    LM_ERR("lcr rule <%u> stopper is NULL or not int\n",
			   rule_id);
		    goto err;
		}
		stopper = (unsigned int)VAL_INT(ROW_VALUES(row) + 3);
		if ((stopper != 0) && (stopper != 1)) {
		    LM_ERR("lcr rule <%u> stopper is not 0 or 1\n", rule_id);
		    goto err;
		}

		if ((VAL_NULL(ROW_VALUES(row) + 4) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row) + 4) != DB1_INT)) {
		    LM_ERR("lcr rule <%u> enabled is NULL or not int\n",
			   rule_id);
		    goto err;
		}

		if (VAL_NULL(ROW_VALUES(row) + 2) == 1) {
		    from_uri_len = 0;
		    from_uri = 0;
		} else {
		    if (VAL_TYPE(ROW_VALUES(row) + 2) != DB1_STRING) {
			LM_ERR("lcr rule <%u> from_uri is not string\n",
			       rule_id);
			goto err;
		    }
		    from_uri = (char *)VAL_STRING(ROW_VALUES(row) + 2);
		    from_uri_len = strlen(from_uri);
		}
		if (from_uri_len > MAX_URI_LEN) {
		    LM_ERR("lcr rule <%u> from_uri is too long\n", rule_id);
		    goto err;
		}
		if (from_uri_len > 0) {
		    from_uri_re = reg_ex_comp(from_uri);
		    if (from_uri_re == 0) {
			LM_ERR("failed to compile lcr rule <%u> from_uri "
			       "<%s>\n", rule_id, from_uri);
			goto err;
		    }
		} else {
		    from_uri_re = 0;
		}

		if (!rule_hash_table_insert(rules, lcr_id, rule_id, prefix_len,
					    prefix, from_uri_len, from_uri,
					    from_uri_re, stopper) ||
		    !prefix_len_insert(rules, prefix_len)) {
		    goto err;
		}
	    }

	    if (DB_CAPABILITY(lcr_dbf, DB_CAP_FETCH)) {
		if (lcr_dbf.fetch_result(dbh, &res, fetch_rows_param) < 0) {
		    LM_ERR("fetching of rows from lcr_rule table failed\n");
		    goto err;
		}
	    } else {
		break;
	    }

	} while (RES_ROW_N(res) > 0);

	lcr_dbf.free_result(dbh, res);
	res = NULL;

	/* Reload gws */

	gws = gw_pt[0];

	if (lcr_dbf.use_table(dbh, &lcr_gw_table) < 0) {
	    LM_ERR("error while trying to use lcr_gw table\n");
	    goto err;
	}

	VAL_INT(vals) = lcr_id;
	if (lcr_dbf.query(dbh, key_cols, op, vals, gw_cols, 1, 12, 0, &res)
	    < 0) {
	    LM_ERR("failed to query gw data\n");
	    goto err;
	}

	if (RES_ROW_N(res) + 1 > lcr_gw_count_param) {
	    LM_ERR("too many gateways\n");
	    goto err;
	}

	null_gw_ip_addr = gw_cnt = 0;

	for (i = 0; i < RES_ROW_N(res); i++) {
	    row = RES_ROWS(res) + i;
	    if ((VAL_NULL(ROW_VALUES(row) + 11) == 1) ||
		(VAL_TYPE(ROW_VALUES(row) + 11) != DB1_INT)) {
		LM_ERR("lcr_gw id at row <%u> is null or not int\n", i);
		goto err;
	    }
	    gw_id = (unsigned int)VAL_INT(ROW_VALUES(row) + 11);
	    if (VAL_NULL(ROW_VALUES(row) + 10)) {
		defunct_until = 0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 10) != DB1_INT) {
		    LM_ERR("lcr_gw defunct at row <%u> is not int\n", i);
		    goto err;
		}
		defunct_until = (unsigned int)VAL_INT(ROW_VALUES(row) + 10);
		if (defunct_until > 4294967294UL) {
		    LM_DBG("skipping disabled gw <%u>\n", gw_id);
		    continue;
		}
	    }
	    if (!VAL_NULL(ROW_VALUES(row)) &&
		(VAL_TYPE(ROW_VALUES(row)) != DB1_STRING)) {
		LM_ERR("lcr_gw gw_name at row <%u> is not null or string\n", i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row))) {
		gw_name_len = 0;
		gw_name = (char *)0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row)) != DB1_STRING) {
		    LM_ERR("lcr_gw gw_name at row <%u> is not string\n", i);
		    goto err;
		}
		gw_name = (char *)VAL_STRING(ROW_VALUES(row));
		gw_name_len = strlen(gw_name);
	    }
	    if (gw_name_len > MAX_NAME_LEN) {
		LM_ERR("lcr_gw gw_name <%u> at row <%u> it too long\n",
		       gw_name_len, i);
		goto err;
	    }
	    if (!VAL_NULL(ROW_VALUES(row) + 1) &&
		(VAL_TYPE(ROW_VALUES(row) + 1) != DB1_STRING)) {
		LM_ERR("lcr_gw ip_addr at row <%u> is not null or string\n",
		       i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 1)) {
		ip_string = (char *)0;
		ip_addr.s_addr = 0;
		null_gw_ip_addr = 1;
	    } else {
		ip_string = (char *)VAL_STRING(ROW_VALUES(row) + 1);
		if (inet_aton(ip_string, &ip_addr) == 0) {
		    LM_ERR("lcr_gw ip_addr <%s> at row <%u> is invalid\n",
			   ip_string, i);
		    goto err;
		}
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 2)) {
		port = 0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 2) != DB1_INT) {
		    LM_ERR("lcr_gw port at row <%u> is not int\n", i);
		    goto err;
		}
		port = (unsigned int)VAL_INT(ROW_VALUES(row) + 2);
	    }
	    if (port > 65536) {
		LM_ERR("lcr_gw port <%d> at row <%u> is too large\n", port, i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 3)) {
		scheme = SIP_URI_T;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 3) != DB1_INT) {
		    LM_ERR("lcr_gw uri scheme at row <%u> is not int\n", i);
		    goto err;
		}
		scheme = (uri_type)VAL_INT(ROW_VALUES(row) + 3);
	    }
	    if ((scheme != SIP_URI_T) && (scheme != SIPS_URI_T)) {
		LM_ERR("lcr_gw has unknown or unsupported URI scheme <%u> at "
		       "row <%u>\n", (unsigned int)scheme, i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 4)) {
		transport = PROTO_NONE;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 4) != DB1_INT) {
		    LM_ERR("lcr_gw transport at row <%u> is not int\n", i);
		    goto err;
		}
		transport = (uri_transport)VAL_INT(ROW_VALUES(row) + 4);
	    }
	    if ((transport != PROTO_UDP) && (transport != PROTO_TCP) &&
		(transport != PROTO_TLS) && (transport != PROTO_SCTP) &&
		(transport != PROTO_NONE)) {
		LM_ERR("lcr_gw has unknown or unsupported transport <%u> at "
		       " row <%u>\n", (unsigned int)transport, i);
		goto err;
	    }
	    if ((scheme == SIPS_URI_T) && (transport == PROTO_UDP)) {
		LM_ERR("lcr_gw has wrong transport <%u> for SIPS URI "
		       "scheme at row <%u>\n", transport, i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 5)) {
		params_len = 0;
		params = (char *)0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 5) != DB1_STRING) {
		    LM_ERR("lcr_gw params at row <%u> is not string\n", i);
		    goto err;
		}
		params = (char *)VAL_STRING(ROW_VALUES(row) + 5);
		params_len = strlen(params);
		if ((params_len > 0) && (params[0] != ';')) {
		    LM_ERR("lcr_gw params at row <%u> does not start "
			   "with ';'\n", i);
		    goto err;
		}
	    }
	    if (params_len > MAX_PARAMS_LEN) {
		LM_ERR("lcr_gw params length <%u> at row <%u> it too large\n",
		       params_len, i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 6)) {
		if (ip_string == 0) {
		    LM_ERR("lcr_gw gw ip_addr and hostname are both null "
			   "at row <%u>\n", i);
		    goto err;
		}
		hostname_len = 0;
		hostname = (char *)0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 6) != DB1_STRING) {
		    LM_ERR("hostname at row <%u> is not string\n", i);
		    goto err;
		}
		hostname = (char *)VAL_STRING(ROW_VALUES(row) + 6);
		hostname_len = strlen(hostname);
	    }
	    if (hostname_len > MAX_HOST_LEN) {
		LM_ERR("lcr_gw hostname at row <%u> it too long\n", i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 7)) {
		strip = 0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 7) != DB1_INT) {
		    LM_ERR("lcr_gw strip count at row <%u> is not int\n", i);
		    goto err;
		}
		strip = (unsigned int)VAL_INT(ROW_VALUES(row) + 7);
	    }
	    if (strip > MAX_USER_LEN) {
		LM_ERR("lcr_gw strip count <%u> at row <%u> it too large\n",
		       strip, i);
		goto err;
	    }
	    if (VAL_NULL(ROW_VALUES(row) + 8)) {
		tag_len = 0;
		tag = (char *)0;
	    } else {
		if (VAL_TYPE(ROW_VALUES(row) + 8) != DB1_STRING) {
		    LM_ERR("lcr_gw tag at row <%u> is not string\n", i);
		    goto err;
		}
		tag = (char *)VAL_STRING(ROW_VALUES(row) + 8);
		tag_len = strlen(tag);
	    }
	    if (tag_len > MAX_TAG_LEN) {
		LM_ERR("lcr_gw tag at row <%u> it too long\n", i);
		goto err;
	    }
	    if ((VAL_NULL(ROW_VALUES(row) + 9) == 1) ||
		(VAL_TYPE(ROW_VALUES(row) + 9) != DB1_INT)) {
		LM_ERR("lcr_gw flags at row <%u> is null or not int\n", i);
		goto err;
	    }
	    flags = (unsigned int)VAL_INT(ROW_VALUES(row) + 9);
	    gw_cnt++;
	    if (!insert_gw(gws, gw_cnt, gw_id, gw_name, gw_name_len,
			   scheme, (unsigned int)ip_addr.s_addr, port,
			   transport, params, params_len, hostname,
			   hostname_len, ip_string, strip, tag, tag_len, flags,
			   defunct_until)) {
		goto err;
	    }
	}

	lcr_dbf.free_result(dbh, res);
	res = NULL;

	qsort(&(gws[1]), gw_cnt, sizeof(struct gw_info), comp_gws);
	gws[0].ip_addr = gw_cnt;
	gws[0].port = null_gw_ip_addr;

	/* Reload targets */

	if (lcr_dbf.use_table(dbh, &lcr_rule_target_table) < 0) {
	    LM_ERR("error while trying to use lcr_rule_target table\n");
	    goto err;
	}

	VAL_INT(vals) = lcr_id;
	if (DB_CAPABILITY(lcr_dbf, DB_CAP_FETCH)) {
	    if (lcr_dbf.query(dbh, key_cols, op, vals, target_cols, 1, 4, 0, 0)
		< 0) {
		LM_ERR("db query on lcr_rule_target table failed\n");
		goto err;
	    }
	    if (lcr_dbf.fetch_result(dbh, &res, fetch_rows_param) < 0) {
		LM_ERR("failed to fetch rows from lcr_rule_target table\n");
		goto err;
	    }
	} else {
	    if (lcr_dbf.query(dbh, key_cols, op, vals, target_cols, 1, 4, 0,
			      &res) < 0) {
		LM_ERR("db query on lcr_rule_target table failed\n");
		goto err;
	    }
	}

	n = 0;
	do {
	    LM_DBG("loading, cycle %d with <%d> rows", n++, RES_ROW_N(res));
	    for (i = 0; i < RES_ROW_N(res); i++) {
		row = RES_ROWS(res) + i;
		if ((VAL_NULL(ROW_VALUES(row)) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row)) != DB1_INT)) {
		    LM_ERR("lcr_rule_target rule_id at row <%u> is null "
			   "or not int\n", i);
		    goto err;
		}
		rule_id = (unsigned int)VAL_INT(ROW_VALUES(row));
		if ((VAL_NULL(ROW_VALUES(row) + 1) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row) + 1) != DB1_INT)) {
		    LM_ERR("lcr_rule_target gw_id at row <%u> is null "
			   "or not int\n", i);
		    goto err;
		}
		gw_id = (unsigned int)VAL_INT(ROW_VALUES(row) + 1);
		if ((VAL_NULL(ROW_VALUES(row) + 2) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row) + 2) != DB1_INT)) {
		    LM_ERR("lcr_rule_target priority at row <%u> is null "
			   "or not int\n", i);
		    goto err;
		}
		priority = (unsigned int)VAL_INT(ROW_VALUES(row) + 2);
		if (priority > 255) {
		    LM_ERR("lcr_rule_target priority value at row <%u> is "
			   "not 0-255\n", i);
		    goto err;
		}
		if ((VAL_NULL(ROW_VALUES(row) + 3) == 1) ||
		    (VAL_TYPE(ROW_VALUES(row) + 3) != DB1_INT)) {
		    LM_ERR("lcr_rule_target weight at row <%u> is null "
			   "or not int\n", i);
		    goto err;
		}
		weight = (unsigned int)VAL_INT(ROW_VALUES(row) + 3);
		if ((weight < 1) || (weight > 254)) {
		    LM_ERR("lcr_rule_target weight value at row <%u> is "
			   "not 1-254\n", i);
		    goto err;
		}
		tmp = rule_hash_table_insert_target(rules, gws, rule_id, gw_id,
						    priority, weight);
		if (tmp == 2) {
		    LM_INFO("skipping disabled <gw/rule> = <%u/%u>\n",
			    gw_id, rule_id);
		} else if (tmp != 1) {
		    LM_ERR("could not insert target to rule <%u>\n", rule_id);
		    goto err;
		}
	    }
	    if (DB_CAPABILITY(lcr_dbf, DB_CAP_FETCH)) {
		if (lcr_dbf.fetch_result(dbh, &res, fetch_rows_param) < 0) {
		    LM_ERR("fetching of rows from lcr_rule_target table "
			   "failed\n");
		    goto err;
		}
	    } else {
		break;
	    }
	} while (RES_ROW_N(res) > 0);

	lcr_dbf.free_result(dbh, res);
	res = NULL;

	/* swap tables */
	rule_pt_tmp = rule_pt[lcr_id];
	gw_pt_tmp = gw_pt[lcr_id];
	rule_pt[lcr_id] = rules;
	gw_pt[lcr_id] = gws;
	rule_pt[0] = rule_pt_tmp;
	gw_pt[0] = gw_pt_tmp;
    }

    lcr_db_close();
    return 1;

 err:
    lcr_dbf.free_result(dbh, res);
    lcr_db_close();
    return -1;
}


inline int encode_avp_value(char *value, unsigned int gw_index, uri_type scheme,
			    unsigned int strip, char *tag, unsigned int tag_len,
			    unsigned int ip_addr, char *hostname,
			    unsigned int hostname_len, unsigned int port,
			    char *params, unsigned int params_len,
			    uri_transport transport, unsigned int flags)
{
    char *at, *string;
    int len;

    at = value;

    /* gw index */
    string = int2str(gw_index, &len);
    append_str(at, string, len);
    append_chr(at, '|');
    /* scheme */
    string = int2str(scheme, &len);
    append_str(at, string, len);
    append_chr(at, '|');
    /* strip */
    string = int2str(strip, &len);
    append_str(at, string, len);
    append_chr(at, '|');
    /* tag */
    append_str(at, tag, tag_len);
    append_chr(at, '|');
    /* ip_addr */
    if (ip_addr > 0) {
	string = int2str(ip_addr, &len);
	append_str(at, string, len);
    }
    append_chr(at, '|');
    /* hostname */
    append_str(at, hostname, hostname_len);
    append_chr(at, '|');
    /* port */
    if (port > 0) {
	string = int2str(port, &len);
	append_str(at, string, len);
    }
    append_chr(at, '|');
    /* params */
    append_str(at, params, params_len);
    append_chr(at, '|');
    /* transport */
    string = int2str(transport, &len);
    append_str(at, string, len);
    append_chr(at, '|');
    /* flags */
    string = int2str(flags, &len);
    append_str(at, string, len);
    return at - value;
}

inline int decode_avp_value(char *value, unsigned int *gw_index, str *scheme,
			    unsigned int *strip, str *tag, unsigned int *addr,
			    str *hostname, str *port, str *params,
			    str *transport, unsigned int *flags)
{
    unsigned int u;
    str s;
    char *sep;

    /* gw index */
    s.s = value;
    sep = index(s.s, '|');
    if (sep == NULL) {
	LM_ERR("index was not found in AVP value\n");
	return 0;
    }
    s.len = sep - s.s;
    str2int(&s, gw_index);
    /* scheme */
    s.s = sep + 1;
    sep = index(s.s, '|');
    if (sep == NULL) {
	LM_ERR("scheme was not found in AVP value\n");
	return 0;
    }
    s.len = sep - s.s;
    str2int(&s, &u);
    if (u == SIP_URI_T) {
	scheme->s = "sip:";
	scheme->len = 4;
    } else {
	scheme->s = "sips:";
	scheme->len = 5;
    }
    /* strip */
    s.s = sep + 1;
    sep = index(s.s, '|');
    if (sep == NULL) {
	LM_ERR("strip was not found in AVP value\n");
	return 0;
    }
    s.len = sep - s.s;
    str2int(&s, strip);
    /* tag */
    tag->s = sep + 1;
    sep = index(tag->s, '|');
    if (sep == NULL) {
	LM_ERR("tag was not found in AVP value\n");
	return 0;
    }
    tag->len = sep - tag->s;
    /* addr */
    s.s = sep + 1;
    sep = index(s.s, '|');
    if (sep == NULL) {
	LM_ERR("ip_addr was not found in AVP value\n");
	return 0;
    }
    s.len = sep - s.s;
    if (s.len > 0) {
	str2int(&s, addr);
    } else {
	*addr = 0;
    }
    /* hostname */
    hostname->s = sep + 1;
    sep = index(hostname->s, '|');
    if (sep == NULL) {
	LM_ERR("hostname was not found in AVP value\n");
	return 0;
    }
    hostname->len = sep - hostname->s;
    /* port */
    port->s = sep + 1;
    sep = index(port->s, '|');
    if (sep == NULL) {
	LM_ERR("scheme was not found in AVP value\n");
	return 0;
    }
    port->len = sep - port->s;
    /* params */
    params->s = sep + 1;
    sep = index(params->s, '|');
    if (sep == NULL) {
	LM_ERR("params was not found in AVP value\n");
	return 0;
    }
    params->len = sep - params->s;
    /* transport */
    s.s = sep + 1;
    sep = index(s.s, '|');
    if (sep == NULL) {
	LM_ERR("transport was not found in AVP value\n");
	return 0;
    }
    s.len = sep - s.s;
    str2int(&s, &u);
    switch (u) {
    case PROTO_NONE:
    case PROTO_UDP:
	transport->s = (char *)0;
	transport->len = 0;
	break;
    case PROTO_TCP:
	transport->s = ";transport=tcp";
	transport->len = 14;
	break;
    case PROTO_TLS:
	transport->s = ";transport=tls";
	transport->len = 14;
	break;
    case PROTO_SCTP:
	transport->s = ";transport=sctp";
	transport->len = 15;
	break;
    default:
	LM_ERR("unknown transport '%d'\n", u);
	return 0;
    }
    /* flags */
    s.s = sep + 1;
    s.len = strlen(s.s);
    str2int(&s, flags);

    return 1;
}
    

/* Add gateways in matched_gws array into gw_uri_avps */
void add_gws_into_avps(struct gw_info *gws, struct matched_gw_info *matched_gws,
		       unsigned int gw_cnt, str *ruri_user)
{
    unsigned int i, index, strip, hostname_len, params_len;
    int tag_len;
    str value;
    char encoded_value[MAX_URI_LEN];
    int_str val;

    delete_avp(gw_uri_avp_type|AVP_VAL_STR, gw_uri_avp);

    for (i = 0; i < gw_cnt; i++) {
	if (matched_gws[i].duplicate == 1) continue;
	index = matched_gws[i].gw_index;
      	hostname_len = gws[index].hostname_len;
      	params_len = gws[index].params_len;
	strip = gws[index].strip;
	if (strip > ruri_user->len) {
	    LM_ERR("strip count of gw is too large <%u>\n", strip);
	    goto skip;
	}
	tag_len = gws[index].tag_len;
	if (5 /* gw_index */ + 5 /* scheme */ + 4 /* strip */ + tag_len +
	    1 /* @ */ + ((hostname_len > 15)?hostname_len:15) + 6 /* port */ +
	    params_len /* params */ + 15 /* transport */ + 10 /* flags */ +
	    7 /* separators */ > MAX_URI_LEN) {
	    LM_ERR("too long AVP value\n");
	    goto skip;
	}
	value.len = 
	    encode_avp_value(encoded_value, index, gws[index].scheme,
			     strip, gws[index].tag, tag_len,
			     gws[index].ip_addr,
			     gws[index].hostname, hostname_len,
			     gws[index].port, gws[index].params, params_len,
			     gws[index].transport, gws[index].flags);
	value.s = (char *)&(encoded_value[0]);
	val.s = value;
	add_avp(gw_uri_avp_type|AVP_VAL_STR, gw_uri_avp, val);

	LM_DBG("added gw_uri_avp <%.*s> with weight <%u>\n",
	       value.len, value.s, matched_gws[i].weight);
    skip:
	continue;
    }
}


/*
 * Load info of matching GWs into gw_uri_avps
 */
static int load_gws(struct sip_msg* _m, fparam_t *_lcr_id,
					pv_spec_t* _from_uri)
{
    str ruri_user, from_uri;
    int i, j, lcr_id;
    unsigned int gw_index, now, dex;
    int_str val;
    struct matched_gw_info matched_gws[MAX_NO_OF_GWS + 1];
    struct rule_info **rules, *rule, *pl;
    struct gw_info *gws;
    struct target *t;
    pv_value_t pv_val;

    /* Get and check parameter values */
    if (get_int_fparam(&lcr_id, _m, _lcr_id) != 0) {
	LM_ERR("no lcr_id param value\n");
	return -1;
    }
    if ((lcr_id < 1) || (lcr_id > lcr_count_param)) {
	LM_ERR("invalid lcr_id parameter value %d\n", lcr_id);
	return -1;
    }
    if (_from_uri) {
		if (pv_get_spec_value(_m, _from_uri, &pv_val) == 0) {
			if (pv_val.flags & PV_VAL_STR)
				from_uri = pv_val.rs;
			else {
				LM_ERR("non string from_uri parameter value\n");
				return -1;
			}
		} else {
			LM_ERR("could not get from_uri pvar value\n");
			return -1;
		}
    } else {
	from_uri.len = 0;
	from_uri.s = (char *)0;
    }

    /* Use rules and gws with index lcr_id */
    rules = rule_pt[lcr_id];
    gws = gw_pt[lcr_id];

    /* Find Request-URI user */
    if ((parse_sip_msg_uri(_m) < 0) || (!_m->parsed_uri.user.s)) {
	LM_ERR("error while parsing R-URI\n");
	return -1;
    }
    ruri_user = _m->parsed_uri.user;

    /*
     * Find lcr entries that match based on prefix and from_uri and collect
     * gateways of matching entries into matched_gws array so that each
     * gateway appears in the array only once.
     */

    pl = rules[lcr_rule_hash_size_param];
    gw_index = 0;

    if (defunct_capability_param > 0) {
	delete_avp(defunct_gw_avp_type, defunct_gw_avp);
    }

    now = time((time_t *)NULL);

    /* check prefixes in from longest to shortest */
    while (pl) {
	if (ruri_user.len < pl->prefix_len) {
	    pl = pl->next;
	    continue;
	}
	rule = rule_hash_table_lookup(rules, pl->prefix_len, ruri_user.s);
	while (rule) {
	    /* Match prefix */
	    if ((rule->prefix_len == pl->prefix_len) && 
		(strncmp(rule->prefix, ruri_user.s, pl->prefix_len) == 0)) {
		/* Match from uri */
		if ((rule->from_uri_len == 0) ||
		    (pcre_exec(rule->from_uri_re, NULL, from_uri.s,
			       from_uri.len, 0, 0, NULL, 0) >= 0)) {
		    /* Load gws associated with this rule */
		    t = rule->targets;
		    while (t) {
                        /* If this gw is defunct, skip it */
		        if (gws[t->gw_index].defunct_until > now) goto skip_gw;
			matched_gws[gw_index].gw_index = t->gw_index;
			matched_gws[gw_index].prefix_len = pl->prefix_len;
			matched_gws[gw_index].priority = t->priority;
			matched_gws[gw_index].weight = t->weight *
			    (rand() >> 8);
			matched_gws[gw_index].duplicate = 0;
			LM_DBG("added matched_gws[%d]=[%u, %u, %u, %u]\n",
			       gw_index, t->gw_index, pl->prefix_len,
			       t->priority, matched_gws[gw_index].weight);
			gw_index++;
		    skip_gw:
			t = t->next;
		    }
		    /* Do not look further if this matching rule was stopper */
		    if (rule->stopper == 1) goto done;
		}
	    }
	    rule = rule->next;
	}
	pl = pl->next;
    }

 done:
    /* Sort gateways in reverse order based on prefix_len, priority,
       and randomized weight */
    qsort(matched_gws, gw_index, sizeof(struct matched_gw_info), comp_matched);

    /* Remove duplicate gws */
    for (i = gw_index - 1; i >= 0; i--) {
	if (matched_gws[i].duplicate == 1) continue;
	dex = matched_gws[i].gw_index;
	for (j = i - 1; j >= 0; j--) {
	    if (matched_gws[j].gw_index == dex) {
		matched_gws[j].duplicate = 1;
	    }
	}
    }

    /* Add gateways into gw_uris_avp */
    add_gws_into_avps(gws, matched_gws, gw_index, &ruri_user);

    /* Add lcr_id into AVP */
    if (defunct_capability_param > 0) {
	delete_avp(lcr_id_avp_type, lcr_id_avp);
	val.n = lcr_id;
	add_avp(lcr_id_avp_type, lcr_id_avp, val);
    }
    
    if (gw_index > 0) {
	return 1;
    } else {
	return 2;
    }
}


static int load_gws_1(struct sip_msg* _m, char *_lcr_id, char *_s2)
{
    return load_gws(_m, (fparam_t *)_lcr_id, 0);
}


static int load_gws_2(struct sip_msg* _m, char *_lcr_id, char *_from_uri)
{
    return load_gws(_m, (fparam_t *)_lcr_id, (pv_spec_t *)_from_uri);
}


/* Generate Request-URI and Destination URI */
static int generate_uris(struct sip_msg* _m, char *r_uri, str *r_uri_user,
			 unsigned int *r_uri_len, char *dst_uri,
			 unsigned int *dst_uri_len, unsigned int *addr,
			 unsigned int *gw_index, unsigned int *flags)
{
    int_str gw_uri_val;
    struct usr_avp *gu_avp;
    str scheme, tag, hostname, port, params, transport, addr_str;
    char *at;
    unsigned int strip;
    struct ip_addr a;
    
    gu_avp = search_first_avp(gw_uri_avp_type, gw_uri_avp, &gw_uri_val, 0);

    if (!gu_avp) return 0; /* No more gateways left */

    decode_avp_value(gw_uri_val.s.s, gw_index, &scheme, &strip, &tag, addr,
		     &hostname, &port, &params, &transport, flags);

    if (*addr > 0) {
	a.af = AF_INET;
	a.len = 4;
	a.u.addr32[0] = *addr;
	addr_str.s = ip_addr2a(&a);
	addr_str.len = strlen(addr_str.s);
    } else {
	addr_str.len = 0;
    }
    
    if (scheme.len + r_uri_user->len - strip + tag.len + 1 /* @ */ +
	((hostname.len > 15)?hostname.len:15) + 1 /* : */ +
	port.len + params.len + transport.len + 1 /* null */ > MAX_URI_LEN) {
	LM_ERR("too long Request URI or DST URI\n");
	return -1;
    }

    if ((dont_strip_or_tag_flag_param != -1) &&
	isflagset(_m, dont_strip_or_tag_flag_param)) {
	strip = 0;
	tag.len = 0;
    }

    at = r_uri;
    
    append_str(at, scheme.s, scheme.len);
    append_str(at, tag.s, tag.len);
    if (strip > r_uri_user->len) {
	LM_ERR("strip count <%u> is larger than R-URI user <%.*s>\n",
	       strip, r_uri_user->len, r_uri_user->s);
	return -1;
    }
    append_str(at, r_uri_user->s + strip, r_uri_user->len - strip);

    append_chr(at, '@');

    if ((addr_str.len > 0) && (hostname.len > 0)) {
	/* both ip_addr and hostname specified:
	   place hostname in r-uri and ip_addr in dst-uri */
	append_str(at, hostname.s, hostname.len);
	if (params.len > 0) {
	    append_str(at, params.s, params.len);
	}
	*at = '\0';
	*r_uri_len = at - r_uri;
	at = dst_uri;
	append_str(at, scheme.s, scheme.len);
	append_str(at, addr_str.s, addr_str.len);
	if (port.len > 0) {
	    append_chr(at, ':');
	    append_str(at, port.s, port.len);
	}
	if (transport.len > 0) {
	    append_str(at, transport.s, transport.len);
	}
	*at = '\0';
	*dst_uri_len = at - dst_uri;
    } else {
	/* either ip_addr or hostname specified:
	   place the given one in r-uri and leave dst-uri empty */
	if (addr_str.len > 0) {
	    append_str(at, addr_str.s, addr_str.len);
	} else {
	    append_str(at, hostname.s, hostname.len);
	}
	if (port.len > 0) {
	    append_chr(at, ':');
	    append_str(at, port.s, port.len);
	}
	if (transport.len > 0) {
	    append_str(at, transport.s, transport.len);
	}
	if (params.len > 0) {
	    append_str(at, params.s, params.len);
	}
	*at = '\0';
	*r_uri_len = at - r_uri;
	*dst_uri_len = 0;
    }

    destroy_avp(gu_avp);
	
    LM_DBG("r_uri <%.*s>, dst_uri <%.*s>\n",
	   (int)*r_uri_len, r_uri, (int)*dst_uri_len, dst_uri);

    return 1;
}


/*
 * Defunct current gw until time given as argument has passed.
 */
static int defunct_gw(struct sip_msg* _m, char *_defunct_period, char *_s2)
{
    int_str lcr_id_val, index_val;
    struct gw_info *gws;
    unsigned int gw_index, defunct_until;
    int defunct_period;

    /* Check defunct gw capability */
    if (defunct_capability_param == 0) {
	LM_ERR("no defunct gw capability, activate by setting "
	       "defunct_capability_param module param\n");
	return -1;
    }

    /* Get parameter value */
    if (get_int_fparam(&defunct_period, _m, (fparam_t *)_defunct_period) != 0) {
	LM_ERR("no defunct_period param value\n");
	return -1;
    }
    if (defunct_period < 1) {
	LM_ERR("invalid defunct_period param value\n");
	return -1;
    }

    /* Get AVP values */
    if (search_first_avp(lcr_id_avp_type, lcr_id_avp, &lcr_id_val, 0)
	== NULL) {
	LM_ERR("lcr_id_avp was not found\n");
	return -1;
    }
    gws = gw_pt[lcr_id_val.n];
    if (search_first_avp(defunct_gw_avp_type, defunct_gw_avp,
			 &index_val, 0) == NULL) {
	LM_ERR("defucnt_gw_avp was not found\n");
	return -1;
    }
    gw_index = index_val.n;
    if ((gw_index < 1) || (gw_index > gws[0].ip_addr)) {
	LM_ERR("gw index <%u> is out of bounds\n", gw_index);
	return -1;
    }
    
    /* Defunct gw */
    defunct_until = time((time_t *)NULL) + defunct_period;
    LM_DBG("defuncting gw with name <%.*s> until <%u>\n",
	   gws[gw_index].gw_name_len, gws[gw_index].gw_name, defunct_until);
    gws[gw_index].defunct_until = defunct_until;
    return 1;
}    


/*
 * When called first time, rewrites scheme, host, port, and
 * transport parts of R-URI based on first gw_uri_avp value, which is then
 * destroyed.  Saves R-URI user to ruri_user_avp for later use.
 *
 * On other calls, rewrites R-URI, where scheme, host, port,
 * and transport of URI are taken from the first gw_uri_avp value, 
 * which is then destroyed. URI user is taken either from ruri_user_avp
 * value saved earlier.
 *
 * Returns 1 upon success and -1 upon failure.
 */
static int next_gw(struct sip_msg* _m, char* _s1, char* _s2)
{
    int_str ruri_user_val, val;
    struct usr_avp *ru_avp;
    int rval;
    str uri_str;
    unsigned int flags, r_uri_len, dst_uri_len, addr, gw_index;
    char r_uri[MAX_URI_LEN], dst_uri[MAX_URI_LEN];

    ru_avp = search_first_avp(ruri_user_avp_type, ruri_user_avp,
			      &ruri_user_val, 0);

    if (ru_avp == NULL) {
	
	/* First invocation either in route or failure route block.
	 * Take Request-URI user from Request-URI and generate Request
         * and Destination URIs. */

	if (parse_sip_msg_uri(_m) < 0) {
	    LM_ERR("parsing of R-URI failed\n");
	    return -1;
	}
	if (!generate_uris(_m, r_uri, &(_m->parsed_uri.user), &r_uri_len,
			   dst_uri, &dst_uri_len, &addr, &gw_index, &flags)) {
	    return -1;
	}

	/* Save Request-URI user into uri_user_avp for use in subsequent
         * invocations. */

	val.s = _m->parsed_uri.user;
	add_avp(ruri_user_avp_type|AVP_VAL_STR, ruri_user_avp, val);
	LM_DBG("added ruri_user_avp <%.*s>\n", val.s.len, val.s.s);

    } else {

	/* Subsequent invocation either in route or failure route block.
	 * Take Request-URI user from ruri_user_avp and generate Request
         * and Destination URIs. */

	if (!generate_uris(_m, r_uri, &(ruri_user_val.s), &r_uri_len, dst_uri,
			   &dst_uri_len, &addr, &gw_index, &flags)) {
	    return -1;
	}
    }

    /* Rewrite Request URI */
    uri_str.s = r_uri;
    uri_str.len = r_uri_len;
    rewrite_uri(_m, &uri_str);
    
    /* Set Destination URI if not empty */
    if (dst_uri_len > 0) {
	uri_str.s = dst_uri;
	uri_str.len = dst_uri_len;
	LM_DBG("setting du to <%.*s>\n", uri_str.len, uri_str.s);
	rval = set_dst_uri(_m, &uri_str);
	if (rval != 0) {
	    LM_ERR("calling do_action failed with return value <%d>\n", rval);
	    return -1;
	}
	
    }

    /* Set flags_avp */
    val.n = flags;
    add_avp(flags_avp_type, flags_avp, val);
    LM_DBG("added flags_avp <%u>\n", (unsigned int)val.n);

    /* Add index of selected gw to defunct gw AVP */
    if (defunct_capability_param > 0) {
	delete_avp(defunct_gw_avp_type, defunct_gw_avp);
	val.n = gw_index;
	add_avp(defunct_gw_avp_type, defunct_gw_avp, val);
	LM_DBG("added defunct_gw_avp <%u>", addr);
    }
    
    return 1;
}


/*
 * Checks if request comes from ip address of a gateway
 */
static int do_from_gw(struct sip_msg* _m, unsigned int lcr_id,
		      unsigned int src_addr)
{
    struct gw_info *res, gw, *gws;
    int_str val;
	
    gws = gw_pt[lcr_id];

    /* Skip lcr instance if some of its gws do not have ip_addr */
    if (gws[0].port != 0) {
	LM_DBG("lcr instance <%u> has gw(s) without ip_addr\n", lcr_id);
	return -1;
    }

    /* Search for gw ip address */
    gw.ip_addr = src_addr;
    res = (struct gw_info *)bsearch(&gw, &(gws[1]), gws[0].ip_addr,
				    sizeof(struct gw_info), comp_gws);

    /* Store flags and return result */
    if (res == NULL) {
	LM_DBG("request did not come from gw\n");
	return -1;
    } else {
	LM_DBG("request game from gw\n");
	val.n = res->flags;
	add_avp(flags_avp_type, flags_avp, val);
	LM_DBG("added flags_avp <%u>\n", (unsigned int)val.n);
	return 1;
    }
}


/*
 * Checks if request comes from ip address of a gateway taking source
 * address from request.
 */
static int from_gw_1(struct sip_msg* _m, char* _lcr_id, char* _s2)
{
    int lcr_id;
    unsigned int src_addr;

    /* Get and check parameter value */
    if (get_int_fparam(&lcr_id, _m, (fparam_t *)_lcr_id) != 0) {
	LM_ERR("no lcr_id param value\n");
	return -1;
    }
    if ((lcr_id < 1) || (lcr_id > lcr_count_param)) {
	LM_ERR("invalid lcr_id parameter value %d\n", lcr_id);
	return -1;
    }

    /* Get source address */
    src_addr = _m->rcv.src_ip.u.addr32[0];

    /* Do test */
    return do_from_gw(_m, lcr_id, src_addr);
}


/*
 * Checks if request comes from ip address of a gateway taking source
 * address from param.
 */
static int from_gw_2(struct sip_msg* _m, char* _lcr_id, char* _addr)
{
    unsigned int src_addr;
    int lcr_id;
    pv_value_t pv_val;
    struct ip_addr *ip;

    /* Get and check parameter values */
    if (get_int_fparam(&lcr_id, _m, (fparam_t *)_lcr_id) != 0) {
	LM_ERR("no lcr_id param value\n");
	return -1;
    }
    if ((lcr_id < 1) || (lcr_id > lcr_count_param)) {
	LM_ERR("invalid lcr_id parameter value %d\n", lcr_id);
	return -1;
    }

    if (_addr && (pv_get_spec_value(_m, (pv_spec_t *)_addr, &pv_val) == 0)) {
	if (pv_val.flags & PV_VAL_INT) {
	    src_addr = pv_val.ri;
	} else if (pv_val.flags & PV_VAL_STR) {
	    if ((ip = str2ip(&pv_val.rs)) == NULL) {
		LM_DBG("request did not come from gw "
		       "(addr param value is not an IP address)\n");
		return -1;
	    } else {
		src_addr = ip->u.addr32[0];
	    }
	} else {
	    LM_ERR("addr param has no value\n");
	    return -1;
	}
    } else {
	LM_ERR("could not get source address from param\n");
	return -1;
    }

    /* Do test */
    return do_from_gw(_m, lcr_id, src_addr);
}


/*
 * Checks if request comes from ip address of any gateway taking source
 * address from request.
 */
static int from_any_gw_0(struct sip_msg* _m, char* _s1, char* _s2)
{
    unsigned int src_addr, i;

    src_addr = _m->rcv.src_ip.u.addr32[0];

    for (i = 1; i <= lcr_count_param; i++) {
	if (do_from_gw(_m, i, src_addr) == 1) {
	    return i;
	}
    }
    return -1;
}


/*
 * Checks if request comes from ip address of a a gateway taking source
 * address from param.
 */
static int from_any_gw_1(struct sip_msg* _m, char* _addr, char* _s2)
{
    unsigned int i, src_addr;
    pv_value_t pv_val;
    struct ip_addr *ip;

    /* Get parameter value */
    if (_addr && (pv_get_spec_value(_m, (pv_spec_t *)_addr, &pv_val) == 0)) {
	if (pv_val.flags & PV_VAL_INT) {
	    src_addr = pv_val.ri;
	} else if (pv_val.flags & PV_VAL_STR) {
	    if ((ip = str2ip(&pv_val.rs)) == NULL) {
		LM_DBG("request did not come from gw "
		       "(addr param value is not an IP address)\n");
		return -1;
	    } else {
		src_addr = ip->u.addr32[0];
	    }
	} else {
	    LM_ERR("addr param has no value\n");
	    return -1;
	}
    } else {
	LM_ERR("could not get source address from param\n");
	return -1;
    }

    /* Do test */
    for (i = 1; i <= lcr_count_param; i++) {
	if (do_from_gw(_m, i, src_addr) == 1) {
	    return i;
	}
    }
    return -1;
}


/*
 * Checks if in-dialog request goes to ip address of a gateway.
 */
static int do_to_gw(struct sip_msg* _m, unsigned int lcr_id,
		    unsigned int dst_addr)
{
    struct gw_info *res, gw, *gws;

    gws = gw_pt[lcr_id];

    /* Skip lcr instance if some of its gws do not have ip addr */
    if (gws[0].port != 0) {
	LM_DBG("lcr instance <%u> has gw(s) without ip_addr\n", lcr_id);
	return -1;
    }

    /* Search for gw ip address */
    gw.ip_addr = dst_addr;
    res = (struct gw_info *)bsearch(&gw, &(gws[1]), gws[0].ip_addr,
				    sizeof(struct gw_info), comp_gws);

    /* Return result */
    if (res == NULL) {
	LM_DBG("request is not going to gw\n");
	return -1;
    } else {
	LM_DBG("request goes to gw\n");
	return 1;
    }
}


/*
 * Checks if request goes to ip address of a gateway taking destination
 * address from Request URI.
 */
static int to_gw_1(struct sip_msg* _m, char* _lcr_id, char* _s2)
{
    int lcr_id;
    unsigned int dst_addr;
    struct ip_addr *ip;

    /* Get and check parameter value */
    if (get_int_fparam(&lcr_id, _m, (fparam_t *)_lcr_id) != 0) {
	LM_ERR("no lcr_id param value\n");
	return -1;
    }
    if ((lcr_id < 1) || (lcr_id > lcr_count_param)) {
	LM_ERR("invalid lcr_id parameter value %d\n", lcr_id);
	return -1;
    }

    /* Get destination address */
    if ((_m->parsed_uri_ok == 0) && (parse_sip_msg_uri(_m) < 0)) {
	LM_ERR("while parsing Request-URI\n");
	return -1;
    }
    if (_m->parsed_uri.host.len > 15) {
	LM_DBG("request is not going to gw "
	       "(Request-URI host is not an IP address)\n");
	return -1;
    }
    if ((ip = str2ip(&(_m->parsed_uri.host))) == NULL) {
	LM_DBG("request is not going to gw "
	       "(Request-URI host is not an IP address)\n");
	return -1;
    } else {
	dst_addr = ip->u.addr32[0];
    }

    /* Do test */
    return do_to_gw(_m, lcr_id, dst_addr);
}


/*
 * Checks if request goes to ip address of a gateway taking destination
 * address from param.
 */
static int to_gw_2(struct sip_msg* _m, char* _lcr_id, char* _addr)
{
    int lcr_id;
    unsigned int dst_addr;
    pv_value_t pv_val;
    struct ip_addr *ip;

    /* Get and check parameter values */
    if (get_int_fparam(&lcr_id, _m, (fparam_t *)_lcr_id) != 0) {
	LM_ERR("no lcr_id param value\n");
	return -1;
    }
    if ((lcr_id < 1) || (lcr_id > lcr_count_param)) {
	LM_ERR("invalid lcr_id parameter value %d\n", lcr_id);
	return -1;
    }

    if (_addr && (pv_get_spec_value(_m, (pv_spec_t *)_addr, &pv_val) == 0)) {
	if (pv_val.flags & PV_VAL_INT) {
	    dst_addr = pv_val.ri;
	} else if (pv_val.flags & PV_VAL_STR) {
	    if ((ip = str2ip(&pv_val.rs)) == NULL) {
		LM_DBG("request is not going to gw "
		       "(addr param value is not an IP address)\n");
		return -1;
	    } else {
		dst_addr = ip->u.addr32[0];
	    }
	} else {
	    LM_ERR("addr param has no value\n");
	    return -1;
	}
    } else {
	LM_ERR("could not get destination address from param\n");
	return -1;
    }
    
    /* Do test */
    return do_to_gw(_m, lcr_id, dst_addr);
}


/*
 * Checks if request goes to ip address of any gateway taking destination
 * address from Request-URI.
 */
static int to_any_gw_0(struct sip_msg* _m, char* _s1, char* _s2)
{
    unsigned int dst_addr, i;
    struct ip_addr *ip;

    /* Get destination address */
    if ((_m->parsed_uri_ok == 0) && (parse_sip_msg_uri(_m) < 0)) {
	LM_ERR("while parsing Request-URI\n");
	return -1;
    }
    if (_m->parsed_uri.host.len > 15) {
	LM_DBG("request is not going to gw "
	       "(Request-URI host is not an IP address)\n");
	return -1;
    }
    if ((ip = str2ip(&(_m->parsed_uri.host))) == NULL) {
	LM_DBG("request is not going to gw "
	       "(Request-URI host is not an IP address)\n");
	return -1;
    } else {
	dst_addr = ip->u.addr32[0];
    }

    /* Do test */
    for (i = 1; i <= lcr_count_param; i++) {
	if (do_to_gw(_m, i, dst_addr) == 1) {
	    return i;
	}
    }
    return -1;
}


/*
 * Checks if request goes to ip address of any gateway taking destination
 * address from param.
 */
static int to_any_gw_1(struct sip_msg* _m, char* _addr, char* _s2)
{
    unsigned int i, dst_addr;
    pv_value_t pv_val;
    struct ip_addr *ip;

    /* Get parameter value */
    if (_addr && (pv_get_spec_value(_m, (pv_spec_t *)_addr, &pv_val) == 0)) {
	if (pv_val.flags & PV_VAL_INT) {
	    dst_addr = pv_val.ri;
	} else if (pv_val.flags & PV_VAL_STR) {
	    if ((ip = str2ip(&pv_val.rs)) == NULL) {
		LM_DBG("request did go to any gw "
		       "(addr param value is not an IP address)\n");
		return -1;
	    } else {
		dst_addr = ip->u.addr32[0];
	    }
	} else {
	    LM_ERR("addr param has no value\n");
	    return -1;
	}
    } else {
	LM_ERR("could not get destination address from param\n");
	return -1;
    }

    /* Do test */
    for (i = 1; i <= lcr_count_param; i++) {
	if (do_to_gw(_m, i, dst_addr) == 1) {
	    return i;
	}
    }
    return -1;
}
