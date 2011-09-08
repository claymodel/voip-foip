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
 *  \brief icd_caller.h  -  a call to distribute
 *
 * The icd_caller module represents a call that needs dispatching by the system.
 *
 * The module has many attributes that are controlled through getters and
 * setters, including the distributor the call belongs to, the music on hold
 * to be played, the state of the call, the owner of the call(?), the id of
 * the caller, the timeout, start time, authentication token, last modified time,
 * and the channel associated with this object. It has notifier callbacks,
 * listeners, default pluggable behaviours, and a set of comparison functions
 * for calls.
 *
 * Authentication can happen in one of two ways on a caller. It can be set
 * using icd_caller__set_authorization() with a token that the caller treats
 * as a black box. Or an authentication function can be places in the icd_caller,
 * and the caller asked to perform an authentication itself.
 *
 */

#ifndef ICD_CALLER_H
#define ICD_CALLER_H

#include <time.h>
#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    icd_status icd_caller_load_module(void);
/***** Init - Destroyer *****/

/* Create a caller. data is a parsable string of parameters. */
    icd_caller *create_icd_caller(icd_config * data);

/* Destroy a caller, freeing its memory and cleaning up after it. */
    icd_status destroy_icd_caller(icd_caller ** callp);

/* Initialize a previously created caller */
    icd_status init_icd_caller(icd_caller * that, icd_config * data);

/* Clear a caller so it must be reinitialized to be reused */
   icd_status icd_caller__clear(icd_caller * that);

/* Clear the caller (logout) so that it can login again. */
   icd_status icd_caller__clear_suspend(icd_caller * that);
/***** Actions *****/

/* Add the caller to a queue. This creates a member object that is stored in the queue. */
    icd_status icd_caller__add_to_queue(icd_caller * that, icd_queue * queue);

/* Remove the caller from a queue and destroys the member object. */
    icd_status icd_caller__remove_from_queue(icd_caller * that, icd_queue * queue);
   
    icd_status icd_caller__remove_from_all_queues(icd_caller * that);

/* Lets the caller know that the queue has added it to its distributor. */
    icd_status icd_caller__added_to_distributor(icd_caller * that, icd_distributor * distributor);

/* Lets the caller know that it has been removed from this distributor. */
    icd_status icd_caller__removed_from_distributor(icd_caller * that, icd_distributor * distributor);

/* Attach a channel to the caller structure */
    icd_status icd_caller__assign_channel(icd_caller * that, cw_channel * chan);

/* Identify a caller to link with this caller */
    icd_status icd_caller__link_to_caller(icd_caller * that, icd_caller * associate);

/* Unlink this caller from another */
    icd_status icd_caller__unlink_from_caller(icd_caller * that, icd_caller * associate);

/* Start bridging to the linked caller. */
    icd_status icd_caller__bridge(icd_caller * that);

/* Authenticate caller using the provided auth_fn() */
    icd_status icd_caller__authenticate(icd_caller * that, void *extra);

/* Returns 1 if the caller has a given role */
    int icd_caller__has_role(icd_caller * that, icd_role role);

/* Adds a role to the caller */
    icd_status icd_caller__add_role(icd_caller * that, icd_role role);

/* Clears a single role from the caller */
    icd_status icd_caller__clear_role(icd_caller * that, icd_role role);

/* Gets all the roles from the caller struct */
    int icd_caller__get_roles(icd_caller * that);

/* Clears all roles from the caller */
    icd_status icd_caller__clear_roles(icd_caller * that);

/* Returns 1 if the caller has a given flag */
    int icd_caller__has_flag(icd_caller * that, icd_flag flag);

/* Adds a flag to the caller */
    icd_status icd_caller__add_flag(icd_caller * that, icd_flag flag);

/* Clears a single flag from the caller */
    icd_status icd_caller__clear_flag(icd_caller * that, icd_flag flag);

/* Gets all the flags from the caller struct */
    int icd_caller__get_flags(icd_caller * that);

/* Clears all flags from the caller */
    icd_status icd_caller__clear_flags(icd_caller * that);

/* Get an existing  memberships object for this caller,  */
    icd_member_list *icd_caller__get_memberships(icd_caller * that);

/* Get an existing head from memberships object for this caller,  */
    icd_member *icd_caller__get_memberships_peek(icd_caller * that);

/* Get an existing  member object for a given queue for this caller, NULL if not a member */
    icd_member *icd_caller__get_member_for_queue(icd_caller * that, icd_queue * queue);

/* Get an existing  member object for a given distributor for this caller, NULL if not a member */
    icd_member *icd_caller__get_member_for_distributor(icd_caller * that, icd_distributor * dist);

/* Prints the contents of the caller structure to the given file descriptor. */
    icd_status icd_caller__dump(icd_caller * that, int verbosity, int fd);

/* Start the caller thread. */
    icd_status icd_caller__start_caller_response(icd_caller * that);

/* Pause the caller thread. */
    icd_status icd_caller__pause_caller_response(icd_caller * that);

/* Stop and finish the caller thread. */
    icd_status icd_caller__stop_caller_response(icd_caller * that);

/* Make sure this caller has a channel */
    cw_channel *icd_caller__create_channel(icd_caller * caller);

/* If this caller is on-hook, ring them to get the channel up */
    icd_status icd_caller__dial_channel(icd_caller * that);

/***** Getters and Setters *****/

/* Set the ID for that caller call */
    icd_status icd_caller__set_id(icd_caller * that, int);

/* Get the ID for that caller call */
    int icd_caller__get_id(icd_caller * that);

/* Set the name string to identify this caller. */
    icd_status icd_caller__set_name(icd_caller * that, char *name);

/* Get the name string being used to identify this caller. */
    char *icd_caller__get_name(icd_caller * that);

/* Set the call count caller. */
    icd_status icd_caller__set_callcount(icd_caller * that, int callcount);

/* Get the callcount. */
    int icd_caller__get_callcount(icd_caller * that);

/* Set the distributor strategy to use to assign this call. */
    icd_status icd_caller__set_distributor(icd_caller * that, icd_distributor * dist);

/* Get the distributor being used to assign this call. */
    icd_distributor *icd_caller__get_distributor(icd_caller * that);

/* Set the music on hold for that caller call */
    icd_status icd_caller__set_moh(icd_caller * that, char *moh);

/* set bridge technology */
    icd_status icd_caller__set_bridge_technology(icd_caller * that, icd_bridge_technology tech);

/* get bridge technology */
    icd_bridge_technology icd_caller__get_bridge_technology(icd_caller * that);

    icd_conference *icd_caller__get_conference(icd_caller * that);

/* Set the conf_mode  for that caller call */
    icd_status icd_caller__set_conf_mode(icd_caller * that, int conf_mode);

/* Get the conf_mode for that  caller call */
    int icd_caller__get_conf_mode(icd_caller * that);

/* Get the music on hold for that  caller call */
    char *icd_caller__get_moh(icd_caller * that);

/* Set the announce for that caller call */
    icd_status icd_caller__set_announce(icd_caller * that, char *announce);

/* Get the announce for that caller call */
    char *icd_caller__get_announce(icd_caller * that);

/* Set the channel string for that caller call */
    icd_status icd_caller__set_channel_string(icd_caller * that, char *channel);

/* Get the channel string for that caller call */
    char *icd_caller__get_channel_string(icd_caller * that);

/* Set the caller id for that caller call */
    icd_status icd_caller__set_caller_id(icd_caller * that, char *caller_id);

/* Get the caller id for that caller call */
    char *icd_caller__get_caller_id(icd_caller * that);

/* Set the state for that caller call */
    icd_status icd_caller__set_state(icd_caller * that, icd_caller_state state);

/* Get the state for that caller call */
    icd_caller_state icd_caller__get_state(icd_caller * that);

/* Get the state string for that caller call */
    char *icd_caller__get_state_string(icd_caller * that);

/* Set the owner for that caller call */
    icd_status icd_caller__set_owner(icd_caller * that, void *owner);

/* Get the owner for that caller call */
    void *icd_caller__get_owner(icd_caller * that);

/* Set the channel for that caller call */
    icd_status icd_caller__set_channel(icd_caller * that, cw_channel * chan);

/* Get the channel for that caller call */
    cw_channel *icd_caller__get_channel(icd_caller * that);

/* Set the timeout for that caller call */
    icd_status icd_caller__set_timeout(icd_caller * that, int);

/* Get the timeout for that caller call */
    int icd_caller__get_timeout(icd_caller * that);

/* Set the time that the caller call started */
    icd_status icd_caller__set_start(icd_caller * that, time_t);

/* Set the time that the caller call started to now */
    icd_status icd_caller__set_start_now(icd_caller * that);

/* Get the time that the caller call started */
    time_t icd_caller__get_start(icd_caller * that);

/* Set the time that the caller call was last modified */
    icd_status icd_caller__set_last_mod(icd_caller * that, time_t);

/* Get the time that the caller call was last modified */
    time_t icd_caller__get_last_mod(icd_caller * that);

/* Set the authorization for that caller */
    icd_status icd_caller__set_authorization(icd_caller * that, void *);

/* Get the authorization for that caller */
    void *icd_caller__get_authorization(icd_caller * that);

/* Get the priority for the caller */
    icd_status icd_caller__set_priority(icd_caller * that, int);

/* Get the priority for the caller */
    int icd_caller__get_priority(icd_caller * that);

/* Set the time of the last time the caller's call changed state */
    icd_status icd_caller__set_last_state_change(icd_caller * that, time_t);

/* Get the time of the last time the caller's call changed state */
    time_t icd_caller__get_last_state_change(icd_caller * that);

/* Set pushback flag */
    icd_status icd_caller__set_pushback(icd_caller * that);

/* Reset pushback flag */
    icd_status icd_caller__reset_pushback(icd_caller * that);

/* Get pushback flag */
    int icd_caller__get_pushback(icd_caller * that);

/* Set on-hook flag */
    icd_status icd_caller__set_onhook(icd_caller * that, int onhook);

/* Get on-hook flag */
    int icd_caller__get_onhook(icd_caller * that);

/* Set dynamic flag */
    icd_status icd_caller__set_dynamic(icd_caller * that, int dynamic);

/* Get dynamic flag */
    int icd_caller__get_dynamic(icd_caller * that);

/* Get using_caller_thread flag */
    int icd_caller__get_using_caller_thread(icd_caller * that);

/* Get acknowledge flag */
    int icd_caller__get_acknowledge_call(icd_caller * that);

/* Get last_announce_position */
    int icd_caller__get_last_announce_position(icd_caller * that);
    int icd_caller__set_last_announce_position(icd_caller * that, int position);

/* Get last_announce_time */
    time_t icd_caller__get_last_announce_time(icd_caller * that);
    int icd_caller__set_last_announce_time(icd_caller * that);

    void icd_caller__set_holdannounce(icd_caller * that, icd_queue_holdannounce * ha);

/* Get count of member (ie distributor) for a caller */
    int icd_caller__get_member_count(icd_caller * that);

/* Get current position in for member (ie distributor)  */
    int icd_caller__get_position(icd_caller * that, icd_member * member);

/* Get pending for member (ie distributor)  */
    int icd_caller__get_pending(icd_caller * that, icd_member * member);

/* Get the type of entertainment None, MOH, or Ring etc  */
    icd_entertain icd_caller__get_entertained(icd_caller * that);

/* Set active member */
    icd_status icd_caller__set_active_member(icd_caller * that, icd_member * active_member);

/* Get active member */
    icd_member *icd_caller__get_active_member(icd_caller * that);

/* Get list of associations */
    icd_caller_list *icd_caller__get_associations(icd_caller * that);

/* Get list of Listeners, nned this for factory events ouside of icd_caller.c */
    icd_listeners *icd_caller__get_listeners(icd_caller * that);

/* Get the plugable fn list */
    icd_plugable_fn_list *icd_caller__get_plugable_fns_list(icd_caller * that);

/* Get the Name of the current plugable function ptr */
    char *icd_caller__get_plugable_fns_name(icd_caller * that);

/***** Callback Setters *****/
/* set the plugable function to find the struc of plugable functions */
    icd_status icd_caller__set_plugable_fn_ptr(icd_caller * that,
        icd_plugable_fn * (*get_plugable_fn) (icd_caller *));

/* Set the dump function for this caller */
    icd_status icd_caller__set_dump_fn(icd_caller * that, icd_status(*dump_fn) (icd_caller * caller, int verbosity,
            int fd, void *extra), void *extra);

/***** Locking *****/

/* Lock the entire caller */
    icd_status icd_caller__lock(icd_caller * that);

/* Unlock the entire caller */
    icd_status icd_caller__unlock(icd_caller * that);

/**** Listeners ****/

    icd_status icd_caller__add_listener(icd_caller * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_caller__remove_listener(icd_caller * that, void *listener);

/**** Predefined Pluggable Actions ****/
/* Get an existing plugable_fn_list  object r */
    icd_plugable_fn_list *icd_caller__get_plugable_fn_list(icd_caller * that);

    icd_plugable_fn *icd_caller_get_plugable_fns(icd_caller * that);

/* Notify function that causes caller to require authentication. */
    int icd_caller__require_authentication(icd_event * event, void *extra);

/* Always returns true without doing anything */
    int icd_caller__authenticate_always_succeeds(icd_caller *, void *);

/* Authenticate via password on the keypad. */
    int icd_caller__authenticate_by_keypad(icd_caller * caller, void *authenticate_token);

/* Plays "Agent logged in", useful as an authenticate notification function. */
    int icd_caller__play_logged_in_message(icd_event * event, void *extra);

/* Use this when you don't want the notify function to do anything. */
    int icd_caller__dummy_notify_hook(icd_event * that, void *extra);

    /* Notify hook when the caller is deleted from the queue */
    int icd_caller__delq_notify_hook(icd_event * that, void *extra);

    /* Notify hook when the caller is deleted from the distributor queue */
    int icd_caller__deldist_notify_hook(icd_event * that, void *extra);

/* Standard state action functions */

/* Standard function for conferneces */
    int icd_caller__standard_state_conference(icd_event * event, void *extra);

/* Standard function for reacting to the ready state */
    int icd_caller__standard_state_ready(icd_event * event, void *extra);

/* Standard function for reacting to the distribute state */
    int icd_caller__standard_state_distribute(icd_event * event, void *extra);

/* Standard function for reacting to the get channels and bridge state */
    int icd_caller__standard_state_get_channels(icd_event * event, void *extra);

/* Standard function for reacting to the bridged state */
    int icd_caller__standard_state_bridged(icd_event * event, void *extra);

/* Standard function for reacting to the call end state */
    int icd_caller__standard_state_call_end(icd_event * event, void *extra);

/* Standard function for reacting to the bridge failed state */
    int icd_caller__standard_state_bridge_failed(icd_event * event, void *extra);

/* Standard function for reacting to the channel failed state */
    int icd_caller__standard_state_channel_failed(icd_event * event, void *extra);

/* Standard function for reacting to the associate failed state */
    int icd_caller__standard_state_associate_failed(icd_event * event, void *extra);

/* Standard function for reacting to the suspend state */
    int icd_caller__standard_state_suspend(icd_event * event, void *extra);

/* Standard behaviours called from inside the caller state action functions */
    icd_status icd_caller__play_sound_file(icd_caller *caller, char *file);

/* Default behaviours for moh (why are these static?) */
    icd_status icd_caller__standard_start_waiting(icd_caller * caller);
    icd_status icd_caller__standard_stop_waiting(icd_caller * caller);

/* Standard function to prepare a caller object for entering a distributor */
    icd_status icd_caller__standard_prepare_caller(icd_caller * caller);

/* Standard function for cleaning up a caller after a bridge ends or fails */
    icd_status icd_caller__standard_cleanup_caller(icd_caller * caller);

/* Standard function for starting the bridging process on a caller */
    icd_status icd_caller__standard_launch_caller(icd_caller * caller);

/* Standard caller dump function */
    icd_status icd_caller__standard_dump(icd_caller * caller, int verbosity, int fd, void *extra);

/***** Comparison functions (" all "icd_caller *") *****/
    int icd_caller__cmp_call_start_time_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_caller_id_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_caller_id_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_caller_name_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_caller_name_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_timeout_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_timeout_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_last_mod_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_last_mod_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_start_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_start_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_callcount_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_callcount_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_state_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_state_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_priority_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_priority_reverse_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_last_state_change_order(icd_caller * caller1, icd_caller * caller2);
    int icd_caller__cmp_last_state_change_reverse_order(icd_caller * caller1, icd_caller * caller2);
    icd_status icd_caller__remove_all_associations(icd_caller * that);

/* TBD - Eliminate need for these function calls to be public, if possible */
    int icd_caller__in_thread(icd_caller * that);
    void icd_caller__dump_debug(icd_caller * that);
    icd_status icd_caller__join_callers(icd_caller * that, icd_caller * associate);
    icd_caller *icd_caller__clone_if_necessary(icd_caller * that);
    void icd_caller__dump_debug_fd(icd_caller * that, int fd, char *indent);
    void icd_caller__invalidate(icd_caller * that);
    icd_status icd_caller__fail_bridging(icd_caller * bridger);
    void icd_caller__loop(icd_caller * that, int do_spawn);
    void *icd_caller__get_param(icd_caller * that, char *param);
    icd_status icd_caller__set_param(icd_caller * that, char *param, void *value);
    icd_status icd_caller__set_param_string(icd_caller * that, char *param, void *value);
    icd_status icd_caller__return_to_distributors(icd_caller * caller);
    icd_status icd_caller__set_state_on_associations(icd_caller * that, icd_caller_state state);
    void icd_caller__start_waiting(icd_caller * that);
    void icd_caller__stop_waiting(icd_caller * that);

    int icd_caller__set_chimepos(icd_caller * that, int pos);
    int icd_caller__get_chimepos(icd_caller * that);

    time_t icd_caller__set_chimenext(icd_caller * that, time_t next);
    time_t icd_caller__get_chimenext(icd_caller * that);
    void *icd_caller__standard_run(void *ptr);
    icd_plugable_fn *icd_caller__get_pluggable_fn(icd_caller * that);
    int icd_caller__owns_channel(icd_caller * that);
    void icd_caller__params_to_astheader(icd_caller * that, char *prefix, char *buf, size_t size);

    void icd_caller__init_group_registry(void);
    void icd_caller__destroy_group_registry(void);
    icd_caller_group *create_icd_caller_group(char *name, icd_caller * owner);
    void destroy_icd_caller_group(icd_caller_group ** group);
    icd_status icd_caller__rem_group_pointer(icd_caller * caller, icd_caller_group * group);
    icd_status icd_caller__add_group_pointer(icd_caller * caller, icd_caller_group * group);
    icd_status icd_caller__join_group(icd_caller * caller, icd_caller_group * group);
    icd_status icd_caller__leave_group(icd_caller * caller, icd_caller_group * group);
    icd_status icd_caller__join_group_by_name(icd_caller * caller, char *name);
    icd_status icd_caller__leave_group_by_name(icd_caller * caller, char *name);
    void icd_caller__group_chanup(icd_caller_group * group);
    int icd_caller__del_param(icd_caller * that, char *param);

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

