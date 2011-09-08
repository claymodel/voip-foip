/* 
 * $Id$ 
 *
 * Date Header Field Name Parsing Macros
 *
 * Copyright (c) 2007 iptelorg GmbH
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
 * \brief Parser :: Date Header Field Name Parsing Macros
 *
 * \ingroup parser
 */



#ifndef CASE_DATE_H
#define CASE_DATE_H


#define date_CASE          \
     hdr->type = HDR_DATE_T; \
     p += 4;               \
     goto dc_end


#endif /* CASE_DATE_H */
