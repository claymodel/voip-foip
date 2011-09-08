/*
 * $Id$
 *
 * BDB Database Driver for SIP-router
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * SIP-router is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef _BDB_URI_H_
#define _BDB_URI_H_

/** \addtogroup bdb
 * @{
 */

/*! \file
 * Berkeley DB : The functions parsing and interpreting bdb: URIs.
 *
 * \ingroup database
 */


#include "../../lib/srdb2/db_uri.h"
#include "../../lib/srdb2/db_drv.h"

#include <db.h>

/*! BDB driver specific payload to attach to db_uri structures.
 * This is the BDB specific structure that will be attached
 * to generic db_uri structures in the database API in SER. The
 * structure contains parsed elements of the ldap: URI.
 */
typedef struct bdb_uri {
	db_drv_t drv;
	char* uri;             /**< The whole URI, including scheme */
	str path;
} bdb_uri_t, *bdb_uri_p;


/*! Create a new bdb_uri structure and parse the URI in parameter.
 * This function builds a new bdb_uri structure from the body of
 * the generic URI given to it in parameter.
 * @param uri A generic db_uri structure.
 * @retval 0 on success
 * @retval A negative number on error.
 */
int bdb_uri(db_uri_t* uri);


/** @} */

#endif /* _BDB_URI_H_ */
