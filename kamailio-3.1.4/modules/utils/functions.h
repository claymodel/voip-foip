/*
 * headers of script functions of utils module
 *
 * Copyright (C) 2008 Juha Heinanen
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
 * \brief SIP-router utils :: 
 * \ingroup utils
 * Module: \ref utils
 */


#ifndef UTILS_FUNCTIONS_H
#define UTILS_FUNCTIONS_H

#include "../../parser/msg_parser.h"


/* 
 * Performs http_query and saves possible result (first body line of reply)
 * to pvar.
 */
int http_query(struct sip_msg* _m, char* _page, char* _params, char* _dst);


#endif /* UTILS_FUNCTIONS_H */
