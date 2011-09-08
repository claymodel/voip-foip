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
 * \brief icd_agent.c - an agent or device
 * 
 * The icd_agent module represents an internal entity that is going to deal with a
 * customer call as represented by icd_customer. That entity might be a person at
 * the end of a channel, or it might be a program or device.
 *
 * This module is for the most part implemented as icd_caller, which
 * icd_agent is castable as. Only things specific to the agent side
 * of a call will be here.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_caller_private.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"

//static icd_module module_id = ICD_AGENT;

static icd_plugable_fn icd_agent_plugable_fns;  /*static of standard agent specific func ptrs */

struct icd_agent {
    icd_caller caller;
    int simo_call_count;        /* current number of calls in progress via this obj */
    int simo_max_calls;         /* how many calls at once this caller obj can take */
    char agent_id[50];
    char agent_pass[50];
    icd_memory *memory;
    int allocated;
};

/* called from icd_caller_load_module */
icd_status icd_agent_load_module(icd_config * data)
{
    icd_plugable_fn *plugable_fns;
    char buf[80];
    icd_status result;

    plugable_fns = &icd_agent_plugable_fns;
    ICD_MEMSET_ZERO(plugable_fns, sizeof(plugable_fns));

    /*set function pointers in static struc icd_agent_plugable_fns for Agent Caller plugable functions 
     * we have a choice here of how to code the standard actions
     * by default install just suspend, call_end, and cleanup_caller since they are role specfic
     */
    snprintf(buf, sizeof(buf), "AgentDefault");
    result = init_icd_plugable_fns(plugable_fns, buf, data);    /* set all func ptrs to icd_caller_standard[] */
    result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_agent__standard_state_call_end, NULL);
    result = icd_plugable__set_state_suspend_fn(plugable_fns, icd_agent__standard_state_suspend, NULL);
    result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_agent__standard_cleanup_caller);

    plugable_fns->allocated = 1;

    return ICD_SUCCESS;
}

/***** Init - Destroyer *****/

/* Create an agent. data is a config obj */
icd_agent *create_icd_agent(icd_config * data)
{
    icd_agent *agent;
    icd_status result;

    /* make a new list from scratch */
    ICD_MALLOC(agent, sizeof(icd_agent));

    if (agent == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Agent\n");
        return NULL;
    }
    ((icd_caller *) agent)->state = ICD_CALLER_STATE_CREATED;
    result = init_icd_agent(agent, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(agent);
    }

    agent->allocated = 1;
    return agent;
}

/* Destroy an agent, freeing its memory and cleaning up after it. */
icd_status destroy_icd_agent(icd_agent ** agentp)
{
    int clear_result;

    assert(agentp != NULL);
    assert((*agentp) != NULL);

    if ((*agentp)->caller.params) {
        /*      cw_log(LOG_WARNING,"caller destroyer freeing hash memory\n"); */
        vh_destroy(&(*agentp)->caller.params);
    }

    if ((clear_result = icd_caller__clear((icd_caller *) (*agentp))) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the caller if from the heap */
    if ((*agentp)->allocated) {
        ((icd_caller *) (*agentp))->state = ICD_CALLER_STATE_DESTROYED;
        ICD_FREE((*agentp));
        *agentp = NULL;
    }
    return ICD_SUCCESS;
}

/* Configure an agent. data is a config obj. */
icd_status init_icd_agent(icd_agent * that, icd_config * data)
{
    icd_status(*init) (icd_agent * that, icd_config * data);
    icd_caller *caller = (icd_caller *) that;

    icd_status result;
    char *moh;

    assert(that != NULL);
    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_agent));
    /* Initialize base class */
    result = init_icd_caller(caller, data);
    if (result != ICD_SUCCESS) {
        return result;
    }

    that->simo_max_calls = -1;
    icd_caller__add_role(caller, ICD_AGENT_ROLE);
    moh = icd_caller__get_moh(caller);
    if (moh == NULL) {
        icd_caller__set_moh(caller, "default");
    }

    /* Set plugable function pointer for Agent Caller plugable functions */
    icd_caller__set_plugable_fn_ptr(caller, icd_agent_get_plugable_fns);

    /*
     * if we wanted to customized for a specific instance of a agent caller we can add
     * a pointer in the caller->plugable_fns_list for a specific dist use the family
     * of helpers in the plugable_fns_list.c otherwise we set the function ptrs to 
     * the staic struc declared locally here in icd_agent_plugable_fns
     plugable_fns = (icd_plugable_fn *)icd_list__peek((icd_list *)caller->plugable_fns_list);
     icd_list_iterator *iter;

     iter = icd_list__get_iterator((icd_list *)(caller->plugable_fns_list));
     while (icd_list_iterator__has_more(iter)) {
     plugable_fns = (icd_plugable_fn *)icd_list_iterator__next(iter);

     result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_agent__standard_state_call_end, NULL);
     result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_agent__standard_cleanup_caller);
     }
     destroy_icd_list_iterator(&iter);

     */

    /* fix me This is supposed to be the agent_init in a loadable module 
     *  icd_config_registry__register_ptr(registry, "agents.init", "myModule",
     *  icd_module__init_agent);
     *  then we run 
     * icd_config__get_any_value(data, "agent.init.", myModule_init_agent_)
     */
    init = (icd_status(*)(icd_agent * that, icd_config * data))
        icd_config__get_value(data, "agents.init");
    if (init != NULL) {
        cw_verbose(VERBOSE_PREFIX_1 "Agent plugable init for [%s] \n", icd_caller__get_name(caller));
        return init(that, data);
    }

    return ICD_SUCCESS;
}

/* Clear an agent so that it needs to be reinitialized to be reused. */
icd_status icd_agent__clear(icd_agent * that)
{
    assert(that != NULL);

    return icd_caller__clear((icd_caller *) that);
}

/***** Locking *****/

/* Lock the agent */
icd_status icd_agent__lock(icd_agent * that)
{
    assert(that != NULL);

    return icd_caller__lock((icd_caller *) that);
}

/* Unlock the agent */
icd_status icd_agent__unlock(icd_agent * that)
{
    assert(that != NULL);

    return icd_caller__unlock((icd_caller *) that);
}

/**** Listener functions ****/

icd_status icd_agent__add_listener(icd_agent * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    return icd_caller__add_listener((icd_caller *) that, listener, lstn_fn, extra);
}

icd_status icd_agent__remove_listener(icd_agent * that, void *listener)
{
    assert(that != NULL);

    return icd_caller__remove_listener((icd_caller *) that, listener);
}

/*****Getters and Setters *****/

/* Set the number of calls received. No need for locking as we are blowing away the value. */
icd_status icd_agent__set_call_count(icd_agent * that, int count)
{
    assert(that != NULL);

    that->simo_call_count = count;
    return ICD_SUCCESS;
}

/* Returns the number of calls this agent has received */
int icd_agent__get_call_count(icd_agent * that)
{
    assert(that != NULL);

    return that->simo_call_count;
}

/* Increment the number of calls the agent has received. Lock required. */
icd_status icd_agent__increment_call_count(icd_agent * that)
{
    assert(that != NULL);

    if (icd_agent__lock(that)) {
        that->simo_call_count++;
        icd_agent__unlock(that);
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/* Reduce the number of calls, because we incremented in error (?). */
icd_status icd_agent__decrement_call_count(icd_agent * that)
{
    assert(that != NULL);

    if (that->simo_call_count > 0) {
        if (icd_agent__lock(that) == ICD_SUCCESS) {
            that->simo_call_count--;
            icd_agent__unlock(that);
            return ICD_SUCCESS;
        }
        return ICD_ELOCK;
    }
    return ICD_ERESOURCE;
}

icd_plugable_fn *icd_agent_get_plugable_fns(icd_caller * that)
{
    icd_plugable_fn *plugable_fns = NULL;
    char *dist_name = NULL;
    icd_distributor *dist = NULL;
    icd_member *member;

    assert(that != NULL);
    member = icd_caller__get_active_member(that);
    if (member == NULL) {
        /* always return our standard plugable interface when no distributor */
        plugable_fns = &icd_agent_plugable_fns;

    } else {
        dist = icd_member__get_distributor(member);
	if (dist == NULL){
        	plugable_fns = &icd_agent_plugable_fns;
	}	
        dist_name = vh_read(icd_distributor__get_params(dist), "dist");
        /* check if there is a caller specific plugable_fn in plugable_fns_list for this dist */
        /* plugable_fns = icd_plugable_fn_list__fetch_fns(that->plugable_fns_list, dist_name); */
        if (plugable_fns == NULL) {
            /* if we fetched the caller specific plugable_fn_list for this dist and found 
             * no caller specific plugable defined then we try dist specfic else
             * we just go with our static local interface for agent 
             */
            plugable_fns = icd_distributor__get_plugable_fn(dist, that);
            if (plugable_fns == NULL)
                plugable_fns = &icd_agent_plugable_fns;
        }
    }

    if (plugable_fns == NULL) {
        if (icd_verbose > 4)
            cw_log(LOG_NOTICE, "Agent Caller %d [%s] has no plugable fn aborting ala crash\n",
                icd_caller__get_id(that), icd_caller__get_name(that));
    } else {
        if (icd_verbose > 4)
            cw_log(LOG_NOTICE,
                "\nAgent Caller %d [%s] using icd_agent_get_plugable_fns[%s] ready_fn[%p] for Dist[%s]\n",
                icd_caller__get_id(that), icd_caller__get_name(that), icd_plugable__get_name(plugable_fns),
                plugable_fns->state_ready_fn, dist_name);
    }
    assert(plugable_fns != NULL);
    return plugable_fns;

}

/* Standard functions for reacting to an AGENT state change */

int icd_agent__standard_state_ready(icd_event * event, void *extra)
{
    icd_caller *that;

    //icd_member *member;
    //icd_list_iterator *iter;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);

    return 0;

}

/* Standard function for reacting to the call end state */
int icd_agent__standard_state_call_end(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_plugable_fn *icd_run;
    icd_caller *associate;
    icd_status result;
    icd_caller_state state;

    /* char *action; */
    char *wait;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

    /* Actions for when the call is terminated
     *   - go into ready state
     *   - go into suspend state
     *   - destroy (we need cleanup thread?)
     */

    /* Let our associates know. This should probably be a separate function
       call so that the associate can do its own cleanup, but for now we are
       going to use a simple state change to bridge_end if not in conference
       call.
       icd_caller__set_state_on_associations(that,ICD_CALLER_STATE_CALL_END);
       icd_caller__set_active_member(that, NULL);
     */
    /*things to do with agents at call end, this is not for use for Fail states */
    if (that->associations != NULL){
        associate = (icd_caller *) icd_list__pop((icd_list *) that->associations);
        while(associate != NULL) {
             icd_caller_list__remove_caller_by_element(associate->associations, that);
	     state = icd_caller__get_state(associate);
	     if((state == ICD_CALLER_STATE_CONFERENCED) || (state == ICD_CALLER_STATE_BRIDGED)){ 
           		result = icd_caller__set_state(associate, ICD_CALLER_STATE_CALL_END);
	     }   
	     if(state == ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE){ 
         		result = icd_caller__set_state(associate, ICD_CALLER_STATE_ASSOCIATE_FAILED);
             }    
    	associate = (icd_caller *) icd_list__pop((icd_list *) that->associations);
      	}
    }  
    icd_caller__set_active_member(that, NULL);
    if (icd_caller__get_onhook(that) == 0) {
        /* Off Hook agent hung up */
        if ((that->chan == NULL || (that->chan != NULL && that->chan->_softhangup))) {
            icd_caller__set_state(that, ICD_CALLER_STATE_SUSPEND);
            return 0;
        }
    }
    /* so we are now either a (onHook with or w/o channel) or (OffHook with a channel) */
    if (icd_debug)
        cw_log(LOG_WARNING, "Caller id[%d] [%s] Set Push Back\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    icd_caller__set_pushback(that);

    wait = vh_read(that->params, "wrapup");
    if (wait != NULL) {
        icd_caller__set_state(that, ICD_CALLER_STATE_SUSPEND);
        return 0;
    }
    icd_run = that->get_plugable_fn(that);
    icd_run->cleanup_caller_fn(that);   /* default is icd_agent__standard_cleanup_caller(that)  READY or CLEAR */

    return 0;

}

/*! Standard function for reacting to the suspend state */
/*!
 * This is the standard function for controlling what happens when we
 * enter the SUSPEND state in icd_caller.
 *
 * It has a variety of actions it can perform, including:
 *    1) Just returning back to the __run() loop (where it will be suspended
 *       until the state changes again)
 *    2) Sleeping for a given period of time, going to the READY state
 *       when done.
 *    3) Listen for DTMF (either a specific key or any key) and go to the
 *       READY state when the key is heard or a timeout occurs.
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
 *
 */
int icd_agent__standard_state_suspend(icd_event * event, void *extra)
{
    icd_caller *that;
    char *action;
    char *entertain;
    char *wakeup;
    char *wait;
    int waittime;
    char res;
    char *pos = NULL;
    int cleanup_required = 0;
    cw_channel * chan;
    
    assert(event != NULL);
    that = icd_event__get_source(event);
    assert(that != NULL);
    assert(that->params != NULL);

    that->thread_state = ICD_THREAD_STATE_FINISHED;   
    icd_caller__stop_waiting(that);
    icd_caller__clear_suspend( that);
    /* OffHook agent with PBX thread hungup */
//    icd_bridge__safe_hangup(that);
    chan = icd_caller__get_channel(that);
    if (/*icd_caller__get_onhook(that) == 0 && */ chan){
//        cw_stopstream(chan);
//        cw_generator_deactivate(chan);
//        cw_clear_flag(chan ,  CW_FLAG_BLOCKING);
//        cw_softhangup(chan ,  CW_SOFTHANGUP_EXPLICIT);
//        cw_hangup(chan );
//        icd_caller__set_channel(that, NULL);
     };	 
/*    
    if (that->chan != NULL) {
        cw_clear_flag(that->chan ,  CW_FLAG_BLOCKING);
        cw_softhangup(that->chan ,  CW_SOFTHANGUP_EXPLICIT);
        cw_hangup(that->chan );
        icd_caller__set_channel(that, NULL);
    }
*/
    return 0; //pf
    
    
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
        icd_plugable_fn *icd_run;
        sleep(waittime);
        icd_run = that->get_plugable_fn(that);
        icd_run->cleanup_caller_fn(that);   /* default is icd_agent__standard_cleanup_caller(that)  READY or CLEAR */
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
    return 0;
}

/* Standard function for cleaning up an agent caller after a bridge ends or fails */
icd_status icd_agent__standard_cleanup_caller(icd_caller * that)
{

    if (icd_caller__get_pushback(that)) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller %d [%s] has agent role with push back trying to add it to the queue \n",
                icd_caller__get_id(that), icd_caller__get_name(that));
        if (icd_caller__get_onhook(that)) {
            icd_bridge__safe_hangup(that);
            /*%TC wait for hangup to reset, a zap chan may have come from fail state not call end no wrapuptime
             * so we go back on dist & try dial & zap rets busy in endless loop :(, maybe we hack 
             * icd_bridge_dial_callweaver_channel to wait 1sec & retry if a Zap channel rets busy ?
             */
            sleep(1);
        }
        icd_caller__set_state(that, ICD_CALLER_STATE_READY);
        /* What about any channels we created and associated with this agent? */
//      Stanard_state_ready function do this task	
//        icd_caller__return_to_distributors(that);
//        icd_caller__reset_pushback(that);
    } else {
        if (icd_debug)
            cw_log(LOG_DEBUG, "Caller %d [%s] has agent role with no pushback needed, exit icd thread finished \n",
                icd_caller__get_id(that), icd_caller__get_name(that));
        icd_bridge__safe_hangup(that);
        that->thread_state = ICD_THREAD_STATE_FINISHED;
	
    }
    return ICD_SUCCESS;

}

/* Standard function for reacting to the bridge failed state */
int icd_agent__standard_state_bridge_failed(icd_event * event, void *extra)
{
    icd_caller *that = NULL;
    int *value;

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
    icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_BRIDGE_FAILED);

    /* increment the count of the number of failures */
    value = (int *) extra;
    (*value)++;

    that->require_pushback = 1;
    icd_caller__set_state(that, ICD_CALLER_STATE_READY);

    return 0;
    //return icd_caller__pushback_and_ready_on_fail(event, extra);
}

icd_agent *icd_agent__generate_queued_call(char *id, char *queuename, char *dialstr, char *vars, void *plug)
{
    icd_agent *agent = NULL;
    icd_config *config;
    void_hash_table *arghash;
    icd_queue *queue;
    char buf[128], agentname[80];
    char key[30];

    if (!queuename || !dialstr) {
        cw_log(LOG_ERROR, "Invalid Parameters\n");
        return NULL;
    }
    strncpy(buf, dialstr, sizeof(buf));

    queue = (icd_queue *) icd_fieldset__get_value(queues, queuename);
    if (queue == NULL) {
        cw_log(LOG_ERROR, "AGENT FAILURE! Agent assigned to undefined Queue [%s]\n", queuename);
        return NULL;
    }
    arghash = vh_init("args");

    if (vars)
        vh_carve_data(arghash, vars, ':');
    config = create_icd_config(app_icd_config_registry, "agent_config");
    icd_config__set_raw(config, "params", arghash);

    agent = create_icd_agent(config);
    destroy_icd_config(&config);
    if (agent) {
        if (plug)
            icd_caller__set_plugable_fn_ptr((icd_caller *) agent, plug);

        icd_caller__add_flag((icd_caller *) agent, ICD_ORPHAN_FLAG);
        sprintf(agentname, "generated_%s:%d", queuename, icd_caller__get_id((icd_caller *) agent));
        icd_caller__set_name((icd_caller *) agent, agentname);
        icd_caller__set_dynamic((icd_caller *) agent, 1);

        icd_caller__set_param_string((icd_caller *) agent, "channel", buf);

        icd_caller__set_channel_string((icd_caller *) agent, buf);
        icd_caller__add_role((icd_caller *) agent, ICD_LOOPER_ROLE);
        icd_caller__add_to_queue((icd_caller *) agent, queue);

        if (!strcasecmp(id, "auto")) {
            snprintf(key, 30, "%d", icd_caller__get_id((icd_caller *) agent));
            icd_caller__set_param_string((icd_caller *) agent, "identifier", key);
        } else
            icd_caller__set_param_string((icd_caller *) agent, "identifier", id);

        icd_caller__set_param_string((icd_caller *) agent, "queuename", queuename);
        icd_caller__loop((icd_caller *) agent, 1);
    }

    return agent;
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

