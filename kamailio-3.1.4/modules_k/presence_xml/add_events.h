/*
 * $Id: add_events.h 2006-12-07 18:05:05Z anca_vamanu $
 *
 * presence_xml module - 
 *
 * Copyright (C) 2006 Voice Sistem S.R.L.
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
 * --------
 *  2007-04-18  initial version (anca)
 */

/*! \file
 * \brief Kamailio Presence_XML :: 
 * \ref add_events.c
 * \ingroup presence_xml
 */


#ifndef _XML_ADD_EV_H_
#define _XML_ADD_EV_H_

int xml_add_events(void);
int	xml_publ_handl(struct sip_msg* msg);

#endif
