/**
 * $Id$
 *
 * Copyright (C) 2009 Daniel-Constantin Mierla (asipto.com)
 *
 * This file is part of kamailio, a free SIP server.
 *
 * openser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * openser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../../dprint.h"
#include "../../globals.h"
#include "../../dset.h"

#include "faked_msg.h"

#define FAKED_SIP_MSG "OPTIONS sip:you@kamailio.org SIP/2.0\r\nVia: SIP/2.0/UDP 127.0.0.1\r\nFrom: <you@kamailio.org>;tag=123\r\nTo: <you@kamailio.org>\r\nCall-ID: 123\r\nCSeq: 1 OPTIONS\r\nContent-Length: 0\r\n\r\n"
#define FAKED_SIP_MSG_LEN (sizeof(FAKED_SIP_MSG)-1)
static char _faked_sip_buf[FAKED_SIP_MSG_LEN+1];
static struct sip_msg _faked_msg;
static unsigned int _faked_msg_no = 0;

int faked_msg_init(void)
{
	if(_faked_msg_no>0)
		return 0;
	/* init faked sip msg */
	memcpy(_faked_sip_buf, FAKED_SIP_MSG, FAKED_SIP_MSG_LEN);
	_faked_sip_buf[FAKED_SIP_MSG_LEN] = '\0';
	
	memset(&_faked_msg, 0, sizeof(struct sip_msg));

	_faked_msg.buf=_faked_sip_buf;
	_faked_msg.len=FAKED_SIP_MSG_LEN;

	_faked_msg.set_global_address=default_global_address;
	_faked_msg.set_global_port=default_global_port;

	if (parse_msg(_faked_msg.buf, _faked_msg.len, &_faked_msg)!=0)
	{
		LM_ERR("parse_msg failed\n");
		return -1;
	}
	return 0;
}

struct sip_msg* faked_msg_next(void)
{
	_faked_msg.id=_faked_msg_no++;
	clear_branches();
	return &_faked_msg;
}
