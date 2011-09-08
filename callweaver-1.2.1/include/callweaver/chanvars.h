/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 * \brief Channel Variables
 */

#ifndef _CALLWEAVER_CHANVARS_H
#define _CALLWEAVER_CHANVARS_H

#include "callweaver/linkedlists.h"

struct cw_var_t {
	CW_LIST_ENTRY(cw_var_t) entries;
	// added 'hash' to accommodate hash based system to recognise identifiers
	unsigned int hash;
	char *value;
	char name[0];
};

CW_LIST_HEAD_NOLOCK(varshead, cw_var_t);

struct cw_var_t *cw_var_assign(const char *name, const char *value);
void cw_var_delete(struct cw_var_t *var);
char *cw_var_name(struct cw_var_t *var);
char *cw_var_full_name(struct cw_var_t *var);
char *cw_var_value(struct cw_var_t *var);
#define cw_var_hash(v) (v ? v->hash : 0)

#endif /* _CALLWEAVER_CHANVARS_H */
