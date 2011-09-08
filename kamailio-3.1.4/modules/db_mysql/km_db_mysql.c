/* 
 * $Id$ 
 *
 * MySQL module interface
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2008 1&1 Internet AG
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
/*
 * History:
 * --------
 *  2003-03-11  updated to the new module exports interface (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 */

/*! \file
 *  \brief DB_MYSQL :: Core
 *  \ingroup db_mysql
 *  Module: \ref db_mysql
 */

/*! \defgroup db_mysql DB_MYSQL :: the MySQL driver for Kamailio
 *  \brief The Kamailio database interface to the MySQL database
 *  - http://www.mysql.org
 *
 */

#include "../../sr_module.h"
#include "../../dprint.h"
#include "km_dbase.h"
#include "km_db_mysql.h"

#include <mysql/mysql.h>

unsigned int db_mysql_timeout_interval = 2;   /* Default is 6 seconds */
unsigned int db_mysql_auto_reconnect = 1;     /* Default is enabled   */

/* MODULE_VERSION */

/*! \brief
 * MySQL database module interface
 */
static kam_cmd_export_t cmds[] = {
	{"db_bind_api",         (cmd_function)db_mysql_bind_api,      0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

/*! \brief
 * Exported parameters
 */
static param_export_t params[] = {
/*	{"ping_interval",    INT_PARAM, &db_mysql_ping_interval}, */
	{"timeout_interval", INT_PARAM, &db_mysql_timeout_interval},
	{"auto_reconnect",   INT_PARAM, &db_mysql_auto_reconnect},
	{0, 0, 0}
};

struct kam_module_exports kam_exports = {	
	"db_mysql",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,          /*  module parameters */
	0,               /* exported statistics */
	0,               /* exported MI functions */
	0,               /* exported pseudo-variables */
	0,               /* extra processes */
	kam_mysql_mod_init,  /* module initialization function */
	0,               /* response function*/
	0,               /* destroy function */
	0                /* per-child init function */
};


int kam_mysql_mod_init(void)
{
	LM_DBG("MySQL client version is %s\n", mysql_get_client_info());
	return 0;
}

int db_mysql_bind_api(db_func_t *dbb)
{
	if(dbb==NULL)
		return -1;

	memset(dbb, 0, sizeof(db_func_t));

	dbb->use_table        = db_mysql_use_table;
	dbb->init             = db_mysql_init;
	dbb->close            = db_mysql_close;
	dbb->query            = db_mysql_query;
	dbb->fetch_result     = db_mysql_fetch_result;
	dbb->raw_query        = db_mysql_raw_query;
	dbb->free_result      = db_mysql_free_result;
	dbb->insert           = db_mysql_insert;
	dbb->delete           = db_mysql_delete;
	dbb->update           = db_mysql_update;
	dbb->replace          = db_mysql_replace;
	dbb->last_inserted_id = db_last_inserted_id;
	dbb->insert_update    = db_insert_update;

	return 0;
}

