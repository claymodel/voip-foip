/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Russell Bryant <russelb@clemson.edu> 
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
 * \brief Functions for reading or setting the MusicOnHold class
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/funcs/func_moh.c $", "$Revision: 4723 $")

#include "callweaver/module.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/utils.h"


static void *moh_function;
static const char *moh_func_name = "MUSICCLASS";
static const char *moh_func_synopsis = "Read or Set the MusicOnHold class";
static const char *moh_func_syntax = "MUSICCLASS()";
static const char *moh_func_desc =
	"This function will read or set the music on hold class for a channel.\n";


static char *function_moh_read(struct cw_channel *chan, int argc, char **argv, char *buf, size_t len)
{
	cw_copy_string(buf, chan->musicclass, len);

	return buf;
}

static void function_moh_write(struct cw_channel *chan, int argc, char **argv, const char *value) 
{
	cw_copy_string(chan->musicclass, value, MAX_MUSICCLASS);
}


static char *tdesc = "MOH functions";

int unload_module(void)
{
        return cw_unregister_function(moh_function);
}

int load_module(void)
{
        moh_function = cw_register_function(moh_func_name, function_moh_read, function_moh_write, moh_func_synopsis, moh_func_syntax, moh_func_desc);
	return 0;
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	return 0;
}

/*
Local Variables:
mode: C
c-file-style: "linux"
indent-tabs-mode: nil
End:
*/
