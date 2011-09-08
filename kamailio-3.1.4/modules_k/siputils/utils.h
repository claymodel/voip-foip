/*
 * $Id$
 *
 * Sdp mangler module
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
 *
 * History:
 * --------
 *  2003-04-07 first version.  
 */

/*!
 * \file
 * \brief SIP-utils :: Mangler
 * \ingroup siputils
 * - Module; \ref siputils
 */


#ifndef UTILS_H
#define UTILS_H

#include "../../parser/msg_parser.h"	/* struct sip_msg */

/*  replace a part of a sip message identified by (start address,length) with a new part 
	@param msg a pointer to a sip message
	@param oldstr the start address of the part to be modified
	@param oldlen the length of the part being modified
	@param newstr the start address of the part to be added
	@param oldlen the length of the part being added
	@return 0 in case of success, negative on error 
*/

int patch (struct sip_msg *msg, char *oldstr, unsigned int oldlen,
	   char *newstr, unsigned int newlen);
/*
	modify the Content-Length header of a sip message
	@param msg a pointer to a sip message
	@param newValue the new value of Content-Length
	@return 0 in case of success, negative on error 
*/
int patch_content_length (struct sip_msg *msg, unsigned int newValue);

#endif
