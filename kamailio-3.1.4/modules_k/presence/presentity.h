/*
 * $Id$
 *
 * presence module - presence server implementation
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
 * \brief Kamailio presence module :: Presentity handling
 * \ref presentity.c
 * \ingroup presence 
 */


#ifndef PRESENTITY_H
#define PRESENTITY_H

#include "../../str.h"
#include "../../parser/msg_parser.h" 
#include "event_list.h"
//#include "presence.h"

extern char prefix;

typedef struct presentity
{
	int presid;
	str user;
	str domain;
	pres_ev_t* event;
	str etag;
	str* sender;
	time_t expires;
	time_t received_time;
} presentity_t;

/* create new presentity */
presentity_t* new_presentity( str* domain,str* user,int expires, 
 		pres_ev_t* event, str* etag, str* sender);

/* update presentity in database */
int update_presentity(struct sip_msg* msg,presentity_t* p,str* body,int t_new,
		int* sent_reply, char* sphere);

/* free memory */
void free_presentity(presentity_t* p);

char* generate_ETag(int publ_count);

int pres_htable_restore(void);

char* extract_sphere(str body);

char* get_sphere(str* pres_uri);
typedef char* (*pres_get_sphere_t)(str* pres_uri);


#endif

