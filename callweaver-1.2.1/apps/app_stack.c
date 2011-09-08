/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (c) 2004-2005 Tilghman Lesher <app_stack_v003@the-tilghman.com>.
 *
 * This code is released by the author with no restrictions on usage.
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
 * \brief Stack applications Gosub, Return, etc.
 * 
 * \ingroup applications
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "callweaver.h"

#include "callweaver/options.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/chanvars.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/config.h"
#include "callweaver/app.h"

#define STACKVAR	"~GOSUB~STACK~"

static const char *tdesc = "Stack Routines";

static void *gosub_app;
static void *gosubif_app;
static void *return_app;
static void *pop_app;

static const char *gosub_name = "Gosub";
static const char *gosubif_name = "GosubIf";
static const char *return_name = "Return";
static const char *pop_name = "StackPop";

static const char *gosub_synopsis = "Jump to label, saving return address";
static const char *gosubif_synopsis = "Jump to label, saving return address";
static const char *return_synopsis = "Return from gosub routine";
static const char *pop_synopsis = "Remove one address from gosub stack";

static const char *gosub_syntax = "Gosub([[context, ]exten, ]label|priority[(arg,...)])";
static const char *gosubif_syntax = "GosubIf(condition ? [[context, ]exten, ]label|priority[(arg, ...)] [: [[context, ]exten, ]label|priority][(arg, ...)])";
static const char *return_syntax = "Return()";
static const char *pop_syntax = "StackPop()";

static const char *gosub_descrip =
"  Jumps to the label specified, saving the return address.\n"
"  Returns 0 if the label exists or -1 otherwise.\n";
static const char *gosubif_descrip =
"  If the condition is true, then jump to labeliftrue.  If false, jumps to\n"
"labeliffalse, if specified.  In either case, a jump saves the return point\n"
"in the dialplan, to be returned to with a Return.\n"
"  Returns 0 if the label exists or -1 otherwise.\n";
static const char *return_descrip =
"  Jumps to the last label in the stack, removing it.\n"
"  Returns 0 if there's a label in the stack or -1 otherwise.\n";
static const char *pop_descrip =
"  Removes last label in the stack, discarding it.\n"
"  Always returns 0, even if the stack is empty.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int pop_exec(struct cw_channel *chan, int argc, char **argv)
{
	pbx_builtin_setvar_helper(chan, STACKVAR, NULL);

	return 0;
}

static int return_exec(struct cw_channel *chan, int argc, char **argv)
{
	char buf[3 + 3 + 1];
	char *label;
	int i;

	label = pbx_builtin_getvar_helper(chan, STACKVAR);

	if (cw_strlen_zero(label)) {
		cw_log(LOG_ERROR, "Return without Gosub: stack is empty\n");
		return -1;
	}

	cw_separate_app_args(label, ',', 100, argv);
	argc = atoi(argv[3]);

	memcpy(buf, "ARG", 3);
	for (i = atoi(argv[3]); i; i--) {
		sprintf(buf+3, "%d", i+1);
		pbx_builtin_setvar_helper(chan, buf, NULL);
	}

	pbx_builtin_setvar_helper(chan, STACKVAR, NULL);

	if (cw_explicit_goto(chan, argv[0], argv[1], atoi(argv[2]))) {
		cw_log(LOG_WARNING, "No statement after Gosub to return to?\n");
		return -1;
	}

	return 0;
}

static int gosub_exec(struct cw_channel *chan, int argc, char **argv)
{
	char buf[3 + 1 + CW_MAX_CONTEXT + 1 + CW_MAX_EXTENSION + 1 + 11 + 11];
	char *context, *exten, *p, *q;
	int i;

	if (argc < 1 || argc > 3) {
		cw_log(LOG_ERROR, "Syntax: %s\n", gosub_syntax);
		return -1;
	}

	exten = (argc > 1 ? argv[argc-2] : NULL);
	context = (argc > 2 ? argv[argc-3] : NULL);

	if ((p = strchr(argv[argc-1], '('))) {
		*(p++) = '\0';
		if (!(q = strrchr(p, ')'))) {
			cw_log(LOG_ERROR, "Syntax: %s\n", gosub_syntax);
			return -1;
		}

		*q = '\0';
	}

	snprintf(buf, sizeof(buf), "%s,%s,%d", chan->context, chan->exten, chan->priority + 1);

	if (cw_explicit_gotolabel(chan, context, exten, argv[argc-1]))
		return -1;

	argc = (p ? cw_separate_app_args(p, ',', 100, argv) : 0);
	i = strlen(buf);
	snprintf(buf+i, sizeof(buf)-i, ",%d", argc);
	pbx_builtin_pushvar_helper(chan, STACKVAR, buf);

	memcpy(buf, "ARG", 3);
	for (i = 0; i < argc; i++) {
		sprintf(buf+3, "%d", i+1);
		pbx_builtin_pushvar_helper(chan, buf, argv[i]);
	}

	return 0;
}

static int gosubif_exec(struct cw_channel *chan, int argc, char **argv)
{
	char *s, *q;
	int i;

	/* First argument is "<condition ? ..." */
	if (argc < 1 || !(s = strchr(argv[0], '?'))) {
		cw_log(LOG_ERROR, "Syntax: %s\n", gosubif_syntax);
		return -1;
	}

	/* Trim trailing space from the condition */
	q = s;
	do { *(q--) = '\0'; } while (q >= argv[0] && isspace(*q));

	do { *(s++) = '\0'; } while (isspace(*s));

	if (pbx_checkcondition(argv[0])) {
		/* True: we want everything between '?' and ':' */
		argv[0] = s;
		for (i = 0; i < argc; i++) {
			if ((s = strchr(argv[i], ':'))) {
				do { *(s--) = '\0'; } while (s >= argv[i] && isspace(*s));
				argc = i + 1;
				break;
			}
		}
		return gosub_exec(chan, argc, argv);
	} else {
		/* False: we want everything after ':' (if anything) */
		argv[0] = s;
		for (i = 0; i < argc; i++) {
			if ((s = strchr(argv[i], ':'))) {
				do { *(s++) = '\0'; } while (isspace(*s));
				argv[i] = s;
				return gosub_exec(chan, argc - i, argv + i);
			}
		}
		/* No ": ..." so we just drop through */
		return 0;
	}
}

int unload_module(void)
{
	int res = 0;

	res |= cw_unregister_application(return_app);
	res |= cw_unregister_application(pop_app);
	res |= cw_unregister_application(gosubif_app);
	res |= cw_unregister_application(gosub_app);

	STANDARD_HANGUP_LOCALUSERS;
	return res;
}

int load_module(void)
{
	pop_app = cw_register_application(pop_name, pop_exec, pop_synopsis, pop_syntax, pop_descrip);
	return_app = cw_register_application(return_name, return_exec, return_synopsis, return_syntax, return_descrip);
	gosubif_app = cw_register_application(gosubif_name, gosubif_exec, gosubif_synopsis, gosubif_syntax, gosubif_descrip);
	gosub_app = cw_register_application(gosub_name, gosub_exec, gosub_synopsis, gosub_syntax, gosub_descrip);

	return 0;
}

char *description(void)
{
	return (char *) tdesc;
}

int usecount(void)
{
	int res;

	STANDARD_USECOUNT(res);

	return res;
}
