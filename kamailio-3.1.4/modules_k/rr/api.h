/*
 * $Id$
 *
 * Copyright (C) 2005 Voice Sistem SRL
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*!
 * \file
 * \brief Kamailio RR module (record-routing) API
 *
 * Kamailio RR module (record-routing) API.
 * The RR module provides an internal API to be used by other
 * Kamailio modules. The API offers support for SIP dialog based
 * functionalities.

 * For internal(non-script) usage, the RR module offers to other
 * module the possibility to register callback functions to be
 * executed each time a local Route header is processed. The
 * callback function will receive as parameter the register
 * parameter and the Route header parameter string.
 * \ingroup rr
 */


#ifndef RR_API_H_
#define RR_API_H_

#include "../../str.h"
#include "../../sr_module.h"
#include "loose.h"
#include "rr_cb.h"

typedef  int (*add_rr_param_t)(struct sip_msg*, str*);
typedef  int (*check_route_param_t)(struct sip_msg*, regex_t*);
typedef  int (*is_direction_t)(struct sip_msg*, int);
typedef  int (*get_route_param_t)(struct sip_msg*, str*, str*);


/*! record-route API export binding */
struct rr_binds {
	add_rr_param_t		add_rr_param;
	check_route_param_t	check_route_param;
	is_direction_t			is_direction;
	get_route_param_t		get_route_param;
	register_rrcb_t			register_rrcb;
	int					append_fromtag;
};

typedef  int (*load_rr_f)( struct rr_binds* );

/*!
* \brief API bind function exported by the module - it will load the other functions
 * \param rr_binds record-route API export binding
 * \return 1
 */
int load_rr( struct rr_binds * );


/*!
 * \brief Function to be called directly from other modules to load the RR API
 * \param rrb record-route API export binding
 * \return 0 on success, -1 if the API loader could not imported
 */
inline static int load_rr_api( struct rr_binds *rrb )
{
	load_rr_f load_rr_v;

	/* import the RR auto-loading function */
	if ( !(load_rr_v=(load_rr_f)find_export("load_rr", 0, 0))) {
		LM_ERR("failed to import load_rr\n");
		return -1;
	}
	/* let the auto-loading function load all RR stuff */
	load_rr_v( rrb );

	return 0;
}


#endif
