/*
 * $Id$ 
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
 *  2003-02-25 - created by janakj
 *  2003-03-11 - New module interface (janakj)
 *  2003-03-16 - flags export parameter added (janakj)
 *  2003-03-19  all mallocs/frees replaced w/ pkg_malloc/pkg_free
 *  2003-04-05  default_uri #define used (jiri)
 *  2004-06-07  updated to the new DB api: calls to group_db_* (andrei)
 *  2005-10-06 - added support for regexp-based groups (bogdan)
 */

/**
 * \file
 * \brief Group membership module
 * \ingroup group
 * - Module: \ref group
 */

/*!
 * \defgroup group GROUP :: The Kamailio group Module
 * This module provides functions to check if a certain user belongs to a
 * group. This group definitions are read from a DB table.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../error.h"
#include "../../mem/mem.h"
#include "../../usr_avp.h"
#include "group_mod.h"
#include "group.h"
#include "re_group.h"

MODULE_VERSION

#define TABLE_VERSION    2
#define RE_TABLE_VERSION 1

/*!
 * \brief Module destroy function prototype
 */
static void destroy(void);


/*!
 * \brief Module child-init function prototype
 */
static int child_init(int rank);


/*!
 * \brief Module initialization function prototype
 */
static int mod_init(void);

/*! Header field fixup */
static int hf_fixup(void** param, int param_no);

/*! get user group ID fixup */
static int get_gid_fixup(void** param, int param_no);


#define TABLE "grp"
#define TABLE_LEN (sizeof(TABLE) - 1)

#define USER_COL "username"
#define USER_COL_LEN (sizeof(USER_COL) - 1)

#define DOMAIN_COL "domain"
#define DOMAIN_COL_LEN (sizeof(DOMAIN_COL) - 1)

#define GROUP_COL "grp"
#define GROUP_COL_LEN (sizeof(GROUP_COL) - 1)

#define RE_TABLE "re_grp"
#define RE_TABLE_LEN (sizeof(TABLE) - 1)

#define RE_EXP_COL "reg_exp"
#define RE_EXP_COL_LEN (sizeof(USER_COL) - 1)

#define RE_GID_COL "group_id"
#define RE_GID_COL_LEN (sizeof(DOMAIN_COL) - 1)

/*
 * Module parameter variables
 */
static str db_url = {DEFAULT_RODB_URL, DEFAULT_RODB_URL_LEN};
/*! Table name where group definitions are stored */
str table         = {TABLE, TABLE_LEN}; 
str user_column   = {USER_COL, USER_COL_LEN};
str domain_column = {DOMAIN_COL, DOMAIN_COL_LEN};
str group_column  = {GROUP_COL, GROUP_COL_LEN};
int use_domain    = 0;

/* table and columns used for regular expression-based groups */
str re_table      = {0, 0};
str re_exp_column = {RE_EXP_COL, RE_EXP_COL_LEN};
str re_gid_column = {RE_GID_COL, RE_GID_COL_LEN};
int multiple_gid  = 1;

/* DB functions and handlers */
db_func_t group_dbf;
db1_con_t* group_dbh = 0;


/*!
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"is_user_in",      (cmd_function)is_user_in,      2,  hf_fixup, 0,
			REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"get_user_group",  (cmd_function)get_user_group,  2,  get_gid_fixup, 0,
			REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{0, 0, 0, 0, 0, 0}
};


/*!
 * Exported parameters
 */
static param_export_t params[] = {
	{"db_url",        STR_PARAM, &db_url.s       },
	{"table",         STR_PARAM, &table.s        },
	{"user_column",   STR_PARAM, &user_column.s  },
	{"domain_column", STR_PARAM, &domain_column.s},
	{"group_column",  STR_PARAM, &group_column.s },
	{"use_domain",    INT_PARAM, &use_domain     },
	{"re_table",      STR_PARAM, &re_table.s     },
	{"re_exp_column", STR_PARAM, &re_exp_column.s},
	{"re_gid_column", STR_PARAM, &re_gid_column.s},
	{"multiple_gid",  INT_PARAM, &multiple_gid   },
	{0, 0, 0}
};


/*!
 * Module interface
 */
struct module_exports exports = {
	"group", 
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,       /* Exported functions */
	params,     /* Exported parameters */
	0,          /* exported statistics */
	0,          /* exported MI functions */
	0,          /* exported pseudo-variables */
	0,          /* extra processes */
	mod_init,   /* module initialization function */
	0,          /* response function */
	destroy,    /* destroy function */
	child_init  /* child initialization function */
};


static int child_init(int rank)
{
	if (rank==PROC_INIT || rank==PROC_MAIN || rank==PROC_TCP_MAIN)
		return 0; /* do nothing for the main process */

	return group_db_init(&db_url);
}


static int mod_init(void)
{
	/* Calculate lengths */
	db_url.len = strlen(db_url.s);
	table.len = strlen(table.s);
	user_column.len = strlen(user_column.s);
	domain_column.len = strlen(domain_column.s);
	group_column.len = strlen(group_column.s);

	re_table.len = (re_table.s && re_table.s[0])?strlen(re_table.s):0;
	re_exp_column.len = strlen(re_exp_column.s);
	re_gid_column.len = strlen(re_gid_column.s);

	/* Find a database module */
	if (group_db_bind(&db_url)) {
		return -1;
	}

	if (group_db_init(&db_url) < 0 ){
		LM_ERR("unable to open database connection\n");
		return -1;
	}

	/* check version for group table */
	if (db_check_table_version(&group_dbf, group_dbh, &table, TABLE_VERSION) < 0) {
			LM_ERR("error during group table version check.\n");
			return -1;
	}

	if (re_table.len) {
		/* check version for group re_group table */
		if (db_check_table_version(&group_dbf, group_dbh, &re_table, RE_TABLE_VERSION) < 0) {
			LM_ERR("error during re_group table version check.\n");
			return -1;
		}
		if (load_re( &re_table )!=0 ) {
			LM_ERR("failed to load <%s> table\n", re_table.s);
			return -1;
		}
	}

	group_db_close();
	return 0;
}


static void destroy(void)
{
	group_db_close();
}


/*!
 * \brief Convert HF description string to hdr_field pointer
 *
 * Convert a header field description string to hdr_field structure
 * Supported strings: "Request-URI", "To", "From", "Credentials"
 * \param str1 header field description string
 * \return hdr_field structure on success, NULL on failure
 */
static group_check_p get_hf( char *str1)
{
	group_check_p gcp=NULL;
	str s;

	gcp = (group_check_p)pkg_malloc(sizeof(group_check_t));
	if(gcp == NULL) {
		LM_ERR("no pkg more memory\n");
		return 0;
	}
	memset(gcp, 0, sizeof(group_check_t));

	if (!strcasecmp( str1, "Request-URI")) {
		gcp->id = 1;
	} else if (!strcasecmp( str1, "To")) {
		gcp->id = 2;
	} else if (!strcasecmp( str1, "From")) {
		gcp->id = 3;
	} else if (!strcasecmp( str1, "Credentials")) {
		gcp->id = 4;
	} else {
		s.s = str1; s.len = strlen(s.s);
		if(pv_parse_spec( &s, &gcp->sp)==NULL
			|| gcp->sp.type!=PVT_AVP)
		{
			LM_ERR("unsupported User Field identifier\n");
			pkg_free( gcp );
			return 0;
		}
		gcp->id = 5;
	}

	/* do not free all the time, needed by pseudo-variable spec */
	if(gcp->id!=5)
		pkg_free(str1);

	return gcp;
}


/*!
 * \brief Header fixup function
 * \param param fixed parameter
 * \param param_no number of parameters
 * \return 0 on success, negative on failure
 */
static int hf_fixup(void** param, int param_no)
{
	void* ptr;
	str* s;

	if (param_no == 1) {
		ptr = *param;
		if ( (*param = (void*)get_hf( ptr ))==0 )
			return E_UNSPEC;
	} else if (param_no == 2) {
		s = (str*)pkg_malloc(sizeof(str));
		if (!s) {
			LM_ERR("no pkg memory left\n");
			return E_UNSPEC;
		}
		s->s = (char*)*param;
		s->len = strlen(s->s);
		*param = (void*)s;
	}

	return 0;
}


/*!
 * \brief Group ID fixup
 * \param param fixed parameter
 * \param param_no number of parameters
 * \return 0 on success, negative on failure
 */
static int get_gid_fixup(void** param, int param_no)
{
	pv_spec_t *sp;
	void *ptr;
	str  name;

	if (param_no == 1) {
		ptr = *param;
		if ( (*param = (void*)get_hf( ptr ))==0 )
			return E_UNSPEC;
	} else if (param_no == 2) {
		name.s = (char*)*param;
		name.len = strlen(name.s);
		sp = (pv_spec_t*)pkg_malloc(sizeof(pv_spec_t));
		if (sp == NULL) {
			LM_ERR("no more pkg memory\n");
			return E_UNSPEC;
		}
		if(pv_parse_spec(&name, sp)==NULL || sp->type!=PVT_AVP)
		{
			LM_ERR("bad AVP spec <%s>\n", name.s);
			pv_spec_free(sp);
			return E_UNSPEC;
		}

		*param = sp;
	}

	return 0;
}
