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
		       
#include "ht_api.h"
#include "ht_var.h"

/* pkg copy */
ht_cell_t *_htc_local=NULL;

int pv_get_ht_cell(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	str htname;
	ht_cell_t *htc=NULL;
	ht_pv_t *hpv;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
	{
		hpv->ht = ht_get_table(&hpv->htname);
		if(hpv->ht==NULL)
			return pv_get_null(msg, param, res);
	}
	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	htc = ht_cell_pkg_copy(hpv->ht, &htname, _htc_local);
	if(htc==NULL)
		return pv_get_null(msg, param, res);
	if(_htc_local!=htc)
	{
		ht_cell_pkg_free(_htc_local);
		_htc_local=htc;
	}

	if(htc->flags&AVP_VAL_STR)
		return pv_get_strval(msg, param, res, &htc->value.s);
	
	/* integer */
	return pv_get_sintval(msg, param, res, htc->value.n);
}

int pv_set_ht_cell(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	str htname;
	int_str isval;
	ht_pv_t *hpv;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
		hpv->ht = ht_get_table(&hpv->htname);
	if(hpv->ht==NULL)
		return -1;

	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	LM_DBG("set value for $ht(%.*s=>%.*s)\n", hpv->htname.len, hpv->htname.s,
			htname.len, htname.s);
	if((val==NULL) || (val->flags&PV_VAL_NULL))
	{
		/* delete it */
		ht_del_cell(hpv->ht, &htname);
		return 0;
	}

	if(val->flags&PV_TYPE_INT)
	{
		isval.n = val->ri;
		if(ht_set_cell(hpv->ht, &htname, 0, &isval, 1)!=0)
		{
			LM_ERR("cannot set $ht(%.*s)\n", htname.len, htname.s);
			return -1;
		}
	} else {
		isval.s = val->rs;
		if(ht_set_cell(hpv->ht, &htname, AVP_VAL_STR, &isval, 1)!=0)
		{
			LM_ERR("cannot set $ht(%.*s)\n", htname.len, htname.s);
			return -1;
		}
	}
	return 0;
}

int pv_parse_ht_name(pv_spec_p sp, str *in)
{
	ht_pv_t *hpv=NULL;
	char *p;
	str pvs;

	if(in->s==NULL || in->len<=0)
		return -1;

	hpv = (ht_pv_t*)pkg_malloc(sizeof(ht_pv_t));
	if(hpv==NULL)
		return -1;

	memset(hpv, 0, sizeof(ht_pv_t));

	p = in->s;

	while(p<in->s+in->len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p++;
	if(p>in->s+in->len || *p=='\0')
		goto error;
	hpv->htname.s = p;
	while(p < in->s + in->len)
	{
		if(*p=='=' || *p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
			break;
		p++;
	}
	if(p>in->s+in->len || *p=='\0')
		goto error;
	hpv->htname.len = p - hpv->htname.s;
	if(*p!='=')
	{
		while(p<in->s+in->len && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
			p++;
		if(p>in->s+in->len || *p=='\0' || *p!='=')
			goto error;
	}
	p++;
	if(*p!='>')
		goto error;
	p++;

	pvs.len = in->len - (int)(p - in->s);
	pvs.s = p;
	LM_DBG("htable [%.*s] - key [%.*s]\n", hpv->htname.len, hpv->htname.s,
			pvs.len, pvs.s);
	if(pv_parse_format(&pvs, &hpv->pve)<0 || hpv->pve==NULL)
	{
		LM_ERR("wrong format[%.*s]\n", in->len, in->s);
		goto error;
	}
	hpv->ht = ht_get_table(&hpv->htname);
	sp->pvp.pvn.u.dname = (void*)hpv;
	sp->pvp.pvn.type = PV_NAME_OTHER;
	return 0;

error:
	if(hpv!=NULL)
		pkg_free(hpv);
	return -1;
}

int pv_get_ht_cell_expire(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	str htname;
	ht_pv_t *hpv;
	unsigned int now;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
	{
		hpv->ht = ht_get_table(&hpv->htname);
		if(hpv->ht==NULL)
			return pv_get_null(msg, param, res);
	}
	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	if(ht_get_cell_expire(hpv->ht, &htname, &now)!=0)
		return pv_get_null(msg, param, res);
	/* integer */
	return pv_get_uintval(msg, param, res, now);
}

int pv_set_ht_cell_expire(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	str htname;
	int_str isval;
	ht_pv_t *hpv;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
		hpv->ht = ht_get_table(&hpv->htname);
	if(hpv->ht==NULL)
		return -1;

	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	LM_DBG("set expire value for $ht(%.*s=>%.*s)\n", hpv->htname.len,
			hpv->htname.s, htname.len, htname.s);
	isval.n = 0;
	if(val!=NULL)
	{
		if(val->flags&PV_TYPE_INT)
			isval.n = val->ri;
	}
	if(ht_set_cell_expire(hpv->ht, &htname, 0, &isval)!=0)
	{
		LM_ERR("cannot set $ht(%.*s)\n", htname.len, htname.s);
		return -1;
	}

	return 0;
}

int pv_get_ht_cn(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	str htname;
	ht_pv_t *hpv;
	int cnt = 0;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
	{
		hpv->ht = ht_get_table(&hpv->htname);
		if(hpv->ht==NULL)
			return pv_get_null(msg, param, res);
	}
	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	
	cnt = ht_count_cells_re(&htname, hpv->ht, 0);

	/* integer */
	return pv_get_sintval(msg, param, res, cnt);
}

int pv_get_ht_cv(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	str htname;
	ht_pv_t *hpv;
	int cnt = 0;

	hpv = (ht_pv_t*)param->pvn.u.dname;

	if(hpv->ht==NULL)
	{
		hpv->ht = ht_get_table(&hpv->htname);
		if(hpv->ht==NULL)
			return pv_get_null(msg, param, res);
	}
	if(pv_printf_s(msg, hpv->pve, &htname)!=0)
	{
		LM_ERR("cannot get $ht name\n");
		return -1;
	}
	
	cnt = ht_count_cells_re(&htname, hpv->ht, 1);

	/* integer */
	return pv_get_sintval(msg, param, res, cnt);
}

