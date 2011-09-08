/**
 * $Id$
 *
 * Copyright (C) 2009 Daniel-Constantin Mierla (asipto.com)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../sr_module.h"
#include "../../timer.h"
#include "../../dprint.h"
#include "../../route.h"
#include "../../receive.h"
#include "../../action.h"
#include "../../socket_info.h"
#include "../../dset.h"
#include "../../pt.h"
#include "../../timer_proc.h"
#include "../../script_cb.h"
#include "../../parser/parse_param.h"


MODULE_VERSION

typedef struct _stm_route {
	str timer;
	unsigned int route;
	struct _stm_route *next;
} stm_route_t;

typedef struct _stm_timer {
	str name;
	unsigned int mode;
	unsigned int interval;
	stm_route_t *rt;
	struct _stm_timer *next;
} stm_timer_t;

stm_timer_t *_stm_list = NULL;

/** module functions */
static int mod_init(void);
static int child_init(int);

int stm_t_param(modparam_t type, void* val);
int stm_e_param(modparam_t type, void* val);
void stm_timer_exec(unsigned int ticks, void *param);


static param_export_t params[]={
	{"timer",             STR_PARAM|USE_FUNC_PARAM, (void*)stm_t_param},
	{"exec",              STR_PARAM|USE_FUNC_PARAM, (void*)stm_e_param},
	{0,0,0}
};


/** module exports */
struct module_exports exports= {
	"rtimer",
	DEFAULT_DLFLAGS, /* dlopen flags */
	0,
	params,
	0,          /* exported statistics */
	0,          /* exported MI functions */
	0,          /* exported pseudo-variables */
	0,          /* extra processes */
	mod_init,   /* module initialization function */
	0,
	0,
	child_init  /* per-child init function */
};


#define STM_SIP_MSG "OPTIONS sip:you@kamailio.org SIP/2.0\r\nVia: SIP/2.0/UDP 127.0.0.1\r\nFrom: <you@kamailio.org>;tag=123\r\nTo: <you@kamailio.org>\r\nCall-ID: 123\r\nCSeq: 1 OPTIONS\r\nContent-Length: 0\r\n\r\n"
#define STM_SIP_MSG_LEN (sizeof(STM_SIP_MSG)-1)
static char _stm_sip_buf[STM_SIP_MSG_LEN+1];
static struct sip_msg _stm_msg;
static unsigned int _stm_msg_no = 1;
/**
 * init module function
 */
static int mod_init(void)
{
	stm_timer_t *it;
	if(_stm_list==NULL)
		return 0;

	it = _stm_list;
	while(it)
	{
		if(it->mode==0)
		{
			if(register_timer(stm_timer_exec, (void*)it, it->interval)<0)
			{
				LM_ERR("failed to register timer function\n");
				return -1;
			}
		} else {
			register_dummy_timers(1);
		}
		it = it->next;
	}

	/* init faked sip msg */
	memcpy(_stm_sip_buf, STM_SIP_MSG, STM_SIP_MSG_LEN);
	_stm_sip_buf[STM_SIP_MSG_LEN] = '\0';
	
	memset(&_stm_msg, 0, sizeof(struct sip_msg));

	_stm_msg.buf=_stm_sip_buf;
	_stm_msg.len=STM_SIP_MSG_LEN;

	_stm_msg.set_global_address=default_global_address;
	_stm_msg.set_global_port=default_global_port;

	if (parse_msg(_stm_msg.buf, _stm_msg.len, &_stm_msg)!=0)
	{
		LM_ERR("parse_msg failed\n");
		return -1;
	}

	return 0;
}

static int child_init(int rank)
{
	stm_timer_t *it;
	if(_stm_list==NULL)
		return 0;

	if (rank!=PROC_MAIN)
		return 0;

	it = _stm_list;
	while(it)
	{
		if(it->mode!=0)
		{
			if(fork_dummy_timer(PROC_TIMER, "TIMER RT", 1 /*socks flag*/,
								stm_timer_exec, (void*)it, it->interval
								/*sec*/)<0) {
				LM_ERR("failed to register timer routine as process\n");
				return -1; /* error */
			}
		}
		it = it->next;
	}

	return 0;
}

void stm_timer_exec(unsigned int ticks, void *param)
{
	stm_timer_t *it;
	stm_route_t *rt;


	if(param==NULL)
		return;
	it = (stm_timer_t*)param;
	if(it->rt==NULL)
		return;

	for(rt=it->rt; rt; rt=rt->next)
	{
		/* update local parameters */
		_stm_msg.id=_stm_msg_no++;
		clear_branches();
		if (exec_pre_script_cb(&_stm_msg, REQUEST_CB_TYPE)==0 )
			continue; /* drop the request */
		set_route_type(REQUEST_ROUTE);
		run_top_route(main_rt.rlist[rt->route], &_stm_msg, 0);
		exec_post_script_cb(&_stm_msg, REQUEST_CB_TYPE);
	}
}

int stm_t_param(modparam_t type, void *val)
{
	param_t* params_list = NULL;
	param_hooks_t phooks;
	param_t *pit=NULL;
	stm_timer_t tmp;
	stm_timer_t *nt;
	str s;

	if(val==NULL)
		return -1;
	s.s = (char*)val;
	s.len = strlen(s.s);
	if(s.s[s.len-1]==';')
		s.len--;
	if (parse_params(&s, CLASS_ANY, &phooks, &params_list)<0)
		return -1;
	memset(&tmp, 0, sizeof(stm_timer_t));
	for (pit = params_list; pit; pit=pit->next)
	{
		if (pit->name.len==4
				&& strncasecmp(pit->name.s, "name", 4)==0) {
			tmp.name = pit->body;
		} else if(pit->name.len==4
				&& strncasecmp(pit->name.s, "mode", 4)==0) {
			str2int(&pit->body, &tmp.mode);
		}  else if(pit->name.len==8
				&& strncasecmp(pit->name.s, "interval", 8)==0) {
			str2int(&pit->body, &tmp.interval);
		}
	}
	if(tmp.name.s==NULL)
	{
		LM_ERR("invalid timer name\n");
		free_params(params_list);
		return -1;
	}
	/* check for same timer */
	nt = _stm_list;
	while(nt) {
		if(nt->name.len==tmp.name.len
				&& strncasecmp(nt->name.s, tmp.name.s, tmp.name.len)==0)
			break;
		nt = nt->next;
	}
	if(nt!=NULL)
	{
		LM_ERR("duplicate timer with same name: %.*s\n",
				tmp.name.len, tmp.name.s);
		free_params(params_list);
		return -1;
	}
	if(tmp.interval==0)
		tmp.interval = 120;

	nt = (stm_timer_t*)pkg_malloc(sizeof(stm_timer_t));
	if(nt==0)
	{
		LM_ERR("no more pkg memory\n");
		free_params(params_list);
		return -1;
	}
	memcpy(nt, &tmp, sizeof(stm_timer_t));
	nt->next = _stm_list;
	_stm_list = nt;
	free_params(params_list);
	return 0;
}

int stm_e_param(modparam_t type, void *val)
{
	param_t* params_list = NULL;
	param_hooks_t phooks;
	param_t *pit=NULL;
	stm_route_t tmp;
	stm_route_t *rt;
	stm_timer_t *nt;
	str s;
	char c;

	if(val==NULL)
		return -1;
	s.s = (char*)val;
	s.len = strlen(s.s);
	if(s.s[s.len-1]==';')
		s.len--;
	if (parse_params(&s, CLASS_ANY, &phooks, &params_list)<0)
		return -1;
	memset(&tmp, 0, sizeof(stm_route_t));
	for (pit = params_list; pit; pit=pit->next)
	{
		if (pit->name.len==5
				&& strncasecmp(pit->name.s, "timer", 5)==0) {
			tmp.timer = pit->body;
		} else if(pit->name.len==5
				&& strncasecmp(pit->name.s, "route", 5)==0) {
			s = pit->body;
		}
	}
	if(tmp.timer.s==NULL)
	{
		LM_ERR("invalid timer name\n");
		free_params(params_list);
		return -1;
	}
	/* get the timer */
	nt = _stm_list;
	while(nt) {
		if(nt->name.len==tmp.timer.len
				&& strncasecmp(nt->name.s, tmp.timer.s, tmp.timer.len)==0)
			break;
		nt = nt->next;
	}
	if(nt==NULL)
	{
		LM_ERR("timer not found - name: %.*s\n",
				tmp.timer.len, tmp.timer.s);
		free_params(params_list);
		return -1;
	}
	c = s.s[s.len];
	s.s[s.len] = '\0';
	tmp.route = route_get(&main_rt, s.s);
	s.s[s.len] = c;
	if(tmp.route == -1)
	{
		LM_ERR("invalid route: %.*s\n",
				s.len, s.s);
		free_params(params_list);
		return -1;
	}

	rt = (stm_route_t*)pkg_malloc(sizeof(stm_route_t));
	if(rt==0)
	{
		LM_ERR("no more pkg memory\n");
		free_params(params_list);
		return -1;
	}
	memcpy(rt, &tmp, sizeof(stm_route_t));
	rt->next = nt->rt;
	nt->rt = rt;
	free_params(params_list);
	return 0;
}

