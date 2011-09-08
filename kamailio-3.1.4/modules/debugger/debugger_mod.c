/**
 * $Id$
 *
 * Copyright (C) 2010 Daniel-Constantin Mierla (asipto.com)
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../mod_fix.h"
#include "../../parser/parse_param.h"
#include "../../shm_init.h"

#include "debugger_api.h"

MODULE_VERSION

static int  mod_init(void);
static int child_init(int rank);
static void mod_destroy(void);

static int w_dbg_breakpoint(struct sip_msg* msg, char* point, char* str2);
static int fixup_dbg_breakpoint(void** param, int param_no);

/* parameters */
extern int _dbg_cfgtrace;
extern int _dbg_breakpoint;
extern int _dbg_cfgtrace_level;
extern int _dbg_cfgtrace_facility;
extern char *_dbg_cfgtrace_prefix;
extern int _dbg_step_usleep;
extern int _dbg_step_loops;

static char * _dbg_cfgtrace_facility_str = 0;

static cmd_export_t cmds[]={
	{"dbg_breakpoint", (cmd_function)w_dbg_breakpoint, 1,
		fixup_dbg_breakpoint, 0, ANY_ROUTE},
	{0, 0, 0, 0, 0, 0}
};

static param_export_t params[]={
	{"cfgtrace",          INT_PARAM, &_dbg_cfgtrace},
	{"breakpoint",        INT_PARAM, &_dbg_breakpoint},
	{"log_level",         INT_PARAM, &_dbg_cfgtrace_level},
	{"log_facility",      STR_PARAM, &_dbg_cfgtrace_facility_str},
	{"log_prefix",        STR_PARAM, &_dbg_cfgtrace_prefix},
	{"step_usleep",       INT_PARAM, &_dbg_step_usleep},
	{"step_loops",        INT_PARAM, &_dbg_step_loops},
	{0, 0, 0}
};

struct module_exports exports = {
	"debugger",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,
	0,
	0,              /* exported MI functions */
	0,              /* exported pseudo-variables */
	0,              /* extra processes */
	mod_init,       /* module initialization function */
	0,              /* response function */
	mod_destroy,    /* destroy function */
	child_init      /* per child init function */
};


/**
 * init module function
 */
static int mod_init(void)
{
	int fl;
	if (_dbg_cfgtrace_facility_str!=NULL)
	{
		fl = str2facility(_dbg_cfgtrace_facility_str);
		if (fl != -1)
		{
			_dbg_cfgtrace_facility = fl;
		} else {
			LM_ERR("invalid log facility configured");
			return -1;
		}
	}

	if(dbg_init_rpc()!=0)
	{
		LM_ERR("failed to register RPC commands\n");
		return -1;
	}

	return dbg_init_bp_list();
}

static int child_init(int rank)
{
	LM_DBG("rank is (%d)\n", rank);
	if (rank==PROC_INIT)
		return dbg_init_pid_list();
	return dbg_init_mypid();
}
/**
 * destroy module function
 */
static void mod_destroy(void)
{
}

static int w_dbg_breakpoint(struct sip_msg* msg, char* point, char* str2)
{
	return 1;
}

/**
 * get the pointer to action structure
 */
static struct action *dbg_fixup_get_action(void **param, int param_no)
{
	struct action *ac, ac2;
	action_u_t *au, au2;
	/* param points to au->u.string, get pointer to au */
	au = (void*) ((char *)param - ((char *)&au2.u.string-(char *)&au2));
	au = au - 1 - param_no;
	ac = (void*) ((char *)au - ((char *)&ac2.val-(char *)&ac2));
	return ac;
}


static int fixup_dbg_breakpoint(void** param, int param_no)
{
	struct action *a;
	char *p;

	if(param_no!=1)
		return -1;
	a = dbg_fixup_get_action(param, param_no);
	p = (char*)(*param);

    return dbg_add_breakpoint(a, (*p=='0')?0:1);
}


