/*
 *  $Id$
 *
 * Copyright (C)  2007-2008 Voice Sistem SRL
 *
 * Copyright (C)  2008 Juha Heinanen
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * SIP-router is distributed in the hope that it will be useful,
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
 *  2007-08-01 initial version (ancuta onofrei)
 *  2008-10-09 module is now using pcre regexp lib (juha heinanen)
 */

/*!
 * \file
 * \brief SIP-router dialplan :: Module interface
 * \ingroup dialplan
 * Module: \ref dialplan
 */

/*! \defgroup dialplan SIP-router dialplan transformations module
 *
 */




#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../../sr_module.h"
#include "../../lib/srdb1/db.h"
#include "../../dprint.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../action.h"
#include "../../pvar.h"
#include "../../dset.h"
#include "../../mem/mem.h"
#include "../../lib/kmi/mi.h"
#include "../../parser/parse_to.h"
#include "../../lvalue.h"
#include "dialplan.h"
#include "dp_db.h"

MODULE_VERSION

#define DEFAULT_PARAM    "$ruri.user"

static int mod_init(void);
static int child_init(int rank);
static void mod_destroy();
static int mi_child_init();

static struct mi_root * mi_reload_rules(struct mi_root *cmd_tree,void *param);
static struct mi_root * mi_translate(struct mi_root *cmd_tree, void *param);
static int dp_translate_f(struct sip_msg* msg, char* str1, char* str2);
static int dp_trans_fixup(void ** param, int param_no);

str attr_pvar_s = {NULL,0};
pv_spec_t * attr_pvar = NULL;

str default_param_s = str_init(DEFAULT_PARAM);
dp_param_p default_par2 = NULL;

int dp_fetch_rows = 1000;

static param_export_t mod_params[]={
	{ "db_url",			STR_PARAM,	&dp_db_url.s },
	{ "table_name",		STR_PARAM,	&dp_table_name.s },
	{ "dpid_col",		STR_PARAM,	&dpid_column.s },
	{ "pr_col",			STR_PARAM,	&pr_column.s },
	{ "match_op_col",	STR_PARAM,	&match_op_column.s },
	{ "match_exp_col",	STR_PARAM,	&match_exp_column.s },
	{ "match_len_col",	STR_PARAM,	&match_len_column.s },
	{ "subst_exp_col",	STR_PARAM,	&subst_exp_column.s },
	{ "repl_exp_col",	STR_PARAM,	&repl_exp_column.s },
	{ "attrs_col",		STR_PARAM,	&attrs_column.s },
	{ "attrs_pvar",	    STR_PARAM,	&attr_pvar_s.s},
	{ "attribute_pvar",	STR_PARAM,	&attr_pvar_s.s},
	{ "fetch_rows",		INT_PARAM,	&dp_fetch_rows},
	{0,0,0}
};

static mi_export_t mi_cmds[] = {
	{ "dp_reload",  mi_reload_rules,   MI_NO_INPUT_FLAG,  0,  mi_child_init},
	{ "dp_translate",  mi_translate,   0,  0,  0},
	{ 0, 0, 0, 0, 0}
};

static cmd_export_t cmds[]={
	{"dp_translate",(cmd_function)dp_translate_f,	2,	dp_trans_fixup,  0,
				REQUEST_ROUTE|FAILURE_ROUTE|LOCAL_ROUTE|BRANCH_ROUTE},
	{"dp_translate",(cmd_function)dp_translate_f,	1,	dp_trans_fixup,  0,
				REQUEST_ROUTE|FAILURE_ROUTE|LOCAL_ROUTE|BRANCH_ROUTE},
	{0,0,0,0,0,0}
};

struct module_exports exports= {
	"dialplan",     /* module's name */
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,      	    /* exported functions */
	mod_params,     /* param exports */
	0,				/* exported statistics */
	mi_cmds,		/* exported MI functions */
	0,				/* exported pseudo-variables */
	0,				/* additional processes */
	mod_init,		/* module initialization function */
	0,				/* reply processing function */
	mod_destroy,
	child_init		/* per-child init function */
};


static int mod_init(void)
{
	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	dp_db_url.len = dp_db_url.s ? strlen(dp_db_url.s) : 0;
	LM_DBG("db_url=%s/%d/%p\n", ZSW(dp_db_url.s), dp_db_url.len,dp_db_url.s);
	dp_table_name.len   = strlen(dp_table_name.s);
	dpid_column.len     = strlen( dpid_column.s);
	pr_column.len       = strlen(pr_column.s);
	match_op_column.len = strlen(match_op_column.s);
	match_exp_column.len= strlen(match_exp_column.s);
	match_len_column.len= strlen(match_len_column.s);
	subst_exp_column.len= strlen(subst_exp_column.s);
	repl_exp_column.len = strlen(repl_exp_column.s);
	attrs_column.len    = strlen(attrs_column.s);

	if(attr_pvar_s.s) {
		attr_pvar = (pv_spec_t *)shm_malloc(sizeof(pv_spec_t));
		if(!attr_pvar){
			LM_ERR("out of shm memory\n");
			return -1;
		}

		attr_pvar_s.len = strlen(attr_pvar_s.s);
		if( (pv_parse_spec(&attr_pvar_s, attr_pvar)==NULL) ||
		((attr_pvar->type != PVT_AVP) && (attr_pvar->type!=PVT_SCRIPTVAR))) {
				LM_ERR("invalid pvar name\n");
				return -1;
			}
	}

	default_par2 = (dp_param_p)shm_malloc(sizeof(dp_param_t));
	if(default_par2 == NULL){
		LM_ERR("no shm more memory\n");
		return -1;
	}
	memset(default_par2, 0, sizeof(dp_param_t));

	default_param_s.len = strlen(default_param_s.s);
	if (pv_parse_spec( &default_param_s, &default_par2->v.sp[0])==NULL) {
		LM_ERR("input pv is invalid\n");
		return -1;
	}

	default_param_s.len = strlen(default_param_s.s);
	if (pv_parse_spec( &default_param_s, &default_par2->v.sp[1])==NULL) {
		LM_ERR("output pv is invalid\n");
		return -1;
	}

	if(dp_fetch_rows<=0)
		dp_fetch_rows = 1000;

	if(init_data() != 0) {
		LM_ERR("could not initialize data\n");
		return -1;
	}

	return 0;
}


static int child_init(int rank)
{
	return 0;
}


static void mod_destroy(void)
{
	/*destroy shared memory*/
	if(default_par2){
		shm_free(default_par2);
		default_par2 = NULL;
	}
	if(attr_pvar){
		shm_free(attr_pvar);
		attr_pvar = NULL;
	}
	destroy_data();
}


static int mi_child_init(void)
{
    return 0;
}


static int dp_get_ivalue(struct sip_msg* msg, dp_param_p dp, int *val)
{
	pv_value_t value;

	if(dp->type==DP_VAL_INT) {
		LM_DBG("integer value\n");
		*val = dp->v.id;
		return 0;
	}

	LM_DBG("searching %d\n",dp->v.sp[0].type);

	if( pv_get_spec_value( msg, &dp->v.sp[0], &value)!=0
	|| value.flags&(PV_VAL_NULL|PV_VAL_EMPTY) || !(value.flags&PV_VAL_INT)) {
		LM_ERR("no AVP or SCRIPTVAR found (error in scripts)\n");
		return -1;
	}
	*val = value.ri;
	return 0;
}


static int dp_get_svalue(struct sip_msg * msg, pv_spec_t spec, str* val)
{
	pv_value_t value;

	LM_DBG("searching %d \n", spec.type);

	if ( pv_get_spec_value(msg,&spec,&value)!=0 || value.flags&PV_VAL_NULL
	|| value.flags&PV_VAL_EMPTY || !(value.flags&PV_VAL_STR)){
			LM_ERR("no AVP or SCRIPTVAR found (error in scripts)\n");
			return -1;
	}

	*val = value.rs;
	return 0;
}


static int dp_update(struct sip_msg * msg, pv_spec_t * src, pv_spec_t * dest,
		     str * repl, str * attrs)
{
	int no_change;
	pv_value_t val;

	no_change = (dest->type == PVT_NONE) || (!repl->s) || (!repl->len);

	if (no_change)
		goto set_attr_pvar;

	memset(&val, 0, sizeof(pv_value_t));
	val.flags = PV_VAL_STR;
	val.rs = *repl;

	if(dest->setf(msg, &dest->pvp, (int)EQ_T, &val)<0)
	{
		LM_ERR("setting dst pseudo-variable failed\n");
		return -1;
	}

	if(is_route_type(FAILURE_ROUTE)
				&& (dest->type==PVT_RURI || dest->type==PVT_RURI_USERNAME)) {
		if (append_branch(msg, 0, 0, 0, Q_UNSPECIFIED, 0, 0)!=1 ){
			LM_ERR("append_branch action failed\n");
			return -1;
		}
	}

set_attr_pvar:

	if(!attr_pvar)
		return 0;
	
	val.rs = *attrs;
	if(attr_pvar->setf(msg, &attr_pvar->pvp, (int)EQ_T, &val)<0)
	{
		LM_ERR("setting attr pseudo-variable failed\n");
		return -1;
	}

	return 0;
}


static int dp_translate_f(struct sip_msg* msg, char* str1, char* str2)
{
	int dpid;
	str input, output;
	dpl_id_p idp;
	dp_param_p id_par, repl_par;
	str attrs, * attrs_par;

	if(!msg)
		return -1;

	/*verify first param's value*/
	id_par = (dp_param_p) str1;
	if (dp_get_ivalue(msg, id_par, &dpid) != 0){
		LM_ERR("no dpid value\n");
		return -1;
	}

	if ((idp = select_dpid(dpid)) ==0 ){
		LM_DBG("no information available for dpid %i\n", dpid);
		return -1;
	}

	repl_par = (str2!=NULL)? ((dp_param_p)str2):default_par2;
	if (dp_get_svalue(msg, repl_par->v.sp[0], &input)!=0){
		LM_ERR("invalid param 2\n");
		return -1;
	}

	LM_DBG("input is %.*s\n", input.len, input.s);

	attrs_par = (!attr_pvar)?NULL:&attrs;
	if (translate(msg, input, &output, idp, attrs_par)!=0){
		LM_DBG("could not translate %.*s "
			"with dpid %i\n", input.len, input.s, idp->dp_id);
		return -1;
	}
	LM_DBG("input %.*s with dpid %i => output %.*s\n",
			input.len, input.s, idp->dp_id, output.len, output.s);

	/*set the output*/
	if (dp_update(msg, &repl_par->v.sp[0], &repl_par->v.sp[1], 
	&output, attrs_par) !=0){
		LM_ERR("cannot set the output\n");
		return -1;
	}

	return 1;
		
}

#define verify_par_type(_par_no, _spec)\
	do{\
		if( ((_par_no == 1) \
			&& ((_spec).type != PVT_AVP) && ((_spec).type!=PVT_SCRIPTVAR) )\
		  ||((_par_no == 2) \
			&& ((_spec).type != PVT_AVP) && ((_spec).type!=PVT_SCRIPTVAR) \
		  	&& ((_spec).type!=PVT_RURI) && (_spec.type!=PVT_RURI_USERNAME))){\
				\
			LM_ERR("Unsupported Parameter TYPE\n");\
				return E_UNSPEC;\
			}\
	}while(0);


/* first param: DPID: type: INT, AVP, SVAR
 * second param: SRC type: any psedo variable type
 * second param: DST type: RURI, RURI_USERNAME, AVP, SVAR, N/A
 * default value for the second param: $ru.user/$ru.user
 */
static int dp_trans_fixup(void ** param, int param_no){

	int dpid;
	dp_param_p dp_par= NULL;
	char *p, *s=NULL;
	str lstr;

	if(param_no!=1 && param_no!=2) 
		return 0;

	p = (char*)*param;
	if(!p || (*p == '\0')){
		LM_DBG("null param %i\n", param_no);
		return E_CFG;
	}

	LM_DBG("param_no is %i\n", param_no);

	dp_par = (dp_param_p)pkg_malloc(sizeof(dp_param_t));
	if(dp_par == NULL){
		LM_ERR("no more pkg memory\n");
		return E_OUT_OF_MEM;
	}
	memset(dp_par, 0, sizeof(dp_param_t));

	if(param_no == 1) {
		if(*p != '$') {
			dp_par->type = DP_VAL_INT;
			lstr.s = *param; lstr.len = strlen(*param);
			if(str2sint(&lstr, &dpid) != 0) {
				LM_ERR("bad number <%s>\n",(char *)(*param));
				pkg_free(dp_par);
				return E_CFG;
			}

			dp_par->type = DP_VAL_INT;
			dp_par->v.id = dpid;
		}else{
			lstr.s = p; lstr.len = strlen(p);
			if (pv_parse_spec( &lstr, &dp_par->v.sp[0])==NULL)
				goto error;

			verify_par_type(param_no, dp_par->v.sp[0]);
			dp_par->type = DP_VAL_SPEC;
		}
	} else {

	    if (((s = strchr(p, '/')) != 0) && (*(s+1)=='\0'))
		goto error;

	    if (s != 0) {
		*s = '\0'; s++;
	    }

	    lstr.s = p; lstr.len = strlen(p);
	    if(pv_parse_spec( &lstr, &dp_par->v.sp[0])==NULL)
		goto error;

	    if (s != 0) {
		lstr.s = s; lstr.len = strlen(s);
		if (pv_parse_spec( &lstr, &dp_par->v.sp[1] )==NULL)
		    goto error;
		verify_par_type(param_no, dp_par->v.sp[1]);
	    } else {
		dp_par->v.sp[1].type = PVT_NONE;
	    }

	    dp_par->type = DP_VAL_SPEC;

	}
	
	*param = (void *)dp_par;

	return 0;

error:
	LM_ERR("failed to parse param %i\n", param_no);
	return E_INVALID_PARAMS;
}


static struct mi_root * mi_reload_rules(struct mi_root *cmd_tree, void *param)
{
	struct mi_root* rpl_tree= NULL;

	if (dp_connect_db() < 0) {
	    LM_ERR("failed to reload rules fron database (db connect)\n");
	    return 0;
	}
	    
	if(dp_load_db() != 0){
	    LM_ERR("failed to reload rules fron database (db load)\n");
	    dp_disconnect_db();
	    return 0;
	}

	dp_disconnect_db();

	rpl_tree = init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
	if (rpl_tree==0)
		return 0;
	
	return rpl_tree;
}

/* 
 *  mi cmd:  dp_translate
 *			<dialplan id> 
 *			<input>
 *		* */

static struct mi_root * mi_translate(struct mi_root *cmd, void *param)
{

	struct mi_root* rpl= NULL;
	struct mi_node* root, *node;
	dpl_id_p idp;
	str dpid_str;
	str input;
	int dpid;
	str attrs;
	str output= {0, 0};

	node = cmd->node.kids;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	/* Get the id parameter */
	dpid_str = node->value;
	if(dpid_str.s == NULL || dpid_str.len== 0)	{
		LM_ERR( "empty idp parameter\n");
		return init_mi_tree(404, "Empty id parameter", 18);
	}
	if(str2sint(&dpid_str, &dpid) != 0)	{
		LM_ERR("Wrong id parameter - should be an integer\n");
		return init_mi_tree(404, "Wrong id parameter", 18);
	}

	if ((idp = select_dpid(dpid)) ==0 ){
		LM_ERR("no information available for dpid %i\n", dpid);
		return init_mi_tree(404, "No information available for dpid", 33);
	}

	node = node->next;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	if(node->next!= NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	input=  node->value;
	if(input.s == NULL || input.len== 0)	{
		LM_ERR( "empty input parameter\n");
		return init_mi_tree(404, "Empty input parameter", 21);
	}

	LM_DBG("trying to translate %.*s with dpid %i\n",
			input.len, input.s, idp->dp_id);
	if (translate(NULL, input, &output, idp, &attrs)!=0){
		LM_DBG("could not translate %.*s with dpid %i\n", 
			input.len, input.s, idp->dp_id);
		return init_mi_tree(404, "No translation", 14);
	}
	LM_DBG("input %.*s with dpid %i => output %.*s\n",
			input.len, input.s, idp->dp_id, output.len, output.s);

	rpl = init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
	if (rpl==0)
		goto error;

	root= &rpl->node;

	node = add_mi_node_child(root, 0, "Output", 6, output.s, output.len );
	if( node == NULL)
		goto error;

	node = add_mi_node_child(root, 0, "ATTRIBUTES", 10, attrs.s, attrs.len);
	if( node == NULL)
		goto error;

	return rpl;

error:
	if(rpl)
		free_mi_tree(rpl);
	return 0;
}

