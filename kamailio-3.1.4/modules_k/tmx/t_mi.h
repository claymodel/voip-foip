/*
 * $Id: mi.h 4518 2008-07-28 15:39:28Z henningw $
 *
 * Header file for TM MI functions
 *
 * Copyright (C) 2006 Voice Sistem SRL
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
 *  2006-12-04  created (bogdan)
 */

/*! \file
 * \brief TM :: MI functions
 *
 * \ingroup tm
 * - Module: \ref tm
 * - \ref mi.c
 */

#ifndef _TM_MI_H_
#define _TM_MI_H_

#include "../../lib/kmi/mi.h"

#define MI_TM_UAC      "t_uac_dlg"
#define MI_TM_CANCEL   "t_uac_cancel"
#define MI_TM_HASH     "t_hash"
#define MI_TM_REPLY    "t_reply"

struct mi_root* mi_tm_uac_dlg(struct mi_root* cmd_tree, void* param);

struct mi_root* mi_tm_cancel(struct mi_root* cmd_tree, void* param);

struct mi_root* mi_tm_hash(struct mi_root* cmd_tree, void* param);

struct mi_root* mi_tm_reply(struct mi_root* cmd_tree, void* param);

#endif
