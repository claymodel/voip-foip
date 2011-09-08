/*
 * CallWeaver -- An open source telephony toolkit.
 *
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

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <callweaver/monitor.h>

static int stub_cw_monitor_start( struct cw_channel *chan, const char *format_spec, const char *fname_base, int need_lock )
{
	cw_log(LOG_NOTICE, "res_monitor not loaded!\n");
	return -1;
}

static int stub_cw_monitor_stop( struct cw_channel *chan, int need_lock)
{
	cw_log(LOG_NOTICE, "res_monitor not loaded!\n");
	return -1;
}

static int stub_cw_monitor_change_fname( struct cw_channel *chan, const char *fname_base, int need_lock )
{
	cw_log(LOG_NOTICE, "res_monitor not loaded!\n");
	return -1;
}

static void stub_cw_monitor_setjoinfiles(struct cw_channel *chan, int turnon)
{
	cw_log(LOG_NOTICE, "res_monitor not loaded!\n");
}



int (*cw_monitor_start)( struct cw_channel *chan, const char *format_spec, const char *fname_base, int need_lock ) =
	stub_cw_monitor_start;

int (*cw_monitor_stop)( struct cw_channel *chan, int need_lock) =
	stub_cw_monitor_stop;

int (*cw_monitor_change_fname)( struct cw_channel *chan, const char *fname_base, int need_lock ) =
	stub_cw_monitor_change_fname;

void (*cw_monitor_setjoinfiles)(struct cw_channel *chan, int turnon) =
	stub_cw_monitor_setjoinfiles;

