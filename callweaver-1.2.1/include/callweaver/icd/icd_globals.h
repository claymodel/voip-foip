/*
 * ICD - Intelligent Call Distributor 
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com>
 * Additions, Changes and Support by Tim R. Clark <tclark at shaw dot ca>
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o., Piotr Figurny and Michal Bielicki
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2005, Digium, Inc.
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
 
#ifndef ICD_GLOBALS_H

#define ICD_GLOBALS_H
#include "callweaver/icd/icd_common.h"
#include <signal.h>
#include <setjmp.h>

/* turn this flag via the cli in icd_command to enable a wack of icd debugging verbose */
extern int icd_debug;

/* turn this flag via the cli in icd_command to enable  verbosity of the icd show and dump cmds */
extern int icd_verbose;

extern char icd_delimiter;

extern icd_config_registry *app_icd_config_registry;

/* This is the lock customers add, remove and seek */
extern cw_mutex_t customers_lock;

/* %TC should this not be in here rather than icd_event.h
extern icd_event_factory *event_factory;
*/

jmp_buf env;

#endif

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

