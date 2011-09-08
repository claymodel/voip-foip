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
		       
#ifndef _HT_DB_H_
#define _HT_DB_H_

#include "ht_api.h"

extern str ht_db_url;
extern str ht_db_name_column;
extern str ht_db_ktype_column;
extern str ht_db_vtype_column;
extern str ht_db_value_column;
extern str ht_array_size_suffix;
extern int ht_fetch_rows;

int ht_db_init_params(void);
int ht_db_init_con(void);
int ht_db_open_con(void);
int ht_db_close_con(void);
int ht_db_load_table(ht_t *ht, str *dbtable, int mode);

#endif
