/*
 * $Id$
 *
 * Accounting module
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
 * 2003-04-04  grand acc cleanup (jiri)
 * 2003-11-04  multidomain support for mysql introduced (jiri)
 * 2004-06-06  removed db_url, db_handle (andrei)
 * 2005-06-28  multi leg call support added (bogdan)
 * 2006-09-19  final stage of a masive re-structuring and cleanup (bogdan)
 */

/*! \file
 * \ingroup acc
 * \brief Acc::  Core module
 *
 * - \ref acc_mod.c
 * - Module: \ref acc
 */

#ifndef _ACC_MOD_H
#define _ACC_MOD_H

/* module parameter declaration */
extern int report_cancels;
extern int report_ack;
extern int early_media;
extern int failed_transaction_flag;
extern int detect_direction;

extern int log_facility;
extern int log_level;
extern int log_flag;
extern int log_missed_flag;


#ifdef RAD_ACC
extern int radius_flag;
extern int radius_missed_flag;
extern void *rh;
#endif

#ifdef DIAM_ACC
#include "diam_tcp.h"
extern rd_buf_t *rb;
extern int diameter_flag;
extern int diameter_missed_flag;
#endif

#ifdef SQL_ACC
extern int db_flag;
extern int db_missed_flag;

extern str db_table_acc;
extern str db_table_mc;

extern str acc_method_col;
extern str acc_fromuri_col;
extern str acc_fromtag_col;
extern str acc_touri_col;
extern str acc_totag_col;
extern str acc_callid_col;
extern str acc_cseqno_col;
extern str acc_sipcode_col;
extern str acc_sipreason_col;
extern str acc_time_col;
#endif /* SQL_ACC */


#endif
