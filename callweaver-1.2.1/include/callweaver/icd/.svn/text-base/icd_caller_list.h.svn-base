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
 
/*! \file
 * \brief icd_caller_list.h  -  list of callers
 *
 * The icd_caller_list is a typical icd_list with a few extensions that are
 * specific to keeping a list of icd_callers. It sets some default attributes
 * for the callers it holds, such as the moh, announce, and default distributor
 * for the callers to belong to.
 *
 * In addition, it keeps track of a few events fired off by individual
 * icd_caller elements and amalgamates them into one place. So if you want
 * to be notified when any icd_caller in this list is bridged, you
 * would register yourself with icd_caller_list__set_assigned_agent_notify_fn().
 *
 */

#ifndef ICD_CALLER_LIST_H
#define ICD_CALLER_LIST_H
#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Init - Destroyer *****/

/* Create a caller list. data is a parsable string of parameters. */
    icd_caller_list *create_icd_caller_list(char *name, icd_config * data);

/* Destroy a caller list, freeing its memory and cleaning up after it. */
    icd_status destroy_icd_caller_list(icd_caller_list ** listp);

/* Initialize a caller list */
    icd_status init_icd_caller_list(icd_caller_list * that, char *name, icd_config * data);

/* Clear a caller list */
    icd_status icd_caller_list__clear(icd_caller_list * that);

/***** Actions *****/

/* Adds a caller to the list, returns success or failure (typesafe wrapper). */
    icd_status icd_caller_list__push(icd_caller_list * that, icd_caller * new_call);

/* Retrieves a caller from the list, returns null if there are none (typesafe wrapper). */
    icd_caller *icd_caller_list__pop(icd_caller_list * that);

/* Returns 1 if there are calls in the list, 0 otherwise. */
    int icd_caller_list__has_callers(icd_caller_list * that);

/* Returns the numerical position of the caller from the head of the list (0 based) */
    int icd_caller_list__caller_position(icd_caller_list * that, icd_caller * target);

/* Prints the contents of the caller structures to the given file descriptor. */
    icd_status icd_caller_list__dump(icd_caller_list * that, int verbosity, int fd);

/* Retrieves a caller from the list when given an id. */
    icd_caller *icd_caller_list__fetch_caller(icd_caller_list * that, char *id);

/* Removes a caller from the list when given an id, returns success or failure. */
    icd_status icd_caller_list__remove_caller(icd_caller_list * that, char *id);

/* Removes a caller from the list when given the object itself, returns success or failure. */
    icd_status icd_caller_list__remove_caller_by_element(icd_caller_list * that, icd_caller * target);

/***** Getters and Setters *****/

/* Set the list name */
    icd_status icd_caller_list__set_name(icd_caller_list * that, char *name);

/* Get the list name */
    char *icd_caller_list__get_name(icd_caller_list * that);

/***** Callback Setters *****/

/* Rollup function for notifying state change of any icd_caller in list */
    icd_status icd_caller_list__set_state_change_notify_fn(icd_caller_list * that,
        int (*state_fn) (icd_event * event, void *extra), void *extra);

/* Rollup function for notifying channel attachment of any icd_caller in list */
    icd_status icd_caller_list__set_channel_attached_notify_fn(icd_caller_list * that,
        int (*chan_fn) (icd_event * event, void *extra), void *extra);

/* Rollup function for notifying that an icd_caller in list has been linked to another caller */
    icd_status icd_caller_list__set_linked_notify_fn(icd_caller_list * that, int (*link_fn) (icd_event * event,
            void *extra), void *extra);

/* Rollup function for notifying that an icd_caller in list has been bridged */
    icd_status icd_caller_list__set_bridged_notify_fn(icd_caller_list * that, int (*bridge_fn) (icd_event * event,
            void *extra), void *extra);

/* Rollup function for notifying that one of the icd_caller in list has tried to authenticate */
    icd_status icd_caller_list__set_authenticate_notify_fn(icd_caller_list * that,
        int (*authn_fn) (icd_event * event, void *extra), void *extra);

/***** Locking *****/

/* Lock the list */
    icd_status icd_caller_list__lock(icd_caller_list * that);

/* Unlock the list */
    icd_status icd_caller_list__unlock(icd_caller_list * that);

/***** Listeners *****/

    icd_status icd_caller_list__add_listener(icd_caller_list * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_caller_list__remove_listener(icd_caller_list * that, void *listener);

/**** Predefined Hooks ****/

    int icd_caller_list__dummy_notify(icd_event * event, void *extra);

/* Standard caller list dump function */
    icd_status icd_caller_list__standard_dump(icd_list * list, int verbosity, int fd, void *extra);

#ifdef __cplusplus
}
#endif
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

