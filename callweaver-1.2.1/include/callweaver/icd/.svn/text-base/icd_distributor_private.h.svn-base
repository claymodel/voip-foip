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
 
#ifndef ICD_DISTRIBUTOR_PRIVATE_H
#define ICD_DISTRIBUTOR_PRIVATE_H
#include "callweaver/icd/icd_plugable_fn.h"

/*===== Private functions =====*/

icd_status icd_distributor__set_config_params(icd_distributor * that, icd_config * data);
icd_status icd_distributor__create_lists(icd_distributor * that, icd_config * data);
icd_status icd_distributor__correct_list_config(icd_config * data);
icd_status icd_distributor__set_config_params(icd_distributor * that, icd_config * data);
icd_status icd_distributor__create_thread(icd_distributor * that);
void *icd_distributor__run(void *that);

typedef enum {
    ICD_DISTRIBUTOR_STATE_CREATED, ICD_DISTRIBUTOR_STATE_INITIALIZED,
    ICD_DISTRIBUTOR_STATE_CLEARED, ICD_DISTRIBUTOR_STATE_DESTROYED,
    ICD_DISTRIBUTOR_STATE_LAST_STANDARD
} icd_distributor_state;

struct icd_distributor {
    char name[ICD_STRING_LEN];
    icd_member_list *customers;
    icd_member_list *agents;
    icd_plugable_fn *(*get_plugable_fn) (icd_caller * caller);
      icd_status(*link_fn) (icd_distributor *, void *extra);
      icd_status(*dump_fn) (icd_distributor *, int verbosity, int fd, void *extra);
    void *(*run_fn) (void *that);
    void *link_fn_extra;
    void *dump_fn_extra;
    int customer_list_allocated;
    int agent_list_allocated;
    int allocated;
    icd_distributor_state state;
    icd_thread_state thread_state;
    icd_listeners *listeners;
    cw_mutex_t lock;
    pthread_t thread;
    cw_cond_t wakeup;
    icd_memory *memory;
    void_hash_table *params;
/* This is for distibutor to know that link_fn call is needed  */    
    unsigned int number_of_callers_added;
};

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

