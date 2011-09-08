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

#include <regex.h>

#include "../../mem/shm_mem.h"
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "../../lib/kcore/hash_func.h"
#include "../../ut.h"
#include "../../re.h"

#include "ht_api.h"
#include "ht_db.h"


#define ht_compute_hash(_s)        core_case_hash(_s,0,0)
#define ht_get_entry(_h,_size)    (_h)&((_size)-1)


ht_t *_ht_root = NULL;
ht_t *_ht_pkg_root = NULL;


ht_cell_t* ht_cell_new(str *name, int type, int_str *val, unsigned int cellid)
{
	ht_cell_t *cell;
	unsigned int msize;

	msize = sizeof(ht_cell_t) + (name->len + 1)*sizeof(char);

	if(type&AVP_VAL_STR)
		msize += (val->s.len + 1)*sizeof(char);

	cell = (ht_cell_t*)shm_malloc(msize);
	if(cell==NULL)
	{
		LM_ERR("no more shm\n");
		return NULL;
	}

	memset(cell, 0, msize);
	cell->msize = msize;
	cell->cellid = cellid;
	cell->flags = type;
	cell->name.len = name->len;
	cell->name.s = (char*)cell + sizeof(ht_cell_t);
	memcpy(cell->name.s, name->s, name->len);
	cell->name.s[name->len] = '\0';
	if(type&AVP_VAL_STR)
	{
		cell->value.s.s = (char*)cell->name.s + name->len + 1;
		cell->value.s.len = val->s.len;
		memcpy(cell->value.s.s, val->s.s, val->s.len);
		cell->value.s.s[val->s.len] = '\0';
	} else {
		cell->value.n = val->n;
	}
	return cell;
}

int ht_cell_free(ht_cell_t *cell)
{
	if(cell==NULL)
		return -1;
	shm_free(cell);
	return 0;
}


int ht_cell_pkg_free(ht_cell_t *cell)
{
	if(cell==NULL)
		return -1;
	pkg_free(cell);
	return 0;
}

ht_t* ht_get_table(str *name)
{
	unsigned int htid;
	ht_t *ht;

	htid = ht_compute_hash(name);

	/* does it exist */
	ht = _ht_root;
	while(ht!=NULL)
	{
		if(htid == ht->htid && name->len==ht->name.len 
				&& strncmp(name->s, ht->name.s, name->len)==0)
		{
			LM_DBG("htable found [%.*s]\n", name->len, name->s);
			return ht;
		}
		ht = ht->next;
	}
	return NULL;
}

int ht_pkg_init(str *name, int autoexp, str *dbtable, int size)
{
	unsigned int htid;
	ht_t *ht;

	htid = ht_compute_hash(name);

	/* does it exist */
	ht = _ht_pkg_root;
	while(ht!=NULL)
	{
		if(htid == ht->htid && name->len==ht->name.len 
				&& strncmp(name->s, ht->name.s, name->len)==0)
		{
			LM_ERR("htable already configured [%.*s]\n", name->len, name->s);
			return -1;
		}
		ht = ht->next;
	}

	ht = (ht_t*)pkg_malloc(sizeof(ht_t));
	if(ht==NULL)
	{
		LM_ERR("no more pkg\n");
		return -1;
	}
	memset(ht, 0, sizeof(ht_t));

	if(size<=1)
		ht->htsize = 8;
	else if(size>14)
		ht->htsize = 1<<14;
	else ht->htsize = 1<<size;
	ht->htid = htid;
	ht->htexpire = autoexp;
	ht->name = *name;
	if(dbtable!=NULL && dbtable->len>0)
		ht->dbtable = *dbtable;

	ht->next = _ht_pkg_root;
	_ht_pkg_root = ht;
	return 0;
}

int ht_shm_init(void)
{
	ht_t *htp;
	ht_t *htp0;
	ht_t *ht;
	int i;

	htp = _ht_pkg_root;

	while(htp)
	{
		htp0 = htp->next;
		ht = (ht_t*)shm_malloc(sizeof(ht_t));
		if(ht==NULL)
		{
			LM_ERR("no more shm\n");
			return -1;
		}
		memcpy(ht, htp, sizeof(ht_t));

		ht->entries = (ht_entry_t*)shm_malloc(ht->htsize*sizeof(ht_entry_t));
		if(ht->entries==NULL)
		{
			LM_ERR("no more shm.\n");
			shm_free(ht);
			return -1;
		}
		memset(ht->entries, 0, ht->htsize*sizeof(ht_entry_t));

		for(i=0; i<ht->htsize; i++)
		{
			if(lock_init(&ht->entries[i].lock)==0)
			{
				LM_ERR("cannot initalize lock[%d]\n", i);
				i--;
				while(i>=0)
				{
					lock_destroy(&ht->entries[i].lock);
					i--;
				}
				shm_free(ht->entries);
				shm_free(ht);
				return -1;

			}
		}
		ht->next = _ht_root;
		_ht_root = ht;
		pkg_free(htp);
		htp = htp0;
	}
	_ht_pkg_root = NULL;

	return 0;
}

int ht_destroy(void)
{
	int i;
	ht_cell_t *it, *it0;
	ht_t *ht;
	ht_t *ht0;

	if(_ht_root==NULL)
		return -1;

	ht = _ht_root;
	while(ht)
	{
		ht0 = ht->next;
		for(i=0; i<ht->htsize; i++)
		{
			/* free entries */
			it = ht->entries[i].first;
			while(it)
			{
				it0 = it;
				it = it->next;
				ht_cell_free(it0);
			}
			/* free locks */
			lock_destroy(&ht->entries[i].lock);
		}
		shm_free(ht->entries);
		shm_free(ht);
		ht = ht0;
	}
	_ht_root = NULL;
	return 0;
}


int ht_set_cell(ht_t *ht, str *name, int type, int_str *val, int mode)
{
	unsigned int idx;
	unsigned int hid;
	ht_cell_t *it, *prev, *cell;
	time_t now;

	if(ht==NULL || ht->entries==NULL)
		return -1;

	hid = ht_compute_hash(name);
	
	idx = ht_get_entry(hid, ht->htsize);

	now = 0;
	if(ht->htexpire>0)
		now = time(NULL);
	prev = NULL;
	if(mode) lock_get(&ht->entries[idx].lock);
	it = ht->entries[idx].first;
	while(it!=NULL && it->cellid < hid)
	{
		prev = it;
		it = it->next;
	}
	while(it!=NULL && it->cellid == hid)
	{
		if(name->len==it->name.len 
				&& strncmp(name->s, it->name.s, name->len)==0)
		{
			/* update value */
			if(it->flags&AVP_VAL_STR)
			{
				if(type&AVP_VAL_STR)
				{
					if(it->value.s.len >= val->s.len)
					{
						/* copy */
						it->value.s.len = val->s.len;
						memcpy(it->value.s.s, val->s.s, val->s.len);
						it->value.s.s[it->value.s.len] = '\0';
						it->expire = now + ht->htexpire;
					} else {
						/* new */
						cell = ht_cell_new(name, type, val, hid);
						if(cell == NULL)
						{
							LM_ERR("cannot create new cell\n");
							if(mode) lock_release(&ht->entries[idx].lock);
							return -1;
						}
						cell->next = it->next;
						cell->prev = it->prev;
						cell->expire = now + ht->htexpire;
						if(it->prev)
							it->prev->next = cell;
						else
							ht->entries[idx].first = cell;
						if(it->next)
							it->next->prev = cell;
						ht_cell_free(it);
					}
				} else {
					it->flags &= ~AVP_VAL_STR;
					it->value.n = val->n;
					it->expire = now + ht->htexpire;
				}
				if(mode) lock_release(&ht->entries[idx].lock);
				return 0;
			} else {
				if(type&AVP_VAL_STR)
				{
					/* new */
					cell = ht_cell_new(name, type, val, hid);
					if(cell == NULL)
					{
						LM_ERR("cannot create new cell.\n");
						if(mode) lock_release(&ht->entries[idx].lock);
						return -1;
					}
					cell->expire = now + ht->htexpire;
					cell->next = it->next;
					cell->prev = it->prev;
					if(it->prev)
						it->prev->next = cell;
					else
						ht->entries[idx].first = cell;
					if(it->next)
						it->next->prev = cell;
					ht_cell_free(it);
				} else {
					it->value.n = val->n;
					it->expire = now + ht->htexpire;
				}
				if(mode) lock_release(&ht->entries[idx].lock);
				return 0;
			}
		}
		prev = it;
		it = it->next;
	}
	/* add */
	cell = ht_cell_new(name, type, val, hid);
	if(cell == NULL)
	{
		LM_ERR("cannot create new cell.\n");
		if(mode) lock_release(&ht->entries[idx].lock);
		return -1;
	}
	cell->expire = now + ht->htexpire;
	if(prev==NULL)
	{
		if(ht->entries[idx].first!=NULL)
		{
			cell->next = ht->entries[idx].first;
			ht->entries[idx].first->prev = cell;
		}
		ht->entries[idx].first = cell;
	} else {
		cell->next = prev->next;
		cell->prev = prev;
		if(prev->next)
			prev->next->prev = cell;
		prev->next = cell;
	}
	ht->entries[idx].esize++;
	if(mode) lock_release(&ht->entries[idx].lock);
	return 0;
}

int ht_del_cell(ht_t *ht, str *name)
{
	unsigned int idx;
	unsigned int hid;
	ht_cell_t *it;

	if(ht==NULL || ht->entries==NULL)
		return -1;

	hid = ht_compute_hash(name);
	
	idx = ht_get_entry(hid, ht->htsize);

	/* head test and return */
	if(ht->entries[idx].first==NULL)
		return 0;
	
	lock_get(&ht->entries[idx].lock);
	it = ht->entries[idx].first;
	while(it!=NULL && it->cellid < hid)
		it = it->next;
	while(it!=NULL && it->cellid == hid)
	{
		if(name->len==it->name.len 
				&& strncmp(name->s, it->name.s, name->len)==0)
		{
			/* found */
			if(it->prev==NULL)
				ht->entries[idx].first = it->next;
			else
				it->prev->next = it->next;
			if(it->next)
				it->next->prev = it->prev;
			ht->entries[idx].esize--;
			lock_release(&ht->entries[idx].lock);
			ht_cell_free(it);
			return 0;
		}
		it = it->next;
	}
	lock_release(&ht->entries[idx].lock);
	return 0;
}

ht_cell_t* ht_cell_pkg_copy(ht_t *ht, str *name, ht_cell_t *old)
{
	unsigned int idx;
	unsigned int hid;
	ht_cell_t *it, *cell;

	if(ht==NULL || ht->entries==NULL)
		return NULL;

	hid = ht_compute_hash(name);
	
	idx = ht_get_entry(hid, ht->htsize);

	/* head test and return */
	if(ht->entries[idx].first==NULL)
		return NULL;
	
	lock_get(&ht->entries[idx].lock);
	it = ht->entries[idx].first;
	while(it!=NULL && it->cellid < hid)
		it = it->next;
	while(it!=NULL && it->cellid == hid)
	{
		if(name->len==it->name.len 
				&& strncmp(name->s, it->name.s, name->len)==0)
		{
			/* found */
			if(old!=NULL)
			{
				if(old->msize>=it->msize)
				{
					memcpy(old, it, it->msize);
					lock_release(&ht->entries[idx].lock);
					return old;
				}
			}
			cell = (ht_cell_t*)pkg_malloc(it->msize);
			if(cell!=NULL)
				memcpy(cell, it, it->msize);
			lock_release(&ht->entries[idx].lock);
			return cell;
		}
		it = it->next;
	}
	lock_release(&ht->entries[idx].lock);
	return NULL;
}

int ht_dbg(void)
{
	int i;
	ht_cell_t *it;
	ht_t *ht;

	ht = _ht_root;
	while(ht)
	{
		LM_ERR("===== htable[%.*s] hid: %u exp: %u>\n", ht->name.len,
				ht->name.s, ht->htid, ht->htexpire);
		for(i=0; i<ht->htsize; i++)
		{
			lock_get(&ht->entries[i].lock);
			LM_ERR("htable[%d] -- <%d>\n", i, ht->entries[i].esize);
			it = ht->entries[i].first;
			while(it)
			{
				LM_ERR("\tcell: %.*s\n", it->name.len, it->name.s);
				LM_ERR("\thid: %u msize: %u flags: %d expire: %u\n", it->cellid,
						it->msize, it->flags, (unsigned int)it->expire);
				if(it->flags&AVP_VAL_STR)
					LM_ERR("\tv-s:%.*s\n", it->value.s.len, it->value.s.s);
				else
					LM_ERR("\tv-i:%d\n", it->value.n);
				it = it->next;
			}
			lock_release(&ht->entries[i].lock);
		}
		ht = ht->next;
	}
	return 0;
}

int ht_table_spec(char *spec)
{
	str name;
	str dbtable = {0, 0};
	unsigned int autoexpire = 0;
	unsigned int size = 4;
	int type = 0;
	str in;
	str tok;
	char *p;

	/* parse: name=>dbtable=abc;autoexpire=123;size=123*/
	in.s = spec;
	in.len = strlen(in.s);

	p = in.s;
	while(p<in.s+in.len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p++;
	if(p>in.s+in.len || *p=='\0')
		goto error;
	name.s = p;
	while(p < in.s + in.len)
	{
		if(*p=='=' || *p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
			break;
		p++;
	}
	if(p>in.s+in.len || *p=='\0')
		goto error;
	name.len = p - name.s;
	if(*p!='=')
	{
		while(p<in.s+in.len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
			p++;
		if(p>in.s+in.len || *p=='\0' || *p!='=')
			goto error;
	}
	p++;
	if(*p!='>')
		goto error;
	p++;
	while(p<in.s+in.len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p++;

next_token:
	tok.s = p;
	while(p < in.s + in.len)
	{
		if(*p=='=' || *p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
			break;
		p++;
	}
	if(p>in.s+in.len || *p=='\0')
		goto error;
	tok.len = p - tok.s;
	if(tok.len==7 && strncmp(tok.s, "dbtable", 7)==0)
		type = 1;
	else if(tok.len==10 && strncmp(tok.s, "autoexpire", 10)==0)
		type = 2;
	else if(tok.len==4 && strncmp(tok.s, "size", 4)==0)
		type = 3;
	else goto error;

	if(*p!='=')
	{
		while(p<in.s+in.len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
			p++;
		if(p>in.s+in.len || *p=='\0' || *p!='=')
			goto error;
	}
	p++;
	while(p<in.s+in.len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p++;
	if(p>in.s+in.len || *p=='\0')
		goto error;
	tok.s = p;
	while(p < in.s + in.len)
	{
		if(*p==';' || *p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
			break;
		p++;
	}
	if(p>in.s+in.len || *p=='\0')
		goto error;
	tok.len = p - tok.s;
	switch(type)
	{
		case 1:
			dbtable = tok;
			LM_DBG("htable [%.*s] - dbtable [%.*s]\n", name.len, name.s,
					dbtable.len, dbtable.s);
			break;
		case 2:
			if(str2int(&tok, &autoexpire)!=0)
				goto error;
			LM_DBG("htable [%.*s] - expire [%u]\n", name.len, name.s,
					autoexpire);
			break;
		case 3:
			if(str2int(&tok, &size)!=0)
				goto error;
			LM_DBG("htable [%.*s] - size [%u]\n", name.len, name.s,
					size);
			break;
	}
	while(p<in.s+in.len && (*p==';' || *p==' ' || *p=='\t'
				|| *p=='\n' || *p=='\r'))
		p++;
	if(p<in.s+in.len)
		goto next_token;

	return ht_pkg_init(&name, autoexpire, &dbtable, size);

error:
	LM_ERR("invalid htable parameter [%.*s] at [%d]\n", in.len, in.s,
			(int)(p-in.s));
	return -1;
}

int ht_db_load_tables(void)
{
	ht_t *ht;

	ht = _ht_root;
	while(ht)
	{
		if(ht->dbtable.len>0)
		{
			LM_DBG("loading db table [%.*s] in ht [%.*s]\n",
					ht->dbtable.len, ht->dbtable.s,
					ht->name.len, ht->name.s);
			if(ht_db_load_table(ht, &ht->dbtable, 0)!=0)
				return -1;
		}
		ht = ht->next;
	}
	return 0;
}

int ht_has_autoexpire(void)
{
	ht_t *ht;

	if(_ht_root==NULL)
		return 0;

	ht = _ht_root;
	while(ht)
	{
		if(ht->htexpire>0)
			return 1;
		ht = ht->next;
	}
	return 0;
}

void ht_timer(unsigned int ticks, void *param)
{
	ht_t *ht;
	ht_cell_t *it;
	ht_cell_t *it0;
	time_t now;
	int i;

	if(_ht_root==NULL)
		return;

	now = time(NULL);
	
	ht = _ht_root;
	while(ht)
	{
		if(ht->htexpire>0)
		{
			for(i=0; i<ht->htsize; i++)
			{
				/* free entries */
				lock_get(&ht->entries[i].lock);
				it = ht->entries[i].first;
				while(it)
				{
					it0 = it->next;
					if(it->expire!=0 && it->expire<now)
					{
						/* expired */
						if(it->prev==NULL)
							ht->entries[i].first = it->next;
						else
							it->prev->next = it->next;
						if(it->next)
							it->next->prev = it->prev;
						ht->entries[i].esize--;
						ht_cell_free(it);
					}
					it = it0;
				}
				lock_release(&ht->entries[i].lock);
			}
		}
		ht = ht->next;
	}
	return;
}

int ht_set_cell_expire(ht_t *ht, str *name, int type, int_str *val)
{
	unsigned int idx;
	unsigned int hid;
	ht_cell_t *it;
	time_t now;

	if(ht==NULL || ht->entries==NULL)
		return -1;

	/* str value - ignore */
	if(type&AVP_VAL_STR)
		return 0;
	/* not auto-expire htable */
	if(ht->htexpire==0)
		return 0;

	hid = ht_compute_hash(name);
	
	idx = ht_get_entry(hid, ht->htsize);

	now = 0;
	if(val->n>0)
		now = time(NULL) + val->n;
	LM_DBG("set auto-expire to %u (%d)\n", (unsigned int)now,
			val->n);

	lock_get(&ht->entries[idx].lock);
	it = ht->entries[idx].first;
	while(it!=NULL && it->cellid < hid)
		it = it->next;
	while(it!=NULL && it->cellid == hid)
	{
		if(name->len==it->name.len 
				&& strncmp(name->s, it->name.s, name->len)==0)
		{
			/* update value */
			it->expire = now;
			lock_release(&ht->entries[idx].lock);
			return 0;
		}
		it = it->next;
	}
	lock_release(&ht->entries[idx].lock);
	return 0;
}

int ht_get_cell_expire(ht_t *ht, str *name, unsigned int *val)
{
	unsigned int idx;
	unsigned int hid;
	ht_cell_t *it;
	time_t now;

	if(ht==NULL || ht->entries==NULL)
		return -1;

	*val = 0;
	/* not auto-expire htable */
	if(ht->htexpire==0)
		return 0;

	hid = ht_compute_hash(name);
	
	idx = ht_get_entry(hid, ht->htsize);

	now = time(NULL);
	lock_get(&ht->entries[idx].lock);
	it = ht->entries[idx].first;
	while(it!=NULL && it->cellid < hid)
		it = it->next;
	while(it!=NULL && it->cellid == hid)
	{
		if(name->len==it->name.len 
				&& strncmp(name->s, it->name.s, name->len)==0)
		{
			/* update value */
			*val = (unsigned int)(it->expire - now);
			lock_release(&ht->entries[idx].lock);
			return 0;
		}
		it = it->next;
	}
	lock_release(&ht->entries[idx].lock);
	return 0;
}

int ht_rm_cell_re(str *sre, ht_t *ht, int mode)
{
	ht_cell_t *it;
	ht_cell_t *it0;
	int i;
	regex_t re;
	int match;
	regmatch_t pmatch;

	if(sre==NULL || sre->len<=0 || ht==NULL)
		return -1;

	if (regcomp(&re, sre->s, REG_EXTENDED|REG_ICASE|REG_NEWLINE))
	{
		LM_ERR("bad re %s\n", sre->s);
		return -1;
	}

	for(i=0; i<ht->htsize; i++)
	{
		/* free entries */
		lock_get(&ht->entries[i].lock);
		it = ht->entries[i].first;
		while(it)
		{
			it0 = it->next;
			match = 0;
			if(mode==0)
			{
				if (regexec(&re, it->name.s, 1, &pmatch, 0)==0)
					match = 1;
			} else {
				if(it->flags&AVP_VAL_STR)
					if (regexec(&re, it->value.s.s, 1, &pmatch, 0)==0)
						match = 1;
			}
			if(match==1)
			{
				if(it->prev==NULL)
					ht->entries[i].first = it->next;
				else
					it->prev->next = it->next;
				if(it->next)
					it->next->prev = it->prev;
				ht->entries[i].esize--;
				ht_cell_free(it);
			}
			it = it0;
		}
		lock_release(&ht->entries[i].lock);
	}
	regfree(&re);
	return 0;
}

int ht_count_cells_re(str *sre, ht_t *ht, int mode)
{
	ht_cell_t *it;
	ht_cell_t *it0;
	int i;
	regex_t re;
	regmatch_t pmatch;
	int cnt = 0;
	int op = 0;
	str sval;
	str tval;
	int ival = 0;

	if(sre==NULL || sre->len<=0 || ht==NULL)
		return 0;

	if(sre->len>=2)
	{
		switch(sre->s[0]) {
			case '~':
				switch(sre->s[1]) {
					case '~':
						op = 1; /* regexp */
					break;
					case '%':
						op = 2; /* rlike */
					break;
				}
			break;
			case '%':
				switch(sre->s[1]) {
					case '~':
						op = 3; /* llike */
					break;
				}
			break;
			case '=':
				switch(sre->s[1]) {
					case '=':
						op = 4; /* str eq */
					break;
				}
			break;
			case 'e':
				switch(sre->s[1]) {
					case 'q':
						op = 5; /* int eq */
					break;
				}
			break;
			case '*':
				switch(sre->s[1]) {
					case '*':
						op = 6; /* int eq */
					break;
				}
			break;
		}
	}

	if(op==6) {
		/* count all */
		for(i=0; i<ht->htsize; i++)
			cnt += ht->entries[i].esize;
		return cnt;
	}

	if(op > 0) {
		if(sre->len<=2)
			return 0;
		sval = *sre;
		sval.s += 2;
		sval.len -= 2;
		if(op==5) {
			if(mode==0)
			{
				/* match by name */
				return 0;
			}
			str2sint(&sval, &ival);
		}
	} else {
		sval = *sre;
		op = 1;
	}

	if(op==1)
	{
		if (regcomp(&re, sval.s, REG_EXTENDED|REG_ICASE|REG_NEWLINE))
		{
			LM_ERR("bad re %s\n", sre->s);
			return 0;
		}
	}

	for(i=0; i<ht->htsize; i++)
	{
		/* free entries */
		lock_get(&ht->entries[i].lock);
		it = ht->entries[i].first;
		while(it)
		{
			it0 = it->next;
			if(op==5)
			{
				if(!(it->flags&AVP_VAL_STR))
					if( it->value.n==ival)
						cnt++;
			} else {
				tval.len = -1;
				if(mode==0)
				{
					/* match by name */
					tval = it->name;
				} else {
					if(it->flags&AVP_VAL_STR)
						tval = it->value.s;
				}
				if(tval.len>-1) {
					switch(op) {
						case 1: /* regexp */
							if (regexec(&re, tval.s, 1, &pmatch, 0)==0)
								cnt++;
						break;
						case 2: /* rlike */
							if(sval.len<=tval.len 
									&& strncmp(sval.s,
										tval.s+tval.len-sval.len, sval.len)==0)
								cnt++;
						break;
						case 3: /* llike */
							if(sval.len<=tval.len 
									&& strncmp(sval.s, tval.s, sval.len)==0)
								cnt++;
						break;
						case 4: /* str eq */
							if(sval.len==tval.len 
									&& strncmp(sval.s, tval.s, sval.len)==0)
								cnt++;
						break;
					}
				}
			}
			it = it0;
		}
		lock_release(&ht->entries[i].lock);
	}
	if(op==1)
		regfree(&re);
	return cnt;
}

