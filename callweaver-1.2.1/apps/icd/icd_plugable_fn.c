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
 
 /* \file
  * \brief icd_pluggable_fn.c Pluggable function definitions
  */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include <assert.h>
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"
#include "callweaver/icd/icd_fieldset.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_distributor.h"

static int PLUGABLE_FN_ID_POOL = 1;

icd_status init_icd_plugable_fns(icd_plugable_fn * that, char *name, icd_config * data)
{
    char buf[80];

    /* %TC plugable funcs are now allowd on the caller as a list of plugable's by dist name
     *  this allow behavior for a specfic caller, a role of a caller, or by a distribution stratgey
     * in implementing custom strategies with agents belonging to multiple q's the behavior
     * for the same agent during any caller state after 'READY' could change based on the strategy, 
     * so we could of added conditional logic to each agents plugable functions for each strategy
     * but then the code structure would prolly break over time as different strategies were added
     */
    that->id = PLUGABLE_FN_ID_POOL++;
    if (name == NULL) {
        snprintf(buf, sizeof(buf), "Default");
        strncpy(that->name, buf, sizeof(that->name));
    } else {
        strncpy(that->name, name, sizeof(that->name));
    }
    /* do we make dump of this struct as well ? for now its part of caller
       that->dump_fn = icd_config__get_any_value(data, "dump", icd_caller__standard_dump);
       that->dump_fn_extra = icd_config__get_any_value(data, "dump.extra", NULL)    
     */

    /* If we jack this one all the rest are a lot less useful don't try this at home folks -tony */
    that->run_fn = icd_config__get_any_value(data, "run.function", icd_caller__standard_run);

    that->addq_fn = icd_config__get_any_value(data, "add.queue.notify", icd_caller__dummy_notify_hook);
    that->delq_fn = icd_config__get_any_value(data, "remove.queue.notify", icd_caller__delq_notify_hook);
    that->adddist_fn = icd_config__get_any_value(data, "add.dist.notify", icd_caller__dummy_notify_hook);
    that->deldist_fn = icd_config__get_any_value(data, "remove.dist.notify", icd_caller__deldist_notify_hook);
    that->state_fn = icd_config__get_any_value(data, "change.state.notify", icd_caller__dummy_notify_hook);
    that->chan_fn = icd_config__get_any_value(data, "assign.channel.notify", icd_caller__dummy_notify_hook);
    that->link_fn = icd_config__get_any_value(data, "link.caller.notify", icd_caller__dummy_notify_hook);
    that->bridge_fn = icd_config__get_any_value(data, "bridge.caller.notify", icd_caller__dummy_notify_hook);
    that->authn_fn = icd_config__get_any_value(data, "authenticate.notify", icd_caller__dummy_notify_hook);

    that->auth_fn =
        icd_config__get_any_value(data, "authenticate.function", icd_caller__authenticate_always_succeeds);

    that->addq_fn_extra = icd_config__get_any_value(data, "add.queue.extra", NULL);
    that->delq_fn_extra = icd_config__get_any_value(data, "remove.queue.extra", NULL);
    that->adddist_fn_extra = icd_config__get_any_value(data, "add.dist.extra", NULL);
    that->deldist_fn_extra = icd_config__get_any_value(data, "remove.dist.extra", NULL);
    that->state_fn_extra = icd_config__get_any_value(data, "change.state.extra", NULL);
    that->chan_fn_extra = icd_config__get_any_value(data, "assign.channel.extra", NULL);
    that->link_fn_extra = icd_config__get_any_value(data, "link.caller.extra", NULL);
    that->bridge_fn_extra = icd_config__get_any_value(data, "bridge.caller.extra", NULL);
    that->authn_fn_extra = icd_config__get_any_value(data, "authenticate.notify.extra", NULL);

    /* Setting the state functions to appropriate functions. */
    that->state_ready_fn =
        icd_config__get_any_value(data, "state.ready.function", icd_caller__standard_state_ready);
    that->state_ready_fn_extra = icd_config__get_any_value(data, "state.ready.extra", NULL);

    that->start_waiting_fn =
        icd_config__get_any_value(data, "start.waiting.function", icd_caller__standard_start_waiting);
    that->stop_waiting_fn =
        icd_config__get_any_value(data, "stop.waiting.function", icd_caller__standard_stop_waiting);

    that->state_distribute_fn =
        icd_config__get_any_value(data, "state.distribute.function", icd_caller__standard_state_distribute);
    that->state_distribute_fn_extra = icd_config__get_any_value(data, "state.distribute.extra", NULL);

    that->state_get_channels_fn =
        icd_config__get_any_value(data, "state.get.channels.function", icd_caller__standard_state_get_channels);
    that->state_get_channels_fn_extra = icd_config__get_any_value(data, "state.get.channels.extra", NULL);

    that->state_bridged_fn =
        icd_config__get_any_value(data, "state.bridged.function", icd_caller__standard_state_bridged);
    that->state_bridged_fn_extra = icd_config__get_any_value(data, "state.bridged.extra", NULL);

    /*#_Start role & strategy specific icd by default sets this in the agent or customer */
    that->state_channel_failed_fn =
        icd_config__get_any_value(data, "state.channel.failed.function", icd_caller__standard_state_channel_failed);
    that->state_channel_failed_fn_extra = icd_config__get_any_value(data, "state.channel.failed.extra", 0);

    that->state_bridge_failed_fn =
        icd_config__get_any_value(data, "state.bridge.failed.function", icd_caller__standard_state_bridge_failed);
    that->state_bridge_failed_fn_extra = icd_config__get_any_value(data, "state.bridge.failed.extra", 0);

    that->state_associate_failed_fn =
        icd_config__get_any_value(data, "state.associate.failed.function",
        icd_caller__standard_state_associate_failed);
    that->state_associate_failed_fn_extra = icd_config__get_any_value(data, "state.associate.failed.extra", 0);

    that->state_call_end_fn =
        icd_config__get_any_value(data, "state.call.end.function", icd_caller__standard_state_call_end);
    that->state_call_end_fn_extra = icd_config__get_any_value(data, "state.call.end.extra", NULL);

    that->state_suspend_fn =
        icd_config__get_any_value(data, "state.suspend.function", icd_caller__standard_state_suspend);
    that->state_suspend_fn_extra = icd_config__get_any_value(data, "state.suspend.extra", NULL);

    that->cleanup_caller_fn =
        icd_config__get_any_value(data, "cleanup.caller.function", icd_caller__standard_cleanup_caller);

    /*#_End Role/Strategy specfic func set by icd in agent or customer */

    that->state_conference_fn =
        icd_config__get_any_value(data, "state.conference.function", icd_caller__standard_state_conference);

    that->prepare_caller_fn =
        icd_config__get_any_value(data, "prepare.caller.function", icd_caller__standard_prepare_caller);
    that->launch_caller_fn =
        icd_config__get_any_value(data, "launch.caller.function", icd_caller__standard_launch_caller);

    return ICD_SUCCESS;
}

/* we are going to create plugable funcs in the icd_plugable_fn_list for each
 * unique distributor that is assigned to the queue the caller will participate in
 * typically a customer caller will belong to only 1 q at a time, an agent many 
*/
void icd_plugable__create_standard_fns(icd_plugable_fn_list * that, icd_config * data)
{
    char *dist_name;
    char *queuelist;
    char *currqueue;
    icd_queue *queue;
    icd_plugable_fn *plugable_fns;

    assert(that != NULL);
    assert(data != NULL);

    queuelist = icd_config__get_value(data, "queues");
    if (queuelist == NULL)
        queuelist = icd_config__get_value(data, "queue");
    cw_log(LOG_NOTICE, "QueueLIST[%s]\n", queuelist);
    while (queuelist != NULL) {
        /* This has been normalized to use only a '|' or ',' to separate queue names */
        currqueue = strsep(&queuelist, "|,");
        if (currqueue != NULL && strlen(currqueue) > 0) {
            queue = icd_fieldset__get_value(queues, currqueue);
            if (queue != NULL) {
                dist_name = vh_read(icd_distributor__get_params(icd_queue__get_distributor(queue)), "dist");
                cw_log(LOG_NOTICE, "CurrQueue-distname[%s]\n", dist_name);
                plugable_fns = icd_plugable_fn_list__fetch_fns(that, dist_name);
                if (plugable_fns == NULL) {
                    plugable_fns = create_icd_plugable_fns(data, dist_name);
                    if (plugable_fns != NULL) {
                        icd_plugable_fn_list__add_fns(that, plugable_fns);
                        cw_log(LOG_NOTICE, "Add Plugable funcs for Callers dist[%s]\n", dist_name);
                    } else {
                        cw_log(LOG_NOTICE, "Create_icd_plugable_fns returned null [%s]\n", dist_name);
                    }
                } else {
                    cw_log(LOG_NOTICE, "icd_plugable_fn_list__fetch_fns think it found [%s]\n", dist_name);
                }
            }
        }

    }
    if (icd_plugable_fn_list_count(that) == 0) {
        plugable_fns = create_icd_plugable_fns(data, "default");
        if (plugable_fns != NULL) {
            icd_plugable_fn_list__add_fns(that, plugable_fns);
            cw_log(LOG_NOTICE, "No Dists found Adding Plugable funcs for Callers dist[Default]\n");
        }
    }

}
icd_plugable_fn *create_icd_plugable_fns(icd_config * data, char *name)
{
    icd_plugable_fn *plugable_fns;
    icd_status result;

    /* make a new plugable_fns from scratch */
    ICD_MALLOC(plugable_fns, sizeof(icd_plugable_fn));
    if (plugable_fns == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD plugable fns\n");
        return NULL;
    }
    result = init_icd_plugable_fns(plugable_fns, name, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(plugable_fns);
        return NULL;
    }
    plugable_fns->allocated = 1;
    return plugable_fns;
}

icd_status destroy_icd_plugable_fn(icd_plugable_fn ** plugable_fnsp)
{
    int clear_result;

    assert(plugable_fnsp != NULL);
    assert((*plugable_fnsp) != NULL);

    /* Next, clear the  object of any function ptrs. */
    if ((clear_result = icd_plugable__clear_fns(*plugable_fnsp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Finally, destroy the object itself if from the heap */
    if ((*plugable_fnsp)->allocated) {
        ICD_FREE((*plugable_fnsp));
        *plugable_fnsp = NULL;
    }
    return ICD_SUCCESS;
}

icd_status icd_plugable__set_fn(void *that, icd_plugable_fn * (*get_plugable_fn) (icd_caller *))
{

    that = get_plugable_fn;
    return ICD_SUCCESS;

}

/* Get the name string being used to identify this caller. */
char *icd_plugable__get_name(icd_plugable_fn * that)
{
    assert(that != NULL);

    if (that->name == NULL) {
        return "";
    }
    return that->name;
}

icd_status icd_plugable__clear_fns(icd_plugable_fn * that)
{

    that->addq_fn = NULL;
    that->delq_fn = NULL;
    that->adddist_fn = NULL;
    that->deldist_fn = NULL;
    that->state_fn = NULL;
    that->chan_fn = NULL;
    that->link_fn = NULL;
    that->bridge_fn = NULL;
    that->authn_fn = NULL;

    /* Setting the state functions to appropriate functions. */
    that->state_ready_fn = NULL;
    that->state_distribute_fn = NULL;
    that->state_get_channels_fn = NULL;
    that->state_bridged_fn = NULL;
    that->state_bridge_failed_fn = NULL;
    that->state_channel_failed_fn = NULL;
    that->state_associate_failed_fn = NULL;
    that->state_call_end_fn = NULL;
    that->state_suspend_fn = NULL;
    that->state_conference_fn = NULL;

    that->prepare_caller_fn = NULL;
    that->cleanup_caller_fn = NULL;
    that->launch_caller_fn = NULL;

    that->addq_fn_extra = NULL;
    that->delq_fn_extra = NULL;
    that->adddist_fn_extra = NULL;
    that->deldist_fn_extra = NULL;
    that->state_fn_extra = NULL;
    that->chan_fn_extra = NULL;
    that->link_fn_extra = NULL;
    that->bridge_fn_extra = NULL;
    that->authn_fn_extra = NULL;
    that->state_ready_fn_extra = NULL;
    that->state_distribute_fn_extra = NULL;
    that->state_get_channels_fn_extra = NULL;
    that->state_bridged_fn_extra = NULL;

    that->state_bridge_failed_fn_extra = NULL;
    that->state_channel_failed_fn_extra = NULL;
    that->state_associate_failed_fn_extra = NULL;
    that->state_call_end_fn_extra = NULL;
    that->state_suspend_fn_extra = NULL;

    return ICD_SUCCESS;
}

/***** Callback Setters *****/

/* Sets the authentication function to a particular function. */
icd_status icd_plugable__set_authenticate_fn(icd_plugable_fn * that, int (*auth_fn) (icd_caller *, void *))
{
    assert(that != NULL);

    that->auth_fn = auth_fn;
    return ICD_SUCCESS;
}

/* Sets the added to queue notification hook to a particular function. */
icd_status icd_plugable__set_added_to_queue_notify_fn(icd_plugable_fn * that, int (*addq_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->addq_fn = addq_fn;
    that->addq_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the removed from queue notification hook to a particular function. */
icd_status icd_plugable__set_removed_from_queue_notify_fn(icd_plugable_fn * that, int (*delq_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->delq_fn = delq_fn;
    that->delq_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the added to distributor notification hook to a particular function. */
icd_status icd_plugable__set_added_to_distributor_notify_fn(icd_plugable_fn * that,
    int (*adddist_fn) (icd_event * that, void *extra), void *extra)
{
    assert(that != NULL);

    that->adddist_fn = adddist_fn;
    that->adddist_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the removed from distributor notification hook to a particular function. */
icd_status icd_plugable__set_removed_from_distributor_notify_fn(icd_plugable_fn * that,
    int (*deldist_fn) (icd_event * that, void *extra), void *extra)
{
    assert(that != NULL);

    that->deldist_fn = deldist_fn;
    that->deldist_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the call_state_change notification hook to a particular function. */
icd_status icd_plugable__set_state_change_notify_fn(icd_plugable_fn * that, int (*state_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_fn = state_fn;
    that->state_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the channel assigned notification hook to a particular function. */
icd_status icd_plugable__set_channel_assigned_notify_fn(icd_plugable_fn * that, int (*chan_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->chan_fn = chan_fn;
    that->chan_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the linked to caller notification hook to a particular function. */
icd_status icd_plugable__set_linked_notify_fn(icd_plugable_fn * that, int (*link_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->link_fn = link_fn;
    that->link_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the bridged notification hook to a particular function. */
icd_status icd_plugable__set_bridged_notify_fn(icd_plugable_fn * that, int (*bridge_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->bridge_fn = bridge_fn;
    that->bridge_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Sets the function to be called once authentication has successfully completed. */
icd_status icd_plugable__set_authenticate_notify_fn(icd_plugable_fn * that, int (*authn_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->authn_fn = authn_fn;
    that->authn_fn_extra = extra;

    return ICD_SUCCESS;
}

/*  */
icd_status icd_plugable__set_state_ready_fn(icd_plugable_fn * that, int (*ready_fn) (icd_event * that, void *extra),
    void *extra)
{
    assert(that != NULL);

    that->state_ready_fn = ready_fn;
    that->state_ready_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_distribute_fn(icd_plugable_fn * that, int (*dist_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_distribute_fn = dist_fn;
    that->state_distribute_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_get_channels_fn(icd_plugable_fn * that, int (*chan_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_get_channels_fn = chan_fn;
    that->state_get_channels_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_bridged_fn(icd_plugable_fn * that, int (*bridged_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_bridged_fn = bridged_fn;
    that->state_bridged_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_conference_fn(icd_plugable_fn * that,
    int (*state_conference_fn) (icd_event * that, void *extra), void *extra)
{
    assert(that != NULL);

    that->state_conference_fn = state_conference_fn;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_call_end_fn(icd_plugable_fn * that, int (*end_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_call_end_fn = end_fn;
    that->state_call_end_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_bridge_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_bridge_failed_fn = fail_fn;
    that->state_bridge_failed_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_channel_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_channel_failed_fn = fail_fn;
    that->state_channel_failed_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_associate_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_associate_failed_fn = fail_fn;
    that->state_associate_failed_fn_extra = extra;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_state_suspend_fn(icd_plugable_fn * that, int (*suspend_fn) (icd_event * that,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_suspend_fn = suspend_fn;
    that->state_suspend_fn_extra = extra;

    return ICD_SUCCESS;
}

/* Set up the pluggable actions from within the caller thread */
icd_status icd_plugable__set_start_waiting_fn(icd_plugable_fn * that, int (*start_fn) (icd_caller * caller))
{
    assert(that != NULL);

    that->start_waiting_fn = start_fn;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_stop_waiting_fn(icd_plugable_fn * that, int (*stop_fn) (icd_caller * caller))
{
    assert(that != NULL);

    that->stop_waiting_fn = stop_fn;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_prepare_caller_fn(icd_plugable_fn * that,
    icd_status(*prepare_fn) (icd_caller * caller))
{
    assert(that != NULL);

    that->prepare_caller_fn = prepare_fn;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_cleanup_caller_fn(icd_plugable_fn * that,
    icd_status(*cleanup_fn) (icd_caller * caller))
{
    assert(that != NULL);

    that->cleanup_caller_fn = cleanup_fn;

    return ICD_SUCCESS;
}

icd_status icd_plugable__set_launch_caller_fn(icd_plugable_fn * that, icd_status(*launch_fn) (icd_caller * caller))
{
    assert(that != NULL);

    that->launch_caller_fn = launch_fn;

    return ICD_SUCCESS;
}

/*
icd_plugable_fn *icd_plugable_get_fns(icd_caller *that,) {
    assert(that != NULL);
    
    return icd_caller__get_plugable_fns(that);

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

