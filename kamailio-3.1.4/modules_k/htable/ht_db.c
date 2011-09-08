/**
 * $Id$
 *
 * Copyright (C) 2008 Elena-Ramona Modroiu (asipto.com)
 *
 * This file is part of kamailio, a free SIP server.
 *
 * openser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * openser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "../../dprint.h"
#include "../../usr_avp.h"
#include "../../ut.h"
#include "../../lib/srdb1/db.h"

#include "ht_db.h"

/** database connection */
db1_con_t *ht_db_con = NULL;
db_func_t ht_dbf;

str ht_array_size_suffix = str_init("::size");

/** db parameters */
str ht_db_url   = {0, 0};
str ht_db_name_column   = str_init("key_name");
str ht_db_ktype_column  = str_init("key_type");
str ht_db_vtype_column  = str_init("value_type");
str ht_db_value_column  = str_init("key_value");
int ht_fetch_rows = 100;

int ht_db_init_params(void)
{
	if(ht_db_url.s==0)
		return 0;

	if(ht_fetch_rows<=0)
		ht_fetch_rows = 100;
	if(ht_array_size_suffix.s==NULL || ht_array_size_suffix.s[0]=='\0')
		ht_array_size_suffix.s = "::size";
	ht_array_size_suffix.len = strlen(ht_array_size_suffix.s);

	ht_db_url.len   = strlen(ht_db_url.s);
	ht_db_name_column.len   = strlen(ht_db_name_column.s);
	ht_db_ktype_column.len  = strlen(ht_db_ktype_column.s);
	ht_db_vtype_column.len  = strlen(ht_db_vtype_column.s);
	ht_db_value_column.len  = strlen(ht_db_value_column.s);
	return 0;
}

int ht_db_init_con(void)
{
	/* binding to DB module */
	if(db_bind_mod(&ht_db_url, &ht_dbf))
	{
		LM_ERR("database module not found\n");
		return -1;
	}

	if (!DB_CAPABILITY(ht_dbf, DB_CAP_ALL))
	{
		LM_ERR("database module does not "
		    "implement all functions needed by the module\n");
		return -1;
	}
	return 0;
}
int ht_db_open_con(void)
{
	/* open a connection with the database */
	ht_db_con = ht_dbf.init(&ht_db_url);
	if(ht_db_con==NULL)
	{
		LM_ERR("failed to connect to the database\n");        
		return -1;
	}
	
	LM_DBG("database connection opened successfully\n");
	return 0;
}

int ht_db_close_con(void)
{
	if (ht_db_con!=NULL && ht_dbf.close!=NULL)
		ht_dbf.close(ht_db_con);
	ht_db_con=NULL;
	return 0;
}

#define HT_NAME_BUF_SIZE	256
static char ht_name_buf[HT_NAME_BUF_SIZE];

int ht_db_load_table(ht_t *ht, str *dbtable, int mode)
{
	db_key_t db_cols[4] = {&ht_db_name_column, &ht_db_ktype_column,
		&ht_db_vtype_column, &ht_db_value_column};
	db_key_t db_ord = &ht_db_name_column;
	db1_res_t* db_res = NULL;
	str kname;
	str pname;
	str hname;
	str kvalue;
	int ktype;
	int vtype;
	int last_ktype;
	int n;
	int_str val;
	int i;
	int ret;
	int cnt;

	if(ht_db_con==NULL)
	{
		LM_ERR("no db connection\n");
		return -1;
	}

	if (ht_dbf.use_table(ht_db_con, dbtable) < 0)
	{
		LM_ERR("failed to use_table\n");
		return -1;
	}

	LM_DBG("=============== loading hash table [%.*s] from database [%.*s]\n",
			ht->name.len, ht->name.s, dbtable->len, dbtable->s);
	cnt = 0;

	if (DB_CAPABILITY(ht_dbf, DB_CAP_FETCH)) {
		if(ht_dbf.query(ht_db_con,0,0,0,db_cols,0,4,db_ord,0) < 0)
		{
			LM_ERR("Error while querying db\n");
			return -1;
		}
		if(ht_dbf.fetch_result(ht_db_con, &db_res, ht_fetch_rows)<0)
		{
			LM_ERR("Error while fetching result\n");
			if (db_res)
				ht_dbf.free_result(ht_db_con, db_res);
			goto error;
		} else {
			if(RES_ROW_N(db_res)==0)
			{
				LM_DBG("Nothing to be loaded in hash table\n");
				return 0;
			}
		}
	} else {
		if((ret=ht_dbf.query(ht_db_con, NULL, NULL, NULL, db_cols,
				0, 3, db_ord, &db_res))!=0
			|| RES_ROW_N(db_res)<=0 )
		{
			if( ret==0)
			{
				ht_dbf.free_result(ht_db_con, db_res);
				return 0;
			} else {
				goto error;
			}
		}
	}

	pname.len = 0;
	pname.s = "";
	n = 0;
	last_ktype = 0;
	do {
		for(i=0; i<RES_ROW_N(db_res); i++)
		{
			cnt++;
			/* not NULL values enforced in table definition ?!?! */
			kname.s = (char*)(RES_ROWS(db_res)[i].values[0].val.string_val);
			if(kname.s==NULL) {
				LM_ERR("null key in row %d\n", i);
				goto error;
			}
			kname.len = strlen(kname.s);
			ktype = RES_ROWS(db_res)[i].values[1].val.int_val;
			if(last_ktype==1)
			{
				if(pname.len>0
						&& (pname.len!=kname.len
							|| strncmp(pname.s, kname.s, pname.len)!=0))
				{
					/* new key name, last was an array => add its size */
					snprintf(ht_name_buf, HT_NAME_BUF_SIZE, "%.*s%.*s",
						pname.len, pname.s, ht_array_size_suffix.len,
						ht_array_size_suffix.s);
					hname.s = ht_name_buf;
					hname.len = strlen(ht_name_buf);
					val.n = n;

					if(ht_set_cell(ht, &hname, 0, &val, mode))
					{
						LM_ERR("error adding array size to hash table.\n");
						goto error;
					}
					pname.len = 0;
					pname.s = "";
					n = 0;
				}
			}
			last_ktype = ktype;
			pname = kname;
			if(ktype==1)
			{
				snprintf(ht_name_buf, HT_NAME_BUF_SIZE, "%.*s[%d]",
						kname.len, kname.s, n);
				hname.s = ht_name_buf;
				hname.len = strlen(ht_name_buf);
				n++;
			} else {
				hname = kname;
			}
			vtype = RES_ROWS(db_res)[i].values[2].val.int_val;
			kvalue.s = (char*)(RES_ROWS(db_res)[i].values[3].val.string_val);
			if(kvalue.s==NULL) {
				LM_ERR("null value in row %d\n", i);
				goto error;
			}
			kvalue.len = strlen(kvalue.s);

			/* add to hash */
			if(vtype==1)
				str2sint(&kvalue, &val.n);
			else
				val.s = kvalue;
				
			if(ht_set_cell(ht, &hname, (vtype)?0:AVP_VAL_STR, &val, mode))
			{
				LM_ERR("error adding to hash table\n");
				goto error;
			}
	 	}
		if (DB_CAPABILITY(ht_dbf, DB_CAP_FETCH)) {
			if(ht_dbf.fetch_result(ht_db_con, &db_res, ht_fetch_rows)<0) {
				LM_ERR("Error while fetching!\n");
				goto error;
			}
		} else {
			break;
		}
	}  while(RES_ROW_N(db_res)>0);

	if(last_ktype==1)
	{
		snprintf(ht_name_buf, HT_NAME_BUF_SIZE, "%.*s%.*s",
			pname.len, pname.s, ht_array_size_suffix.len,
			ht_array_size_suffix.s);
		hname.s = ht_name_buf;
		hname.len = strlen(ht_name_buf);
		val.n = n;
		if(ht_set_cell(ht, &hname, 0, &val, mode))
		{
			LM_ERR("error adding array size to hash table.\n");
			goto error;
		}
	}

	ht_dbf.free_result(ht_db_con, db_res);
	LM_DBG("loaded %d values in hash table\n", cnt);

	return 0;
error:
	ht_dbf.free_result(ht_db_con, db_res);
	return -1;

}

