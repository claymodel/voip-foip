/*
 * $Id$
 *
 * Copyright (C) 2006 Juha Heinanen
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
 */

/*!
 * \file
 * \brief P-Preferred-Identity header parser
 * \ingroup parser
 */

#ifndef PARSE_PPI_H
#define PARSE_PPI_H

#include "../../parser/msg_parser.h"
#include "../../parser/parse_to.h"


/*! casting macro for accessing P-Preferred-Identity body */
#define get_ppi(p_msg)  ((struct to_body*)(p_msg)->ppi->parsed)


/*!
 * \brief This method is used to parse P-Preferred-Identity header (RFC 3325).
 *
 * Currently only one name-addr / addr-spec is supported in the header
 * and it must contain a sip or sips URI.
 * \param msg sip msg
 * \return 0 on success, -1 on failure.
 */
int parse_ppi_header( struct sip_msg *msg);

struct sip_uri *parse_ppi_uri(struct sip_msg *msg);
 
#endif /* PARSE_PPI_H */
