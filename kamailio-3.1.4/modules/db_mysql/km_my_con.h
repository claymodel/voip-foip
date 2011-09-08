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
 *  \brief DB_MYSQL :: Core
 *  \ref my_con.c
 *  \ingroup db_mysql
 *  Module: \ref db_mysql
 */


#ifndef KM_MY_CON_H
#define KM_MY_CON_H

#include "../../lib/srdb1/db_pool.h"
#include "../../lib/srdb1/db_id.h"

#include <time.h>
#include <mysql/mysql.h>


struct my_con {
	struct db_id* id;        /*!< Connection identifier */
	unsigned int ref;        /*!< Reference count */
	struct pool_con* next;   /*!< Next connection in the pool */

	MYSQL_RES* res;          /*!< Actual result */
	MYSQL* con;              /*!< Connection representation */
	MYSQL_ROW row;           /*!< Actual row in the result */
	time_t timestamp;        /*!< Timestamp of last query */
};


/*
 * Some convenience wrappers
 */
#define CON_RESULT(db_con)     (((struct my_con*)((db_con)->tail))->res)
#define CON_CONNECTION(db_con) (((struct my_con*)((db_con)->tail))->con)
#define CON_ROW(db_con)        (((struct my_con*)((db_con)->tail))->row)
#define CON_TIMESTAMP(db_con)  (((struct my_con*)((db_con)->tail))->timestamp)


/*! \brief
 * Create a new connection structure,
 * open the MySQL connection and set reference count to 1
 */
struct my_con* db_mysql_new_connection(const struct db_id* id);


/*! \brief
 * Close the connection and release memory
 */
void db_mysql_free_connection(struct pool_con* con);

#endif /* KM_MY_CON_H */
