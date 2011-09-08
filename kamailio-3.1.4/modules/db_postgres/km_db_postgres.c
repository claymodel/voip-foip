/*
 * $Id$ 
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

/*! \file
 *  \brief DB_POSTGRES :: Core
 *  \ingroup db_postgres
 *  Module: \ref db_postgres
 */

/*! \defgroup db_postgres DB_POSTGRES :: the PostgreSQL driver for Kamailio
 *  \brief The Kamailio database interface to the PostgreSQL database
 *  - http://www.postgresql.org
 *
 */

#include <stdio.h>
#include "../../sr_module.h"
#include "../../lib/srdb1/db_con.h"
#include "../../lib/srdb1/db.h"
#include "km_dbase.h"
#include "km_db_postgres.h"


/*MODULE_VERSION*/

/*
 * PostgreSQL database module interface
 */

static kam_cmd_export_t cmds[]={
	{"db_bind_api",     (cmd_function)db_postgres_bind_api,     0, 0, 0, 0},
	{0,0,0,0,0,0}
};



struct kam_module_exports kam_exports = {
	"db_postgres",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	0,        /*  module parameters */
	0,        /* exported statistics */
	0,        /* exported MI functions */
	0,        /* exported pseudo-variables */
	0,        /* extra processes */
	km_postgres_mod_init, /* module initialization function */
	0,        /* response function*/
	0,        /* destroy function */
	0         /* per-child init function */
};


int km_postgres_mod_init(void)
{
	return 0;
}

int db_postgres_bind_api(db_func_t *dbb)
{
	if(dbb==NULL)
		return -1;

	memset(dbb, 0, sizeof(db_func_t));

	dbb->use_table        = db_postgres_use_table;
	dbb->init             = db_postgres_init;
	dbb->close            = db_postgres_close;
	dbb->query            = db_postgres_query;
	dbb->fetch_result     = db_postgres_fetch_result;
	dbb->raw_query        = db_postgres_raw_query;
	dbb->free_result      = db_postgres_free_result;
	dbb->insert           = db_postgres_insert;
	dbb->delete           = db_postgres_delete; 
	dbb->update           = db_postgres_update;

	return 0;
}
