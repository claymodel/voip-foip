/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 * Tilghman Lesher <tlesher@vcch.com>
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
 * \brief Custom localtime functions for multiple timezones
 */

#ifndef _CALLWEAVER_LOCALTIME_H
#define _CALLWEAVER_LOCALTIME_H

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

extern int cw_tzsetwall(void);
extern void cw_tzset(const char *name);
extern struct tm *cw_localtime(const time_t *timep, struct tm *p_tm, const char *zone);
extern time_t cw_mktime(struct tm * const tmp, const char *zone);
extern char *cw_ctime(const time_t * const timep);
extern char *cw_ctime_r(const time_t * const timep, char *buf);

#endif /* _CALLWEAVER_LOCALTIME_H */
