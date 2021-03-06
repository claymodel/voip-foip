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
 * \brief Execute arbitrary authenticate commands
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_authenticate.c $", "$Revision: 4723 $")

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/app.h"
#include "callweaver/callweaver_db.h"
#include "callweaver/utils.h"

static char *tdesc = "Authentication Application";

static void *auth_app;
static const char *auth_name = "Authenticate";
static const char *auth_synopsis = "Authenticate a user";
static const char *auth_syntax = "Authenticate(password[,options[,trylimit]])";
static const char *auth_chanvar = "AUTH_STATUS";
static const char *auth_descrip =
"Requires a user to enter a given password in order to continue execution.\n"
"If the password begins with the '/' character, it is interpreted as\n"
"a file which contains a list of valid passwords (1 per line).\n"
"An optional set of options may be provided by concatenating any\n"
"of the following letters:\n"
"     a - Set account code to the password that is entered\n"
"     d - Interpret path as database key, not literal file\n"
"     D - Turn on debug output\n"
"     m - Interpret path as a file which contains a list of\n"
"         account codes and password hashes delimited with ':'\n"
"         one per line. When password matched, corresponding\n"
"         account code will be set\n"
"     r - Remove database key upon successful entry (valid with 'd' only)\n"
"\n"
"When using a database key, the value associated with the key can be anything.\n"
"The variable AUTH_STATUS is set on exit to either SUCCESS or FAILURE\n"
"trylimit allows you to specify the number of times to try authentication.\n"
"Default is 3, and if set to 0, no limit is enforced\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int auth_exec(struct cw_channel *chan, int argc, char **argv)
{
	int res=0;
	int retries, trylimit=3, nolimit=0;
	struct localuser *u;
	char passwd[256];
	char *prompt;
	int debug=0;
	int i;

	pbx_builtin_setvar_helper(chan, auth_chanvar, "FAILURE"); /* default to fail */
	
	if (argc < 1 || argc > 3) {
		cw_log(LOG_ERROR, "Syntax: %s\n", auth_syntax);
		return -1;
	}
	
	LOCAL_USER_ADD(u);

	if (chan->_state != CW_STATE_UP) {
		res = cw_answer(chan);
		if (res) {
			LOCAL_USER_REMOVE(u);
			return -1;
		}
	}
	
	if (argc > 1 && strchr(argv[1], 'D'))
		debug = 1;

	if(argc>2) {
		i = atoi(argv[2]);
		if (i == 0)
			nolimit++;
		else if (i > 0)
			trylimit = i;
		else
			cw_log(LOG_ERROR,"Can't set a negative limit.\n");
	}
	
	/* Start asking for password */
	prompt = "agent-pass";
    if (debug)
        cw_verbose("Starting %s with retry limit set to %i\n", auth_name, trylimit);
	
	for (retries = 0; retries < trylimit || nolimit; retries++) {
		res = cw_app_getdata(chan, prompt, passwd, sizeof(passwd) - 2, 0);
		if (res < 0)
			break;
		res = 0;
		if (argv[0][0] == '/') {
			if (argc > 1 && strchr(argv[1], 'd')) {
				char tmp[256];
				/* Compare against a database key */
				if (!cw_db_get(argv[0] + 1, passwd, tmp, sizeof(tmp))) {
					/* It's a good password */
					if (argc > 1 && strchr(argv[1], 'r')) {
						cw_db_del(argv[0] + 1, passwd);
					}
					break;
				}
			} else {
				/* Compare against a file */
				FILE *f;
				f = fopen(argv[0], "r");
				if (f) {
					char buf[256] = "";
					char md5passwd[33] = "";
					char *md5secret = NULL;

					while (!feof(f)) {
						fgets(buf, sizeof(buf), f);
						if (!feof(f) && !cw_strlen_zero(buf)) {
							buf[strlen(buf) - 1] = '\0';
							if (argc > 1 && strchr(argv[1], 'm')) {
								md5secret = strchr(buf, ':');
								if (md5secret == NULL)
									continue;
								*md5secret = '\0';
								md5secret++;
								cw_md5_hash(md5passwd, passwd);
								if (!strcmp(md5passwd, md5secret)) {
									if (argc > 1 && strchr(argv[1], 'a'))
										cw_cdr_setaccount(chan, buf);
									break;
								}
							} else {
								if (!strcmp(passwd, buf)) {
									if (argc > 1 && strchr(argv[1], 'a'))
										cw_cdr_setaccount(chan, buf);
									break;
								}
							}
						}
					}
					fclose(f);
					if (!cw_strlen_zero(buf)) {
						if (argc > 1 && strchr(argv[1], 'm')) {
							if (md5secret && !strcmp(md5passwd, md5secret))
								break;
						} else {
							if (!strcmp(passwd, buf))
								break;
						}
					}
				} else 
					cw_log(LOG_WARNING, "Unable to open file '%s' for authentication: %s\n", argv[0], strerror(errno));
			}
		} else {
			/* Compare against a fixed password */
			if (!strcmp(passwd, argv[0])) 
				break;
		}
		prompt="auth-incorrect";
	}
	if ((retries < trylimit) && !res) {
		if (argc > 1 && strchr(argv[1], 'a') && !strchr(argv[1], 'm')) 
			cw_cdr_setaccount(chan, passwd);
		res = cw_streamfile(chan, "auth-thankyou", chan->language);
		pbx_builtin_setvar_helper(chan, auth_chanvar, "SUCCESS"); /* default to fail */
		if (!res)
			res = cw_waitstream(chan, "");
	}
	LOCAL_USER_REMOVE(u);
	return res;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(auth_app);
	return res;
}

int load_module(void)
{
	auth_app = cw_register_application(auth_name, auth_exec, auth_synopsis, auth_syntax, auth_descrip);
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


