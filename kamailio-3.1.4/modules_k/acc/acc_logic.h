/*
 * $Id$
 *
 * Accounting module
 *
 * Copyright (C) 2006 Voice Sistem SRL
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
 * ---------
 * 2005-09-19  created during a big re-structuring of acc module(bogdan)
 */

/*! \file
 * \ingroup acc
 * \brief Acc:: Accounting logic
 *
 * - \ref acc_logic.c
 * - Module: \ref acc
 */

#ifndef _ACC_ACC_LOGIC_H
#define _ACC_ACC_LOGIC_H

#include "../../str.h"
#include "../../modules/tm/t_hooks.h"
#include "acc_api.h"


void acc_onreq( struct cell* t, int type, struct tmcb_params *ps );

int w_acc_log_request(struct sip_msg *rq, char *comment, char *foo);

#ifdef SQL_ACC
int w_acc_db_request(struct sip_msg *rq, char *comment, char *table);
#endif

#ifdef RAD_ACC
int w_acc_rad_request(struct sip_msg *rq, char *comment, char *foo);
#endif

#ifdef DIAM_ACC
int w_acc_diam_request(struct sip_msg *rq, char *comment, char *foo);
#endif

int acc_api_exec(struct sip_msg *rq, acc_engine_t *eng,
		acc_param_t* comment);

#endif
