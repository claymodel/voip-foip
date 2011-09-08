/* 
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 * Copyright (C) 2006-2007 iptelorg GmbH
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MY_FLD_H
#define _MY_FLD_H  1

/** @addtogroup mysql
 *  @{
 */

#include "../../lib/srdb2/db_drv.h"
#include "../../lib/srdb2/db_fld.h"
#include <mysql/mysql.h>

struct my_fld {
	db_drv_t gen;

	char* name;
	my_bool is_null;
	MYSQL_TIME time;
	unsigned long length;
	str buf;
};

int my_fld(db_fld_t* fld, char* table);

/** @} */

#endif /* _MY_FLD_H */
