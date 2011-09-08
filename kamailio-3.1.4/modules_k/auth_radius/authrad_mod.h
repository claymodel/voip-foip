/*
 * $Id$
 *
 * Digest Authentication - Radius support
 *
 * Copyright (C) 2001-2003 FhG Fokus
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
 * History:
 * -------
 * 2003-03-09: Based on auth_mod.h from radius_authorize (janakj)
 */


#ifndef AUTHRAD_MOD_H
#define AUTHRAD_MOD_H

#include "../../modules/auth/api.h"
#include "../../lib/kcore/radius.h"

extern struct attr attrs[];
extern struct val vals[];
extern void *rh;

extern struct extra_attr *auth_extra;

extern int use_ruri_flag;

extern auth_api_s_t auth_api;

#endif /* AUTHRAD_MOD_H */
