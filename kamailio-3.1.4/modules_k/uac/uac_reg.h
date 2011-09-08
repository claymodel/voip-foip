/**
 * $Id$
 *
 * Copyright (C) 2010 Daniel-Constantin Mierla (asipto.com)
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
		       
#ifndef _UAC_REG_H_
#define _UAC_REG_H_

#include "../../pvar.h"

extern int reg_timer_interval;
extern int reg_htable_size;
extern int reg_fetch_rows;
extern str reg_contact_addr;
extern str reg_db_url;
extern str reg_db_table;

extern str l_uuid_column;
extern str l_username_column;
extern str l_domain_column;
extern str r_username_column;
extern str r_domain_column;
extern str realm_column;
extern str auth_username_column;
extern str auth_password_column;
extern str auth_proxy_column;
extern str expires_column;

int uac_reg_init_db(void);
int uac_reg_load_db(void);
int uac_reg_init_ht(unsigned int sz);
int uac_reg_free_ht(void);

void uac_reg_timer(unsigned int ticks);
int uac_reg_init_rpc(void);

int  uac_reg_lookup(struct sip_msg *msg, str *src, pv_spec_t *dst, int mode);
#endif
