/*
 * $Id$
 *
 * BDB Database Driver for SIP-router
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * SIP-router is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef _BDB_CMD_H_
#define _BDB_CMD_H_

/** \addtogroup bdb
 * @{
 */

/*! \file
 * Berkeley DB : 
 * Declaration of bdb_cmd data structure that contains BDB specific data
 * stored in db_cmd structures and related functions.
 *
 * \ingroup database
 */

#include <stdarg.h>
#include <sys/time.h>
#include <db.h>

#include "../../lib/srdb2/db_drv.h"
#include "../../lib/srdb2/db_cmd.h"
#include "../../lib/srdb2/db_res.h"
#include "../../str.h"

#include "bdb_con.h"

/** Extension structure of db_cmd adding BDB specific data.
 * This data structure extends the generic data structure db_cmd in the
 * database API with data specific to the ldap driver.
 */
typedef struct _bdb_cmd {
	db_drv_t gen;    /**< Generic part of the data structure (must be first */
	bdb_con_t *bcon; /**< DB connection handle */
	DB *dbp;         /**< DB structure handle */
	DBC *dbcp;       /**< DB cursor handle */
	int next_flag;
	str skey;
	int skey_size;
} bdb_cmd_t, *bdb_cmd_p;


/** Creates a new bdb_cmd data structure.
 * This function allocates and initializes memory for a new bdb_cmd data
 * structure. The data structure is then attached to the generic db_cmd
 * structure in cmd parameter.
 * @param cmd A generic db_cmd structure to which the newly created bdb_cmd
 *            structure will be attached.
 */
int bdb_cmd(db_cmd_t* cmd);


/** The main execution function in BDB SER driver.
 * This is the main execution function in this driver. It is executed whenever
 * a SER module calls db_exec and the target database of the commands is
 * ldap.
 * @param res A pointer to (optional) result structure if the command returns
 *            a result.
 * @retval 0 if executed successfully
 * @retval A negative number if the database server failed to execute command
 * @retval A positive number if there was an error on client side (SER)
 */
int bdb_cmd_exec(db_res_t* res, db_cmd_t* cmd);


int bdb_cmd_first(db_res_t* res);


int bdb_cmd_next(db_res_t* res);

/** @} */

#endif /* _BDB_CMD_H_ */
