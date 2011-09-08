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
 * \brief icd_customer.c - a customer call
 * 
 * The icd_customer module represents a call that needs to be serviced by an
 * internal entity, aka an icd_agent. Although typically this will actually
 * be a customer of the company, it could be any call that needs servicing.
 *
 * This module is for the most part implemented as icd_caller, which
 * icd_customer is castable as. Only things specific to the customer side
 * of a call will be here.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_private.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"

//static icd_module module_id = ICD_CUSTOMER;

static icd_plugable_fn icd_customer_plugable_fns;       /*static of standard customer specific func ptrs */

struct icd_customer {
    icd_caller caller;
    int id;
    icd_memory *memory;
    int allocated;
};

/* called from icd_caller_load_module */
icd_status icd_customer_load_module(icd_config * data)
{
    icd_plugable_fn *plugable_fns;
    char buf[80];
    icd_status result;

    plugable_fns = &icd_customer_plugable_fns;
    ICD_MEMSET_ZERO(plugable_fns, sizeof(plugable_fns));

    /*set function pointers in static struc icd_agent_plugable_fns for customer Caller plugable functions 
     * we have a choice here of how to code the standard actions
     * by default install just call_end and cleanup_caller since they are role specfic
     */
    snprintf(buf, sizeof(buf), "DefaultCustomer");
    result = init_icd_plugable_fns(plugable_fns, buf, data);    /* set all func ptrs to icd_call_standard[] */
    result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_customer__standard_state_call_end, NULL);
    result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_customer__standard_cleanup_caller);
    plugable_fns->allocated = 1;

    return ICD_SUCCESS;
}

/***** Init - Destroyer *****/

/* Create an customer. data is a config obj */
icd_customer *create_icd_customer(icd_config * data)
{
    icd_customer *customer;
    icd_status result;

    /* make a new customer from scratch */

    ICD_MALLOC(customer, sizeof(icd_customer));

    if (customer == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Customer\n");
        return NULL;
    }
    customer->allocated = 1;
    ((icd_caller *) customer)->state = ICD_CALLER_STATE_CREATED;
    result = init_icd_customer(customer, data);
    if (result != ICD_SUCCESS) {
    }

    return customer;
}

/* Destroy an customer, freeing its memory and cleaning up after it. */
icd_status destroy_icd_customer(icd_customer ** customerp)
{
    int clear_result;

    assert(customerp != NULL);
    assert((*customerp) != NULL);

    if ((*customerp)->caller.params && (*customerp)->caller.params->allocated) {
        /*      cw_log(LOG_WARNING,"caller destroyer freeing hash memory\n"); */
        vh_destroy(&(*customerp)->caller.params);
    }

    if ((clear_result = icd_customer__clear(*customerp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the customer if from the heap */
    if ((*customerp)->allocated) {
        ((icd_caller *) (*customerp))->state = ICD_CALLER_STATE_DESTROYED;
        ICD_FREE((*customerp));
        *customerp = NULL;
    }
    return ICD_SUCCESS;
}

/* Configure an customer. data is a config obj. */
icd_status init_icd_customer(icd_customer * that, icd_config * data)
{
    icd_status(*init) (icd_customer * that, icd_config * data);
    char *moh;
    icd_caller *caller = (icd_caller *) that;
    icd_status result;

    assert(that != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_caller));
    /* Initialize base class */
    result = init_icd_caller((icd_caller *) that, data);
    if (result != ICD_SUCCESS) {
        return result;
    }

    icd_caller__add_role((icd_caller *) that, ICD_CUSTOMER_ROLE);
    moh = icd_caller__get_moh((icd_caller *) that);
    if (moh == NULL) {
        icd_caller__set_moh((icd_caller *) that, "default");
    }
    /* Set plugable function pointer for Agent Caller plugable functions */
    icd_caller__set_plugable_fn_ptr(caller, icd_customer_get_plugable_fns);
    /*
     * if we wanted to customized for a specific instance of a agent caller we can add
     * a pointer in the caller->plugable_fns_list for a speific dist use the family
     * of helpers in the plugable_fns_list.c otherwise we set the function ptrs to 
     * the staic struc declared locally here in icd_agent_plugable_fns
     plugable_fns = (icd_plugable_fn *)icd_list__peek((icd_list *)caller->plugable_fns_list);
     icd_list_iterator *iter;

     iter = icd_list__get_iterator((icd_list *)(caller->plugable_fns_list));
     while (icd_list_iterator__has_more(iter)) {
     plugable_fns = (icd_plugable_fn *)icd_list_iterator__next(iter);

     result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_customer__standard_state_call_end, NULL);
     result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_customer__standard_cleanup_caller);
     }
     destroy_icd_list_iterator(&iter);

     */

    /* fix me This is supposed to be the agent_init in a loadable module 
     *  icd_config_registry__register_ptr(registry, "customer.init", "myModule",
     *  icd_module__init_agent);
     *  then we run 
     * icd_config__get_any_value(data, "customer.init.", myModule_init_agent_)
     */

    init = (icd_status(*)(icd_customer * that, icd_config * data))
        icd_config__get_value(data, "customers.init");
    if (init != NULL) {
        cw_verbose(VERBOSE_PREFIX_1 "Customer plugable init for [%s] \n", icd_caller__get_name(caller));
        return init(that, data);
    }

    return ICD_SUCCESS;
}

/* Clear a customer object. */
icd_status icd_customer__clear(icd_customer * that)
{
    assert(that != NULL);

    return icd_caller__clear((icd_caller *) that);
}

/***** Locking *****/

/* Lock the customer */
icd_status icd_customer__lock(icd_customer * that)
{
    assert(that != NULL);

    return icd_caller__lock((icd_caller *) that);
}

/* Unlock the customer */
icd_status icd_customer__unlock(icd_customer * that)
{
    assert(that != NULL);

    return icd_caller__unlock((icd_caller *) that);
}

/**** Listeners ****/

icd_status icd_customer__add_listener(icd_customer * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    return icd_caller__add_listener((icd_caller *) that, listener, lstn_fn, extra);
}

icd_status icd_customer__remove_listener(icd_customer * that, void *listener)
{
    assert(that != NULL);

    return icd_caller__remove_listener((icd_caller *) that, listener);
}

/* Standard functions for reacting to an AGENT state change */
icd_plugable_fn *icd_customer_get_plugable_fns(icd_caller * that)
{
    icd_plugable_fn *plugable_fns = NULL;
    char *dist_name = NULL;
    icd_distributor *dist = NULL;
    icd_member *member;


    assert(that != NULL);

    member = icd_caller__get_active_member(that);
    if (member == NULL) {
        /* always return our standard plugable interface when no distributor */
        plugable_fns = &icd_customer_plugable_fns;

    } else {
        /* check if there is a caller specific plugable_fn in plugable_fns_list for this dist */
        //plugable_fns = icd_plugable_fn_list__fetch_fns(that->plugable_fns_list, dist_name);    
        //if (plugable_fns == NULL) {
        /* if we fetched the caller specific plugable_fn_list for this dist and found 
         * no caller specific plugable defined then we try dist specific else
         * we just go with our static local interface for agent 
         */
        dist = icd_member__get_distributor(member);
	if(dist == NULL){
        	plugable_fns = &icd_customer_plugable_fns;
	}
	else {	
        	dist_name = vh_read(icd_distributor__get_params(dist), "dist");
        	plugable_fns = icd_distributor__get_plugable_fn(dist, that);
        	if (plugable_fns == NULL)
            		plugable_fns = &icd_customer_plugable_fns;
	}
        //}        

    }

    if (plugable_fns == NULL) {
        if (icd_verbose > 4)
            cw_log(LOG_NOTICE, "Customer Caller %d [%s] has no plugable fn aborting ala crash\n",
                icd_caller__get_id(that), icd_caller__get_name(that));
    } else {
        if (icd_verbose > 4)
            cw_log(LOG_NOTICE,
                "\nCustomer Caller %d [%s] using icd_customer_get_plugable_fns[%s] ready_fn[%p] for Dist[%s]\n",
                icd_caller__get_id(that), icd_caller__get_name(that), icd_plugable__get_name(plugable_fns),
                plugable_fns->state_ready_fn, dist_name);
    }
    assert(plugable_fns != NULL);
    return plugable_fns;

}

int icd_customer__standard_state_ready(icd_event * event, void *extra)
{
    icd_caller *that;

    //icd_member *member;
    //icd_list_iterator *iter;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);

    return 0;
}

/* Standard function for reacting to the call end state */
int icd_customer__standard_state_call_end(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_plugable_fn *icd_run;
/*     icd_list_iterator *iter; */
/*     icd_caller *associate; */
/*     icd_status result; */
/*     icd_caller_state state; */
    
    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

    /* Actions for when the call is terminated
     *   - destroy (we need cleanup thread)
     */

    /* Let our associates know. This should probably be a separate function
       call so that the associate can do its own cleanup, but for now we are
       going to use a simple state change to bridge_end if not in conference
       call.
     */
    if (that->associations != NULL){
        icd_caller__remove_all_associations(that);
/*        associate = (icd_caller *) icd_list__pop((icd_list *) that->associations);
        while(associate != NULL) {
             icd_caller_list__remove_caller_by_element(associate->associations, that);
	     state = icd_caller__get_state(associate);
	     if(state == ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE){ 
         		result = icd_caller__set_state(associate, ICD_CALLER_STATE_ASSOCIATE_FAILED);
             }    
    	     
	     associate = (icd_caller *) icd_list__pop((icd_list *) that->associations);
	     
      	}  */
    }
    if (that->chan && icd_caller__get_onhook(that)) {
        icd_bridge__safe_hangup(that);
    }

    icd_run = that->get_plugable_fn(that);
    icd_run->cleanup_caller_fn(that);   /* default is icd_customer__standard_cleanup_caller(that) CLEAR */
    return 0;
}

/* Standard function for cleaning up a caller after a bridge ends or fails */
icd_status icd_customer__standard_cleanup_caller(icd_caller * that)
{
    assert(that != NULL);
    if (icd_debug)
        cw_log(LOG_DEBUG,
            "Caller %d [%s] has a customer role with no pushback needed, exit icd thread finished \n",
            icd_caller__get_id(that), icd_caller__get_name(that));

    that->thread_state = ICD_THREAD_STATE_FINISHED;

    return ICD_SUCCESS;
}

icd_customer *icd_customer__generate_queued_call(char *id, char *queuename, char *dialstr, char *vars, char delim,
    void *plug)
{
    icd_customer *customer = NULL;
    icd_config *config;
    void_hash_table *arghash;
    icd_queue *queue;
    char buf[128], custname[80];
    char key[30];

    if (!queuename) {
        cw_log(LOG_ERROR, "Invalid Parameters\n");
        return NULL;
    }
    if (dialstr)
        strncpy(buf, dialstr, sizeof(buf));

    arghash = vh_init("args");
    if (vars)
        vh_carve_data(arghash, vars, delim);

    if (!strcmp(queuename, "auto"))
        queuename = vh_read(arghash, "queuename");

    queue = (icd_queue *) icd_fieldset__get_value(queues, queuename);

    if (queue == NULL) {
        cw_log(LOG_ERROR, "CUSTOMER FAILURE! Customer assigned to undefined Queue [%s]\n", queuename);
        return NULL;
    }

    config = create_icd_config(app_icd_config_registry, "customer_config");
    icd_config__set_raw(config, "params", arghash);

    customer = create_icd_customer(config);
    destroy_icd_config(&config);
    if (customer) {
        if (plug)
            icd_caller__set_plugable_fn_ptr((icd_caller *) customer, plug);

        icd_caller__add_flag((icd_caller *) customer, ICD_ORPHAN_FLAG);
        sprintf(custname, "generated_%s:%d", queuename, icd_caller__get_id((icd_caller *) customer));
        icd_caller__set_name((icd_caller *) customer, custname);
        icd_caller__set_dynamic((icd_caller *) customer, 1);
        icd_caller__set_onhook((icd_caller *) customer, 1);

        if (dialstr) {
            icd_caller__set_param_string((icd_caller *) customer, "channel", buf);
            icd_caller__set_channel_string((icd_caller *) customer, buf);
            icd_caller__create_channel((icd_caller *) customer);        /* %TC check this first */
        }
        icd_caller__add_role((icd_caller *) customer, ICD_LOOPER_ROLE);
        icd_caller__add_to_queue((icd_caller *) customer, queue);

        if (!strcasecmp(id, "auto")) {
            snprintf(key, 30, "%d", icd_caller__get_id((icd_caller *) customer));
            icd_caller__set_param_string((icd_caller *) customer, "identifier", key);
        } else
            icd_caller__set_param_string((icd_caller *) customer, "identifier", id);

        if (!strcasecmp(id, "auto")) {
            snprintf(key, 30, "%d", icd_caller__get_id((icd_caller *) customer));
            vh_write_cp_string(arghash, "identifier", key);
        } else
            vh_write_cp_string(arghash, "identifier", id);

        icd_caller__set_param_string((icd_caller *) customer, "queuename", queuename);

    }

    return customer;
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

