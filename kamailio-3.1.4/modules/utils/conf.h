/*
 * $Id$
 *
 * Copyright (C) 2009 1&1 Internet AG
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
 */

/*!
 * \file
 * \brief SIP-router utils :: 
 * \ingroup utils
 * Module: \ref utils
 */


#ifndef CONF_H
#define CONF_H

#include "../../lib/kmi/mi.h"
#include "../../parser/msg_parser.h"
#include "../../proxy.h"

int conf_str2id(char *id_str);

int conf_parse_switch(char *settings);

int conf_parse_filter(char *settings);

int conf_parse_proxy(char *settings);

struct proxy_l *conf_needs_forward(struct sip_msg *msg, int id);

int conf_show(struct mi_root* rpl_tree);

int conf_init(int max_id);

void conf_destroy(void);

#endif
