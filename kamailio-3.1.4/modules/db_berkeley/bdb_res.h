/* 
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2006-2007 iptelorg GmbH
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * SIP-router is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*! \file
 * Berkeley DB : 
 *
 * \ingroup database
 */


#ifndef _BDB_RES_H_
#define _BDB_RES_H_

#include "../../lib/srdb2/db_drv.h"
#include "../../lib/srdb2/db_res.h"

typedef struct _bdb_res {
	db_drv_t gen;
} bdb_res_t;

int bdb_res(db_res_t* cmd);

#endif /* _BDB_RES_H_ */
