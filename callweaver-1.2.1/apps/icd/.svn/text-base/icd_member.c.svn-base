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
 *  \brief icd_member.c - membership of caller in distributor
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
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_fieldset.h"

static icd_module module_id = ICD_MEMBER;

typedef enum {
    ICD_MEMBER_STATE_CREATED, ICD_MEMBER_STATE_INITIALIZED,
    ICD_MEMBER_STATE_CLEARED, ICD_MEMBER_STATE_DESTROYED,
    ICD_MEMBER_STATE_L, CW_STANDARD
} icd_member_state;

struct icd_member {
    char name[ICD_STRING_LEN];
    icd_queue *queue;
    icd_caller *caller;
    icd_distributor *distributor;
    time_t entered_distributor;
    time_t entered_queue;
    int calls;                  /* TC is this call count ? why do we have it here & in caller struct 
                                   Does all the call count fails etc belong in here ?? */
    int answered_calls;
    icd_fieldset *params;
    icd_member_state state;
    icd_plugable_fn *(*get_plugable_fn) (icd_caller * caller);
      icd_status(*dump_fn) (icd_member * member, int verbosity, int fd, void *extra);
    void *dump_fn_extra;
    icd_listeners *listeners;
    icd_memory *memory;
    int allocated;
    cw_mutex_t lock;
};

/*===== Private APIs =====*/

/*static char *icd_member__build_name(icd_member *that);*/

/*===== Public Implementations =====*/

/* Constructor for a member object. */
icd_member *create_icd_member(icd_queue * queue, icd_caller * caller, icd_config * data)
{
    icd_member *member;
    icd_status result;

    assert(queue != NULL);
    assert(caller != NULL);

    /* make a new member object from scratch */
    ICD_MALLOC(member, sizeof(icd_member));
    if (member == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Member\n");
        return NULL;
    }
    member->allocated = 1;
    member->state = ICD_MEMBER_STATE_CREATED;
    result = init_icd_member(member, queue, caller, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(member);
        return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, member, member->name, module_id, ICD_EVENT_CREATE, NULL,
        member->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_member(&member);
        return NULL;
    }
    return member;
}

/* Destructor for a member object. Automatically resets the pointer
 * passed in to NULL. Only use this for objects created with
 * create_icd_member.
 */
icd_status destroy_icd_member(icd_member ** memberp)
{
    icd_status vetoed;
    int clear_result;

    assert(memberp != NULL);
    assert((*memberp) != NULL);

    /* First, notify event hooks and listeners */
    vetoed =
        icd_event_factory__generate(event_factory, *memberp, (*memberp)->name, module_id, ICD_EVENT_DESTROY, NULL,
        (*memberp)->listeners, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD Member %s has been vetoed\n", icd_member__get_name(*memberp));
        return ICD_EVETO;
    }

    /* Next, clear the member object of any attributes and free dependent structures. */
    if ((clear_result = icd_member__clear(*memberp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Finally, destroy the member object itself if from the heap */
    if ((*memberp)->allocated) {
        (*memberp)->state = ICD_MEMBER_STATE_DESTROYED;
        ICD_FREE((*memberp));
        *memberp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initializer for an already created member object. */
icd_status init_icd_member(icd_member * that, icd_queue * queue, icd_caller * caller, icd_config * data)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(queue != NULL);
    assert(caller != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_member));
    cw_mutex_init(&that->lock);
    that->queue = queue;
    that->caller = caller;
    that->distributor = icd_queue__get_distributor(queue);
    //if (icd_distributor__get_plugable_fn_ptr(that->distributor) !=NULL)
    that->get_plugable_fn = icd_distributor__get_plugable_fn_ptr(that->distributor);

    snprintf(that->name, sizeof(that->name), "Queue %s Member %s", icd_queue__get_name(that->queue),
        icd_caller__get_name(that->caller));

    that->entered_queue = (time_t) 0;
    that->entered_distributor = (time_t) 0;
    that->calls = 0;
    that->answered_calls = 0;
    that->listeners = create_icd_listeners();
    that->allocated = 0;
    that->params = NULL;

    if (data != NULL) {
        that->params = icd_config__get_any_value(data, "params", NULL);
    }

    vetoed = icd_event__generate(ICD_EVENT_INIT, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    that->state = ICD_MEMBER_STATE_INITIALIZED;

    return ICD_SUCCESS;
}

/* Clear the member object so it needs to be reinitialized to be reused. */
icd_status icd_member__clear(icd_member * that)
{
    icd_status vetoed;

    assert(that != NULL);

    /* Special case to deal with previously cleared */
    if (that->state == ICD_MEMBER_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_CLEAR, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Lock the object and remove everything in it. */
    if (icd_member__lock(that) == ICD_SUCCESS) {

        that->queue = NULL;
        that->distributor = NULL;
        that->caller = NULL;
        that->calls = 0;
        that->answered_calls = 0;
        destroy_icd_listeners(&(that->listeners));
        that->params = NULL;
        icd_member__unlock(that);
        cw_mutex_destroy(&(that->lock));
        /* the clear call was above the unlock call causing it to fail -Tony */
        that->state = ICD_MEMBER_STATE_CLEARED;
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Member %s in order to clear it\n",
        icd_member__get_name(that));
    return ICD_ELOCK;
}

/***** Behaviours *****/

/* Let's the caller know it is about to be distributed. Returns
 * ICD_ERESOURCE if the caller is already being distributed or bridged. 
 * Returns ICD_ELOCK if the caller could not be locked.
 */
icd_status icd_member__distribute(icd_member * that)
{
    icd_status vetoed;
    icd_status result;

    assert(that != NULL);
    assert(that->caller != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTED, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    result = icd_caller__set_state(that->caller, ICD_CALLER_STATE_DISTRIBUTING);
    if (result == ICD_SUCCESS) {
        icd_caller__set_active_member(that->caller, that);
    }
    return result;
}

/* Identify a caller to link with the caller contained in this member object */
icd_status icd_member__link_to_caller(icd_member * that, icd_member * associate)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(that->caller != NULL);
    assert(associate != NULL);
    assert(associate->caller != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_LINK, associate);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    return icd_caller__link_to_caller(that->caller, associate->caller);
}

/* Tell the caller contained in this object to start bridging to the linked caller. */
icd_status icd_member__bridge(icd_member * that)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(that->caller != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_BRIDGE, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    return icd_caller__bridge(that->caller);
}

/* Record the time the member is added to the queue */
icd_status icd_member__added_to_queue(icd_member * that)
{
    icd_status vetoed;

    assert(that != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_ADD, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    time(&that->entered_queue);
    return ICD_SUCCESS;
}

/* Record the time the member is added to the distributor */
icd_status icd_member__added_to_distributor(icd_member * that)
{
    icd_status vetoed;

    assert(that != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTE, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    time(&that->entered_distributor);
    return ICD_SUCCESS;
}

/* Increments the count of calls the member has been passed */
icd_status icd_member__increment_calls(icd_member * that)
{
    icd_status result;

    assert(that != NULL);

    result = icd_member__lock(that);
    if (result != ICD_SUCCESS) {
        return ICD_ELOCK;
    }
    that->calls++;
    icd_member__unlock(that);
    /* Note that we should increment the caller count here. */

    return ICD_SUCCESS;
}

/* Increments the count of calls the member has answered */
icd_status icd_member__increment_answered(icd_member * that)
{
    icd_status result;

    assert(that != NULL);

    result = icd_member__lock(that);
    if (result != ICD_SUCCESS) {
        return ICD_ELOCK;
    }
    that->answered_calls++;
    icd_member__unlock(that);
    /* Note that we should increment the caller count here. */

    return ICD_SUCCESS;
}

/* Prints the contents of the member object to the given file descriptor. */
icd_status icd_member__dump(icd_member * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(that->dump_fn != NULL);

    return that->dump_fn(that, verbosity, fd, that->dump_fn_extra);
}

int icd_member__cmp_call_start_time_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_call_start_time_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_caller_id_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_caller_id_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_caller_id_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_caller_id_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_caller_name_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_caller_name_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_caller_name_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_caller_name_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_timeout_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_timeout_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_timeout_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_timeout_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_last_mod_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_last_mod_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_last_mod_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_last_mod_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_start_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_start_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_start_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_start_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_callcount_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_callcount_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_callcount_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_callcount_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_state_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_state_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_state_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_state_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_priority_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_priority_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_priority_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_priority_order(arg2->caller, arg1->caller);
}

int icd_member__cmp_last_state_change_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_last_state_change_order(arg1->caller, arg2->caller);
}

int icd_member__cmp_last_state_change_reverse_order(icd_member * arg1, icd_member * arg2)
{
    assert(arg1 != NULL);
    assert(arg2 != NULL);

    return icd_caller__cmp_last_state_change_order(arg2->caller, arg1->caller);
}

/***** Getters and Setters *****/

/* Sets the queue of this member object */
icd_status icd_member__set_queue(icd_member * that, icd_queue * queue)
{
    assert(that != NULL);

    that->queue = queue;
    return ICD_SUCCESS;
}

/* Gets the queue of this member object */
icd_queue *icd_member__get_queue(icd_member * that)
{
    assert(that != NULL);

    return that->queue;
}

/* Gets the distributor of this member object */
icd_distributor *icd_member__get_distributor(icd_member * that)
{
    assert(that != NULL);

    return that->distributor;
}

/* Sets the caller of this member object */
icd_status icd_member__set_caller(icd_member * that, icd_caller * caller)
{
    assert(that != NULL);

    that->caller = caller;
    return ICD_SUCCESS;
}

/* Gets the caller of this member object */
icd_caller *icd_member__get_caller(icd_member * that)
{
    assert(that != NULL);

    return that->caller;
}

/* Sets the number of calls dealt with by this member object */
icd_status icd_member__set_calls(icd_member * that, int calls)
{
    assert(that != NULL);

    that->calls = calls;
    return ICD_SUCCESS;
}

/* Gets the number of calls dealt with by this member object */
int icd_member__get_calls(icd_member * that)
{
    assert(that != NULL);

    return that->calls;
}

/* Sets the number of answered calls by this member object */
icd_status icd_member__set_answered(icd_member * that, int answered)
{
    assert(that != NULL);

    that->answered_calls = answered;
    return ICD_SUCCESS;
}

/* Gets the number of answered alls by this member object */
int icd_member__get_answered(icd_member * that)
{
    assert(that != NULL);

    return that->answered_calls;
}

/* Sets the name of this member object */
icd_status icd_member__set_name(icd_member * that, char *newname)
{
    assert(that != NULL);
    strncpy(that->name, newname, sizeof(that->name));
    return ICD_SUCCESS;
}

/* Gets the name of this member object */
char *icd_member__get_name(icd_member * that)
{
    assert(that != NULL);

    return strlen(that->name) ? that->name : NULL;
}

/***** Callback Setters *****/

/* Set the dump function for this list */
icd_status icd__member__set_dump_func(icd_member * that, icd_status(*dump_fn) (icd_member * list, int verbosity,
        int fd, void *extra), void *extra)
{
    assert(that != NULL);

    that->dump_fn = dump_fn;
    that->dump_fn_extra = extra;
    return ICD_SUCCESS;
}

/***** Fields *****/

/* Sets an extension field on this object. */
icd_status icd_member__set_field(icd_member * that, char *key, void *value)
{
    char namebuf[200];
    icd_status vetoed;

    assert(that != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_SETFIELD, key);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    if (that->params == NULL) {
        snprintf(namebuf, 200, "Fieldset for %s", icd_member__get_name(that));
        that->params = create_icd_fieldset(namebuf);
    }
    return icd_fieldset__set_value(that->params, key, value);
}

/* Gets the value from an extension field on this object, NULL if none. */
void *icd_member__get_field(icd_member * that, char *key)
{
    assert(that != NULL);

    if (that->params == NULL) {
        return NULL;
    }
    return icd_fieldset__get_value(that->params, key);
}

/***** Locking *****/

/* Lock the member object */
icd_status icd_member__lock(icd_member * that)
{
    int result;

    assert(that != NULL);

    if (that->state == ICD_MEMBER_STATE_CLEARED || that->state == ICD_MEMBER_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    result = cw_mutex_lock(&that->lock);
    if (result == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/* Unlock the member object */
icd_status icd_member__unlock(icd_member * that)
{
    int result;

    assert(that != NULL);

    if (that->state == ICD_MEMBER_STATE_CLEARED || that->state == ICD_MEMBER_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    result = cw_mutex_unlock(&that->lock);
    if (result == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/**** Listeners ****/

icd_status icd_member__add_listener(icd_member * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_member__remove_listener(icd_member * that, void *listener)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}

/***** Predefined behaviours *****/

/* Standard member list dump function */
icd_status icd_member__standard_dump(icd_member * that, int verbosity, int fd, void *extra)
{
    /* TBD */
    return ICD_SUCCESS;
}

/*===== Private Implementations =====*/

/* Allocates a string for the name of the membership. 
char *icd_member__build_name(icd_member *that) {
    char buf[200];

    assert(that != NULL);
    assert(that->queue != NULL);
    assert(that->caller != NULL);

    snprintf(buf, 200, "Queue %s Member %s", icd_queue__get_name(that->queue), 
            icd_caller__get_name(that->caller));
    return strdup(buf);
}
*/

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

