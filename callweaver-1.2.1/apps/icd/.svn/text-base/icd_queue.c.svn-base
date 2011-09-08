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
  *  \brief icd_queue.c - central queueing definitions
*/ 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_event.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_listeners.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_private.h"

static icd_module module_id = ICD_QUEUE;
static struct cw_app *monitor_app = NULL;

icd_queue_holdannounce *create_icd_queue_holdannounce(icd_config * config);
icd_status init_icd_queue_holdannounce(icd_queue_holdannounce * that, icd_config * config);
icd_status destroy_icd_queue_holdannounce(icd_queue_holdannounce * holdannounce);


struct icd_queue {
    char *name;
    icd_distributor *distributor;
    icd_member_list *customers;
    icd_member_list *agents;
    icd_queue_holdannounce holdannounce;
    icd_queue_chimeinfo chimeinfo;
    char monitor_args[256];
    int priority;               /* priority of this queue in relation to other queues */
    int wait_timeout;           /* How many seconds before timing out of queue */
    void_hash_table *params;
    icd_listeners *listeners;
    icd_queue_state state;
    int flag;                   /*accept calls, tagged iter mem q, match em from config untag mark for delete */
      icd_status(*dump_fn) (icd_queue *, int verbosity, int fd, void *extra);
    void *dump_fn_extra;
    icd_memory *memory;
    cw_mutex_t lock;
    int allocated;
};

static int parse_list(char *str, char delim, char *alist[])
{
    char *args, *p;
    int x = 1;

    alist[0] = args = str;
    while ((p = strchr(args, delim))) {
        *p = '\0';
        p++;
        alist[x++] = p;
        args = p;
        p = NULL;
    }

    return (x);
}

/* alloc/free/init */
icd_queue *create_icd_queue(char *name, icd_config * config)
{
    icd_queue *queue;
    icd_status result;

    /* make a new queue object from scratch */
    ICD_MALLOC(queue, sizeof(icd_queue));
    if (queue == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Queue\n");
        return NULL;
    }
    queue->state = ICD_QUEUE_STATE_CREATED;
    memset(&queue->chimeinfo, 0, sizeof(icd_queue_chimeinfo));
    memset(&queue->chimeinfo.chimedata, 0, sizeof(&queue->chimeinfo.chimedata));
    memset(&queue->chimeinfo.chimelist, 0, sizeof(&queue->chimeinfo.chimelist));
    queue->chimeinfo.chimepos = 0;
    queue->chimeinfo.chimelistlen = 0;
    queue->chimeinfo.chime_freq = 30;
    queue->chimeinfo.chime_repeat_to = 1;
    queue->chimeinfo.chimenext = 0;
    result = init_icd_queue(queue, name, config);
    if (result != ICD_SUCCESS) {
        ICD_FREE(queue);
        return NULL;
    }
    queue->allocated = 1;
    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, queue, queue->name, module_id, ICD_EVENT_CREATE, NULL,
        queue->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_queue(&queue);
        return NULL;
    }
    return queue;
}

icd_status destroy_icd_queue(icd_queue ** queuep)
{
    icd_status vetoed;
    int clear_result;

    assert(queuep != NULL);
    assert((*queuep) != NULL);

    /* First, notify event hooks and listeners */
    vetoed =
        icd_event_factory__generate(event_factory, *queuep, (*queuep)->name, module_id, ICD_EVENT_DESTROY, NULL,
        (*queuep)->listeners, NULL);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD Queue %s has been vetoed\n", icd_queue__get_name(*queuep));
        return ICD_EVETO;
    }

    /* Next, clear the queue object of any attributes and free dependent structures. */
    if ((clear_result = icd_queue__clear(*queuep)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Finally, destroy the queue object itself if from the heap */
    if ((*queuep)->allocated) {
        (*queuep)->state = ICD_QUEUE_STATE_DESTROYED;
        ICD_FREE((*queuep));
        *queuep = NULL;
    }
    return ICD_SUCCESS;
}

icd_status init_icd_queue(icd_queue * that, char *name, icd_config * config)
{
    icd_config *customers_config;
    icd_config *agents_config;
    icd_status vetoed;
    char buf[80];
    char *fieldval;

    assert(that != NULL);

    customers_config = NULL;
    agents_config = NULL;

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_queue));

    /* Set name before veto so we can report on what fails */
    if (name == NULL) {
        that->name = strdup("");
    } else {
        that->name = strdup(name);
    }

    cw_mutex_init(&that->lock);
    that->distributor = create_icd_distributor(name, config);
    if (config != NULL) {
        customers_config = icd_config__get_subset(config, "customers.");
        agents_config = icd_config__get_subset(config, "agents.");
    }

    fieldval = icd_config__get_param(config, "monitor_args");
    if (fieldval) {
        strncpy(that->monitor_args, fieldval, sizeof(that->monitor_args));
    }

    fieldval = icd_config__get_param(config, "priority");
    if (fieldval)
        that->priority = atoi(fieldval);

    fieldval = icd_config__get_param(config, "waittimeout");
    if (fieldval)
        that->wait_timeout = atoi(fieldval);

    fieldval = icd_config__get_param(config, "chimelist");
    if (fieldval) {
        strncpy(that->chimeinfo.chimedata, fieldval, sizeof(that->chimeinfo.chimedata));
        that->chimeinfo.chimelistlen = parse_list(that->chimeinfo.chimedata, ',', that->chimeinfo.chimelist);
    }
    fieldval = icd_config__get_param(config, "chime_repeat_to");
    if (fieldval)
        that->chimeinfo.chime_repeat_to = atoi(fieldval);

    if (that->chimeinfo.chime_repeat_to < 1 || that->chimeinfo.chime_repeat_to > that->chimeinfo.chimelistlen)
        that->chimeinfo.chime_repeat_to = 1;

    fieldval = icd_config__get_param(config, "chime_freq");
    if (fieldval)
        that->chimeinfo.chime_freq = atoi(fieldval);

    if (that->chimeinfo.chime_freq < 10)
        that->chimeinfo.chime_freq = 10;


    snprintf(buf, sizeof(buf), "Customers of Queue %s", name);
    that->customers = create_icd_member_list(buf, customers_config);
    snprintf(buf, sizeof(buf), "Agents of Queue %s", name);
    that->agents = create_icd_member_list(buf, agents_config);
    if (customers_config != NULL) {
        destroy_icd_config(&customers_config);
    }
    if (agents_config != NULL) {
        destroy_icd_config(&agents_config);
    }

    memset(&that->holdannounce, 0, sizeof(icd_queue_holdannounce));
    init_icd_queue_holdannounce(&that->holdannounce, config);

    that->params = NULL;
    if (config != NULL) {
        that->params = icd_config__get_any_value(config, "params", NULL);
    }
    that->listeners = create_icd_listeners();

    icd_queue__set_dump_func(that, icd_config__get_any_value(config, "dump", icd_queue__standard_dump),
        icd_config__get_any_value(config, "dump.extra", NULL));

    that->state = ICD_QUEUE_STATE_INITIALIZED;

    vetoed = icd_event__generate(ICD_EVENT_INIT, NULL);
    if (vetoed == ICD_EVETO) {
        icd_queue__clear(that);
        that->name = NULL;
        return ICD_EVETO;
    }
    return ICD_SUCCESS;
}

icd_status icd_queue__clear(icd_queue * that)
{
    icd_status vetoed;

    assert(that != NULL);

    /* Special case to deal with previously cleared */
    if (that->state == ICD_QUEUE_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_CLEAR, NULL);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Lock the object and remove everything in it. */
    if (icd_queue__lock(that) == ICD_SUCCESS) {
        that->state = ICD_QUEUE_STATE_CLEARED;
        if (that->name != NULL) {
            ICD_STD_FREE(that->name);
            that->name = NULL;
        }
        destroy_icd_distributor(&(that->distributor));
        that->distributor = NULL;

        if (that->customers != NULL) {
            destroy_icd_member_list(&(that->customers));
        }
        if (that->agents != NULL) {
            destroy_icd_member_list(&(that->agents));
        }
        memset(&that->holdannounce, 0, sizeof(icd_queue_holdannounce));

        if (that->listeners != NULL) {
            destroy_icd_listeners(&(that->listeners));
        }
        that->params = NULL;
        that->dump_fn = NULL;
        that->dump_fn_extra = NULL;

        icd_queue__unlock(that);
        cw_mutex_destroy(&(that->lock));
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Queue %s in order to clear it\n", icd_queue__get_name(that));
    return ICD_ELOCK;
}

/***** Behaviours *****/

/* Starts the queue's distributor so that it distributes calls. */
icd_status icd_queue__start_distributing(icd_queue * that)
{
    assert(that != NULL);
    assert(that->distributor != NULL);

    return icd_distributor__start_distributing(that->distributor);
}

/* Pauses the distributor of this queue next time it tries to link callers. */
icd_status icd_queue__pause_distributing(icd_queue * that)
{
    assert(that != NULL);
    assert(that->distributor != NULL);

    return icd_distributor__pause_distributing(that->distributor);
}

/* Stops the distributor thread for this queue. */
icd_status icd_queue__stop_distributing(icd_queue * that)
{
    assert(that != NULL);
    assert(that->distributor != NULL);

    return icd_distributor__stop_distributing(that->distributor);
}

/* called whenver someone enters or exits the queue to calculate the avg hold time */

icd_status icd_queue__calc_holdtime(icd_queue * that)
{
    icd_member *member;
    icd_caller *caller;
    icd_list_iterator *iter;
    time_t start, now;
    float total = 0, count = 0;
    int old = 0, new = 0;

    old = icd_queue__get_holdannounce_holdtime(that);
    icd_list__lock((icd_list *) (that->customers));
    iter = icd_list__get_iterator((icd_list *) (that->customers));
    while (icd_list_iterator__has_more_nolock(iter)) {
        member = (icd_member *) icd_list_iterator__next(iter);
        caller = icd_member__get_caller(member);
	if(caller == NULL) 
		continue;
        start = icd_caller__get_start(caller);
        time(&now);
        total += (now - start) / 60;
        count++;
    }
    icd_list__unlock((icd_list *) (that->customers));
    new = (total < 1 || count < 1) ? 0 : (int) (total / count);
    if (new != old) {
        cw_verbose("== APP_ICD: Setting hold time to %d minutes for queue %s == \n", new,
            icd_queue__get_name(that));
        icd_queue__set_holdannounce_holdtime(that, new);
    }
    destroy_icd_list_iterator(&iter);
    return ICD_SUCCESS;

}

/* Adds a customer to the queue */
icd_status icd_queue__customer_join(icd_queue * that, icd_member * member)
{
    icd_status vetoed;
    icd_caller *caller;
    char msg[120];

    assert(that != NULL);
    assert(member != NULL);

    /* First, notify event hooks and listeners 
       vetoed = icd_event__generate(ICD_EVENT_ADD, member);
     */
    caller = icd_member__get_caller(member);
    snprintf(msg, sizeof(msg), "name[%s] callerID[%s]->[%s]", icd_caller__get_name(caller), icd_caller__get_caller_id(caller), that->name);
    vetoed = icd_event__generate_with_msg(ICD_EVENT_ADD, msg, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    icd_caller__set_holdannounce(caller, &that->holdannounce);
    icd_caller__set_start_now(caller);
    /* TBD - We should consider an option to "Auto" distribute customers and/or agents */
    return icd_member_list__push(that->customers, member);
}

/* Removes a customer from the queue */
icd_status icd_queue__customer_quit(icd_queue * that, icd_member * member)
{
    icd_caller *caller;
    icd_status vetoed;

    assert(that != NULL);
    assert(that->distributor != NULL);
    assert(member != NULL);

    /* Check for valid caller */
    caller = icd_member__get_caller(member);
    if (caller == NULL || icd_caller__has_role(caller, ICD_CUSTOMER_ROLE) == 0) {
        cw_log(LOG_WARNING, "Invalid caller %s requesting to be removed from customer queue %s\n",
            icd_caller__get_name(caller), icd_queue__get_name(that));
        return ICD_ENOTFOUND;
    }
    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_REMOVE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Remove from both the distributor and the queue */
    icd_distributor__remove_customer(that->distributor, (icd_customer *) caller);
    return icd_member_list__remove_member_by_element(that->customers, member);
}

/* Tells the queue to add customer to distributor. Removes if already present. */
icd_status icd_queue__customer_distribute(icd_queue * that, icd_member * member)
{
    icd_status vetoed;
    icd_caller *caller;
    struct cw_channel *chan = NULL;
    char msg[120];

    assert(that != NULL);
    assert(that->distributor != NULL);
    assert(member != NULL);

    /* Check for veto
       vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTE, member);
     */
    caller = icd_member__get_caller(member);
    chan = icd_caller__get_channel(caller);
    snprintf(msg, sizeof(msg)-1, "Customer id[%d] name[%s] callerID[%s] to Queue[%s] Dist[%s]", icd_caller__get_id(caller),
        icd_caller__get_name(caller), icd_caller__get_caller_id(caller), that->name, (char *) vh_read(icd_distributor__get_params(that->distributor),
            "dist"));

    vetoed = icd_event__generate_with_msg(ICD_EVENT_DISTRIBUTE, msg, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return icd_distributor__add_customer(that->distributor, member);
}

/* Tells the queue to push the customer back on the top of the distributor. */
icd_status icd_queue__customer_pushback(icd_queue * that, icd_member * member)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(that->distributor != NULL);
    assert(member != NULL);

    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return icd_distributor__pushback_customer(that->distributor, member);
}

/*Count of active agents - states other that INITIALIZED and SUSPEND  */ 
int icd_queue__agent_active_count(icd_queue * that)
{
    icd_member *member;
    icd_caller *caller;
    icd_list_iterator *iter;
    int agent_count = 0;
    
    icd_list__lock((icd_list *) (that->agents));
    iter = icd_list__get_iterator((icd_list *) (that->agents));
    while (icd_list_iterator__has_more_nolock(iter)) {
        member = (icd_member *) icd_list_iterator__next(iter);
        caller = icd_member__get_caller(member);
	if((icd_caller__get_state(caller) != ICD_CALLER_STATE_INITIALIZED) 
	&& (icd_caller__get_state(caller) != ICD_CALLER_STATE_SUSPEND))
           agent_count++;
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock((icd_list *) (that->agents));
    return agent_count;

}


/* Adds an agent to the queue */
icd_status icd_queue__agent_join(icd_queue * that, icd_member * member)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(member != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__generate(ICD_EVENT_ADD, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* TBD - We should consider an option to "Auto" distribute customers and/or agents */
    return icd_member_list__push(that->agents, member);
}

/* Removes an agent from the queue */
icd_status icd_queue__agent_quit(icd_queue * that, icd_member * member)
{
    icd_caller *caller;
    icd_status vetoed;

    assert(that != NULL);
    assert(that->distributor != NULL);
    assert(member != NULL);

    /* Check for valid caller */
    caller = icd_member__get_caller(member);
    if (caller == NULL || icd_caller__has_role(caller, ICD_AGENT_ROLE) == 0) {
        cw_log(LOG_WARNING, "Invalid caller %s requesting to be removed from agent queue %s\n",
            icd_caller__get_name(caller), icd_queue__get_name(that));
    }
    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_REMOVE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Remove from both the distributor and the queue */

    cw_log(LOG_WARNING, "DEBUG, %d REMOVED FROM DIST\n", icd_caller__get_id(caller));
    icd_distributor__remove_agent(that->distributor, (icd_agent *) caller);
    return icd_member_list__remove_member_by_element(that->agents, member);
}

/* Removes an agent from the distributor of the queue */
icd_status icd_queue__agent_dist_quit(icd_queue * that, icd_member * member)
{
    icd_caller *caller;
    icd_status vetoed;

    assert(that != NULL);
    assert(member != NULL);

    if (that->distributor == NULL) {
        return ICD_SUCCESS;
    }
    /* Check for valid caller */
    caller = icd_member__get_caller(member);
    if (caller == NULL || icd_caller__has_role(caller, ICD_AGENT_ROLE) == 0) {
        cw_log(LOG_WARNING, "Invalid caller %s requesting to be removed from agent distributor %s\n",
            icd_caller__get_name(caller), icd_queue__get_name(that));
    }
    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_REMOVE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Remove from both the distributor and the queue */

    cw_log(LOG_WARNING, "DEBUG, %d REMOVED FROM DIST\n", icd_caller__get_id(caller));
    return icd_distributor__remove_agent(that->distributor, (icd_agent *) caller);
}

/* Removes a customer from the distributor of the queue */
icd_status icd_queue__customer_dist_quit(icd_queue * that, icd_member * member)
{
    icd_caller *caller;
    icd_status vetoed;

    assert(that != NULL);
    assert(member != NULL);

    if (that->distributor == NULL) {
        return ICD_SUCCESS;
    }
    /* Check for valid caller */
    caller = icd_member__get_caller(member);
    if (caller == NULL || icd_caller__has_role(caller, ICD_CUSTOMER_ROLE) == 0) {
        cw_log(LOG_WARNING, "Invalid caller %s requesting to be removed from customer distributor %s\n",
            icd_caller__get_name(caller), icd_queue__get_name(that));
    }
    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_REMOVE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }

    /* Remove from both the distributor and the queue */

    cw_log(LOG_WARNING, "DEBUG, %d REMOVED FROM DIST\n", icd_caller__get_id(caller));
    return icd_distributor__remove_customer(that->distributor, (icd_customer *) caller);
}
/* Tells the queue to add agent to distributor. Removes if already present. */
icd_status icd_queue__agent_distribute(icd_queue * that, icd_member * member)
{
    icd_status vetoed;
    icd_caller *caller;
    struct cw_channel *chan = NULL;
    char msg[120];

    assert(that != NULL);
    assert(member != NULL);

    /* Check for veto 
       vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTE, member);
     */
    caller = icd_member__get_caller(member);
    chan = icd_caller__get_channel(caller);
    snprintf(msg, sizeof(msg)-1, "Agent id[%d] name[%s] callerID[%s] to Queue[%s] Dist[%s]", icd_caller__get_id(caller),
        icd_caller__get_name(caller), icd_caller__get_caller_id(caller), that->name, (char *) vh_read(icd_distributor__get_params(that->distributor),
            "dist"));

    vetoed = icd_event__generate_with_msg(ICD_EVENT_DISTRIBUTE, msg, member);

    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return icd_distributor__add_agent(that->distributor, member);
}

/* Tells the queue to push back the agent to the top of the  distributor. */
icd_status icd_queue__agent_pushback(icd_queue * that, icd_member * member)
{
    icd_status vetoed;

    assert(that != NULL);
    assert(member != NULL);

    /* Check for veto */
    vetoed = icd_event__generate(ICD_EVENT_DISTRIBUTE, member);
    if (vetoed == ICD_EVETO) {
        return ICD_EVETO;
    }
    return icd_distributor__pushback_agent(that->distributor, member);
}

/* Create a machine parse able display of the queue */
icd_status icd_queue__dump(icd_queue * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(that->dump_fn != NULL);

    return that->dump_fn(that, verbosity, fd, that->dump_fn_extra);
    return ICD_SUCCESS;
}

/* Standard dump function for distributor */
icd_status icd_queue__standard_dump(icd_queue * that, int verbosity, int fd, void *extra)
{
    static char *indent = "    ";
    vh_keylist *keys;
    icd_distributor *dist;

    assert(that != NULL);

    cw_cli(fd, "\nDumping icd_queue {\n");
    cw_cli(fd, "%sname=%s (%s)\n", indent, icd_queue__get_name(that), that->allocated ? "alloced" : "on heap");

    cw_cli(fd, "%sparams {\n", indent);
    for (keys = vh_keys(that->params); keys; keys = keys->next)
        cw_cli(fd, "%s%s%s=%s\n", indent, indent, keys->name, (char *) vh_read(that->params, keys->name));

    cw_cli(fd, "%s}\n\n", indent);

    cw_cli(fd, "%sdump_fn=%p\n", indent, that->dump_fn);
    cw_cli(fd, "\n%s customers=%p {\n", indent, that->customers);
    if (verbosity > 1) {
        icd_member_list__dump(that->customers, verbosity - 1, fd);
    } else
        icd_member_list__dump(that->customers, 0, fd);

    cw_cli(fd, "%s}\n\n%sagents=%p  {\n", indent, indent, that->agents);
    if (verbosity > 1) {
        icd_member_list__dump(that->agents, verbosity - 1, fd);
    } else
        icd_member_list__dump(that->agents, 0, fd);
    cw_cli(fd, "%s}\n", indent);
    dist = (icd_distributor *) icd_queue__get_distributor(that);
    if (dist)
        icd_distributor__dump(dist, verbosity, fd);
    return ICD_SUCCESS;
}

/* Create a cli ui display of the queue */
icd_status icd_queue__show(icd_queue * that, int verbosity, int fd)
{
#define FMT_QUEUE_DATA "%-18s %-8d %-14d %-15d %-10d %-18d\n"

    assert(that != NULL);
    //vh_keylist *keys;

    cw_cli(fd, FMT_QUEUE_DATA, icd_queue__get_name(that), 
        icd_queue__get_agent_count(that),
        icd_queue__agent_active_count(that),
        icd_distributor__agents_pending(that->distributor),
        icd_queue__get_customer_count(that),
        icd_distributor__customers_pending(that->distributor));


    return ICD_SUCCESS;

}

/***** Getters and Setters *****/

/* Sets the name of the queue */
icd_status icd_queue__set_name(icd_queue * that, char *name)
{
    assert(that != NULL);

    if (that->name != NULL) {
        ICD_STD_FREE(that->name);
        that->name = NULL;
    }
    that->name = strdup(name);
    return ICD_SUCCESS;
}

/* Gets the name of the queue */
char *icd_queue__get_name(icd_queue * that)
{
    if (that->name == NULL) {
        return "";
    }
    return that->name;
}

char *icd_queue__get_monitor_args(icd_queue * that)
{
    return that->monitor_args[0] ? that->monitor_args : NULL;
}

char *icd_queue__check_recording(icd_queue *that, icd_caller *caller) 
{
    char *monitor_args = NULL;
    char buf[512], buf2[768];
    struct cw_channel *chan;
    struct tm *ptr;
    time_t tm;
 
    

    if ((monitor_args = icd_queue__get_monitor_args(that))) {
        tm = time(NULL);
        ptr = localtime(&tm);
        strftime(buf, sizeof(buf), monitor_args, ptr);
	strncpy(buf2, buf, sizeof(buf2));
        chan = icd_caller__get_channel(caller);
        if (!monitor_app) {
            monitor_app = pbx_findapp("Muxmon");
        }
        if (monitor_app && chan ) {
            pbx_substitute_variables_helper(chan, buf, buf2, sizeof(buf2));
            pbx_exec(chan, monitor_app, buf2);
        }
      
    }
    
    return monitor_args;
}

/* Return the number agents for this queue */
int icd_queue__get_agent_count(icd_queue * that)
{
    assert(that != NULL);
    assert(that->agents != NULL);

    return icd_list__count((icd_list *) (that->agents));
}

/* Return the number customer for this queue */
int icd_queue__get_customer_count(icd_queue * that)
{
    assert(that != NULL);
    assert(that->customers != NULL);

    return icd_list__count((icd_list *) (that->customers));
}

/* Gives the position of this agent in the list. */
int icd_queue__get_agent_position(icd_queue * that, icd_agent * target)
{
    icd_member *member;

    assert(that != NULL);
    assert(that->agents != NULL);
    assert(target != NULL);

    member = icd_caller__get_member_for_queue((icd_caller *) target, that);
    return icd_member_list__member_position(that->agents, member);
}

/* Gives the position of this customer in the list. */
int icd_queue__get_customer_position(icd_queue * that, icd_customer * target)
{
    icd_member *member;

    assert(that != NULL);
    assert(that->customers != NULL);
    assert(target != NULL);

    member = icd_caller__get_member_for_queue((icd_caller *) target, that);
    return icd_member_list__member_position(that->customers, member);
}

/* how long for customer wait b4 exiting the customer from the q if NO agents are in the q 
int icd_queue__get_noagent_timeout(icd_queue *that) {
    assert(that != NULL);
    
    return that->noagent_timeout;
}    
*/

/* how long for customer wait b4 exiting the customer from the q if agents are in the q*/
int icd_queue__get_wait_timeout(icd_queue * that)
{
    assert(that != NULL);

    return that->wait_timeout;
}

int icd_queue__get_holdannounce_cycle(icd_queue * that)
{
    assert(that != NULL);

    return that->holdannounce.cycle;
}

int icd_queue__get_holdannounce_frequency(icd_queue * that)
{
    assert(that != NULL);

    return that->holdannounce.frequency;
}

int icd_queue__get_holdannounce_holdtime(icd_queue * that)
{
    assert(that != NULL);
    assert(&that->holdannounce != NULL);
    return that->holdannounce.holdtime;
}

int icd_queue__set_holdannounce_holdtime(icd_queue * that, int time)
{
    assert(that != NULL);
    if (icd_queue__lock(that) == ICD_SUCCESS) {
        that->holdannounce.holdtime = time;
        icd_queue__unlock(that);
    }

    return that->holdannounce.holdtime;
}

char *icd_queue__get_holdannounce_sound_next(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_next == NULL || !strlen(that->holdannounce.sound_next)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-youarenext";
    }
    return that->holdannounce.sound_next;
}

char *icd_queue__get_holdannounce_sound_thereare(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_thereare == NULL || !strlen(that->holdannounce.sound_thereare)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-thereare";
    }
    return that->holdannounce.sound_thereare;
}

char *icd_queue__get_holdannounce_sound_calls(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_calls == NULL || !strlen(that->holdannounce.sound_calls)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-callswaiting";
    }
    return that->holdannounce.sound_calls;
}

char *icd_queue__get_holdannounce_sound_holdtime(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_holdtime == NULL || !strlen(that->holdannounce.sound_holdtime)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-holdtime";
    }
    return that->holdannounce.sound_holdtime;
}

char *icd_queue__get_holdannounce_sound_minutes(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_minutes == NULL || !strlen(that->holdannounce.sound_minutes)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-minutes";
    }
    return that->holdannounce.sound_minutes;
}

char *icd_queue__get_holdannounce_sound_thanks(icd_queue * that)
{
    assert(&that->holdannounce != NULL);
    if (that->holdannounce.sound_thanks == NULL || !strlen(that->holdannounce.sound_thanks)) {
        cw_log(LOG_WARNING, "This is not supposed to happen.\n");
        return "queue-thankyou";
    }
    return that->holdannounce.sound_thanks;
}

/* Sets the distributor that the queue uses */
icd_status icd_queue__set_distributor(icd_queue * that, icd_distributor * dist)
{
    assert(that != NULL);
    assert(dist != NULL);

    that->distributor = dist;
    return ICD_SUCCESS;

}

icd_status icd_queue__set_dump_func(icd_queue * that, icd_status(*dump_fn) (icd_queue *, int verbosity, int fd,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->dump_fn = dump_fn;
    that->dump_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Returns the distributor for this queue */
icd_distributor *icd_queue__get_distributor(icd_queue * that)
{
    assert(that != NULL);

    return that->distributor;
}

icd_member_list *icd_queue__get_customers(icd_queue * that)
{
    assert(that != NULL);

    return that->customers;
}

icd_member_list *icd_queue__get_agents(icd_queue * that)
{
    assert(that != NULL);

    return that->agents;
}

icd_queue_holdannounce *icd_queue__get_holdannounce(icd_queue * that)
{
    assert(that != NULL);

    return &that->holdannounce;
}

/***** Locking *****/

/* Lock the entire queue */
icd_status icd_queue__lock(icd_queue * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_QUEUE_STATE_CLEARED || that->state == ICD_QUEUE_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    retval = cw_mutex_lock(&that->lock);

    if (retval == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/* Unlock the entire queue */
icd_status icd_queue__unlock(icd_queue * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_QUEUE_STATE_CLEARED || that->state == ICD_QUEUE_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    retval = cw_mutex_unlock(&that->lock);
    if (retval == 0) {
        return ICD_SUCCESS;
    }
    return ICD_ELOCK;
}

/**** Listeners ****/

icd_status icd_queue__add_listener(icd_queue * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_queue__remove_listener(icd_queue * that, void *listener)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}

/**** Iterator functions ****/

/* Returns an iterator of customer callers in the queue. */
icd_list_iterator *icd_queue__get_customer_iterator(icd_queue * that)
{
    assert(that != NULL);

    return (icd_list__get_iterator((icd_list *) (that->customers)));
}

/* Returns an iterator of agent callers in the queue. */
icd_list_iterator *icd_queue__get_agent_iterator(icd_queue * that)
{
    assert(that != NULL);

    return (icd_list__get_iterator((icd_list *) (that->agents)));
}

/* Private APIs */

icd_queue_holdannounce *create_icd_queue_holdannounce(icd_config * config)
{
    icd_queue_holdannounce *announce;
    icd_status result;

    assert(config != NULL);

    announce = (icd_queue_holdannounce *) ICD_STD_MALLOC(sizeof(icd_queue_holdannounce));
    if (announce == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Queue Hold Announcements\n");
        return NULL;
    }
    memset(announce, 0, sizeof(icd_queue_holdannounce));

    result = init_icd_queue_holdannounce(announce, config);
    result = ICD_SUCCESS;
    if (result != ICD_SUCCESS) {
        ICD_STD_FREE(announce);
        return NULL;
    }
    announce->allocated = 1;

    return announce;
};

icd_status init_icd_queue_holdannounce(icd_queue_holdannounce * that, icd_config * config)
{
    char *type;
    char *tmp;

    assert(that != NULL);

    if (config != NULL) {
        that->frequency = icd_config__get_int_value(config, "hold.frequency", 0);
        type = icd_config__get_value(config, "hold.type");
        that->cycle = 0;        /* Default 0=never, -1=every time, 1=only once */
        if (type != NULL && strlen(type) > 0) {
            if (strcmp("all", type) == 0) {
                that->cycle = -1;
            } else if (strcmp("once", type) == 0) {
                that->cycle = 1;
            }
        }
        tmp = icd_config__get_param(config, "hold.youarenext");
        strncpy(that->sound_next, tmp ? tmp : "queue-youarenext", SOUND_FILE_SIZE);
        tmp = icd_config__get_param(config, "hold.thereare");
        strncpy(that->sound_thereare, tmp ? tmp : "queue-thereare", SOUND_FILE_SIZE);
        tmp = icd_config__get_param(config, "hold.callswaiting");
        strncpy(that->sound_calls, tmp ? tmp : "queue-callswaiting", SOUND_FILE_SIZE);
        tmp = icd_config__get_param(config, "hold.holdtime");
        strncpy(that->sound_holdtime, tmp ? tmp : "queue-holdtime", SOUND_FILE_SIZE);
        tmp = icd_config__get_param(config, "hold.minutes");
        strncpy(that->sound_minutes, tmp ? tmp : "queue-minutes", SOUND_FILE_SIZE);
        tmp = icd_config__get_param(config, "hold.thanks");
        strncpy(that->sound_thanks, tmp ? tmp : "queue-thankyou", SOUND_FILE_SIZE);

    }
    return ICD_SUCCESS;
}

icd_status destroy_icd_queue_holdannounce(icd_queue_holdannounce * that)
{
    assert(that != NULL);

    that->frequency = 0;
    that->cycle = 0;
    that->holdtime = 0;
    if (that && that->allocated)
        ICD_STD_FREE(that);

    return ICD_SUCCESS;
}

char *icd_queue__chime(icd_queue * queue, icd_caller * caller)
{
    char *ret = NULL;
    time_t now;
    int pos = 0;

    time(&now);

    if (icd_caller__get_chimenext(caller) >= now)
        return ret;

    pos = icd_caller__get_chimepos(caller);
    ret = queue->chimeinfo.chimelist[pos++];

    if (pos > queue->chimeinfo.chimelistlen)
        pos = queue->chimeinfo.chime_repeat_to - 1;

    icd_caller__set_chimepos(caller, pos);

    return ret;
}

int icd_queue__set_chime_freq(icd_queue * that, int freq)
{

    if (icd_queue__lock(that) == ICD_SUCCESS) {
        that->chimeinfo.chime_freq = freq;
        icd_queue__unlock(that);
    }
    return that->chimeinfo.chime_freq;
}

int icd_queue__get_chime_freq(icd_queue * that)
{
    return that->chimeinfo.chime_freq;
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

