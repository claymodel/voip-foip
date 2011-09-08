/*
 * $Id$
 *
 * Challenge related functions
 *
 * Copyright (C) 2001-2003 FhG Fokus
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


#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "../../str.h"

/**
 * Create {WWW,Proxy}-Authenticate header field
 * @param nonce nonce value
 * @param algorithm algorithm value
 * @return -1 on error, 0 on success
 * 
 * The result is stored in an attribute.
 * If nonce is not null that it is used, instead of call calc_nonce.
 * If algorithm is not null that it is used irrespective of _PRINT_MD5
 * 
 * Major usage of nonce and algorithm params is AKA authentication. 
 */
typedef int (*build_challenge_hf_t)(struct sip_msg* msg, int stale,
		str* realm, str* nonce, str* algorithm, int hftype);
int build_challenge_hf(struct sip_msg* msg, int stale, str* realm,
		str* nonce, str* algorithm, int hftype);

int get_challenge_hf(struct sip_msg* msg, int stale, str* realm,
		str* nonce, str* algorithm, struct qp* qop, int hftype, str *ahf);

#endif /* CHALLENGE_H */
