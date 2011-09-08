/*
 * $Id$
 *
 * Copyright (C) 2008 iptelorg GmbH
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
 *
 */
/*! \file
 * \brief Parser :: Body handling
 *
 * \ingroup parser
 */


#ifndef PARSE_BODY_H
#define PARSE_BODY_H

/*! \brief Returns the pointer within the msg body to the given type/subtype,
 * and sets the length.
 * The result can be the whole msg body, or a part of a multipart body.
 */
char *get_body_part(	struct sip_msg *msg,
			unsigned short type, unsigned short subtype,
			int *len);

#endif /* PARSE_BODY_H */
