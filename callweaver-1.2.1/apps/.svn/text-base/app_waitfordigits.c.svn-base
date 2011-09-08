/*
 * vim:ts=4:sw=4:autoindent:smartindent:cindent
 *
 * Asterisk -- A telephony toolkit for Linux.
 *
 * Trivial application to playback a sound file
 * 
 * Copyright (C) 1999, Mark Spencer
 *
 * Mark Spencer <markster@linux-support.net>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <callweaver/lock.h>
#include <callweaver/file.h>
#include <callweaver/logger.h>
#include <callweaver/channel.h>
#include <callweaver/pbx.h>
#include <callweaver/module.h>
#include <callweaver/translate.h>
#include <callweaver/utils.h>
#include <callweaver/cli.h>

#ifdef CW_MODULE_INFO
#define CW_VERSION_1_4
#else
#define CW_VERSION_1_2
#endif

static char *tdesc = "Waits for overlap dialed digits";

static void *waitfordigits_app;
static const char *waitfordigits_name = "WaitForDigits";
static const char *waitfordigits_synopsis = "Wait for digits";
static const char *waitfordigits_syntax = "WaitForDigits(milliseconds,[maxnum],addexten,[control],[priority]):\n";
static const char *waitfordigits_descrip =
" WaitForDigits(milliseconds,[maxnum],addexten,[control],[priority]):\n"
" Waits for given milliseconds for digits which are dialed in overlap mode.\n" 
" Upon exit, sets the variable NEWEXTEN to the new extension (old + new digits)\n"
" The original EXTEN is not touched\n"
"\n"
" [maxnum] -	waitfordigits waits only until [maxnum] digits are dialed\n"
" 		then it exits immediately\n"
"\n"
" 'addexten' -	indicates that the dialed digits should be added\n"
" 		to the predialed extension. Note that this is not necessary\n"
"		for mISDN channels, they add overlap digits automatically\n"
"\n"
" [control] -	this option allows you to send a CW_CONTROL_XX frame\n"
" 		after the timeout has expired.\n" 
"		see (include/callweaver/frame.h) for possible values\n"
" 		this is useful for mISDN channels to send a PROCEEDING\n"
"		which is the number 15.\n"
"\n"
" [priority] -	Jumps to the given Priority in the current context. Note\n"
"		that the extension might have changed in waitfordigits!\n"
"		This option is especially useful if you use waitfordigits\n"
"		in the 's' extension to jump to the 1. priority of the\n"
"		post-dialed extenions.\n"
"\n"
"Channel Variables:\n"
" ALREADY_WAITED indicates if waitfordigits has already waited. If this\n"
" 		 Variable is set, the application exits immediatly, without\n"
"		 further waiting.\n"
"\n"
" WAIT_STOPKEY	 Set this to a Digit which you want to use for immediate\n"
"		 exit of waitfordigits before the timeout has expired.\n"
"		 WAIT_STOPKEY=# would be a typical value\n"
;

//#include "compat.h"

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;



#define CHANNEL_INFO " (%s,%s)"

#define CHANNEL_INFO_PARTS(a) , a->cid.cid_num, a->exten

static int waitfordigits_exec(struct cw_channel *chan, int argc, char **argv)
{
	int res = 0;
	struct localuser *u;

	/** Default values **/
	int timeout=3500;
	int maxnum=20;
	int addexten=0;
	
	int control=0;
	int nextprio=0;

	const char *tmp=pbx_builtin_getvar_helper(chan,"ALREADY_WAITED") ;
	const char *stopkey=pbx_builtin_getvar_helper(chan,"WAIT_STOPKEY") ;
	char aw=0;

	char dig=0;
	char numsubst[255];
	
	if (argc < 1 || argc > 5)
	{
		cw_log(LOG_ERROR, "Syntax: %s\n", waitfordigits_syntax);
		return -1;
	}

	if (tmp)
	{
		aw=atoi(tmp);
	}
	
	LOCAL_USER_ADD(u);
	
// waitfordigits(milliseconds,[maxnum],addexten,[control],[priority]):\n"
	if (argv[0])
	{
		timeout=atoi(argv[0]);
	}
	else
	{
		cw_log(LOG_WARNING, "WaitForDigits was passed invalid data '%s'. The timeout must be a positive integer.\n", argv[0]);
	}
	if (argv[0] == NULL || timeout <= 0)

	if (argc >= 2 && argv[0])
	{
		maxnum = atoi(argv[0]);
	}

	if (argc >= 3 && argv[2])
	{
		if (strcmp(argv[2], "addexten") == 0 || strcmp(argv[2], "true"))
		{
			addexten = 1;
		}
	}

	if (argc >= 4 && argv[3])
	{
		control=atoi(argv[3]);
	}

	if (argc >= 5 && argv[4])
	{
		nextprio=atoi(argv[4]);
	}

	cw_verbose("You passed timeout:%d maxnum:%d addexten:%d control:%d\n",
	      timeout, maxnum, addexten, control);
	
	/** Check wether we have something to do **/
	if (chan->_state == CW_STATE_UP || aw > 0 )
	{
	  LOCAL_USER_REMOVE(u);
	  return 0;
	}
	
	pbx_builtin_setvar_helper(chan,"ALREADY_WAITED","1");
	  
	/** Saving predialed Extension **/
	strcpy(numsubst, chan->exten);
	while ( (strlen(numsubst)< maxnum) && (dig=cw_waitfordigit(chan, timeout)))
	{
		int l=strlen(numsubst); 
		if (dig<=0)
		{
			if (dig ==-1)
			{
				cw_log(LOG_NOTICE, "Timeout reached.\n ");
			}
			else
			{
				cw_log(LOG_NOTICE, "Error at adding dig: %c!\n",dig);
				res=-1;
			}
			break;
		}

		if (stopkey && (dig == stopkey[0]) ) {
			break;
		}
		  
		numsubst[l]=dig;
		numsubst[l+1]=0;
		//cw_log(LOG_NOTICE, "Adding Dig to Chan: %c\n",dig);
	}
	  
	/** Restoring Extension if requested **/
	if (addexten)
	{
		cw_verbose("Overwriting extension:%s with new Number: %s\n",chan->exten, numsubst);
		strcpy(chan->exten, numsubst);
	}
	else
	{
		cw_verbose( "Not Overwriting extension:%s with new Number: %s\n",chan->exten, numsubst);
	}
	  
	/** Posting Exten to Var: NEWEXTEN **/
	pbx_builtin_setvar_helper(chan,"NEWEXTEN",numsubst);
  
	if (chan->_state != CW_STATE_UP && (control>0) ) {
		cw_verbose( "Sending CONTROL: %d  to Channel %s\n",control, chan->exten);
		cw_indicate(chan, control);
	}
	else
	{
		cw_verbose( "Not Sending any control to Channel %s state is %d\n",chan->exten, chan->_state);
	}
	
	if (nextprio>0)
	{ 
		chan->priority=--nextprio;
	}

	LOCAL_USER_REMOVE(u);

	return res;
}

int unload_module(void)
{
	STANDARD_HANGUP_LOCALUSERS;
	return cw_unregister_application(waitfordigits_app);
}

int load_module(void)
{
	waitfordigits_app = cw_register_application(waitfordigits_name, waitfordigits_exec, waitfordigits_synopsis, waitfordigits_syntax, waitfordigits_descrip);
	return 0;
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}

