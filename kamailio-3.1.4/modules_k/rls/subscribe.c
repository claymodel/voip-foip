/*
 * $Id: subscribe.c 2230 2007-06-06 07:13:20Z anca_vamanu $
 *
 * rls module - resource list server
 *
 * Copyright (C) 2007 Voice Sistem S.R.L.
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
 * History:
 * --------
 *  2007-09-11  initial version (anca)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../ut.h"
#include "../../dprint.h"
#include "../../data_lump_rpl.h"
#include "../../lib/kcore/cmpapi.h"
#include "../../lib/kcore/hash_func.h"
#include "../../lib/kcore/parse_supported.h"
#include "../../lib/kcore/parser_helpers.h"
#include "../../parser/msg_parser.h"
#include "../../parser/parse_event.h"
#include "../../parser/parse_expires.h"
#include "../../parser/parse_cseq.h"
#include "../../parser/parse_from.h"
#include "../../parser/contact/parse_contact.h"
#include "../../parser/parse_rr.h"
#include "../presence/subscribe.h"
#include "../presence/utils_func.h"
#include "../presence/hash.h"
#include "subscribe.h"
#include "notify.h"
#include "rls.h"

int counter= 0;

static str su_200_rpl     = str_init("OK");
static str pu_421_rpl     = str_init("Extension Required");
static str pu_400_rpl     = str_init("Bad request");
static str stale_cseq_rpl = str_init("Stale Cseq Value");
static str pu_489_rpl     = str_init("Bad Event");

#define Stale_cseq_code 401

subs_t* constr_new_subs(struct sip_msg* msg, struct to_body *pto, 
		pres_ev_t* event);
int resource_subscriptions(subs_t* subs, xmlNodePtr rl_node);

int update_rlsubs( subs_t* subs,unsigned int hash_code);
int remove_expired_rlsubs( subs_t* subs,unsigned int hash_code);

/**
 * return the XML node for rls-services matching uri
 */
xmlNodePtr rls_get_by_service_uri(xmlDocPtr doc, str* uri)
{
	xmlNodePtr root, node;
	char* val;

	root = XMLDocGetNodeByName(doc, "rls-services", NULL);
	if(root==NULL)
	{
		LM_ERR("no rls-services node in XML document\n");
		return NULL;
	}

	for(node=root->children; node; node=node->next)
	{
		if(xmlStrcasecmp(node->name, (unsigned char*)"service")==0)
		{
			val = XMLNodeGetAttrContentByName(node, "uri");
			if(val!=NULL)
			{
				if((uri->len==strlen(val)) && (strncmp(val, uri->s, uri->len)==0))
				{
					xmlFree(val);
					return node;
				}
				xmlFree(val);
			}
		}
	}
	return NULL;
}

/**
 * return service_node and rootdoc based on service uri, user and domain
 * - document is taken from DB or via xcap client
 */
int rls_get_service_list(str *service_uri, str *user, str *domain,
		 xmlNodePtr *service_node, xmlDocPtr *rootdoc)
{
	db_key_t query_cols[5];
	db_val_t query_vals[5];
	db_key_t result_cols[3];
	int n_query_cols = 0;
	db1_res_t *result = 0;
	db_row_t *row;
	db_val_t *row_vals;
	str body;
	int n_result_cols= 0;
	int etag_col, xcap_col;
	char *etag= NULL;
	xcap_get_req_t req;
	xmlDocPtr xmldoc = NULL;
	xcap_doc_sel_t doc_sel;
	char *xcapdoc= NULL;

	if(service_uri==NULL || user==NULL || domain==NULL)
	{
		LM_ERR("invalid parameters\n");
		return -1;
	}

	/* first search in database */
	query_cols[n_query_cols] = &str_username_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = *user;
	n_query_cols++;
	
	query_cols[n_query_cols] = &str_domain_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = *domain;
	n_query_cols++;
	
	query_cols[n_query_cols] = &str_doc_type_col;
	query_vals[n_query_cols].type = DB1_INT;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.int_val= RLS_SERVICE;
	n_query_cols++;

	LM_DBG("searching document for user sip:%.*s@%.*s\n",
		user->len, user->s, domain->len, domain->s);

	if(rls_dbf.use_table(rls_db, &rls_xcap_table) < 0)
	{
		LM_ERR("in use_table-[table]= %.*s\n",
				rls_xcap_table.len, rls_xcap_table.s);
		return -1;
	}

	result_cols[xcap_col= n_result_cols++] = &str_doc_col;
	result_cols[etag_col= n_result_cols++] = &str_etag_col;

	if(rls_dbf.query(rls_db, query_cols, 0 , query_vals, result_cols,
				n_query_cols, n_result_cols, 0, &result)<0)
	{
		LM_ERR("failed querying table xcap for document [service_uri]=%.*s\n",
				service_uri->len, service_uri->s);
		if(result)
			rls_dbf.free_result(rls_db, result);
		return -1;
	}

	if(result->n<=0)
	{
		LM_DBG("No rl document found\n");
		
		if(rls_integrated_xcap_server)
		{
			rls_dbf.free_result(rls_db, result);
			return 0;
		}
		
		/* make an initial request to xcap_client module */
		memset(&doc_sel, 0, sizeof(xcap_doc_sel_t));
		doc_sel.auid.s= "rls-services";
		doc_sel.auid.len= strlen("rls-services");
		doc_sel.doc_type= RLS_SERVICE;
		doc_sel.type= USERS_TYPE;
		if(uandd_to_uri(*user, *domain, &doc_sel.xid)<0)
		{
			LM_ERR("failed to create uri from user and domain\n");
			goto error;
		}

		memset(&req, 0, sizeof(xcap_get_req_t));
		req.xcap_root= xcap_root;
		req.port= xcap_port;
		req.doc_sel= doc_sel;
		req.etag= etag;
		req.match_type= IF_NONE_MATCH;

		xcapdoc= xcap_GetNewDoc(req, *user, *domain);
		if(xcapdoc==NULL)
		{
			LM_ERR("while fetching data from xcap server\n");
			pkg_free(doc_sel.xid.s);
			goto error;
		}
		pkg_free(doc_sel.xid.s);
		body.s = xcapdoc;
		body.len = strlen(xcapdoc);
	} else {
		row = &result->rows[0];
		row_vals = ROW_VALUES(row);

		body.s = (char*)row_vals[xcap_col].val.string_val;
		if(body.s== NULL)
		{
			LM_ERR("xcap doc is null\n");
			goto error;
		}
		body.len = strlen(body.s);
		if(body.len==0)
		{
			LM_ERR("xcap document is empty\n");
			goto error;
		}
	}

	LM_DBG("rls_services document:\n%.*s", body.len, body.s);
	xmldoc = xmlParseMemory(body.s, body.len);
	if(xmldoc==NULL)
	{
		LM_ERR("while parsing XML memory\n");
		goto error;
	}

	*service_node = rls_get_by_service_uri(xmldoc, service_uri);
	if(*service_node==NULL)
	{
		LM_DBG("service uri %.*s not found in rl document for user"
			" sip:%.*s@%.*s\n", service_uri->len, service_uri->s,
			user->len, user->s, domain->len, domain->s);
		rootdoc = NULL;
		if(xmldoc!=NULL)
			xmlFreeDoc(xmldoc);
	}
	else
	{
		*rootdoc = xmldoc;
	}

	rls_dbf.free_result(rls_db, result);
	if(xcapdoc!=NULL)
		pkg_free(xcapdoc);

	return 0;

error:
	if(result!=NULL)
		rls_dbf.free_result(rls_db, result);
	if(xmldoc!=NULL)
		xmlFreeDoc(xmldoc);
	if(xcapdoc!=NULL)
		pkg_free(xcapdoc);

	return -1;
}

/**
 * reply 421 with require header
 */
int reply_421(struct sip_msg* msg)
{
	str hdr_append;
	char buffer[256];
	
	hdr_append.s = buffer;
	hdr_append.s[0]='\0';
	hdr_append.len = sprintf(hdr_append.s, "Require: eventlist\r\n");
	if(hdr_append.len < 0)
	{
		LM_ERR("unsuccessful sprintf\n");
		return -1;
	}
	hdr_append.s[hdr_append.len]= '\0';

	if (add_lump_rpl( msg, hdr_append.s, hdr_append.len, LUMP_RPL_HDR)==0 )
	{
		LM_ERR("unable to add lump_rl\n");
		return -1;
	}

	if (slb.freply(msg, 421, &pu_421_rpl) < 0)
	{
		LM_ERR("while sending reply\n");
		return -1;
	}
	return 0;

}

/**
 * reply 200 ok with require header
 */
int reply_200(struct sip_msg* msg, str* contact, int expires)
{
	str hdr_append;
	int len;
	
	hdr_append.s = (char *)pkg_malloc( sizeof(char)*(contact->len+ 70));
	if(hdr_append.s == NULL)
	{
		LM_ERR("no more pkg memory\n");
		return -1;
	}
	hdr_append.len = sprintf(hdr_append.s, "Expires: %d\r\n", expires);	
	if(hdr_append.len< 0)
	{
		LM_ERR("unsuccessful sprintf\n");
		goto error;
	}
	strncpy(hdr_append.s+hdr_append.len ,"Contact: <", 10);
	hdr_append.len += 10;
	strncpy(hdr_append.s+hdr_append.len, contact->s, contact->len);
	hdr_append.len+= contact->len;
	strncpy(hdr_append.s+hdr_append.len, ">", 1);
	hdr_append.len += 1;
	strncpy(hdr_append.s+hdr_append.len, CRLF, CRLF_LEN);
	hdr_append.len += CRLF_LEN;

	len = sprintf(hdr_append.s+ hdr_append.len, "Require: eventlist\r\n");
	if(len < 0)
	{
		LM_ERR("unsuccessful sprintf\n");
		goto error;
	}
	hdr_append.len+= len;
	hdr_append.s[hdr_append.len]= '\0';
	
	if (add_lump_rpl( msg, hdr_append.s, hdr_append.len, LUMP_RPL_HDR)==0 )
	{
		LM_ERR("unable to add lump_rl\n");
		goto error;
	}

	if(slb.freply(msg, 200, &su_200_rpl) < 0)
	{
		LM_ERR("while sending reply\n");
		goto error;
	}	
	pkg_free(hdr_append.s);
	return 0;

error:
	pkg_free(hdr_append.s);
	return -1;
}	

/**
 * reply 489 with allow-events header
 */
int reply_489(struct sip_msg * msg)
{
	str hdr_append;
	char buffer[256];
	str* ev_list;

	hdr_append.s = buffer;
	hdr_append.s[0]='\0';
	hdr_append.len = sprintf(hdr_append.s, "Allow-Events: ");
	if(hdr_append.len < 0)
	{
		LM_ERR("unsuccessful sprintf\n");
		return -1;
	}

	if(pres_get_ev_list(&ev_list)< 0)
	{
		LM_ERR("while getting ev_list\n");
		return -1;
	}	
	memcpy(hdr_append.s+ hdr_append.len, ev_list->s, ev_list->len);
	hdr_append.len+= ev_list->len;
	pkg_free(ev_list->s);
	pkg_free(ev_list);
	memcpy(hdr_append.s+ hdr_append.len, CRLF, CRLF_LEN);
	hdr_append.len+=  CRLF_LEN;
	hdr_append.s[hdr_append.len]= '\0';
		
	if (add_lump_rpl( msg, hdr_append.s, hdr_append.len, LUMP_RPL_HDR)==0 )
	{
		LM_ERR("unable to add lump_rl\n");
		return -1;
	}
	if (slb.freply(msg, 489, &pu_489_rpl) < 0)
	{
		LM_ERR("while sending reply\n");
		return -1;
	}
	return 0;
}
	

/**
 * handle RLS subscription
 */
int rls_handle_subscribe(struct sip_msg* msg, char* s1, char* s2)
{
	subs_t subs;
	pres_ev_t* event = NULL;
	int err_ret = -1;
	str* contact = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr service_node = NULL;
	unsigned int hash_code;
	int to_tag_gen = 0;
	event_t* parsed_event;
	param_t* ev_param = NULL;
	str reason;
	int rt;

	memset(&subs, 0, sizeof(subs_t));

	/** sanity checks - parse all headers */
	if (parse_headers(msg, HDR_EOH_F, 0)<-1)
	{
		LM_ERR("failed parsing all headers\n");
		if (slb.freply(msg, 400, &pu_400_rpl) < 0)
		{
			LM_ERR("while sending 400 reply\n");
			return -1;
		}
		return 0;
	}
	/* check for To and From headesr */
	if(parse_to_uri(msg)<0 || parse_from_uri(msg)<0)
	{
		LM_ERR("failed to find To or From headers\n");
		if (slb.freply(msg, 400, &pu_400_rpl) < 0)
		{
			LM_ERR("while sending 400 reply\n");
			return -1;
		}
		return 0;
	}
	if(get_from(msg)->tag_value.s ==NULL || get_from(msg)->tag_value.len==0)
	{
		LM_ERR("no from tag value present\n");
		return -1;
	}
	if(msg->callid==NULL || msg->callid->body.s==NULL)
	{
		LM_ERR("cannot find callid header\n");
		return -1;
	}
	if(parse_sip_msg_uri(msg)<0)
	{
		LM_ERR("failed parsing Request URI\n");
		return -1;
	}

	/* check for header 'Support: eventlist' */
	if(msg->supported==NULL)
	{
		LM_DBG("supported header not found - not for rls\n");
		goto forpresence;
	}

	if(parse_supported(msg)<0)
	{
		LM_ERR("failed to parse supported headers\n");
		return -1;
	}

	if(!(get_supported(msg) & F_SUPPORTED_EVENTLIST))
	{
		LM_DBG("No support for 'eventlist' - not for rls\n");
		goto forpresence;
	}

	/* inspecting the Event header field */
	if(msg->event && msg->event->body.len > 0)
	{
		if (!msg->event->parsed && (parse_event(msg->event) < 0))
		{
			LM_ERR("cannot parse Event header\n");
			goto error;
		}
		if(! ( ((event_t*)msg->event->parsed)->type & rls_events) )
		{	
			goto forpresence;
		}
	} else {
		goto bad_event;
	}

	/* search event in the list */
	parsed_event = (event_t*)msg->event->parsed;
	event = pres_search_event(parsed_event);
	if(event==NULL)
	{
		goto bad_event;
	}
	subs.event= event;
	
	/* extract the id if any*/
	ev_param= parsed_event->params.list;
	while(ev_param)
	{
		if(ev_param->name.len==2 && strncmp(ev_param->name.s, "id", 2)==0)
		{
			subs.event_id = ev_param->body;
			break;
		}
		ev_param= ev_param->next;
	}		

	if(get_to(msg)->tag_value.s==NULL || get_to(msg)->tag_value.len==0)
	{ /* initial Subscribe */
		/*verify if Request URI represents a list by asking xcap server*/	
		if(uandd_to_uri(msg->parsed_uri.user, msg->parsed_uri.host,
					&subs.pres_uri)<0)
		{
			LM_ERR("while constructing uri from user and domain\n");
			goto error;
		}
		if(rls_get_service_list(&subs.pres_uri, &(GET_FROM_PURI(msg)->user),
					&(GET_FROM_PURI(msg)->host), &service_node, &doc)<0)
		{
			LM_ERR("while attepmting to get a resource list\n");
			goto error;
		}
		if(doc==NULL)
		{
			/* if not for RLS, pass it to presence serivce */
			LM_DBG("list not found - searched for uri <%.*s>\n",
					subs.pres_uri.len, subs.pres_uri.s);
			goto forpresence;
		}
	} else {
		/* search if a stored dialog */
		hash_code = core_hash(&msg->callid->body,
				&get_to(msg)->tag_value, hash_size);
		lock_get(&rls_table[hash_code].lock);

		if(pres_search_shtable(rls_table, msg->callid->body,
				get_to(msg)->tag_value, get_from(msg)->tag_value,
				hash_code)==NULL)
		{
			lock_release(&rls_table[hash_code].lock);
			LM_DBG("subscription dialog not found for <%.*s>\n",
					get_from(msg)->uri.len, get_from(msg)->uri.s);
			goto forpresence;
		}
		lock_release(&rls_table[hash_code].lock);
	}

	/* extract dialog information from message headers */
	if(pres_extract_sdialog_info(&subs, msg, rls_max_expires,
				&to_tag_gen, rls_server_address)<0)
	{
		LM_ERR("bad subscribe request\n");
		goto error;
	}

	/* if correct reply with 200 OK */
	if(reply_200(msg, &subs.local_contact, subs.expires)<0)
		goto error;

	hash_code = core_hash(&subs.callid, &subs.to_tag, hash_size);

	if(get_to(msg)->tag_value.s==NULL || get_to(msg)->tag_value.len==0)
	{ /* initial subscribe */
		subs.local_cseq = 0;

		if(subs.expires != 0)
		{
			subs.version = 1;
			if(pres_insert_shtable(rls_table, hash_code, &subs)<0)
			{
				LM_ERR("while adding new subscription\n");
				goto error;
			}
		}
	} else {
		rt = update_rlsubs(&subs, hash_code);
		if(rt<0)
		{
			LM_ERR("while updating resource list subscription\n");
			goto error;
		}
	
		if(rt>=400)
		{
			reason = (rt==400)?pu_400_rpl:stale_cseq_rpl;
		
			if (slb.freply(msg, 400, &reason) < 0)
			{
				LM_ERR("while sending reply\n");
				goto error;
			}
			return 0;
		}	

		if(rls_get_service_list(&subs.pres_uri, &subs.from_user,
					&subs.from_domain, &service_node, &doc)<0)
		{
			LM_ERR("failed getting resource list\n");
			goto error;
		}
		if(doc==NULL)
		{
			/* warning: no document returned?!?! */
			LM_WARN("no document returned for uri <%.*s>\n",
					subs.pres_uri.len, subs.pres_uri.s);
			goto done;
		}
	}

	/* sending notify with full state */
	if(send_full_notify(&subs, service_node, subs.version, &subs.pres_uri,
				hash_code)<0)
	{
		LM_ERR("failed sending full state notify\n");
		goto error;
	}
	/* send subscribe requests for all in the list */
	if(resource_subscriptions(&subs, service_node)< 0)
	{
		LM_ERR("failed sending subscribe requests to resources in list\n");
		goto error;
	}
	remove_expired_rlsubs(&subs, hash_code);

done:
	if(contact!=NULL)
	{	
		if(contact->s!=NULL)
			pkg_free(contact->s);
		pkg_free(contact);
	}

	if(subs.pres_uri.s!=NULL)
		pkg_free(subs.pres_uri.s);
	if(subs.record_route.s!=NULL)
		pkg_free(subs.record_route.s);
	if(doc!=NULL)
		xmlFreeDoc(doc);
	return 1;

forpresence:
	if(subs.pres_uri.s!=NULL)
		pkg_free(subs.pres_uri.s);
	return to_presence_code;

bad_event:
	err_ret = 0;
	if(reply_489(msg)<0)
	{
		LM_ERR("failed sending 489 reply\n");
		err_ret = -1;
	}

error:
	LM_ERR("occured in rls_handle_subscribe\n");

	if(contact!=NULL)
	{	
		if(contact->s!=NULL)
			pkg_free(contact->s);
		pkg_free(contact);
	}
	if(subs.pres_uri.s!=NULL)
		pkg_free(subs.pres_uri.s);

	if(subs.record_route.s!=NULL)
		pkg_free(subs.record_route.s);

	if(doc!=NULL)
		xmlFreeDoc(doc);
	return err_ret;
}

int remove_expired_rlsubs( subs_t* subs, unsigned int hash_code)
{
	subs_t* s, *ps;
	int found= 0;

	if(subs->expires!=0)
		return 0;

	/* search the record in hash table */
	lock_get(&rls_table[hash_code].lock);

	s= pres_search_shtable(rls_table, subs->callid,
			subs->to_tag, subs->from_tag, hash_code);
	if(s== NULL)
	{
		LM_DBG("record not found in hash table\n");
		lock_release(&rls_table[hash_code].lock);
		return -1;
	}
	/* delete record from hash table */
	ps= rls_table[hash_code].entries;
	while(ps->next)
	{
		if(ps->next== s)
		{
			found= 1;
			break;
		}
		ps= ps->next;
	}
	if(found== 0)
	{
		LM_ERR("record not found\n");
		lock_release(&rls_table[hash_code].lock);
		return -1;
	}
	ps->next= s->next;
	shm_free(s);

	lock_release(&rls_table[hash_code].lock);

	return 0;

}

int update_rlsubs( subs_t* subs, unsigned int hash_code)
{
	subs_t* s;

	/* search the record in hash table */
	lock_get(&rls_table[hash_code].lock);

	s= pres_search_shtable(rls_table, subs->callid,
			subs->to_tag, subs->from_tag, hash_code);
	if(s== NULL)
	{
		LM_DBG("record not found in hash table\n");
		lock_release(&rls_table[hash_code].lock);
		return -1;
	}

	s->expires= subs->expires+ (int)time(NULL);
	
	if(s->db_flag & NO_UPDATEDB_FLAG)
		s->db_flag= UPDATEDB_FLAG;
	
	if(	s->remote_cseq>= subs->remote_cseq)
	{
		lock_release(&rls_table[hash_code].lock);
		LM_DBG("stored cseq= %d\n", s->remote_cseq);
		return Stale_cseq_code;
	}

	s->remote_cseq= subs->remote_cseq;

	subs->pres_uri.s= (char*)pkg_malloc(s->pres_uri.len* sizeof(char));
	if(subs->pres_uri.s== NULL)
	{
		ERR_MEM(PKG_MEM_STR);
	}
	memcpy(subs->pres_uri.s, s->pres_uri.s, s->pres_uri.len);
	subs->pres_uri.len= s->pres_uri.len;

	if(s->record_route.s!=NULL && s->record_route.len>0)
	{
		subs->record_route.s =
				(char*)pkg_malloc(s->record_route.len* sizeof(char));
		if(subs->record_route.s==NULL)
		{
			ERR_MEM(PKG_MEM_STR);
		}
		memcpy(subs->record_route.s, s->record_route.s, s->record_route.len);
		subs->record_route.len= s->record_route.len;
	}

	subs->local_cseq= s->local_cseq;
	subs->version= s->version;

	lock_release(&rls_table[hash_code].lock);

	return 0;

error:
	lock_release(&rls_table[hash_code].lock);
	return -1;
}

int send_resource_subs(char* uri, void* param)
{
	str pres_uri;

	pres_uri.s = uri;
	pres_uri.len = strlen(uri);

	((subs_info_t*)param)->pres_uri = &pres_uri;
	((subs_info_t*)param)->remote_target = &pres_uri;

	return pua_send_subscribe((subs_info_t*)param);
}

/**
 * send subscriptions to the list from XML node
 */
int resource_subscriptions(subs_t* subs, xmlNodePtr xmlnode)
{
	char* uri= NULL;
	subs_info_t s;
	str wuri= {0, 0};
	str extra_headers;
	str did_str= {0, 0};
		
	/* if is initial send an initial Subscribe 
	 * else search in hash table for a previous subscription */

	if(CONSTR_RLSUBS_DID(subs, &did_str)<0)
	{
		LM_ERR("cannot build rls subs did\n");
		goto error;
	}

	memset(&s, 0, sizeof(subs_info_t));

	if(uandd_to_uri(subs->from_user, subs->from_domain, &wuri)<0)
	{
		LM_ERR("while constructing uri from user and domain\n");
		goto error;
	}
	s.id = did_str;
	s.watcher_uri = &wuri;
	s.contact = &rls_server_address;
	s.event = get_event_flag(&subs->event->name);
	if(s.event<0)
	{
		LM_ERR("not recognized event\n");
		goto error;
	}
	s.expires = subs->expires;
	s.source_flag = RLS_SUBSCRIBE;
	if(rls_outbound_proxy.s)
		s.outbound_proxy = &rls_outbound_proxy;
	extra_headers.s = "Supported: eventlist\r\n"
				"Accept: application/pidf+xml, application/rlmi+xml,"
					" application/watcherinfo+xml,"
					" multipart/related\r\n";
	extra_headers.len = strlen(extra_headers.s);

	s.extra_headers = &extra_headers;
	
	if(process_list_and_exec(xmlnode, send_resource_subs, (void*)(&s))<0)
	{
		LM_ERR("while processing list\n");
		goto error;
	}

	pkg_free(wuri.s);
	pkg_free(did_str.s);

	return 0;

error:
	if(wuri.s)
		pkg_free(wuri.s);
	if(uri)
		xmlFree(uri);
	if(did_str.s)
		pkg_free(did_str.s);
	return -1;

}

