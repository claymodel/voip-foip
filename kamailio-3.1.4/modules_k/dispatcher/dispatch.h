/**
 * $Id$
 *
 * dispatcher module
 * 
 * Copyright (C) 2004-2006 FhG Fokus
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
 * History
 * -------
 * 2004-07-31  first version, by daniel
 * 2007-01-11  Added a function to check if a specific gateway is in
 *		a group (carsten)
 * 2007-02-09  Added active probing of failed destinations and automatic
 *		re-enabling of destinations
 * 2007-05-08  Ported the changes to SVN-Trunk and renamed ds_is_domain
 *		to ds_is_from_list.
 */

/*! \file
 * \ingroup dispatcher
 * \brief Dispatcher :: Dispatch
 */

#ifndef _DISPATCH_H_
#define _DISPATCH_H_

#include <stdio.h>
#include "../../pvar.h"
#include "../../parser/msg_parser.h"
#include "../../modules/tm/tm_load.h"


#define DS_HASH_USER_ONLY	1  /*!< use only the uri user part for hashing */
#define DS_FAILOVER_ON		2  /*!< store the other dest in avps */

#define DS_INACTIVE_DST		1  /*!< inactive destination */
#define DS_PROBING_DST		2  /*!< checking destination */
#define DS_RESET_FAIL_DST	4  /*!< Reset-Failure-Counter */

extern str ds_db_url;
extern str ds_table_name;
extern str ds_set_id_col;
extern str ds_dest_uri_col;
extern str ds_dest_flags_col;
extern str ds_dest_priority_col;
extern str ds_dest_attrs_col;

extern int ds_flags; 
extern int ds_use_default;

extern int_str dst_avp_name;
extern unsigned short dst_avp_type;
extern int_str grp_avp_name;
extern unsigned short grp_avp_type;
extern int_str cnt_avp_name;
extern unsigned short cnt_avp_type;
extern int_str dstid_avp_name;
extern unsigned short dstid_avp_type;
extern int_str attrs_avp_name;
extern unsigned short attrs_avp_type;

extern pv_elem_t * hash_param_model;

extern str ds_setid_pvname;
extern pv_spec_t ds_setid_pv;

/* Structure containing pointers to TM-functions */
extern struct tm_binds tmb;
extern str ds_ping_method;
extern str ds_ping_from;
extern int probing_threshhold; /*!< number of failed requests,
						before a destination is taken into probing */ 
extern int ds_probing_mode;

int init_data(void);
int init_ds_db(void);
int ds_load_list(char *lfile);
int ds_connect_db(void);
void ds_disconnect_db(void);
int ds_load_db(void);
int ds_destroy_list(void);
int ds_select_dst(struct sip_msg *msg, int set, int alg, int mode);
int ds_next_dst(struct sip_msg *msg, int mode);
int ds_set_state(int group, str *address, int state, int type);
int ds_mark_dst(struct sip_msg *msg, int mode);
int ds_print_list(FILE *fout);
int ds_print_mi_list(struct mi_node* rpl);
int ds_print_sets(void);

int ds_load_unset(struct sip_msg *msg);
int ds_load_update(struct sip_msg *msg);

int ds_hash_load_init(unsigned int htsize, int expire, int initexpire);
int ds_hash_load_destroy(void);

int ds_is_from_list(struct sip_msg *_m, int group);
/*! \brief
 * Timer for checking inactive destinations
 */
void ds_check_timer(unsigned int ticks, void* param);


/*! \brief
 * Timer for checking active calls load
 */
void ds_ht_timer(unsigned int ticks, void *param);

/*! \brief
 * Check if the reply-code is valid:
 */
int ds_ping_check_rplcode(int);

#endif

