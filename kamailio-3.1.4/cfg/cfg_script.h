/*
 * $Id$
 *
 * Copyright (C) 2008 iptelorg GmbH
 *
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
 *  2008-01-24	dynamic groups are introduced in order to make
 *		variable declaration possible in the script (Miklos)
 */

#ifndef _CFG_SCRIPT_H
#define _CFG_SCRIPT_H

#include "../str.h"

/* structure used for temporary storing the variables
 * which are declared in the script */
typedef struct _cfg_script_var {
	unsigned int	type;
	union {
		str	s;
		int	i;
	} val;
	int	min;
	int	max;
	struct _cfg_script_var	*next;
	int	name_len;
	char	*name;
	char	*descr;
} cfg_script_var_t;

/* allocates memory for a new config script variable
 * The value of the variable is not set!
 */
cfg_script_var_t *new_cfg_script_var(char *gname, char *vname, unsigned int type,
					char *descr);

/* fix-up the dynamically declared group */
int cfg_script_fixup(cfg_group_t *group, unsigned char *block);

/* destory a dynamically allocated group definition */
void cfg_script_destroy(cfg_group_t *group);

#endif /* _CFG_SCRIPT_H */
