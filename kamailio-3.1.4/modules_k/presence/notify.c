/*
 * $Id$
 *
 * presence module- presence server implementation
 *
 * Copyright (C) 2006 Voice Sistem S.R.L.
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
 *  2006-08-15  initial version (anca)
 */

/*! \file
 * \brief Kamailio presence module :: Notification with SIP NOTIFY
 * \ingroup presence 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>

#include "../../trim.h"
#include "../../ut.h"
#include "../../globals.h"
#include "../../str.h"
#include "../../lib/srdb1/db.h"
#include "../../lib/srdb1/db_val.h"
#include "../../lib/kcore/hash_func.h"
#include "../../socket_info.h"
#include "../../modules/tm/tm_load.h"
#include "../pua/hash.h"
#include "presentity.h"
#include "presence.h"
#include "notify.h"
#include "utils_func.h"

#define ALLOC_SIZE 3000
#define MAX_FORWARD 70

c_back_param* shm_dup_cbparam(subs_t*);
void free_cbparam(c_back_param* cb_param);

void p_tm_callback( struct cell *t, int type, struct tmcb_params *ps);
int add_waiting_watchers(watcher_t *watchers, str pres_uri, str event);
int add_watcher_list(subs_t *s, watcher_t *watchers);
str* create_winfo_xml(watcher_t* watchers, char* version,
		str resource, str event, int STATE_FLAG);
void free_watcher_list(watcher_t* watchers);

str str_to_user_col = str_init("to_user");
str str_username_col = str_init("username");
str str_domain_col = str_init("domain");
str str_body_col = str_init("body");
str str_to_domain_col = str_init("to_domain");
str str_watcher_username_col = str_init("watcher_username");
str str_watcher_domain_col = str_init("watcher_domain");
str str_event_id_col = str_init("event_id");
str str_event_col = str_init("event");
str str_etag_col = str_init("etag");
str str_from_tag_col = str_init("from_tag");
str str_to_tag_col = str_init("to_tag");
str str_callid_col = str_init("callid");
str str_local_cseq_col = str_init("local_cseq");
str str_remote_cseq_col = str_init("remote_cseq");
str str_record_route_col = str_init("record_route");
str str_contact_col = str_init("contact");
str str_expires_col = str_init("expires");
str str_status_col = str_init("status");
str str_reason_col = str_init("reason");
str str_socket_info_col = str_init("socket_info");
str str_local_contact_col = str_init("local_contact");
str str_version_col = str_init("version");
str str_presentity_uri_col = str_init("presentity_uri");
str str_inserted_time_col = str_init("inserted_time");
str str_received_time_col = str_init("received_time");
str str_id_col = str_init("id");
str str_sender_col = str_init("sender");

char* get_status_str(int status_flag)
{
	switch(status_flag)
	{
		case ACTIVE_STATUS: return "active";
		case PENDING_STATUS: return "pending";
		case TERMINATED_STATUS: return "terminated";
		case WAITING_STATUS: return "waiting";
	}
	return NULL;
}

void printf_subs(subs_t* subs)
{	
	LM_DBG("\n\t[pres_uri]= %.*s\n\t[to_user]= %.*s\t[to_domain]= %.*s"
		"\n\t[w_user]= %.*s\t[w_domain]= %.*s\n\t[event]= %.*s\n\t[status]= %s"
		"\n\t[expires]= %u\n\t[callid]= %.*s\t[local_cseq]=%d"
		"\n\t[to_tag]= %.*s\t[from_tag]= %.*s""\n\t[contact]= %.*s"
		"\t[record_route]= %.*s\n",subs->pres_uri.len,subs->pres_uri.s,
		subs->to_user.len,subs->to_user.s,subs->to_domain.len,
		subs->to_domain.s,subs->from_user.len,subs->from_user.s,
		subs->from_domain.len,subs->from_domain.s,subs->event->name.len,
		subs->event->name.s,get_status_str(subs->status),subs->expires,
		subs->callid.len,subs->callid.s,subs->local_cseq,subs->to_tag.len,
		subs->to_tag.s,subs->from_tag.len, subs->from_tag.s,subs->contact.len,
		subs->contact.s,subs->record_route.len,subs->record_route.s);
}

int build_str_hdr(subs_t* subs, int is_body, str* hdr)
{
	pres_ev_t* event= subs->event;
	str expires = {0, 0};
	str status = {0, 0};
	str tmp = {0, 0};
	str trans = {";transport=", 11};

	if(hdr == NULL)
	{
		LM_ERR("bad parameter\n");
		return -1;
	}
	expires.s = int2str(subs->expires, &expires.len);
	status.s= get_status_str(subs->status);
	if(status.s == NULL)
	{
		LM_ERR("bad status %d\n", subs->status);
		return -1;
	}
	status.len = strlen(status.s);

	hdr->len = 18 /*Max-Forwards:  + val*/ + CRLF_LEN + 
		7 /*Event: */ + subs->event->name.len +4 /*;id=*/+ subs->event_id.len+
		CRLF_LEN + 10 /*Contact: <*/ + subs->local_contact.len + 1/*>*/ +
		15/*";transport=xxxx"*/ + CRLF_LEN + 20 /*Subscription-State: */ +
		status.len + 10 /*reason/expires params*/
		+ (subs->reason.len>expires.len?subs->reason.len:expires.len)
		+ CRLF_LEN + (is_body?
		(14 /*Content-Type: */+subs->event->content_type.len + CRLF_LEN):0) + 1;

	hdr->s = (char*)pkg_malloc(hdr->len);
	if(hdr->s == NULL)
	{
		LM_ERR("no more pkg\n");
		return -1;
	}

	strncpy(hdr->s, "Max-Forwards: ", 14);
	tmp.s = int2str((unsigned long)MAX_FORWARD, &tmp.len);
	strncpy(hdr->s+14, tmp.s, tmp.len);
	tmp.s = hdr->s + tmp.len + 14;
	strncpy(tmp.s, CRLF, CRLF_LEN);
	tmp.s += CRLF_LEN;

	strncpy(tmp.s  ,"Event: ", 7);
	tmp.s += 7;
	strncpy(tmp.s, event->name.s, event->name.len);
	tmp.s += event->name.len;
	if(subs->event_id.len && subs->event_id.s) 
	{
 		strncpy(tmp.s, ";id=", 4);
 		tmp.s += 4;
 		strncpy(tmp.s, subs->event_id.s, subs->event_id.len);
 		tmp.s += subs->event_id.len;
 	}
	strncpy(tmp.s, CRLF, CRLF_LEN);
	tmp.s += CRLF_LEN;

	strncpy(tmp.s, "Contact: <", 10);
	tmp.s += 10;
	strncpy(tmp.s, subs->local_contact.s, subs->local_contact.len);
	tmp.s +=  subs->local_contact.len;
	if(subs->sockinfo_str.s!=NULL
			&& str_search(&subs->local_contact, &trans)==0)
	{
		/* fix me */
		switch(subs->sockinfo_str.s[0]) {
			case 's':
			case 'S':
				strncpy(tmp.s, ";transport=sctp", 15);
				tmp.s += 15;
			break;
			case 't':
			case 'T':
				switch(subs->sockinfo_str.s[1]) {
					case 'c':
					case 'C':
						strncpy(tmp.s, ";transport=tcp", 14);
						tmp.s += 14;
					break;
					case 'l':
					case 'L':
						strncpy(tmp.s, ";transport=tls", 14);
						tmp.s += 14;
					break;
				}
			break;
		}
	}
	*tmp.s =  '>';
	tmp.s++;
	strncpy(tmp.s, CRLF, CRLF_LEN);
	tmp.s += CRLF_LEN;
	
	strncpy(tmp.s, "Subscription-State: ", 20);
	tmp.s += 20;
	strncpy(tmp.s, status.s, status.len);
	tmp.s += status.len;
	
	if(subs->status == TERMINATED_STATUS)
	{
		LM_DBG("state = terminated\n");
		
		strncpy(tmp.s, ";reason=", 8);
		tmp.s += 8;
		strncpy(tmp.s, subs->reason.s, subs->reason.len);
		tmp.s += subs->reason.len;
	} else {	
		strncpy(tmp.s, ";expires=", 9);
		tmp.s += 9;
		LM_DBG("expires = %d\n", subs->expires);
		strncpy(tmp.s, expires.s, expires.len);
		tmp.s += expires.len;
	}
	strncpy(tmp.s, CRLF, CRLF_LEN);
	tmp.s += CRLF_LEN;
	
	if(is_body)
	{	
		strncpy(tmp.s,"Content-Type: ", 14);
		tmp.s += 14;
		strncpy(tmp.s, event->content_type.s, event->content_type.len);
		tmp.s += event->content_type.len;
		strncpy(tmp.s, CRLF, CRLF_LEN);
		tmp.s += CRLF_LEN;
	}
	
	*tmp.s = '\0';
	hdr->len = tmp.s - hdr->s;

	return 0;
}

int get_wi_subs_db(subs_t* subs, watcher_t* watchers)
{	
	subs_t sb;
	db_key_t query_cols[6];
	db_op_t  query_ops[6];
	db_val_t query_vals[6];
	db_key_t result_cols[6];
	db1_res_t *result = NULL;
	db_row_t *row = NULL ;	
	db_val_t *row_vals = NULL;
	int n_result_cols = 0;
	int n_query_cols = 0;
	int i;
	int status_col, expires_col, from_user_col, from_domain_col, callid_col;

	query_cols[n_query_cols] = &str_presentity_uri_col;
	query_ops[n_query_cols] = OP_EQ;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val= subs->pres_uri;
	n_query_cols++;

	query_cols[n_query_cols] = &str_event_col;
	query_ops[n_query_cols] = OP_EQ;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = subs->event->wipeer->name;
	n_query_cols++;

	result_cols[status_col=n_result_cols++] = &str_status_col;
	result_cols[expires_col=n_result_cols++] = &str_expires_col;
	result_cols[from_user_col=n_result_cols++] = &str_watcher_username_col;
	result_cols[from_domain_col=n_result_cols++] = &str_watcher_domain_col;
	result_cols[callid_col=n_result_cols++] = &str_callid_col;

	if (pa_dbf.use_table(pa_db, &active_watchers_table) < 0) 
	{
		LM_ERR("in use_table\n");
		goto error;
	}

	if (pa_dbf.query (pa_db, query_cols, query_ops, query_vals,
		 result_cols, n_query_cols, n_result_cols, 0,  &result) < 0) 
	{
		LM_ERR("querying active_watchers db table\n");
		goto error;
	}

	if(result== NULL )
	{
		goto error;
	}

	if(result->n <= 0)
	{
		LM_DBG("The query in db table for active subscription"
				" returned no result\n");
		pa_dbf.free_result(pa_db, result);
		return 0;
	}
	
	for(i=0; i<result->n; i++)
	{
		row = &result->rows[i];
		row_vals = ROW_VALUES(row);
		
		sb.from_user.s= (char*)row_vals[from_user_col].val.string_val;
		sb.from_user.len= strlen(sb.from_user.s);

		sb.from_domain.s= (char*)row_vals[from_domain_col].val.string_val;
		sb.from_domain.len= strlen(sb.from_domain.s);

		sb.callid.s= (char*)row_vals[callid_col].val.string_val;
		sb.callid.len= strlen(sb.callid.s);

		sb.event =subs->event->wipeer;
		sb.status= row_vals[status_col].val.int_val;
		
		if(add_watcher_list(&sb, watchers)<0)
			goto error;

	}
	
	pa_dbf.free_result(pa_db, result);
	return 0;

error:
	if(result)
		pa_dbf.free_result(pa_db, result);
	return -1;
}

str* get_wi_notify_body(subs_t* subs, subs_t* watcher_subs)
{
	str* notify_body = NULL;
	char* version_str;
	watcher_t *watchers = NULL;
	int len = 0;
	unsigned int hash_code;
	subs_t* s= NULL;
	int state = FULL_STATE_FLAG;

	hash_code = 0;
	version_str = int2str(subs->version, &len);
	if(version_str ==NULL)
	{
		LM_ERR("converting int to str\n ");
		goto error;
	}

	watchers= (watcher_t*)pkg_malloc(sizeof(watcher_t));
	if(watchers== NULL)
	{
		ERR_MEM(PKG_MEM_STR);
	}
	memset(watchers, 0, sizeof(watcher_t));

	if(watcher_subs != NULL) 
	{		
		if(add_watcher_list(watcher_subs, watchers)< 0)
			goto error;
		state = PARTIAL_STATE_FLAG;

		goto done;
	}

	if(fallback2db)
	{
		if(get_wi_subs_db(subs, watchers)< 0)
		{
			LM_ERR("getting watchers from database\n");
			goto error;
		}
	}

	hash_code= core_hash(&subs->pres_uri, &subs->event->wipeer->name,
            shtable_size);
	lock_get(&subs_htable[hash_code].lock);

	s= subs_htable[hash_code].entries;

    while(s->next)
	{
		s= s->next;

		if(s->expires< (int)time(NULL))
		{	
			LM_DBG("expired record\n");
			continue;
		}

		if(fallback2db && s->db_flag!= INSERTDB_FLAG)
		{
			LM_DBG("record already found in database\n");
			continue;
		}

        if(s->event== subs->event->wipeer &&
			s->pres_uri.len== subs->pres_uri.len &&
			strncmp(s->pres_uri.s, subs->pres_uri.s,subs->pres_uri.len)== 0)
		{
			if(add_watcher_list(s, watchers)< 0)
			{
				lock_release(&subs_htable[hash_code].lock);
				goto error;
			}
		}
	}
	
	if(add_waiting_watchers(watchers, subs->pres_uri,
				subs->event->wipeer->name)< 0 )
	{
		LM_ERR("failed to add waiting watchers\n");
		goto error;
	}

done:
	notify_body = create_winfo_xml(watchers,version_str,subs->pres_uri,
			subs->event->wipeer->name, state);
	
	if(watcher_subs == NULL) 
		lock_release(&subs_htable[hash_code].lock);

	if(notify_body== NULL)
	{
		LM_ERR("in function create_winfo_xml\n");
		goto error;
	}
	free_watcher_list(watchers);
	return notify_body;

error:
	if(notify_body)
	{
		if(notify_body->s)
			xmlFree(notify_body->s);
		pkg_free(notify_body);
	}
	free_watcher_list(watchers);
	return NULL;
}

void free_watcher_list(watcher_t* watchers)
{
	watcher_t* w;
	while(watchers)
	{	
		w= watchers;
		if(w->uri.s !=NULL)
			pkg_free(w->uri.s);
		if(w->id.s !=NULL)
			pkg_free(w->id.s);
		watchers= watchers->next;
		pkg_free(w);
	}
}

int add_watcher_list(subs_t *s, watcher_t *watchers)
{
	watcher_t* w;

	w= (watcher_t*)pkg_malloc(sizeof(watcher_t));
	if(w== NULL)
	{
		LM_ERR("No more private memory\n");
		return -1;
	}
	w->status= s->status;
	if(uandd_to_uri(s->from_user, s->from_domain, &w->uri)<0)
	{
		LM_ERR("failed to create uri\n");
		goto error;
	}
	w->id.s = (char*)pkg_malloc(s->callid.len+ 1);
	if(w->id.s == NULL)
	{
		LM_ERR("no more memory\n");
		goto error;
	}
	memcpy(w->id.s, s->callid.s, s->callid.len);
	w->id.len = s->callid.len;
	w->id.s[w->id.len] = '\0';

	w->next= watchers->next;
	watchers->next= w;

	return 0;

error:
	if(w)
	{
		if(w->uri.s)
			pkg_free(w->uri.s);
		pkg_free(w);
	}
	return -1;
}

str* build_empty_bla_body(str pres_uri)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlAttrPtr attr;
	str* body= NULL;
	char* text;
	int len;
	char* entity= NULL;

	doc = xmlNewDoc(BAD_CAST "1.0");
	if(doc== NULL)
	{
		LM_ERR("failed to construct xml document\n");
		return NULL;
	}

	node = xmlNewNode(NULL, BAD_CAST "dialog-info");
	if(node== NULL)
	{
		LM_ERR("failed to initialize node\n");
		goto error;
	}
	xmlDocSetRootElement(doc, node);

	attr =  xmlNewProp(node, BAD_CAST "xmlns",BAD_CAST  "urn:ietf:params:xml:ns:dialog-info");
	if(attr== NULL)
	{
		LM_ERR("failed to initialize node attribute\n");
		goto error;
	}
	attr = xmlNewProp(node, BAD_CAST "version", BAD_CAST "1");
	if(attr== NULL)
	{
		LM_ERR("failed to initialize node attribute\n");
		goto error;
	}

	attr = xmlNewProp(node, BAD_CAST "state", BAD_CAST "full");
	if(attr== NULL)
	{
		LM_ERR("failed to initialize node attribute\n");
		goto error;
	}

	entity = (char*)pkg_malloc(pres_uri.len+1);
	if(entity== NULL)
	{
		LM_ERR("no more memory\n");
		goto error;
	}
	memcpy(entity, pres_uri.s, pres_uri.len);
	entity[pres_uri.len]= '\0';

	attr = xmlNewProp(node, BAD_CAST "entity", BAD_CAST entity);
	if(attr== NULL)
	{
		LM_ERR("failed to initialize node attribute\n");
		pkg_free(entity);
		goto error;
	}
	
	body = (str*) pkg_malloc(sizeof(str));
	if(body== NULL)
	{
		LM_ERR("no more private memory");
		pkg_free(entity);
		goto error;
	}

	xmlDocDumpFormatMemory(doc,(xmlChar**)(void*)&text, &len, 1);
	body->s = (char*) pkg_malloc(len);
	if(body->s == NULL)
	{
		LM_ERR("no more private memory");
		pkg_free(body);
		pkg_free(entity);
		goto error;
	}
	memcpy(body->s, text, len);
	body->len= len;

	
	pkg_free(entity);
	xmlFreeDoc(doc);
	xmlFree(text);

	return body;

error:
	xmlFreeDoc(doc);
	return NULL;

}

str* get_p_notify_body(str pres_uri, pres_ev_t* event, str* etag,
		str* contact)
{
	db_key_t query_cols[6];
	db_val_t query_vals[6];
	db_key_t result_cols[6];
	db1_res_t *result = NULL;
	int body_col, expires_col, etag_col= 0, sender_col;
	str** body_array= NULL;
	str* notify_body= NULL;	
	db_row_t *row= NULL ;	
	db_val_t *row_vals;
	int n_result_cols = 0;
	int n_query_cols = 0;
	int i, n= 0, len;
	int build_off_n= -1; 
	str etags;
	str* body;
	int size= 0;
	struct sip_uri uri;
	unsigned int hash_code;
	str sender;

	if(parse_uri(pres_uri.s, pres_uri.len, &uri)< 0)
	{
		LM_ERR("while parsing uri\n");
		return NULL;
	}

	/* search in hash table if any record exists */
	hash_code= core_hash(&pres_uri, NULL, phtable_size);
	if(search_phtable(&pres_uri, event->evp->type, hash_code)== NULL)
	{
		LM_DBG("No record exists in hash_table\n");
		if(fallback2db)
			goto db_query;

		/* for pidf manipulation */
		if(event->agg_nbody)
		{
			notify_body = event->agg_nbody(&uri.user, &uri.host, NULL, 0, -1);
			if(notify_body)
				goto done;
		}			
		return NULL;
	}

db_query:

	query_cols[n_query_cols] = &str_domain_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = uri.host;
	n_query_cols++;

	query_cols[n_query_cols] = &str_username_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = uri.user;
	n_query_cols++;

	query_cols[n_query_cols] = &str_event_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val= event->name;
	n_query_cols++;

	result_cols[body_col=n_result_cols++] = &str_body_col;
	result_cols[expires_col=n_result_cols++] = &str_expires_col;
	result_cols[etag_col=n_result_cols++] = &str_etag_col;
	result_cols[sender_col=n_result_cols++] = &str_sender_col;
	
	if (pa_dbf.use_table(pa_db, &presentity_table) < 0) 
	{
		LM_ERR("in use_table\n");
		return NULL;
	}

	static str query_str = str_init("received_time");
	if (pa_dbf.query (pa_db, query_cols, 0, query_vals,
		 result_cols, n_query_cols, n_result_cols, &query_str ,  &result) < 0) 
	{
		LM_ERR("failed to query %.*s table\n", presentity_table.len, presentity_table.s);
		if(result)
			pa_dbf.free_result(pa_db, result);
		return NULL;
	}
	
	if(result== NULL)
		return NULL;

	if (result->n<=0 )
	{
		LM_DBG("The query returned no result\n[username]= %.*s"
			"\t[domain]= %.*s\t[event]= %.*s\n",uri.user.len, uri.user.s,
			uri.host.len, uri.host.s, event->name.len, event->name.s);
		
		pa_dbf.free_result(pa_db, result);
		result= NULL;

		if(event->agg_nbody)
		{
			notify_body = event->agg_nbody(&uri.user, &uri.host, NULL, 0, -1);
			if(notify_body)
				goto done;
		}			
		return NULL;
	}
	else
	{
		n= result->n;
		if(event->agg_nbody== NULL )
		{
			LM_DBG("Event does not require aggregation\n");
			row = &result->rows[n-1];
			row_vals = ROW_VALUES(row);
			
			/* if event BLA - check if sender is the same as contact */
			/* if so, send an empty dialog info document */
			if( EVENT_DIALOG_SLA(event->evp) && contact ) {
				sender.s = (char*)row_vals[sender_col].val.string_val;
				if(sender.s== NULL || strlen(sender.s)==0)
					goto after_sender_check;
				sender.len= strlen(sender.s);
			
				if(sender.len== contact->len &&
						strncmp(sender.s, contact->s, sender.len)== 0)
				{
					notify_body= build_empty_bla_body(pres_uri);
					pa_dbf.free_result(pa_db, result);
					return notify_body;
				}
			}

after_sender_check:
			if(row_vals[body_col].val.string_val== NULL)
			{
				LM_ERR("NULL notify body record\n");
				goto error;
			}
			len= strlen(row_vals[body_col].val.string_val);
			if(len== 0)
			{
				LM_ERR("Empty notify body record\n");
				goto error;
			}
			notify_body= (str*)pkg_malloc(sizeof(str));
			if(notify_body== NULL)
			{
				ERR_MEM(PKG_MEM_STR);	
			}
			memset(notify_body, 0, sizeof(str));
			notify_body->s= (char*)pkg_malloc( len* sizeof(char));
			if(notify_body->s== NULL)
			{
				pkg_free(notify_body);
				ERR_MEM(PKG_MEM_STR);
			}
			memcpy(notify_body->s, row_vals[body_col].val.string_val, len);
			notify_body->len= len;
			pa_dbf.free_result(pa_db, result);
			
			return notify_body;
		}
		
		LM_DBG("Event requires aggregation\n");
		
		body_array =(str**)pkg_malloc( (n+2) *sizeof(str*));
		if(body_array == NULL)
		{
			ERR_MEM(PKG_MEM_STR);
		}
		memset(body_array, 0, (n+2) *sizeof(str*));

		if(etag!= NULL)
		{
			LM_DBG("searched etag = %.*s len= %d\n", 
					etag->len, etag->s, etag->len);
			LM_DBG("etag not NULL\n");
			for(i= 0; i< n; i++)
			{
				row = &result->rows[i];
				row_vals = ROW_VALUES(row);
				etags.s = (char*)row_vals[etag_col].val.string_val;
				etags.len = strlen(etags.s);

				LM_DBG("etag = %.*s len= %d\n", etags.len, etags.s, etags.len);
				if( (etags.len == etag->len) && (strncmp(etags.s,
								etag->s,etags.len)==0 ) )
				{
					LM_DBG("found etag\n");
					build_off_n= i;
				}
				len= strlen((char*)row_vals[body_col].val.string_val);
				if(len== 0)
				{
					LM_ERR("Empty notify body record\n");
					goto error;
				}
			
				size= sizeof(str)+ len* sizeof(char);
				body= (str*)pkg_malloc(size);
				if(body== NULL)
				{
					ERR_MEM(PKG_MEM_STR);
				}
				memset(body, 0, size);
				size= sizeof(str);
				body->s= (char*)body+ size;
				memcpy(body->s, (char*)row_vals[body_col].val.string_val, len);
				body->len= len;

				body_array[i]= body;
			}
		}	
		else
		{	
			for(i=0; i< n; i++)
			{
				row = &result->rows[i];
				row_vals = ROW_VALUES(row);
				
				len= strlen((char*)row_vals[body_col].val.string_val);
				if(len== 0)
				{
					LM_ERR("Empty notify body record\n");
					goto error;
				}
				
				size= sizeof(str)+ len* sizeof(char);
				body= (str*)pkg_malloc(size);
				if(body== NULL)
				{
					ERR_MEM(PKG_MEM_STR);
				}
				memset(body, 0, size);
				size= sizeof(str);
				body->s= (char*)body+ size;
				memcpy(body->s, row_vals[body_col].val.string_val, len);
				body->len= len;

				body_array[i]= body;
			}			
		}
		pa_dbf.free_result(pa_db, result);
		result= NULL;
		
		notify_body = event->agg_nbody(&uri.user, &uri.host, body_array, n, build_off_n);
	}

done:	
	if(body_array!=NULL)
	{
		for(i= 0; i< n; i++)
		{
			if(body_array[i])
				pkg_free(body_array[i]);
		}
		pkg_free(body_array);
	}
	return notify_body;

error:
	if(result!=NULL)
		pa_dbf.free_result(pa_db, result);

	if(body_array!=NULL)
	{
		for(i= 0; i< n; i++)
		{
			if(body_array[i])
				pkg_free(body_array[i]);
			else
				break;

		}
	
		pkg_free(body_array);
	}
	return NULL;
}

int free_tm_dlg(dlg_t *td)
{
	if(td)
	{
		if(td->loc_uri.s)
			pkg_free(td->loc_uri.s);
		if(td->rem_uri.s)
			pkg_free(td->rem_uri.s);

		if(td->route_set)
			free_rr(&td->route_set);
		pkg_free(td);
	}
	return 0;
}

dlg_t* build_dlg_t(subs_t* subs)
{
	dlg_t* td =NULL;
	int found_contact = 1;

	td = (dlg_t*)pkg_malloc(sizeof(dlg_t));
	if(td == NULL)
	{
		ERR_MEM(PKG_MEM_STR);
	}
	memset(td, 0, sizeof(dlg_t));

	td->loc_seq.value = subs->local_cseq;
	td->loc_seq.is_set = 1;

	td->id.call_id = subs->callid;
	td->id.rem_tag = subs->from_tag;
	td->id.loc_tag =subs->to_tag;
	
	uandd_to_uri(subs->to_user, subs->to_domain, &td->loc_uri);
	if(td->loc_uri.s== NULL)
	{
		LM_ERR("while creating uri\n");
		goto error;
	}

	if(subs->contact.len ==0 || subs->contact.s == NULL )
	{
		found_contact = 0;
	}
	else
	{
		LM_DBG("CONTACT = %.*s\n", subs->contact.len , subs->contact.s);
		td->rem_target = subs->contact;
	}

	uandd_to_uri(subs->from_user, subs->from_domain, &td->rem_uri);
	if(td->rem_uri.s ==NULL)
	{
		LM_ERR("while creating uri\n");
		goto error;
	}
	
	if(found_contact == 0)
	{
		td->rem_target = td->rem_uri;
	}
	if(subs->record_route.s && subs->record_route.len)
	{
		if(parse_rr_body(subs->record_route.s, subs->record_route.len,
			&td->route_set)< 0)
		{
			LM_ERR("in function parse_rr_body\n");
			goto error;
		}
	}	
	td->state= DLG_CONFIRMED ;

	if (subs->sockinfo_str.len) {
		int port, proto;
        str host;
		char* tmp;
		if ((tmp = as_asciiz(&subs->sockinfo_str)) == NULL) {
			LM_ERR("no pkg memory left\n");
			goto error;
		}
		if (parse_phostport (tmp,&host.s,
				&host.len,&port, &proto )) {
			LM_ERR("bad sockinfo string\n");
			pkg_free(tmp);
			goto error;
		}
		pkg_free(tmp);
		td->send_sock = grep_sock_info (
			&host, (unsigned short) port, (unsigned short) proto);
	}
	
	return td;

error:		
	free_tm_dlg(td);
	return NULL;
}

int get_subs_db(str* pres_uri, pres_ev_t* event, str* sender,
		subs_t** s_array, int* n)
{
	db_key_t query_cols[7];
	db_op_t  query_ops[7];
	db_val_t query_vals[7];
	db_key_t result_cols[19];
	int n_result_cols = 0, n_query_cols = 0;
	db_row_t *row ;	
	db_val_t *row_vals ;
	db1_res_t *result = NULL;
	int from_user_col, from_domain_col, from_tag_col;
	int to_user_col, to_domain_col, to_tag_col;
	int expires_col= 0,callid_col, cseq_col, i, reason_col;
	int version_col= 0, record_route_col = 0, contact_col = 0;
	int sockinfo_col= 0, local_contact_col= 0, event_id_col = 0;
	subs_t s, *s_new;
	int inc= 0;
		
	if (pa_dbf.use_table(pa_db, &active_watchers_table) < 0) 
	{
		LM_ERR("in use_table\n");
		return -1;
	}

	LM_DBG("querying database table = active_watchers\n");
	query_cols[n_query_cols] = &str_presentity_uri_col;
	query_ops[n_query_cols] = OP_EQ;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = *pres_uri;
	n_query_cols++;
	
	query_cols[n_query_cols] = &str_event_col;
	query_ops[n_query_cols] = OP_EQ;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = event->name;
	n_query_cols++;

	query_cols[n_query_cols] = &str_status_col;
	query_ops[n_query_cols] = OP_EQ;
	query_vals[n_query_cols].type = DB1_INT;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.int_val = ACTIVE_STATUS;
	n_query_cols++;

	query_cols[n_query_cols] = &str_contact_col;
	query_ops[n_query_cols] = OP_NEQ;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	if(sender)
	{	
		LM_DBG("Do not send Notify to:[uri]= %.*s\n",sender->len,sender->s);
		query_vals[n_query_cols].val.str_val = *sender;
	} else {
		query_vals[n_query_cols].val.str_val.s = "";
		query_vals[n_query_cols].val.str_val.len = 0;
	}
	n_query_cols++;

	result_cols[to_user_col=n_result_cols++]      =   &str_to_user_col;
	result_cols[to_domain_col=n_result_cols++]    =   &str_to_domain_col;
	result_cols[from_user_col=n_result_cols++]    =   &str_watcher_username_col;
	result_cols[from_domain_col=n_result_cols++]  =   &str_watcher_domain_col;
	result_cols[event_id_col=n_result_cols++]     =   &str_event_id_col;
	result_cols[from_tag_col=n_result_cols++]     =   &str_from_tag_col;
	result_cols[to_tag_col=n_result_cols++]       =   &str_to_tag_col;
	result_cols[callid_col=n_result_cols++]       =   &str_callid_col;
	result_cols[cseq_col=n_result_cols++]         =   &str_local_cseq_col;
	result_cols[record_route_col=n_result_cols++] =   &str_record_route_col;
	result_cols[contact_col=n_result_cols++]      =   &str_contact_col;
	result_cols[expires_col=n_result_cols++]      =   &str_expires_col;
	result_cols[reason_col=n_result_cols++]       =   &str_reason_col;
	result_cols[sockinfo_col=n_result_cols++]     =   &str_socket_info_col;
	result_cols[local_contact_col=n_result_cols++]=   &str_local_contact_col;
	result_cols[version_col=n_result_cols++]      =   &str_version_col;

	if (pa_dbf.query(pa_db, query_cols, query_ops, query_vals,result_cols,
				n_query_cols, n_result_cols, 0, &result) < 0) 
	{
		LM_ERR("while querying database\n");
		if(result)
		{
			pa_dbf.free_result(pa_db, result);
		}
		return -1;
	}

	if(result== NULL)
		return -1;

	if(result->n <=0 )
	{
		LM_DBG("The query for subscribtion for [uri]= %.*s for [event]= %.*s"
			" returned no result\n",pres_uri->len, pres_uri->s,
			event->name.len, event->name.s);
		pa_dbf.free_result(pa_db, result);
		return 0;
	}
	LM_DBG("found %d dialogs\n", result->n);
	
	for(i=0; i<result->n; i++)
	{
		row = &result->rows[i];
		row_vals = ROW_VALUES(row);	
		
		//	if(row_vals[expires_col].val.int_val< (int)time(NULL))
		//		continue;

		if(row_vals[reason_col].val.string_val) {
		    if(strlen(row_vals[reason_col].val.string_val) != 0)
			continue;
		}

		//	s.reason.len= strlen(s.reason.s);

		memset(&s, 0, sizeof(subs_t));
		s.status= ACTIVE_STATUS;
		
		s.pres_uri= *pres_uri;
		s.to_user.s= (char*)row_vals[to_user_col].val.string_val;
		s.to_user.len= strlen(s.to_user.s);
		
		s.to_domain.s= (char*)row_vals[to_domain_col].val.string_val;
		s.to_domain.len= strlen(s.to_domain.s);
		
		s.from_user.s= (char*)row_vals[from_user_col].val.string_val;
		s.from_user.len= strlen(s.from_user.s);
		
		s.from_domain.s= (char*)row_vals[from_domain_col].val.string_val;
		s.from_domain.len= strlen(s.from_domain.s);
		
		s.event_id.s=(char*)row_vals[event_id_col].val.string_val;
		s.event_id.len= (s.event_id.s)?strlen(s.event_id.s):0;
		
		s.to_tag.s= (char*)row_vals[to_tag_col].val.string_val;
		s.to_tag.len= strlen(s.to_tag.s);
		
		s.from_tag.s= (char*)row_vals[from_tag_col].val.string_val; 
		s.from_tag.len= strlen(s.from_tag.s);
		
		s.callid.s= (char*)row_vals[callid_col].val.string_val;
		s.callid.len= strlen(s.callid.s);
		
		s.record_route.s=  (char*)row_vals[record_route_col].val.string_val;
		s.record_route.len= (s.record_route.s)?strlen(s.record_route.s):0;

		s.contact.s= (char*)row_vals[contact_col].val.string_val;
		s.contact.len= strlen(s.contact.s);
		
		s.sockinfo_str.s = (char*)row_vals[sockinfo_col].val.string_val;
		s.sockinfo_str.len = s.sockinfo_str.s?strlen(s.sockinfo_str.s):0;

		s.local_contact.s = (char*)row_vals[local_contact_col].val.string_val;
		s.local_contact.len = s.local_contact.s?strlen(s.local_contact.s):0;
		
		s.event= event;
		s.local_cseq = row_vals[cseq_col].val.int_val;
		if(row_vals[expires_col].val.int_val < (int)time(NULL))
		    s.expires = 0;
		else
		    s.expires = row_vals[expires_col].val.int_val -
			(int)time(NULL);
		s.version = row_vals[version_col].val.int_val;

		s_new= mem_copy_subs(&s, PKG_MEM_TYPE);
		if(s_new== NULL)
		{
			LM_ERR("while copying subs_t structure\n");
			goto error;
		}
		s_new->next= (*s_array);
		(*s_array)= s_new;
		printf_subs(s_new);
		inc++;
		
	}
	pa_dbf.free_result(pa_db, result);
	*n= inc;

	return 0;

error:
	if(result)
		pa_dbf.free_result(pa_db, result);
	
	return -1;
}

int update_in_list(subs_t* s, subs_t* s_array, int new_rec_no, int n)
{
	int i= 0;
	subs_t* ls;

	ls= s_array;
	
	while(i< new_rec_no)
	{
		i++;
		ls= ls->next;
	}

	for(i = 0; i< n; i++)
	{
		if(ls== NULL)
		{
			LM_ERR("wrong records count\n");
			return -1;
		}
		printf_subs(ls);
		
		if(ls->callid.len== s->callid.len &&
		strncmp(ls->callid.s, s->callid.s, s->callid.len)== 0 &&
		ls->to_tag.len== s->to_tag.len &&
		strncmp(ls->to_tag.s, s->to_tag.s, s->to_tag.len)== 0 &&
		ls->from_tag.len== s->from_tag.len &&
		strncmp(ls->from_tag.s, s->from_tag.s, s->from_tag.len)== 0 )
		{
			ls->local_cseq= s->local_cseq;
			ls->expires= s->expires- (int)time(NULL);
			ls->version= s->version;
			ls->status= s->status;
			return 1;
		}
		ls= ls->next;
	}
	return -1;
}

subs_t* get_subs_dialog(str* pres_uri, pres_ev_t* event, str* sender)
{
	unsigned int hash_code;
	subs_t* s= NULL, *s_new;
	subs_t* s_array= NULL;
	int n= 0, i= 0;
	
	/* if fallback2db -> should take all dialogs from db
	 * and the only those dialogs from cache with db_flag= INSERTDB_FLAG */

	if(fallback2db)
	{
		if(get_subs_db(pres_uri, event, sender, &s_array, &n)< 0)			
		{
			LM_ERR("getting dialogs from database\n");
			goto error;
		}
	}
	hash_code= core_hash(pres_uri, &event->name, shtable_size);
	
	lock_get(&subs_htable[hash_code].lock);

	s= subs_htable[hash_code].entries;

	while(s->next)
	{
		s= s->next;
	
		printf_subs(s);
		
		if(s->expires< (int)time(NULL))
		{
			LM_DBG("expired subs\n");
			continue;
		}
		
		if((!(s->status== ACTIVE_STATUS &&
            s->reason.len== 0 &&
			s->event== event && s->pres_uri.len== pres_uri->len &&
			strncmp(s->pres_uri.s, pres_uri->s, pres_uri->len)== 0)) || 
			(sender && sender->len== s->contact.len && 
			strncmp(sender->s, s->contact.s, sender->len)== 0))
			continue;

		if(fallback2db)
		{
			if(s->db_flag== NO_UPDATEDB_FLAG)
			{
				LM_DBG("s->db_flag==NO_UPDATEDB_FLAG\n");
				continue;
			}
			
			if(s->db_flag== UPDATEDB_FLAG)
			{
				LM_DBG("s->db_flag== UPDATEDB_FLAG\n");
				if(n>0 && update_in_list(s, s_array, i, n)< 0)
				{
					LM_DBG("dialog not found in list fetched from database\n");
					/* insert record */
				}
				else
					continue;			
			}
		}
		
		LM_DBG("s->db_flag= INSERTDB_FLAG\n");
		s_new= mem_copy_subs(s, PKG_MEM_TYPE);
		if(s_new== NULL)
		{
			LM_ERR("copying subs_t structure\n");
			lock_release(&subs_htable[hash_code].lock);
			goto error;
		}
		s_new->expires-= (int)time(NULL);
		s_new->next= s_array;
		s_array= s_new;
		i++;
	}

	lock_release(&subs_htable[hash_code].lock);
	LM_DBG("found %d dialogs( %d in database and %d in hash_table)\n",n+i,n,i);

	return s_array;

error:
	free_subs_list(s_array, PKG_MEM_TYPE, 0);
	return NULL;
	
}

int publ_notify(presentity_t* p, str pres_uri, str* body, str* offline_etag, str* rules_doc)
{
	str *notify_body = NULL, *aux_body = NULL;
	subs_t* subs_array= NULL, *s= NULL;
	int ret_code= -1;

	subs_array= get_subs_dialog(&pres_uri, p->event , p->sender);
	if(subs_array == NULL)
	{
		LM_DBG("Could not find subs_dialog\n");
		ret_code= 0;
		goto done;
	}

	/* if the event does not require aggregation - we have the final body */
	if(p->event->agg_nbody)
	{	
		notify_body = get_p_notify_body(pres_uri, p->event , offline_etag, NULL);
		if(notify_body == NULL)
		{
			LM_DBG("Could not get the notify_body\n");
			/* goto error; */
		}
	}

	s= subs_array;
	while(s)
	{
		s->auth_rules_doc= rules_doc;
		if (p->event->aux_body_processing) {
			aux_body = p->event->aux_body_processing(s, notify_body?notify_body:body);
		}

		if(notify(s, NULL, aux_body?aux_body:(notify_body?notify_body:body), 0)< 0 )
		{
			LM_ERR("Could not send notify for %.*s\n",
					p->event->name.len, p->event->name.s);
		}

		if(aux_body!=NULL) {
			if(aux_body->s)	{
				p->event->aux_free_body(aux_body->s);
			}
			pkg_free(aux_body);
		}
		s= s->next;
	}
	ret_code= 0;

done:
	free_subs_list(subs_array, PKG_MEM_TYPE, 0);
	
	if(notify_body!=NULL)
	{
		if(notify_body->s)
		{
			if(	p->event->agg_nbody== NULL && p->event->apply_auth_nbody== NULL)
				pkg_free(notify_body->s);
			else
				p->event->free_body(notify_body->s);
		}
		pkg_free(notify_body);
	}
	return ret_code;
}	

int query_db_notify(str* pres_uri, pres_ev_t* event, subs_t* watcher_subs )
{
	subs_t* subs_array = NULL, *s= NULL;
	str* notify_body = NULL, *aux_body = NULL;
	int ret_code= -1;

	subs_array= get_subs_dialog(pres_uri, event , NULL);
	if(subs_array == NULL)
	{
		LM_DBG("Could not get subscription dialog\n");
		ret_code= 1;
		goto done;
	}
	
	if(event->type & PUBL_TYPE)
	{
		notify_body = get_p_notify_body(*pres_uri, event, NULL, NULL);
		if(notify_body == NULL)
		{
			LM_DBG("Could not get the notify_body\n");
			/* goto error; */
		}
	}	

	s= subs_array;
	
	while(s)
	{

		if (event->aux_body_processing) {
			aux_body = event->aux_body_processing(s, notify_body);
		}

		if(notify(s, watcher_subs, aux_body?aux_body:notify_body, 0)< 0 )
		{
			LM_ERR("Could not send notify for [event]=%.*s\n",
					event->name.len, event->name.s);
			goto done;
		}

		if(aux_body!=NULL) {
			if(aux_body->s)	{
				event->aux_free_body(aux_body->s);
			}
			pkg_free(aux_body);
		}
		s= s->next;
	}

	ret_code= 1;

done:
	free_subs_list(subs_array, PKG_MEM_TYPE, 0);
	if(notify_body!=NULL)
	{
		if(notify_body->s)
		{
			if(event->type & WINFO_TYPE)
				pkg_free(notify_body->s);
			else
			if(event->agg_nbody== NULL && event->apply_auth_nbody== NULL)
				pkg_free(notify_body->s);
			else
				event->free_body(notify_body->s);
		}
		pkg_free(notify_body);
	}

	return ret_code;
}

int send_notify_request(subs_t* subs, subs_t * watcher_subs,
		str* n_body,int force_null_body)
{
	dlg_t* td = NULL;
	str met = {"NOTIFY", 6};
	str str_hdr = {0, 0};
	str* notify_body = NULL;
	int result= 0;
    c_back_param *cb_param= NULL;
	str* final_body= NULL;
	uac_req_t uac_r;
	
	LM_DBG("dialog info:\n");
	printf_subs(subs);

    /* getting the status of the subscription */

	if(force_null_body)
	{
		goto jump_over_body;
	}

	if(n_body!= NULL && subs->status== ACTIVE_STATUS)
	{
		if( subs->event->req_auth)
		{
			
			if(subs->auth_rules_doc && subs->event->apply_auth_nbody)
			{
				if(subs->event->apply_auth_nbody(n_body, subs, &notify_body)< 0)
				{
					LM_ERR("in function apply_auth_nbody\n");
					goto error;
				}
			}
			if(notify_body== NULL)
				notify_body= n_body;
		}
		else
			notify_body= n_body;
	}	
	else
	{	
		if(subs->status== TERMINATED_STATUS || 
				subs->status== PENDING_STATUS) 
		{
			LM_DBG("state terminated or pending- notify body NULL\n");
			notify_body = NULL;
		}
		else  
		{		
			if(subs->event->type & WINFO_TYPE)	
			{	
				notify_body = get_wi_notify_body(subs, watcher_subs);
				if(notify_body == NULL)
				{
					LM_DBG("Could not get notify_body\n");
					goto error;
				}
			}
			else
			{
				notify_body = get_p_notify_body(subs->pres_uri,
					subs->event, NULL, (subs->contact.s)?&subs->contact:NULL);
				if(notify_body == NULL || notify_body->s== NULL)
				{
					LM_DBG("Could not get the notify_body\n");
				}
				else		/* apply authorization rules if exists */
				if(subs->event->req_auth)
				{
					 
					if(subs->auth_rules_doc && subs->event->apply_auth_nbody
							&& subs->event->apply_auth_nbody(notify_body,
								subs,&final_body)<0)
					{
						LM_ERR("in function apply_auth\n");
						goto error;
					}
					if(final_body)
					{
						xmlFree(notify_body->s);
						pkg_free(notify_body);
						notify_body= final_body;
					}
				}
			}
		}
	}
	
jump_over_body:

	if(subs->expires<= 0)
	{
        subs->expires= 0;
		subs->status= TERMINATED_STATUS;
		subs->reason.s= "timeout";
		subs->reason.len= 7;
	}

	/* build extra headers */
	if( build_str_hdr( subs, notify_body?1:0, &str_hdr)< 0 )
	{
		LM_ERR("while building headers\n");
		goto error;
	}	
	LM_DBG("headers:\n%.*s\n", str_hdr.len, str_hdr.s);

	/* construct the dlg_t structure */
	td = build_dlg_t(subs);
	if(td ==NULL)
	{
		LM_ERR("while building dlg_t structure\n");
		goto error;	
	}

	cb_param = shm_dup_cbparam(subs);
	if(cb_param == NULL)
	{
		LM_ERR("while duplicating cb_param in share memory\n");
		goto error;	
	}	

	set_uac_req(&uac_r, &met, &str_hdr, notify_body, td, TMCB_LOCAL_COMPLETED,
			p_tm_callback, (void*)cb_param);
	result = tmb.t_request_within(&uac_r);
	if(result< 0)
	{
		LM_ERR("in function tmb.t_request_within\n");
		free_cbparam(cb_param);
		goto error;	
	}

	LM_INFO("NOTIFY %.*s via %.*s on behalf of %.*s for event %.*s\n",
		td->rem_uri.len, td->rem_uri.s, td->hooks.next_hop->len,
		td->hooks.next_hop->s,
		td->loc_uri.len, td->loc_uri.s, subs->event->name.len,
		subs->event->name.s);

	free_tm_dlg(td);
	
	if(str_hdr.s) pkg_free(str_hdr.s);
	
	if((int)(long)n_body!= (int)(long)notify_body)
	{
		if(notify_body!=NULL)
		{
			if(notify_body->s!=NULL)
			{
				if(subs->event->type& WINFO_TYPE )
					xmlFree(notify_body->s);
				else
				if(subs->event->apply_auth_nbody== NULL
						&& subs->event->agg_nbody== NULL)
					pkg_free(notify_body->s);
				else
				subs->event->free_body(notify_body->s);
			}
			pkg_free(notify_body);
		}
	}	
	return 0;

error:
	free_tm_dlg(td);
	if(str_hdr.s!=NULL)
		pkg_free(str_hdr.s);
	if((int)(long)n_body!= (int)(long)notify_body)
	{
		if(notify_body!=NULL)
		{
			if(notify_body->s!=NULL)
			{
				if(subs->event->type& WINFO_TYPE)
					xmlFree(notify_body->s);
				else
				if(subs->event->apply_auth_nbody== NULL
						&& subs->event->agg_nbody== NULL)
					pkg_free(notify_body->s);
				else
				subs->event->free_body(notify_body->s);
			}
			pkg_free(notify_body);
		}
	}	
	return -1;
}


int notify(subs_t* subs, subs_t * watcher_subs,str* n_body,int force_null_body)
{
	/* update first in hash table and the send Notify */
	if(subs->expires!= 0 && subs->status != TERMINATED_STATUS)
	{
		unsigned int hash_code;
		hash_code= core_hash(&subs->pres_uri, &subs->event->name, shtable_size);

		if(update_shtable(subs_htable, hash_code, subs, LOCAL_TYPE)< 0)
		{
			if(subs->db_flag!= INSERTDB_FLAG && fallback2db)
			{
				LM_DBG("record not found in subs htable\n");
				if(update_subs_db(subs, LOCAL_TYPE)< 0)
				{
					LM_ERR("updating subscription in database\n");
					return -1;
				}
			}
			else
			{
				LM_ERR("record not found in subs htable\n");
				return -1;
			}
		}
	}
     
    if(subs->reason.s && subs->status== ACTIVE_STATUS && 
        subs->reason.len== 12 && strncmp(subs->reason.s, "polite-block", 12)== 0)
    {
        force_null_body = 1;
    }

	if(send_notify_request(subs, watcher_subs, n_body, force_null_body)< 0)
	{
		LM_ERR("sending Notify not successful\n");
		return -1;
	}
	return 0;
}

void p_tm_callback( struct cell *t, int type, struct tmcb_params *ps)
{
	if(ps->param==NULL || *ps->param==NULL || 
			((c_back_param*)(*ps->param))->pres_uri.s == NULL || 
			((c_back_param*)(*ps->param))->ev_name.s== NULL ||
			((c_back_param*)(*ps->param))->to_tag.s== NULL)
	{
		LM_DBG("message id not received\n");
		if(*ps->param !=NULL  )
			free_cbparam((c_back_param*)(*ps->param));
		return;
	}
	
	LM_DBG("completed with status %d [to_tag:%.*s]\n",
			ps->code,((c_back_param*)(*ps->param))->to_tag.len,
			((c_back_param*)(*ps->param))->to_tag.s);

	if(ps->code >= 300 && (ps->code != 408 || timeout_rm_subs))
	{
		unsigned int hash_code;

		c_back_param*  cb= (c_back_param*)(*ps->param);

		hash_code= core_hash(&cb->pres_uri, &cb->ev_name, shtable_size);
		delete_shtable(subs_htable, hash_code, cb->to_tag);

		delete_db_subs(cb->pres_uri, cb->ev_name, cb->to_tag);
	}	

	if(*ps->param !=NULL  )
		free_cbparam((c_back_param*)(*ps->param));
	return ;
}

void free_cbparam(c_back_param* cb_param)
{
	if(cb_param!= NULL)
		shm_free(cb_param);
}

c_back_param* shm_dup_cbparam(subs_t* subs)
{
	int size;
	c_back_param* cb_param = NULL;
	
	size = sizeof(c_back_param) + subs->pres_uri.len+
			subs->event->name.len + subs->to_tag.len;

	cb_param= (c_back_param*)shm_malloc(size);
	LM_DBG("=== %d/%d/%d\n", subs->pres_uri.len,
			subs->event->name.len, subs->to_tag.len);
	if(cb_param==NULL)
	{
		LM_ERR("no more shared memory\n");
		return NULL;
	}
	memset(cb_param, 0, size);

	cb_param->pres_uri.s = (char*)cb_param + sizeof(c_back_param);
	memcpy(cb_param->pres_uri.s, subs->pres_uri.s, subs->pres_uri.len);
	cb_param->pres_uri.len = subs->pres_uri.len;
	cb_param->ev_name.s = (char*)(cb_param->pres_uri.s) + cb_param->pres_uri.len;
	memcpy(cb_param->ev_name.s, subs->event->name.s, subs->event->name.len);
	cb_param->ev_name.len = subs->event->name.len;
	cb_param->to_tag.s = (char*)(cb_param->ev_name.s) + cb_param->ev_name.len;
	memcpy(cb_param->to_tag.s, subs->to_tag.s, subs->to_tag.len);
	cb_param->to_tag.len = subs->to_tag.len;

	return cb_param;
}


str* create_winfo_xml(watcher_t* watchers, char* version,
		str resource, str event, int STATE_FLAG)
{
	xmlDocPtr doc = NULL;       
    xmlNodePtr root_node = NULL, node = NULL;
	xmlNodePtr w_list_node = NULL;	
	char content[200];
	str *body= NULL;
	char* res= NULL;
	watcher_t* w;

    LIBXML_TEST_VERSION;
    
	doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "watcherinfo");
    xmlDocSetRootElement(doc, root_node);

    xmlNewProp(root_node, BAD_CAST "xmlns",
			BAD_CAST "urn:ietf:params:xml:ns:watcherinfo");
    xmlNewProp(root_node, BAD_CAST "version", BAD_CAST version );
   
	if(STATE_FLAG & FULL_STATE_FLAG)
	{
		if( xmlNewProp(root_node, BAD_CAST "state", BAD_CAST "full") == NULL)
		{
			LM_ERR("while adding new attribute\n");
			goto error;
		}
	}
	else	
	{	
		if( xmlNewProp(root_node, BAD_CAST "state", 
					BAD_CAST "partial")== NULL) 
		{
			LM_ERR("while adding new attribute\n");
			goto error;
		}
	}

	w_list_node =xmlNewChild(root_node, NULL, BAD_CAST "watcher-list",NULL);
	if( w_list_node == NULL)
	{
		LM_ERR("while adding child\n");
		goto error;
	}
	res= (char*)pkg_malloc(MAX_unsigned(resource.len, event.len) + 1);
	if(res== NULL)
	{
		ERR_MEM(PKG_MEM_STR);
	}
	memcpy(res, resource.s, resource.len);
	res[resource.len]= '\0';
	xmlNewProp(w_list_node, BAD_CAST "resource", BAD_CAST res);
	memcpy(res, event.s, event.len);
	res[event.len]= '\0';
	xmlNewProp(w_list_node, BAD_CAST "package", BAD_CAST res);
	pkg_free(res);


	w= watchers->next;
	while(w)
	{
		strncpy( content,w->uri.s, w->uri.len);
		content[ w->uri.len ]='\0';
		node = xmlNewChild(w_list_node, NULL, BAD_CAST "watcher",
				BAD_CAST content) ;
		if( node ==NULL)
		{
			LM_ERR("while adding child\n");
			goto error;
		}
		if(xmlNewProp(node, BAD_CAST "id", BAD_CAST w->id.s)== NULL)
		{
			LM_ERR("while adding new attribute\n");
			goto error;
		}	
		
		if(xmlNewProp(node, BAD_CAST "event", BAD_CAST "subscribe")== NULL)
		{
			LM_ERR("while adding new attribute\n");
			goto error;
		}	
		
		if(xmlNewProp(node, BAD_CAST "status", 
					BAD_CAST get_status_str(w->status))== NULL)
		{
			LM_ERR("while adding new attribute\n");
			goto error;
		}
		w= w->next;
	}
    body = (str*)pkg_malloc(sizeof(str));
	if(body == NULL)
	{
		ERR_MEM(PKG_MEM_STR);	
	}
	memset(body, 0, sizeof(str));

	xmlDocDumpFormatMemory(doc,(xmlChar**)(void*)&body->s, &body->len, 1);

	xmlFreeDoc(doc);

	xmlCleanupParser();

    xmlMemoryDump();

    return body;

error:
    if(doc)
		xmlFreeDoc(doc);
	return NULL;
}

int watcher_found_in_list(watcher_t * watchers, str wuri)
{
	watcher_t * w;

	w = watchers->next;

	while(w)
	{
		if(w->uri.len == wuri.len && strncmp(w->uri.s, wuri.s, wuri.len)== 0)
			return 1;
		w= w->next;
	}

	return 0;
}

int add_waiting_watchers(watcher_t *watchers, str pres_uri, str event)
{
	watcher_t * w;
	db_key_t query_cols[3];
	db_val_t query_vals[3];
	db_key_t result_cols[2];
	db1_res_t *result = NULL;
	db_row_t *row= NULL ;	
	db_val_t *row_vals;
	int n_result_cols = 0;
	int n_query_cols = 0;
	int wuser_col, wdomain_col;
	str wuser, wdomain, wuri;
	int i;

	/* select from watchers table the users that have subscribed
	 * to the presentity and have status pending */

	query_cols[n_query_cols] = &str_presentity_uri_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = pres_uri;
	n_query_cols++;

	query_cols[n_query_cols] = &str_event_col;
	query_vals[n_query_cols].type = DB1_STR;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.str_val = event;
	n_query_cols++;

	query_cols[n_query_cols] = &str_status_col;
	query_vals[n_query_cols].type = DB1_INT;
	query_vals[n_query_cols].nul = 0;
	query_vals[n_query_cols].val.int_val = PENDING_STATUS;
	n_query_cols++;

	result_cols[wuser_col=n_result_cols++] = &str_watcher_username_col;
	result_cols[wdomain_col=n_result_cols++] = &str_watcher_domain_col;
	
	if (pa_dbf.use_table(pa_db, &watchers_table) < 0) 
	{
		LM_ERR("sql use table 'watchers_table' failed\n");
		return -1;
	}

	if (pa_dbf.query (pa_db, query_cols, 0, query_vals,
		 result_cols, n_query_cols, n_result_cols, 0, &result) < 0) 
	{
		LM_ERR("failed to query %.*s table\n",
				watchers_table.len, watchers_table.s);
		if(result)
			pa_dbf.free_result(pa_db, result);
		return -1;
	}
	
	if(result== NULL)
	{
		LM_ERR("mysql query failed - null result\n");
		return -1;
	}

	if (result->n<=0 )
	{
		LM_DBG("The query returned no result\n");
		pa_dbf.free_result(pa_db, result);
		return 0;
	}

	for(i=0; i< result->n; i++)
	{
		row = &result->rows[i];
		row_vals = ROW_VALUES(row);

		wuser.s = (char*)row_vals[wuser_col].val.string_val;
		wuser.len = strlen(wuser.s);

		wdomain.s = (char*)row_vals[wdomain_col].val.string_val;
		wdomain.len = strlen(wdomain.s);

		if(uandd_to_uri(wuser, wdomain, &wuri)<0)
		{
			LM_ERR("creating uri from username and domain\n");
			goto error;
		}

		if(watcher_found_in_list(watchers, wuri))
		{
			pkg_free(wuri.s);
			continue;
		}
		
		w= (watcher_t*)pkg_malloc(sizeof(watcher_t));
		if(w== NULL)
		{
			ERR_MEM(PKG_MEM_STR);
		}
		memset(w, 0, sizeof(watcher_t));

		w->status= WAITING_STATUS;
		w->uri = wuri;
		w->id.s = (char*)pkg_malloc(w->uri.len*2 +1);
		if(w->id.s== NULL)
		{
			pkg_free(w->uri.s);
			pkg_free(w);
			ERR_MEM(PKG_MEM_STR);
		}

		to64frombits((unsigned char *)w->id.s,
			(const unsigned char*)w->uri.s, w->uri.len);
		w->id.len = strlen(w->id.s);
		w->event= event;
	
		w->next= watchers->next;
		watchers->next= w;

	}

	pa_dbf.free_result(pa_db, result);
	return 0;

error:
	if(result)
		pa_dbf.free_result(pa_db, result);
	return -1;

}

