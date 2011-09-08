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
 * \brief icd_caller.c  -  a call to distribute
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
 * Authentication happens when icd_caller__authenticate() is called. This is
 * distinct from authorization, which uses an opaque authorization token to
 * keep the authorization data in. You can use listeners to access that
 * authorization token and veto or not veto specific actions based on the
 * interpretation of the token.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>

#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_globals.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"

/* These are temporary until we factor out icd_callweaver */
#include "callweaver/app.h"

/*===== Private types and functions =====*/

#include "callweaver/icd/icd_caller_private.h"
/*standard func ptr struc when no custom function finder (get_plugable_fn) for func ptrs icd_plugable_fn  */
static icd_plugable_fn icd_caller_plugable_fns;

static icd_module module_id = ICD_CALLER;

static int CALLER_ID_POOL = 1;

struct icd_caller_states {
    icd_caller_state oldstate;
    icd_caller_state newstate;
};

static icd_fieldset *CALLER_GROUP_REGISTRY;

static icd_status icd_caller__create_thread(icd_caller * that);
static void *icd_caller__run(void *that);
icd_status icd_caller__valid_state_change(icd_caller * that, struct icd_caller_states *states);

/* Added these missing functions in a single pass, where do they go? */

icd_status icd_caller__set_state_conference_fn(icd_caller * that, int (*state_conference_fn) (icd_event * that,
        void *extra), void *extra);
int icd_caller__ready_state_on_fail(icd_event * event, void *extra);
int icd_caller__limited_ready_state_on_fail(icd_event * event, void *extra);
int icd_caller__destroy_on_fail(icd_event * event, void *extra);
int icd_caller__pushback_and_ready_on_fail(icd_event * event, void *extra);
icd_status icd_caller__set_param_string(icd_caller * that, char *param, void *value);
icd_status icd_caller__set_param_if_null(icd_caller * that, char *param, void *value);

/* TBD make these string shorter to fit the cli skip ICD_CALLER */
char *icd_caller_state_strings[] = {
    "ICD_CALLER_STATE_CREATED", "ICD_CALLER_STATE_INITIALIZED",
    "ICD_CALLER_STATE_CLEARED", "ICD_CALLER_STATE_DESTROYED",
    "ICD_CALLER_STATE_READY", "ICD_CALLER_STATE_DISTRIBUTING",
    "ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE", "ICD_CALLER_STATE_BRIDGED",
    "ICD_CALLER_STATE_BRIDGE_FAILED", "ICD_CALLER_STATE_CHANNEL_FAILED",
    "ICD_CALLER_STATE_ASSOCIATE_FAILED", "ICD_CALLER_STATE_CALL_END",
    "ICD_CALLER_STATE_SUSPEND", "ICD_CALLER_STATE_CONFERENCED"
};

char *icd_bridge_status_strings[] = {
    "BRIDGE_UP", "BRIDGE_DOWN"
};

char *icd_thread_state_strings[] = {
    "ICD_THREAD_STATE_UNINITIALIZED", "ICD_THREAD_STATE_PAUSED",
    "ICD_THREAD_STATE_RUNNING", "ICD_THREAD_STATE_FINISHED"
};

char *cw_state_strings[] = {
    " CW_STATE_DOWN", " CW_STATE_RESERVED", " CW_STATE_OFFHOOK",
    " CW_STATE_DIALING", " CW_STATE_RING", " CW_STATE_RINGING",
    " CW_STATE_UP", " CW_STATE_BUSY", " CW_STATE_DIALING_OFFHOOK"
};

/*

icd_caller_group

The IDEA..

icd_caller_group is a struct containing a list of caller associations that can be given a name
and kept independant from the caller itself (just like associates only in a standalone obj).

icd_caller_group_list is just a way to pin and endless amount of group memberships to a caller
each caller contains 1 pointer to a icd_caller_group_list which is a linked list of groups

Reasoning...

With the groups defined, agents can belong to lots of groups and you can do things to the group as a whole
for instance if an agent is in a group of offhook agents you could use:
void icd_caller__group_chanup(icd_caller_group *group);
to command every caller in the group to go into chanup

these relationships can co-exist with regular assocations.

TODO...

 decide if it's stupid or not.

*/

struct icd_caller_group {
    icd_caller *owner;
    icd_caller_list *members;
    int allocated;
    char name[80];
};

struct icd_caller_group_list {
    icd_caller_group *group;
    icd_caller_group_list *next;
};

void icd_caller__init_group_registry()
{
    CALLER_GROUP_REGISTRY = create_icd_fieldset("CALLER_GROUP_REGISTRY");
}

void icd_caller__destroy_group_registry()
{
    destroy_icd_fieldset(&CALLER_GROUP_REGISTRY);
}

icd_caller_group *create_icd_caller_group(char *name, icd_caller * owner)
{
    icd_caller_group *group;

    group = (icd_caller_group *) ICD_STD_MALLOC(sizeof(icd_caller_group));
    if (group == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Caller Group\n");
        return NULL;
    }
    memset(group, 0, sizeof(icd_caller_group));
    group->allocated = 1;
    group->owner = owner;
    strncpy(group->name, name, sizeof(group->name));

    if (group) {
        if (!CALLER_GROUP_REGISTRY)
            icd_caller__init_group_registry();
    }
    icd_fieldset__set_value(CALLER_GROUP_REGISTRY, name, group);
    return group;
}

void destroy_icd_caller_group(icd_caller_group ** group)
{
    ICD_STD_FREE((&group));
}

icd_status icd_caller__rem_group_pointer(icd_caller * caller, icd_caller_group * group)
{
    icd_caller_group_list *ptr, *last;

    for (ptr = caller->group_list; ptr; ptr = ptr->next) {
        if (ptr->group == group) {
            if (ptr == caller->group_list) {
                free(caller->group_list);
                caller->group_list = NULL;
            } else {
                last->next = ptr->next;
                free(ptr);
                ptr = NULL;
            }
            last = ptr;
        }
    }

    return ICD_SUCCESS;
}

icd_status icd_caller__add_group_pointer(icd_caller * caller, icd_caller_group * group)
{
    icd_caller_group_list *ptr, *new_list;

    new_list = (icd_caller_group_list *) ICD_STD_MALLOC(sizeof(icd_caller_group_list));
    if (!new_list)
        return ICD_EGENERAL;

    new_list->group = group;
    new_list->next = NULL;
    for (ptr = caller->group_list; ptr && ptr->next; ptr = ptr->next) ;
    ptr->next = new_list;

    return ICD_SUCCESS;
}

icd_status icd_caller__join_group(icd_caller * caller, icd_caller_group * group)
{
    icd_config *data;

    if (!group->members) {
        data = create_icd_config(app_icd_config_registry, "list");
        group->members = create_icd_caller_list("list", data);
        destroy_icd_config(&data);
    }

    icd_caller_list__push(group->members, caller);
    icd_caller__add_group_pointer(caller, group);

    return ICD_SUCCESS;
}

icd_status icd_caller__leave_group(icd_caller * caller, icd_caller_group * group)
{
    if (group->members) {
        icd_caller_list__remove_caller_by_element(group->members, caller);
        icd_caller__rem_group_pointer(caller, group);
    }
    return ICD_SUCCESS;
}

icd_status icd_caller__join_group_by_name(icd_caller * caller, char *name)
{
    icd_caller_group *group;

    group = icd_fieldset__get_value(CALLER_GROUP_REGISTRY, name);
    if (!group)
        create_icd_caller_group(name, caller);

    return icd_caller__join_group(caller, group);

}

icd_status icd_caller__leave_group_by_name(icd_caller * caller, char *name)
{
    icd_caller_group *group;

    group = icd_fieldset__get_value(CALLER_GROUP_REGISTRY, name);
    return group ? icd_caller__leave_group(caller, group) : ICD_EGENERAL;
}

void icd_caller__group_chanup(icd_caller_group * group)
{
    icd_list_iterator *iter;
    icd_caller *caller;
    cw_channel *chan;

    assert(group != NULL);

    iter = icd_list__get_iterator((icd_list *) group->members);
    while (icd_list_iterator__has_more(iter)) {
        caller = (icd_caller *) icd_list_iterator__next(iter);
        chan = icd_caller__get_channel(caller);
        if (!chan)
            icd_caller__create_channel(caller);
        chan = icd_caller__get_channel(caller);
        if (chan)
            icd_caller__dial_channel(caller);
    }
    destroy_icd_list_iterator(&iter);
}

/*===== Public functions =====*/
/* called from app_icd.c->app_icd_load_module */
icd_status icd_caller_load_module()
{
    icd_plugable_fn *plugable_fns;
    icd_config *config;
    char buf[80];
    icd_status result;

    plugable_fns = &icd_caller_plugable_fns;
    ICD_MEMSET_ZERO(plugable_fns, sizeof(plugable_fns));
    /* create a config just to keep the api on init_icd_plugable_fns happy */
    config = create_icd_config(app_icd_config_registry, "agent_config");
    snprintf(buf, sizeof(buf), "CallerDefault");
    /*set function pointers in static struc icd_caller_plugable_fns for std icd_caller plugable functions */
    result = init_icd_plugable_fns(plugable_fns, buf, config);  /* set all func ptrs to icd_call_standard[] */
    plugable_fns->allocated = 1;

    /*set function pointers in static strucs for agents/customers */
    result = icd_agent_load_module(config);
    result = icd_customer_load_module(config);
    destroy_icd_config(&config);

    return ICD_SUCCESS;
}

/***** Init - Destroyer *****/

/* Create a caller. */
icd_caller *create_icd_caller(icd_config * data)
{
    icd_caller *caller;
    icd_status result;

    /* make a new caller from scratch */
    caller = (icd_caller *) ICD_STD_MALLOC(sizeof(icd_caller));
    if (caller == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Caller\n");
        return NULL;
    }
    caller->state = ICD_CALLER_STATE_CREATED;
    result = init_icd_caller(caller, data);
    if (result != ICD_SUCCESS) {
        ICD_STD_FREE(caller);
        return NULL;
    }
    caller->allocated = 1;
    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, caller, icd_caller__get_name(caller), module_id,
        ICD_EVENT_CREATE, NULL, caller->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_caller(&caller);
        return NULL;
    }
    return caller;
}

/* Destroy a caller, freeing its memory and cleaning up after it. */
icd_status destroy_icd_caller(icd_caller ** callp)
{
    icd_status vetoed;
    int clear_result;

    assert(callp != NULL);
    assert((*callp) != NULL);

    /* First, notify event hooks and listeners */
    vetoed =
        icd_event_factory__generate(event_factory, *callp, (*callp)->name, module_id, ICD_EVENT_DESTROY, NULL,
        (*callp)->listeners, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD Caller %s has been vetoed\n", icd_caller__get_name(*callp));
        return ICD_EVETO;
    }

    if ((clear_result = icd_caller__clear(*callp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the caller if from the heap */
    if ((*callp)->allocated) {
        (*callp)->state = ICD_CALLER_STATE_DESTROYED;
        ICD_STD_FREE(*callp);
        *callp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize a previously created caller */
icd_status init_icd_caller(icd_caller * that, icd_config * data)
{
    pthread_condattr_t condattr;
    void *key;
    char *fieldstr;
    int fieldint;
    char buf[80];
    int result;
    icd_status vetoed;

    assert(that != NULL);
    assert(data != NULL);

    /* install all the config params into a hash to look at later */
    key = icd_config__get_value(data, "params");
    if (key) {
        that->params = (void_hash_table *) key;
    }
/* TC cloner stuff use to say how many times a caller object that is remote can be called
    char *clonestr;
    clonestr = icd_config__get_value(data, "maxclones");
    if (clonestr != NULL) {
    that->maxclones = atoi(clonestr);
    } else {
    that->maxclones = 0;
    }
    clonestr = icd_config__get_value(data, "am_clone");
    if (clonestr != NULL) {
    that->am_clone = atoi(clonestr);
    } else {
    that->am_clone = 0;
    }

    that->chan = (struct cw_channel *)icd_config__get_value(data, "chan");
    that->distributor = NULL;
*/

    that->owns_channel = 0;
    that->holdannounce = NULL;
    ICD_MEMSET_ZERO(&that->holdinfo, sizeof(icd_caller_holdinfo));
    that->holdinfo.last_announce_position = 0;
    that->holdinfo.last_announce_time = 0;
    time(&that->holdinfo.chimenext);
    that->conference = NULL;
    that->bridge_technology = ICD_BRIDGE_STANDARD;
    that->conf_fd = 0;
    that->conf_mode = 0;
    that->id = CALLER_ID_POOL++;
    that->name = icd_config__get_strdup(data, "name", "");
    that->owner = NULL;
    that->authorization = NULL;
    that->require_pushback = 0;
    that->active_member = NULL;
    time(&(that->caller_created));
    time(&(that->last_mod));
    time(&(that->last_state_change));
    that->start_call = (time_t) 0;
    that->timeout = 120000;
    that->authenticated = 0;
    that->acknowledge_call = 0;
    that->acknowledged = 0;
    that->priority = 0;
    that->entertained = ICD_ENTERTAIN_NONE;
    that->role = 0;
    that->flag = 0;
    that->allocated = 0;
    that->bridge_fail_count = 0;
    that->channel_fail_count = 0;
    that->onhook = 0;

    fieldint = icd_config__get_int_value(data, "timeout", that->timeout);
    if (fieldint != that->timeout)
        fieldint *= 1000;
    if (fieldint > 2000 && fieldint < 240000) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] has a time out of %d\n", icd_caller__get_id(that),
                icd_caller__get_name(that), fieldint);
        that->timeout = fieldint;
    } else {
        cw_log(LOG_WARNING, "Caller id[%d] [%s] sanity check Invalid timeout %d\n", icd_caller__get_id(that),
            icd_caller__get_name(that), fieldint);
    }

    fieldstr = icd_config__get_value(data, "onhook");
    if (fieldstr != NULL && cw_true(fieldstr)) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] has been identified as onhook\n", icd_caller__get_id(that),
                icd_caller__get_name(that));
        that->onhook = 1;
    }
    that->dynamic = 0;
    fieldstr = icd_config__get_value(data, "dynamic");
    if (fieldstr != NULL && cw_true(fieldstr)) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] has been identified as dynamic not from icd_agents.conf\n",
                icd_caller__get_id(that), icd_caller__get_name(that));
        that->dynamic = 1;
    }
    fieldstr = icd_config__get_value(data, "acknowledge_call");
    if (fieldstr != NULL && cw_true(fieldstr)) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] has been identified as requiring acknowledgement\n",
                icd_caller__get_id(that), icd_caller__get_name(that));
        that->acknowledge_call = 1;
    }

    that->priority = icd_config__get_int_value(data, "priority", 0);

    that->chan_string = icd_config__get_strdup(data, "channel", "");
    that->caller_id = icd_config__get_strdup(data, "caller_id", "");
    that->get_plugable_fn = icd_config__get_any_value(data, "get.plugable.function", icd_caller_get_plugable_fns);

    snprintf(buf, sizeof(buf), "Plugable functions of Caller %s", icd_caller__get_name(that));
    that->plugable_fns_list = create_icd_plugable_fn_list(buf, data);

    /* optional to install a list of function ptrs strucs for each q / distributor
     * but this realy might belong conditionally in the agent /customer so they can only be
     * added conditional when its required to have an caller specific instance behavior changed
     * eg only for agent smith or customer jones etc and more typically this will prolly
     * be done in a loadable module's caller init function ...
     * as an example this standard routine will install a caller specific instance of standard
     * function ptrs for each unique dist that services any q a caller belongs to
     icd_plugable__create_standard_fns(that->plugable_fns_list, data);
     */

    that->dump_fn = icd_config__get_any_value(data, "dump", icd_caller__standard_dump);
    that->dump_fn_extra = icd_config__get_any_value(data, "dump.extra", NULL);

    snprintf(buf, sizeof(buf), "Memberships of Caller %s", icd_caller__get_name(that));
    that->memberships = create_icd_member_list(buf, data);
    snprintf(buf, sizeof(buf), "Associations of Caller %s", icd_caller__get_name(that));
    that->associations = create_icd_caller_list(buf, data);

    that->listeners = create_icd_listeners();

    cw_mutex_init(&(that->lock));
    that->thread_state = ICD_THREAD_STATE_UNINITIALIZED;
    that->using_caller_thread = 0;

    /* Create the condition that wakes up the thread running in __run() */
    result = pthread_condattr_init(&condattr);
    result = cw_cond_init(&(that->wakeup), &condattr);
    result = pthread_condattr_destroy(&condattr);

    vetoed = icd_event__generate(ICD_EVENT_INIT, NULL);
    if (vetoed == ICD_EVETO) {
        icd_caller__clear(that);
        return ICD_EVETO;
    }
    icd_caller__set_state(that, ICD_CALLER_STATE_INITIALIZED);

    return ICD_SUCCESS;
}

void icd_caller__start_waiting(icd_caller * that)
{
    assert(that != NULL);
    /* type safe wrapper we dont expose caller guts to outside world */
    that->get_plugable_fn(that)->start_waiting_fn(that);
}

void icd_caller__stop_waiting(icd_caller * that)
{
    assert(that != NULL);
    /* type safe wrapper we dont expose caller guts to outside world */
    that->get_plugable_fn(that)->stop_waiting_fn(that);
}

/* Clear the caller so that it needs to be reinitialized to be reused. */
icd_status icd_caller__clear(icd_caller * that)
{
    icd_status vetoed;

    assert(that != NULL);

    if (that->state == ICD_CALLER_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    /* destroy and end conference object if it still exists */
    if (that->conference)
        icd_conference__clear(that);
    if (that->conf_fd && (fcntl(that->conf_fd, F_GETFL) > -1))
        close(that->conf_fd);

    /* Notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_CLEAR, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_WARNING, "Clearing of ICD Caller %s has been vetoed\n", icd_caller__get_name(that));
        return ICD_EVETO;
    }

    icd_caller__set_state(that, ICD_CALLER_STATE_CLEARED);
    if (that->params && that->params->allocated) {
        /*cw_log(LOG_WARNING,"caller clear() freeing hash memory\n"); */
        vh_destroy(&(that->params));
    }

    that->id = 0;
    if (that->listeners != NULL) {
        destroy_icd_listeners(&(that->listeners));
    }
    if (that->associations != NULL) {
        /* Unlink all associations first */
        icd_caller__remove_all_associations(that);
        /* Then destroy object */
        destroy_icd_caller_list(&(that->associations));
    }
    icd_caller__set_active_member(that, NULL);
    if (that->memberships != NULL) {
        icd_caller__remove_from_all_queues(that);
        destroy_icd_member_list(&(that->memberships));
    }

    if (that->using_caller_thread) {
        pthread_cancel(that->thread);
    }
    cw_cond_destroy(&(that->wakeup));
    cw_mutex_destroy(&(that->lock));

    that->owner = NULL;
    that->authorization = NULL;
    that->chan = NULL;
    that->authenticated = 0;
    that->priority = 0;
    that->role = 0;

    if (that->chan_string != NULL) {
        ICD_STD_FREE(that->chan_string);
        that->chan_string = NULL;
    }
    if (that->caller_id != NULL) {
        ICD_STD_FREE(that->caller_id);
        that->caller_id = NULL;
    }
    if (that->plugable_fns_list != NULL) {
        icd_plugable_fn_remove_all_plugable_fns(that->plugable_fns_list);
        destroy_icd_plugable_fn_list(&(that->plugable_fns_list));
    }

    that->dump_fn = NULL;
    that->dump_fn_extra = NULL;
    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD Caller id[%d] [%s] has been cleared\n", icd_caller__get_id(that),
            icd_caller__get_name(that));

    if (that->name != NULL) {
        ICD_STD_FREE(that->name);
        that->name = NULL;
    }
    return ICD_SUCCESS;
}

/* Clear the caller (logout) so that it can login again. */
icd_status icd_caller__clear_suspend(icd_caller * that)
{
    icd_member *member;
    icd_list_iterator *iter;
    icd_queue * queue;
     
    assert(that != NULL);

    if (that->state == ICD_CALLER_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    if (that->associations != NULL) {
        /* Unlink all associations first */
        icd_caller__remove_all_associations(that);
    }
    icd_caller__set_active_member(that, NULL);
    if (that->memberships != NULL) {
//        icd_caller__remove_from_all_queues(that);
       icd_list__lock((icd_list *) (that->memberships));
       iter = icd_list__get_iterator((icd_list *) (that->memberships));
       while (icd_list_iterator__has_more_nolock(iter)) {
             member = (icd_member *) icd_list_iterator__next(iter);
	     queue = icd_member__get_queue(member);
	     if (queue)
                 icd_queue__agent_dist_quit(queue, member);		     
        }
        icd_list__unlock((icd_list *) (that->memberships));
        destroy_icd_list_iterator(&iter);
    }	

    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD Caller id[%d] [%s] has been suspend - cleared\n", icd_caller__get_id(that),
            icd_caller__get_name(that));

    return ICD_SUCCESS;

}



/***** Actions *****/

/* Add the caller to a queue. This creates a member object that is stored in the queue. */
icd_status icd_caller__add_to_queue(icd_caller * that, icd_queue * queue)
{
    icd_member *member;
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);
    assert(that->memberships != NULL);
    assert(queue != NULL);

    /* Check to see if already in queue, return EEXISTS if so */
    member = icd_member_list__get_for_queue(that->memberships, queue);
    if (member != NULL) {
        cw_log(LOG_NOTICE, "Attempted to add caller %s twice into the same queue %s\n", icd_caller__get_name(that),
            icd_queue__get_name(queue));
        return ICD_EEXISTS;
    }

    /* Check for veto. Often the addq hook will require authentication. */
    icd_run = that->get_plugable_fn(that);

    vetoed = icd_event__notify(ICD_EVENT_ADD, queue, icd_run->addq_fn, icd_run->addq_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    member = create_icd_member(queue, that, NULL);
    icd_member_list__push(that->memberships, member);
    if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
        icd_queue__agent_join(queue, member);
    } else {
        icd_queue__customer_join(queue, member);
        icd_queue__calc_holdtime(queue);

    }
    return ICD_SUCCESS;
}

/* Remove the caller from a queue and destroys the member object. */
icd_status icd_caller__remove_from_queue(icd_caller * that, icd_queue * queue)
{
    icd_member *member;
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);
    assert(that->memberships != NULL);
    assert(queue != NULL);

    /* Make sure it is actually in the queue */
    member = icd_member_list__get_for_queue(that->memberships, queue);
    if (member == NULL) {
        cw_log(LOG_WARNING, "Attempted to remove caller %s from non-member queue %s", icd_caller__get_name(that),
            icd_queue__get_name(queue));
        return ICD_ENOTFOUND;
    }

    /* Check for veto. */
    icd_run = that->get_plugable_fn(that);
    vetoed = icd_event__notify(ICD_EVENT_REMOVE, queue, icd_run->delq_fn, icd_run->delq_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
        icd_queue__agent_quit(queue, member);
    } else {
        icd_queue__customer_quit(queue, member);
    }
    icd_member_list__remove_member_by_element(that->memberships, member);
    destroy_icd_member(&member);
    icd_queue__calc_holdtime(queue);    /*%TC should be done as a listeners via icd_event__notify */
    return ICD_SUCCESS;
}

/* Lets the caller know that the queue has added it to its distributor. This
   is really just a mechanism so that the caller notify function can be called
   on entering a distributor. */
icd_status icd_caller__added_to_distributor(icd_caller * that, icd_distributor * distributor)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);

    icd_run = that->get_plugable_fn(that);
    vetoed = icd_event__notify(ICD_EVENT_ADD, distributor, icd_run->adddist_fn, icd_run->adddist_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return ICD_SUCCESS;
}

/* Lets the caller know that it has been removed from this distributor. */
icd_status icd_caller__removed_from_distributor(icd_caller * that, icd_distributor * distributor)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);

    icd_run = that->get_plugable_fn(that);
    vetoed = icd_event__notify(ICD_EVENT_REMOVE, distributor, icd_run->deldist_fn, icd_run->deldist_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return ICD_SUCCESS;
}

/* Attach a channel to the caller structure. */
icd_status icd_caller__assign_channel(icd_caller * that, cw_channel * chan)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);
    assert(chan != NULL);

    icd_run = that->get_plugable_fn(that);
    vetoed = icd_event__notify(ICD_EVENT_ASSIGN, chan, icd_run->chan_fn, icd_run->chan_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    that->chan = chan;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Allows the distributor to identify a caller to link with this caller */
icd_status icd_caller__link_to_caller(icd_caller * that, icd_caller * associate)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;
    char buf[120];

    assert(that != NULL);
    assert(associate != NULL);

    snprintf(buf, sizeof(buf), "CREATE LINK: name[%s] CallerID[%s] to name[%s] CallerID[%s]", 
    that->name, that->caller_id, associate->name, associate->caller_id);
    icd_run = that->get_plugable_fn(that);
    vetoed =
        icd_event__notify_with_message(ICD_EVENT_LINK, buf, associate, icd_run->link_fn, icd_run->link_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    icd_caller_list__push(that->associations, associate);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Unlink this caller from another */
icd_status icd_caller__unlink_from_caller(icd_caller * that, icd_caller * associate)
{
    icd_status vetoed;
    icd_status result;
    icd_status final_result;

    assert(that != NULL);
    assert(associate != NULL);

    vetoed = icd_event__generate(ICD_EVENT_UNLINK, associate);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    final_result = ICD_SUCCESS;
    /* Remove both sides of the link */
    result = icd_caller_list__remove_caller_by_element(that->associations, associate);
    if (result != ICD_SUCCESS) {
        final_result = result;
    }
    if (result == ICD_SUCCESS && associate->associations != NULL) {
        result = icd_caller_list__remove_caller_by_element(associate->associations, that);
        if (result != ICD_SUCCESS) {
            final_result = result;
        }
    }
    return final_result;
}

/* Allows the distributor to tell the caller to start bridging to the linked caller. */
icd_status icd_caller__bridge(icd_caller * that)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);

    icd_run = that->get_plugable_fn(that);
    vetoed = icd_event__notify(ICD_EVENT_BRIDGE, NULL, icd_run->bridge_fn, icd_run->bridge_fn_extra);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    icd_caller__set_state(that, ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE);

    return ICD_SUCCESS;
}

/* Tells the caller to authenticate itself using the provided authenticate_fn() */
icd_status icd_caller__authenticate(icd_caller * that, void *extra)
{
    icd_status vetoed;
    icd_plugable_fn *icd_run;

    assert(that != NULL);

    icd_run = that->get_plugable_fn(that);
    if (icd_run->auth_fn == NULL) {
        return ICD_ENOTFOUND;
    }
    if (icd_run->auth_fn(that, extra) > 0) {
        vetoed = icd_event__notify(ICD_EVENT_AUTHENTICATE, extra, icd_run->authn_fn, icd_run->authn_fn_extra);
        if (vetoed == ICD_EVETO) {
            return vetoed;
        }
        that->authenticated = 1;
        time(&(that->last_mod));
        return ICD_SUCCESS;
    }
    return ICD_EGENERAL;
}

/* Returns 1 if the caller has a given role */
int icd_caller__has_role(icd_caller * that, icd_role role)
{
    assert(that != NULL);
    return (that->role > 0 && ((that->role & role) == role)) ? 1 : 0;
}

/* Adds a role to the caller */
icd_status icd_caller__add_role(icd_caller * that, icd_role role)
{
    assert(that != NULL);

    if (icd_caller__has_role(that, role)) {
        return ICD_EEXISTS;
    }
    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->role += role;
        time(&(that->last_mod));
        icd_caller__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to add role\n",
        icd_caller__get_name(that));
    return ICD_ELOCK;
}

/* Clears a single role from the caller */
icd_status icd_caller__clear_role(icd_caller * that, icd_role role)
{
    assert(that != NULL);

    if (icd_caller__has_role(that, role)) {
        if (icd_caller__lock(that) == ICD_SUCCESS) {
            that->role -= role;
            time(&(that->last_mod));
            icd_caller__unlock(that);
            return ICD_SUCCESS;
        }
        cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to clear role\n",
            icd_caller__get_name(that));
        return ICD_ELOCK;
    }
    return ICD_ENOTFOUND;
}

/* Gets all the roles from the caller struct */
int icd_caller__get_roles(icd_caller * that)
{
    assert(that != NULL);

    return that->role;
}

/* Clears all roles from the caller */
icd_status icd_caller__clear_roles(icd_caller * that)
{
    assert(that != NULL);

    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->role = 0;
        time(&(that->last_mod));
        icd_caller__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to clear all roles\n",
        icd_caller__get_name(that));
    return ICD_ELOCK;
}

/* Returns 1 if the caller has a given flag */
int icd_caller__has_flag(icd_caller * that, icd_flag flag)
{
    assert(that != NULL);
    return (that->flag > 0 && ((that->flag & flag) == flag)) ? 1 : 0;
}

/* Adds a flag to the caller */
icd_status icd_caller__add_flag(icd_caller * that, icd_flag flag)
{
    assert(that != NULL);

    if (icd_caller__has_flag(that, flag)) {
        return ICD_EEXISTS;
    }
    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->flag += flag;
        time(&(that->last_mod));
        icd_caller__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to add flag\n",
        icd_caller__get_name(that));
    return ICD_ELOCK;
}

/* Clears a single flag from the caller */
icd_status icd_caller__clear_flag(icd_caller * that, icd_flag flag)
{
    assert(that != NULL);

    if (icd_caller__has_flag(that, flag)) {
        if (icd_caller__lock(that) == ICD_SUCCESS) {
            that->flag -= flag;
            time(&(that->last_mod));
            icd_caller__unlock(that);
            return ICD_SUCCESS;
        }
        cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to clear flag\n",
            icd_caller__get_name(that));
        return ICD_ELOCK;
    }
    return ICD_ENOTFOUND;
}

/* Gets all the flags from the caller struct */
int icd_caller__get_flags(icd_caller * that)
{
    assert(that != NULL);

    return that->flag;
}

/* Clears all flags from the caller */
icd_status icd_caller__clear_flags(icd_caller * that)
{
    assert(that != NULL);

    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->flag = 0;
        time(&(that->last_mod));
        icd_caller__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Caller %s in order to clear all flags\n",
        icd_caller__get_name(that));
    return ICD_ELOCK;
}

/* Get an existing  memberships object for this caller,  */
icd_member_list *icd_caller__get_memberships(icd_caller * that)
{
    assert(that != NULL);
    assert(that->memberships != NULL);

    return that->memberships;
}

/* Get a head from existing memberships object for this caller,  */
icd_member *icd_caller__get_memberships_peek(icd_caller * that)
{
    assert(that != NULL);
    assert(that->memberships != NULL);

    return icd_list__peek((icd_list *) that->memberships);
}

/* Get an existing  member object for a given queue for this caller, NULL if not a member */
icd_member *icd_caller__get_member_for_queue(icd_caller * that, icd_queue * queue)
{
    assert(that != NULL);
    assert(that->memberships != NULL);
    assert(queue != NULL);

    return icd_member_list__get_for_queue(that->memberships, queue);
}

/* Get an existing  member object for a given distributor for this caller, NULL if not a member */
icd_member *icd_caller__get_member_for_distributor(icd_caller * that, icd_distributor * dist)
{
    assert(that != NULL);
    assert(that->memberships != NULL);
    assert(dist != NULL);

    return icd_member_list__get_for_distributor(that->memberships, dist);
}

/* Return the number of memberships for this caller */
int icd_caller__get_member_count(icd_caller * that)
{
    assert(that != NULL);
    assert(that->memberships != NULL);

    return icd_list__count((icd_list *) (that->memberships));
}

/* Prints the contents of the caller structure to the given file descriptor. */
icd_status icd_caller__dump(icd_caller * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(that->dump_fn != NULL);

    return that->dump_fn(that, verbosity, fd, that->dump_fn_extra);
}

/* Start the caller thread. */
icd_status icd_caller__start_caller_response(icd_caller * that)
{
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_RUNNING;
    /* either we verbose here or icd_caller__create_thread not both */
    if (icd_verbose > 4)
        cw_verbose(VERBOSE_PREFIX_1 "Started thread for caller id[%d] [%s]\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    return ICD_SUCCESS;
}

/* Pause the caller thread. */
icd_status icd_caller__pause_caller_response(icd_caller * that)
{
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_PAUSED;
    return ICD_SUCCESS;
}

/* Stop and finish the caller thread. */
icd_status icd_caller__stop_caller_response(icd_caller * that)
{
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_FINISHED;

    return ICD_SUCCESS;
}

/***** Getters and Setters *****/

/* Set the ID for that caller call */
icd_status icd_caller__set_id(icd_caller * that, int id)
{
    assert(that != NULL);

    that->id = id;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the ID for that caller call */
int icd_caller__get_id(icd_caller * that)
{
    assert(that != NULL);

    return that->id;
}

/* Set the name string to identify this caller. */
icd_status icd_caller__set_name(icd_caller * that, char *name)
{
    assert(that != NULL);

    if (that->name != NULL) {
        ICD_STD_FREE(that->name);
    }
    that->name = strdup(name);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the name string being used to identify this caller. */
char *icd_caller__get_name(icd_caller * that)
{
    assert(that != NULL);

    if (that->name == NULL) {
        return "no-name";
    }
    return that->name;
}

/* set bridge technology */
icd_status icd_caller__set_bridge_technology(icd_caller * that, icd_bridge_technology tech)
{
    assert(that != NULL);
    that->bridge_technology = tech;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* get bridge technology */
icd_bridge_technology icd_caller__get_bridge_technology(icd_caller * that)
{
    assert(that != NULL);
    return that->bridge_technology;
}

/* get ptr to a conference  */
icd_conference *icd_caller__get_conference(icd_caller * that)
{
    assert(that != NULL);
    return that->conference;
}

/* Set the music on hold for that caller call */
icd_status icd_caller__set_moh(icd_caller * that, char *moh)
{
    assert(that != NULL);
    icd_caller__set_param(that, "moh", moh);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the music on hold for that  caller call */
char *icd_caller__get_moh(icd_caller * that)
{
    assert(that != NULL);
    return (char *) icd_caller__get_param(that, "moh");
}

/* Set the conf_mode  for that caller call */
icd_status icd_caller__set_conf_mode(icd_caller * that, int conf_mode)
{
    assert(that != NULL);
    that->conf_mode = conf_mode;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the conf_mode for that  caller call */
int icd_caller__get_conf_mode(icd_caller * that)
{
    assert(that != NULL);
    return that->conf_mode;
}

/* Set the announce for that caller call */
icd_status icd_caller__set_announce(icd_caller * that, char *announce)
{
    assert(that != NULL);

    icd_caller__set_param(that, "announce", announce);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the announce for that caller call */
char *icd_caller__get_announce(icd_caller * that)
{
    assert(that != NULL);

    return (char *) icd_caller__get_param(that, "announce");
}

/* Set the channel string for that caller call */
icd_status icd_caller__set_channel_string(icd_caller * that, char *channel)
{
    if (that->chan_string != NULL) {
        ICD_STD_FREE(that->chan_string);
    }
    that->chan_string = strdup(channel);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the channel string for that caller call */
char *icd_caller__get_channel_string(icd_caller * that)
{
    assert(that != NULL);

    if (that->chan_string == NULL) {
        return "-";
    }
    return that->chan_string;
}

/* Set the caller id for that caller call */
icd_status icd_caller__set_caller_id(icd_caller * that, char *caller_id)
{
    if (that->caller_id != NULL) {
        ICD_STD_FREE(that->caller_id);
    }
    that->caller_id = strdup(caller_id);
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the caller id for that caller call */
char *icd_caller__get_caller_id(icd_caller * that)
{
    assert(that != NULL);

    if (that->caller_id == NULL) {
        return "";
    }
    return that->caller_id;
}

/* Get the caller position for in a given member (ie Distributor) */
int icd_caller__get_position(icd_caller * that, icd_member * member)
{
    icd_distributor *distributor;
    int count = 0;

    assert(that != NULL);
    assert(member != NULL);

    distributor = icd_member__get_distributor(member);
    /*position is base 0 so offset 1 */
    if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
        count = icd_distributor__customer_position(distributor, (icd_customer *) that);
    } else {
        count = icd_distributor__agent_position(distributor, (icd_agent *) that);
    }
    if (count != -1)            /*could not find in distributor's agent/cust list */
        count++;

    return count;
}

/* Get the number of pending callers for a given member (ie Distributor) */
int icd_caller__get_pending(icd_caller * that, icd_member * member)
{
    icd_distributor *distributor;

    assert(that != NULL);
    assert(member != NULL);

    distributor = icd_member__get_distributor(member);
    if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
        return icd_distributor__customers_pending(distributor);
    } else {
        return icd_distributor__agents_pending(distributor);
    }

}

/* Set the call count caller. */
icd_status icd_caller__set_callcount(icd_caller * that, int callcount)
{
    assert(that != NULL);
    if (callcount == -1)
        that->callcount++;
    else
        that->callcount = callcount;

    return ICD_SUCCESS;
}

/* Get the callcount. */
int icd_caller__get_callcount(icd_caller * that)
{
    assert(that != NULL);
    return that->callcount;
}

/* Set the state for that caller call */
icd_status icd_caller__set_state(icd_caller * that, icd_caller_state state)
{
    struct icd_caller_states states;
    icd_plugable_fn *icd_run;
    char buf[120];
    icd_status vetoed;

    assert(that != NULL);

    if (icd_caller__lock(that) == ICD_SUCCESS) {
    
    states.oldstate = icd_caller__get_state(that);
    states.newstate = state;

    /* just changing to the same state so, um yeah it worked!... */
    if (states.oldstate == states.newstate) {
        icd_caller__unlock(that);
/* This state error code is important to let know other distributors not to use this caller. An error code should be probably is case of other states as well */	
	if (state == ICD_CALLER_STATE_DISTRIBUTING) {
		return ICD_ESTATE;
	}
	else { 
        	return ICD_SUCCESS;
	}
    }
    

    if (icd_caller__valid_state_change(that, &states) != ICD_SUCCESS) {
        icd_caller__unlock(that);
        return ICD_ESTATE;
    }

    snprintf(buf, sizeof(buf), "[%s]->[%s]", icd_caller_state_strings[that->state],
        icd_caller_state_strings[state]);
    icd_run = that->get_plugable_fn(that);
    vetoed =
        icd_event__notify_with_message(ICD_EVENT_STATECHANGE, buf, &states, icd_run->state_fn,
        icd_run->state_fn_extra);
    if (vetoed == ICD_EVETO) {
        icd_caller__unlock(that);
        return ICD_EVETO;
    }

        that->state = state;
        time(&that->last_mod);
        time(&that->last_state_change);
        cw_cond_signal(&(that->wakeup));
        icd_caller__unlock(that);

        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on Caller id[%d] [%s] in order to set state\n",
        icd_caller__get_id(that), icd_caller__get_name(that));
    return ICD_ELOCK;
}

/* Get the state for that caller call */
icd_caller_state icd_caller__get_state(icd_caller * that)
{
    assert(that != NULL);

    return that->state;
}

char *icd_caller__get_state_string(icd_caller * that)
{
    assert(that != NULL);

    return icd_caller_state_strings[that->state];
}

/* Set the owner for that caller call */
icd_status icd_caller__set_owner(icd_caller * that, void *owner)
{
    assert(that != NULL);

    that->owner = owner;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the owner for that caller call */
void *icd_caller__get_owner(icd_caller * that)
{
    assert(that != NULL);

    return that->owner;
}

/* Set the channel for that caller call */
icd_status icd_caller__set_channel(icd_caller * that, cw_channel * chan)
{
    assert(that != NULL);

    that->chan = chan;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the channel for that caller call */
cw_channel *icd_caller__get_channel(icd_caller * that)
{
    assert(that != NULL);

    return that->chan ? that->chan : NULL;
}

/* Set the timeout in milliseconds for that caller call */
icd_status icd_caller__set_timeout(icd_caller * that, int timeout)
{
    assert(that != NULL);

    that->timeout = timeout;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the entertained flag 1-moh, 2-ringing */
icd_entertain icd_caller__get_entertained(icd_caller * that)
{
    assert(that != NULL);

    return that->entertained;
}

/* Get the timeout in milliseconds for that caller call */
int icd_caller__get_timeout(icd_caller * that)
{
    assert(that != NULL);

    return that->timeout;
}

/* Set the time that the caller call started */
icd_status icd_caller__set_start(icd_caller * that, time_t start)
{
    assert(that != NULL);

    that->start_call = start;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Set the time that the caller call started to now */
icd_status icd_caller__set_start_now(icd_caller * that)
{
    assert(that != NULL);

    time(&(that->start_call));
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the time that the caller call started */
time_t icd_caller__get_start(icd_caller * that)
{
    assert(that != NULL);

    return that->start_call;
}

/* Set the time that the caller call was last modified */
icd_status icd_caller__set_last_mod(icd_caller * that, time_t mod)
{
    assert(that != NULL);

    that->last_mod = mod;
    return ICD_SUCCESS;
}

/* Get the time that the caller call was last modified */
time_t icd_caller__get_last_mod(icd_caller * that)
{
    assert(that != NULL);

    return that->last_mod;
}

/* Set the authorization for that caller */
icd_status icd_caller__set_authorization(icd_caller * that, void *auth)
{
    assert(that != NULL);

    that->authorization = auth;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the authorization for that caller */
void *icd_caller__get_authorization(icd_caller * that)
{
    assert(that != NULL);

    return that->authorization;
}

/* Get the priority for the caller */
icd_status icd_caller__set_priority(icd_caller * that, int pri)
{
    assert(that != NULL);

    that->priority = pri;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the priority for the caller */
int icd_caller__get_priority(icd_caller * that)
{
    assert(that != NULL);

    return that->priority;
}

/* Set the time of the last time the caller's call changed state */
icd_status icd_caller__set_last_state_change(icd_caller * that, time_t statetime)
{
    assert(that != NULL);

    that->last_state_change = statetime;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get the time of the last time the caller's call changed state */
time_t icd_caller__get_last_state_change(icd_caller * that)
{
    assert(that != NULL);

    return that->last_state_change;
}

/* Set pushback flag */
icd_status icd_caller__set_pushback(icd_caller * that)
{
    assert(that != NULL);

    that->require_pushback = 1;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Reset pushback flag */
icd_status icd_caller__reset_pushback(icd_caller * that)
{
    assert(that != NULL);

    that->require_pushback = 0;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get pushback flag */
int icd_caller__get_pushback(icd_caller * that)
{
    assert(that != NULL);

    return that->require_pushback;
}

/* Set on-hook flag */
icd_status icd_caller__set_onhook(icd_caller * that, int onhook)
{
    assert(that != NULL);

    that->onhook = onhook;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get on-hook flag */
int icd_caller__get_onhook(icd_caller * that)
{
    assert(that != NULL);

    return that->onhook;
}

/* Set dynamic flag */
icd_status icd_caller__set_dynamic(icd_caller * that, int dynamic)
{
    assert(that != NULL);

    that->dynamic = dynamic;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get dynamic flag */
int icd_caller__get_dynamic(icd_caller * that)
{
    assert(that != NULL);

    return that->dynamic;
}

/* Get that->using_caller_thread flag */
int icd_caller__get_using_caller_thread(icd_caller * that)
{
    assert(that != NULL);

    return that->using_caller_thread;
}

/* Get that->using_caller_thread flag */
int icd_caller__owns_channel(icd_caller * that)
{
    assert(that != NULL);

    return that->owns_channel;
}

/* Get that->acknowledge_call flag */
int icd_caller__get_acknowledge_call(icd_caller * that)
{
    assert(that != NULL);

    return that->acknowledge_call;
}

/* Get that->identifier */
/*
int icd_caller__get_identifier(icd_caller * that)
{
    assert(that != NULL);

    return that->identifier;
}
*/
/* Get last_announce_position */
int icd_caller__get_last_announce_position(icd_caller * that)
{
    assert(that != NULL);

    return that->holdinfo.last_announce_position;
}
int icd_caller__set_last_announce_position(icd_caller * that, int position)
{
    assert(that != NULL);

    that->holdinfo.last_announce_position = position;
    time(&(that->last_mod));
    return ICD_SUCCESS;
}

/* Get last_announce_time */
time_t icd_caller__get_last_announce_time(icd_caller * that)
{
    assert(that != NULL);

    return that->holdinfo.last_announce_time;
}

int icd_caller__set_last_announce_time(icd_caller * that)
{
    assert(that != NULL);

    time(&(that->holdinfo.last_announce_time));
    return ICD_SUCCESS;
}

void icd_caller__set_holdannounce(icd_caller * that, icd_queue_holdannounce * ha)
{
    that->holdannounce = ha;
}

/* Set active member */
icd_status icd_caller__set_active_member(icd_caller * that, icd_member * active_member)
{
    assert(that != NULL);

    that->active_member = active_member;
    time(&(that->last_mod));

    return ICD_SUCCESS;
}

/* Get active member aka the current distributor */
icd_member *icd_caller__get_active_member(icd_caller * that)
{
    assert(that != NULL);

    return that->active_member;
}

/* Get the List of plugable functions */
icd_plugable_fn_list *icd_caller__get_plugable_fns_list(icd_caller * that)
{
    assert(that != NULL);

    return that->plugable_fns_list;
}

/* Get the List of associations this caller has */
icd_caller_list *icd_caller__get_associations(icd_caller * that)
{
    assert(that != NULL);

    return that->associations;

}

/* Get the List of listeners this caller has */
icd_listeners *icd_caller__get_listeners(icd_caller * that)
{
    assert(that != NULL);

    return that->listeners;

}

/***** Callback Setters *****/
/* The dump function is a virtual function. You set the function to execute here. */
icd_status icd_caller__set_dump_fn(icd_caller * that, icd_status(*dump_fn) (icd_caller *, int verbosity, int fd,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->dump_fn = dump_fn;
    that->dump_fn_extra = extra;
    return ICD_SUCCESS;
}

/***** Locking *****/

/* Lock the entire caller */
icd_status icd_caller__lock(icd_caller * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_CALLER_STATE_CLEARED || that->state == ICD_CALLER_STATE_DESTROYED) {
        cw_log(LOG_WARNING, "Caller id[%d] [%s] Lock failed due to state cleared or destroyed (%d)\n",
            icd_caller__get_id(that), icd_caller__get_name(that), that->state);
        return ICD_ERESOURCE;
    }
   if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] Try to Lock\n", icd_caller__get_id(that),
                icd_caller__get_name(that));
    retval = cw_mutex_lock(&that->lock);

    if (retval == 0) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] Lock for succeeded\n", icd_caller__get_id(that),
                icd_caller__get_name(that));
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Caller id[%d] [%s] Lock failed code %d\n", icd_caller__get_id(that),
        icd_caller__get_name(that), retval);
    return ICD_ELOCK;
}

/* Unlock the entire caller */
icd_status icd_caller__unlock(icd_caller * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_CALLER_STATE_DESTROYED) {
        cw_log(LOG_WARNING, "Caller id[%d] [%s] Unlock failed due to state destroyed (%d)\n",
            icd_caller__get_id(that), icd_caller__get_name(that), that->state);
        return ICD_ERESOURCE;
    }
   if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] Try to UnLock\n", icd_caller__get_id(that),
                icd_caller__get_name(that));
    retval = cw_mutex_unlock(&that->lock);
    if (retval == 0) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] UnLock for succeeded\n", icd_caller__get_id(that),
                icd_caller__get_name(that));
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, " Caller id[%d] [%s] UnLock failed code %d\n", icd_caller__get_id(that),
        icd_caller__get_name(that), retval);
    return ICD_ELOCK;
}

/**** Listeners ****/

icd_status icd_caller__add_listener(icd_caller * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_caller__remove_listener(icd_caller * that, void *listener)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}

/**** Predefined Pluggable Actions ****/

/* Require authentication on add to queue by making this the ADD event notify
   function. Note that the "extra" field holds the token needed to authenticate
   against, either the text password or hash or SSL certificate or something
   else. */
int icd_caller__require_authentication(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_plugable_fn *icd_run;
    int authenticated;
    int vetoed;

    assert(event != NULL);

    authenticated = 0;
    that = (icd_caller *) icd_event__get_source(event);
    if (that == NULL || that->authenticated != 0) {
        /* Nothing to be done. */
        return 0;
    }
    authenticated = icd_caller__authenticate(that, extra);
    if (authenticated) {
        icd_run = that->get_plugable_fn(that);
        vetoed = icd_event__notify(ICD_EVENT_AUTHENTICATE, extra, icd_run->authn_fn, icd_run->authn_fn_extra);
        if (vetoed == 0) {
            that->authenticated = 1;
            return 0;
        }
    }
    /* Veto "add-to-queue" if authenticate failed or veto for authenticating. */
    return 1;
}

/* Always returns true without doing anything */
int icd_caller__authenticate_always_succeeds(icd_caller * caller, void *authenticate_token)
{
    return 1;
}

/* Authenticate by requiring password to be typed into phone. Return 1 on successful
   login, 0 otherwise. */
int icd_caller__authenticate_by_keypad(icd_caller * caller, void *authenticate_token)
{
    char *password;
    char entry_buf[20];
    int cw_result;

    assert(caller != NULL);
    assert(caller->chan != NULL);
    assert(authenticate_token != NULL);

    password = (char *) authenticate_token;
    memset(entry_buf, 0, sizeof(entry_buf));
    cw_result = cw_app_getdata(caller->chan, "agent-pass", entry_buf, sizeof(entry_buf) - 1, 0);
    if (cw_result == 0) {
        if (strcmp(password, entry_buf) == 0) {
        }
    }
    return 0;
}

/* When authenticated, this plays a little "Agent Logged In" message on the
   channel. Note that if the channel isn't up at this point, it is brought
   up. */
int icd_caller__play_logged_in_message(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_status result;
    int cw_result;

    assert(event != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    if (that->chan == NULL) {
        return 0;
    }
    /* This just makes sure the channel is up */
    result = icd_caller__dial_channel(that);
    if (result != ICD_SUCCESS) {
        /* What can we do? The channel didn't come up for some reason. */
        return 0;
    }
    cw_result = cw_streamfile(that->chan, "agent-loginok", that->chan->language);
    if (cw_result == 0) {
        cw_result = cw_waitstream(that->chan,  CW_DIGIT_ANY);
    }
    return 0;
}

/* Bridge-failed Actions */

/* Set state back to READY */
int icd_caller__ready_state_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;

    assert(event != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    icd_caller__set_state(that, ICD_CALLER_STATE_READY);

    /* No veto allowed here */
    return 0;
}

/* Set state to READY if bridge failed < 5 times */
int icd_caller__limited_ready_state_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;
    int *value;

    assert(event != NULL);
    //assert(extra != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    if (extra != NULL)
        value = (int *) extra;

    /*
       TBD - Set up cleanup thread and add to list it keeps It will call icd_caller__clear()
       (*value)++;
       if ((*value) < 5) {
       icd_caller__set_state(that, ICD_CALLER_STATE_READY);
       } else {
       }
       on channel state failed, as of now,
       simply call the standard cleanup caller which will put the agent back to the q
       and customer is hung up with his channel torn down.
       More complex cases are yet to be dealt with
     */
    icd_caller__set_pushback(that);
    that->get_plugable_fn(that)->cleanup_caller_fn(that);       /*standard is in icd_agent/customer.c */
    /* No veto allowed here */
    return 0;
}

/* Destroy caller - Note that we need a cleanup thread for this one */
int icd_caller__destroy_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;

    assert(event != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    /* TBD - Set up cleanup thread and add to list it keeps.
       It will call icd_caller__clear() */

    /* No veto allowed here */
    return 0;
}

/* Set pushed_back attribute and then set state to READY */
int icd_caller__pushback_and_ready_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;
    int *value = 0;

    assert(event != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    /* increment the count of the number of failures */
    if (extra != NULL) {
        value = (int *) extra;
        (*value)++;
    }
    icd_caller__set_pushback(that);
    icd_caller__set_state(that, ICD_CALLER_STATE_READY);

    /* No veto allowed here */
    return 0;
}

/* Dummy notify function never vetos anything. */
int icd_caller__dummy_notify_hook(icd_event * event, void *extra)
{
    /*icd_caller *that; */

    assert(event != NULL);
    /*that = (icd_caller *)icd_event__get_source(event); */
    return 0;
}

int icd_caller__delq_notify_hook(icd_event * event, void *extra)
{
    return 0;
}

int icd_caller__deldist_notify_hook(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_status result;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);

    cw_log(LOG_WARNING, "Caller %d [%s] is hung up. \n", icd_caller__get_id(that), icd_caller__get_name(that));

    if (that == NULL || icd_caller__get_state(that) == ICD_CALLER_STATE_CALL_END) {
        /* Nothing to be done. */
        return 0;
    }

    /* If the caller is in an initialized state like ICD_CALLER_STATE_READY or
       ICD_CALLER_STATE_DISTRIBUTING cleaning up is to be done?
     */

    if (icd_caller__get_state(that) == ICD_CALLER_STATE_BRIDGED) {

        result = icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
        if (result != ICD_SUCCESS) {
            /* Clean up invalid state change */
        }
    }
    return 0;
}

/* Standard function for reacting to the ready state */
/*!
 * Required state in order to be available for distributor to link. If not in
 * this state, distributor discards member when popped.
 */
int icd_caller__standard_state_ready(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_member *member;
    icd_list_iterator *iter;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);

    /* clear all association b4 push onto Distributor since dist will set associations */
    icd_caller__remove_all_associations(that);
    /* lock to avoid bad interraction with async remove from queue command */ 
    icd_list__lock((icd_list *) (that->memberships));
    iter = icd_list__get_iterator((icd_list *) (that->memberships));
    while (icd_list_iterator__has_more_nolock(iter)) {
        member = (icd_member *) icd_list_iterator__next(iter);
        if (that->require_pushback && (member == that->active_member)) {
            if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
                icd_queue__customer_pushback(icd_member__get_queue(member), member);
            } else {
                icd_queue__agent_pushback(icd_member__get_queue(member), member);
/*                icd_queue__agent_distribute(icd_member__get_queue(member), member);*/
            }
        } else {
            if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
                icd_queue__customer_distribute(icd_member__get_queue(member), member);
            } else {
                icd_queue__agent_distribute(icd_member__get_queue(member), member);
            }
        }
    }
    that->require_pushback = 0;
    destroy_icd_list_iterator(&iter);
    icd_list__unlock((icd_list *) (that->memberships));

    /* You play hold music and listen on channel for offhook only */
    if (icd_caller__get_onhook(that)) {
        if (icd_verbose > 4)
            cw_log(LOG_NOTICE, "Caller id[%d] [%s] is an onhook agent so hangs up immediately.  \n",
                icd_caller__get_id(that), icd_caller__get_name(that));
        return 0;
    }

    if (!icd_bridge__check_hangup(that)) {
        if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
            icd_bridge__wait_call_customer(that);
        } else if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
            icd_bridge__wait_call_agent(that);
        } else {
            cw_log(LOG_WARNING, "Invalid role not a customer or an agent...\n");
            icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
            return -1;          /* does anybody care what I return? */
        }
    } else {
        icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
    }

    return 0;
}

/* Standard function for reacting to the distributing state, just mark caller so no dist can take him */
int icd_caller__standard_state_distribute(icd_event * event, void *extra)
{

    assert(event != NULL);

    return 0;
}

/* Standard function for reacting to the get channels and bridge state */
int icd_caller__standard_state_get_channels(icd_event * event, void *extra)
{
    icd_status result;
    icd_caller *that;
    icd_caller *associate;
    icd_list_iterator *iter;
    icd_config *config;
    icd_list *live_associations;
    icd_conference *conf;
    int link_count = 0;
    icd_bridge_technology bridge_tech;
    char conf_name[10];
    icd_queue *queue = NULL;
    icd_member *member;
    icd_caller *caller;
    int res, it;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);
    
    icd_list__lock((icd_list *) (that->memberships));
    member = icd_caller__get_active_member(that);
    /*check if mamber is valid   */    
    if(member){
       if(icd_member_list__member_position(that->memberships, member) >= 0){
       		queue = icd_member__get_queue(member);
       }
    }
    if(!queue){
       if (icd_caller__has_role(that, ICD_BRIDGER_ROLE)) {
            icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_READY);
       }
       icd_caller__set_state(that, ICD_CALLER_STATE_READY);
	   icd_list__unlock((icd_list *) (that->memberships));
       return 1;
    }
    icd_list__unlock((icd_list *) (that->memberships));
    icd_queue__calc_holdtime(queue);

    if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)){
	     icd_queue__check_recording(queue, caller);
     }

    
    /* caller has BRIDGER ROLE */
    if (!icd_caller__has_role(that, ICD_BRIDGER_ROLE)) {   
       /* caller has BRIDGEE ROLE usually (always?) customer*/
       /* Let the bridger know that bredgee prompts are finished */ 
       icd_caller__add_flag(that, ICD_ACK_EXTERN_FLAG);
       return 0;
    }
    result = ICD_SUCCESS;
    /* Distributor sets this state on BRIDGER */
    
        /* %TC set state on all bridgees to get chan realy to keep our FSM happy
         * do we realy just have the Dist set bridgees to this state ??
         */

        /* Try to create a channel and bring it up if not present */
        icd_caller__set_active_member(that, NULL);
        if (that->chan == NULL) {
            icd_caller__create_channel(that);
        }

        /* A channel was created in the previous step or was already present */
        if (that->chan) {
            /* update the usage count of channels */
            cw_update_use_count();
            /* Bring 'up' the channel if it is not already 'up' */
            result = icd_caller__dial_channel(that);
            if (icd_caller__has_role(that, ICD_AGENT_ROLE) && !icd_caller__get_onhook(that) &&
	        icd_caller__get_acknowledge_call(that) && result == ICD_SUCCESS){
//            	if(that->entertained) 
//                      icd_caller__stop_waiting(that);
			/* Check if any customer has an NO_ACK -param. In that case now akcnowledgment waiting is needed */ 		       
 	                icd_list__lock((icd_list *) (that->associations));
        		iter = icd_list__get_iterator((icd_list *) (that->associations));
        		int ack_wait = 1;
        		char *no_ack;
        		while (icd_list_iterator__has_more_nolock(iter)) {
            		associate = (icd_caller *) icd_list_iterator__next(iter);
    				no_ack = icd_caller__get_param(associate, "no_ack");
    				if (no_ack != NULL)
    					if(cw_true(no_ack)) {
    						ack_wait = 0;
    						break;
    				}
        		}
        		destroy_icd_list_iterator(&iter);
 	                icd_list__unlock((icd_list *)(that->associations));
				if(ack_wait){			
		    		res = icd_bridge_wait_ack(that);  
		    		if (res)
           	  			result = ICD_EGENERAL;
				}
           	} 	
        }

        /* We have either failed to create a channel or bring it up. */
        if (!that->chan || (result != ICD_SUCCESS)) {
            icd_caller__set_state(that, ICD_CALLER_STATE_CHANNEL_FAILED);
            icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_ASSOCIATE_FAILED);
            return 0;
        }

        config = create_icd_config(app_icd_config_registry, "caller");
        live_associations = create_icd_list(config);
        destroy_icd_config(&config);
        icd_list__lock((icd_list *) (that->associations));
        iter = icd_list__get_iterator((icd_list *) (that->associations));

        while (icd_list_iterator__has_more_nolock(iter)) {

            associate = (icd_caller *) icd_list_iterator__next(iter);
                cw_log(LOG_WARNING, "\nCallers %d and %d are about to be bridged\n", icd_caller__get_id(that),
                                            icd_caller__get_id(associate));

            /* Try to create a channel if not present. */
            if (associate->chan == NULL)
                icd_caller__create_channel(associate);

            /* A channel was created in the previous step or was already present. */
            if (associate->chan) {

                /* Update the usage count of channels. */
                cw_update_use_count();
                /* Bring 'up' the channel if it is not already 'up'. */
                result = icd_caller__dial_channel(associate);
            }

            /* We have either failed to create a channel or bring it up. */
            if (associate->chan && (result == ICD_SUCCESS)) {
                if(icd_caller__set_state(associate, ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE) == ICD_SUCCESS){
                	icd_caller__clear_flag(associate, ICD_ACK_EXTERN_FLAG);
                	icd_list__push(live_associations, associate);
		}
            }
        }

        destroy_icd_list_iterator(&iter);
        icd_list__unlock((icd_list *) (that->associations));

        /* Atleast one association is up. */
        link_count = icd_list__count(live_associations);

        if (link_count <= 0) {
            	cw_log(LOG_WARNING, "No associations are found, setting our state to channel failed\n");
            	icd_caller__set_state(that, ICD_CALLER_STATE_CHANNEL_FAILED);
        	destroy_icd_list(&live_associations);
    		return 0;
	}
            bridge_tech = (link_count > 1 || (that->bridge_technology == ICD_BRIDGE_CONFERENCE)
                || icd_conference__get_global_usage())? ICD_BRIDGE_CONFERENCE : ICD_BRIDGE_STANDARD;

            /* Bridge only the first element of the associations */
	    /*to make sure that associate structure is valid (CUSTOMER hangup case ) */
            icd_list__lock((icd_list *) (that->associations));
            associate = (icd_caller *) icd_list__peek((icd_list *) live_associations);
            if (icd_verbose > 4) {
	            if(icd_caller_list__caller_position(that->associations, associate) >=0){ 
                	cw_log(LOG_NOTICE, "%s=%s -> %s=%s\n\n", that->chan->name, cw_state_strings[that->chan->_state],
                    associate->chan->name, cw_state_strings[associate->chan->_state]);
            	}
	    }

/* We want for customer to wait until prompt (if any) is finished. Change of customer state causes end of waiting loop after prompt is finished.  */	    
            res=0;
	    for(it=0; it<=100; it++){
        if(icd_caller_list__caller_position(that->associations, associate) <0){ 
			break; 
		}    
		if((icd_caller__get_state(associate) != ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE) 
		   || icd_caller__has_flag(associate, ICD_ACK_EXTERN_FLAG)){
		    break;
		}    
	        res = cw_waitfordigit(that->chan, 200);
                if (res < 0) {
                    break;
                }
            }
            if(res <0){
                cw_log(LOG_WARNING, "Channel of bridger [%s] failed while waiting for bridgee[%s]\n", icd_caller__get_name(that), icd_caller__get_name(associate));
                icd_caller__set_state(that, ICD_CALLER_STATE_CHANNEL_FAILED);
                icd_list__unlock((icd_list *) (that->associations));
                icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_ASSOCIATE_FAILED);
                destroy_icd_list(&live_associations); 
		return 1;
	    }	
            if(it>100){
                cw_log(LOG_WARNING, "Bridger [%s] waited too long for bridgee [%s] prompt to be finished\n", icd_caller__get_name(that), icd_caller__get_name(associate));
                icd_caller__set_state(that, ICD_CALLER_STATE_CHANNEL_FAILED);
                icd_list__unlock((icd_list *) (that->associations));
                icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_ASSOCIATE_FAILED);
                destroy_icd_list(&live_associations); 
		return 1;
	    }	
            if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
                 icd_caller__stop_waiting(that);
                 if (!icd_caller__get_onhook(that)) {
                     icd_caller__play_sound_file(that, "beep");
                }
            }
            /* In case of hangup of associate   */       
	            if((icd_caller_list__caller_position(that->associations, associate) <0) ||
		       (icd_caller__get_state(associate) != ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE)) { 
	                icd_caller__set_state(that, ICD_CALLER_STATE_CHANNEL_FAILED); 
                	icd_list__unlock((icd_list *) (that->associations));
                	icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_ASSOCIATE_FAILED); 
	                icd_caller__start_waiting(that); 
	                destroy_icd_list(&live_associations); 
	                return 1;    
	            } 
            icd_caller__stop_waiting(associate); 
	    
            result = ICD_SUCCESS;
            if (bridge_tech == ICD_BRIDGE_CONFERENCE) {
                if (that->conference)
                    conf = that->conference;
                else {
                    conf = icd_conference__new("bridge");
                    if (conf) {
                        sprintf(conf_name, "%d", conf->ztc.confno);
			icd_conference__register(conf_name, conf);
                    }
                }
                if (conf) {
                    cw_log(LOG_NOTICE, "Conference Located....%d\n", conf->ztc.confno);
                    icd_conference__associate(that, conf, 1);
                    icd_conference__associate(associate, conf, 0);
                    icd_caller__set_state(that, ICD_CALLER_STATE_CONFERENCED);
                    icd_caller__set_state(associate, ICD_CALLER_STATE_CONFERENCED);
                    icd_list__unlock((icd_list *) (that->associations));
                    destroy_icd_list(&live_associations);
                    return 0;
                }
            }

            if (link_count == 1)
                result = icd_bridge_call(that, associate);

            if (result != ICD_SUCCESS) {
                /* TBD - Cleanup from invalid state change */
            }

        icd_list__unlock((icd_list *) (that->associations));
        destroy_icd_list(&live_associations);
    	return 0;
}

/* Standard function for reacting to the bridged state */
int icd_caller__standard_state_bridged(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_caller *associate;
    icd_list_iterator *iter;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

    if (icd_caller__has_role(that, ICD_BRIDGEE_ROLE)) {
        iter = icd_list__get_iterator((icd_list *) (that->associations));
        while (icd_list_iterator__has_more(iter)) {

            associate = (icd_caller *) icd_list_iterator__next(iter);
            /* dont tell your bridger they already know */
            if (icd_caller__get_state(associate) != ICD_CALLER_STATE_BRIDGED) {
                /* hangup any OnHook callers stop em ringing */
                if (!icd_caller__get_onhook(associate)) /* TC lock the list */
                    icd_bridge__safe_hangup(associate);
                icd_caller__set_state(associate, ICD_CALLER_STATE_BRIDGE_FAILED);
                /*
                   Perhaps this is state change on assoc. to BRIDGE_FAILED?
                   typical case here is ring all and now this customer is bridged
                   so we need to tell all other bridgers some one bridged us so tell
                   all other bridger to stop trying,
                   BRUCE TBD - notify associate you are now bridged
                 */
            }
        }
        destroy_icd_list_iterator(&iter);
    }
    icd_caller__set_start_now(that);
    that->callcount++;
    return 0;
}

/* Standard function for reacting to the call end state */
int icd_caller__standard_state_call_end(icd_event * event, void *extra)
{
    icd_caller *that = NULL;
    int ret;

    assert(event != NULL);

    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

/* %TC we now going to use icd_[agent/customer]standard_state_call_end
 * and see how that work, if all in agreement then we can nuke this
 * this is implmented using the new plugable role specific static function ptrs struc
 * in the agent / customer
*/
    if (icd_caller__has_role(that, ICD_AGENT_ROLE))
        ret = icd_agent__standard_state_call_end(event, extra);
    else
        ret = icd_customer__standard_state_call_end(event, extra);

    return 0;
}

/* Standard function for reacting to the bridge failed state */
int icd_caller__standard_state_bridge_failed(icd_event * event, void *extra)
{
    icd_caller *that = NULL;

    assert(event != NULL);

    /* Actions for when the bridge failed:
     *   - set state to READY (and thus back into distributor)
     *   - set state to READY if bridge failed < 5 times
     *   - set pushed_back attribute and then set state to READY
     *   - destroy caller
     *
     */
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);
    
    if (icd_caller__has_role(that, ICD_BRIDGER_ROLE)) {    
    	icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_BRIDGE_FAILED);
    }

    return icd_caller__pushback_and_ready_on_fail(event, extra);
}

/* Standard function for reacting to the channel failed state */
int icd_caller__standard_state_channel_failed(icd_event * event, void *extra)
{
    assert(event != NULL);

    return icd_caller__limited_ready_state_on_fail(event, extra);
}

/* Standard function for reacting to the associate failed state */
int icd_caller__standard_state_associate_failed(icd_event * event, void *extra)
{
    assert(event != NULL);
    /*
       that = (icd_caller *)icd_event__get_source(event);
       assert(that != NULL);
     */

    return icd_caller__pushback_and_ready_on_fail(event, extra);
}

/*! Standard function for reacting to the suspend state */
/*!
 * This is the standard function for controlling what happens when we
 * enter the SUSPEND state in icd_caller.
 *
 * It has a variety of actions it can perform, including:
 *    1) Just returning back to the __run() loop (where it will be suspended
 *   until the state changes again)
 *    2) Sleeping for a given period of time, going to the READY state
 *   when done.
 *    3) Listen for DTMF (either a specific key or any key) and go to the
 *   READY state when the key is heard or a timeout occurs.
 *    4) Exit icd_caller if agent off-hook with hangup
 *
 * For any of these options, entertainment can conditionally be played during
 * the suspension. If it is, we stop it when we leave this function unless
 * we are returning right away, in which case it is assumed that the combination
 * of instant return with entertainment that has been requested means that they
 * want the entertainment to continue. In this case, we assume that the state
 * change after ours will deal with stopping the entertainment.
 *
 * /param event the ICD_CALLER_STATE_SUSPEND event object
 * /param extra not used
 * /return 0 because we do not want to veto this ever
 */

int icd_caller__standard_state_suspend(icd_event * event, void *extra)
{


/*this is realy an agent speific state so it it now plugged into
 * icd_agent__standard_state_suspend
 * well remove this once we see this working well
 *
*/
    icd_caller *that;
/*     char *action; */
/*     char *entertain; */
/*     char *wakeup; */
/*     char *wait; */
/*     int waittime; */
/*     char res; */
/*     char *pos = NULL; */
/*     int cleanup_required = 0; */
    int ret;

    if (icd_caller__has_role(that, ICD_AGENT_ROLE))
        ret = icd_agent__standard_state_call_end(event, extra);

#if 0
    assert(event != NULL);
    that = icd_event__get_source(event);
    assert(that != NULL);
    assert(that->params != NULL);

    /* OffHook agent with PBX thread hungup */
    if ((that->chan == NULL || (that->chan != NULL && that->chan->_softhangup))
        && (icd_caller__get_onhook(that) == 0)) {
        that->entertained = ICD_ENTERTAIN_NONE;
        that->thread_state = ICD_THREAD_STATE_FINISHED;
        return 0;
    }
    action = vh_read(that->params, "suspend.action");
    entertain = vh_read(that->params, "suspend.entertain");
    wait = vh_read(that->params, "wrapup");
    wakeup = vh_read(that->params, "suspend.wakeup");

    /* TC remove from distributor maybe bcus hes holding a place in line */

    /* Decode the right action to perform given various parameters */
    if (action == NULL) {
        if (wakeup != NULL) {
            action = "listen";
        } else if (wait != NULL) {
            action = "sleep";
        }
    }

    if (entertain != NULL && cw_true(entertain)) {
        icd_caller__start_waiting(that);
        cleanup_required = 1;
    }
    if (action == NULL || strcmp(action, "none") == 0) {
        /* Note that we started the entertainment without stopping it here */
        return 0;
    }

    waittime = 0;
    if (wait != NULL) {
        waittime = atoi(wait);
    }
    if (waittime == 0) {
        /* Default wrapup time */
        waittime = 120;
    }

    if (strcmp(action, "sleep") == 0) {
        sleep(waittime);
        icd_caller__set_state(that, ICD_CALLER_STATE_READY);
    } else if ((strcmp(action, "listen") == 0) && (icd_caller__get_onhook(that) == 0)) {
        while (pos == NULL) {
            res = cw_waitfordigit(that->chan, waittime);
            if (res < 1) {
                if (wakeup == NULL || strlen(wakeup) == 0) {
                    break;
                } else {
                    pos = strchr(wakeup, res);
                }
            }
        }
    }
    if (cleanup_required) {
        icd_caller__stop_waiting(that);
    }

#endif
    return 0;
}

/* Standard behaviours called from inside the caller state action functions */

/* Standard function to yse a conference-style bridge */
int icd_caller__standard_state_conference(icd_event * event, void *extra)
{
    icd_caller *caller;
/*     icd_queue *queue; */
/*     icd_member *member; */
    
    caller = (icd_caller *) icd_event__get_source(event);
    assert(caller != NULL);
   
    icd_caller__set_start_now(caller);
    caller->callcount++;
    icd_conference__join(caller);
    
    return 0;
}

/* Standard function to prepare a caller object for entering a distributor */
icd_status icd_caller__standard_prepare_caller(icd_caller * that)
{
    assert(that != NULL);

    return ICD_SUCCESS;
}

/* Standard function for cleaning up a caller after a bridge ends or fails */
icd_status icd_caller__standard_cleanup_caller(icd_caller * that)
{
    int ret;

    assert(that != NULL);
/* TC we now going to use icd_[agent/customer]standard_cleanup_caller
 * and see how that work, if all in agreement then we can nuke this
*/

    if (icd_caller__has_role(that, ICD_AGENT_ROLE))
        ret = icd_agent__standard_cleanup_caller(that);
    else
        ret = icd_customer__standard_cleanup_caller(that);

    return ICD_SUCCESS;
}

/* Standard function for starting the bridging process on a caller */
icd_status icd_caller__standard_launch_caller(icd_caller * that)
{
    assert(that != NULL);

    return ICD_SUCCESS;
}

/* Standard function for printing out a copy of the caller */
icd_status icd_caller__standard_dump(icd_caller * caller, int verbosity, int fd, void *extra)
{
    int skip_opening;

    assert(caller != NULL);

    if (extra == NULL) {
        skip_opening = 0;
    } else {
        skip_opening = *((int *) extra);
    }
    /* should be assume verbosity = -1 means debug ??
       if (skip_opening == 0) {
       cw_cli(fd,"\nDumping icd_caller {\n");
       }
     */
    icd_caller__dump_debug_fd(caller, fd, "      ");
    /*
       cw_cli(fd,"      name=%s\n", icd_caller__get_name(caller));
       cw_cli(fd,"    id=%d\n", caller->id);
     */
    /* TBD Write some of these out, too
       struct cw_channel *chan;
       void *owner;
       void *authorization;
       icd_caller_list *associations;
       int in_thread;
       int in_loop;
       icd_caller_state state;
       time_t caller_created;
       time_t last_mod;
       time_t last_state_change;
       time_t start_call;
       int timeout;
       int authenticated;
       int maxclones;
       int clonecount;
       int am_clone;
       icd_caller *parent;
       void_hash_table *params;
       int priority;
       int role;
       int allocated;
       int (*auth_fn)(icd_caller *, void *);
       int (*state_fn)(icd_caller *, int, int);
       int (*chan_fn)(icd_caller *, cw_channel *);
       int (*added_fn)(icd_caller *, icd_caller_list *);
       int (*link_fn)(icd_caller *, icd_caller *);
       int (*bridge_fn)(icd_caller *, icd_caller *);
       int (*authn_fn)(icd_caller *, int);
       icd_status (*dump_fn)(icd_caller *. int verbosity, int fd, void *extra);
       void *dump_fn_extra;
       icd_thread_state thread_state;
     */
    /*
       if (skip_opening == 0) {
       cw_cli(fd,"}\n");
       }
     */
    return ICD_SUCCESS;
}

/***** Comparison functions ("void *" are all "icd_caller *") *****/
/* These functions are set in icd_distributor.c in the init_icd_distributor_ordered
 * for icd_list__set_node_insert_func that uses the icd_list__insert_ordered call
 * The icd_caller_cmp_[myfunc] is passed 2 icd_caller nodes from icd_member.c cmp
 * (Arg1-an existing node from a list, and Arg2-the New Node to Insert)
 * these functions are called repeatably from the icd_list_insert_ordered untill the
 * correct position in the list for the new icd_member node Arg2 is located,
 * the cmp functions should return the following
 * 0 if the comparison logic say these value are the same
 * 1 if arg1 > arg2
 * -1 if arg2 > arg1
*/
/* this is used by roundrobinr compare last call start time, if the same we compare the created time */
int icd_caller__cmp_call_start_time_order(icd_caller * caller1, icd_caller * caller2)
{
    time_t Time1;
    time_t Time2;

    assert(caller1 != NULL);
    assert(caller2 != NULL);

    Time1 = icd_caller__get_start(caller1);
    Time2 = icd_caller__get_start(caller2);

    if (Time1 == Time2) {
        Time1 = caller1->caller_created;
        Time2 = caller2->caller_created;
    }

    return (Time1 > Time2) ? -1 : ((Time1 < Time1) ? 1 : 0);
}

int icd_caller__cmp_caller_id_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->id > caller2->id) ? 1 : ((caller1->id < caller2->id) ? -1 : 0);
}

int icd_caller__cmp_caller_id_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_caller_id_order(caller2, caller1);
}

int icd_caller__cmp_caller_name_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return strcmp(caller1->name, caller2->name);
}

int icd_caller__cmp_caller_name_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_caller_name_order(caller2, caller1);
}

int icd_caller__cmp_timeout_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->timeout > caller2->timeout) ? 1 : ((caller1->timeout < caller2->timeout) ? -1 : 0);
}

int icd_caller__cmp_timeout_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_timeout_order(caller2, caller1);
}

int icd_caller__cmp_last_mod_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->last_mod > caller2->last_mod) ? 1 : ((caller1->last_mod < caller2->last_mod) ? -1 : 0);
}

int icd_caller__cmp_last_mod_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_last_mod_order(caller2, caller1);
}

int icd_caller__cmp_start_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->caller_created > caller2->caller_created) ? 1 : ((caller1->caller_created <
            caller2->caller_created) ? -1 : 0);
}

int icd_caller__cmp_start_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_start_order(caller2, caller1);
}

int icd_caller__cmp_callcount_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->callcount > caller2->callcount) ? 1 : ((caller1->callcount < caller2->callcount) ? -1 : 0);
}

int icd_caller__cmp_callcount_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_callcount_order(caller2, caller1);
}

int icd_caller__cmp_state_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    /* We may want to get more sophisticated if the state order isn't our order. */
    return (caller1->state > caller2->state) ? 1 : ((caller1->state < caller2->state) ? -1 : 0);
}

int icd_caller__cmp_state_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_state_order(caller2, caller1);
}

int icd_caller__cmp_priority_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->priority > caller2->priority) ? 1 : ((caller1->priority < caller2->priority) ? -1 : 0);
}

int icd_caller__cmp_priority_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_priority_order(caller2, caller1);
}

int icd_caller__cmp_last_state_change_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return (caller1->last_state_change > caller2->last_state_change) ? 1 : ((caller1->last_state_change <
            caller2->last_state_change) ? -1 : 0);
}

int icd_caller__cmp_last_state_change_reverse_order(icd_caller * caller1, icd_caller * caller2)
{
    assert(caller1 != NULL);
    assert(caller2 != NULL);

    return icd_caller__cmp_last_state_change_order(caller2, caller1);
}

/*===== Private Implementations =====*/

/* Create a thread for caller that runs independently of the PBX */
static icd_status icd_caller__create_thread(icd_caller * that)
{
    pthread_attr_t attr;
    int result;

    assert(that != NULL);

    /* Adjust thread attributes here before creating it */
    result = pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);       /* Or do we want parent scheduling? */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    /* Adjust the thread state before creating thread */
    that->thread_state = ICD_THREAD_STATE_PAUSED;
    /* Create thread */
    result = cw_pthread_create(&(that->thread), &attr, icd_caller__run, that);
    that->using_caller_thread = 1;
    cw_verbose(VERBOSE_PREFIX_2 "Spawn thread for Caller id[%d] [%s]\n", icd_caller__get_id(that),
        icd_caller__get_name(that));
    /* Clean up */
    result = pthread_attr_destroy(&attr);
    return ICD_SUCCESS;
}

void icd_caller__dump_debug(icd_caller * that)
{
    icd_caller__dump_debug_fd(that, ICD_STDERR, "  == ");
}

void icd_caller__dump_debug_fd(icd_caller * that, int fd, char *indent)
{
    char *ptr;
    char *action;
    char *entertain;
    char *wakeup;
    char *wait;
    int waittime;

    icd_member *member;
    icd_queue *queue;
    icd_list_iterator *iter;

    if (indent) {
        cw_cli(fd, "%s", indent);
    }

    if (that->chan != NULL && that->chan->name) {
        cw_cli(fd, "Chan[%s] ", that->chan->name);
    }
    if (that->chan_string) {
        cw_cli(fd, "ChanStr[%s]", icd_caller__get_channel_string(that));
    } else {
        cw_cli(fd, "ChanStr[]");
    }

    if (icd_caller__get_onhook(that)) {
        cw_cli(fd, " OnHook[YES]");
    } else {
        cw_cli(fd, " OnHook[NO]");
    }

    if (icd_caller__get_dynamic(that)) {
        cw_cli(fd, " Dynamic[YES]");
    } else {
        cw_cli(fd, " Dynamic[NO]");
    }

    if (icd_caller__get_pushback(that)) {
        cw_cli(fd, " PushBack[YES]");
    } else {
        cw_cli(fd, " PushBack[NO]");
    }

    if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
        /* Agent specfic attributes */
        cw_cli(fd, " Timeout[%d]", icd_caller__get_timeout(that));
        cw_cli(fd, " AckCall[%d]", icd_caller__get_acknowledge_call(that));
        cw_cli(fd, " Priority[%d]", icd_caller__get_priority(that));

        action = vh_read(that->params, "suspend.action");
        entertain = vh_read(that->params, "suspend.entertain");
        wakeup = vh_read(that->params, "suspend.wakeup");
        wait = vh_read(that->params, "wrapup");
        waittime = 0;
        if (wait != NULL) {
            waittime = atoi(wait);
        }
        cw_cli(fd, " Entertain[%s]", entertain);
        cw_cli(fd, " WrapUp[%d]", waittime);
    }
    /*%TC remove cw_log in icd_[blah]_get_plugable_fns since that dumps here */
    cw_cli(fd, "plugable_fns[%s] FnCount[%d]", icd_plugable__get_name(that->get_plugable_fn(that)),
        icd_plugable_fn_list_count(that->plugable_fns_list));

    if (that->memberships != NULL) {
        /* no memberships if just in a queue with socket based distributor */
        iter = icd_list__get_iterator((icd_list *) (that->memberships));
        while (icd_list_iterator__has_more(iter)) {
            member = icd_list_iterator__next(iter);
            queue = icd_member__get_queue(member);
            cw_cli(fd, " Q[%s][%d/%d]", icd_queue__get_name(queue), icd_caller__get_position(that, member),
                icd_caller__get_pending(that, member)
                );
        }
        destroy_icd_list_iterator(&iter);
    }

    ptr = icd_caller__get_name(that);
    if (strlen(ptr) > 0) {
        cw_cli(fd, " [%s]", ptr);
    } else {
        cw_cli(fd, " [no-name]");
    }

    cw_cli(fd, " ID[%d] STATE[%s] ", that->id, icd_caller_state_strings[that->state]);
    cw_cli(fd, "Role[%d] -> ", that->role);

    if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
        cw_cli(fd, "[AGENT]");
    }
    if (icd_caller__has_role(that, ICD_CUSTOMER_ROLE)) {
        cw_cli(fd, "[CUSTOMER]");
    }
    if (icd_caller__has_role(that, ICD_BRIDGER_ROLE)) {
        cw_cli(fd, "[BRIDGER]");
    }
    if (icd_caller__has_role(that, ICD_BRIDGEE_ROLE)) {
        cw_cli(fd, "[BRIDGEE]");
    }
    if (icd_caller__has_role(that, ICD_LOOPER_ROLE)) {
        cw_cli(fd, "[LOOPER]");
    }
    if (icd_caller__has_role(that, ICD_CLONER_ROLE)) {
        cw_cli(fd, "[CLONER]");
    }
    if (icd_caller__has_role(that, ICD_CLONE_ROLE)) {
        cw_cli(fd, "[CLONE]");
    }
    if (icd_caller__has_role(that, ICD_INVALID_ROLE)) {
        cw_cli(fd, "[INVALID]");
    }

    cw_cli(fd, "Flag[%d] -> ", that->flag);

    if (icd_caller__has_flag(that, ICD_MONITOR_FLAG)) {
        cw_cli(fd, "[MONITOR]");
    }
    if (icd_caller__has_flag(that, ICD_CONF_MEMBER_FLAG)) {
        cw_cli(fd, "[CONF_MEMBER]");
    }
    if (icd_caller__has_flag(that, ICD_NOHANGUP_FLAG)) {
        cw_cli(fd, "[NOHANGUP]");
    }

    cw_cli(fd, "\n");
}

/* Equivalent to cw_request() */
cw_channel *icd_caller__create_channel(icd_caller * that)
{
    struct cw_channel *chan;
    char *chanstring;
    char *context;
    char *priority;
    char *extension;

    assert(that != NULL);

    chan = icd_caller__get_channel(that);
    if (chan != NULL) {
        return chan;
    }
    chanstring = icd_caller__get_channel_string(that);
    context = icd_caller__get_param(that, "context");
    priority = icd_caller__get_param(that, "priority");
    extension = icd_caller__get_param(that, "extension");
    if (icd_debug)
        cw_log(LOG_DEBUG, "Creating Channel for caller %d [%s]  chan=%s, c=%s, p=%s, e=%s\n",
            icd_caller__get_id(that), icd_caller__get_name(that), chanstring, context, priority, extension);

    chan = icd_bridge_get_callweaver_channel(chanstring, context, priority, extension);

    if (chan != NULL) {
        icd_caller__assign_channel(that, chan);
    } else {
        if (chanstring == NULL) {
            chanstring = "null";
        }
        if (context == NULL) {
            context = "null";
        }
        if (priority == NULL) {
            priority = "null";
        }
        if (extension == NULL) {
            extension = "null";
        }
        cw_log(LOG_WARNING, "Channel for caller %d [%s] could not be created\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
        cw_log(LOG_WARNING, "    channel=[%s] context=[%s] priority=[%s] extension=[%s]", chanstring, context,
            priority, extension);
    }
    that->owns_channel = 1;

    return chan;
}

/* Assigning a channel to a caller does not necessarily bring the channel
   up. This function does that by ensuring the channel is answered. */
/* TBD - This assumes we are using a channel string. How can we change that? */
icd_status icd_caller__dial_channel(icd_caller * that)
{
    char *chanstring;
    char *verify_app, *verify_app_arg;
    int timeout;
    int result;
    struct cw_app *app;

    assert(that != NULL);
    assert(that->chan != NULL);

    if (that->chan->_state == CW_STATE_UP) {
        return ICD_SUCCESS;
    }
    /* Deal with issue in callweaver that leaves state in RINGING */
    result = cw_answer(that->chan);
    if (that->chan->_state == CW_STATE_UP) {
        return ICD_SUCCESS;
    }

    /* If we got here, our channel is definitely down so we need to ring it */
    if (icd_debug)
        cw_log(LOG_NOTICE, "Attempting to dial channel for caller %d [%s] \n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    chanstring = icd_caller__get_channel_string(that);
    timeout = that->timeout;
    result = icd_bridge_dial_callweaver_channel(that, chanstring, timeout);
    if (that->chan != NULL && that->chan->_state == CW_STATE_UP) {
        cw_set_read_format(that->chan, cw_best_codec(that->chan->nativeformats));
        cw_set_write_format(that->chan, that->chan->readformat);
        /* do we raise icd channel up here or down in icd_bridge done in bridge for now
           icd_event__generate(ICD_EVENT_CHANNEL_UP, NULL);
         */
        verify_app = icd_caller__get_param(that, "verify_app");
        verify_app_arg = icd_caller__get_param(that, "verify_app_arg");
        result = 0;
        if (verify_app) {
            app = pbx_findapp(verify_app);
            if (app) {
                cw_verbose(VERBOSE_PREFIX_2 "Calling Verify App: %s(%s)\n", verify_app,
                    verify_app_arg ? verify_app_arg : "");
                result = pbx_exec(that->chan, app, (verify_app_arg ? verify_app_arg : ""));
            }

        }
        if (result == 0 && that->chan) {
            return ICD_SUCCESS;
        } else if (that->chan) {
            cw_hangup(that->chan);
            that->chan = NULL;
        }

    }

    if (that->chan == NULL) {
        cw_log(LOG_WARNING, "Caller id[%d] [%s] channel just went away\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    } else {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller id[%d] [%s] channel state is %d [%s]\n", icd_caller__get_id(that),
                icd_caller__get_name(that), that->chan->_state, cw_state2str(that->chan->_state));
    }

    if (chanstring == NULL) {
        chanstring = "null";
    }
    cw_log(LOG_WARNING, "Caller id[%d] [%s] channel[%s] did not come up timeout[%d] \n", icd_caller__get_id(that),
        icd_caller__get_name(that), chanstring, timeout);
    return ICD_EGENERAL;
}

void *icd_caller__get_param(icd_caller * that, char *param)
{
    /* tbd conver to icd_fieldset
       return icd_fieldset__get_value(that->params, param)
     */

    return (that && that->params && param) ? vh_read(that->params, param) : NULL;

}

int icd_caller__del_param(icd_caller * that, char *param)
{
    /* tbd conver to icd_fieldset
       return icd_fieldset__get_value(that->params, param)
     */
    return (that && that->params && param) ? vh_delete(that->params, param) : 0;
}

icd_status icd_caller__set_param(icd_caller * that, char *param, void *value)
{

    if (that && that->params && param && value) {
        if (vh_write(that->params, param, value)) {
            return ICD_SUCCESS;
        }
    }
    return ICD_EGENERAL;
}

icd_status icd_caller__set_param_string(icd_caller * that, char *param, void *value)
{

    if (that && that->params && param && value) {
        if (vh_write_cp_string(that->params, param, value)) {
            return ICD_SUCCESS;
        }
    }
    return ICD_EGENERAL;
}

icd_status icd_caller__set_param_if_null(icd_caller * that, char *param, void *value)
{
    void *test;

    if (that && that->params && param && value) {
        test = vh_read(that->params, param);
        if (!test) {
            if (vh_write(that->params, param, value)) {
                return ICD_SUCCESS;
            }
        }
    }
    return ICD_EGENERAL;
}

/* Drops all linked callers */
icd_status icd_caller__remove_all_associations(icd_caller * that)
{
    icd_caller *associate;
    icd_status result;
    icd_status final_result;

    assert(that != NULL);
    assert(that->associations != NULL);

//    icd_caller_list__lock(that->associations);
    final_result = ICD_SUCCESS;
    associate = icd_list__pop((icd_list *) that->associations);
    while(associate != NULL) {
        result = icd_caller_list__remove_caller_by_element(associate->associations, that);
        if (result != ICD_SUCCESS) {
            final_result = result;
        }
    	associate = icd_list__pop((icd_list *) that->associations);
    }
    return final_result;
}

/* Drops caller from all queues it is a member of. */
icd_status icd_caller__remove_from_all_queues(icd_caller * that)
{
    icd_member *member;
    icd_queue *queue;
    icd_status result;
    icd_status final_result;

    assert(that != NULL);
    assert(that->memberships != NULL);

    final_result = ICD_SUCCESS;
    member = icd_list__peek((icd_list *) that->memberships);
    while (member != NULL) {
        queue = icd_member__get_queue(member);
        result = icd_caller__remove_from_queue(that, queue);
        if (result != ICD_SUCCESS) {
            final_result = result;
        }
        member = icd_list__peek((icd_list *) that->memberships);
    }
    return final_result;
}

icd_status icd_caller__join_callers(icd_caller * that, icd_caller * associate)
{
    int result;
    icd_member *member;
    icd_list_iterator *iter;
    icd_queue *queue;

    if (icd_debug)
        cw_log(LOG_DEBUG, "CROSS-LINK: %d to %d\n", that->id, associate->id);
/*We remove callers from all distributors */
 	
    icd_list__lock((icd_list *) (that->memberships));
    iter = icd_list__get_iterator((icd_list *) (that->memberships));
    while (icd_list_iterator__has_more_nolock(iter)) {
            member = (icd_member *) icd_list_iterator__next(iter);
	     	queue = icd_member__get_queue(member);
	     	if (queue){
	     		if(icd_caller__has_role(that, ICD_AGENT_ROLE)) {
                	icd_queue__agent_dist_quit(queue, member);		     
	     		}
	     		else {
                	icd_queue__customer_dist_quit(queue, member);		     
	     		}
			}
    }
    icd_list__unlock((icd_list *) (that->memberships));
    destroy_icd_list_iterator(&iter);

    icd_list__lock((icd_list *) (associate->memberships));
    iter = icd_list__get_iterator((icd_list *) (associate->memberships));
    while (icd_list_iterator__has_more_nolock(iter)) {
         member = (icd_member *) icd_list_iterator__next(iter);
	     queue = icd_member__get_queue(member);
	     	if (queue){
	     		if(icd_caller__has_role(associate, ICD_AGENT_ROLE)) {
                	icd_queue__agent_dist_quit(queue, member);		     
	     		}
	     		else {
                	icd_queue__customer_dist_quit(queue, member);		     
	     		}
			}
    }
    icd_list__unlock((icd_list *) (associate->memberships));
    destroy_icd_list_iterator(&iter);
    
    result = icd_caller__link_to_caller(that, associate);
    if (result != ICD_SUCCESS) {
        return result;
    }
    result = icd_caller__link_to_caller(associate, that);
    if (result != ICD_SUCCESS) {
	icd_caller__remove_all_associations(that);
    }
    return result;
}

/* Send a caller back to all of its distributors */
icd_status icd_caller__return_to_distributors(icd_caller * that)
{
    icd_member *member;
    icd_queue *queue;
    icd_list_iterator *iter;
    icd_status result;
    icd_status final_result;

    assert(that != NULL);
    assert(that->memberships != NULL);

    final_result = ICD_SUCCESS;
    icd_list__lock((icd_list *) (that->memberships));
    iter = icd_list__get_iterator((icd_list *) (that->memberships));
    while (icd_list_iterator__has_more_nolock(iter)) {
        member = icd_list_iterator__next(iter);
        queue = icd_member__get_queue(member);
        if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
            result = icd_queue__agent_distribute(queue, member);
        } else {
            result = icd_queue__customer_distribute(queue, member);
        }
        if (result != ICD_SUCCESS) {
            final_result = result;
        }
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock((icd_list *) (that->memberships));
    return final_result;
}

/* default moh/ringing handlers.... */
icd_status icd_caller__standard_start_waiting(icd_caller * caller)
{
    char *moh;

    if (!caller->chan) {
        return ICD_ENOTFOUND;
    }

    if (icd_caller__has_flag(caller, ICD_ENTLOCK_FLAG))
        return ICD_SUCCESS;

    moh = icd_caller__get_moh(caller);
    if (caller->entertained == ICD_ENTERTAIN_NONE) {
        if (!strcmp(moh, "ringing") && caller->chan) {
            cw_indicate(caller->chan,  CW_CONTROL_RINGING);
            caller->entertained = ICD_ENTERTAIN_RING;
        } else if (caller->chan) {
            cw_moh_start(caller->chan, moh);
            caller->entertained = ICD_ENTERTAIN_MOH;
        }
    }
    return ICD_SUCCESS;
}

icd_status icd_caller__play_sound_file(icd_caller *caller, char *file)
{
    int res = 0;
    int ent = 0;

    if(caller->entertained) {
        ent = caller->entertained;
        icd_caller__stop_waiting(caller);
    }

    if (caller->chan == NULL || !file || !strlen(file))
        res = -1;
    else {
        res = cw_streamfile(caller->chan, file, caller->chan->language);
        if (!res) {
            res = cw_waitstream(caller->chan,  CW_DIGIT_ANY);
        }
    }

    if(ent)
        icd_caller__start_waiting(caller);
    
    return res;
}

icd_status icd_caller__standard_stop_waiting(icd_caller * caller)
{

    if (!caller->chan) {
        return ICD_ENOTFOUND;
    }

    if (icd_caller__has_flag(caller, ICD_ENTLOCK_FLAG))
        return ICD_SUCCESS;

    if (caller->chan) {
        cw_clear_flag(caller->chan,  CW_FLAG_BLOCKING);

        cw_moh_stop(caller->chan);

        if (caller->chan->stream) {
            cw_stopstream(caller->chan);       /* cut off any files that are playing */
        }
    }
    caller->entertained = ICD_ENTERTAIN_NONE;

    return ICD_SUCCESS;
}

icd_status icd_caller__set_state_on_associations(icd_caller * that, icd_caller_state state)
{

    icd_list_iterator *iter;
    icd_caller *associate;
    icd_status result;

    assert(that != NULL);
    assert(that->associations != NULL);

    icd_list__lock((icd_list *) (that->associations));
    iter = icd_list__get_iterator((icd_list *) (that->associations));

    while (icd_list_iterator__has_more_nolock(iter)) {
        
        associate = (icd_caller *) icd_list_iterator__next(iter);
	if (associate != NULL)
            result = icd_caller__set_state(associate, state);

        if (result != ICD_SUCCESS) {

            /* TBD - Cleanup from invalid state change */
        }
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock((icd_list *) (that->associations));
    return result;
}

/* Return the number of plugable function sets for this caller
int icd_caller__get_plugable_fns_count(icd_caller *that) {
    assert(that != NULL);
    assert(that->plugable_fns_list != NULL);

    return icd_list__count((icd_list *)(that->plugable_fns_list));
}
*/
/* Sets the plugable_fn function to a particular function. */
icd_status icd_caller__set_plugable_fn_ptr(icd_caller * that, icd_plugable_fn * (*get_plugable_fn) (icd_caller *))
{
    assert(that != NULL);
    //icd_plugable__set_fn(that->get_plugable_fn ,get_plugable_fn);
    that->get_plugable_fn = get_plugable_fn;
    if (icd_debug)
        cw_log(LOG_NOTICE, "\nCaller %d [%s] SET plugable_fn_ptr[%s] ready_fn[%p]\n", icd_caller__get_id(that),
            icd_caller__get_name(that), icd_plugable__get_name(that->get_plugable_fn(that)),
            that->get_plugable_fn(that)->state_ready_fn);

    return ICD_SUCCESS;
}

icd_plugable_fn_list *icd_caller__get_plugable_fn_list(icd_caller * that)
{
    assert(that != NULL);
    assert(that->plugable_fns_list != NULL);

    return that->plugable_fns_list;
}

char *icd_caller__get_plugable_fns_name(icd_caller * that)
{
    assert(that != NULL);

    return icd_plugable__get_name(that->get_plugable_fn(that));
}

icd_plugable_fn *icd_caller_get_plugable_fns(icd_caller * that)
{
    icd_plugable_fn *plugable_fns = NULL;
    icd_distributor *dist = NULL;

    assert(that != NULL);

    plugable_fns = &icd_caller_plugable_fns;
    if (icd_caller__get_active_member(that) == NULL)
        plugable_fns = &icd_caller_plugable_fns;
    else {
        dist = (icd_distributor *) icd_caller__get_active_member(that);
        plugable_fns = icd_distributor__get_plugable_fn(dist, that);
        if (plugable_fns == NULL)
            plugable_fns = &icd_caller_plugable_fns;
    }
    if (plugable_fns == NULL) {
        cw_log(LOG_ERROR, "Caller %d [%s] has no plugable fn aborting ala crash\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    } else {
        if (icd_debug)
            cw_log(LOG_DEBUG, "\nCaller id[%d] [%s] using plugable_fns[%s] ready_fn[%p]\n",
                icd_caller__get_id(that), icd_caller__get_name(that), icd_plugable__get_name(plugable_fns),
                plugable_fns->state_ready_fn);
    }
    assert(plugable_fns != NULL);
    return plugable_fns;

}

/* the is the entry point from any exec in app_icd via pbx dialplan */
void icd_caller__loop(icd_caller * that, int do_spawn)
{
    assert(that != NULL);
    /* Should this be an assert instead? */
    if (that->thread_state != ICD_THREAD_STATE_UNINITIALIZED) {
        return;
    }

    if (do_spawn) {
        icd_caller__create_thread(that);        /* this kick starts icd_caller__run */
        icd_caller__start_caller_response(that);
    } else {
        /* Need to "start" beforehand because we don't return til thread ends */
        icd_caller__start_caller_response(that);
        icd_caller__run(that);
        /* PBX thread returning, set state for next PBX thread */
        that->thread_state = ICD_THREAD_STATE_UNINITIALIZED;
    }
}

icd_plugable_fn *icd_caller__get_pluggable_fn(icd_caller * that)
{
    assert(that != NULL);
    return that->get_plugable_fn(that);
}

/*
  Danger Will Robinson!  Some people who know what their doing can
  hijack the entire run function of the caller and/or distributor.
  Once you do this, of course, it invalidates your warranty ;)
*/

static void *icd_caller__run(void *ptr)
{
    icd_caller *that;
    icd_plugable_fn *icd_run;

    assert(ptr != NULL);
    assert(((icd_caller *) ptr)->thread_state != ICD_THREAD_STATE_UNINITIALIZED);
    that = (icd_caller *) ptr;

    icd_run = that->get_plugable_fn(that);
    if (!icd_run->run_fn)
        icd_run->run_fn = icd_caller__standard_run;

    return icd_run->run_fn(that);

}

/* The run method for the caller thread. Method signature dictated by pthread. */
void *icd_caller__standard_run(void *ptr)
{
    icd_caller *that;
    icd_caller_state state;
    icd_caller_state last_state;
    icd_status result;
    icd_status vetoed;
    icd_list *live_associations;
    icd_plugable_fn *icd_run;
    icd_customer *customer;
    icd_agent *agent;

    assert(ptr != NULL);
    assert(((icd_caller *) ptr)->thread_state != ICD_THREAD_STATE_UNINITIALIZED);

    that = (icd_caller *) ptr;
    last_state = ICD_CALLER_STATE_CREATED;

    /* Note that for most state changes, the actual action is handled in a
       notification function. The event is not checked for a veto,
       for 2 reasons:
       a) the state change has already happened,
       b) the only recovery from a veto that could happen in this switch is a state change,
       and it is assumed that the notification function will take care of any
       state change that is required.
       Note these actions are all replaceable using the plugable function finders in the
       caller or the distributor strucs eg icd_caller->get_plugable_fn  or icd_distributor->get_plugable_fn
       they will find a plugable func struc based on the prototype icd_plugable_fn in icd_plugable_fn.h
       This struc has elements like (that->state_[STATE]_fn) these functions ptrs
       are defaulted to Standard functions (icd_[caller|agent|customer]__standard_state_[STATE])
       for reacting to the state changes.
       These funcs are located in icd_caller.c icd_agent.c and icd_customer.c
       A family of plugable helpers are also in icd_plugable_fn.c to assist in replaceing the Standard
       action with an custom actions by 3rd party loadable modules
       All state action are represented with the exception of the first and last states
       a) ICD_CALLER_STATE_INITIALIZED
       b) ICD_CALLER_STATE_CLEARED
     */

    /*
       if(setjmp(env) == SIGSEGV) {
       cw_log(LOG_WARNING,"DANGER WILL ROBINSON! This caller created a SEGV\n");
       if(that != NULL) {
       cw_verbose("Let's Not use this caller.\n");
       // dis guy iz trubble forgedd aboudit
       icd_bridge__safe_hangup(that);
       if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
       agent = (icd_agent *) that;
       destroy_icd_agent(&agent);
       }
       else {
       customer = (icd_customer *) that;
       destroy_icd_customer(&customer);
       }

       }
       else {
       // this is not really the right place
       abort();
       }
       return NULL;
       }
     */

    while (that->thread_state != ICD_THREAD_STATE_FINISHED) {
        if (that->thread_state == ICD_THREAD_STATE_RUNNING) {
            result = icd_caller__lock(that);
            state = icd_caller__get_state(that);
            if (last_state != state) {
                result = icd_caller__unlock(that);
                last_state = state;
                icd_run = that->get_plugable_fn(that);
                switch (state) {
                case ICD_CALLER_STATE_INITIALIZED:
                    /* First state of the caller, no pluggable actions allowed
                     *  When the following state change happens, it means that the
                     *  caller has a thread in its run() method.
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] in state - ICD_CALLER_STATE_INITIALIZED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that));
                     */
                    icd_caller__set_state(that, ICD_CALLER_STATE_READY);
                case ICD_CALLER_STATE_READY:
                    /* Indicates a  caller. is ready for bridging
                     * standard plugable function is icd_caller__standard_state_ready
                     * Required state in order to be available for
                     * distributor to link. If not in this state,
                     * distributor discards member when popped.
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - READY \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_ready_fn);
                     */

                    if (that->active_member != NULL) {
                        if (last_state == ICD_CALLER_STATE_DISTRIBUTING) {
                            /* Distributing didn't result in bridge */
                            that->require_pushback = 1;
                        }
                    }

                    icd_event__notify(ICD_EVENT_READY, NULL, icd_run->state_ready_fn,
                        icd_run->state_ready_fn_extra);

                    break;
                case ICD_CALLER_STATE_DISTRIBUTING:
                    /* Indicates a distributor is going to link this caller.
                     * No action is usually required. This is a thread information state.
                     * standard plugable function is icd_caller__standard_state_distribute
                     if (icd_verbose > 4)*/
                     cw_log(LOG_NOTICE, "Caller id[%d] name[%s] caller_id[%s] run[%p] in state - DISTRIBUTING \n",
                     icd_caller__get_id(that), icd_caller__get_name(that), icd_caller__get_caller_id(that), icd_run->state_distribute_fn);
                     
                     
                    icd_event__notify(ICD_EVENT_DISTRIBUTE, NULL, icd_run->state_distribute_fn,
                        icd_run->state_distribute_fn_extra);
                    break;
                case ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE:
                    /*Indicates that the distributors designated bridger will now attempt to
                     * bridge callers
                     * bringing up channels if required to make a bridged call
                     * standard plugable function is icd_caller__standard_state_get_channels
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - GET_CHANNELS_AND_BRIDGE \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_get_channels_fn );
                     */
                    icd_event__notify(ICD_EVENT_GET_CHANNELS, NULL, icd_run->state_get_channels_fn,
                        icd_run->state_get_channels_fn_extra);
                    break;

                case ICD_CALLER_STATE_CONFERENCED:
                    /* conference bridge
                     * No action is usually required. This is a thread information state.
                     * standard plugable function is icd_caller__standard_state_conference
                     Everyone flagged for the same conference will end up here
                     until one of the members of the conference exits.
                     The fisrt one out will tell everyone else to exit too (for now).
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - CONFERENCED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_conference_fn);
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] finished Conference \n",
                     icd_caller__get_id(that), icd_caller__get_name(that));
                     */
                    icd_event__notify(ICD_EVENT_CONFERENCE, NULL, icd_run->state_conference_fn,
                        icd_run->state_conference_fn_extra);

                    break;
                case ICD_CALLER_STATE_BRIDGED:
                    /* Indicates Bridger has bridged an assocate, set state on self as well
                     * No action is usually required. This is a thread information state.
                     * standard plugable function is icd_caller__standard_state_bridged
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - BRIDGED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_bridged_fn);
                     */
                    vetoed =
                        icd_event__notify(ICD_EVENT_BRIDGED, live_associations, icd_run->state_bridged_fn,
                        icd_run->state_bridged_fn_extra);
                    break;
                case ICD_CALLER_STATE_CALL_END:
                    /*Indicates agent/customer have finished call, either party may have hungup the call
                     *may occur from waiting turn, attemping to bridge etc
                     * standard plugable function is icd_caller__standard_state_bridge_end
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - CALL_END \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_call_end_fn);
                     */
                    icd_event__notify(ICD_EVENT_BRIDGE_END, NULL, icd_run->state_call_end_fn,
                        icd_run->state_call_end_fn_extra);
                    break;
                case ICD_CALLER_STATE_SUSPEND:
                    /* Actions to perform when caller suspend action in the queue like warpup time
                     * standard plugable function is icd_caller__standard_state_suspend
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - STATE_SUSPEND \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_suspend_fn);
                     */
                    cw_log(LOG_NOTICE," Caller id[%d] name[%s] caller_id[%s] state - SUSPEND \n",icd_caller__get_id(that), icd_caller__get_name(that), icd_caller__get_caller_id(that));
                    icd_event__notify(ICD_EVENT_SUSPEND, NULL, icd_run->state_suspend_fn,
                        icd_run->state_suspend_fn_extra);
                    break;
                case ICD_CALLER_STATE_CLEARED:
                    /*Last state of the caller no plugable actions allowed */
                    /*
                       if (icd_verbose > 4)
                       cw_log(LOG_NOTICE, "Caller %d [%s] about to be stopped in state CLEARED \n",
                       icd_caller__get_id(that), icd_caller__get_name(that));
                     */
                    icd_caller__stop_caller_response(that);
                    break;

                    /* ALL failure states */
                case ICD_CALLER_STATE_BRIDGE_FAILED:
                    /* Indicates that the bridge attempt failed
                     * standard plugable function is icd_caller__standard_state_bridge_failed
                     * Needed? Does Bridge Failed work instead? TBD
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - BRIDGE_FAILED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_bridge_failed_fn);
                     */
                    icd_event__notify(ICD_EVENT_BRIDGE_FAILED, NULL, icd_run->state_bridge_failed_fn,
                        icd_run->state_bridge_failed_fn_extra);
                    break;
                case ICD_CALLER_STATE_CHANNEL_FAILED:
                    /* Indicates a specific caller whose channel did not come up
                     * standard plugable function is icd_caller__standard_state_channel_failed
                     * Needed? Does Bridge Failed work instead? TBD
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - CHANNEL_FAILED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_channel_failed_fn);
                     */
                    icd_event__notify(ICD_EVENT_CHANNEL_FAILED, NULL, icd_run->state_channel_failed_fn,
                        icd_run->state_channel_failed_fn_extra);
                    break;
                case ICD_CALLER_STATE_ASSOCIATE_FAILED:
                    /* Actions for the bridgee when the bridger's channel is down
                     * standard plugable function is icd_caller__standard_state_associate_failed
                     * Needed? Does Bridge Failed work instead?
                     if (icd_verbose > 4)
                     cw_log(LOG_NOTICE, "Caller %d [%s] run[%p] in state - ASSOCIATE_FAILED \n",
                     icd_caller__get_id(that), icd_caller__get_name(that),icd_run->state_associate_failed_fn);
                     */
                    icd_event__notify(ICD_EVENT_ASSOC_FAILED, NULL, icd_run->state_associate_failed_fn,
                        icd_run->state_associate_failed_fn_extra);
                    break;

                default:
                    cw_log(LOG_WARNING, "Unrecognized Caller State");
                }
            } else {
                cw_cond_wait(&(that->wakeup), &(that->lock));      /* wait for signal */
                result = icd_caller__unlock(that);
            }
        } else {
            /* TBD - Make paused thread work better.
             *        - Setting last_state when woken up should be smarter.
             *        - Use pthread_cond_wait()
             *        - Use same or different condition variable?
             */
        }
        /* Play nice */
        sched_yield();
    }

    if (icd_caller__has_flag(that, ICD_ORPHAN_FLAG)) {
        icd_bridge__safe_hangup(that);
        if (icd_caller__has_role(that, ICD_AGENT_ROLE)) {
            agent = (icd_agent *) that;
            destroy_icd_agent(&agent);
        } else {
            customer = (icd_customer *) that;
            destroy_icd_customer(&customer);
        }
    }

    return NULL;

}

/* Ensures that moving from one state to another is a valid state change */
/* TBD: This should really use a list of valid state changes so it is extendible. */
icd_status icd_caller__valid_state_change(icd_caller * that, struct icd_caller_states * states)
{
    assert(that != NULL);
    assert(states != NULL);

    switch (states->oldstate) {

    case ICD_CALLER_STATE_CREATED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_INITIALIZED:
        case ICD_CALLER_STATE_DESTROYED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_INITIALIZED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CONFERENCED:
        case ICD_CALLER_STATE_CLEARED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_READY:
        switch (states->newstate) {
        case ICD_CALLER_STATE_CALL_END:
        case ICD_CALLER_STATE_DISTRIBUTING:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_SUSPEND:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_DISTRIBUTING:
        switch (states->newstate) {
        case ICD_CALLER_STATE_CALL_END:        /* we can hangup while in state distributing */
        case ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE:
        case ICD_CALLER_STATE_ASSOCIATE_FAILED:
        case ICD_CALLER_STATE_CHANNEL_FAILED:
        case ICD_CALLER_STATE_BRIDGED:
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_CONFERENCED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE:
        switch (states->newstate) {
        case ICD_CALLER_STATE_CALL_END:        /* we can hangup while in state distributing */
        case ICD_CALLER_STATE_BRIDGED:
        case ICD_CALLER_STATE_BRIDGE_FAILED:
        case ICD_CALLER_STATE_ASSOCIATE_FAILED:
        case ICD_CALLER_STATE_CHANNEL_FAILED:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_CONFERENCED:
        case ICD_CALLER_STATE_SUSPEND:    /* logout of agent while call is waiting */
        case ICD_CALLER_STATE_READY:      /* transfer of customer */
            return ICD_SUCCESS;
        default:
            break;
        }
        break;

    case ICD_CALLER_STATE_CONFERENCED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_CALL_END:
        case ICD_CALLER_STATE_BRIDGE_FAILED:
            return ICD_SUCCESS;
        default:
            break;

        }
        break;

    case ICD_CALLER_STATE_BRIDGED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_BRIDGE_FAILED:
        case ICD_CALLER_STATE_CALL_END:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_READY:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_BRIDGE_FAILED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_SUSPEND:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_CHANNEL_FAILED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_SUSPEND:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_ASSOCIATE_FAILED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_SUSPEND:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_CALL_END:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
        case ICD_CALLER_STATE_SUSPEND:
        case ICD_CALLER_STATE_CONFERENCED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_SUSPEND:
        switch (states->newstate) {
        case ICD_CALLER_STATE_READY:
        case ICD_CALLER_STATE_CLEARED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    case ICD_CALLER_STATE_CLEARED:
        switch (states->newstate) {
        case ICD_CALLER_STATE_DESTROYED:
            return ICD_SUCCESS;
        default:
            break;
        }
        break;
    default:
        break;
    }
    cw_log(LOG_WARNING, "Invalid state change attempted on caller %s: %s to %s\n", icd_caller__get_name(that),
        icd_caller_state_strings[states->oldstate], icd_caller_state_strings[states->newstate]);
    return ICD_ESTATE;
}

int icd_caller__set_chimepos(icd_caller * that, int pos)
{
    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->holdinfo.chimepos = pos;
        icd_caller__unlock(that);
    }

    return that->holdinfo.chimepos;
}

int icd_caller__get_chimepos(icd_caller * that)
{
    return that->holdinfo.chimepos;
}

time_t icd_caller__set_chimenext(icd_caller * that, time_t next)
{
    if (icd_caller__lock(that) == ICD_SUCCESS) {
        that->holdinfo.chimenext = next;
        icd_caller__unlock(that);
    }
    return that->holdinfo.chimenext;
}

time_t icd_caller__get_chimenext(icd_caller * that)
{
    return that->holdinfo.chimenext;
}

void icd_caller__params_to_astheader(icd_caller * that, char *prefix, char *buf, size_t size)
{
    void_hash_table *hash;
    vh_keylist *keys;
    char tmp[256];
    char *val;

    memset(buf, 0, sizeof(buf));

    hash = that->params;
    if (!hash)
        return;

    for (keys = vh_keys(hash); keys; keys = keys->next) {
        val = vh_read(hash, keys->name);
        snprintf(tmp, sizeof(tmp), "%s%s: %s\r\n", prefix, keys->name, val);
        strncat(buf, tmp, size);
    }

    vh_keylist_destroy(&keys);

    return;
}

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

