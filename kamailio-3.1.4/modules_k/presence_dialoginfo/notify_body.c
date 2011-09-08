/*
 * $Id: notify_body.c 1337 2006-12-07 18:05:05Z bogdan_iancu $
 *
 * presence_dialoginfo module -  
 *
 * Copyright (C) 2006 Voice Sistem S.R.L.
 * Copyright (C) 2008 Klaus Darilion, IPCom
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
 *  2008-08-25  initial version (kd)
 */
/*! \file
 * \brief Kamailio Presence_XML :: Notify BODY handling
 * \ingroup presence_xml
 */

#define MAX_INT_LEN 11 /* 2^32: 10 chars + 1 char sign */

#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>

#include "../../mem/mem.h"
#include "../presence/utils_func.h"
#include "../presence/hash.h"
#include "../presence/event_list.h"
#include "../presence/presence.h"
#include "../presence/presentity.h"
#include "notify_body.h"
#include "pidf.h"

str* agregate_xmls(str* pres_user, str* pres_domain, str** body_array, int n);
extern int force_single_dialog;

void free_xml_body(char* body)
{
	if(body== NULL)
		return;

	xmlFree(body);
	body= NULL;
}


str* dlginfo_agg_nbody(str* pres_user, str* pres_domain, str** body_array, int n, int off_index)
{
	str* n_body= NULL;

	LM_DBG("[pres_user]=%.*s [pres_domain]= %.*s, [n]=%d\n",
			pres_user->len, pres_user->s, pres_domain->len, pres_domain->s, n);

	if(body_array== NULL)
		return NULL;

	n_body= agregate_xmls(pres_user, pres_domain, body_array, n);
	LM_DBG("[n_body]=%p\n", n_body);
	if(n_body) {
		LM_DBG("[*n_body]=%.*s\n",
			n_body->len, n_body->s);
	}
	if(n_body== NULL && n!= 0)
	{
		LM_ERR("while aggregating body\n");
	}

	xmlCleanupParser();
    xmlMemoryDump();

	return n_body;
}	

str* agregate_xmls(str* pres_user, str* pres_domain, str** body_array, int n)
{
	int i, j= 0;

	xmlDocPtr  doc = NULL;
	xmlNodePtr root_node = NULL;
	xmlNsPtr   namespace = NULL;

	xmlNodePtr p_root= NULL;
	xmlDocPtr* xml_array ;
	xmlNodePtr node = NULL;
	char *state;
	int winner_priority = -1, priority ;
	xmlNodePtr winner_dialog_node = NULL ;
	str *body= NULL;
    char buf[MAX_URI_SIZE+1];

	LM_DBG("[pres_user]=%.*s [pres_domain]= %.*s, [n]=%d\n",
			pres_user->len, pres_user->s, pres_domain->len, pres_domain->s, n);

	xml_array = (xmlDocPtr*)pkg_malloc( n*sizeof(xmlDocPtr));
	if(xml_array== NULL)
	{
		LM_ERR("while allocating memory");
		return NULL;
	}
	memset(xml_array, 0, n*sizeof(xmlDocPtr)) ;

	/* parse all the XML documents */
	for(i=0; i<n; i++)
	{
		if(body_array[i] == NULL )
			continue;

		xml_array[j] = NULL;
		xml_array[j] = xmlParseMemory( body_array[i]->s, body_array[i]->len );
		
		/* LM_DBG("parsing XML body: [n]=%d, [i]=%d, [j]=%d xml_array[j]=%p\n", n, i, j, xml_array[j] ); */

		if( xml_array[j]== NULL)
		{
			LM_ERR("while parsing xml body message\n");
			goto error;
		}
		j++;

	} 

	if(j== 0)  /* no body */
	{
		if(xml_array)
			pkg_free(xml_array);
		return NULL;
	}

	/* n: number of bodies in total */
	/* j: number of useful bodies; created XML structures */
	/* i: loop counter */
	/* LM_DBG("number of bodies in total [n]=%d, number of useful bodies [j]=%d\n", n, j ); */

	/* create the new NOTIFY body  */
    if ( (pres_user->len + pres_domain->len + 1) > MAX_URI_SIZE) {
        LM_ERR("entity URI too long, maximum=%d\n", MAX_URI_SIZE);
        return NULL;
    }
    memcpy(buf, pres_user->s, pres_user->len);
    buf[pres_user->len] = '@';
    memcpy(buf + pres_user->len + 1, pres_domain->s, pres_domain->len);
    buf[pres_user->len + 1 + pres_domain->len]= '\0';

    doc = xmlNewDoc(BAD_CAST "1.0");
    if(doc==0)
        return NULL;

    root_node = xmlNewNode(NULL, BAD_CAST "dialog-info");
    if(root_node==0)
        goto error;

    xmlDocSetRootElement(doc, root_node);
	namespace = xmlNewNs(root_node, BAD_CAST "urn:ietf:params:xml:ns:dialog-info", NULL);
	if (!namespace) {
		LM_ERR("creating namespace failed\n");
	}
	xmlSetNs(root_node, namespace);
	/* The version must be increased for each new document and is a 32bit int.
       As the version is different for each watcher, we can not set here the
       correct value. Thus, we just put here a placeholder which will be 
	   replaced by the correct value in the aux_body_processing callback.
	   Thus we have CPU intensive XML aggregation only once and can use
	   quick search&replace in the per-watcher aux_body_processing callback.
	   We use 11 chracters as an signed int (although RFC says unsigned int we
	   use signed int as presence module stores "version" in DB as
	   signed int) has max. 10 characters + 1 character for the sign
	*/
    xmlNewProp(root_node, BAD_CAST "version", BAD_CAST "00000000000");
    xmlNewProp(root_node, BAD_CAST  "state",  BAD_CAST "full" );
    xmlNewProp(root_node, BAD_CAST "entity",  BAD_CAST buf);

	/* loop over all bodies and create the aggregated body */
	for(i=0; i<j; i++)
	{
		/* LM_DBG("[n]=%d, [i]=%d, [j]=%d xml_array[i]=%p\n", n, i, j, xml_array[j] ); */
		p_root= xmlDocGetRootElement(xml_array[i]);
			if(p_root ==NULL) {
				LM_ERR("while geting the xml_tree root element\n");
				goto error;
			}
			if (p_root->children) {
			for (node = p_root->children; node; node = node->next) {
				if (node->type == XML_ELEMENT_NODE) {
					LM_DBG("node type: Element, name: %s\n", node->name);
					/* we do not copy the node, but unlink it and then add it ot the new node
					 * this destroys the original document but we do not need it anyway.
					 * using "copy" instead of "unlink" would also copy the namespace which 
					 * would then be declared redundant (libxml unfortunately can not remove 
					 * namespaces)
					 */
					if (!force_single_dialog || (j==1)) {
						xmlUnlinkNode(node);
						if(xmlAddChild(root_node, node)== NULL) {
							LM_ERR("while adding child\n");
							goto error;
						}
					} else {
						/* try to put only the most important into the XML document
						 * order of importance: terminated->trying->proceeding->confirmed->early
						 */
						state = xmlNodeGetNodeContentByName(node, "state", NULL);
						if (state) {
							LM_DBG("state element content = %s\n", state);
							priority = get_dialog_state_priority(state);
							if (priority > winner_priority) {
								winner_priority = priority;
								LM_DBG("new winner priority = %s (%d)\n", state, winner_priority);
								winner_dialog_node = node;
							}
							xmlFree(state);
						}
					}
				}
			}
		}
	}

	if (force_single_dialog && (j!=1)) {
		xmlUnlinkNode(winner_dialog_node);
		if(xmlAddChild(root_node, winner_dialog_node)== NULL) {
			LM_ERR("while adding winner-child\n");
			goto error;
		}
	}

	body = (str*)pkg_malloc(sizeof(str));
	if(body == NULL) {
		ERR_MEM(PKG_MEM_STR);
	}

	xmlDocDumpFormatMemory(doc,(xmlChar**)(void*)&body->s, 
			&body->len, 1);	

  	for(i=0; i<j; i++)
	{
		if(xml_array[i]!=NULL)
			xmlFreeDoc( xml_array[i]);
	}
	if (doc)
		xmlFreeDoc(doc);
	if(xml_array!=NULL)
		pkg_free(xml_array);
    
	xmlCleanupParser();
    xmlMemoryDump();

	return body;

error:
	if(xml_array!=NULL)
	{
		for(i=0; i<j; i++)
		{
			if(xml_array[i]!=NULL)
				xmlFreeDoc( xml_array[i]);
		}
		pkg_free(xml_array);
	}
	if(body)
		pkg_free(body);

	return NULL;
}


int get_dialog_state_priority(char *state) {
	if (strcasecmp(state,"terminated") == 0)
		return 0;
	if (strcasecmp(state,"trying") == 0)
		return 1;
	if (strcasecmp(state,"proceeding") == 0)
		return 2;
	if (strcasecmp(state,"confirmed") == 0)
		return 3;
	if (strcasecmp(state,"early") == 0)
		return 4;

	return 0;
}


str *dlginfo_body_setversion(subs_t *subs, str *body) {
	char *version_start=0;
	char version[MAX_INT_LEN + 2]; /* +2 becasue of trailing " and \0 */
	int version_len;

	if (!body) {
		return NULL;
	}

	/* xmlDocDumpFormatMemory creates \0 terminated string */
	/* version parameters starts at minimum at character 34 */
	if (body->len < 41) {
		LM_ERR("body string too short!\n");
		return NULL;
	}
	version_start = strstr(body->s + 34, "version=");
	if (!version_start) {
	    LM_ERR("version string not found!\n");
		return NULL;
	}
	version_start += 9;

	version_len = snprintf(version, MAX_INT_LEN + 2,"%d\"", subs->version);
	if (version_len >= MAX_INT_LEN + 2) {
		LM_ERR("failed to convert 'version' to string\n");
		memcpy(version_start, "00000000000\"", 12);
		return NULL;
	}
	/* Replace the placeholder 00000000000 with the version.
	 * Put the padding behind the ""
	 */
	LM_DBG("replace version with \"%s\n",version);
	memcpy(version_start, version, version_len);
	memset(version_start + version_len, ' ', 12 - version_len);

	return NULL;
}
