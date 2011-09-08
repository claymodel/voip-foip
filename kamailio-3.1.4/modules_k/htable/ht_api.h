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
		       
#ifndef _HT_API_H_
#define _HT_API_H_

#include <time.h>

#include "../../usr_avp.h"
#include "../../locking.h"
#include "../../pvar.h"

typedef struct _ht_cell
{
    unsigned int cellid;
    unsigned int msize;
	int flags;
	str name;
	int_str value;
	time_t  expire;
    struct _ht_cell *prev;
    struct _ht_cell *next;
} ht_cell_t;

typedef struct _ht_entry
{
	unsigned int esize;
	ht_cell_t *first;
	gen_lock_t lock;	
} ht_entry_t;

typedef struct _ht
{
	str name;
	unsigned int htid;
	unsigned int htexpire;
	str dbtable;
	unsigned int htsize;
	ht_entry_t *entries;
	struct _ht *next;
} ht_t;

typedef struct _ht_pv {
	str htname;
	ht_t *ht;
	pv_elem_t *pve;
} ht_pv_t, *ht_pv_p;

int ht_pkg_init(str *name, int autoexp, str *dbtable, int size);
int ht_shm_init(void);
int ht_destroy(void);
int ht_set_cell(ht_t *ht, str *name, int type, int_str *val, int mode);
int ht_del_cell(ht_t *ht, str *name);

int ht_dbg(void);
ht_cell_t* ht_cell_pkg_copy(ht_t *ht, str *name, ht_cell_t *old);
int ht_cell_pkg_free(ht_cell_t *cell);
int ht_cell_free(ht_cell_t *cell);

int ht_table_spec(char *spec);
ht_t* ht_get_table(str *name);
int ht_db_load_tables(void);

int ht_has_autoexpire(void);
void ht_timer(unsigned int ticks, void *param);
int ht_set_cell_expire(ht_t *ht, str *name, int type, int_str *val);
int ht_get_cell_expire(ht_t *ht, str *name, unsigned int *val);

int ht_rm_cell_re(str *sre, ht_t *ht, int mode);
int ht_count_cells_re(str *sre, ht_t *ht, int mode);

#endif
