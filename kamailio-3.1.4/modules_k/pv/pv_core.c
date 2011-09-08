/*
 * $Id$
 *
 * Copyright (C) 2001-2005 FhG Fokus
 * Copyright (C) 2005 Voice Sistem SRL
 * Copyright (C) 2010 Daniel-Constantin Mierla (asipto.com)
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


#include "../../qvalue.h"
#include "../../lib/kcore/errinfo.h"
#include "../../ut.h" 
#include "../../route_struct.h"
#include "../../dset.h"
#include "../../action.h"
#include "../../socket_info.h"
#include "../../data_lump.h"
#include "../../lib/kcore/cmpapi.h"

#include "../../parser/parse_from.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_hname2.h"
#include "../../parser/parse_content.h"
#include "../../parser/parse_refer_to.h"
#include "../../parser/parse_rpid.h"
#include "../../parser/parse_diversion.h"
#include "../../lib/kcore/parse_ppi.h"
#include "../../lib/kcore/parse_pai.h"
#include "../../lib/kcore/parser_helpers.h"
#include "../../parser/digest/digest.h"

#include "pv_core.h"
#include "pv_svar.h"


static str str_udp    = { "UDP", 3 };
static str str_5060   = { "5060", 4 };
static str pv_str_1   = { "1", 1 };

int _pv_pid = 0;

#define PV_FIELD_DELIM ", "
#define PV_FIELD_DELIM_LEN (sizeof(PV_FIELD_DELIM) - 1)
#define PV_LOCAL_BUF_SIZE	511

static char pv_local_buf[PV_LOCAL_BUF_SIZE+1];


int pv_get_msgid(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	return pv_get_uintval(msg, param, res, msg->id);
}

int pv_get_udp(struct sip_msg *msg, pv_param_t *param, 
		pv_value_t *res)
{
	return pv_get_strintval(msg, param, res, &str_udp, (int)PROTO_UDP);
}

int pv_get_5060(struct sip_msg *msg, pv_param_t *param, pv_value_t *res)
{
	return pv_get_strintval(msg, param, res, &str_5060, 5060);
}

int pv_get_true(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	return pv_get_intstrval(msg, param, res, 1, &pv_str_1);
}

/*extern int _last_returned_code;
int pv_get_return_code(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	return pv_get_sintval(msg, param, res, _last_returned_code);
}
*/

int pv_get_pid(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(_pv_pid == 0)
		_pv_pid = (int)getpid();
	return pv_get_sintval(msg, param, res, _pv_pid);
}


int pv_get_method(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type == SIP_REQUEST)
	{
		return pv_get_strintval(msg, param, res,
				&msg->first_line.u.request.method,
				(int)msg->first_line.u.request.method_value);
	}
	
	if(msg->cseq==NULL && ((parse_headers(msg, HDR_CSEQ_F, 0)==-1) || 
				(msg->cseq==NULL)))
	{
		LM_ERR("no CSEQ header\n");
		return pv_get_null(msg, param, res);
	}
	
	return pv_get_strintval(msg, param, res,
			&get_cseq(msg)->method,
			get_cseq(msg)->method_id);
}

int pv_get_version(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type == SIP_REQUEST)
	{
		return pv_get_strval(msg, param, res,
				&msg->first_line.u.request.version);
	}

	return pv_get_strval(msg, param, res,
				&msg->first_line.u.reply.version);
}

int pv_get_status(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type != SIP_REPLY)
		return pv_get_null(msg, param, res);

	return pv_get_intstrval(msg, param, res,
			(int)msg->first_line.u.reply.statuscode,
			&msg->first_line.u.reply.status);
}

int pv_get_reason(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type != SIP_REPLY)
		return pv_get_null(msg, param, res);
	
	return pv_get_strval(msg, param, res, &msg->first_line.u.reply.reason);
}


int pv_get_ruri(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL || res==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)	/* REPLY doesnt have a ruri */
		return pv_get_null(msg, param, res);

	if(msg->parsed_uri_ok==0 /* R-URI not parsed*/ && parse_sip_msg_uri(msg)<0)
	{
		LM_ERR("failed to parse the R-URI\n");
		return pv_get_null(msg, param, res);
	}
	
	if (msg->new_uri.s!=NULL)
		return pv_get_strval(msg, param, res, &msg->new_uri);
	return pv_get_strval(msg, param, res, &msg->first_line.u.request.uri);
}

int pv_get_ouri(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL || res==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)	/* REPLY doesnt have a ruri */
		return pv_get_null(msg, param, res);

	if(msg->parsed_orig_ruri_ok==0
			/* orig R-URI not parsed*/ && parse_orig_ruri(msg)<0)
	{
		LM_ERR("failed to parse the R-URI\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_strval(msg, param, res, &msg->first_line.u.request.uri);
}

int pv_get_xuri_attr(struct sip_msg *msg, struct sip_uri *parsed_uri,
		pv_param_t *param, pv_value_t *res)
{
	if(param->pvn.u.isname.name.n==1) /* username */
	{
		if(parsed_uri->user.s==NULL || parsed_uri->user.len<=0)
			return pv_get_null(msg, param, res);
		return pv_get_strval(msg, param, res, &parsed_uri->user);
	} else if(param->pvn.u.isname.name.n==2) /* domain */ {
		if(parsed_uri->host.s==NULL || parsed_uri->host.len<=0)
			return pv_get_null(msg, param, res);
		return pv_get_strval(msg, param, res, &parsed_uri->host);
	} else if(param->pvn.u.isname.name.n==3) /* port */ {
		if(parsed_uri->port.s==NULL)
			return pv_get_5060(msg, param, res);
		return pv_get_strintval(msg, param, res, &parsed_uri->port,
				(int)parsed_uri->port_no);
	} else if(param->pvn.u.isname.name.n==4) /* protocol */ {
		if(parsed_uri->transport_val.s==NULL)
			return pv_get_udp(msg, param, res);
		return pv_get_strintval(msg, param, res, &parsed_uri->transport_val,
				(int)parsed_uri->proto);
	}
	LM_ERR("unknown specifier\n");
	return pv_get_null(msg, param, res);
}

int pv_get_ruri_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)	/* REPLY doesnt have a ruri */
		return pv_get_null(msg, param, res);

	if(msg->parsed_uri_ok==0 /* R-URI not parsed*/ && parse_sip_msg_uri(msg)<0)
	{
		LM_ERR("failed to parse the R-URI\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_xuri_attr(msg, &(msg->parsed_uri), param, res);
}	

int pv_get_ouri_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)	/* REPLY doesnt have a ruri */
		return pv_get_null(msg, param, res);

	if(msg->parsed_orig_ruri_ok==0
			/* orig R-URI not parsed*/ && parse_orig_ruri(msg)<0)
	{
		LM_ERR("failed to parse the R-URI\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_xuri_attr(msg, &(msg->parsed_orig_ruri), param, res);
}

extern err_info_t _oser_err_info;
int pv_get_errinfo_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(param->pvn.u.isname.name.n==0) /* class */ {
		return pv_get_sintval(msg, param, res, _oser_err_info.eclass);
	} else if(param->pvn.u.isname.name.n==1) /* level */ {
		return pv_get_sintval(msg, param, res, _oser_err_info.level);
	} else if(param->pvn.u.isname.name.n==2) /* info */ {
		if(_oser_err_info.info.s==NULL)
			pv_get_null(msg, param, res);
		return pv_get_strval(msg, param, res, &_oser_err_info.info);
	} else if(param->pvn.u.isname.name.n==3) /* rcode */ {
		return pv_get_sintval(msg, param, res, _oser_err_info.rcode);
	} else if(param->pvn.u.isname.name.n==4) /* rreason */ {
		if(_oser_err_info.rreason.s==NULL)
			pv_get_null(msg, param, res);
		return pv_get_strval(msg, param, res, &_oser_err_info.rreason);
	} else {
		LM_DBG("invalid attribute!\n");
		return pv_get_null(msg, param, res);
	}
	return 0;
}

int pv_get_contact(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->contact==NULL && parse_headers(msg, HDR_CONTACT_F, 0)==-1) 
	{
		LM_DBG("no contact header\n");
		return pv_get_null(msg, param, res);
	}
	
	if(!msg->contact || !msg->contact->body.s || msg->contact->body.len<=0)
    {
		LM_DBG("no contact header!\n");
		return pv_get_null(msg, param, res);
	}
	
//	res->s = ((struct to_body*)msg->contact->parsed)->uri.s;
//	res->len = ((struct to_body*)msg->contact->parsed)->uri.len;
	return pv_get_strval(msg, param, res, &msg->contact->body);
}

int pv_get_xto_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res, struct to_body *xto, int type)
{
	struct sip_uri *uri;
	if(xto==NULL)
		return -1;

	if(param->pvn.u.isname.name.n==1) /* uri */
		return pv_get_strval(msg, param, res, &xto->uri);
	
	if(param->pvn.u.isname.name.n==4) /* tag */
	{
		if (xto->tag_value.s==NULL || xto->tag_value.len<=0)
		{
		        LM_DBG("no Tag parameter\n");
		        return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &xto->tag_value);
	}

	if(param->pvn.u.isname.name.n==5) /* display name */
	{
		if(xto->display.s==NULL || xto->display.len<=0)
		{
			LM_DBG("no Display name\n");
			return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &xto->display);
	}

	if(type==0)
	{
		if((uri=parse_to_uri(msg))==NULL)
		{
			LM_ERR("cannot parse To URI\n");
			return pv_get_null(msg, param, res);
		}
	} else {
		if((uri=parse_from_uri(msg))==NULL)
		{
			LM_ERR("cannot parse From URI\n");
			return pv_get_null(msg, param, res);
		}
	}

	if(param->pvn.u.isname.name.n==2) /* username */
	{
	    if(uri->user.s==NULL || uri->user.len<=0)
		{
		    LM_DBG("no username\n");
			return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &uri->user);
	} else if(param->pvn.u.isname.name.n==3) /* domain */ {
	    if(uri->host.s==NULL || uri->host.len<=0)
		{
		    LM_DBG("no domain\n");
			return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &uri->host);
	}

	LM_ERR("unknown specifier\n");
	return pv_get_null(msg, param, res);
}

int pv_get_to_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(msg->to==NULL && parse_headers(msg, HDR_TO_F, 0)==-1)
	{
		LM_ERR("cannot parse To header\n");
		return pv_get_null(msg, param, res);
	}
	if(msg->to==NULL || get_to(msg)==NULL) {
		LM_DBG("no To header\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_xto_attr(msg, param, res, get_to(msg), 0);
}

int pv_get_from_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(parse_from_header(msg)<0)
	{
		LM_ERR("cannot parse From header\n");
		return pv_get_null(msg, param, res);
	}
	
	if(msg->from==NULL || get_from(msg)==NULL) {
		LM_DBG("no From header\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_xto_attr(msg, param, res, get_from(msg), 1);
}

int pv_get_cseq(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	if(msg->cseq==NULL && ((parse_headers(msg, HDR_CSEQ_F, 0)==-1)
				|| (msg->cseq==NULL)) )
	{
		LM_ERR("cannot parse CSEQ header\n");
		return pv_get_null(msg, param, res);
	}
	return pv_get_strval(msg, param, res, &(get_cseq(msg)->number));
}

int pv_get_msg_buf(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(msg==NULL)
		return -1;
	
	s.s = msg->buf;
	s.len = msg->len;
	return pv_get_strval(msg, param, res, &s);
}

int pv_get_msg_len(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	return pv_get_uintval(msg, param, res, msg->len);
}

int pv_get_flags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	return pv_get_uintval(msg, param, res, msg->flags);
}

static inline char* int_to_8hex(int val)
{
	unsigned short digit;
	int i;
	static char outbuf[9];
	
	outbuf[8] = '\0';
	for(i=0; i<8; i++)
	{
		if(val!=0)
		{
			digit =  val & 0x0f;
			outbuf[7-i] = digit >= 10 ? digit + 'a' - 10 : digit + '0';
			val >>= 4;
		}
		else
			outbuf[7-i] = '0';
	}
	return outbuf;
}


int pv_get_hexflags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(msg==NULL || res==NULL)
		return -1;

	s.s = int_to_8hex(msg->flags);
	s.len = 8;
	return pv_get_strintval(msg, param, res, &s, (int)msg->flags);
}

int pv_get_bflags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	flag_t flags;
	if (getbflagsval(0, &flags) < 0) {
		ERR("pv_get_bflags: Error while obtainig values of branch flags\n");
		return -1;
	}
	return pv_get_uintval(msg, param, res, flags);
}

int pv_get_hexbflags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	flag_t flags;
	str s;
	if(res==NULL)
		return -1;

	if (getbflagsval(0, &flags) < 0) {
		ERR("pv_get_hexbflags: Error while obtaining values of branch flags\n");
		return -1;
	}
	s.s = int_to_8hex((int)flags);
	s.len = 8;
	return pv_get_strintval(msg, param, res, &s, (int)flags);
}

int pv_get_sflags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	return pv_get_uintval(msg, param, res, getsflags());
}

int pv_get_hexsflags(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(res==NULL)
		return -1;

	s.s = int_to_8hex((int)getsflags());
	s.len = 8;
	return pv_get_strintval(msg, param, res, &s, (int)getsflags());
}

int pv_get_callid(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	if(msg->callid==NULL && ((parse_headers(msg, HDR_CALLID_F, 0)==-1) ||
				(msg->callid==NULL)) )
	{
		LM_ERR("cannot parse Call-Id header\n");
		return pv_get_null(msg, param, res);
	}

	return pv_get_strval(msg, param, res, &msg->callid->body);
}

int pv_get_srcip(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(msg==NULL)
		return -1;

	s.s = ip_addr2a(&msg->rcv.src_ip);
	s.len = strlen(s.s);
	return pv_get_strval(msg, param, res, &s);
}

int pv_get_srcport(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	return pv_get_uintval(msg, param, res, msg->rcv.src_port);
}

int pv_get_rcvip(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	if(msg->rcv.bind_address==NULL 
			|| msg->rcv.bind_address->address_str.s==NULL)
		return pv_get_null(msg, param, res);
	
	return pv_get_strval(msg, param, res, &msg->rcv.bind_address->address_str);
}

int pv_get_rcvport(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	if(msg->rcv.bind_address==NULL 
			|| msg->rcv.bind_address->port_no_str.s==NULL)
		return pv_get_null(msg, param, res);
	
	return pv_get_intstrval(msg, param, res,
			(int)msg->rcv.bind_address->port_no,
			&msg->rcv.bind_address->port_no_str);
}

int pv_get_force_sock(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;
	
	if (msg->force_send_socket==0)
		return pv_get_null(msg, param, res);

	return pv_get_strval(msg, param, res, &msg->force_send_socket->sock_str);
}

int pv_get_useragent(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL) 
		return -1;
	if(msg->user_agent==NULL && ((parse_headers(msg, HDR_USERAGENT_F, 0)==-1)
			 || (msg->user_agent==NULL)))
	{
		LM_DBG("no User-Agent header\n");
		return pv_get_null(msg, param, res);
	}
	
	return pv_get_strval(msg, param, res, &msg->user_agent->body);
}

int pv_get_refer_to(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(parse_refer_to_header(msg)==-1)
	{
		LM_DBG("no Refer-To header\n");
		return pv_get_null(msg, param, res);
	}
	
	if(msg->refer_to==NULL || get_refer_to(msg)==NULL)
		return pv_get_null(msg, param, res);

	return pv_get_strval(msg, param, res, &(get_refer_to(msg)->uri));
}

int pv_get_diversion(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str *val;
	str name;

	if(msg == NULL)
		return -1;

	if(parse_diversion_header(msg) == -1)
	{
		LM_DBG("no Diversion header\n");
		return pv_get_null(msg, param, res);
	}
	
	if(msg->diversion == NULL || get_diversion(msg) == NULL)
	{
		LM_DBG("no Diversion header\n");
		return pv_get_null(msg, param, res);
	}

	if(param->pvn.u.isname.name.n == 1)  { /* uri */
		return pv_get_strval(msg, param, res, &(get_diversion(msg)->uri));
	}

	if(param->pvn.u.isname.name.n == 2)  { /* reason param */
	    name.s = "reason";
	    name.len = 6;
	    val = get_diversion_param(msg, &name);
	    if (val) {
			return pv_get_strval(msg, param, res, val);
	    } else {
			return pv_get_null(msg, param, res);
	    }
	}

	if(param->pvn.u.isname.name.n == 3)  { /* privacy param */
	    name.s = "privacy";
	    name.len = 7;
	    val = get_diversion_param(msg, &name);
	    if (val) {
			return pv_get_strval(msg, param, res, val);
	    } else {
			return pv_get_null(msg, param, res);
	    }
	}

	LM_ERR("unknown diversion specifier\n");
	return pv_get_null(msg, param, res);
}

int pv_get_rpid(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL)
		return -1;

	if(parse_rpid_header(msg)==-1)
	{
		LM_DBG("no RPID header\n");
		return pv_get_null(msg, param, res);
	}
	
	if(msg->rpid==NULL || get_rpid(msg)==NULL)
		return pv_get_null(msg, param, res);

	return pv_get_strval(msg, param, res, &(get_rpid(msg)->uri));
}

int pv_get_ppi_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
    struct sip_uri *uri;
    
    if(msg==NULL)
	return -1;

    if(parse_ppi_header(msg) < 0) {
	LM_DBG("no P-Preferred-Identity header\n");
	return pv_get_null(msg, param, res);
    }
	
    if(msg->ppi == NULL || get_ppi(msg) == NULL) {
	       LM_DBG("no P-Preferred-Identity header\n");
		return pv_get_null(msg, param, res);
    }
    
    if(param->pvn.u.isname.name.n == 1) { /* uri */
		return pv_get_strval(msg, param, res, &(get_ppi(msg)->uri));
    }
	
    if(param->pvn.u.isname.name.n==4) { /* display name */
		if(get_ppi(msg)->display.s == NULL ||
				get_ppi(msg)->display.len <= 0) {
		    LM_DBG("no P-Preferred-Identity display name\n");
			return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &(get_ppi(msg)->display));
    }

    if((uri=parse_ppi_uri(msg))==NULL) {
		LM_ERR("cannot parse P-Preferred-Identity URI\n");
		return pv_get_null(msg, param, res);
    }

    if(param->pvn.u.isname.name.n==2) { /* username */
		if(uri->user.s==NULL || uri->user.len<=0) {
		    LM_DBG("no P-Preferred-Identity username\n");
		    return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &uri->user);
    } else if(param->pvn.u.isname.name.n==3) { /* domain */
		if(uri->host.s==NULL || uri->host.len<=0) {
			LM_DBG("no P-Preferred-Identity domain\n");
			return pv_get_null(msg, param, res);
		}
		return pv_get_strval(msg, param, res, &uri->host);
    }

	LM_ERR("unknown specifier\n");
	return pv_get_null(msg, param, res);
}


int pv_get_pai(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
    if(msg==NULL)
		return -1;
    
    if(parse_pai_header(msg)==-1)
    {
		LM_DBG("no P-Asserted-Identity header\n");
		return pv_get_null(msg, param, res);
    }
	
    if(msg->pai==NULL || get_pai(msg)==NULL) {
		LM_DBG("no P-Asserted-Identity header\n");
		return pv_get_null(msg, param, res);
    }
    
	return pv_get_strval(msg, param, res, &(get_pai(msg)->uri));
}

/* proto of received message: $pr or $proto*/
int pv_get_proto(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(msg==NULL)
		return -1;

	switch(msg->rcv.proto)
	{
		case PROTO_UDP:
			s.s = "udp";
			s.len = 3;
		break;
		case PROTO_TCP:
			s.s = "tcp";
			s.len = 3;
		break;
		case PROTO_TLS:
			s.s = "tls";
			s.len = 3;
		break;
		case PROTO_SCTP:
			s.s = "sctp";
			s.len = 4;
		break;
		default:
			s.s = "NONE";
			s.len = 4;
	}

	return pv_get_strintval(msg, param, res, &s, (int)msg->rcv.proto);
}

int pv_get_dset(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
    if(msg==NULL)
		return -1;
    
    s.s = print_dset(msg, &s.len);
    if (s.s == NULL)
		return pv_get_null(msg, param, res);
    s.len -= CRLF_LEN;
	return pv_get_strval(msg, param, res, &s);
}

int pv_get_dsturi(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
    if(msg==NULL)
		return -1;
    
    if (msg->dst_uri.s == NULL) {
		LM_DBG("no destination URI\n");
		return pv_get_null(msg, param, res);
    }

	return pv_get_strval(msg, param, res, &msg->dst_uri);
}

int pv_get_dsturi_attr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	struct sip_uri uri;

	if(msg==NULL)
		return -1;
    
	if (msg->dst_uri.s == NULL) {
		LM_DBG("no destination URI\n");
		return pv_get_null(msg, param, res);
	}

	if(parse_uri(msg->dst_uri.s, msg->dst_uri.len, &uri)!=0)
	{
		LM_ERR("failed to parse dst uri\n");
		return pv_get_null(msg, param, res);
	}
	
	if(param->pvn.u.isname.name.n==1) /* domain */
	{
		if(uri.host.s==NULL || uri.host.len<=0)
			return pv_get_null(msg, param, res);
		return pv_get_strval(msg, param, res, &uri.host);
	} else if(param->pvn.u.isname.name.n==2) /* port */ {
		if(uri.port.s==NULL)
			return pv_get_5060(msg, param, res);
		return pv_get_strintval(msg, param, res, &uri.port, (int)uri.port_no);
	} else if(param->pvn.u.isname.name.n==3) /* proto */ {
		if(uri.transport_val.s==NULL)
			return pv_get_udp(msg, param, res);
		return pv_get_strintval(msg, param, res, &uri.transport_val,
				(int)uri.proto);
	}

	LM_ERR("invalid specifier\n");
	return pv_get_null(msg, param, res);
}

int pv_get_content_type(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL) 
		return -1;

	if(msg->content_type==NULL
			&& ((parse_headers(msg, HDR_CONTENTTYPE_F, 0)==-1)
			 || (msg->content_type==NULL)))
	{
		LM_DBG("no Content-Type header\n");
		return pv_get_null(msg, param, res);
	}
	
	return pv_get_strval(msg, param, res, &msg->content_type->body);
}


int pv_get_content_length(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	if(msg==NULL) 
		return -1;
	if(msg->content_length==NULL
			&& ((parse_headers(msg, HDR_CONTENTLENGTH_F, 0)==-1)
			 || (msg->content_length==NULL)))
	{
		LM_DBG("no Content-Length header\n");
		return pv_get_null(msg, param, res);
	}
	
	return pv_get_intstrval(msg, param, res,
			(int)(long)msg->content_length->parsed,
			&msg->content_length->body);
}

int pv_get_msg_body(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
	if(msg==NULL)
		return -1;

	s.s = get_body( msg );

	if(s.s == NULL)
	{
		LM_DBG("no message body\n");
		return pv_get_null(msg, param, res);
	}    
	s.len = msg->buf + msg->len - s.s;

	return pv_get_strval(msg, param, res, &s);
}


int pv_get_body_size(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str s;
    if(msg==NULL)
		return -1;
    
	s.s = get_body( msg );

	s.len = 0;
	if (s.s != NULL)
		s.len = msg->buf + msg->len - s.s;
	return pv_get_sintval(msg, param, res, s.len);
}


int pv_get_authattr(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	struct hdr_field *hdr;
	
    if(msg==NULL)
		return -1;
    
	if ((msg->REQ_METHOD == METHOD_ACK) || 
			(msg->REQ_METHOD == METHOD_CANCEL)) {
		LM_DBG("no [Proxy-]Authorization header\n");
		return pv_get_null(msg, param, res);
	}

	if ((parse_headers(msg, HDR_PROXYAUTH_F|HDR_AUTHORIZATION_F, 0)==-1)
			|| (msg->proxy_auth==0 && msg->authorization==0))
	{
		LM_DBG("no [Proxy-]Authorization header\n");
		return pv_get_null(msg, param, res);
	}

	hdr = (msg->proxy_auth==0)?msg->authorization:msg->proxy_auth;
	
	if(parse_credentials(hdr)!=0) {
	        LM_ERR("failed to parse credentials\n");
		return pv_get_null(msg, param, res);
	}
	switch(param->pvn.u.isname.name.n)
	{
		case 4:
			return pv_get_strval(msg, param, res,
					&((auth_body_t*)(hdr->parsed))->digest.username.domain);
		case 3:
			if(((auth_body_t*)(hdr->parsed))->digest.uri.len==0)
				return pv_get_null(msg, param, res);
			return pv_get_strval(msg, param, res,
					&((auth_body_t*)(hdr->parsed))->digest.uri);
		break;
		case 2:
			return pv_get_strval(msg, param, res,
					&((auth_body_t*)(hdr->parsed))->digest.realm);
		break;
		case 1:
			return pv_get_strval(msg, param, res,
					&((auth_body_t*)(hdr->parsed))->digest.username.user);
		break;
		default:
			return pv_get_strval(msg, param, res,
					&((auth_body_t*)(hdr->parsed))->digest.username.whole);
	}	    
}

static inline str *cred_user(struct sip_msg *rq)
{
	struct hdr_field* h;
	auth_body_t* cred;

	get_authorized_cred(rq->proxy_auth, &h);
	if (!h) get_authorized_cred(rq->authorization, &h);
	if (!h) return 0;
	cred=(auth_body_t*)(h->parsed);
	if (!cred || !cred->digest.username.user.len) 
			return 0;
	return &cred->digest.username.user;
}


static inline str *cred_realm(struct sip_msg *rq)
{
	str* realm;
	struct hdr_field* h;
	auth_body_t* cred;

	get_authorized_cred(rq->proxy_auth, &h);
	if (!h) get_authorized_cred(rq->authorization, &h);
	if (!h) return 0;
	cred=(auth_body_t*)(h->parsed);
	if (!cred) return 0;
	realm = GET_REALM(&cred->digest);
	if (!realm->len || !realm->s) {
		return 0;
	}
	return realm;
}

int pv_get_acc_username(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	static char buf[MAX_URI_SIZE];
	str* user;
	str* realm;
	struct sip_uri puri;
	struct to_body* from;
	str s;

	/* try to take it from credentials */
	user = cred_user(msg);
	if (user) {
		realm = cred_realm(msg);
		if (realm) {
			s.len = user->len+1+realm->len;
			if (s.len > MAX_URI_SIZE) {
				LM_ERR("uri too long\n");
				return pv_get_null(msg, param, res);
			}
			s.s = buf;
			memcpy(s.s, user->s, user->len);
			(s.s)[user->len] = '@';
			memcpy(s.s+user->len+1, realm->s, realm->len);
			return pv_get_strval(msg, param, res, &s);
		}
		return pv_get_strval(msg, param, res, user);
	}
		
	/* from from uri */
	if(parse_from_header(msg)<0)
	{
		LM_ERR("cannot parse FROM header\n");
		return pv_get_null(msg, param, res);
	}
	if (msg->from && (from=get_from(msg)) && from->uri.len) {
		if (parse_uri(from->uri.s, from->uri.len, &puri) < 0 ) {
			LM_ERR("bad From URI\n");
			return pv_get_null(msg, param, res);
		}
		s.len = puri.user.len + 1 + puri.host.len;
		if (s.len > MAX_URI_SIZE) {
			LM_ERR("from URI too long\n");
			return pv_get_null(msg, param, res);
		}
		s.s = buf;
		memcpy(s.s, puri.user.s, puri.user.len);
		(s.s)[puri.user.len] = '@';
		memcpy(s.s + puri.user.len + 1, puri.host.s, puri.host.len);
	} else {
		s.len = 0;
		s.s = 0;
	}
	return pv_get_strval(msg, param, res, &s);
}

int pv_get_branch(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str branch;
	qvalue_t q;

	if(msg==NULL || res==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)
		return pv_get_null(msg, param, res);


	branch.s = get_branch(0, &branch.len, &q, 0, 0, 0, 0);
	if (!branch.s) {
		return pv_get_null(msg, param, res);
	}
	
	return pv_get_strval(msg, param, res, &branch);
}

#define Q_PARAM ">;q="
#define Q_PARAM_LEN (sizeof(Q_PARAM) - 1)

int pv_get_branches(struct sip_msg *msg, pv_param_t *param,
		pv_value_t *res)
{
	str uri;
	str s;
	qvalue_t q;
	int cnt, i;
	unsigned int qlen;
	char *p, *qbuf;

	if(msg==NULL || res==NULL)
		return -1;

	if(msg->first_line.type == SIP_REPLY)
		return pv_get_null(msg, param, res);
  
	cnt = s.len = 0;

	while ((uri.s = get_branch(cnt, &uri.len, &q, 0, 0, 0, 0)))
	{
		cnt++;
		s.len += uri.len;
		if (q != Q_UNSPECIFIED)
		{
			s.len += 1 + Q_PARAM_LEN + len_q(q);
		}
	}

	if (cnt == 0)
		return pv_get_null(msg, param, res);   

	s.len += (cnt - 1) * PV_FIELD_DELIM_LEN;

	if (s.len + 1 > PV_LOCAL_BUF_SIZE)
	{
		LM_ERR("local buffer length exceeded\n");
		return pv_get_null(msg, param, res);
	}

	i = 0;
	p = pv_local_buf;

	while ((uri.s = get_branch(i, &uri.len, &q, 0, 0, 0, 0)))
	{
		if (i)
		{
			memcpy(p, PV_FIELD_DELIM, PV_FIELD_DELIM_LEN);
			p += PV_FIELD_DELIM_LEN;
		}

		if (q != Q_UNSPECIFIED)
		{
			*p++ = '<';
		}

		memcpy(p, uri.s, uri.len);
		p += uri.len;
		if (q != Q_UNSPECIFIED)
		{
			memcpy(p, Q_PARAM, Q_PARAM_LEN);
			p += Q_PARAM_LEN;

			qbuf = q2str(q, &qlen);
			memcpy(p, qbuf, qlen);
			p += qlen;
		}
		i++;
	}

	s.s = &(pv_local_buf[0]);
	return pv_get_strval(msg, param, res, &s);
}

int pv_get_avp(struct sip_msg *msg,  pv_param_t *param, pv_value_t *res)
{
	unsigned short name_type;
	int_str avp_name;
	int_str avp_value;
	struct usr_avp *avp;
	int_str avp_value0;
	struct usr_avp *avp0;
	int idx;
	int idxf;
	char *p;
	int n=0;
	struct search_state state;

	if(msg==NULL || res==NULL || param==NULL)
		return -1;

	/* get the name */
	if(pv_get_avp_name(msg, param, &avp_name, &name_type)!=0)
	{
		LM_ERR("invalid name\n");
		return -1;
	}
	/* get the index */
	if(pv_get_spec_index(msg, param, &idx, &idxf)!=0)
	{
		LM_ERR("invalid index\n");
		return -1;
	}
	
	memset(&state, 0, sizeof(struct search_state));
	if ((avp=search_first_avp(name_type, avp_name, &avp_value, &state))==0)
		return pv_get_null(msg, param, res);
	res->flags = PV_VAL_STR;
	if(idxf==0 && idx==0)
	{
		if(avp->flags & AVP_VAL_STR)
		{
			res->rs = avp_value.s;
		} else {
			res->rs.s = int2str(avp_value.n, &res->rs.len);
			res->ri = avp_value.n;
			res->flags |= PV_VAL_INT|PV_TYPE_INT;
		}
		return 0;
	}
	if(idxf==PV_IDX_ALL)
	{
		p = pv_local_buf;
		do {
			if(p!=pv_local_buf)
			{
				if(p-pv_local_buf+PV_FIELD_DELIM_LEN+1>PV_LOCAL_BUF_SIZE)
				{
					LM_ERR("local buffer length exceeded\n");
					return pv_get_null(msg, param, res);
				}
				memcpy(p, PV_FIELD_DELIM, PV_FIELD_DELIM_LEN);
				p += PV_FIELD_DELIM_LEN;
			}
			if(avp->flags & AVP_VAL_STR)
			{
				res->rs = avp_value.s;
			} else {
				res->rs.s = int2str(avp_value.n, &res->rs.len);
			}
			
			if(p-pv_local_buf+res->rs.len+1>PV_LOCAL_BUF_SIZE)
			{
				LM_ERR("local buffer length exceeded!\n");
				return pv_get_null(msg, param, res);
			}
			memcpy(p, res->rs.s, res->rs.len);
			p += res->rs.len;
		} while ((avp=search_next_avp(&state, &avp_value))!=0);
		*p = 0;
		res->rs.s = pv_local_buf;
		res->rs.len = p - pv_local_buf;
		return 0;
	}

	/* we have a numeric index */
	if(idx<0)
	{
		n = 1;
		avp0 = avp;
		while ((avp0=search_next_avp(&state, &avp_value0))!=0) n++;
		idx = -idx;
		if(idx>n)
		{
			LM_DBG("index out of range\n");
			return pv_get_null(msg, param, res);
		}
		idx = n - idx;
		if(idx==0)
		{
			if(avp->flags & AVP_VAL_STR)
			{
				res->rs = avp_value.s;
			} else {
				res->rs.s = int2str(avp_value.n, &res->rs.len);
				res->ri = avp_value.n;
				res->flags |= PV_VAL_INT|PV_TYPE_INT;
			}
			return 0;
		}
	}
	n=0;
	while(n<idx 
		  && (avp=search_next_avp(&state, &avp_value))!=0)
		n++;

	if(avp!=0)
	{
		if(avp->flags & AVP_VAL_STR)
		{
			res->rs = avp_value.s;
		} else {
			res->rs.s = int2str(avp_value.n, &res->rs.len);
			res->ri = avp_value.n;
			res->flags |= PV_VAL_INT|PV_TYPE_INT;
		}
		return 0;
	}

	LM_DBG("index out of range\n");
	return pv_get_null(msg, param, res);
}

int pv_get_hdr(struct sip_msg *msg,  pv_param_t *param, pv_value_t *res)
{
	int idx;
	int idxf;
	pv_value_t tv;
	struct hdr_field *hf;
	struct hdr_field *hf0;
	char *p;
	int n;

	if(msg==NULL || res==NULL || param==NULL)
		return -1;

	/* get the name */
	if(param->pvn.type == PV_NAME_PVAR)
	{
		if(pv_get_spec_name(msg, param, &tv)!=0 || (!(tv.flags&PV_VAL_STR)))
		{
			LM_ERR("invalid name\n");
			return -1;
		}
	} else {
		if(param->pvn.u.isname.type == AVP_NAME_STR)
		{
			tv.flags = PV_VAL_STR;
			tv.rs = param->pvn.u.isname.name.s;
		} else {
			tv.flags = 0;
			tv.ri = param->pvn.u.isname.name.n;
		}
	}
	/* we need to be sure we have parsed all headers */
	if(parse_headers(msg, HDR_EOH_F, 0)<0)
	{
		LM_ERR("error parsing headers\n");
		return pv_get_null(msg, param, res);
	}

	for (hf=msg->headers; hf; hf=hf->next)
	{
		if(tv.flags == 0)
		{
			if (tv.ri==hf->type)
				break;
		} else {
			if (cmp_hdrname_str(&hf->name, &tv.rs)==0)
				break;
		}
	}
	if(hf==NULL)
		return pv_get_null(msg, param, res);
	/* get the index */
	if(pv_get_spec_index(msg, param, &idx, &idxf)!=0)
	{
		LM_ERR("invalid index\n");
		return -1;
	}

	/* get the value */
	res->flags = PV_VAL_STR;
	if(idxf==0 && idx==0)
	{
		res->rs  = hf->body;
		return 0;
	}
	if(idxf==PV_IDX_ALL)
	{
		p = pv_local_buf;
		do {
			if(p!=pv_local_buf)
			{
				if(p-pv_local_buf+PV_FIELD_DELIM_LEN+1>PV_LOCAL_BUF_SIZE)
				{
					LM_ERR("local buffer length exceeded\n");
					return pv_get_null(msg, param, res);
				}
				memcpy(p, PV_FIELD_DELIM, PV_FIELD_DELIM_LEN);
				p += PV_FIELD_DELIM_LEN;
			}
			if(p-pv_local_buf+hf->body.len+1>PV_LOCAL_BUF_SIZE)
			{
				LM_ERR("local buffer length exceeded [%d/%d]!\n",
						(int)(p-pv_local_buf+hf->body.len+1),
						hf->body.len);
				return pv_get_null(msg, param, res);
			}
			memcpy(p, hf->body.s, hf->body.len);
			p += hf->body.len;
			/* next hf */
			for (hf=hf->next; hf; hf=hf->next)
			{
				if(tv.flags == 0)
				{
					if (tv.ri==hf->type)
						break;
				} else {
					if (cmp_hdrname_str(&hf->name, &tv.rs)==0)
					break;
				}
			}
		} while (hf);
		*p = 0;
		res->rs.s = pv_local_buf;
		res->rs.len = p - pv_local_buf;
		return 0;
	}

	/* we have a numeric index */
	hf0 = 0;
	if(idx<0)
	{
		n = 1;
		/* count headers */
		for (hf0=hf->next; hf0; hf0=hf0->next)
		{
			if(tv.flags == 0)
			{
				if (tv.ri==hf0->type)
					n++;
			} else {
				if (cmp_hdrname_str(&hf0->name, &tv.rs)==0)
					n++;
			}
		}
		idx = -idx;
		if(idx>n)
		{
			LM_DBG("index out of range\n");
			return pv_get_null(msg, param, res);
		}
		idx = n - idx;
		if(idx==0)
		{
			res->rs  = hf->body;
			return 0;
		}
	}
	n=0;
	while(n<idx)
	{
		for (hf0=hf->next; hf0; hf0=hf0->next)
		{
			if(tv.flags == 0)
			{
				if (tv.ri==hf0->type)
					n++;
			} else {
				if (cmp_hdrname_str(&hf0->name, &tv.rs)==0)
					n++;
			}
			if(n==idx)
				break;
		}
		if(hf0==NULL)
			break;
	}

	if(hf0!=0)
	{
		res->rs  = hf0->body;
		return 0;
	}

	LM_DBG("index out of range\n");
	return pv_get_null(msg, param, res);

}

int pv_get_scriptvar(struct sip_msg *msg,  pv_param_t *param,
		pv_value_t *res)
{
	int ival = 0;
	char *sval = NULL;
	script_var_t *sv=NULL;
	
	if(msg==NULL || res==NULL)
		return -1;

	if(param==NULL || param->pvn.u.dname==0)
		return pv_get_null(msg, param, res);
	
	sv= (script_var_t*)param->pvn.u.dname;

	if(sv->v.flags&VAR_VAL_STR)
	{
		res->rs = sv->v.value.s;
		res->flags = PV_VAL_STR;
	} else {
		sval = sint2str(sv->v.value.n, &ival);

		res->rs.s = sval;
		res->rs.len = ival;

		res->ri = sv->v.value.n;
		res->flags = PV_VAL_STR|PV_VAL_INT|PV_TYPE_INT;
	}
	return 0;
}
/********* end PV get functions *********/

/********* start PV set functions *********/
int pv_set_avp(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	int_str avp_name;
	int_str avp_val;
	int flags;
	unsigned short name_type;
	int idxf;
	int idx;
	
	if(param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	/* get the name */
	if(pv_get_avp_name(msg, param, &avp_name, &name_type)!=0)
	{
		LM_ALERT("BUG in getting dst AVP name\n");
		goto error;
	}
	/* get the index */
	if(pv_get_spec_index(msg, param, &idx, &idxf)!=0)
	{
		LM_ERR("invalid index\n");
		return -1;
	}

	if((val==NULL) || (val->flags&PV_VAL_NULL))
	{
		if(idxf == PV_IDX_ALL)
			destroy_avps(name_type, avp_name, 1);
		else
			destroy_avps(name_type, avp_name, 0);
		return 0;
	}
	if(idxf == PV_IDX_ALL)
		destroy_avps(name_type, avp_name, 1);
	flags = name_type;
	if(val->flags&PV_TYPE_INT)
	{
		avp_val.n = val->ri;
	} else {
		avp_val.s = val->rs;
		flags |= AVP_VAL_STR;
	}
	if (add_avp(flags, avp_name, avp_val)<0)
	{
		LM_ERR("error - cannot add AVP\n");
		goto error;
	}
	return 0;
error:
	return -1;
}

int pv_set_scriptvar(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	int_str avp_val;
	int flags;

	if(param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	if(param->pvn.u.dname==0)
	{
		LM_ERR("error - cannot find svar\n");
		goto error;
	}
	if((val==NULL) || (val->flags&PV_VAL_NULL))
	{
		avp_val.n = 0;
		set_var_value((script_var_t*)param->pvn.u.dname, &avp_val, 0);
		return 0;
	}
	flags = 0;
	if(val->flags&PV_TYPE_INT)
	{
		avp_val.n = val->ri;
	} else {
		avp_val.s = val->rs;
		flags |= VAR_VAL_STR;
	}
	if(set_var_value((script_var_t*)param->pvn.u.dname, &avp_val, flags)==NULL)
	{
		LM_ERR("error - cannot set svar [%.*s] \n",
				((script_var_t*)param->pvn.u.dname)->name.len,
				((script_var_t*)param->pvn.u.dname)->name.s);
		goto error;
	}
	return 0;
error:
	return -1;
}

int pv_set_dsturi(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if((val==NULL) || (val->flags&PV_VAL_NULL))
	{
		reset_dst_uri(msg);
		return 1;
	}
	if(!(val->flags&PV_VAL_STR))
	{
		LM_ERR("error - str value requred to set dst uri\n");
		goto error;
	}
	
	if(set_dst_uri(msg, &val->rs)!=0)
		goto error;
	/* dst_uri changed, so it makes sense to re-use the current uri for
		forking */
	ruri_mark_new(); /* re-use uri for serial forking */

	return 0;
error:
	return -1;
}

int pv_set_ruri(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	struct action  act;
	struct run_act_ctx h;
	char backup;

	if(msg==NULL || param==NULL || val==NULL || (val->flags&PV_VAL_NULL))
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	if(!(val->flags&PV_VAL_STR))
	{
		LM_ERR("str value required to set R-URI\n");
		goto error;
	}
	
	memset(&act, 0, sizeof(act));
	act.val[0].type = STRING_ST;
	act.val[0].u.string = val->rs.s;
	backup = val->rs.s[val->rs.len];
	val->rs.s[val->rs.len] = '\0';
	act.type = SET_URI_T;
	init_run_actions_ctx(&h);
	if (do_action(&h, &act, msg)<0)
	{
		LM_ERR("do action failed\n");
		val->rs.s[val->rs.len] = backup;
		goto error;
	}
	val->rs.s[val->rs.len] = backup;

	return 0;
error:
	return -1;
}

int pv_set_ruri_user(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	struct action  act;
	struct run_act_ctx h;
	char backup;

	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if((val==NULL) || (val->flags&PV_VAL_NULL))
	{
		memset(&act, 0, sizeof(act));
		act.type = SET_USER_T;
		act.val[0].type = STRING_ST;
		act.val[0].u.string = "";
		init_run_actions_ctx(&h);
		if (do_action(&h, &act, msg)<0)
		{
			LM_ERR("do action failed)\n");
			goto error;
		}
		return 0;
	}

	if(!(val->flags&PV_VAL_STR))
	{
		LM_ERR("str value required to set R-URI user\n");
		goto error;
	}
	
	memset(&act, 0, sizeof(act));
	act.val[0].type = STRING_ST;
	act.val[0].u.string = val->rs.s;
	backup = val->rs.s[val->rs.len];
	val->rs.s[val->rs.len] = '\0';
	act.type = SET_USER_T;
	init_run_actions_ctx(&h);
	if (do_action(&h, &act, msg)<0)
	{
		LM_ERR("do action failed\n");
		val->rs.s[val->rs.len] = backup;
		goto error;
	}
	val->rs.s[val->rs.len] = backup;

	return 0;
error:
	return -1;
}

int pv_set_ruri_host(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	struct action  act;
	struct run_act_ctx h;
	char backup;

	if(msg==NULL || param==NULL || val==NULL || (val->flags&PV_VAL_NULL))
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	if(!(val->flags&PV_VAL_STR))
	{
		LM_ERR("str value required to set R-URI hostname\n");
		goto error;
	}
	
	memset(&act, 0, sizeof(act));
	act.val[0].type = STRING_ST;
	act.val[0].u.string = val->rs.s;
	backup = val->rs.s[val->rs.len];
	val->rs.s[val->rs.len] = '\0';
	act.type = SET_HOST_T;
	init_run_actions_ctx(&h);
	if (do_action(&h, &act, msg)<0)
	{
		LM_ERR("do action failed\n");
		val->rs.s[val->rs.len] = backup;
		goto error;
	}
	val->rs.s[val->rs.len] = backup;

	return 0;
error:
	return -1;
}

int pv_set_ruri_port(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	struct action  act;
	struct run_act_ctx h;
	char backup;

	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if(val == NULL || (val->flags&PV_VAL_NULL))
	{
		memset(&act, 0, sizeof(act));
		act.type = SET_PORT_T;
		act.val[0].type = STRING_ST;
		act.val[0].u.string = "";
		init_run_actions_ctx(&h);
		if (do_action(&h, &act, msg)<0)
		{
			LM_ERR("do action failed)\n");
			goto error;
		}
		return 0;
	}

	if(!(val->flags&PV_VAL_STR))
	{
		val->rs.s = int2str(val->ri, &val->rs.len);
		val->flags |= PV_VAL_STR;
	}
	
	memset(&act, 0, sizeof(act));
	act.val[0].type = STRING_ST;
	act.val[0].u.string = val->rs.s;
	backup = val->rs.s[val->rs.len];
	val->rs.s[val->rs.len] = '\0';
	act.type = SET_PORT_T;
	init_run_actions_ctx(&h);
	if (do_action(&h, &act, msg)<0)
	{
		LM_ERR("do action failed\n");
		val->rs.s[val->rs.len] = backup;
		goto error;
	}
	val->rs.s[val->rs.len] = backup;

	return 0;
error:
	return -1;
}

int pv_set_branch(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	if(msg==NULL || param==NULL || val==NULL || (val->flags&PV_VAL_NULL))
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	if(!(val->flags&PV_VAL_STR) || val->rs.len<=0)
	{
		LM_ERR("str value required to set the branch\n");
		goto error;
	}
	
	if (km_append_branch( msg, &val->rs, 0, 0, Q_UNSPECIFIED, 0,
			msg->force_send_socket)!=1 )
	{
		LM_ERR("append_branch action failed\n");
		goto error;
	}

	return 0;
error:
	return -1;
}

int pv_set_force_sock(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	struct socket_info *si;
	int port, proto;
	str host;
	char backup;
	
	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}

	if(val==NULL || (val->flags&PV_VAL_NULL))
	{
		reset_force_socket(msg);
		return 0;
	}

	if(!(val->flags&PV_VAL_STR) || val->rs.len<=0)
	{
		LM_ERR("str value required to set the force send sock\n");
		goto error;
	}
	
	backup = val->rs.s[val->rs.len];
	val->rs.s[val->rs.len] = '\0';
	if (parse_phostport(val->rs.s, &host.s, &host.len, &port, &proto) < 0)
	{
		LM_ERR("invalid socket specification\n");
		val->rs.s[val->rs.len] = backup;
		goto error;
	}
	val->rs.s[val->rs.len] = backup;
	si = grep_sock_info(&host, (unsigned short)port, (unsigned short)proto);
	if (si!=NULL)
	{
		set_force_socket(msg, si);
	} else {
		LM_WARN("no socket found to match [%.*s]\n",
				val->rs.len, val->rs.s);
	}

	return 0;
error:
	return -1;
}

int pv_set_mflags(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if(val == NULL || (val->flags&PV_VAL_NULL))
	{
		msg->flags = 0;
		return 0;
	}

	if(!(val->flags&PV_VAL_INT))
	{
		LM_ERR("assigning non-int value to msg flags\n");
		return -1;
	}
	
	msg->flags = val->ri;

	return 0;
}

int pv_set_sflags(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if(val == NULL || (val->flags&PV_VAL_NULL))
	{
		setsflagsval(0);
		return 0;
	}

	if(!(val->flags&PV_VAL_INT))
	{
		LM_ERR("assigning non-int value to script flags\n");
		return -1;
	}
	
	setsflagsval((unsigned int)val->ri);

	return 0;
}


int pv_set_bflags(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	if(val == NULL || (val->flags&PV_VAL_NULL))
	{
		setbflagsval(0, 0);
		return 0;
	}

	if(!(val->flags&PV_VAL_INT))
	{
		LM_ERR("assigning non-int value to branch 0 flags\n");
		return -1;
	}
	
	setbflagsval(0, (flag_t)val->ri);

	return 0;
}

int pv_set_xto_attr(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val, struct to_body *tb, int type)
{
	str buf = {0, 0};
	struct lump *l = NULL;
	int loffset = 0;
	int llen = 0;

	if(msg==NULL || param==NULL)
	{
		LM_ERR("bad parameters\n");
		return -1;
	}
					
	switch(type)
	{
		case 0: /* uri */
			if(val == NULL || (val->flags&PV_VAL_NULL))
			{
				LM_WARN("To header URI cannot be deleted\n");
				return 0;
			}
			if(!(val->flags&PV_VAL_STR))
			{
				LM_ERR("attempt to assign non-str value to To header URI\n");
				return -1;
			}

			buf.s = pkg_malloc(val->rs.len);
			if (buf.s==0)
			{
				LM_ERR("no more pkg mem\n");
				goto error;
			}
			buf.len = val->rs.len;
			memcpy(buf.s, val->rs.s, val->rs.len);
			loffset = tb->uri.s - msg->buf;
			llen    = tb->uri.len;
		break;
		case 1: /* username */
			if(val == NULL || (val->flags&PV_VAL_NULL))
			{
				if(tb->parsed_uri.user.len==0)
					return 0; /* nothing to delete */
				/* delete username */
				loffset = tb->parsed_uri.user.s - msg->buf;
				llen    = tb->parsed_uri.user.len;
				/* delete '@' after */
				if(tb->parsed_uri.user.s[tb->parsed_uri.user.len]=='@')
					llen++;
				break;
			}
			if(!(val->flags&PV_VAL_STR))
			{
				LM_ERR("attempt to assign non-str value to To header"
						" display name\n");
				return -1;
			}
			buf.s = pkg_malloc(val->rs.len+1);
			if (buf.s==0)
			{
				LM_ERR("no more pkg mem\n");
				goto error;
			}
			buf.len = val->rs.len;
			memcpy(buf.s, val->rs.s, val->rs.len);
			if(tb->parsed_uri.user.len==0)
			{
				l = anchor_lump(msg, tb->parsed_uri.host.s - msg->buf, 0, 0);
				buf.s[buf.len] = '@';
				buf.len++;
			} else {
				/* delete username */
				loffset = tb->parsed_uri.user.s - msg->buf;
				llen    = tb->parsed_uri.user.len;
			}
		break;
		case 2: /* domain */
			if(val == NULL || (val->flags&PV_VAL_NULL))
			{
				LM_WARN("To header URI domain cannot be deleted\n");
				return 0;
			}
			if(!(val->flags&PV_VAL_STR))
			{
				LM_ERR("attempt to assign non-str value to To header"
						" URI domain\n");
				return -1;
			}
			buf.s = pkg_malloc(val->rs.len);
			if (buf.s==0)
			{
				LM_ERR("no more pkg mem\n");
				goto error;
			}
			buf.len = val->rs.len;
			memcpy(buf.s, val->rs.s, val->rs.len);
			loffset = tb->parsed_uri.host.s - msg->buf;
			llen    = tb->parsed_uri.host.len;
		break;
		case 3: /* display */
			if(val == NULL || (val->flags&PV_VAL_NULL))
			{
				if(tb->display.len==0)
					return 0; /* nothing to delete */
				/* delete display */
				loffset = tb->display.s - msg->buf;
				llen    = tb->display.len;
				/* delete whitespace after */
				if(tb->display.s[tb->display.len]==' ')
					llen++;
				break;
			}
			if(!(val->flags&PV_VAL_STR))
			{
				LM_ERR("attempt to assign non-str value to To header"
						" display name\n");
				return -1;
			}
			buf.s = pkg_malloc(val->rs.len+1);
			if (buf.s==0)
			{
				LM_ERR("no more pkg mem\n");
				goto error;
			}
			buf.len = val->rs.len;
			memcpy(buf.s, val->rs.s, val->rs.len);
			if(tb->display.len==0)
			{
				l = anchor_lump(msg, tb->body.s - msg->buf, 0, 0);
				buf.s[buf.len] = ' ';
				buf.len++;
			} else {
				/* delete display */
				loffset = tb->display.s - msg->buf;
				llen    = tb->display.len;
			}
		break;
	}

	/* delete old value */
	if(llen>0)
	{
		if ((l=del_lump(msg, loffset, llen, 0))==0)
		{
			LM_ERR("failed to delete xto attribute %d\n", type);
			goto error;
		}
	}
	/* set new value when given */
	if(l!=NULL && buf.len>0)
	{
		if (insert_new_lump_after(l, buf.s, buf.len, 0)==0)
		{
			LM_ERR("failed to set xto attribute %d\n", type);
			goto error;
		}
	}
	return 0;

error:
	if(buf.s!=0)
		pkg_free(buf.s);
	return -1;
}

int pv_set_to_attr(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val, int type)
{
	if(msg==NULL)
		return -1;

	if(msg->to==NULL && parse_headers(msg, HDR_TO_F, 0)==-1) {
		LM_ERR("cannot parse To header\n");
		return -1;
	}
	if(msg->to==NULL || get_to(msg)==NULL) {
		LM_DBG("no To header\n");
		return -1;
	}
	if(parse_to_uri(msg)==NULL) {
		LM_ERR("cannot parse To header URI\n");
		return -1;
	}
	return pv_set_xto_attr(msg, param, op, val, get_to(msg), type);
}

int pv_set_to_uri(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_to_attr(msg, param, op, val, 0);
}

int pv_set_to_username(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_to_attr(msg, param, op, val, 1);
}

int pv_set_to_domain(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_to_attr(msg, param, op, val, 2);
}

int pv_set_to_display(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_to_attr(msg, param, op, val, 3);
}

int pv_set_from_attr(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val, int type)
{
	if(msg==NULL)
		return -1;

	if(parse_from_header(msg)<0)
	{
		LM_ERR("failed to parse From header\n");
		return -1;
	}
	if(parse_from_uri(msg)==NULL)
	{
		LM_ERR("cannot parse From header URI\n");
		return -1;
	}
	return pv_set_xto_attr(msg, param, op, val, get_from(msg), type);
}

int pv_set_from_uri(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_from_attr(msg, param, op, val, 0);
}

int pv_set_from_username(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_from_attr(msg, param, op, val, 1);
}

int pv_set_from_domain(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_from_attr(msg, param, op, val, 2);
}

int pv_set_from_display(struct sip_msg* msg, pv_param_t *param,
		int op, pv_value_t *val)
{
	return pv_set_from_attr(msg, param, op, val, 3);
}

/********* end PV set functions *********/

int pv_parse_scriptvar_name(pv_spec_p sp, str *in)
{
	if(in==NULL || in->s==NULL || sp==NULL)
		return -1;
	
	sp->pvp.pvn.type = PV_NAME_PVAR;
	sp->pvp.pvn.u.dname = (void*)add_var(in);
	if(sp->pvp.pvn.u.dname==NULL)
	{
		LM_ERR("cannot register var [%.*s]\n", in->len, in->s);
		return -1;
	}
	return 0;
}

int pv_parse_hdr_name(pv_spec_p sp, str *in)
{
	str s;
	char *p;
	pv_spec_p nsp = 0;
	struct hdr_field hdr;

	if(in==NULL || in->s==NULL || sp==NULL)
		return -1;
				
	p = in->s;
	if(*p==PV_MARKER)
	{
		nsp = (pv_spec_p)pkg_malloc(sizeof(pv_spec_t));
		if(nsp==NULL)
		{
			LM_ERR("no more memory\n");
			return -1;
		}
		p = pv_parse_spec(in, nsp);
		if(p==NULL)
		{
			LM_ERR("invalid name [%.*s]\n", in->len, in->s);
			pv_spec_free(nsp);
			return -1;
		}
		//LM_ERR("dynamic name [%.*s]\n", in->len, in->s);
		//pv_print_spec(nsp);
		sp->pvp.pvn.type = PV_NAME_PVAR;
		sp->pvp.pvn.u.dname = (void*)nsp;
		return 0;
	}

	if(in->len>=PV_LOCAL_BUF_SIZE-1)
	{
		LM_ERR("name too long\n");
		return -1;
	}
	memcpy(pv_local_buf, in->s, in->len);
	pv_local_buf[in->len] = ':';
	s.s = pv_local_buf;
	s.len = in->len+1;

	if (parse_hname2(s.s, s.s + ((s.len<4)?4:s.len), &hdr)==0)
	{
		LM_ERR("error parsing header name [%.*s]\n", s.len, s.s);
		goto error;
	}
	sp->pvp.pvn.type = PV_NAME_INTSTR;
	if (hdr.type!=HDR_OTHER_T && hdr.type!=HDR_ERROR_T)
	{
		LM_DBG("using hdr type (%d) instead of <%.*s>\n",
			hdr.type, in->len, in->s);
		sp->pvp.pvn.u.isname.type = 0;
		sp->pvp.pvn.u.isname.name.n = hdr.type;
	} else {
		sp->pvp.pvn.u.isname.type = AVP_NAME_STR;
		sp->pvp.pvn.u.isname.name.s = *in;
	}
	return 0;
error:
	return -1;
}

