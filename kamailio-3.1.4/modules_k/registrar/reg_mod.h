/* 
 * $Id$ 
 *
 * registrar module interface
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
 *
 * 2005-07-11  added sip_natping_flag for nat pinging with SIP method
 *             instead of UDP package (bogdan)
 * 2006-11-28  Added statistics tracking for the number of accepted/rejected
 *             registrations, as well as for the max expiry time, max contacts,
 *             and default expiry time. (Jeffrey Magder - SOMA Networks)
 * 2007-02-24  sip_natping_flag moved into branch flags, so migrated to 
 *             nathelper module (bogdan)
 */

/*!
 * \file
 * \brief SIP registrar module - interface
 * \ingroup registrar   
 */  


#ifndef REG_MOD_H
#define REG_MOD_H

#include "../../parser/msg_parser.h"
#include "../../qvalue.h"
#include "../../usr_avp.h"
#include "../usrloc/usrloc.h"
#include "../../modules/sl/sl.h"

/* if DB support is used, this values must not exceed the 
 * storage capacity of the DB columns! See db/schema/entities.xml */
#define CONTACT_MAX_SIZE       255
#define RECEIVED_MAX_SIZE      255
#define USERNAME_MAX_SIZE      64
#define DOMAIN_MAX_SIZE        128
#define CALLID_MAX_SIZE        255
#define UA_MAX_SIZE            255

#define PATH_MODE_STRICT	2
#define PATH_MODE_LAZY		1
#define PATH_MODE_OFF		0

#define REG_SAVE_MEM_FL     (1<<0)
#define REG_SAVE_NORPL_FL   (1<<1)
#define REG_SAVE_REPL_FL    (1<<2)
#define REG_SAVE_ALL_FL     ((1<<3)-1)

extern int nat_flag;
extern int tcp_persistent_flag;
extern int received_avp;
extern int reg_use_domain;
extern float def_q;

extern unsigned short aor_avp_type;
extern int_str aor_avp_name;
extern unsigned short rcv_avp_type;
extern int_str rcv_avp_name;
extern unsigned short reg_callid_avp_type;
extern int_str reg_callid_avp_name;

extern str rcv_param;
extern int method_filtering;
extern int path_enabled;
extern int path_mode;
extern int path_use_params;

extern str sock_hdr_name;
extern int sock_flag;

extern usrloc_api_t ul;/*!< Structure containing pointers to usrloc functions*/

extern sl_api_t slb;

extern stat_var *accepted_registrations;
extern stat_var *rejected_registrations;
extern stat_var *default_expire_stat;
extern stat_var *max_expires_stat;

#endif /* REG_MOD_H */
