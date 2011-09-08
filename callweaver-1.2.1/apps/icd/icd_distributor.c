/*
 * Intelligent Call Distributor
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com> copyright <C> 2003
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o. 
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
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
 * \brief    icd_distributor.c  -  a strategy to assign a call to an agent
 *
 * The icd_distributor module holds a set of all agents and customers that can
 * talk to one another, as well as a thread of execution that can bridge the
 * channels.
 *
 * The distributor has a variety of constructors, one for each type of built-in
 * distributor strategy. You can design your own, but these are provided for
 * you. They are created by assigning appropriate functions to the customer
 * list and agent list by calling icd_list__set_node_insert_func(). The least
 * recent agent strategy, for example, uses icd_list__insert_fifo() on the agent
 * list, while the agent priority strategy uses icd_list__insert_ordered() with
 * the comparison function set to icd_agent__cmp_priority().
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 


#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_distributor_private.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_plugable_fn.h"

#include "callweaver/app.h"

#include "callweaver/icd/icd_bridge.h"
#include <pthread.h>

// ----------
struct cw_channel *agent_channel0 = NULL;
// ----------


static icd_module module_id = ICD_DISTRIBUTOR;


/* Removes caller from list of members for this distributor. */
icd_status icd_distributor__remove_caller(icd_distributor *that, icd_caller *that_caller);

/* Add a caller to the appropriate list in the distributor. */
icd_status icd_distributor__add_caller(icd_distributor *that, icd_member *new_member);
icd_status icd_distributor__pushback_caller(icd_distributor *that, icd_member *new_member);

/*===== Protected functions =====*/

icd_status icd_distributor__select_bridger(icd_caller *primary, 
        icd_caller *secondary);

void icd_distributor__create_plugable_fns(icd_plugable_fn *that, icd_config *data);

/*===== Public Implementations =====*/

/***** Init - Destroyer *****/

/* Create a distributor. data is a parsable string of parameters. */
icd_distributor *create_icd_distributor(char *name, icd_config *data) {
    icd_distributor *distributor;
    icd_status result;

    assert(data != NULL);

    /* make a new distributor from scratch */

    ICD_MALLOC(distributor,sizeof(icd_distributor));

    if (distributor == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Distributor\n");
        return NULL;
    }
    distributor->allocated = 1;
    distributor->state = ICD_DISTRIBUTOR_STATE_CREATED;
    result = init_icd_distributor(distributor, name, data);
    if (result != ICD_SUCCESS) {
         ICD_FREE(distributor);
         return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result = icd_event_factory__generate(event_factory, distributor, name, 
            module_id, ICD_EVENT_CREATE, NULL, distributor->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_distributor(&distributor);
        return NULL;
    }

    return distributor;
}

/* Destroy a distributor. */
icd_status destroy_icd_distributor(icd_distributor **distp) {
    icd_status vetoed;
    int clear_result;

    assert(distp != NULL);
    assert((*distp) != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event_factory__generate(event_factory, *distp, (*distp)->name, 
            module_id, ICD_EVENT_DESTROY, NULL, (*distp)->listeners, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD Distributor %s has been vetoed\n",
                icd_distributor__get_name(*distp));
        return ICD_EVETO;
    }

    if ((clear_result = icd_distributor__clear(*distp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the distributor if from the heap */
    if ((*distp)->allocated) {
         (*distp)->state = ICD_DISTRIBUTOR_STATE_DESTROYED;
        ICD_FREE((*distp));
        *distp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize an already existing distributor. */
icd_status init_icd_distributor(icd_distributor *that, char *name, icd_config *data) {
    icd_status (*init)(icd_distributor *that, char *name, icd_config *data);
    void *key;
    icd_status vetoed;

    assert(that != NULL);
    assert(data != NULL);
    if(that->allocated != 1)
        ICD_MEMSET_ZERO(that,sizeof(icd_distributor));

    icd_distributor__reset_added_callers_number(that);
    /* install all the config params into a hash to look at later */ 
    key = icd_config__get_value(data, "params");
    if(key) {
        that->params = (void_hash_table *)key;
    }
	/* mwa .. ha  .. ha....! Yes, I want to be able to even hijack the run fn -tony*/
    icd_distributor__set_default_run_fn(that);
    /* This is a problem here because it has to be done in every init (eventually) */
    /* OTOH, it is nice to have the name in the verbose message below */
    
    /* for APR supprt it's better to use the pool that->name = strdup(name);*/
    strncpy(that->name,name,sizeof(that->name));

    /*ToDo Load the registry pointers for the plugable funcs that a dist will usually
     * need to customize usually the _state_call_end_fn, cleanup_caller_fn
     * and the fail state funcs, then when a caller want to overide in the agent/customer init
     * they check b4 loading the caller/role specfic funcs to determine if 
     * the dist want an over ride bcus there is an entry in the dist registry for that
     * customization point
    */
    
    /*Init using a registered distributor might be a standard dist 
     * see icd_config.c->load_default_registry_entries or  a custom loadable module
     * see app_icd.c->load_module-> icd_module_load_dynamic_module
    */
    init = (icd_status (*)(icd_distributor *that, char *name, icd_config *data))
        icd_config__get_value(data, "dist");
    if (init != NULL) {
        cw_verbose(VERBOSE_PREFIX_1 "Using Registered dist for [%s] \n", 
                icd_distributor__get_name(that));
        return init(that, name, data);
    }
    /* Nope, use this one as the default. */
    cw_verbose(VERBOSE_PREFIX_1 "Using default dist for [%s] \n", icd_distributor__get_name(that));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_distributor__create_thread(that);

    vetoed = icd_event__generate(ICD_EVENT_INIT, NULL);
    if (vetoed == ICD_EVETO) {
        icd_distributor__clear(that);
        return ICD_EVETO;
    }
    that->state = ICD_DISTRIBUTOR_STATE_INITIALIZED;
	
    return ICD_SUCCESS;
}


/* Clear a distributor */
icd_status icd_distributor__clear(icd_distributor *that) {
    icd_status vetoed;

    assert(that != NULL);

    if (that->state == ICD_CALLER_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    /* Notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_CLEAR, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_WARNING, "Clearing of ICD Distributor %s has been vetoed\n",
                icd_distributor__get_name(that));
        return ICD_EVETO;
    }

    icd_distributor__stop_distributing(that);
    if(that->params && that->params->allocated) {
        vh_destroy(&(that->params));
    }
    if (that->customer_list_allocated) {
        destroy_icd_member_list(&(that->customers));
    } else {
        icd_member_list__clear(that->customers);
        that->customers = NULL;
    }
    if (that->agent_list_allocated) {
        destroy_icd_member_list(&(that->agents));
    } else {
        icd_member_list__clear(that->agents);
        that->agents = NULL;
    }

    that->link_fn = NULL;
    that->link_fn_extra = NULL;
    that->dump_fn = NULL;
    that->dump_fn_extra = NULL;
    that->customer_list_allocated = 0;
    that->agent_list_allocated = 0;
    pthread_cancel(that->thread);
    cw_cond_destroy(&(that->wakeup));
    cw_mutex_destroy(&(that->lock));
    
    return ICD_SUCCESS;
}


/***** Actions *****/

/* Adds an agent to the agent list, removing previous if necessary. */
icd_status icd_distributor__add_agent(icd_distributor *that, icd_member *new_agent) {
    int position;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(new_agent != NULL);

    position = icd_member_list__member_position(that->agents, new_agent);
    if (position >= 0) {
        icd_member_list__remove_member_by_element(that->agents, new_agent);
    }
    return icd_distributor__add_caller(that, new_agent);
}

/* Pushes an agent back onto the top of this distributor. */
icd_status icd_distributor__pushback_agent(icd_distributor *that, icd_member *new_agent) {
    int position;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(new_agent != NULL);

    position = icd_member_list__member_position(that->agents, new_agent);
    if (position >= 0) {
        icd_member_list__remove_member_by_element(that->agents, new_agent);
    }
    return icd_distributor__pushback_caller(that, new_agent);
}

/* Merges the agents in an agent list into the one contained in this distributor. */
icd_status icd_distributor__add_agent_list(icd_distributor *that, icd_member_list *new_list) {
    icd_status result;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(new_list != NULL);

    result = icd_list__merge((icd_list *)that->agents, (icd_list *)new_list);
    if (icd_distributor__agents_pending(that) > 0) {
        result = icd_distributor__lock(that);
        cw_cond_signal(&(that->wakeup));
        result = icd_distributor__unlock(that);
    }
    return result;
}

/* Gives back the number of icd_agents waiting for a connection. */
int icd_distributor__agents_pending(icd_distributor *that) {
    assert(that != NULL);
    assert(that->agents != NULL);

    return icd_list__count((icd_list *)that->agents);
}

/* Gives the position of this agent in the list. */
int icd_distributor__agent_position(icd_distributor *that, icd_agent *target) {
    icd_member *member;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(target != NULL);

    member = icd_caller__get_member_for_distributor((icd_caller *)target, that);
    return icd_member_list__member_position(that->agents, member);
}

/* Removes a specific agent from the distributor's list of available agents. */
icd_status icd_distributor__remove_agent(icd_distributor *that, icd_agent *target) {
    assert(that != NULL);
    assert(that->agents != NULL);
    assert(target != NULL);

    return icd_distributor__remove_caller(that, (icd_caller *)target);
}

/* Adds a customer to the customer list, removing previous if necessary. */
icd_status icd_distributor__add_customer(icd_distributor *that, icd_member *new_customer) {
    int position;

    assert(that != NULL);
    assert(that->customers != NULL);
    assert(new_customer != NULL);

    position = icd_member_list__member_position(that->customers, new_customer);
    if (position >= 0) {
        icd_member_list__remove_member_by_element(that->customers, new_customer);
    }
    return icd_distributor__add_caller(that, new_customer);
}

/* Pushes a customer back onto the top of this distributor. */
icd_status icd_distributor__pushback_customer(icd_distributor *that, icd_member *new_customer) {
    int position;

    assert(that != NULL);
    assert(that->customers != NULL);
    assert(new_customer != NULL);

    position = icd_member_list__member_position(that->customers, new_customer);
    if (position >= 0) {
        icd_member_list__remove_member_by_element(that->customers, new_customer);
    }
    return icd_distributor__pushback_caller(that, new_customer);
}

/* Merges the customers in a customer list into the one contained in this distributor. */
icd_status icd_distributor__add_customer_list(icd_distributor *that, icd_member_list *new_list) {
    icd_status result;

    assert(that != NULL);
    assert(that->customers != NULL);
    assert(new_list != NULL);

    result = icd_list__merge((icd_list *)that->customers, (icd_list *)new_list);
    if (icd_distributor__customers_pending(that)) {
        result = icd_distributor__lock(that);
        cw_cond_signal(&(that->wakeup));
        result = icd_distributor__unlock(that);
    }
    return result;
}

/* Gives back the number of icd_customers waiting for a connection. */
int icd_distributor__customers_pending(icd_distributor *that) {
    assert(that != NULL);
    assert(that->customers != NULL);

    return icd_list__count((icd_list *)that->customers);
}

/* Gives the position of this customer in the list. */
int icd_distributor__customer_position(icd_distributor *that, icd_customer *target) {
    icd_member *member;

    assert(that != NULL);
    assert(that->customers != NULL);
    assert(target != NULL);

    member = icd_caller__get_member_for_distributor((icd_caller *)target, that);
    return icd_member_list__member_position(that->customers, member);
}

/* Removes a specific customer from the distributor's list of available agents. */
icd_status icd_distributor__remove_customer(icd_distributor *that, icd_customer *target) {
    assert(that != NULL);
    assert(that->customers != NULL);
    assert(target != NULL);

    return icd_distributor__remove_caller(that, (icd_caller *)target);
}

/* Print out a copy of the distributor. */
icd_status icd_distributor__dump(icd_distributor *that, int verbosity, int fd) {
    assert(that != NULL);
    assert(that->dump_fn != NULL);

    return that->dump_fn(that, verbosity, fd, that->dump_fn_extra);
    return ICD_SUCCESS;
}

/* Start the distributor thread. */
icd_status icd_distributor__start_distributing(icd_distributor *that) {
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_RUNNING;
    cw_verbose(VERBOSE_PREFIX_1 "Started [%s] State[%d] \n", 
            icd_distributor__get_name(that), that->thread_state);
    return ICD_SUCCESS;
}

/* Pause the distributor thread. */
icd_status icd_distributor__pause_distributing(icd_distributor *that) {
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_PAUSED;
    return ICD_SUCCESS;
}

/* Stop and finish the distributor thread. */
icd_status icd_distributor__stop_distributing(icd_distributor *that) {
    assert(that != NULL);

    that->thread_state = ICD_THREAD_STATE_FINISHED;
    return ICD_SUCCESS;
}


/***** Getters and Setters *****/

/* Sets the agent list to use in this distributor. */
icd_status icd_distributor__set_agent_list(icd_distributor *that, icd_member_list* agents) {
    icd_status result;

    assert(that != NULL);
    assert(agents != NULL);

    if (that->agent_list_allocated) {
      destroy_icd_member_list(&(that->agents));
    }
    that->agent_list_allocated = 0;
    that->agents = agents;
    if (icd_distributor__agents_pending(that)) {
        result = icd_distributor__lock(that);
        cw_cond_signal(&(that->wakeup));
        result = icd_distributor__unlock(that);
    }
    return result;
}

/* Gets the agent list used in this distributor. */
icd_member_list *icd_distributor__get_agent_list(icd_distributor *that) {
    assert(that != NULL);

    return that->agents;
}

/* Sets the customer list to use in this distributor. */
icd_status icd_distributor__set_customer_list(icd_distributor *that, icd_member_list *customers) {
    icd_status result;

    assert(that != NULL);
    assert(customers != NULL);

    if (that->customer_list_allocated) {
      destroy_icd_member_list(&(that->customers));
    }
    that->customer_list_allocated = 0;
    that->customers = customers;
    if (icd_distributor__customers_pending(that)) {
        result = icd_distributor__lock(that);
        cw_cond_signal(&(that->wakeup));
        result = icd_distributor__unlock(that);
    }
    return result;
}

/* Gets the customer list used in this distributor. */
icd_member_list *icd_distributor__get_customer_list(icd_distributor *that) {
    assert(that != NULL);

    return that->customers;
}

/* Set the name of the distributor */
icd_status icd_distributor__set_name(icd_distributor *that, char *name) {
    assert(that != NULL);


    /*that->name = strdup(name);*/
strncpy(that->name,name,sizeof(that->name));
    return ICD_SUCCESS;
}

/* Gets the name of this distributor. Protect against NULL values. */
char *icd_distributor__get_name(icd_distributor *that) {
    assert(that != NULL);

    if (that->name == NULL) {
        return "";
    }
    return that->name;
}


/* TBD - Change this to icd_fieldset */
void_hash_table *icd_distributor__get_params(icd_distributor *that) {
    assert(that != NULL);
    return that->params;
}

void *icd_distributor__get_plugable_fn_ptr(icd_distributor *that) {
    assert(that != NULL);

    return that->get_plugable_fn;

}    
icd_plugable_fn *icd_distributor__get_plugable_fn(icd_distributor *that, icd_caller *caller) {
    assert(that != NULL);
    if (that->get_plugable_fn != NULL)
        return that->get_plugable_fn(caller);
    else 
        return NULL;

}    

/**** Callback Setters ****/

/* Sets the plugable_fn function to a particular function. */
icd_status icd_distributor__set_plugable_fn_ptr(icd_distributor *that, icd_plugable_fn *(*get_plugable_fn)(icd_caller *)) {
    assert(that != NULL);

    that->get_plugable_fn = get_plugable_fn;
    return ICD_SUCCESS;
}

icd_status icd_distributor__set_link_callers_fn(icd_distributor *that, icd_status (*link_fn)(icd_distributor *, void *extra), void *extra) {
    assert(that != NULL);

    that->link_fn = link_fn;
    that->link_fn_extra = extra;
    return ICD_SUCCESS;
}

icd_status icd_distributor__set_dump_func(icd_distributor *that, icd_status (*dump_fn)(icd_distributor *, int verbosity, int fd, void *extra), void *extra) {
    assert(that != NULL);

    that->dump_fn = dump_fn;
    that->dump_fn_extra = extra;
    return ICD_SUCCESS;
}


/***** Locking *****/

/* Lock the distributor. If the mutex is of type errorcheck we should check EDEADLK
   in case the thread locks an object twice. If we decide to use
   trylocks, we need to check for EBUSY. EINVAL indicates the mutex wasn't initialized.*/
icd_status icd_distributor__lock(icd_distributor *that) {
    int retval;

    assert(that != NULL);

    if (that->state == ICD_DISTRIBUTOR_STATE_CLEARED || that->state == ICD_DISTRIBUTOR_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    retval = cw_mutex_lock(&that->lock);
    if (retval == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/* Unlock the distributor. Check error codes EINVAL (mutex not initialized) and EPERM. */
icd_status icd_distributor__unlock(icd_distributor *that) {
    int retval;

    assert(that != NULL);

    if (that->state == ICD_DISTRIBUTOR_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    retval = cw_mutex_unlock(&that->lock);
    if (retval == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}


/**** Listeners ****/

icd_status icd_distributor__add_listener(icd_distributor *that, void *listener, int (*lstn_fn)(void *listener, icd_event *event, void *extra), void *extra) {
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_distributor__remove_listener(icd_distributor *that, void *listener) {
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}


/**** Iterator functions ****/

icd_list_iterator *icd_distributor__get_customer_iterator(icd_distributor *that) {
    assert(that != NULL);

    return (icd_list__get_iterator((icd_list *)(that->customers)));
}

icd_list_iterator *icd_distributor__get_agent_iterator(icd_distributor *that) {
    assert(that != NULL);

    return (icd_list__get_iterator((icd_list *)(that->agents)));
}


/***** Predefined Pluggable Actions *****/
// pop
/* Pop the top agent and customer, and link them. */
icd_status icd_distributor__link_callers_via_pop(icd_distributor *dist, void *extra) {
    icd_member *customer_member;
    icd_member *agent_member;
    int c_id;
    int a_id;
    icd_status result;
    icd_caller *customer_caller = NULL;
    icd_caller *agent_caller = NULL;
    
    assert(dist != NULL);
    assert(dist->customers != NULL);
    assert(dist->agents != NULL);

    if (!icd_member_list__has_members(dist->customers)  || !icd_member_list__has_members(dist->agents)) {
        return ICD_ENOTFOUND;
    }

    agent_member = icd_member_list__pop_locked(dist->agents);
    if(agent_member) {
        if (icd_member__lock(agent_member) == ICD_SUCCESS){
        	agent_caller = icd_member__get_caller(agent_member);
		if(!agent_caller){
			icd_member__unlock(agent_member);
		}
	}
	icd_member_list__unlock(dist->agents);
    }
    if (agent_member == NULL || agent_caller == NULL) {
        cw_log(LOG_ERROR, "ICD Distributor %s could not retrieve agent from list\n", icd_distributor__get_name(dist));
        return ICD_ERESOURCE;
    }
    
    result = icd_member__distribute(agent_member);
    if (result != ICD_SUCCESS) {
	icd_member__unlock(agent_member);
        return result;
    }
    
/*    
    result = icd_member__distribute(agent_member);
    if (result != ICD_SUCCESS) {
        return result;
    }
*/
    customer_member = icd_member_list__pop_locked(dist->customers);
    if (customer_member == NULL) {
        cw_log(LOG_ERROR, "ICD Distributor %s could not retrieve customer from list\n", icd_distributor__get_name(dist));
        icd_caller__start_waiting(agent_caller);
	icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
	icd_member_list__unlock(dist->customers);
	icd_member__unlock(agent_member);
        return ICD_ENOTFOUND;
    }

    if (icd_member__lock(customer_member) == ICD_SUCCESS){
       	customer_caller = icd_member__get_caller(customer_member);
	if(!customer_caller){
		icd_member__unlock(customer_member);
	}
     }
    icd_member_list__unlock(dist->customers);
    if(customer_caller == NULL) {
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
	icd_member__unlock(agent_member);
        return ICD_ERESOURCE;
    }
    
    if(icd_caller__get_state(customer_caller) == ICD_CALLER_STATE_READY){
	icd_member__unlock(customer_member);
	/* TODO There is a danger of customer destroying at this point by customer thread*/
    	icd_caller__lock(customer_caller);
	if(icd_caller__get_state(customer_caller) == ICD_CALLER_STATE_READY){
	/* Still in READY state - OK */
		result = ICD_SUCCESS;
	}
	else {
		result = ICD_ERESOURCE;
    		icd_caller__unlock(customer_caller);
	}
    }
    else {
	result = ICD_ERESOURCE;
	icd_member__unlock(customer_member);
    }	
		
    if (result != ICD_SUCCESS) {
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);   
	icd_member__unlock(agent_member);
        return result;
    }
    
    result = icd_member__distribute(customer_member);
    if (result != ICD_SUCCESS) {
	icd_caller__unlock(customer_caller);
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);   
	icd_member__unlock(agent_member);
        return result;
    }
    

    /* let cloners replicate if necessary 
       if it is an agent cloner and the limit is reached 
       then put the customer back and quit    */
    // TC TBD customer_caller = icd_caller__clone_if_necessary(customer_caller);
    // TC TBD agent_caller = icd_caller__clone_if_necessary(agent_caller);

    result = icd_caller__join_callers(customer_caller, agent_caller);
    if (result != ICD_SUCCESS) {
        icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
	icd_member__unlock(agent_member);
	icd_caller__unlock(customer_caller);
        return result;
    }

    c_id = icd_caller__get_id(customer_caller);
    a_id = icd_caller__get_id(agent_caller);

    /* Figure out who the bridger is, and who the bridgee is */
    result = icd_distributor__select_bridger(agent_caller, customer_caller);

    cw_verbose(VERBOSE_PREFIX_3 "Distributor [%s] Link CustomerID[%d] to AgentID[%d]\n", 
            icd_distributor__get_name(dist), c_id, a_id );
    if (icd_caller__has_role(customer_caller, ICD_BRIDGER_ROLE)) {
        result = icd_caller__bridge(customer_caller);
    } else if (icd_caller__has_role(agent_caller, ICD_BRIDGER_ROLE)) {
        result = icd_caller__bridge(agent_caller);
    } else {
        cw_log(LOG_ERROR, "ICD Distributor %s found no bridger responsible to bridge call\n", icd_distributor__get_name(dist));
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
        icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
	icd_member__unlock(agent_member);
	icd_caller__unlock(customer_caller);
        return ICD_EGENERAL;
    }
/*
    icd_caller__dump_debug(customer_caller);
    icd_caller__dump_debug(agent_caller);
*/
    icd_member__unlock(agent_member);
    icd_caller__unlock(customer_caller);
    return ICD_SUCCESS;
}

/* Pop the top agent and customer and link them, and then add agent back to list */
icd_status icd_distributor__link_callers_via_pop_and_push(icd_distributor *dist, void *extra) {
    icd_member *customer_member;
    icd_member *agent_member;
    int c_id;
    int a_id;
    icd_status result;
    icd_caller *customer_caller = NULL;
    icd_caller *agent_caller = NULL;

    assert(dist != NULL);
    assert(dist->customers != NULL);
    assert(dist->agents != NULL);

    if (!icd_member_list__has_members(dist->customers)  || !icd_member_list__has_members(dist->agents)) {
        return ICD_ENOTFOUND;
    }

    agent_member = icd_member_list__pop(dist->agents);
    if(agent_member)
        agent_caller = icd_member__get_caller(agent_member);
    if (agent_member == NULL || agent_caller == NULL) {
        cw_log(LOG_ERROR, "ICD Distributor %s could not retrieve agent from list\n", icd_distributor__get_name(dist));
        return ICD_ERESOURCE;
    }
	icd_member_list__lock(icd_caller__get_memberships(agent_caller));
	/* check if agent_member still exists */		
    if(agent_member == icd_member_list__get_for_caller(icd_caller__get_memberships(agent_caller), agent_caller)){
       result = icd_member__distribute(agent_member);
    }
    else{
        result = ICD_ENOTFOUND;    	
    }
	icd_member_list__unlock(icd_caller__get_memberships(agent_caller));		
    if (result != ICD_SUCCESS) {
        /* Some other distributor got to this agent first */
        return result;
    }

    customer_member = icd_member_list__pop(dist->customers);
    customer_caller = icd_member__get_caller(customer_member);
    if (customer_member == NULL || customer_caller == NULL) {
        cw_log(LOG_ERROR, "ICD Distributor %s could not retrieve customer from list\n", icd_distributor__get_name(dist));
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
        return ICD_ERESOURCE;
    }
    
    result = icd_member__distribute(customer_member);
    if (result != ICD_SUCCESS) {
        /* Some other distributor got to this customer first */
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
        return result;
    }

    result = icd_caller__join_callers(customer_caller, agent_caller);
    if (result != ICD_SUCCESS) {
        icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
        return result;
    }

    c_id = icd_caller__get_id(customer_caller);
    a_id = icd_caller__get_id(agent_caller);

    /* Figure out who the bridger is, and who the bridgee is */
    result = icd_distributor__select_bridger(agent_caller, customer_caller);

    cw_verbose(VERBOSE_PREFIX_3 "Distributor [%s] Link CustomerID[%d] to AgentID[%d]\n", 
            icd_distributor__get_name(dist), c_id, a_id );
    if (icd_caller__has_role(customer_caller, ICD_BRIDGER_ROLE)) {
        result = icd_caller__bridge(customer_caller);
    } else if (icd_caller__has_role(agent_caller, ICD_BRIDGER_ROLE)) {
        result = icd_caller__bridge(agent_caller);
    } else {
        cw_log(LOG_ERROR, "ICD Distributor %s found no bridger responsible to bridge call\n", icd_distributor__get_name(dist));
        icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
        icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
        return ICD_EGENERAL;
    }

    /*TC do we still want this behavior do we remove pop push bcus agent CANT go back till  state_ready
     * Add agent back on to distributor so it gets its turn in round-robin fashion */
/*    
    result = icd_member_list__push(dist->agents, agent_member);
*/   
/*
    icd_caller__dump_debug(customer_caller);
    icd_caller__dump_debug(agent_caller);
*/
    return ICD_SUCCESS;
}

/* Pop the top customer_member and link to all free agents (who are all bridgers). */
icd_status icd_distributor__link_callers_via_ringall(icd_distributor *dist, void *extra) {
    assert(dist != NULL);
    assert(dist->customers != NULL);
    assert(dist->agents != NULL);

    return ICD_SUCCESS;
}

/* Standard dump function for distributor */
icd_status icd_distributor__standard_dump(icd_distributor *dist, int verbosity, int fd, void *extra) {
    static char *indent = "    ";
    vh_keylist *keys;
    assert(dist != NULL);

    
    cw_cli(fd,"\nDumping icd_distributor {\n");
    cw_cli(fd,"%sname=%s (%s)\n", indent,icd_distributor__get_name(dist), 
            dist->allocated ? "alloced" : "on heap");

    cw_cli(fd,"%sparams {\n",indent);
    for(keys = vh_keys(dist->params); keys; keys=keys->next)
        cw_cli(fd,"%s%s%s=%s\n", indent, indent, keys->name,
                (char *)vh_read(dist->params,keys->name));

    cw_cli(fd,"%s}\n\n",indent);

    cw_cli(fd,"%slink_fn=%p\n", indent,dist->link_fn);
    cw_cli(fd,"%sdump_fn=%p\n", indent,dist->dump_fn);
    cw_cli(fd,"\n%scustomers=%p (%s) {\n", indent,dist->customers, 
            dist->customer_list_allocated ? "alloced" : "on heap");
    if (verbosity > 1) {
        icd_member_list__dump(dist->customers, verbosity - 1, fd);
    }
    else 
        icd_member_list__dump(dist->customers,0, fd);

    cw_cli(fd,"%s}\n\n%sagents=%p (%s) {\n", indent,indent,dist->agents,
            dist->agent_list_allocated ? "alloced" : "on heap");
    if (verbosity > 1) {
        icd_member_list__dump(dist->agents, verbosity - 1, fd);
    }
    else 
        icd_member_list__dump(dist->agents,0, fd);
    cw_cli(fd,"%s}\n",indent);

    return ICD_SUCCESS;
}

/*===== Protected Implementations =====*/

/* Given two callers, decides which should act as bridger and sets its role.
   Not all strategies will use this, but enough that we should make it
   available to subclasses of icd_distributor if there ever are any. */
icd_status icd_distributor__select_bridger(icd_caller *primary, 
        icd_caller *secondary) {
    cw_channel *primary_channel;
    cw_channel *secondary_channel;

    assert(primary != NULL);
    assert(secondary != NULL);

    primary_channel = icd_caller__get_channel(primary);
    secondary_channel = icd_caller__get_channel(secondary);

    if(icd_caller__has_role(primary,ICD_CLONE_ROLE)) {
        icd_caller__add_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(secondary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;
    }
    if(icd_caller__has_role(secondary,ICD_CLONE_ROLE)) {
        icd_caller__add_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(primary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;

    }

    if (primary_channel == NULL) {
        icd_caller__add_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(secondary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;
    }
    if (secondary_channel == NULL) {
/* strategy changed. If scondary - customer has no channel then primary-agent role brigee  is not serviced well by ICD */ 
/*        icd_caller__add_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(primary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGEE_ROLE);
*/	
        icd_caller__add_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(secondary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;
    }

    if (primary_channel->_state == CW_STATE_UP || 
        primary_channel->_state == CW_STATE_OFFHOOK) {
        icd_caller__add_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(secondary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;
    }
    if (secondary_channel->_state == CW_STATE_UP ||
        secondary_channel->_state == CW_STATE_OFFHOOK) {
        icd_caller__add_role(secondary, ICD_BRIDGER_ROLE);
        icd_caller__add_role(primary, ICD_BRIDGEE_ROLE);
        icd_caller__clear_role(primary, ICD_BRIDGER_ROLE);
        icd_caller__clear_role(secondary, ICD_BRIDGEE_ROLE);
        return ICD_SUCCESS;
    }
    icd_caller__add_role(primary, ICD_BRIDGER_ROLE);
    icd_caller__add_role(secondary, ICD_BRIDGEE_ROLE);
    icd_caller__clear_role(secondary, ICD_BRIDGER_ROLE);
    icd_caller__clear_role(primary, ICD_BRIDGEE_ROLE);
    return ICD_SUCCESS;
}

/*===== Private implementations =====*/

/* Create an icd_list for both the agents and the customers */
icd_status icd_distributor__create_lists(icd_distributor *that, icd_config *data) {
    icd_config *customer_list_config;
    icd_config *agent_list_config;
    char buf[80];
    
    assert(that != NULL);
    assert(data != NULL);

    /* Create configurations for customer and agent lists, setting defaults */
    customer_list_config = icd_config__get_subset(data, "customers.");

    icd_distributor__correct_list_config(customer_list_config);
    snprintf(buf, sizeof(buf), "Customers of Distributor %s", icd_distributor__get_name(that));
    that->customers = create_icd_member_list(buf, customer_list_config);
    that->customer_list_allocated = 1;
    destroy_icd_config(&customer_list_config);

    agent_list_config = icd_config__get_subset(data, "agents.");
    icd_distributor__correct_list_config(agent_list_config);
    snprintf(buf, sizeof(buf), "Agents of Distributor %s", icd_distributor__get_name(that));
    that->agents = create_icd_member_list(buf, agent_list_config);
    that->agent_list_allocated = 1;
    destroy_icd_config(&agent_list_config);
    destroy_icd_config(&customer_list_config);

    that->listeners = create_icd_listeners();

    return ICD_SUCCESS;
}

/* Make sure that each list has room for a minimum number of entries. */
icd_status icd_distributor__correct_list_config(icd_config *data) {
    void *list_sizep;

    assert(data != NULL);

    list_sizep = icd_config__get_value(data, "size");
    if (list_sizep == NULL) {
        icd_config__set_value(data, "size", "200");
    }
    return ICD_SUCCESS;
}

/* Pulls parameters out of config and sets them in distributor */
icd_status icd_distributor__set_config_params(icd_distributor *that, icd_config *data) {

    icd_distributor__set_link_callers_fn(that, 
            icd_config__get_any_value(data, "link",  icd_distributor__link_callers_via_pop), 
            icd_config__get_any_value(data, "link.extra", NULL));
    icd_distributor__set_dump_func(that, 
            icd_config__get_any_value(data, "dump",  icd_distributor__standard_dump), 
            icd_config__get_any_value(data, "dump.extra", NULL));
    return ICD_SUCCESS;
}

/* Initializes the thread and the objects that it relies on. */
icd_status icd_distributor__create_thread(icd_distributor *that) {
/* 
%TC Create a single thread of execution for each queue in icd_queue.conf  eg 5 entries = 5 threads
Create the condition that wakes up the dist thread, see cw_cond_signal
We will invoke cw_cond_signal each time an event occurs that requires the dist thread to do some work
like when a customer, agent is added see icd_distributor__add_agent, icd_distributor__add_customer etc
when the dist has no work to do we invoke cw_cond_wait see icd_distributor__run
*/

    pthread_attr_t attr;
    pthread_condattr_t condattr;
    int result;

    assert(that != NULL);

    /* Create the mutex */
    cw_mutex_init(&(that->lock));

    /* Create the condition that wakes up the dist thread */
    result = pthread_condattr_init(&condattr);
    result = cw_cond_init(&(that->wakeup), &condattr);
    result = pthread_condattr_destroy(&condattr);

    /* Create the thread */
    result = pthread_attr_init(&attr);
    /* Adjust thread attributes here before creating it */
    pthread_attr_setschedpolicy(&attr, SCHED_RR); /* Or do we want parent scheduling? */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    /* Adjust distributor state before creating thread */
    that->thread_state = ICD_THREAD_STATE_PAUSED;
    /* Create thread */
    result = cw_pthread_create(&(that->thread), &attr, icd_distributor__run, that);
    cw_verbose(VERBOSE_PREFIX_2 "Spawn Distributor [%s] run thread \n", 
                        icd_distributor__get_name(that));
    /* Clean up */
    result = pthread_attr_destroy(&attr);
    return ICD_SUCCESS;
}

/*
  Danger Will Robinson!  Some people who know what thiey're doing can 
  hijack the entire run fuction of the caller and/or distribotor.
  Once you do this, of course, it invalidates your warranty ;)
*/

icd_status icd_distributor__set_run_fn(icd_distributor *that, void *(*run_fn)(void *that)) {
	assert(that != NULL);  
	that->run_fn = run_fn;
	return ICD_SUCCESS;
}

icd_status icd_distributor__set_default_run_fn(icd_distributor *that) {
	assert(that != NULL);  
	that->run_fn = icd_distributor__standard_run;
	return ICD_SUCCESS;
}

void *icd_distributor__run(void *that) {
	icd_distributor *dist;
    assert(that != NULL);
    assert(((icd_distributor *)that)->customers != NULL);
    assert(((icd_distributor *)that)->agents != NULL);
    dist = (icd_distributor *)that;
	if(!dist->run_fn)
		icd_distributor__set_default_run_fn(dist);
	assert(dist->run_fn != NULL);
	return dist->run_fn(that);
}

/* The run method for the distributor thread. */
void *icd_distributor__standard_run(void *that) {
    icd_distributor *dist;
    icd_status result;

    assert(that != NULL);
    assert(((icd_distributor *)that)->customers != NULL);
    assert(((icd_distributor *)that)->agents != NULL);

    dist = (icd_distributor *)that;
    
    while (dist->thread_state != ICD_THREAD_STATE_FINISHED) {
        if (dist->thread_state == ICD_THREAD_STATE_RUNNING) {
            result = icd_distributor__lock(dist);
            /* Distribute callers if we can, or pause until some are added */
            if (icd_distributor__customers_pending(dist) && 
                    icd_distributor__agents_pending(dist)) {
                icd_distributor__reset_added_callers_number(dist);
                result = icd_distributor__unlock(dist);
                /* func ptr to the icd_distributor__link_callers_via_?? note may also come from custom
                 * function eg from icd_mod_?? installed using icd_distributor__set_link_callers_fn
                */
                if (icd_verbose > 4)
                    cw_verbose(VERBOSE_PREFIX_3 "Distributor__run [%s] link_fn[%p]  \n", 
                        icd_distributor__get_name(dist), dist->link_fn);
                result = dist->link_fn(dist, dist->link_fn_extra);  
            } else {
                cw_cond_wait(&(dist->wakeup), &(dist->lock)); /* wait until signal received */
                result = icd_distributor__unlock(dist);
                if (icd_verbose > 4)
                    cw_verbose(VERBOSE_PREFIX_3 "Distributor__run [%s] wait  \n", 
                        icd_distributor__get_name(dist));
            }
        } else {
            /* TBD - Make paused thread work better. 
             *        - Use cw_cond_wait()
             *        - Use same or different condition variable? 
             */
        }
        /* Play nice */
        sched_yield();
    }
    /* Do any cleanup here */
    return NULL;
}

/* Removes caller from list of members for this distributor. */
/* TBD - Refactor this (and other removes) to only use member rather than caller. */
icd_status icd_distributor__remove_caller(icd_distributor *that, icd_caller *that_caller) {
    icd_member_list *target;
    icd_member *member;

    assert(that != NULL);
    assert(that_caller != NULL);

    //icd_caller__dump_debug(that_caller);
    // These roles were improperly stated as 'ICD_AGENT' and 'ICD_CALLER'
    // this lead to the caller not being removed at all 

    if(icd_caller__has_role(that_caller, ICD_AGENT_ROLE)) {
        target = that->agents;
    } else if(icd_caller__has_role(that_caller, ICD_CUSTOMER_ROLE)) {
        target = that->customers;
    } else {
        cw_log(LOG_ERROR,"HUGE ERROR! INVALID CALLER ENCOUNTERED.\n");
        return ICD_EGENERAL;
    }
    member = icd_caller__get_member_for_distributor(that_caller, that);
    return icd_member_list__remove_member_by_element(target, member);
}
 
/* Add a caller to the appropriate list in the distributor. */
icd_status icd_distributor__add_caller(icd_distributor *that, icd_member *new_member) {
    icd_status result;
    icd_caller *caller;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(new_member != NULL);

    caller = icd_member__get_caller(new_member);
    if(icd_caller__has_role(caller, ICD_AGENT_ROLE)) {
        result = icd_member_list__push(that->agents, new_member);
    } else if(icd_caller__has_role(caller, ICD_CUSTOMER_ROLE)) {
        result = icd_member_list__push(that->customers, new_member);
    } else {
        cw_log(LOG_WARNING,"Danger Will Robinson!  No suitable role to join distributor!");
    }

    result = icd_distributor__lock(that);
    that->number_of_callers_added++;
    cw_cond_signal(&(that->wakeup));
    result = icd_distributor__unlock(that);
    return result;
}

/* Pushback a caller to the top of the appropriate list in the distributor. */
icd_status icd_distributor__pushback_caller(icd_distributor *that, icd_member *new_member) {
    icd_status result;
    icd_caller *caller;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(new_member != NULL);

    /* Why are we doing this here? */
    caller = icd_member__get_caller(new_member);
    if(icd_caller__has_role(caller, ICD_AGENT_ROLE)) {
        result = icd_member_list__pushback(that->agents, new_member);
    } else if(icd_caller__has_role(caller, ICD_CUSTOMER_ROLE)) {
        result = icd_member_list__pushback(that->customers, new_member);
    } else {
        cw_log(LOG_WARNING,"Danger Will Robinson!  No suitable role to join distributor!");
    }

    result = icd_distributor__lock(that);
    that->number_of_callers_added++;
    cw_cond_signal(&(that->wakeup));
    result = icd_distributor__unlock(that);
    return result;
}


/* %TC we have a talking pt here should we make all these distributors load able modules
 * using the icd_module_api interface so this all become icd_mod_[round_robin,least_recent_agent ...].c
 * and do away with concept of 'builtin' distro strategies and just leave the building blocks
 * and the function pt loaders
*/
/* Initialize a round robin distributor. */
icd_status init_icd_distributor_round_robin(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);

    /*that->name = strdup(name);*/
    strncpy(that->name,name,sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_ordered,
            icd_member__cmp_call_start_time_order);    
    icd_distributor__set_link_callers_fn(that, icd_distributor__link_callers_via_pop, NULL);
    icd_distributor__create_thread(that);
    return ICD_SUCCESS;
}

/* Initialize a least recent distributor. */
icd_status init_icd_distributor_least_recent_agent(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);

    strncpy(that->name,name,sizeof(that->name));
    /*that->name = strdup(name);*/
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_ordered,
            icd_member__cmp_last_state_change_reverse_order);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Initialize a most recent distributor. */
icd_status init_icd_distributor_most_recent_agent(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);
    strncpy(that->name,name,sizeof(that->name));
    /*that->name = strdup(name);*/
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_ordered,
            icd_member__cmp_last_state_change_order);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Initialize an agent priority distributor. */
icd_status init_icd_distributor_agent_priority(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);
    strncpy(that->name,name,sizeof(that->name));
    /*that->name = strdup(name);*/
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_ordered,
            icd_member__cmp_priority_order);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Initialize a least calls distributor. */
icd_status init_icd_distributor_least_calls_agent(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);

    /*that->name = strdup(name);*/
    strncpy(that->name,name,sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_ordered,
            icd_member__cmp_callcount_order);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Initialize a random distributor. */
icd_status init_icd_distributor_random(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);

    /*that->name = strdup(name);*/
    strncpy(that->name,name,sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_random, NULL);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Initialize a ringall distributor. */
icd_status init_icd_distributor_ringall(icd_distributor *that, char *name, icd_config *data) {

    assert(that != NULL);
    assert(data != NULL);

    /*that->name = strdup(name);*/
	strncpy(that->name,name,sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *)that->agents, icd_list__insert_fifo, NULL);
    icd_distributor__set_link_callers_fn(that, icd_distributor__link_callers_via_ringall, NULL);
    icd_distributor__create_thread(that);

    return ICD_SUCCESS;
}

/* Gets the number of callers added since last link function call. */
    unsigned int icd_distributor__get_added_callers_number(icd_distributor * that){
    
          return  that->number_of_callers_added;
    }

/*  Resets the number of callers added since last link function call.  */
    void icd_distributor__reset_added_callers_number(icd_distributor * that){
        that->number_of_callers_added = 0;
    }
