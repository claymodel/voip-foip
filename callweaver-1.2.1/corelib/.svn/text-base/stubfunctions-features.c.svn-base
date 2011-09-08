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

#include <stdio.h>
#include <callweaver/features.h>


static int stub_cw_park_call(struct cw_channel *chan, struct cw_channel *host, int timeout, int *extout)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return -1;
}

static int stub_cw_masq_park_call(struct cw_channel *rchan, struct cw_channel *host, int timeout, int *extout)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return -1;
}

static char *stub_cw_parking_ext(void)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return NULL;
}

static char *stub_cw_pickup_ext(void)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return NULL;
}

static int stub_cw_bridge_call(struct cw_channel *chan, struct cw_channel *peer,struct cw_bridge_config *config)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return -1;
}

static int stub_cw_pickup_call(struct cw_channel *chan)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
	return -1;
}

static void stub_cw_register_feature(struct cw_call_feature *feature)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
}

static void stub_cw_unregister_feature(struct cw_call_feature *feature)
{
	cw_log(LOG_NOTICE, "res_features not loaded!\n");
}




int (*cw_park_call)(struct cw_channel *chan, struct cw_channel *host, int timeout, int *extout) =
	stub_cw_park_call;

int (*cw_masq_park_call)(struct cw_channel *rchan, struct cw_channel *host, int timeout, int *extout) =
	stub_cw_masq_park_call;

char *(*cw_parking_ext)(void) =
	stub_cw_parking_ext;

char *(*cw_pickup_ext)(void) =
	stub_cw_pickup_ext;

int (*cw_bridge_call)(struct cw_channel *chan, struct cw_channel *peer,struct cw_bridge_config *config) =
	stub_cw_bridge_call;

int (*cw_pickup_call)(struct cw_channel *chan) =
	stub_cw_pickup_call;

void (*cw_register_feature)(struct cw_call_feature *feature) =
	stub_cw_register_feature;

void (*cw_unregister_feature)(struct cw_call_feature *feature) =
	stub_cw_unregister_feature;

