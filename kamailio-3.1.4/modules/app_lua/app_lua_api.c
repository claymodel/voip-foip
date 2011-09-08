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
#include <sys/stat.h>

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../ut.h"
#include "../../mem/mem.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../lib/kcore/cmpapi.h"

#include "app_lua_api.h"
#include "app_lua_sr.h"
#include "app_lua_exp.h"


#define SRVERSION "1.0"


/**
 *
 */
static sr_lua_env_t _sr_L_env;

/**
 * @return the static Lua env
 */
sr_lua_env_t *sr_lua_env_get(void)
{
	return &_sr_L_env;
}

/**
 *
 */
typedef struct _sr_lua_load
{
	char *script;
	struct _sr_lua_load *next;
} sr_lua_load_t;

/**
 *
 */
static sr_lua_load_t *_sr_lua_load_list = NULL;

/**
 *
 */
int sr_lua_load_script(char *script)
{
	sr_lua_load_t *li;

	li = (sr_lua_load_t*)pkg_malloc(sizeof(sr_lua_load_t));
	if(li==NULL)
	{
		LM_ERR("no more pkg\n");
		return -1;
	}
	memset(li, 0, sizeof(sr_lua_load_t));
	li->script = script;
	li->next = _sr_lua_load_list;
	_sr_lua_load_list = li;
	return 0;
}


/**
 *
 */
int sr_lua_register_module(char *mname)
{
	if(lua_sr_exp_register_mod(mname)==0)
		return 0;
	return -1;
}

/**
 *
 */
void lua_sr_openlibs(lua_State *L)
{
	lua_sr_core_openlibs(L);
	lua_sr_exp_openlibs(L);
}

/**
 *
 */
int lua_sr_init_mod(void)
{
	memset(&_sr_L_env, 0, sizeof(sr_lua_env_t));
	if(lua_sr_exp_init_mod()<0)
		return -1;
	return 0;
}

/**
 *
 */
int lua_sr_init_probe(void)
{
	lua_State *L;
	char *txt;
	sr_lua_load_t *li;
	struct stat sbuf;

	L = lua_open();
	if(L==NULL)
	{
		LM_ERR("cannot open lua\n");
		return -1;
	}
	luaL_openlibs(L);
	lua_sr_openlibs(L);

	/* force loading lua lib now */
	if(luaL_dostring(L, "sr.probe()")!=0)
	{
		txt = (char*)lua_tostring(L, -1);
		LM_ERR("error initializing Lua: %s\n", (txt)?txt:"unknown");
		lua_pop(L, 1);
		lua_close(L);
		return -1;
	}

	/* test if files to be loaded exist */
	if(_sr_lua_load_list != NULL)
	{
		li = _sr_lua_load_list;
		while(li)
		{
			if(stat(li->script, &sbuf)!=0)
			{
				/* file does not exist */
				LM_ERR("cannot find script: %s (wrong path?)\n",
						li->script);
				lua_close(L);
				return -1;
			}
			li = li->next;
		}
	}
	lua_close(L);
	LM_DBG("Lua probe was ok!\n");
	return 0;
}

/**
 *
 */
int lua_sr_init_child(void)
{
	sr_lua_load_t *li;
	int ret;
	char *txt;

	memset(&_sr_L_env, 0, sizeof(sr_lua_env_t));
	_sr_L_env.L = lua_open();
	if(_sr_L_env.L==NULL)
	{
		LM_ERR("cannot open lua\n");
		return -1;
	}
	luaL_openlibs(_sr_L_env.L);
	lua_sr_openlibs(_sr_L_env.L);

	/* set SR lib version */
	lua_pushstring(_sr_L_env.L, "SRVERSION");
	lua_pushstring(_sr_L_env.L, SRVERSION);
	lua_settable(_sr_L_env.L, LUA_GLOBALSINDEX);

	if(_sr_lua_load_list != NULL)
	{
		_sr_L_env.LL = luaL_newstate();
		if(_sr_L_env.LL==NULL)
		{
			LM_ERR("cannot open lua loading state\n");
			return -1;
		}
		luaL_openlibs(_sr_L_env.LL);
		lua_sr_openlibs(_sr_L_env.LL);

		/* set SR lib version */
		lua_pushstring(_sr_L_env.LL, "SRVERSION");
		lua_pushstring(_sr_L_env.LL, SRVERSION);
		lua_settable(_sr_L_env.LL, LUA_GLOBALSINDEX);

		/* force loading lua lib now */
		if(luaL_dostring(_sr_L_env.LL, "sr.probe()")!=0)
		{
			txt = (char*)lua_tostring(_sr_L_env.LL, -1);
			LM_ERR("error initializing Lua: %s\n", (txt)?txt:"unknown");
			lua_pop(_sr_L_env.LL, 1);
			lua_sr_destroy();
			return -1;
		}

		li = _sr_lua_load_list;
		while(li)
		{
			ret = luaL_dofile(_sr_L_env.LL, (const char*)li->script);
			if(ret!=0)
			{
				LM_ERR("failed to load Lua script: %s (err: %d)\n",
						li->script, ret);
				txt = (char*)lua_tostring(_sr_L_env.LL, -1);
				LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
				lua_pop(_sr_L_env.LL, 1);
				lua_sr_destroy();
				return -1;
			}
			li = li->next;
		}
	}
	LM_DBG("Lua initialized!\n");
	return 0;
}

/**
 *
 */
void lua_sr_destroy(void)
{
	if(_sr_L_env.L!=NULL)
	{
		lua_close(_sr_L_env.L);
		_sr_L_env.L = NULL;
	}
	if(_sr_L_env.LL!=NULL)
	{
		lua_close(_sr_L_env.LL);
		_sr_L_env.LL = NULL;
	}
	memset(&_sr_L_env, 0, sizeof(sr_lua_env_t));
}

/**
 *
 */
int lua_sr_initialized(void)
{
	if(_sr_L_env.L==NULL)
		return 0;

	return 1;
}

/**
 *
 */
int app_lua_return_boolean(lua_State *L, int b)
{
	if(b==0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}

/**
 *
 */
int app_lua_dostring(struct sip_msg *msg, char *script)
{
	int ret;
	char *txt;

	LM_DBG("executing Lua string: [[%s]]\n", script);
	LM_DBG("lua top index is: %d\n", lua_gettop(_sr_L_env.L));
	_sr_L_env.msg = msg;
	ret = luaL_dostring(_sr_L_env.L, script);
	if(ret!=0)
	{
		txt = (char*)lua_tostring(_sr_L_env.L, -1);
		LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
		lua_pop (_sr_L_env.L, 1);
	}
	_sr_L_env.msg = 0;
	return (ret==0)?1:-1;
}

/**
 *
 */
int app_lua_dofile(struct sip_msg *msg, char *script)
{
	int ret;
	char *txt;

	LM_DBG("executing Lua file: [[%s]]\n", script);
	LM_DBG("lua top index is: %d\n", lua_gettop(_sr_L_env.L));
	_sr_L_env.msg = msg;
	ret = luaL_dofile(_sr_L_env.L, script);
	if(ret!=0)
	{
		txt = (char*)lua_tostring(_sr_L_env.L, -1);
		LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
		lua_pop(_sr_L_env.L, 1);
	}
	_sr_L_env.msg = 0;
	return (ret==0)?1:-1;
}

/**
 *
 */
int app_lua_runstring(struct sip_msg *msg, char *script)
{
	int ret;
	char *txt;

	if(_sr_L_env.LL==NULL)
	{
		LM_ERR("lua loading state not initialized (call: %s)\n", script);
		return -1;
	}

	LM_DBG("running Lua string: [[%s]]\n", script);
	LM_DBG("lua top index is: %d\n", lua_gettop(_sr_L_env.LL));
	_sr_L_env.msg = msg;
	ret = luaL_dostring(_sr_L_env.LL, script);
	if(ret!=0)
	{
		txt = (char*)lua_tostring(_sr_L_env.LL, -1);
		LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
		lua_pop (_sr_L_env.LL, 1);
	}
	_sr_L_env.msg = 0;
	return (ret==0)?1:-1;
}

/**
 *
 */
int app_lua_run(struct sip_msg *msg, char *func, char *p1, char *p2,
		char *p3)
{
	int n;
	int ret;
	char *txt;

	if(_sr_L_env.LL==NULL)
	{
		LM_ERR("lua loading state not initialized (call: %s)\n", func);
		return -1;
	}

	LM_DBG("executing Lua function: [[%s]]\n", func);
	LM_DBG("lua top index is: %d\n", lua_gettop(_sr_L_env.LL));
	lua_getglobal(_sr_L_env.LL, func);
	if(!lua_isfunction(_sr_L_env.LL, -1))
	{
		LM_ERR("no such function [%s] in lua scripts\n", func);
		LM_ERR("top stack type [%d - %s]\n",
				lua_type(_sr_L_env.LL, -1),
				lua_typename(_sr_L_env.LL,lua_type(_sr_L_env.LL, -1)));
		txt = (char*)lua_tostring(_sr_L_env.LL, -1);
		LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
		return -1;
	}
	n = 0;
	if(p1!=NULL)
	{
		lua_pushstring(_sr_L_env.LL, p1);
		n++;
		if(p2!=NULL)
		{
			lua_pushstring(_sr_L_env.LL, p2);
			n++;
			if(p3!=NULL)
			{
				lua_pushstring(_sr_L_env.LL, p3);
				n++;
			}
		}
	}
	_sr_L_env.msg = msg;
	ret = lua_pcall(_sr_L_env.LL, n, 0, 0);
	_sr_L_env.msg = 0;
	if(ret!=0)
	{
		LM_ERR("error executing: %s (err: %d)\n", func, ret);
		txt = (char*)lua_tostring(_sr_L_env.LL, -1);
		LM_ERR("error from Lua: %s\n", (txt)?txt:"unknown");
		lua_pop(_sr_L_env.LL, 1);
		return -1;
	}

	return 1;
}

void app_lua_dump_stack(lua_State *L)
{
	int i;
	int t;
	int top;

	top = lua_gettop(L);

	LM_DBG("lua stack top index: %d\n", top);
	for (i = 1; i <= top; i++)
	{
		t = lua_type(L, i);
		switch (t)
		{
			case LUA_TSTRING:  /* strings */
				LM_DBG("[%i:s> %s\n", i, lua_tostring(L, i));
			break;
			case LUA_TBOOLEAN:  /* booleans */
				LM_DBG("[%i:b> %s\n", i,
					lua_toboolean(L, i) ? "true" : "false");
			break;
			case LUA_TNUMBER:  /* numbers */
				LM_DBG("[%i:n> %g\n", i, lua_tonumber(L, i));
			break;
			default:  /* other values */
				LM_DBG("[%i:t> %s\n", i, lua_typename(L, t));
			break;
		}
	}
}
