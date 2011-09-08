/**
 * $Id$
 *
 * Copyright (C) 2010 Daniel-Constantin Mierla (asipto.com)
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

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../mod_fix.h"

#include "app_lua_api.h"

MODULE_VERSION

/** parameters */

/* List of allowed chars for a prefix*/
static int  mod_init(void);
static void mod_destroy(void);
static int  child_init(int rank);

static int w_app_lua_dostring(struct sip_msg *msg, char *script, char *extra);
static int w_app_lua_dofile(struct sip_msg *msg, char *script, char *extra);
static int w_app_lua_runstring(struct sip_msg *msg, char *script, char *extra);
static int w_app_lua_run(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3);
static int w_app_lua_run0(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3);
static int w_app_lua_run1(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3);
static int w_app_lua_run2(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3);
static int w_app_lua_run3(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3);

static int fixup_lua_run(void** param, int param_no);

int app_lua_load_param(modparam_t type, void *val);
int app_lua_register_param(modparam_t type, void *val);

static param_export_t params[]={
	{"load",     STR_PARAM|USE_FUNC_PARAM, (void*)app_lua_load_param},
	{"register", STR_PARAM|USE_FUNC_PARAM, (void*)app_lua_register_param},
	{0, 0, 0}
};

static cmd_export_t cmds[]={
	{"lua_dostring", (cmd_function)w_app_lua_dostring, 1, fixup_spve_null,
		0, ANY_ROUTE},
	{"lua_dofile", (cmd_function)w_app_lua_dofile, 1, fixup_spve_null,
		0, ANY_ROUTE},
	{"lua_runstring", (cmd_function)w_app_lua_runstring, 1, fixup_spve_null,
		0, ANY_ROUTE},
	{"lua_run", (cmd_function)w_app_lua_run0, 1, fixup_lua_run,
		0, ANY_ROUTE},
	{"lua_run", (cmd_function)w_app_lua_run1, 2, fixup_lua_run,
		0, ANY_ROUTE},
	{"lua_run", (cmd_function)w_app_lua_run2, 3, fixup_lua_run,
		0, ANY_ROUTE},
	{"lua_run", (cmd_function)w_app_lua_run3, 4, fixup_lua_run,
		0, ANY_ROUTE},
	{0, 0, 0, 0, 0, 0}
};

struct module_exports exports = {
	"app_lua",
	RTLD_NOW | RTLD_GLOBAL, /* dlopen flags */
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
	lua_sr_init_mod();
	return 0;
}


/* each child get a new connection to the database */
static int child_init(int rank)
{
	if(rank==PROC_MAIN || rank==PROC_TCP_MAIN)
		return 0; /* do nothing for the main process */

	if (rank==PROC_INIT)
	{
		/* do a probe before forking */
		if(lua_sr_init_probe()!=0)
			return -1;
		return 0;
	}
	return lua_sr_init_child();
}


static void mod_destroy(void)
{
	lua_sr_destroy();
}

static char _lua_buf_stack[4][512];

static int w_app_lua_dostring(struct sip_msg *msg, char *script, char *extra)
{
	str s;
	if(!lua_sr_initialized())
	{
		LM_ERR("Lua env not intitialized");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)script, &s)<0)
	{
		LM_ERR("cannot get the script\n");
		return -1;
	}
	if(s.len>=511)
	{
		LM_ERR("script too long %d\n", s.len);
		return -1;
	}
	memcpy(_lua_buf_stack[0], s.s, s.len);
	_lua_buf_stack[0][s.len] = '\0';
	return app_lua_dostring(msg, _lua_buf_stack[0]);
}

static int w_app_lua_dofile(struct sip_msg *msg, char *script, char *extra)
{
	str s;
	if(!lua_sr_initialized())
	{
		LM_ERR("Lua env not intitialized");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)script, &s)<0)
	{
		LM_ERR("cannot get the script\n");
		return -1;
	}
	if(s.len>=511)
	{
		LM_ERR("script too long %d\n", s.len);
		return -1;
	}
	memcpy(_lua_buf_stack[0], s.s, s.len);
	_lua_buf_stack[0][s.len] = '\0';
	return app_lua_dofile(msg, _lua_buf_stack[0]);
}

static int w_app_lua_runstring(struct sip_msg *msg, char *script, char *extra)
{
	str s;
	if(!lua_sr_initialized())
	{
		LM_ERR("Lua env not intitialized");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)script, &s)<0)
	{
		LM_ERR("cannot get the script\n");
		return -1;
	}
	if(s.len>=511)
	{
		LM_ERR("script too long %d\n", s.len);
		return -1;
	}
	memcpy(_lua_buf_stack[0], s.s, s.len);
	_lua_buf_stack[0][s.len] = '\0';
	return app_lua_runstring(msg, _lua_buf_stack[0]);
}

static int w_app_lua_run(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	str s;
	if(!lua_sr_initialized())
	{
		LM_ERR("Lua env not intitialized");
		return -1;
	}
	if(fixup_get_svalue(msg, (gparam_p)func, &s)<0)
	{
		LM_ERR("cannot get the function\n");
		return -1;
	}
	if(s.len>=511)
	{
		LM_ERR("function too long %d\n", s.len);
		return -1;
	}
	memcpy(_lua_buf_stack[0], s.s, s.len);
	_lua_buf_stack[0][s.len] = '\0';

	if(p1!=NULL)
	{
		if(fixup_get_svalue(msg, (gparam_p)p1, &s)<0)
		{
			LM_ERR("cannot get p1\n");
			return -1;
		}
		if(s.len>=511)
		{
			LM_ERR("p1 too long %d\n", s.len);
			return -1;
		}
		memcpy(_lua_buf_stack[1], s.s, s.len);
		_lua_buf_stack[1][s.len] = '\0';

		if(p2!=NULL)
		{
			if(fixup_get_svalue(msg, (gparam_p)p2, &s)<0)
			{
				LM_ERR("cannot get p2\n");
				return -1;
			}
			if(s.len>=511)
			{
				LM_ERR("p2 too long %d\n", s.len);
				return -1;
			}
			memcpy(_lua_buf_stack[2], s.s, s.len);
			_lua_buf_stack[2][s.len] = '\0';

			if(p3!=NULL)
			{
				if(fixup_get_svalue(msg, (gparam_p)p3, &s)<0)
				{
					LM_ERR("cannot get p3\n");
					return -1;
				}
				if(s.len>=511)
				{
					LM_ERR("p3 too long %d\n", s.len);
					return -1;
				}
				memcpy(_lua_buf_stack[3], s.s, s.len);
				_lua_buf_stack[3][s.len] = '\0';
			}
		} else {
			p3 = NULL;
		}
	} else {
		p2 = NULL;
		p3 = NULL;
	}

	return app_lua_run(msg, _lua_buf_stack[0],
			(p1!=NULL)?_lua_buf_stack[1]:NULL,
			(p2!=NULL)?_lua_buf_stack[2]:NULL,
			(p3!=NULL)?_lua_buf_stack[3]:NULL);
}

static int w_app_lua_run0(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	return w_app_lua_run(msg, func, NULL, NULL, NULL);
}

static int w_app_lua_run1(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	return w_app_lua_run(msg, func, p1, NULL, NULL);
}

static int w_app_lua_run2(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	return w_app_lua_run(msg, func, p1, p2, NULL);
}

static int w_app_lua_run3(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	return w_app_lua_run(msg, func, p1, p2, p3);
}

int app_lua_load_param(modparam_t type, void *val)
{
	if(val==NULL)
		return -1;
	return sr_lua_load_script((char*)val);
}

int app_lua_register_param(modparam_t type, void *val)
{
	if(val==NULL)
		return -1;
	return sr_lua_register_module((char*)val);
}

static int fixup_lua_run(void** param, int param_no)
{
	return fixup_spve_null(param, 1);
}

