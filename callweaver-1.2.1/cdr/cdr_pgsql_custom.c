/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * cdr_pgsql_custom.c <PostgreSQL module for CDR logging with custom columns>
 * 
 * Copyright (C) 2005 Business Technology Group (http://www.btg.co.nz)
 *  Danel Swarbrick <daniel@btg.co.nz>
 *
 * Based in part on original by Matthew D. Hardeman <mhardemn@papersoft.com>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Custom PostgreSQL CDR logger
 *
 * \author Mikael Bjerkeland <mikael@bjerkeland.com>
 * based on original code by Daniel Swarbrick <daniel@btg.co.nz>
 *
 * See also
 * \arg \ref Config_cdr
 * \arg http://www.postgresql.org/
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/cdr/cdr_pgsql_custom.c $", "$Revision: 4723 $")

#include "callweaver/channel.h"
#include "callweaver/cdr.h"
#include "callweaver/module.h"
#include "callweaver/config.h"
#include "callweaver/pbx.h"
#include "callweaver/logger.h"
#include "callweaver/utils.h"

#define DATE_FORMAT "%Y-%m-%d %T"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <libpq-fe.h>

static char *desc = "Custom PostgreSQL CDR Backend";
static char *name = "pgsql_custom";

CW_MUTEX_DEFINE_STATIC(pgsql_lock);
#define CDR_PGSQL_CONF "cdr_pgsql_custom.conf"
static char conninfo[512];
static char table[128];
static char columns[1024];
static char values[1024];

static PGconn *conn = NULL;

static int parse_config(void);
static int pgsql_reconnect(void);

static int parse_config(void)
{
	struct cw_config *config;
	char *s;

	config = cw_config_load(CDR_PGSQL_CONF);

	if (config) {

		/* get the PostgreSQL DSN */
		s = cw_variable_retrieve(config, "global", "dsn");
		if (s == NULL) {
			cw_log(LOG_WARNING, "cdr_pgsql_custom: No DSN found, using 'dbname=callweaver user=callweaver'.\n");
			strncpy(conninfo, "dbname=callweaver user=callweaver", sizeof(conninfo));
		} else {
			strncpy(conninfo, s, sizeof(conninfo));
		}
		
		/* get the CDR table name */
		s = cw_variable_retrieve(config, "global", "table");
		if (s == NULL) {
			cw_log(LOG_WARNING, "No database table found, assuming 'cdr'.\n");
			strncpy(table, "cdr", sizeof(table));
		} else {
			strncpy(table, s, sizeof(table));
		}

		/* get the CDR columns */
                s = cw_variable_retrieve(config, "master", "columns");
                if (s == NULL) {
                        cw_log(LOG_WARNING, "Column names not specified. Module not loaded.\n");
			return -1;
                } else {
                        strncpy(columns, s, sizeof(columns));
                }

		/* get the CDR column values */
                s = cw_variable_retrieve(config, "master", "values");
                if (s == NULL) {
                        cw_log(LOG_WARNING, "Values not specified. Module not loaded.\n");
			return -1;
                } else {
                        strncpy(values, s, sizeof(values));
                }
		
		if (columns != NULL && values != NULL)
			cw_log(LOG_NOTICE, "Using column layout: %s.\n", columns);


	} else {
		cw_log(LOG_WARNING, "Config file (%s) not found.\n", CDR_PGSQL_CONF);
	}
	cw_config_destroy(config);

	return 1;
}

static int pgsql_reconnect(void)
{
	if (conn != NULL) {
		/* we may already be connected */
		if (PQstatus(conn) == CONNECTION_OK) {
			return 1;
		} else {
			cw_log(LOG_NOTICE, "Existing database connection broken. Trying to reset.\n");

			/* try to reset the connection */
			if (PQstatus(conn) != CONNECTION_BAD)
				PQreset(conn);

			/* check the connection status again */
			if (PQstatus(conn) == CONNECTION_OK) {
				cw_log(LOG_NOTICE, "Existing database connection reset ok.\n");
				return 1;
			} else {
				/* still no luck, tear down the connection and we'll make a new connection */
				cw_log(LOG_NOTICE, "Unable to reset existing database connection.\n");
				PQfinish(conn);
			}
		}
	}

	conn = PQconnectdb(conninfo);

	if (PQstatus(conn) == CONNECTION_OK) {
		cw_log(LOG_NOTICE, "Successfully connected to PostgreSQL database.\n");
		return 1;
	} else {
		cw_log(LOG_WARNING, "Couldn't establish DB connection. Check debug.\n");
		cw_log(LOG_ERROR, "Reason %s\n", PQerrorMessage(conn));
	}		

	return -1;
}

static int pgsql_log(struct cw_cdr *cdr)
{
	PGresult *res;
	char sql_insert_cmd[2048];
	char sql_tmp_cmd[1024];

	struct cw_channel dummy;


	cw_log(LOG_DEBUG,"Inserting a CDR record.\n");

	snprintf(sql_tmp_cmd, sizeof(sql_tmp_cmd), "INSERT INTO %s (%s) VALUES (%s)", table, columns, values);

	/* Not quite the first use of a static struct ast_channel, we need it so the var funcs will work */
	memset(&dummy, 0, sizeof(dummy));
        dummy.cdr = cdr;
        pbx_substitute_variables_helper(&dummy, sql_tmp_cmd, sql_insert_cmd, sizeof(sql_insert_cmd));


	cw_log(LOG_DEBUG, "SQL command executed:  %s\n", sql_insert_cmd);

	/* check if database connection is still good */
	if (!pgsql_reconnect()) {
		cw_log(LOG_ERROR, "Unable to reconnect to database server. Some calls will not be logged!\n");
		return -1;
	}

	cw_mutex_lock(&pgsql_lock);
	res = PQexec(conn, sql_insert_cmd);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		cw_log(LOG_ERROR, "Failed to insert call detail record into database!\n");
		cw_log(LOG_ERROR, "Reason: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		cw_mutex_unlock(&pgsql_lock);
		return -1;
	}

	PQclear(res);
	cw_mutex_unlock(&pgsql_lock);
	return 0;
}

char *description(void)
{
	return desc;
}

static int my_unload_module(void)
{ 
	cw_cdr_unregister(name);
	return 0;
}

static int my_load_module(void)
{
	int res;

	parse_config();
	
	pgsql_reconnect();

	res = cw_cdr_register(name, desc, pgsql_log);
	if (res) {
		cw_log(LOG_ERROR, "Unable to register PGSQL CDR handling\n");
	}

	return res;
}

int load_module(void)
{
	return my_load_module();
}

int unload_module(void)
{
	return my_unload_module();
}

int reload(void)
{
	my_unload_module();
	return my_load_module();
}

int usecount(void)
{
	/* To be able to unload the module */
	if ( cw_mutex_trylock(&pgsql_lock) ) {
		return 1;
	} else {
		cw_mutex_unlock(&pgsql_lock);
		return 0;
	}
}



