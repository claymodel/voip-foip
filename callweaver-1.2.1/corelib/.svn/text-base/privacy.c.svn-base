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
 * \brief Privacy Routines
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL$", "$Revision$")

#include "callweaver/channel.h"
#include "callweaver/file.h"
#include "callweaver/app.h"
#include "callweaver/dsp.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/callweaver_db.h"
#include "callweaver/phone_no_utils.h"
#include "callweaver/privacy.h"
#include "callweaver/utils.h"
#include "callweaver/lock.h"

int cw_privacy_check(char *dest, char *cid)
{
	char tmp[256] = "";
	char *trimcid = "";
	char *n, *l;
	int res;
	char key[256], result[256];
	if (cid)
		cw_copy_string(tmp, cid, sizeof(tmp));
	cw_callerid_parse(tmp, &n, &l);
	if (l) {
		cw_shrink_phone_number(l);
		trimcid = l;
	}
	snprintf(key, sizeof(key), "%s/%s", dest, trimcid);
	res = cw_db_get("privacy", key, result, sizeof(result));
	if (!res) {
		if (!strcasecmp(result, "allow"))
			return CW_PRIVACY_ALLOW;
		if (!strcasecmp(result, "deny"))
			return CW_PRIVACY_DENY;
		if (!strcasecmp(result, "kill"))
			return CW_PRIVACY_KILL;
		if (!strcasecmp(result, "torture"))
			return CW_PRIVACY_TORTURE;
	}
	return CW_PRIVACY_UNKNOWN;
}

int cw_privacy_reset(char *dest)
{
	if (!dest)
		return -1;
	return cw_db_deltree("privacy", dest);
}

int cw_privacy_set(char *dest, char *cid, int status)
{
	char tmp[256] = "";
	char *trimcid = "";
	char *n, *l;
	int res;
	char key[256];
	if (cid)
		cw_copy_string(tmp, cid, sizeof(tmp));
	cw_callerid_parse(tmp, &n, &l);
	if (l) {
		cw_shrink_phone_number(l);
		trimcid = l;
	}
	if (cw_strlen_zero(trimcid)) {
		/* Don't store anything for empty Caller*ID */
		return 0;
	}
	snprintf(key, sizeof(key), "%s/%s", dest, trimcid);
	if (status == CW_PRIVACY_UNKNOWN) 
		res = cw_db_del("privacy", key);
	else if (status == CW_PRIVACY_ALLOW)
		res = cw_db_put("privacy", key, "allow");
	else if (status == CW_PRIVACY_DENY)
		res = cw_db_put("privacy", key, "deny");
	else if (status == CW_PRIVACY_KILL)
		res = cw_db_put("privacy", key, "kill");
	else if (status == CW_PRIVACY_TORTURE)
		res = cw_db_put("privacy", key, "torture");
	else
		res = -1;
	return res;
}
