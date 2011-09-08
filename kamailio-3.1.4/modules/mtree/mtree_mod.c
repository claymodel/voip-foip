/**
 * $Id$
 *
 * Copyright (C) 2010 Daniel-Constantin Mierla (asipto.com)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../lib/srdb1/db_op.h"
#include "../../lib/kmi/mi.h"
#include "../../sr_module.h"
#include "../../lib/srdb1/db.h"
#include "../../mem/shm_mem.h"
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../../parser/parse_uri.h"
#include "../../timer.h"
#include "../../ut.h"
#include "../../locking.h"
#include "../../action.h"
#include "../../mod_fix.h"
#include "../../parser/parse_from.h"

#include "mtree.h"

MODULE_VERSION


#define NR_KEYS			3

int mt_fetch_rows = 1000;

/** database connection */
static db1_con_t *db_con = NULL;
static db_func_t mt_dbf;

#if 0
INSERT INTO version (table_name, table_version) values ('mtree','1');
CREATE TABLE mtree (
    id INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY NOT NULL,
    tprefix VARCHAR(32) NOT NULL,
    tvalue VARCHAR(128) DEFAULT '' NOT NULL,
    CONSTRAINT tprefix_idx UNIQUE (tprefix)
) ENGINE=MyISAM;
INSERT INTO version (table_name, table_version) values ('mtrees','1');
CREATE TABLE mtrees (
    id INT(10) UNSIGNED AUTO_INCREMENT PRIMARY KEY NOT NULL,
    tname VARCHAR(128) NOT NULL,
    tprefix VARCHAR(32) NOT NULL,
    tvalue VARCHAR(128) DEFAULT '' NOT NULL,
    CONSTRAINT tname_tprefix_idx UNIQUE (tname, tprefix)
) ENGINE=MyISAM;
#endif

/** parameters */
static str db_url = str_init(DEFAULT_DB_URL);
static str db_table = str_init("");
static str tname_column   = str_init("tname");
static str tprefix_column = str_init("tprefix");
static str tvalue_column  = str_init("tvalue");

/* List of allowed chars for a prefix*/
str mt_char_list = {"0123456789", 10};

static str value_param = {"$avp(s:tvalue)", 0};
static str dstid_param = {"$avp(s:tdstid)", 0};
static str weight_param = {"$avp(s:tweight)", 0};
static str count_param = {"$avp(s:tcount)", 0};
pv_spec_t pv_value;
pv_spec_t pv_dstid;
pv_spec_t pv_weight;
pv_spec_t pv_count;
int _mt_tree_type = MT_TREE_SVAL;
int _mt_ignore_duplicates = 0;

/* lock, ref counter and flag used for reloading the date */
static gen_lock_t *mt_lock = 0;
static volatile int mt_tree_refcnt = 0;
static volatile int mt_reload_flag = 0;

int mt_param(modparam_t type, void *val);
static int fixup_mt_match(void** param, int param_no);
static int w_mt_match(struct sip_msg* msg, char* str1, char* str2,
		char* str3);

static int  mod_init(void);
static void mod_destroy(void);
static int  child_init(int rank);
static int  mi_child_init(void);

static int mt_match(struct sip_msg *msg, gparam_t *dm, gparam_t *var,
		gparam_t *mode);

static struct mi_root* mt_mi_reload(struct mi_root*, void* param);
static struct mi_root* mt_mi_list(struct mi_root*, void* param);
static struct mi_root* mt_mi_summary(struct mi_root*, void* param);

static int mt_load_db(str *tname);
static int mt_load_db_trees();

static cmd_export_t cmds[]={
	{"mt_match", (cmd_function)w_mt_match, 3, fixup_mt_match,
		0, REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|ONREPLY_ROUTE},
	{0, 0, 0, 0, 0, 0}
};

static param_export_t params[]={
	{"mtree",          STR_PARAM|USE_FUNC_PARAM, (void*)mt_param},
	{"db_url",         STR_PARAM, &db_url.s},
	{"db_table",       STR_PARAM, &db_table.s},
	{"tname_column",   STR_PARAM, &tname_column.s},
	{"tprefix_column", STR_PARAM, &tprefix_column.s},
	{"tvalue_column",  STR_PARAM, &tvalue_column.s},
	{"char_list",      STR_PARAM, &mt_char_list.s},
	{"fetch_rows",     INT_PARAM, &mt_fetch_rows},
	{"pv_value",       STR_PARAM, &value_param.s},
	{"pv_dstid",       STR_PARAM, &dstid_param.s},
	{"pv_weight",      STR_PARAM, &weight_param.s},
	{"pv_count",       STR_PARAM, &count_param.s},
	{"mt_tree_type",   INT_PARAM, &_mt_tree_type},
	{"mt_ignore_duplicates", INT_PARAM, &_mt_ignore_duplicates},
	{0, 0, 0}
};

static mi_export_t mi_cmds[] = {
	{ "mt_reload",  mt_mi_reload,  0,  0,  mi_child_init },
	{ "mt_list",    mt_mi_list,    0,  0,  0 },
	{ "mt_summary", mt_mi_summary, 0,  0,  0 },
	{ 0, 0, 0, 0, 0}
};


struct module_exports exports = {
	"mtree",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,
	0,
	mi_cmds,        /* exported MI functions */
	0,              /* exported pseudo-variables */
	0,              /* extra processes */
	mod_init,       /* module initialization function */
	0,              /* response function */
	mod_destroy,    /* destroy function */
	child_init      /* per child init function */
};



/**
 * init module function
 */
static int mod_init(void)
{
	m_tree_t *pt = NULL;

	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	db_url.len = strlen(db_url.s);
	db_table.len = strlen(db_table.s);
	tname_column.len = strlen(tname_column.s);
	tprefix_column.len = strlen(tprefix_column.s);
	tvalue_column.len = strlen(tvalue_column.s);

	value_param.len = strlen(value_param.s);
	dstid_param.len = strlen(dstid_param.s);
	weight_param.len = strlen(weight_param.s);
	count_param.len = strlen(count_param.s);

	if(pv_parse_spec(&value_param, &pv_value)<00
			|| !(pv_is_w(&pv_value)))
	{
		LM_ERR("cannot parse value pv or is read only\n");
		return -1;
	}

	if(pv_parse_spec(&dstid_param, &pv_dstid)<0
			|| pv_dstid.type!=PVT_AVP)
	{
		LM_ERR("cannot parse dstid avp\n");
		return -1;
	}

	if(pv_parse_spec(&weight_param, &pv_weight)<0
			|| pv_weight.type!=PVT_AVP)
	{
		LM_ERR("cannot parse dstid avp\n");
		return -1;
	}

	if(pv_parse_spec(&count_param, &pv_count)<0
			|| !(pv_is_w(&pv_weight)))
	{
		LM_ERR("cannot parse count pv or is read-only\n");
		return -1;
	}

	if(mt_fetch_rows<=0)
		mt_fetch_rows = 1000;

	mt_char_list.len = strlen(mt_char_list.s);
	if(mt_char_list.len<=0)
	{
		LM_ERR("invalid prefix char list\n");
		return -1;
	}
	LM_INFO("mt_char_list=%s \n", mt_char_list.s);
	mt_char_table_init();

	/* binding to mysql module */
	if(db_bind_mod(&db_url, &mt_dbf))
	{
		LM_ERR("database module not found\n");
		return -1;
	}

	if (!DB_CAPABILITY(mt_dbf, DB_CAP_ALL))
	{
		LM_ERR("database module does not "
		    "implement all functions needed by the module\n");
		return -1;
	}

	/* open a connection with the database */
	db_con = mt_dbf.init(&db_url);
	if(db_con==NULL)
	{
		LM_ERR("failed to connect to the database\n");        
		return -1;
	}
	
	LM_DBG("database connection opened successfully\n");
	
	if ( (mt_lock=lock_alloc())==0) {
		LM_CRIT("failed to alloc lock\n");
		goto error1;
	}
	if (lock_init(mt_lock)==0 ) {
		LM_CRIT("failed to init lock\n");
		goto error1;
	}
	
	if(mt_defined_trees())
	{
		LM_DBG("static trees defined\n");

		pt = mt_get_first_tree();

		while(pt!=NULL)
		{
			/* loading all information from database */
			if(mt_load_db(&pt->tname)!=0)
			{
				LM_ERR("cannot load info from database\n");
				goto error1;
			}
			pt = pt->next;
		}
	} else {
		if(db_table.len<=0)
		{
			LM_ERR("no trees table defined\n");
			goto error1;
		}
		if(mt_init_list_head()<0)
		{
			LM_ERR("unable to init trees list head\n");
			goto error1;
		}
		/* loading all information from database */
		if(mt_load_db_trees()!=0)
		{
			LM_ERR("cannot load trees from database\n");
			goto error1;
		}
	}
	mt_dbf.close(db_con);
	db_con = 0;

#if 0
	mt_print_tree(mt_get_first_tree());
#endif

	/* success code */
	return 0;

error1:
	if (mt_lock)
	{
		lock_destroy( mt_lock );
		lock_dealloc( mt_lock );
		mt_lock = 0;
	}
	mt_destroy_trees();

	if(db_con!=NULL)
		mt_dbf.close(db_con);
	db_con = 0;
	return -1;
}

/**
 * mi and worker process initialization
 */
static int mi_child_init(void)
{
	db_con = mt_dbf.init(&db_url);
	if(db_con==NULL)
	{
		LM_ERR("failed to connect to database\n");
		return -1;
	}

	return 0;
}


/* each child get a new connection to the database */
static int child_init(int rank)
{
	/* skip child init for non-worker process ranks */
	if (rank==PROC_INIT || rank==PROC_MAIN || rank==PROC_TCP_MAIN)
		return 0;

	if ( mi_child_init()!=0 )
		return -1;

	LM_DBG("#%d: database connection opened successfully\n", rank);

	return 0;
}


static void mod_destroy(void)
{
	LM_DBG("cleaning up\n");
	mt_destroy_trees();
	if (db_con!=NULL && mt_dbf.close!=NULL)
		mt_dbf.close(db_con);
		/* destroy lock */
	if (mt_lock)
	{
		lock_destroy( mt_lock );
		lock_dealloc( mt_lock );
		mt_lock = 0;
	}

}

static int fixup_mt_match(void** param, int param_no)
{
    if(param_no==1 || param_no==2) {
		return fixup_spve_null(param, 1);
    }
    if (param_no != 3)	{
		LM_ERR("invalid parameter number %d\n", param_no);
		return E_UNSPEC;
    }
    return fixup_igp_null(param, 1);
}


/* use tree tn, match var, by mode, output in avp params */
static int mt_match(struct sip_msg *msg, gparam_t *tn, gparam_t *var,
		gparam_t *mode)
{
	str tname;
	str tomatch;
	int mval;
	m_tree_t *tr = NULL;
	
	if(msg==NULL)
	{
		LM_ERR("received null msg\n");
		return -1;
	}

	if(fixup_get_svalue(msg, tn, &tname)<0)
	{
		LM_ERR("cannot get the tree name\n");
		return -1;
	}
	if(fixup_get_svalue(msg, var, &tomatch)<0)
	{
		LM_ERR("cannot get the match var\n");
		return -1;
	}
	if(fixup_get_ivalue(msg, mode, &mval)<0)
	{
		LM_ERR("cannot get the mode\n");
		return -1;
	}
	
again:
	lock_get( mt_lock );
	if (mt_reload_flag) {
		lock_release( mt_lock );
		sleep_us(5);
		goto again;
	}
	mt_tree_refcnt++;
	lock_release( mt_lock );

	tr = mt_get_tree(&tname);
	if(tr==NULL)
	{
		/* no tree with such name*/
		goto error;
	}

	if(mt_match_prefix(msg, tr, &tomatch, mval)<0)
	{
		LM_INFO("no prefix found in [%.*s] for [%.*s]\n",
				tname.len, tname.s,
				tomatch.len, tomatch.s);
		goto error;
	}
	
	lock_get( mt_lock );
	mt_tree_refcnt--;
	lock_release( mt_lock );
	return 1;

error:
	lock_get( mt_lock );
	mt_tree_refcnt--;
	lock_release( mt_lock );
	return -1;
}

static int w_mt_match(struct sip_msg* msg, char* str1, char* str2,
		char* str3)
{
	return mt_match(msg, (gparam_t*)str1, (gparam_t*)str2, (gparam_t*)str3);
}

int mt_param(modparam_t type, void *val)
{
	if(val==NULL)
		goto error;

	return mt_table_spec((char*)val);
error:
	return -1;

}

static int mt_load_db(str *tname)
{
	db_key_t db_cols[3] = {&tprefix_column, &tvalue_column};
	str tprefix, tvalue;
	db1_res_t* db_res = NULL;
	int i, ret;
	m_tree_t new_tree; 
	m_tree_t *old_tree = NULL; 
	mt_node_t *bk_head = NULL; 

	if(db_con==NULL)
	{
		LM_ERR("no db connection\n");
		return -1;
	}
	LM_ERR("attempting to load [%.*s]\n", tname->len, tname->s);
	old_tree = mt_get_tree(tname);
	if(old_tree==NULL)
	{
		LM_ERR("tree definition not found [%.*s]\n", tname->len, tname->s);
		return -1;
	}
	memcpy(&new_tree, old_tree, sizeof(m_tree_t));
	new_tree.head = 0;
	new_tree.next = 0;

	if (mt_dbf.use_table(db_con, &old_tree->dbtable) < 0)
	{
		LM_ERR("failed to use_table\n");
		return -1;
	}

	if (DB_CAPABILITY(mt_dbf, DB_CAP_FETCH)) {
		if(mt_dbf.query(db_con, 0, 0, 0, db_cols, 0, 2, 0, 0) < 0)
		{
			LM_ERR("Error while querying db\n");
			return -1;
		}
		if(mt_dbf.fetch_result(db_con, &db_res, mt_fetch_rows)<0)
		{
			LM_ERR("Error while fetching result\n");
			if (db_res)
				mt_dbf.free_result(db_con, db_res);
			goto error;
		} else {
			if(RES_ROW_N(db_res)==0)
			{
				return 0;
			}
		}
	} else {
		if((ret=mt_dbf.query(db_con, NULL, NULL, NULL, db_cols,
				0, 2, 0, &db_res))!=0
			|| RES_ROW_N(db_res)<=0 )
		{
			mt_dbf.free_result(db_con, db_res);
			if( ret==0)
			{
				return 0;
			} else {
				goto error;
			}
		}
	}

	do {
		for(i=0; i<RES_ROW_N(db_res); i++)
		{
			/* check for NULL values ?!?! */
			tprefix.s = (char*)(RES_ROWS(db_res)[i].values[0].val.string_val);
			tprefix.len = strlen(tprefix.s);
			
			tvalue.s = (char*)(RES_ROWS(db_res)[i].values[1].val.string_val);
			tvalue.len = strlen(tvalue.s);

			if(tprefix.s==NULL || tvalue.s==NULL
					|| tprefix.len<=0 || tvalue.len<=0)
			{
				LM_ERR("Error - bad values in db\n");
				continue;
			}
		
			if(mt_add_to_tree(&new_tree, &tprefix, &tvalue)<0)
			{
				LM_ERR("Error adding info to tree\n");
				goto error;
			}
	 	}
		if (DB_CAPABILITY(mt_dbf, DB_CAP_FETCH)) {
			if(mt_dbf.fetch_result(db_con, &db_res, mt_fetch_rows)<0) {
				LM_ERR("Error while fetching!\n");
				if (db_res)
					mt_dbf.free_result(db_con, db_res);
				goto error;
			}
		} else {
			break;
		}
	}  while(RES_ROW_N(db_res)>0);
	mt_dbf.free_result(db_con, db_res);


	/* block all readers */
	lock_get( mt_lock );
	mt_reload_flag = 1;
	lock_release( mt_lock );

	while (mt_tree_refcnt) {
		sleep_us(10);
	}

	bk_head = old_tree->head;
	old_tree->head = new_tree.head;

	mt_reload_flag = 0;

	/* free old data */
	if (bk_head!=NULL)
		mt_free_node(bk_head, new_tree.type);

	return 0;

error:
	mt_dbf.free_result(db_con, db_res);
	if (new_tree.head!=NULL)
		mt_free_node(new_tree.head, new_tree.type);
	return -1;
}

static int mt_load_db_trees()
{
	db_key_t db_cols[3] = {&tname_column, &tprefix_column, &tvalue_column};
	str tprefix, tvalue, tname;
	db1_res_t* db_res = NULL;
	int i, ret;
	m_tree_t *new_head = NULL;
	m_tree_t *new_tree = NULL;
	m_tree_t *old_head = NULL;

	if(db_con==NULL)
	{
		LM_ERR("no db connection\n");
		return -1;
	}

	if (mt_dbf.use_table(db_con, &db_table) < 0)
	{
		LM_ERR("failed to use_table\n");
		return -1;
	}

	if (DB_CAPABILITY(mt_dbf, DB_CAP_FETCH))
	{
		if(mt_dbf.query(db_con,0,0,0,db_cols,0,3,&tname_column,0) < 0)
		{
			LM_ERR("Error while querying db\n");
			return -1;
		}
		if(mt_dbf.fetch_result(db_con, &db_res, mt_fetch_rows)<0)
		{
			LM_ERR("Error while fetching result\n");
			if (db_res)
				mt_dbf.free_result(db_con, db_res);
			goto error;
		} else {
			if(RES_ROW_N(db_res)==0)
			{
				return 0;
			}
		}
	} else {
		if((ret=mt_dbf.query(db_con, NULL, NULL, NULL, db_cols,
				0, 3, &tname_column, &db_res))!=0
			|| RES_ROW_N(db_res)<=0 )
		{
			mt_dbf.free_result(db_con, db_res);
			if( ret==0)
			{
				return 0;
			} else {
				goto error;
			}
		}
	}

	do {
		for(i=0; i<RES_ROW_N(db_res); i++)
		{
			/* check for NULL values ?!?! */
			tname.s = (char*)(RES_ROWS(db_res)[i].values[0].val.string_val);
			tname.len = strlen(tname.s);

			tprefix.s = (char*)(RES_ROWS(db_res)[i].values[1].val.string_val);
			tprefix.len = strlen(tprefix.s);

			tvalue.s = (char*)(RES_ROWS(db_res)[i].values[2].val.string_val);
			tvalue.len = strlen(tvalue.s);

			if(tprefix.s==NULL || tvalue.s==NULL || tname.s==NULL ||
				tprefix.len<=0 || tvalue.len<=0 || tname.len<=0)
			{
				LM_ERR("Error - bad values in db\n");
				continue;
			}
			new_tree = mt_add_tree(&new_head, &tname, &db_table, _mt_tree_type);
			if(new_tree==NULL)
			{
				LM_ERR("New tree cannot be initialized\n");
				goto error;
			}
			if(mt_add_to_tree(new_tree, &tprefix, &tvalue)<0)
			{
				LM_ERR("Error adding info to tree\n");
				goto error;
			}
		}
		if (DB_CAPABILITY(mt_dbf, DB_CAP_FETCH)) {
			if(mt_dbf.fetch_result(db_con, &db_res, mt_fetch_rows)<0) {
				LM_ERR("Error while fetching!\n");
				if (db_res)
					mt_dbf.free_result(db_con, db_res);
				goto error;
			}
		} else {
			break;
		}
	} while(RES_ROW_N(db_res)>0);
	mt_dbf.free_result(db_con, db_res);

	/* block all readers */
	lock_get( mt_lock );
	mt_reload_flag = 1;
	lock_release( mt_lock );

	while (mt_tree_refcnt) {
		sleep_us(10);
	}

	old_head = mt_swap_list_head(new_head);

	mt_reload_flag = 0;
	/* free old data */
	if (old_head!=NULL)
		mt_free_tree(old_head);

	return 0;

error:
	mt_dbf.free_result(db_con, db_res);
	if (new_head!=NULL)
		mt_free_tree(new_head);
	return -1;
}

/**************************** MI ***************************/

/**
 * "mt_reload" syntax :
 * \n
 */
static struct mi_root* mt_mi_reload(struct mi_root *cmd_tree, void *param)
{
	str tname = {0, 0};
	m_tree_t *pt;
	struct mi_node* node = NULL;

	if(db_table.len>0)
	{
		/* re-loading all information from database */
		if(mt_load_db_trees()!=0)
		{
			LM_ERR("cannot re-load info from database\n");
			goto error;
		}
	} else {
		if(!mt_defined_trees())
		{
			LM_ERR("empty tree list\n");
			return init_mi_tree( 500, MI_INTERNAL_ERR_S, MI_INTERNAL_ERR_LEN);
		}

		/* read tree name */
		node = cmd_tree->node.kids;
		if(node != NULL)
		{
			tname = node->value;
			if(tname.s == NULL || tname.len== 0)
				return init_mi_tree( 404, "domain not found", 16);

			if(*tname.s=='.') {
				tname.s = 0;
				tname.len = 0;
			}
		}

		pt = mt_get_first_tree();
	
		while(pt!=NULL)
		{
			if(tname.s==NULL
				|| (tname.s!=NULL && pt->tname.len>=tname.len
					&& strncmp(pt->tname.s, tname.s, tname.len)==0))
			{
				/* re-loading table from database */
				if(mt_load_db(&pt->tname)!=0)
				{
					LM_ERR("cannot re-load info from database\n");	
					goto error;
				}
			}
			pt = pt->next;
		}
	}
	
	return init_mi_tree( 200, MI_OK_S, MI_OK_LEN);

error:
	return init_mi_tree( 500, "Failed to reload",16);
}


int mt_print_mi_node(m_tree_t *tree, mt_node_t *pt, struct mi_node* rpl,
		char *code, int len)
{
	int i;
	struct mi_node* node = NULL;
	struct mi_attr* attr= NULL;

	if(pt==NULL || len>=MT_MAX_DEPTH)
		return 0;
	
	for(i=0; i<MT_NODE_SIZE; i++)
	{
		code[len]=mt_char_list.s[i];
		if(pt[i].tvalue.s!=NULL)
		{
			node = add_mi_node_child(rpl, 0, "MT", 2, 0, 0);
			if(node == NULL)
				goto error;
			attr = add_mi_attr(node, MI_DUP_VALUE, "TNAME", 5,
					tree->tname.s, tree->tname.len);
			if(attr == NULL)
				goto error;
			attr = add_mi_attr(node, MI_DUP_VALUE, "TPREFIX", 7,
						code, len+1);
			if(attr == NULL)
				goto error;
					
			attr = add_mi_attr(node, MI_DUP_VALUE, "TVALUE", 6,
						pt[i].tvalue.s, pt[i].tvalue.len);
			if(attr == NULL)
				goto error;
		}
		if(mt_print_mi_node(tree, pt[i].child, rpl, code, len+1)<0)
			goto error;
	}
	return 0;
error:
	return -1;
}

/**
 * "mt_list" syntax :
 *    tname
 *
 * 	- '.' (dot) means NULL value and will match anything
 */

#define strpos(s,c) (strchr(s,c)-s)
struct mi_root* mt_mi_list(struct mi_root* cmd_tree, void* param)
{
	str tname = {0, 0};
	m_tree_t *pt;
	struct mi_node* node = NULL;
	struct mi_root* rpl_tree = NULL;
	struct mi_node* rpl = NULL;
	static char code_buf[MT_MAX_DEPTH+1];
	int len;

	if(!mt_defined_trees())
	{
		LM_ERR("empty tree list\n");
		return init_mi_tree( 500, MI_INTERNAL_ERR_S, MI_INTERNAL_ERR_LEN);
	}

	/* read tree name */
	node = cmd_tree->node.kids;
	if(node != NULL)
	{
		tname = node->value;
		if(tname.s == NULL || tname.len== 0)
			return init_mi_tree( 404, "domain not found", 16);

		if(*tname.s=='.') {
			tname.s = 0;
			tname.len = 0;
		}
	}

	rpl_tree = init_mi_tree(200, MI_OK_S, MI_OK_LEN);
	if(rpl_tree == NULL)
		return 0;
	rpl = &rpl_tree->node;

	pt = mt_get_first_tree();
	
	while(pt!=NULL)
	{
		if(tname.s==NULL || 
			(tname.s!=NULL && pt->tname.len>=tname.len && 
			 strncmp(pt->tname.s, tname.s, tname.len)==0))
		{
			len = 0;
			if(mt_print_mi_node(pt, pt->head, rpl, code_buf, len)<0)
				goto error;
		}
		pt = pt->next;
	}
	
	return rpl_tree;

error:
	free_mi_tree(rpl_tree);
	return 0;
}

struct mi_root* mt_mi_summary(struct mi_root* cmd_tree, void* param)
{
	m_tree_t *pt;
	struct mi_root* rpl_tree = NULL;
	struct mi_node* node = NULL;
	struct mi_attr* attr= NULL;
	str val;

	if(!mt_defined_trees())
	{
		LM_ERR("empty tree list\n");
		return init_mi_tree( 500, "No trees", 8);
	}

	rpl_tree = init_mi_tree(200, MI_OK_S, MI_OK_LEN);
	if(rpl_tree == NULL)
		return 0;

	pt = mt_get_first_tree();

	while(pt!=NULL)
	{
		node = add_mi_node_child(&rpl_tree->node, 0, "MT", 2, 0, 0);
		if(node == NULL)
			goto error;
		attr = add_mi_attr(node, MI_DUP_VALUE, "TNAME", 5,
				pt->tname.s, pt->tname.len);
		if(attr == NULL)
			goto error;
		val.s = int2str(pt->memsize, &val.len);
		attr = add_mi_attr(node, MI_DUP_VALUE, "MEMSIZE", 7,
				val.s, val.len);
		if(attr == NULL)
			goto error;
		val.s = int2str(pt->nrnodes, &val.len);
		attr = add_mi_attr(node, MI_DUP_VALUE, "NRNODES", 7,
				val.s, val.len);
		if(attr == NULL)
			goto error;
		val.s = int2str(pt->nritems, &val.len);
		attr = add_mi_attr(node, MI_DUP_VALUE, "NRITEMS", 7,
				val.s, val.len);
		if(attr == NULL)
			goto error;

		pt = pt->next;
	}

	return rpl_tree;
error:
	free_mi_tree(rpl_tree);
	return 0;
}
