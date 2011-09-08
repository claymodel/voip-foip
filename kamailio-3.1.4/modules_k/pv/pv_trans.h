/*
 * $Id$
 *
 * Copyright (C) 2007 voice-system.ro
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

/*! \file
 * \brief Transformations support
 */

#ifndef _PV_TRANS_H_
#define _PV_TRANS_H_

#include "../../pvar.h"



enum _tr_type { TR_NONE=0, TR_STRING, TR_URI, TR_PARAMLIST, TR_NAMEADDR,
				TR_TOBODY };
enum _tr_s_subtype { 
	TR_S_NONE=0, TR_S_LEN, TR_S_INT, TR_S_MD5, TR_S_SUBSTR,
	TR_S_SELECT, TR_S_ENCODEHEXA, TR_S_DECODEHEXA,
	TR_S_ESCAPECOMMON, TR_S_UNESCAPECOMMON, TR_S_ESCAPEUSER, TR_S_UNESCAPEUSER,
	TR_S_ESCAPEPARAM, TR_S_UNESCAPEPARAM, TR_S_TOLOWER, TR_S_TOUPPER,
	TR_S_STRIP, TR_S_STRIPTAIL
};
enum _tr_uri_subtype {
	TR_URI_NONE=0, TR_URI_USER, TR_URI_HOST, TR_URI_PASSWD, TR_URI_PORT,
	TR_URI_PARAMS, TR_URI_PARAM, TR_URI_HEADERS, TR_URI_TRANSPORT, TR_URI_TTL,
	TR_URI_UPARAM, TR_URI_MADDR, TR_URI_METHOD, TR_URI_LR,
	TR_URI_R2
};
enum _tr_param_subtype {
	TR_PL_NONE=0, TR_PL_VALUE, TR_PL_VALUEAT, TR_PL_NAME, TR_PL_COUNT
};
enum _tr_nameaddr_subtype {
	TR_NA_NONE=0, TR_NA_NAME, TR_NA_URI, TR_NA_LEN
};
enum _tr_tobody_subtype {
	TR_TOBODY_NONE=0, TR_TOBODY_DISPLAY, TR_TOBODY_URI, TR_TOBODY_TAG,
	TR_TOBODY_URI_USER, TR_TOBODY_URI_HOST, TR_TOBODY_PARAMS
};


char* tr_parse_string(str *in, trans_t *tr);
char* tr_parse_uri(str *in, trans_t *tr);
char* tr_parse_paramlist(str *in, trans_t *tr);
char* tr_parse_nameaddr(str *in, trans_t *tr);
char* tr_parse_tobody(str* in, trans_t *t);

#endif
