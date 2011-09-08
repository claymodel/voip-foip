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

#ifndef ICD_MODULE_API_H
#define ICD_MODULE_API_H

#include <assert.h>
#include "callweaver/icd/icd_globals.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_event.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_distributor_private.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_command.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"
#include "callweaver/icd/icd_fieldset.h"
 
/* Only Used in the external c file than contains the custom code eg icd_module_mystuff.c */
int icd_module_unload(void);
int icd_module_load(icd_config_registry * registry);

struct icd_loadable_object {
    char filename[ICD_STRING_LEN];
    int (*load_fn) (icd_config_registry * registry); 
    int (*unload_fn) (void);
    int (*dist_run) (icd_distributor * that, char *name, icd_config * data);
    /* need to add run for callers */
    void *lib;
    icd_memory *memory;
    int allocated;
};

/* Only used by api_icd.c->load_module(void)  */
icd_status icd_module_load_dynamic_module(icd_config_registry * registry);

/* Only used by api_icd.c->unload_module(void)  */
icd_status icd_module_unload_dynamic_modules(void);

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

