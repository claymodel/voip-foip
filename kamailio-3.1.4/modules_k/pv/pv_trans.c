/*
 * $Id$
 *
 * Copyright (C) 2007 voice-system.ro
 * Copyright (C) 2009 asipto.com
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
 *
 */

/*! \file
 * \brief Support for transformations
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../ut.h" 
#include "../../trim.h" 
#include "../../dset.h"
#include "../../lib/kcore/errinfo.h"

#include "../../parser/parse_param.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_to.h"
#include "../../parser/parse_nameaddr.h"

#include "../../lib/kcore/strcommon.h"
#include "pv_trans.h"

#define is_in_str(p, in) (p<in->s+in->len && *p)

/*! transformation buffer size */
#define TR_BUFFER_SIZE 65536

/*! transformation buffer */
static char _tr_buffer[TR_BUFFER_SIZE];


/*!
 * \brief Evaluate string transformations
 * \param msg SIP message
 * \param tp transformation
 * \param subtype transformation type
 * \param val pseudo-variable
 * \return 0 on success, -1 on error
 */
int tr_eval_string(struct sip_msg *msg, tr_param_t *tp, int subtype,
		pv_value_t *val)
{
	int i, j;
	char *p, *s;
	str st;
	pv_value_t v;

	if(val==NULL || val->flags&PV_VAL_NULL)
		return -1;

	switch(subtype)
	{
		case TR_S_LEN:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);

			val->flags = PV_TYPE_INT|PV_VAL_INT|PV_VAL_STR;
			val->ri = val->rs.len;
			val->rs.s = int2str(val->ri, &val->rs.len);
			break;
		case TR_S_INT:
			if(!(val->flags&PV_VAL_INT))
			{
				if(str2sint(&val->rs, &val->ri)!=0)
					return -1;
			} else { 
				if(!(val->flags&PV_VAL_STR))
					val->rs.s = int2str(val->ri, &val->rs.len);
			}

			val->flags = PV_TYPE_INT|PV_VAL_INT|PV_VAL_STR;
			break;
		case TR_S_MD5:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);

			compute_md5(_tr_buffer, val->rs.s, val->rs.len);
			_tr_buffer[MD5_LEN] = '\0';
			val->flags = PV_VAL_STR;
			val->ri = 0;
			val->rs.s = _tr_buffer;
			val->rs.len = MD5_LEN;
			break;
		case TR_S_ENCODEHEXA:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE/2-1)
				return -1;
			j = 0;
			for(i=0; i<val->rs.len; i++)
			{
				_tr_buffer[j++] = fourbits2char[val->rs.s[i] >> 4];
				_tr_buffer[j++] = fourbits2char[val->rs.s[i] & 0xf];
			}
			_tr_buffer[j] = '\0';
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs.s = _tr_buffer;
			val->rs.len = j;
			break;
		case TR_S_DECODEHEXA:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE*2-1)
				return -1;
			for(i=0; i<val->rs.len/2; i++)
			{
				if(val->rs.s[2*i]>='0'&&val->rs.s[2*i]<='9')
					_tr_buffer[i] = (val->rs.s[2*i]-'0') << 4;
				else if(val->rs.s[2*i]>='a'&&val->rs.s[2*i]<='f')
					_tr_buffer[i] = (val->rs.s[2*i]-'a'+10) << 4;
				else if(val->rs.s[2*i]>='A'&&val->rs.s[2*i]<='F')
					_tr_buffer[i] = (val->rs.s[2*i]-'A'+10) << 4;
				else return -1;

				if(val->rs.s[2*i+1]>='0'&&val->rs.s[2*i+1]<='9')
					_tr_buffer[i] += val->rs.s[2*i+1]-'0';
				else if(val->rs.s[2*i+1]>='a'&&val->rs.s[2*i+1]<='f')
					_tr_buffer[i] += val->rs.s[2*i+1]-'a'+10;
				else if(val->rs.s[2*i+1]>='A'&&val->rs.s[2*i+1]<='F')
					_tr_buffer[i] += val->rs.s[2*i+1]-'A'+10;
				else return -1;
			}
			_tr_buffer[i] = '\0';
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs.s = _tr_buffer;
			val->rs.len = i;
			break;
		case TR_S_ESCAPECOMMON:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE/2-1)
				return -1;
			i = escape_common(_tr_buffer, val->rs.s, val->rs.len);
			_tr_buffer[i] = '\0';
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs.s = _tr_buffer;
			val->rs.len = i;
			break;
		case TR_S_UNESCAPECOMMON:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE-1)
				return -1;
			i = unescape_common(_tr_buffer, val->rs.s, val->rs.len);
			_tr_buffer[i] = '\0';
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs.s = _tr_buffer;
			val->rs.len = i;
			break;
		case TR_S_ESCAPEUSER:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE/2-1)
				return -1;
			st.s = _tr_buffer;
			st.len = TR_BUFFER_SIZE;
			if (escape_user(&val->rs, &st))
				return -1;
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;
		case TR_S_UNESCAPEUSER:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE-1)
				return -1;
			st.s = _tr_buffer;
			st.len = TR_BUFFER_SIZE;
			if (unescape_user(&val->rs, &st))
				return -1;
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;
		case TR_S_ESCAPEPARAM:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE/2-1)
				return -1;
			st.s = _tr_buffer;
			st.len = TR_BUFFER_SIZE;
			if (escape_param(&val->rs, &st) < 0)
				return -1;
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;
		case TR_S_UNESCAPEPARAM:
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(val->rs.len>TR_BUFFER_SIZE-1)
				return -1;
			st.s = _tr_buffer;
			st.len = TR_BUFFER_SIZE;
			if (unescape_param(&val->rs, &st) < 0)
				return -1;
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;
		case TR_S_SUBSTR:
			if(tp==NULL || tp->next==NULL)
			{
				LM_ERR("substr invalid parameters\n");
				return -1;
			}
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(tp->type==TR_PARAM_NUMBER)
			{
				i = tp->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("substr cannot get p1\n");
					return -1;
				}
				i = v.ri;
			}
			if(tp->next->type==TR_PARAM_NUMBER)
			{
				j = tp->next->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->next->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("substr cannot get p2\n");
					return -1;
				}
				j = v.ri;
			}
			LM_DBG("i=%d j=%d\n", i, j);
			if(j<0)
			{
				LM_ERR("substr negative offset\n");
				return -1;
			}
			val->flags = PV_VAL_STR;
			val->ri = 0;
			if(i>=0)
			{
				if(i>=val->rs.len)
				{
					LM_ERR("substr out of range\n");
					return -1;
				}
				if(i+j>=val->rs.len) j=0;
				if(j==0)
				{ /* to end */
					val->rs.s += i;
					val->rs.len -= i;
					break;
				}
				val->rs.s += i;
				val->rs.len = j;
				break;
			}
			i = -i;
			if(i>val->rs.len)
			{
				LM_ERR("substr out of range\n");
				return -1;
			}
			if(i<j) j=0;
			if(j==0)
			{ /* to end */
				val->rs.s += val->rs.len-i;
				val->rs.len = i;
				break;
			}
			val->rs.s += val->rs.len-i;
			val->rs.len = j;
			break;

		case TR_S_SELECT:
			if(tp==NULL || tp->next==NULL)
			{
				LM_ERR("select invalid parameters\n");
				return -1;
			}
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(tp->type==TR_PARAM_NUMBER)
			{
				i = tp->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("select cannot get p1\n");
					return -1;
				}
				i = v.ri;
			}
			val->flags = PV_VAL_STR;
			val->ri = 0;
			if(i<0)
			{
				s = val->rs.s+val->rs.len-1;
				p = s;
				i = -i;
				i--;
				while(p>=val->rs.s)
				{
					if(*p==tp->next->v.s.s[0])
					{
						if(i==0)
							break;
						s = p-1;
						i--;
					}
					p--;
				}
				if(i==0)
				{
					val->rs.s = p+1;
					val->rs.len = s-p;
				} else {
					val->rs.s = "";
					val->rs.len = 0;
				}
			} else {
				s = val->rs.s;
				p = s;
				while(p<val->rs.s+val->rs.len)
				{
					if(*p==tp->next->v.s.s[0])
					{
						if(i==0)
							break;
						s = p + 1;
						i--;
					}
					p++;
				}
				if(i==0)
				{
					val->rs.s = s;
					val->rs.len = p-s;
				} else {
					val->rs.s = "";
					val->rs.len = 0;
				}
			}
			break;

		case TR_S_TOLOWER:
			if(!(val->flags&PV_VAL_STR))
			{
				val->rs.s = int2str(val->ri, &val->rs.len);
				val->flags |= PV_VAL_STR;
				break;
			}
			if(val->rs.len>TR_BUFFER_SIZE-1)
				return -1;
			st.s = _tr_buffer;
			st.len = val->rs.len;
			for (i=0; i<st.len; i++)
				st.s[i]=(val->rs.s[i]>='A' && val->rs.s[i]<='Z')
							?('a' + val->rs.s[i] -'A'):val->rs.s[i];
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;

		case TR_S_TOUPPER:
			if(!(val->flags&PV_VAL_STR))
			{
				val->rs.s = int2str(val->ri, &val->rs.len);
				val->flags |= PV_VAL_STR;
				break;
			}
			if(val->rs.len>TR_BUFFER_SIZE-1)
				return -1;
			st.s = _tr_buffer;
			st.len = val->rs.len;
			for (i=0; i<st.len; i++)
				st.s[i]=(val->rs.s[i]>='a' && val->rs.s[i]<='z')
							?('A' + val->rs.s[i] -'a'):val->rs.s[i];
			memset(val, 0, sizeof(pv_value_t));
			val->flags = PV_VAL_STR;
			val->rs = st;
			break;

		case TR_S_STRIP:
		case TR_S_STRIPTAIL:
			if(tp==NULL)
			{
				LM_ERR("strip invalid parameters\n");
				return -1;
			}
			if(!(val->flags&PV_VAL_STR))
				val->rs.s = int2str(val->ri, &val->rs.len);
			if(tp->type==TR_PARAM_NUMBER)
			{
				i = tp->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("select cannot get p1\n");
					return -1;
				}
				i = v.ri;
			}
			val->flags = PV_VAL_STR;
			val->ri = 0;
			if(i<=0)
				break;
			if(i>=val->rs.len)
			{
				_tr_buffer[0] = '\0';
				val->rs.s = _tr_buffer;
				val->rs.len = 0;
				break;
			}
			if(subtype==TR_S_STRIP)
				val->rs.s += i;
			val->rs.len -= i;
			break;

		default:
			LM_ERR("unknown subtype %d\n",
					subtype);
			return -1;
	}
	return 0;
}

static str _tr_empty = { "", 0 };
static str _tr_uri = {0, 0};
static struct sip_uri _tr_parsed_uri;
static param_t* _tr_uri_params = NULL;


/*!
 * \brief Evaluate URI transformations
 * \param msg SIP message
 * \param tp transformation
 * \param subtype transformation type
 * \param val pseudo-variable
 * \return 0 on success, -1 on error
 */
int tr_eval_uri(struct sip_msg *msg, tr_param_t *tp, int subtype,
		pv_value_t *val)
{
	pv_value_t v;
	str sv;
	param_hooks_t phooks;
	param_t *pit=NULL;

	if(val==NULL || (!(val->flags&PV_VAL_STR)) || val->rs.len<=0)
		return -1;

	if(_tr_uri.len==0 || _tr_uri.len!=val->rs.len ||
			strncmp(_tr_uri.s, val->rs.s, val->rs.len)!=0)
	{
		if(val->rs.len>_tr_uri.len)
		{
			if(_tr_uri.s) pkg_free(_tr_uri.s);
			_tr_uri.s = (char*)pkg_malloc((val->rs.len+1)*sizeof(char));
			if(_tr_uri.s==NULL)
			{
				LM_ERR("no more private memory\n");
				if(_tr_uri_params != NULL)
				{
					free_params(_tr_uri_params);
					_tr_uri_params = 0;
				}
				memset(&_tr_uri, 0, sizeof(str));
				memset(&_tr_parsed_uri, 0, sizeof(struct sip_uri));
				return -1;
			}
		}
		_tr_uri.len = val->rs.len;
		memcpy(_tr_uri.s, val->rs.s, val->rs.len);
		_tr_uri.s[_tr_uri.len] = '\0';
		/* reset old values */
		memset(&_tr_parsed_uri, 0, sizeof(struct sip_uri));
		if(_tr_uri_params != NULL)
		{
			free_params(_tr_uri_params);
			_tr_uri_params = 0;
		}
		/* parse uri -- params only when requested */
		if(parse_uri(_tr_uri.s, _tr_uri.len, &_tr_parsed_uri)!=0)
		{
			LM_ERR("invalid uri [%.*s]\n", val->rs.len,
					val->rs.s);
			if(_tr_uri_params != NULL)
			{
				free_params(_tr_uri_params);
				_tr_uri_params = 0;
			}
			pkg_free(_tr_uri.s);
			memset(&_tr_uri, 0, sizeof(str));
			memset(&_tr_parsed_uri, 0, sizeof(struct sip_uri));
			return -1;
		}
	}
	memset(val, 0, sizeof(pv_value_t));
	val->flags = PV_VAL_STR;

	switch(subtype)
	{
		case TR_URI_USER:
			val->rs = (_tr_parsed_uri.user.s)?_tr_parsed_uri.user:_tr_empty;
			break;
		case TR_URI_HOST:
			val->rs = (_tr_parsed_uri.host.s)?_tr_parsed_uri.host:_tr_empty;
			break;
		case TR_URI_PASSWD:
			val->rs = (_tr_parsed_uri.passwd.s)?_tr_parsed_uri.passwd:_tr_empty;
			break;
		case TR_URI_PORT:
			val->flags |= PV_TYPE_INT|PV_VAL_INT;
			val->rs = (_tr_parsed_uri.port.s)?_tr_parsed_uri.port:_tr_empty;
			val->ri = _tr_parsed_uri.port_no;
			break;
		case TR_URI_PARAMS:
			val->rs = (_tr_parsed_uri.params.s)?_tr_parsed_uri.params:_tr_empty;
			break;
		case TR_URI_PARAM:
			if(tp==NULL)
			{
				LM_ERR("param invalid parameters\n");
				return -1;
			}
			if(_tr_parsed_uri.params.len<=0)
			{
				val->rs = _tr_empty;
				val->flags = PV_VAL_STR;
				val->ri = 0;
				break;
			}

			if(_tr_uri_params == NULL)
			{
				sv = _tr_parsed_uri.params;
				if (parse_params(&sv, CLASS_ANY, &phooks, &_tr_uri_params)<0)
					return -1;
			}
			if(tp->type==TR_PARAM_STRING)
			{
				sv = tp->v.s;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_STR)) || v.rs.len<=0)
				{
					LM_ERR("param cannot get p1\n");
					return -1;
				}
				sv = v.rs;
			}
			for (pit = _tr_uri_params; pit; pit=pit->next)
			{
				if (pit->name.len==sv.len
						&& strncasecmp(pit->name.s, sv.s, sv.len)==0)
				{
					val->rs = pit->body;
					goto done;
				}
			}
			val->rs = _tr_empty;
			break;
		case TR_URI_HEADERS:
			val->rs = (_tr_parsed_uri.headers.s)?_tr_parsed_uri.headers:
						_tr_empty;
			break;
		case TR_URI_TRANSPORT:
			val->rs = (_tr_parsed_uri.transport_val.s)?
				_tr_parsed_uri.transport_val:_tr_empty;
			break;
		case TR_URI_TTL:
			val->rs = (_tr_parsed_uri.ttl_val.s)?
				_tr_parsed_uri.ttl_val:_tr_empty;
			break;
		case TR_URI_UPARAM:
			val->rs = (_tr_parsed_uri.user_param_val.s)?
				_tr_parsed_uri.user_param_val:_tr_empty;
			break;
		case TR_URI_MADDR:
			val->rs = (_tr_parsed_uri.maddr_val.s)?
				_tr_parsed_uri.maddr_val:_tr_empty;
			break;
		case TR_URI_METHOD:
			val->rs = (_tr_parsed_uri.method_val.s)?
				_tr_parsed_uri.method_val:_tr_empty;
			break;
		case TR_URI_LR:
			val->rs = (_tr_parsed_uri.lr_val.s)?
				_tr_parsed_uri.lr_val:_tr_empty;
			break;
		case TR_URI_R2:
			val->rs = (_tr_parsed_uri.r2_val.s)?
				_tr_parsed_uri.r2_val:_tr_empty;
			break;
		default:
			LM_ERR("unknown subtype %d\n",
					subtype);
			return -1;
	}
done:
	return 0;
}

static str _tr_params_str = {0, 0};
static param_t* _tr_params_list = NULL;


/*!
 * \brief Evaluate parameter transformations
 * \param msg SIP message
 * \param tp transformation
 * \param subtype transformation type
 * \param val pseudo-variable
 * \return 0 on success, -1 on error
 */
int tr_eval_paramlist(struct sip_msg *msg, tr_param_t *tp, int subtype,
		pv_value_t *val)
{
	pv_value_t v;
	str sv;
	int n, i;
	param_hooks_t phooks;
	param_t *pit=NULL;

	if(val==NULL || (!(val->flags&PV_VAL_STR)) || val->rs.len<=0)
		return -1;

	if(_tr_params_str.len==0 || _tr_params_str.len!=val->rs.len ||
			strncmp(_tr_params_str.s, val->rs.s, val->rs.len)!=0)
	{
		if(val->rs.len>_tr_params_str.len)
		{
			if(_tr_params_str.s) pkg_free(_tr_params_str.s);
			_tr_params_str.s = (char*)pkg_malloc((val->rs.len+1)*sizeof(char));
			if(_tr_params_str.s==NULL)
			{
				LM_ERR("no more private memory\n");
				memset(&_tr_params_str, 0, sizeof(str));
				if(_tr_params_list != NULL)
				{
					free_params(_tr_params_list);
					_tr_params_list = 0;
				}
				return -1;
			}
		}
		_tr_params_str.len = val->rs.len;
		memcpy(_tr_params_str.s, val->rs.s, val->rs.len);
		_tr_params_str.s[_tr_params_str.len] = '\0';
		
		/* reset old values */
		if(_tr_params_list != NULL)
		{
			free_params(_tr_params_list);
			_tr_params_list = 0;
		}
		
		/* parse params */
		sv = _tr_params_str;
		if (parse_params(&sv, CLASS_ANY, &phooks, &_tr_params_list)<0)
			return -1;
	}
	
	if(_tr_params_list==NULL)
		return -1;

	memset(val, 0, sizeof(pv_value_t));
	val->flags = PV_VAL_STR;

	switch(subtype)
	{
		case TR_PL_VALUE:
			if(tp==NULL)
			{
				LM_ERR("value invalid parameters\n");
				return -1;
			}

			if(tp->type==TR_PARAM_STRING)
			{
				sv = tp->v.s;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_STR)) || v.rs.len<=0)
				{
					LM_ERR("value cannot get p1\n");
					return -1;
				}
				sv = v.rs;
			}
			
			for (pit = _tr_params_list; pit; pit=pit->next)
			{
				if (pit->name.len==sv.len
						&& strncasecmp(pit->name.s, sv.s, sv.len)==0)
				{
					val->rs = pit->body;
					goto done;
				}
			}
			val->rs = _tr_empty;
			break;

		case TR_PL_VALUEAT:
			if(tp==NULL)
			{
				LM_ERR("name invalid parameters\n");
				return -1;
			}

			if(tp->type==TR_PARAM_NUMBER)
			{
				n = tp->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("name cannot get p1\n");
					return -1;
				}
				n = v.ri;
			}
			if(n<0)
			{
				n = -n;
				n--;
				for (pit = _tr_params_list; pit; pit=pit->next)
				{
					if(n==0)
					{
						val->rs = pit->body;
						goto done;
					}
					n--;
				}
			} else {
				/* ugly hack -- params are in reverse order 
				 * - first count then find */
				i = 0;
				for (pit = _tr_params_list; pit; pit=pit->next)
					i++;
				if(n<i)
				{
					n = i - n - 1;
					for (pit = _tr_params_list; pit; pit=pit->next)
					{
						if(n==0)
						{
							val->rs = pit->body;
							goto done;
						}
						n--;
					}
				}
			}
			val->rs = _tr_empty;
			break;

		case TR_PL_NAME:
			if(tp==NULL)
			{
				LM_ERR("name invalid parameters\n");
				return -1;
			}

			if(tp->type==TR_PARAM_NUMBER)
			{
				n = tp->v.n;
			} else {
				if(pv_get_spec_value(msg, (pv_spec_p)tp->v.data, &v)!=0
						|| (!(v.flags&PV_VAL_INT)))
				{
					LM_ERR("name cannot get p1\n");
					return -1;
				}
				n = v.ri;
			}
			if(n<0)
			{
				n = -n;
				n--;
				for (pit = _tr_params_list; pit; pit=pit->next)
				{
					if(n==0)
					{
						val->rs = pit->name;
						goto done;
					}
					n--;
				}
			} else {
				/* ugly hack -- params are in reverse order 
				 * - first count then find */
				i = 0;
				for (pit = _tr_params_list; pit; pit=pit->next)
					i++;
				if(n<i)
				{
					n = i - n - 1;
					for (pit = _tr_params_list; pit; pit=pit->next)
					{
						if(n==0)
						{
							val->rs = pit->name;
							goto done;
						}
						n--;
					}
				}
			}
			val->rs = _tr_empty;
			break;

		case TR_PL_COUNT:
			val->ri = 0;
			for (pit = _tr_params_list; pit; pit=pit->next)
				val->ri++;
			val->flags = PV_TYPE_INT|PV_VAL_INT|PV_VAL_STR;
			val->rs.s = int2str(val->ri, &val->rs.len);
			break;

		default:
			LM_ERR("unknown subtype %d\n",
					subtype);
			return -1;
	}
done:
	return 0;
}

static str _tr_nameaddr_str = {0, 0};
static name_addr_t _tr_nameaddr;


/*!
 * \brief Evaluate name-address transformations
 * \param msg SIP message
 * \param tp transformation
 * \param subtype transformation type
 * \param val pseudo-variable
 * \return 0 on success, -1 on error
 */
int tr_eval_nameaddr(struct sip_msg *msg, tr_param_t *tp, int subtype,
		pv_value_t *val)
{
	str sv;

	if(val==NULL || (!(val->flags&PV_VAL_STR)) || val->rs.len<=0)
		return -1;

	if(_tr_nameaddr_str.len==0 || _tr_nameaddr_str.len!=val->rs.len ||
			strncmp(_tr_nameaddr_str.s, val->rs.s, val->rs.len)!=0)
	{
		if(val->rs.len>_tr_nameaddr_str.len)
		{
			if(_tr_nameaddr_str.s) pkg_free(_tr_nameaddr_str.s);
			_tr_nameaddr_str.s =
						(char*)pkg_malloc((val->rs.len+1)*sizeof(char));
			if(_tr_nameaddr_str.s==NULL)
			{
				LM_ERR("no more private memory\n");
				memset(&_tr_nameaddr_str, 0, sizeof(str));
				memset(&_tr_nameaddr, 0, sizeof(name_addr_t));
				return -1;
			}
		}
		_tr_nameaddr_str.len = val->rs.len;
		memcpy(_tr_nameaddr_str.s, val->rs.s, val->rs.len);
		_tr_nameaddr_str.s[_tr_nameaddr_str.len] = '\0';
		
		/* reset old values */
		memset(&_tr_nameaddr, 0, sizeof(name_addr_t));
		
		/* parse params */
		sv = _tr_nameaddr_str;
		if (parse_nameaddr(&sv, &_tr_nameaddr)<0)
			return -1;
	}
	
	memset(val, 0, sizeof(pv_value_t));
	val->flags = PV_VAL_STR;

	switch(subtype)
	{
		case TR_NA_URI:
			val->rs = (_tr_nameaddr.uri.s)?_tr_nameaddr.uri:_tr_empty;
			break;
		case TR_NA_LEN:
			val->flags = PV_TYPE_INT|PV_VAL_INT|PV_VAL_STR;
			val->ri = _tr_nameaddr.len;
			val->rs.s = int2str(val->ri, &val->rs.len);
			break;
		case TR_NA_NAME:
			val->rs = (_tr_nameaddr.name.s)?_tr_nameaddr.name:_tr_empty;
			break;

		default:
			LM_ERR("unknown subtype %d\n",
					subtype);
			return -1;
	}
	return 0;
}

static str _tr_tobody_str = {0, 0};
static struct to_body _tr_tobody;

/*!
 * \brief Evaluate To-Body transformations
 * \param msg SIP message
 * \param tp transformation
 * \param subtype transformation type
 * \param val pseudo-variable
 * \return 0 on success, -1 on error
 */
int tr_eval_tobody(struct sip_msg *msg, tr_param_t *tp, int subtype,
		pv_value_t *val)
{
	str sv;

	if(val==NULL || (!(val->flags&PV_VAL_STR)) || val->rs.len<=0)
		return -1;

	if(_tr_tobody_str.len==0 || _tr_tobody_str.len!=val->rs.len ||
			strncmp(_tr_tobody_str.s, val->rs.s, val->rs.len)!=0)
	{
		if(_tr_tobody_str.len==0)
			memset(&_tr_tobody, 0, sizeof(struct to_body));
		if(_tr_tobody_str.s==NULL || val->rs.len>_tr_tobody_str.len)
		{
			if(_tr_tobody_str.s) pkg_free(_tr_tobody_str.s);
				_tr_tobody_str.s =
						(char*)pkg_malloc((val->rs.len+3)*sizeof(char));
			if(_tr_tobody_str.s==NULL)
			{
				LM_ERR("no more private memory\n");
				memset(&_tr_tobody_str, 0, sizeof(str));
				memset(&_tr_tobody, 0, sizeof(struct to_body));
				return -1;
			}
		}
		_tr_tobody_str.len = val->rs.len;
		memcpy(_tr_tobody_str.s, val->rs.s, val->rs.len);
		_tr_tobody_str.s[_tr_tobody_str.len] = '\r';
		_tr_tobody_str.s[_tr_tobody_str.len+1] = '\n';
		_tr_tobody_str.s[_tr_tobody_str.len+2] = '\0';
		
		/* reset old values */
		free_to_params(&_tr_tobody);
		memset(&_tr_tobody, 0, sizeof(struct to_body));
		
		/* parse params */
		sv = _tr_tobody_str;
		parse_to(sv.s, sv.s + sv.len + 2, &_tr_tobody);
		if (_tr_tobody.error == PARSE_ERROR)
		{
			memset(&_tr_tobody, 0, sizeof(struct to_body));
			pkg_free(_tr_tobody_str.s);
			memset(&_tr_tobody_str, 0, sizeof(str));
			return -1;
		}
		if (parse_uri(_tr_tobody.uri.s, _tr_tobody.uri.len,
				&_tr_tobody.parsed_uri)<0)
		{
			free_to_params(&_tr_tobody);
			memset(&_tr_tobody, 0, sizeof(struct to_body));
			pkg_free(_tr_tobody_str.s);
			memset(&_tr_tobody_str, 0, sizeof(str));
			return -1;
		}

	}
	
	memset(val, 0, sizeof(pv_value_t));
	val->flags = PV_VAL_STR;

	switch(subtype)
	{
		case TR_TOBODY_URI:
			val->rs = (_tr_tobody.uri.s)?_tr_tobody.uri:_tr_empty;
			break;
		case TR_TOBODY_TAG:
			val->rs = (_tr_tobody.tag_value.s)?_tr_tobody.tag_value:_tr_empty;
			break;
		case TR_TOBODY_DISPLAY:
			val->rs = (_tr_tobody.display.s)?_tr_tobody.display:_tr_empty;
			break;
		case TR_TOBODY_URI_USER:
			val->rs = (_tr_tobody.parsed_uri.user.s)
							?_tr_tobody.parsed_uri.user:_tr_empty;
			break;
		case TR_TOBODY_URI_HOST:
			val->rs = (_tr_tobody.parsed_uri.host.s)
							?_tr_tobody.parsed_uri.host:_tr_empty;
			break;
		case TR_TOBODY_PARAMS:
			if(_tr_tobody.param_lst!=NULL)
			{
				val->rs.s = _tr_tobody.param_lst->name.s;
				val->rs.len = _tr_tobody_str.s + _tr_tobody_str.len
								- val->rs.s;
			} else val->rs = _tr_empty;
			break;

		default:
			LM_ERR("unknown subtype %d\n", subtype);
			return -1;
	}
	return 0;
}


#define _tr_parse_nparam(_p, _p0, _tp, _spec, _n, _sign, _in, _s) \
	while(is_in_str(_p, _in) && (*_p==' ' || *_p=='\t' || *_p=='\n')) _p++; \
	if(*_p==PV_MARKER) \
	{ /* pseudo-variable */ \
		_spec = (pv_spec_t*)pkg_malloc(sizeof(pv_spec_t)); \
		if(_spec==NULL) \
		{ \
			LM_ERR("no more private memory!\n"); \
			goto error; \
		} \
		_s.s = _p; _s.len = _in->s + _in->len - _p; \
		_p0 = pv_parse_spec(&_s, _spec); \
		if(_p0==NULL) \
		{ \
			LM_ERR("invalid spec in substr transformation: %.*s!\n", \
				_in->len, _in->s); \
			goto error; \
		} \
		_p = _p0; \
		_tp = (tr_param_t*)pkg_malloc(sizeof(tr_param_t)); \
		if(_tp==NULL) \
		{ \
			LM_ERR("no more private memory!\n"); \
			goto error; \
		} \
		memset(_tp, 0, sizeof(tr_param_t)); \
		_tp->type = TR_PARAM_SPEC; \
		_tp->v.data = (void*)_spec; \
	} else { \
		if(*_p=='+' || *_p=='-' || (*_p>='0' && *_p<='9')) \
		{ /* number */ \
			_sign = 1; \
			if(*_p=='-') { \
				_p++; \
				_sign = -1; \
			} else if(*_p=='+') _p++; \
			_n = 0; \
			while(is_in_str(_p, _in) && (*_p==' ' || *_p=='\t' || *_p=='\n')) \
					_p++; \
			while(is_in_str(_p, _in) && *_p>='0' && *_p<='9') \
			{ \
				_n = _n*10 + *_p - '0'; \
				_p++; \
			} \
			_tp = (tr_param_t*)pkg_malloc(sizeof(tr_param_t)); \
			if(_tp==NULL) \
			{ \
				LM_ERR("no more private memory!\n"); \
				goto error; \
			} \
			memset(_tp, 0, sizeof(tr_param_t)); \
			_tp->type = TR_PARAM_NUMBER; \
			_tp->v.n = sign*n; \
		} else { \
			LM_ERR("tinvalid param in transformation: %.*s!!\n", \
				_in->len, _in->s); \
			goto error; \
		} \
	}

#define _tr_parse_sparam(_p, _p0, _tp, _spec, _ps, _in, _s) \
	while(is_in_str(_p, _in) && (*_p==' ' || *_p=='\t' || *_p=='\n')) _p++; \
	if(*_p==PV_MARKER) \
	{ /* pseudo-variable */ \
		_spec = (pv_spec_t*)pkg_malloc(sizeof(pv_spec_t)); \
		if(_spec==NULL) \
		{ \
			LM_ERR("no more private memory!\n"); \
			goto error; \
		} \
		_s.s = _p; _s.len = _in->s + _in->len - _p; \
		_p0 = pv_parse_spec(&_s, _spec); \
		if(_p0==NULL) \
		{ \
			LM_ERR("invalid spec in substr transformation: %.*s!\n", \
				_in->len, _in->s); \
			goto error; \
		} \
		_p = _p0; \
		_tp = (tr_param_t*)pkg_malloc(sizeof(tr_param_t)); \
		if(_tp==NULL) \
		{ \
			LM_ERR("no more private memory!\n"); \
			goto error; \
		} \
		memset(_tp, 0, sizeof(tr_param_t)); \
		_tp->type = TR_PARAM_SPEC; \
		_tp->v.data = (void*)_spec; \
	} else { /* string */ \
		while(is_in_str(_p, _in) && (*_p==' ' || *_p=='\t' || *_p=='\n')) \
				_p++; \
		_ps = _p; \
		while(is_in_str(_p, _in) && *_p!=' ' && *_p!='\t' && *_p!='\n' \
				&& *_p!=TR_PARAM_MARKER && *_p!=TR_RBRACKET) \
				_p++; \
		if(*_p=='\0') \
		{ \
			LM_ERR("invalid param in transformation: %.*s!!\n", \
				_in->len, _in->s); \
			goto error; \
		} \
		_tp = (tr_param_t*)pkg_malloc(sizeof(tr_param_t)); \
		if(_tp==NULL) \
		{ \
			LM_ERR("no more private memory!\n"); \
			goto error; \
		} \
		memset(_tp, 0, sizeof(tr_param_t)); \
		_tp->type = TR_PARAM_STRING; \
		_tp->v.s.s = _ps; \
		_tp->v.s.len = _p - _ps; \
	}


/*!
 * \brief Helper fuction to parse a string transformation
 * \param in parsed string
 * \param t transformation
 * \return pointer to the end of the transformation in the string - '}', null on error
 */
char* tr_parse_string(str* in, trans_t *t)
{
	char *p;
	char *p0;
	str name;
	str s;
	pv_spec_t *spec = NULL;
	int n;
	int sign;
	tr_param_t *tp = NULL;

	if(in==NULL || t==NULL)
		return NULL;

	p = in->s;
	name.s = in->s;
	t->type = TR_STRING;
	t->trf = tr_eval_string;

	/* find next token */
	while(is_in_str(p, in) && *p!=TR_PARAM_MARKER && *p!=TR_RBRACKET) p++;
	if(*p=='\0')
	{
		LM_ERR("invalid transformation: %.*s\n",
				in->len, in->s);
		goto error;
	}
	name.len = p - name.s;
	trim(&name);

	if(name.len==3 && strncasecmp(name.s, "len", 3)==0)
	{
		t->subtype = TR_S_LEN;
		goto done;
	} else if(name.len==3 && strncasecmp(name.s, "int", 3)==0) {
		t->subtype = TR_S_INT;
		goto done;
	} else if(name.len==3 && strncasecmp(name.s, "md5", 3)==0) {
		t->subtype = TR_S_MD5;
		goto done;
	} else if(name.len==7 && strncasecmp(name.s, "tolower", 7)==0) {
		t->subtype = TR_S_TOLOWER;
		goto done;
	} else if(name.len==7 && strncasecmp(name.s, "toupper", 7)==0) {
		t->subtype = TR_S_TOUPPER;
		goto done;
	} else if(name.len==11 && strncasecmp(name.s, "encode.hexa", 11)==0) {
		t->subtype = TR_S_ENCODEHEXA;
		goto done;
	} else if(name.len==11 && strncasecmp(name.s, "decode.hexa", 11)==0) {
		t->subtype = TR_S_DECODEHEXA;
		goto done;
	} else if(name.len==13 && strncasecmp(name.s, "escape.common", 13)==0) {
		t->subtype = TR_S_ESCAPECOMMON;
		goto done;
	} else if(name.len==15 && strncasecmp(name.s, "unescape.common", 15)==0) {
		t->subtype = TR_S_UNESCAPECOMMON;
		goto done;
	} else if(name.len==11 && strncasecmp(name.s, "escape.user", 11)==0) {
		t->subtype = TR_S_ESCAPEUSER;
		goto done;
	} else if(name.len==13 && strncasecmp(name.s, "unescape.user", 13)==0) {
		t->subtype = TR_S_UNESCAPEUSER;
		goto done;
	} else if(name.len==12 && strncasecmp(name.s, "escape.param", 12)==0) {
		t->subtype = TR_S_ESCAPEPARAM;
		goto done;
	} else if(name.len==14 && strncasecmp(name.s, "unescape.param", 14)==0) {
		t->subtype = TR_S_UNESCAPEPARAM;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "substr", 6)==0) {
		t->subtype = TR_S_SUBSTR;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid substr transformation: %.*s!\n", in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid substr transformation: %.*s!\n",
				in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s);
		if(tp->type==TR_PARAM_NUMBER && tp->v.n<0)
		{
			LM_ERR("substr negative offset\n");
			goto error;
		}
		t->params->next = tp;
		tp = 0;
		while(is_in_str(p, in) && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid substr transformation: %.*s!!\n",
				in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "select", 6)==0) {
		t->subtype = TR_S_SELECT;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid select transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_PARAM_MARKER || *(p+1)=='\0')
		{
			LM_ERR("invalid select transformation: %.*s!\n", in->len, in->s);
			goto error;
		}
		p++;
		tp = (tr_param_t*)pkg_malloc(sizeof(tr_param_t));
		if(tp==NULL)
		{
			LM_ERR("no more private memory!\n");
			goto error;
		}
		memset(tp, 0, sizeof(tr_param_t));
		tp->type = TR_PARAM_STRING;
		tp->v.s.s = p;
		tp->v.s.len = 1;
		t->params->next = tp;
		tp = 0;
		p++;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid select transformation: %.*s!!\n",
				in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==5 && strncasecmp(name.s, "strip", 5)==0) {
		t->subtype = TR_S_STRIP;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid strip transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid strip transformation: %.*s!!\n",
				in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==9 && strncasecmp(name.s, "striptail", 9)==0) {
		t->subtype = TR_S_STRIPTAIL;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid striptail transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid striptail transformation: %.*s!!\n",
				in->len, in->s);
			goto error;
		}
		goto done;
	}

	LM_ERR("unknown transformation: %.*s/%.*s/%d!\n", in->len, in->s,
			name.len, name.s, name.len);
error:
	if(tp)
		tr_param_free(tp);
	if(spec)
		pv_spec_free(spec);
	return NULL;
done:
	t->name = name;
	return p;
}


/*!
 * \brief Helper fuction to parse a URI transformation
 * \param in parsed string
 * \param t transformation
 * \return pointer to the end of the transformation in the string - '}', null on error
 */
char* tr_parse_uri(str* in, trans_t *t)
{
	char *p;
	char *p0;
	char *ps;
	str name;
	str s;
	pv_spec_t *spec = NULL;
	tr_param_t *tp = NULL;

	if(in==NULL || in->s==NULL || t==NULL)
		return NULL;

	p = in->s;
	name.s = in->s;
	t->type = TR_URI;
	t->trf = tr_eval_uri;

	/* find next token */
	while(*p && *p!=TR_PARAM_MARKER && *p!=TR_RBRACKET) p++;
	if(*p=='\0')
	{
		LM_ERR("invalid transformation: %.*s\n", in->len, in->s);
		goto error;
	}
	name.len = p - name.s;
	trim(&name);

	if(name.len==4 && strncasecmp(name.s, "user", 4)==0)
	{
		t->subtype = TR_URI_USER;
		goto done;
	} else if((name.len==4 && strncasecmp(name.s, "host", 4)==0)
			|| (name.len==6 && strncasecmp(name.s, "domain", 6)==0)) {
		t->subtype = TR_URI_HOST;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "passwd", 6)==0) {
		t->subtype = TR_URI_PASSWD;
		goto done;
	} else if(name.len==4 && strncasecmp(name.s, "port", 4)==0) {
		t->subtype = TR_URI_PORT;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "params", 6)==0) {
		t->subtype = TR_URI_PARAMS;
		goto done;
	} else if(name.len==7 && strncasecmp(name.s, "headers", 7)==0) {
		t->subtype = TR_URI_HEADERS;
		goto done;
	} else if(name.len==5 && strncasecmp(name.s, "param", 5)==0) {
		t->subtype = TR_URI_PARAM;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid param transformation: %.*s\n", in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_sparam(p, p0, tp, spec, ps, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid param transformation: %.*s!\n", in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==9 && strncasecmp(name.s, "transport", 9)==0) {
		t->subtype = TR_URI_TRANSPORT;
		goto done;
	} else if(name.len==3 && strncasecmp(name.s, "ttl", 3)==0) {
		t->subtype = TR_URI_TTL;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "uparam", 6)==0) {
		t->subtype = TR_URI_UPARAM;
		goto done;
	} else if(name.len==5 && strncasecmp(name.s, "maddr", 5)==0) {
		t->subtype = TR_URI_MADDR;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "method", 6)==0) {
		t->subtype = TR_URI_METHOD;
		goto done;
	} else if(name.len==2 && strncasecmp(name.s, "lr", 2)==0) {
		t->subtype = TR_URI_LR;
		goto done;
	} else if(name.len==2 && strncasecmp(name.s, "r2", 2)==0) {
		t->subtype = TR_URI_R2;
		goto done;
	}

	LM_ERR("unknown transformation: %.*s/%.*s!\n", in->len,
			in->s, name.len, name.s);
error:
	if(tp)
		tr_param_free(tp);
	if(spec)
		pv_spec_free(spec);
	return NULL;
done:
	t->name = name;
	return p;
}


/*!
 * \brief Helper fuction to parse a parameter transformation
 * \param in parsed string
 * \param t transformation
 * \return pointer to the end of the transformation in the string - '}', null on error
 */
char* tr_parse_paramlist(str* in, trans_t *t)
{
	char *p;
	char *p0;
	char *ps;
	str s;
	str name;
	int n;
	int sign;
	pv_spec_t *spec = NULL;
	tr_param_t *tp = NULL;

	if(in==NULL || in->s==NULL || t==NULL)
		return NULL;

	p = in->s;
	name.s = in->s;
	t->type = TR_PARAMLIST;
	t->trf = tr_eval_paramlist;

	/* find next token */
	while(is_in_str(p, in) && *p!=TR_PARAM_MARKER && *p!=TR_RBRACKET) p++;
	if(*p=='\0')
	{
		LM_ERR("invalid transformation: %.*s\n",
				in->len, in->s);
		goto error;
	}
	name.len = p - name.s;
	trim(&name);

	if(name.len==5 && strncasecmp(name.s, "value", 5)==0)
	{
		t->subtype = TR_PL_VALUE;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid value transformation: %.*s\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_sparam(p, p0, tp, spec, ps, in, s);
		t->params = tp;
		tp = 0;
		while(*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid value transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==7 && strncasecmp(name.s, "valueat", 7)==0) {
		t->subtype = TR_PL_VALUEAT;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid name transformation: %.*s\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s)
		t->params = tp;
		tp = 0;
		while(is_in_str(p, in) && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid name transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==4 && strncasecmp(name.s, "name", 4)==0) {
		t->subtype = TR_PL_NAME;
		if(*p!=TR_PARAM_MARKER)
		{
			LM_ERR("invalid name transformation: %.*s\n",
					in->len, in->s);
			goto error;
		}
		p++;
		_tr_parse_nparam(p, p0, tp, spec, n, sign, in, s)
		t->params = tp;
		tp = 0;
		while(is_in_str(p, in) && (*p==' ' || *p=='\t' || *p=='\n')) p++;
		if(*p!=TR_RBRACKET)
		{
			LM_ERR("invalid name transformation: %.*s!\n",
					in->len, in->s);
			goto error;
		}
		goto done;
	} else if(name.len==5 && strncasecmp(name.s, "count", 5)==0) {
		t->subtype = TR_PL_COUNT;
		goto done;
	}

	LM_ERR("unknown transformation: %.*s/%.*s!\n",
			in->len, in->s, name.len, name.s);
error:
	if(tp)
		tr_param_free(tp);
	if(spec)
		pv_spec_free(spec);
	return NULL;
done:
	t->name = name;
	return p;
}


/*!
 * \brief Helper fuction to parse a name-address transformation
 * \param in parsed string
 * \param t transformation
 * \return pointer to the end of the transformation in the string - '}', null on error
 */
char* tr_parse_nameaddr(str* in, trans_t *t)
{
	char *p;
	str name;

	if(in==NULL || t==NULL)
		return NULL;

	p = in->s;
	name.s = in->s;
	t->type = TR_NAMEADDR;
	t->trf = tr_eval_nameaddr;

	/* find next token */
	while(is_in_str(p, in) && *p!=TR_PARAM_MARKER && *p!=TR_RBRACKET) p++;
	if(*p=='\0')
	{
		LM_ERR("invalid transformation: %.*s\n",
				in->len, in->s);
		goto error;
	}
	name.len = p - name.s;
	trim(&name);

	if(name.len==3 && strncasecmp(name.s, "uri", 3)==0)
	{
		t->subtype = TR_NA_URI;
		goto done;
	} else if(name.len==3 && strncasecmp(name.s, "len", 3)==0)
	{
		t->subtype = TR_NA_LEN;
		goto done;
	} else if(name.len==4 && strncasecmp(name.s, "name", 4)==0) {
		t->subtype = TR_NA_NAME;
		goto done;
	}


	LM_ERR("unknown transformation: %.*s/%.*s/%d!\n", in->len, in->s,
			name.len, name.s, name.len);
error:
	return NULL;
done:
	t->name = name;
	return p;
}

/*!
 * \brief Helper fuction to parse a name-address transformation
 * \param in parsed string
 * \param t transformation
 * \return pointer to the end of the transformation in the string - '}', null on error
 */
char* tr_parse_tobody(str* in, trans_t *t)
{
	char *p;
	str name;

	if(in==NULL || t==NULL)
		return NULL;

	p = in->s;
	name.s = in->s;
	t->type = TR_TOBODY;
	t->trf = tr_eval_tobody;

	/* find next token */
	while(is_in_str(p, in) && *p!=TR_PARAM_MARKER && *p!=TR_RBRACKET) p++;
	if(*p=='\0')
	{
		LM_ERR("invalid transformation: %.*s\n",
				in->len, in->s);
		goto error;
	}
	name.len = p - name.s;
	trim(&name);

	if(name.len==3 && strncasecmp(name.s, "uri", 3)==0)
	{
		t->subtype = TR_TOBODY_URI;
		goto done;
	} else if(name.len==3 && strncasecmp(name.s, "tag", 3)==0) {
		t->subtype = TR_TOBODY_TAG;
		goto done;
	} else if(name.len==4 && strncasecmp(name.s, "user", 4)==0) {
		t->subtype = TR_TOBODY_URI_USER;
		goto done;
	} else if(name.len==4 && strncasecmp(name.s, "host", 4)==0) {
		t->subtype = TR_TOBODY_URI_HOST;
		goto done;
	} else if(name.len==6 && strncasecmp(name.s, "params", 6)==0) {
		t->subtype = TR_TOBODY_PARAMS;
		goto done;
	} else if(name.len==7 && strncasecmp(name.s, "display", 7)==0) {
		t->subtype = TR_TOBODY_DISPLAY;
		goto done;
	}


	LM_ERR("unknown transformation: %.*s/%.*s/%d!\n", in->len, in->s,
			name.len, name.s, name.len);
error:
	return NULL;
done:
	t->name = name;
	return p;
}

