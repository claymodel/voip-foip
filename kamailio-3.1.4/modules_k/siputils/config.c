/*
 * $Id$
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
 * History
 * -------
 *  2008-02-05	adapting tm module for the configuration framework (Miklos)
 */

/*!
 * \file 
 * \brief Siputils :: Configuration
 * \ingroup Siputils
 */


#include "../../cfg/cfg.h"
#include "../../parser/msg_parser.h" /* method types */

#include "config.h"

struct cfg_group_siputils	default_siputils_cfg = {
		0
	};

void	*siputils_cfg = &default_siputils_cfg;

cfg_def_t	siputils_cfg_def[] = {
	{"ring_timeout",	CFG_VAR_INT | CFG_ATOMIC,	0, 0, ring_timeout_fixup, 0,
		"define how long the Call-id is kept in the internal list" },
	{0, 0, 0, 0, 0, 0}
};

int ring_timeout_fixup(void *handle, str* gname, str* name, void **val){
	if((int)(long)*val > 0) return 0;
	return -1;
}
