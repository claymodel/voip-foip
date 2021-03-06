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
 * \brief Loopback PBX Module
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/pbx/pbx_loopback.c $", "$Revision: 4723 $")

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/config.h"
#include "callweaver/options.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/frame.h"
#include "callweaver/file.h"
#include "callweaver/cli.h"
#include "callweaver/lock.h"
#include "callweaver/linkedlists.h"
#include "callweaver/chanvars.h"
#include "callweaver/sched.h"
#include "callweaver/io.h"
#include "callweaver/utils.h"
#include "callweaver/crypto.h"
#include "callweaver/callweaver_db.h"

static char *tdesc = "Loopback Switch";

/* Loopback switch substitutes ${EXTEN}, ${CONTEXT}, and ${PRIORITY} into
   the data passed to it to try to get a string of the form:

	[exten]@context[:priority][/extramatch]
   
   Where exten, context, and priority are another extension, context, and priority
   to lookup and "extramatch" is an extra match restriction the *original* number 
   must fit if  specified.  The "extramatch" begins with _ like an exten pattern
   if it is specified.  Note that the search context MUST be a different context
   from the current context or the search will not succeed in an effort to reduce
   the likelihood of loops (they're still possible if you try hard, so be careful!)

*/


#define LOOPBACK_COMMON \
	char buf[1024]; \
	int res; \
	char *newexten=(char *)exten, *newcontext=(char *)context; \
	int newpriority=priority; \
	char *newpattern=NULL; \
	loopback_helper(buf, sizeof(buf), exten, context, priority, data); \
	loopback_subst(&newexten, &newcontext, &newpriority, &newpattern, buf); \
	cw_log(LOG_DEBUG, "Parsed into %s @ %s priority %d\n", newexten, newcontext, newpriority); \
	if (!strcasecmp(newcontext, context)) return -1


static char *loopback_helper(char *buf, int buflen, const char *exten, const char *context, int priority, const char *data)
{
	struct cw_var_t *newvariable;
	struct varshead headp;
	char tmp[80];

	snprintf(tmp, sizeof(tmp), "%d", priority);
	CW_LIST_HEAD_INIT_NOLOCK(&headp);
	newvariable = cw_var_assign("EXTEN", exten);
	CW_LIST_INSERT_HEAD(&headp, newvariable, entries);
	newvariable = cw_var_assign("CONTEXT", context);
	CW_LIST_INSERT_HEAD(&headp, newvariable, entries);
	newvariable = cw_var_assign("PRIORITY", tmp);
	CW_LIST_INSERT_HEAD(&headp, newvariable, entries);
	pbx_substitute_variables_varshead(&headp, data, buf, buflen);
	/* Substitute variables */
	while (!CW_LIST_EMPTY(&headp)) {           /* List Deletion. */
		newvariable = CW_LIST_REMOVE_HEAD(&headp, entries);
		cw_var_delete(newvariable);
	}
	return buf;
}

static void loopback_subst(char **newexten, char **newcontext, int *priority, char **newpattern, char *buf)
{
	char *con;
	char *pri;
	*newpattern = strchr(buf, '/');
	if (*newpattern) {
		*(*newpattern) = '\0';
		(*newpattern)++;
	}
	con = strchr(buf, '@');
	if (con) {
		*con = '\0';
		con++;
		pri = strchr(con, ':');
	} else
		pri = strchr(buf, ':');
	if (!cw_strlen_zero(buf))
		*newexten = buf;
	if (con && !cw_strlen_zero(con))
		*newcontext = con;
	if (pri && !cw_strlen_zero(pri))
		sscanf(pri, "%d", priority);
}

static int loopback_exists(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	LOOPBACK_COMMON;

	res = cw_exists_extension(chan, newcontext, newexten, newpriority, callerid);
	if (newpattern)
    {
        switch (cw_extension_pattern_match(exten, newpattern))
        {
        case EXTENSION_MATCH_EXACT:
        case EXTENSION_MATCH_STRETCHABLE:
        case EXTENSION_MATCH_POSSIBLE:
            break;
        default:
            res = 0;
            break;
        }
	}
    return res;
}

static int loopback_canmatch(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	LOOPBACK_COMMON;

	res = cw_canmatch_extension(chan, newcontext, newexten, newpriority, callerid);
	if (newpattern)
    {
        switch (cw_extension_pattern_match(exten, newpattern))
        {
        case EXTENSION_MATCH_EXACT:
        case EXTENSION_MATCH_STRETCHABLE:
        case EXTENSION_MATCH_POSSIBLE:
            break;
        default:
            res = 0;
            break;
        }
	}
	return res;
}

static int loopback_exec(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	LOOPBACK_COMMON;

	res = cw_exec_extension(chan, newcontext, newexten, newpriority, callerid);
	if (newpattern)
    {
        switch (cw_extension_pattern_match(exten, newpattern))
        {
        case EXTENSION_MATCH_EXACT:
        case EXTENSION_MATCH_STRETCHABLE:
        case EXTENSION_MATCH_POSSIBLE:
            break;
        default:
            res = -1;
            break;
        }
	}
	return res;
}

static int loopback_matchmore(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	LOOPBACK_COMMON;

	res = cw_matchmore_extension(chan, newcontext, newexten, newpriority, callerid);
	if (newpattern)
    {
        switch (cw_extension_pattern_match(exten, newpattern))
        {
        case EXTENSION_MATCH_EXACT:
        case EXTENSION_MATCH_STRETCHABLE:
        case EXTENSION_MATCH_POSSIBLE:
            break;
        default:
            res = 0;
            break;
        }
	}
	return res;
}

static struct cw_switch loopback_switch =
{
        name:                   "Loopback",
        description:    		"Loopback Dialplan Switch",
        exists:                 loopback_exists,
        canmatch:               loopback_canmatch,
        exec:                   loopback_exec,
        matchmore:              loopback_matchmore,
};

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	return 1;
}

int unload_module(void)
{
	cw_unregister_switch(&loopback_switch);
	return 0;
}

int load_module(void)
{
	cw_register_switch(&loopback_switch);
	return 0;
}

