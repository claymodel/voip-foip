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

#include "../../mem/mem.h"

#include "tmx_mod.h"
#include "t_var.h"

struct _pv_tmx_data {
	struct cell *T;
	struct sip_msg msg;
	struct sip_msg *tmsgp;
	unsigned int id;
	char *buf;
	int buf_size;
};

static struct _pv_tmx_data _pv_treq;
static struct _pv_tmx_data _pv_trpl;
static struct _pv_tmx_data _pv_tinv;

void pv_tmx_data_init(void)
{
	memset(&_pv_treq, 0, sizeof(struct _pv_tmx_data));
	memset(&_pv_trpl, 0, sizeof(struct _pv_tmx_data));
	memset(&_pv_tinv, 0, sizeof(struct _pv_tmx_data));
}

int pv_t_copy_msg(struct sip_msg *src, struct sip_msg *dst)
{
	dst->id = src->id;
	dst->rcv = src->rcv;
	dst->set_global_address=src->set_global_address;
	dst->set_global_port=src->set_global_port;
	dst->flags = src->flags;
	dst->fwd_send_flags = src->fwd_send_flags;
	dst->rpl_send_flags = src->rpl_send_flags;
	dst->force_send_socket = src->force_send_socket;

	if (parse_msg(dst->buf, dst->len, dst)!=0)
	{
		LM_ERR("parse msg failed\n");
		return -1;
	}
	return 0;
}

int pv_t_update_req(struct sip_msg *msg)
{
	struct cell * t;
	int branch;

	if(msg==NULL)
		return 1;

	if(msg!=FAKED_REPLY && msg->first_line.type!=SIP_REPLY)
		return 1;

	t = _tmx_tmb.t_gett();

	if(t==NULL || t==T_UNDEFINED)
	{
		if(msg==FAKED_REPLY)
			return 1;
		branch=-1;
		if (_tmx_tmb.t_check(msg, &branch ) == -1)
			return 1;
		t = _tmx_tmb.t_gett();
		if ((t == 0) || (t == T_UNDEFINED))
			return 1;

	}

	if(t->uas.request==NULL)
		return 1;

	if(_pv_treq.T==t && t->uas.request==_pv_treq.tmsgp
			&& t->uas.request->id==_pv_treq.id)
		return 0;

	/* make a copy */
	if(_pv_treq.buf==NULL || _pv_treq.buf_size<t->uas.request->len+1)
	{
		if(_pv_treq.buf!=NULL)
			pkg_free(_pv_treq.buf);
		if(_pv_treq.tmsgp)
			free_sip_msg(&_pv_treq.msg);
		_pv_treq.tmsgp = NULL;
		_pv_treq.id = 0;
		_pv_treq.T = NULL;
		_pv_treq.buf_size = t->uas.request->len+1;
		_pv_treq.buf = (char*)pkg_malloc(_pv_treq.buf_size*sizeof(char));
		if(_pv_treq.buf==NULL)
		{
			LM_ERR("no more pkg\n");
			_pv_treq.buf_size = 0;
			return -1;
		}
	}
	if(_pv_treq.tmsgp)
		free_sip_msg(&_pv_treq.msg);
	memset(&_pv_treq.msg, 0, sizeof(struct sip_msg));
	memcpy(_pv_treq.buf, t->uas.request->buf, t->uas.request->len);
	_pv_treq.buf[t->uas.request->len] = '\0';
	_pv_treq.msg.len = t->uas.request->len;
	_pv_treq.msg.buf = _pv_treq.buf;
	_pv_treq.tmsgp = t->uas.request;
	_pv_treq.id = t->uas.request->id;
	_pv_treq.T = t;


	if(pv_t_copy_msg(t->uas.request, &_pv_treq.msg)!=0)
	{
		pkg_free(_pv_treq.buf);
		_pv_treq.buf_size = 0;
		_pv_treq.buf = NULL;
		_pv_treq.tmsgp = NULL;
		_pv_treq.T = NULL;
		return -1;
	}

	return 0;
}

int pv_t_update_rpl(struct sip_msg *msg)
{
	struct cell * t;
	int branch;
	int cancel;

	if(msg==NULL)
		return 1;

	if(msg==FAKED_REPLY || msg->first_line.type!=SIP_REQUEST)
		return 1;

	t = _tmx_tmb.t_gett();

	if(t==NULL || t==T_UNDEFINED)
	{
		if(_tmx_tmb.t_lookup_request(msg, 0, &cancel)<=0)
			return 1;
		t = _tmx_tmb.t_gett();

		if(t==NULL || t==T_UNDEFINED)
			return 1;
	}
	if ( (branch=_tmx_tmb.t_get_picked_branch())<0 )
		return 1;
	if(t->uac[branch].reply==NULL || t->uac[branch].reply==FAKED_REPLY)
		return 1;

	if(_pv_trpl.T==t && t->uac[branch].reply==_pv_trpl.tmsgp
			&& t->uac[branch].reply->id==_pv_trpl.id)
		return 0;

	/* make a copy */
	if(_pv_trpl.buf==NULL || _pv_trpl.buf_size<t->uac[branch].reply->len+1)
	{
		if(_pv_trpl.buf!=NULL)
			pkg_free(_pv_trpl.buf);
		if(_pv_trpl.tmsgp)
			free_sip_msg(&_pv_trpl.msg);
		_pv_trpl.tmsgp = NULL;
		_pv_trpl.id = 0;
		_pv_trpl.T = NULL;
		_pv_trpl.buf_size = t->uac[branch].reply->len+1;
		_pv_trpl.buf = (char*)pkg_malloc(_pv_trpl.buf_size*sizeof(char));
		if(_pv_trpl.buf==NULL)
		{
			LM_ERR("no more pkg\n");
			_pv_trpl.buf_size = 0;
			return -1;
		}
	}
	if(_pv_trpl.tmsgp)
		free_sip_msg(&_pv_trpl.msg);
	memset(&_pv_trpl.msg, 0, sizeof(struct sip_msg));
	memcpy(_pv_trpl.buf, t->uac[branch].reply->buf, t->uac[branch].reply->len);
	_pv_trpl.buf[t->uac[branch].reply->len] = '\0';
	_pv_trpl.msg.len = t->uac[branch].reply->len;
	_pv_trpl.msg.buf = _pv_trpl.buf;
	_pv_trpl.tmsgp = t->uac[branch].reply;
	_pv_trpl.id = t->uac[branch].reply->id;
	_pv_trpl.T = t;

	if(pv_t_copy_msg(t->uac[branch].reply, &_pv_trpl.msg)!=0)
	{
		pkg_free(_pv_trpl.buf);
		_pv_trpl.buf_size = 0;
		_pv_trpl.buf = NULL;
		_pv_trpl.tmsgp = NULL;
		_pv_trpl.T = NULL;
		return -1;
	}

	return 0;
}

int pv_t_update_inv(struct sip_msg *msg)
{
	struct cell * t;

	if(msg==NULL)
		return 1;
	if (msg->REQ_METHOD!=METHOD_CANCEL)
		return 1;

	t = _tmx_tmb.t_lookup_original(msg);

	if(t==NULL || t==T_UNDEFINED)
		return 1;

	if(t->uas.request==NULL) {
		_tmx_tmb.unref_cell(t);
		return 1;
	}

	if(_pv_tinv.T==t && t->uas.request==_pv_tinv.tmsgp
			&& t->uas.request->id==_pv_tinv.id)
		goto done;

	/* make a copy */
	if(_pv_tinv.buf==NULL || _pv_tinv.buf_size<t->uas.request->len+1)
	{
		if(_pv_tinv.buf!=NULL)
			pkg_free(_pv_tinv.buf);
		if(_pv_tinv.tmsgp)
			free_sip_msg(&_pv_tinv.msg);
		_pv_tinv.tmsgp = NULL;
		_pv_tinv.id = 0;
		_pv_tinv.T = NULL;
		_pv_tinv.buf_size = t->uas.request->len+1;
		_pv_tinv.buf = (char*)pkg_malloc(_pv_tinv.buf_size*sizeof(char));
		if(_pv_tinv.buf==NULL)
		{
			LM_ERR("no more pkg\n");
			_pv_tinv.buf_size = 0;
			goto error;
		}
	}
	if(_pv_tinv.tmsgp)
		free_sip_msg(&_pv_tinv.msg);
	memset(&_pv_tinv.msg, 0, sizeof(struct sip_msg));
	memcpy(_pv_tinv.buf, t->uas.request->buf, t->uas.request->len);
	_pv_tinv.buf[t->uas.request->len] = '\0';
	_pv_tinv.msg.len = t->uas.request->len;
	_pv_tinv.msg.buf = _pv_tinv.buf;
	_pv_tinv.tmsgp = t->uas.request;
	_pv_tinv.id = t->uas.request->id;
	_pv_tinv.T = t;


	if(pv_t_copy_msg(t->uas.request, &_pv_tinv.msg)!=0)
	{
		pkg_free(_pv_tinv.buf);
		_pv_tinv.buf_size = 0;
		_pv_tinv.buf = NULL;
		_pv_tinv.tmsgp = NULL;
		_pv_tinv.T = NULL;
		goto error;
	}

done:
	_tmx_tmb.unref_cell(t);
	return 0;

error:
	_tmx_tmb.unref_cell(t);
	return -1;
}

int pv_get_t_var_req(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	pv_spec_t *pv=NULL;

	if(pv_t_update_req(msg))
		return pv_get_null(msg, param, res);

	pv = (pv_spec_t*)param->pvn.u.dname;
	if(pv==NULL || pv_alter_context(pv))
		return pv_get_null(msg, param, res);

	return pv_get_spec_value(&_pv_treq.msg, pv, res);
}

int pv_get_t_var_rpl(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	pv_spec_t *pv=NULL;

	if(pv_t_update_rpl(msg))
		return pv_get_null(msg, param, res);

	pv = (pv_spec_t*)param->pvn.u.dname;
	if(pv==NULL || pv_alter_context(pv))
		return pv_get_null(msg, param, res);

	return pv_get_spec_value(&_pv_trpl.msg, pv, res);
}

int pv_get_t_var_inv(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	pv_spec_t *pv=NULL;

	if(pv_t_update_inv(msg))
		return pv_get_null(msg, param, res);

	pv = (pv_spec_t*)param->pvn.u.dname;
	if(pv==NULL || pv_alter_context(pv))
		return pv_get_null(msg, param, res);

	return pv_get_spec_value(&_pv_tinv.msg, pv, res);
}

int pv_parse_t_var_name(pv_spec_p sp, str *in)
{
	pv_spec_t *pv=NULL;

	if(in->s==NULL || in->len<=0)
		return -1;

	pv = (pv_spec_t*)pkg_malloc(sizeof(pv_spec_t));
	if(pv==NULL)
		return -1;

	memset(pv, 0, sizeof(pv_spec_t));

	if(pv_parse_spec(in, pv)==NULL)
		goto error;

	sp->pvp.pvn.u.dname = (void*)pv;
	sp->pvp.pvn.type = PV_NAME_PVAR;
	return 0;

error:
	LM_ERR("invalid pv name [%.*s]\n", in->len, in->s);
	if(pv!=NULL)
		pkg_free(pv);
	return -1;
}

/* item functions */
int pv_get_tm_branch_idx(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	int l = 0;
	char *ch = NULL;
	tm_ctx_t *tcx = 0;
	int idx = 0;

	if(msg==NULL || res==NULL)
		return -1;

	tcx = _tmx_tmb.tm_ctx_get();
	if(tcx != NULL)
		idx = tcx->branch_index;
	
	ch = int2str(idx, &l);

	res->rs.s = ch;
	res->rs.len = l;

	res->ri = idx;
	res->flags = PV_VAL_STR|PV_VAL_INT|PV_TYPE_INT;

	return 0;
}

int pv_get_tm_reply_code(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	struct cell *t;
	int code;
	int branch;

	if(msg==NULL || res==NULL)
		return -1;

	/* first get the transaction */
	if (_tmx_tmb.t_check( msg , 0 )==-1) return -1;
	if ( (t=_tmx_tmb.t_gett())==0) {
		/* no T */
		code = 0;
	} else {
		switch (get_route_type()) {
			case REQUEST_ROUTE:
				/* use the status of the last sent reply */
				code = t->uas.status;
				break;
			case CORE_ONREPLY_ROUTE:
				/*  t_check() above has the side effect of setting T and
				    REFerencing T => we must unref and unset it for the 
				    main/core onreply_route. */
				_tmx_tmb.t_unref(msg);
				/* no break */
			case TM_ONREPLY_ROUTE:
				/* use the status of the current reply */
				code = msg->first_line.u.reply.statuscode;
				break;
			case FAILURE_ROUTE:
				/* use the status of the winning reply */
				if ( (branch=_tmx_tmb.t_get_picked_branch())<0 ) {
					LM_CRIT("no picked branch (%d) for a final response"
							" in MODE_ONFAILURE\n", branch);
					code = 0;
				} else {
					code = t->uac[branch].last_received;
				}
				break;
			default:
				LM_ERR("unsupported route_type %d\n", get_route_type());
				code = 0;
		}
	}

	LM_DBG("reply code is <%d>\n",code);

	res->rs.s = int2str( code, &res->rs.len);

	res->ri = code;
	res->flags = PV_VAL_STR|PV_VAL_INT|PV_TYPE_INT;
	return 0;
}

