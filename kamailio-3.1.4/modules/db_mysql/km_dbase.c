/* 
 * $Id$ 
 *
 * MySQL module core functions
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2007-2008 1&1 Internet AG
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

/*!
 * \file
 * \brief Implementation of core functions for the MySQL driver.
 *
 * This file contains the implementation of core functions for the MySQL
 * database driver, for example to submit a query or fetch a result.
 * \ingroup db_mysql
 *  Module: \ref db_mysql
 */

#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysql_version.h>
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../../lib/srdb1/db_query.h"
#include "../../lib/srdb1/db_ut.h"
#include "mysql_mod.h"
#include "km_val.h"
#include "km_my_con.h"
#include "km_res.h"
#include "km_row.h"
#include "km_db_mysql.h"
#include "km_dbase.h"


/**
 * \brief Send a SQL query to the server.
 *
 * Send a SQL query to the database server. This methods tries to reconnect
 * to the server if the connection is gone and the auto_reconnect parameter is
 * enabled. It also issues a mysql_ping before the query to connect again after
 * a long waiting period because for some older mysql versions the auto reconnect
 * don't work sufficient. If auto_reconnect is enabled and the server supports it,
 * then the mysql_ping is probably not necessary, but its safer to do it in this
 * cases too.
 *
 * \param _h handle for the db
 * \param _s executed query
 * \return zero on success, negative value on failure
 */
static int db_mysql_submit_query(const db1_con_t* _h, const str* _s)
{	
	time_t t;
	int i, code;

	if (!_h || !_s || !_s->s) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}

	if (my_ping_interval) {
		t = time(0);
		if ((t - CON_TIMESTAMP(_h)) > my_ping_interval) {
			if (mysql_ping(CON_CONNECTION(_h))) {
				LM_WARN("driver error on ping: %s\n", mysql_error(CON_CONNECTION(_h)));
				counter_inc(mysql_cnts_h.driver_err);
			}
		}
		/*
		 * We're doing later a query anyway that will reset the timout of the server,
		 * so it makes sense to set the timestamp value to the actual time in order
		 * to prevent unnecessary pings.
		 */
		CON_TIMESTAMP(_h) = t;
	}

	/* screws up the terminal when the query contains a BLOB :-( (by bogdan)
	 * LM_DBG("submit_query(): %.*s\n", _s->len, _s->s);
	 */

	/* When a server connection is lost and a query is attempted, most of
	 * the time the query will return a CR_SERVER_LOST, then at the second
	 * attempt to execute it, the mysql lib will reconnect and succeed.
	 * However is a few cases, the first attempt returns CR_SERVER_GONE_ERROR
	 * the second CR_SERVER_LOST and only the third succeeds.
	 * Thus the 3 in the loop count. Increasing the loop count over this
	 * value shouldn't be needed, but it doesn't hurt either, since the loop
	 * will most of the time stop at the second or sometimes at the third
	 * iteration. In the case of CR_SERVER_GONE_ERROR and CR_SERVER_LOST the
	 * driver error counter is increased
	 */
	for (i=0; i < (db_mysql_auto_reconnect ? 3 : 1); i++) {
		if (mysql_real_query(CON_CONNECTION(_h), _s->s, _s->len) == 0) {
			return 0;
		}
		code = mysql_errno(CON_CONNECTION(_h));
		if (code != CR_SERVER_GONE_ERROR && code != CR_SERVER_LOST) {
			break;
		}
		counter_inc(mysql_cnts_h.driver_err);
	}
	LM_ERR("driver error on query: %s\n", mysql_error(CON_CONNECTION(_h)));
	return -2;
}



/**
 * Initialize the database module.
 * No function should be called before this
 * \param _url URL used for initialization
 * \return zero on success, negative value on failure
 */
db1_con_t* db_mysql_init(const str* _url)
{
	return db_do_init(_url, (void *)db_mysql_new_connection);
}


/**
 * Shut down the database module.
 * No function should be called after this
 * \param _h handle to the closed connection
 * \return zero on success, negative value on failure
 */
void db_mysql_close(db1_con_t* _h)
{
	db_do_close(_h, db_mysql_free_connection);
}


/**
 * Retrieve a result set
 * \param _h handle to the database
 * \param _r result set that should be retrieved
 * \return zero on success, negative value on failure
 */
static int db_mysql_store_result(const db1_con_t* _h, db1_res_t** _r)
{
	int code;
	if ((!_h) || (!_r)) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}

	*_r = db_new_result();
	if (*_r == 0) {
		LM_ERR("no memory left\n");
		return -2;
	}

	CON_RESULT(_h) = mysql_store_result(CON_CONNECTION(_h));
	if (!CON_RESULT(_h)) {
		if (mysql_field_count(CON_CONNECTION(_h)) == 0) {
			(*_r)->col.n = 0;
			(*_r)->n = 0;
			goto done;
		} else {
			LM_ERR("driver error: %s\n", mysql_error(CON_CONNECTION(_h)));
			code = mysql_errno(CON_CONNECTION(_h));
			if (code == CR_SERVER_GONE_ERROR || code == CR_SERVER_LOST) {
				counter_inc(mysql_cnts_h.driver_err);
			}
			db_free_result(*_r);
			*_r = 0;
			return -3;
		}
	}

	if (db_mysql_convert_result(_h, *_r) < 0) {
		LM_ERR("error while converting result\n");
		LM_DBG("freeing result set at %p\n", _r);
		pkg_free(*_r);
		*_r = 0;
		/* all mem on openser API side is already freed by
		 * db_mysql_convert_result in case of error, but we also need
		 * to free the mem from the mysql lib side */
		mysql_free_result(CON_RESULT(_h));
#if (MYSQL_VERSION_ID >= 40100)
		while( mysql_next_result( CON_CONNECTION(_h) ) > 0 ) {
			MYSQL_RES *res = mysql_store_result( CON_CONNECTION(_h) );
			mysql_free_result(res);
		}
#endif
		CON_RESULT(_h) = 0;
		return -4;
	}

done:
#if (MYSQL_VERSION_ID >= 40100)
	while( mysql_next_result( CON_CONNECTION(_h) ) > 0 ) {
		MYSQL_RES *res = mysql_store_result( CON_CONNECTION(_h) );
		mysql_free_result(res);
	}
#endif

	return 0;
}


/**
 * Release a result set from memory.
 * \param _h handle to the database
 * \param _r result set that should be freed
 * \return zero on success, negative value on failure
 */
int db_mysql_free_result(db1_con_t* _h, db1_res_t* _r)
{
     if ((!_h) || (!_r)) {
	     LM_ERR("invalid parameter value\n");
	     return -1;
     }

     if (db_free_result(_r) < 0) {
	     LM_ERR("unable to free result structure\n");
	     return -1;
     }
     mysql_free_result(CON_RESULT(_h));
     CON_RESULT(_h) = 0;
     return 0;
}


/**
 * Query a table for specified rows.
 * \param _h structure representing database connection
 * \param _k key names
 * \param _op operators
 *\param  _v values of the keys that must match
 * \param _c column names to return
 * \param _n number of key=values pairs to compare
 * \param _nc number of columns to return
 * \param _o order by the specified column
 * \param _r pointer to a structure representing the result
 * \return zero on success, negative value on failure
 */
int db_mysql_query(const db1_con_t* _h, const db_key_t* _k, const db_op_t* _op,
	     const db_val_t* _v, const db_key_t* _c, const int _n, const int _nc,
	     const db_key_t _o, db1_res_t** _r)
{
	return db_do_query(_h, _k, _op, _v, _c, _n, _nc, _o, _r,
	db_mysql_val2str, db_mysql_submit_query, db_mysql_store_result);
}

/**
 * \brief Gets a partial result set, fetch rows from a result
 *
 * Gets a partial result set, fetch a number of rows from a database result.
 * This function initialize the given result structure on the first run, and
 * fetches the nrows number of rows. On subsequenting runs, it uses the
 * existing result and fetches more rows, until it reaches the end of the
 * result set. Because of this the result needs to be null in the first
 * invocation of the function. If the number of wanted rows is zero, the
 * function returns anything with a result of zero.
 * \param _h structure representing the database connection
 * \param _r pointer to a structure representing the result
 * \param nrows number of fetched rows
 * \return zero on success, negative value on failure
 */
int db_mysql_fetch_result(const db1_con_t* _h, db1_res_t** _r, const int nrows)
{
	int rows, i, code;

	if (!_h || !_r || nrows < 0) {
		LM_ERR("Invalid parameter value\n");
		return -1;
	}

	/* exit if the fetch count is zero */
	if (nrows == 0) {
		db_free_result(*_r);
		*_r = 0;
		return 0;
	}

	if(*_r==0) {
		/* Allocate a new result structure */
		*_r = db_new_result();
		if (*_r == 0) {
			LM_ERR("no memory left\n");
			return -2;
		}

		CON_RESULT(_h) = mysql_store_result(CON_CONNECTION(_h));
		if (!CON_RESULT(_h)) {
			if (mysql_field_count(CON_CONNECTION(_h)) == 0) {
				(*_r)->col.n = 0;
				(*_r)->n = 0;
				return 0;
			} else {
				LM_ERR("driver error: %s\n", mysql_error(CON_CONNECTION(_h)));
				code = mysql_errno(CON_CONNECTION(_h));
				if (code == CR_SERVER_GONE_ERROR || code == CR_SERVER_LOST) {
					counter_inc(mysql_cnts_h.driver_err);
				}
				db_free_result(*_r);
				*_r = 0;
				return -3;
			}
		}
		if (db_mysql_get_columns(_h, *_r) < 0) {
			LM_ERR("error while getting column names\n");
			return -4;
		}

		RES_NUM_ROWS(*_r) = mysql_num_rows(CON_RESULT(_h));
		if (!RES_NUM_ROWS(*_r)) {
			LM_DBG("no rows returned from the query\n");
			RES_ROWS(*_r) = 0;
			return 0;
		}

	} else {
		/* free old rows */
		if(RES_ROWS(*_r)!=0)
			db_free_rows(*_r);
		RES_ROWS(*_r) = 0;
		RES_ROW_N(*_r) = 0;
	}

	/* determine the number of rows remaining to be processed */
	rows = RES_NUM_ROWS(*_r) - RES_LAST_ROW(*_r);

	/* If there aren't any more rows left to process, exit */
	if(rows<=0)
		return 0;

	/* if the fetch count is less than the remaining rows to process                 */
	/* set the number of rows to process (during this call) equal to the fetch count */
	if(nrows < rows)
		rows = nrows;

	RES_ROW_N(*_r) = rows;

	LM_DBG("converting row %d of %d count %d\n", RES_LAST_ROW(*_r),
			RES_NUM_ROWS(*_r), RES_ROW_N(*_r));

	RES_ROWS(*_r) = (struct db_row*)pkg_malloc(sizeof(db_row_t) * rows);
	if (!RES_ROWS(*_r)) {
		LM_ERR("no memory left\n");
		return -5;
	}

	for(i = 0; i < rows; i++) {
		CON_ROW(_h) = mysql_fetch_row(CON_RESULT(_h));
		if (!CON_ROW(_h)) {
			LM_ERR("driver error: %s\n", mysql_error(CON_CONNECTION(_h)));
			RES_ROW_N(*_r) = i;
			db_free_rows(*_r);
			return -6;
		}
		if (db_mysql_convert_row(_h, *_r, &(RES_ROWS(*_r)[i])) < 0) {
			LM_ERR("error while converting row #%d\n", i);
			RES_ROW_N(*_r) = i;
			db_free_rows(*_r);
			return -7;
		}
	}

	/* update the total number of rows processed */
	RES_LAST_ROW(*_r) += rows;
	return 0;
}

/**
 * Execute a raw SQL query.
 * \param _h handle for the database
 * \param _s raw query string
 * \param _r result set for storage
 * \return zero on success, negative value on failure
 */
int db_mysql_raw_query(const db1_con_t* _h, const str* _s, db1_res_t** _r)
{
	return db_do_raw_query(_h, _s, _r, db_mysql_submit_query,
	db_mysql_store_result);
}


/**
 * Insert a row into a specified table.
 * \param _h structure representing database connection
 * \param _k key names
 * \param _v values of the keys
 * \param _n number of key=value pairs
 * \return zero on success, negative value on failure
 */
int db_mysql_insert(const db1_con_t* _h, const db_key_t* _k, const db_val_t* _v, const int _n)
{
	return db_do_insert(_h, _k, _v, _n, db_mysql_val2str,
	db_mysql_submit_query);
}


/**
 * Delete a row from the specified table
 * \param _h structure representing database connection
 * \param _k key names
 * \param _o operators
 * \param _v values of the keys that must match
 * \param _n number of key=value pairs
 * \return zero on success, negative value on failure
 */
int db_mysql_delete(const db1_con_t* _h, const db_key_t* _k, const db_op_t* _o,
	const db_val_t* _v, const int _n)
{
	return db_do_delete(_h, _k, _o, _v, _n, db_mysql_val2str,
	db_mysql_submit_query);
}


/**
 * Update some rows in the specified table
 * \param _h structure representing database connection
 * \param _k key names
 * \param _o operators
 * \param _v values of the keys that must match
 * \param _uk updated columns
 * \param _uv updated values of the columns
 * \param _n number of key=value pairs
 * \param _un number of columns to update
 * \return zero on success, negative value on failure
 */
int db_mysql_update(const db1_con_t* _h, const db_key_t* _k, const db_op_t* _o, 
	const db_val_t* _v, const db_key_t* _uk, const db_val_t* _uv, const int _n, 
	const int _un)
{
	return db_do_update(_h, _k, _o, _v, _uk, _uv, _n, _un, db_mysql_val2str,
	db_mysql_submit_query);
}


/**
 * Just like insert, but replace the row if it exists.
 * \param _h database handle
 * \param _k key names
 * \param _v values of the keys that must match
 * \param _n number of key=value pairs
 * \return zero on success, negative value on failure
 */
int db_mysql_replace(const db1_con_t* _h, const db_key_t* _k, const db_val_t* _v, const int _n)
{
	return db_do_replace(_h, _k, _v, _n, db_mysql_val2str,
	db_mysql_submit_query);
}


/**
 * Returns the last inserted ID.
 * \param _h database handle
 * \return returns the ID as integer or returns 0 if the previous statement
 * does not use an AUTO_INCREMENT value.
 */
int db_last_inserted_id(const db1_con_t* _h)
{
	if (!_h) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}
	return mysql_insert_id(CON_CONNECTION(_h));
}


 /**
  * Insert a row into a specified table, update on duplicate key.
  * \param _h structure representing database connection
  * \param _k key names
  * \param _v values of the keys
  * \param _n number of key=value pairs
 */
 int db_insert_update(const db1_con_t* _h, const db_key_t* _k, const db_val_t* _v,
	const int _n)
 {
	int off, ret;
	static str  sql_str;
	static char sql_buf[SQL_BUF_LEN];
 
	if ((!_h) || (!_k) || (!_v) || (!_n)) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}
 
	ret = snprintf(sql_buf, SQL_BUF_LEN, "insert into %.*s (", CON_TABLE(_h)->len, CON_TABLE(_h)->s);
	if (ret < 0 || ret >= SQL_BUF_LEN) goto error;
	off = ret;

	ret = db_print_columns(sql_buf + off, SQL_BUF_LEN - off, _k, _n);
	if (ret < 0) return -1;
	off += ret;

	ret = snprintf(sql_buf + off, SQL_BUF_LEN - off, ") values (");
	if (ret < 0 || ret >= (SQL_BUF_LEN - off)) goto error;
	off += ret;
	ret = db_print_values(_h, sql_buf + off, SQL_BUF_LEN - off, _v, _n, db_mysql_val2str);
	if (ret < 0) return -1;
	off += ret;

	*(sql_buf + off++) = ')';
	
	ret = snprintf(sql_buf + off, SQL_BUF_LEN - off, " on duplicate key update ");
	if (ret < 0 || ret >= (SQL_BUF_LEN - off)) goto error;
	off += ret;
	
	ret = db_print_set(_h, sql_buf + off, SQL_BUF_LEN - off, _k, _v, _n, db_mysql_val2str);
	if (ret < 0) return -1;
	off += ret;
	
	sql_str.s = sql_buf;
	sql_str.len = off;
 
	if (db_mysql_submit_query(_h, &sql_str) < 0) {
		LM_ERR("error while submitting query\n");
		return -2;
	}
	return 0;

error:
	LM_ERR("error while preparing insert_update operation\n");
	return -1;
}


/**
 * Store the name of table that will be used by subsequent database functions
 * \param _h database handle
 * \param _t table name
 * \return zero on success, negative value on failure
 */
int db_mysql_use_table(db1_con_t* _h, const str* _t)
{
	return db_use_table(_h, _t);
}
