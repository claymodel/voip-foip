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
 *  \brief icd_member.h  - membership of caller in distributor
 *
 * ICD_MEMBER
 *
 * This class represents the membership of a caller in a distributor and
 * queue. It is necessary because each caller can be in many queue/
 * distributors, just as one queue or distributor can hold many callers. 
 * This class provides the link between them.
 *
 * It also holds variables that are specific to the caller in a particular
 * queue or distributor. So length of time in queue, number of calls it
 * has been distributed against, these are all help in the ICD_MEMBER
 * structure.
 *
 */

#ifndef ICD_MEMBER_H
#define ICD_MEMBER_H

#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constructors and Destructors *****/

/* Constructor for a member object. */
    icd_member *create_icd_member(icd_queue * queue, icd_caller * caller, icd_config * data);

/* Destructor for a member object. Automatically resets the pointer
 * passed in to NULL. Only use this for objects created with
 * create_icd_member.
 */
    icd_status destroy_icd_member(icd_member ** memberp);

/* Initializer for an already created member object. */
    icd_status init_icd_member(icd_member * that, icd_queue * queue, icd_caller * caller, icd_config * data);

/* Clear the member object so it needs to be reinitialized to be reused. */
    icd_status icd_member__clear(icd_member * that);

/***** Behaviours *****/

/* Let's the caller know it is about to be distributed. Returns
 * ICD_ERESOURCE if the caller is already being distributed or bridged. 
 */
    icd_status icd_member__distribute(icd_member * that);

/* Identify a caller to link with the caller contained in this member object */
    icd_status icd_member__link_to_caller(icd_member * that, icd_member * associate);

/* Tell the caller contained in this object to start bridging to the linked caller. */
    icd_status icd_member__bridge(icd_member * that);

/* Record the time the member is added to the queue */
    icd_status icd_member__added_to_queue(icd_member * that);

/* Record the time the member is added to the distributor */
    icd_status icd_member__added_to_distributor(icd_member * that);

/* Increments the count of calls the member has been passed */
    icd_status icd_member__increment_calls(icd_member * that);

/* Increments the count of calls the member has been answered */
    icd_status icd_member__increment_answered(icd_member * that);

/* Prints the contents of the member object to the given file descriptor. */
    icd_status icd_member__dump(icd_member * that, int verbosity, int fd);

/* Sort functons for members cast as callers :) */
    int icd_member__cmp_call_start_time_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_caller_id_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_caller_id_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_caller_name_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_caller_name_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_timeout_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_timeout_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_last_mod_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_last_mod_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_start_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_start_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_callcount_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_callcount_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_state_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_state_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_priority_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_priority_reverse_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_last_state_change_order(icd_member * arg1, icd_member * arg2);
    int icd_member__cmp_last_state_change_reverse_order(icd_member * arg1, icd_member * arg2);

/***** Getters and Setters *****/

/* Sets the queue of this member object */
    icd_status icd_member__set_queue(icd_member * that, icd_queue * queue);

/* Gets the queue of this member object */
    icd_queue *icd_member__get_queue(icd_member * that);

/* Gets the distributor of this member object */
    icd_distributor *icd_member__get_distributor(icd_member * that);

/* Sets the caller of this member object */
    icd_status icd_member__set_caller(icd_member * that, icd_caller * name);

/* Gets the caller of this member object */
    icd_caller *icd_member__get_caller(icd_member * that);

/* Sets the number of calls dealt with by this member object */
    icd_status icd_member__set_calls(icd_member * that, int calls);

/* Gets the number of calls dealt with by this member object */
    int icd_member__get_calls(icd_member * that);

/* Sets the number of answered calls by this member object */
    icd_status icd_member__set_answered(icd_member * that, int answered);

/* Gets the number of answered alls by this member object */
    int icd_member__get_answered(icd_member * that);

/* Sets the name of this membership */
    icd_status icd_member__set_name(icd_member * that, char *newname);

/* Gets the name of this member object */
    char *icd_member__get_name(icd_member * that);

/* Gets the plugable functions of this member object */
    icd_plugable_fn *icd_member__get_plugable_fns(icd_member * that);

/***** Callback Setters *****/

/* Set the dump function for this list */
    icd_status icd__member__set_dump_func(icd_member * that, icd_status(*dump_fn) (icd_member * list, int verbosity,
            int fd, void *extra), void *extra);

/***** Fields *****/

/* Sets an extension field on this object. */
    icd_status icd_member__set_field(icd_member * that, char *key, void *value);

/* Gets the value from an extension field on this object, NULL if none. */
    void *icd_member__get_field(icd_member * that, char *key);

/***** Locking *****/

/* Lock the member object */
    icd_status icd_member__lock(icd_member * that);

/* Unlock the member object */
    icd_status icd_member__unlock(icd_member * that);

/**** Listeners ****/

    icd_status icd_member__add_listener(icd_member * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_member__remove_listener(icd_member * that, void *listener);

/***** Predefined behaviours *****/

/* Standard member list dump function */
    icd_status icd_member__standard_dump(icd_member * that, int verbosity, int fd, void *extra);

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

