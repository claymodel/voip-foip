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
 *
 * \brief Group Manipulation Applications
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_groupcount.c $", "$Revision: 5374 $")

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/utils.h"
#include "callweaver/cli.h"
#include "callweaver/app.h"

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;


static char *tdesc = "Group Management Routines";

static void *group_count_app;
static void *group_set_app;
static void *group_check_app;
static void *group_match_count_app;

static char *group_count_name = "GetGroupCount";
static char *group_set_name = "SetGroup";
static char *group_check_name = "CheckGroup";
static char *group_match_count_name = "GetGroupMatchCount";

static char *group_count_synopsis = "Get the channel count of a group";
static char *group_set_synopsis = "Set the channel's group";
static char *group_check_synopsis = "Check the channel count of a group against a limit";
static char *group_match_count_synopsis = "Get the channel count of all groups that match a pattern";

static char *group_count_syntax = "GetGroupCount([groupname][@category])";
static char *group_set_syntax = "SetGroup(groupname[@category])";
static char *group_check_syntax = "CheckGroup(max[@category])";
static char *group_match_count_syntax = "GetGroupMatchCount(groupmatch[@category])";

static char *group_count_descrip =
"Usage: GetGroupCount([groupname][@category])\n"
"  Calculates the group count for the specified group, or uses\n"
"the current channel's group if not specifed (and non-empty).\n"
"Stores result in GROUPCOUNT.  Always returns 0.\n"
"This application has been deprecated, please use the function\n"
"GroupCount.\n";

static char *group_set_descrip =
"Usage: SetGroup(groupname[@category])\n"
"  Sets the channel group to the specified value.  Equivalent to\n"
"Set(GROUP=group).  Always returns 0.\n";

static char *group_check_descrip =
"Usage: CheckGroup(max[@category])\n"
"  Checks that the current number of total channels in the\n"
"current channel's group does not exceed 'max'.  If the number\n"
"does not exceed 'max', set the GROUPSTATUS variable to OK.\n"
"Otherwise set GROUPSTATUS to MAX_EXCEEDED.\n"
"Always return 0\n";

static char *group_match_count_descrip =
"Usage: GetGroupMatchCount(groupmatch[@category])\n"
"  Calculates the group count for all groups that match the specified\n"
"pattern. Uses standard regular expression matching (see regex(7)).\n"
"Stores result in GROUPCOUNT.  Always returns 0.\n"
"This application has been deprecated, please use the function\n"
"GroupMatchCount.\n";


static int group_count_exec(struct cw_channel *chan, int argc, char **argv)
{
	static int deprecation_warning = 0;
	char group[80] = "";
	char category[80] = "";
	char ret[80] = "";
	struct localuser *u;
	char *grp;
	int res = 0;
	int count;

	if (!deprecation_warning) {
	        cw_log(LOG_WARNING, "The GetGroupCount application has been deprecated, please use the GROUP_COUNT function.\n");
		deprecation_warning = 1;
	}

	if (argc != 1) {
		cw_log(LOG_ERROR, "Syntax: %s\n", group_count_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);

	cw_app_group_split_group(argv[0], group, sizeof(group), category, sizeof(category));

	if (cw_strlen_zero(group)) {
		grp = pbx_builtin_getvar_helper(chan, category);
		strncpy(group, grp, sizeof(group) - 1);
	}

	count = cw_app_group_get_count(group, category);
	snprintf(ret, sizeof(ret), "%d", count);
	pbx_builtin_setvar_helper(chan, "GROUPCOUNT", ret);

	LOCAL_USER_REMOVE(u);

	return res;
}

static int group_match_count_exec(struct cw_channel *chan, int argc, char **argv)
{
	static int deprecation_warning = 0;
	char group[80] = "";
	char category[80] = "";
	char ret[80] = "";
	struct localuser *u;
	int res = 0;
	int count;

	if (!deprecation_warning) {
	        cw_log(LOG_WARNING, "The GetGroupMatchCount application has been deprecated, please use the GROUP_MATCH_COUNT function.\n");
		deprecation_warning = 1;
	}

	if (argc != 1) {
		cw_log(LOG_ERROR, "Syntax: %s\n", group_match_count_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);

	cw_app_group_split_group(argv[0], group, sizeof(group), category, sizeof(category));

	if (!cw_strlen_zero(group)) {
		count = cw_app_group_match_get_count(group, category);
		snprintf(ret, sizeof(ret), "%d", count);
		pbx_builtin_setvar_helper(chan, "GROUPCOUNT", ret);
	}

	LOCAL_USER_REMOVE(u);

	return res;
}

static int group_set_exec(struct cw_channel *chan, int argc, char **argv)
{
	static int deprecation_warning = 0;
	struct localuser *u;
	int res = 0;

	if (!deprecation_warning) {
	        cw_log(LOG_WARNING, "The SetGroup application has been deprecated, please use the GROUP() function.\n");
		deprecation_warning = 1;
	}

	if (argc != 1) {
		cw_log(LOG_ERROR, "Syntax: %s\n", group_set_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);
	
	if (cw_app_group_set_channel(chan, argv[0]))
		cw_log(LOG_WARNING, "SetGroup requires an argument (group name)\n");

	LOCAL_USER_REMOVE(u);
	return res;
}

static int group_check_exec(struct cw_channel *chan, int argc, char **argv)
{
	static int deprecation_warning = 0;
	char limit[80]="";
	char category[80]="";
	struct localuser *u;
	int res = 0;
	int max, count;

	if (!deprecation_warning) {
	        cw_log(LOG_WARNING, "The CheckGroup application has been deprecated, please use a combination of the GotoIf application and the GROUP_COUNT() function.\n");
		deprecation_warning = 1;
	}

	if (argc != 1) {
		cw_log(LOG_ERROR, "Syntax: %s\n", group_check_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);

  	cw_app_group_split_group(argv[0], limit, sizeof(limit), category, sizeof(category));

 	if ((sscanf(limit, "%d", &max) == 1) && (max > -1)) {
		count = cw_app_group_get_count(pbx_builtin_getvar_helper(chan, category), category);
		if (count > max) {
			pbx_builtin_setvar_helper(chan, "GROUPSTATUS", "OK");
		} else {
			pbx_builtin_setvar_helper(chan, "GROUPSTATUS", "MAX_EXCEEDED");
		}
	} else
		cw_log(LOG_WARNING, "CheckGroup requires a positive integer argument (max)\n");

	LOCAL_USER_REMOVE(u);
	return res;
}

static int group_show_channels(int fd, int argc, char *argv[])
{
#define FORMAT_STRING  "%-25s  %-20s  %-20s\n"

	int numchans = 0;
	struct cw_group_info *gi = NULL;
	regex_t regexbuf;
	int havepattern = 0;

	if (argc < 3 || argc > 4)
		return RESULT_SHOWUSAGE;
	
	if (argc == 4) {
		if (regcomp(&regexbuf, argv[3], REG_EXTENDED | REG_NOSUB))
			return RESULT_SHOWUSAGE;
		havepattern = 1;
	}

	cw_cli(fd, FORMAT_STRING, "Channel", "Group", "Category");
	cw_app_group_list_lock();

	gi = cw_app_group_list_head();
	while (gi) {
		if (!havepattern || !regexec(&regexbuf, gi->group, 0, NULL, 0)) {
			cw_cli(fd, FORMAT_STRING, gi->chan->name, gi->group, (cw_strlen_zero(gi->category) ? "(default)" : gi->category));
			numchans++;
 		}
		gi = CW_LIST_NEXT(gi, list);
	}
	cw_app_group_list_unlock();

	if (havepattern)
		regfree(&regexbuf);

	cw_cli(fd, "%d active channel%s\n", numchans, (numchans != 1) ? "s" : "");
	return RESULT_SUCCESS;
#undef FORMAT_STRING
}


static char show_channels_usage[] =
"Usage: group show channels [pattern]\n"
"       Lists all currently active channels with channel group(s) specified.\n"
"       Optional regular expression pattern is matched to group names for each channel.\n";

static struct cw_cli_entry  cli_show_channels =
	{ { "group", "show", "channels", NULL }, group_show_channels, "Show active channels with group(s)", show_channels_usage};

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	cw_cli_unregister(&cli_show_channels);
	res |= cw_unregister_application(group_count_app);
	res |= cw_unregister_application(group_set_app);
	res |= cw_unregister_application(group_check_app);
	res |= cw_unregister_application(group_match_count_app);
	return res;
}

int load_module(void)
{
	group_count_app = cw_register_application(group_count_name, group_count_exec, group_count_synopsis, group_count_syntax, group_count_descrip);
	group_set_app = cw_register_application(group_set_name, group_set_exec, group_set_synopsis, group_set_syntax, group_set_descrip);
	group_check_app = cw_register_application(group_check_name, group_check_exec, group_check_synopsis, group_check_syntax, group_check_descrip);
	group_match_count_app = cw_register_application(group_match_count_name, group_match_count_exec, group_match_count_syntax, group_match_count_synopsis, group_match_count_descrip);
	cw_cli_register(&cli_show_channels);
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


