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
 *  \brief icd_mod_agent_priorities - distributor strategy to prioritise on agents module
 * 
 * This is a custom call distribution strategy based on the following algo
 * agents are assignged to M priority groups see icd_config/icd_agent.conf-> priority=
 * agents in priority group N are are all called first at the same time
 * when the first agent in that groups times out or all members are busy
 * it then tries agents in priority groups N and N + 1
 * when the first agent in that groups times out or all members are busy
 * its then tries agents in priority groups N,N+1 and N +2 ... up to M
 *
 * 1. pop the customer off the dists customer list
 * 2. if the top agents on hook flag is off then bridge,
 *    then just distribute    else 
 * 3. x- link the customer and the agent
 *  -add agent to customer list
 *  -add customer to agent list
 *  -each customer will have an associate list cnt = to the number of
 *   agent being called
 *  -if agent if off hook then pop agent as wwell
 * 3. when customer is bridged to an agent
 *  -customer remove's from his associate list all agent that he is not
 *   bridged to, what about meetme's ?
 *  -if agent has no more associates in his list then he must hangup as
 *  well
 * 4. if customer has a agent time out
 *  then the agent checks the customer associate list to and if only
 *  this agent is in the list he sets the state on the customer to
 *  get_chan bridge failed
 *
 * 5.) if customer hangs up then he checks if the customer list is the dist is empty
 *  if so then he removes all association on agent that are not BRDIGED &
 *  hangus em u
 *  
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include "callweaver/icd/icd_module_api.h"

/* public apis */
int icd_module_command_agtpri(int fd, int argc, char **argv);
static icd_plugable_fn *icd_module_get_plugable_fns(icd_caller * that);

/* Private implemenations */
static int module_id = 0;
static char *module_name = "Agent_Priority_Groups";

/* maybe we just use the agent struc by default b4  we have an active member 
 * also might use a macro ICD_PLUGABLE_STANDARD; ...*/
static icd_plugable_fn icd_module_plugable_fns;
static icd_plugable_fn icd_module_agent_plugable_fns;
static icd_plugable_fn icd_module_customer_plugable_fns;
static icd_status init_icd_distributor_agent_priority_groups(icd_distributor * that, char *name, icd_config * data);
static icd_status link_callers_via_pop_customer_ring_agent_priority_groups(icd_distributor * dist, void *extra);

/*general handlers for dist */
static int icd_module__state_bridge_failed(icd_event * event, void *extra);
static int icd_module__state_channel_failed(icd_event * event, void *extra);
static int icd_module__state_associate_failed(icd_event * event, void *extra);
static int icd_module__limited_ready_state_on_fail(icd_event * event, void *extra);
static int icd_module__pushback_and_ready_on_fail(icd_event * event, void *extra);

/*agent handlers for dist */
static int icd_module__agent_state_bridged(icd_event * event, void *extra);

/*customer handlers for dist */
static int icd_module__customer_state_bridged(icd_event * event, void *extra);

int icd_module_load(icd_config_registry * registry)
{

    assert(registry != NULL);

    icd_config_registry__register_ptr(registry, "dist", "agentprioritygroups",
        init_icd_distributor_agent_priority_groups);

    module_id = icd_event_factory__add_module(module_name);
    if (module_id == 0)
        cw_log(LOG_WARNING, "Unable to register Module Name[%s]", module_name);

    cw_verbose(VERBOSE_PREFIX_3 "Registered ICD Module[%s]!\n", icd_module_strings[module_id]);

    return 0;
}

int icd_module_unload(void)
{
    /* verify you clean up any mem that was created in this module, 
     * exceptions are objects that are part of caller queues or distributotrs they
     * all have there own clean up
     */
    icd_plugable__clear_fns(&icd_module_plugable_fns);
    icd_plugable__clear_fns(&icd_module_agent_plugable_fns);
    icd_plugable__clear_fns(&icd_module_customer_plugable_fns);
    cw_verbose(VERBOSE_PREFIX_3 "Unloaded ICD Module[[agentprioritygroups]!\n");

    return 0;

}

static icd_status init_icd_distributor_agent_priority_groups(icd_distributor * that, char *name, icd_config * data)
{
    icd_status result;
    icd_plugable_fn *plugable_fns;
    char buf[ICD_STRING_LEN];

    assert(that != NULL);
    assert(data != NULL);

    strncpy(that->name, name, sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *) that->agents, icd_list__insert_ordered,
        icd_member__cmp_priority_order);

    icd_distributor__set_link_callers_fn(that, link_callers_via_pop_customer_ring_agent_priority_groups, NULL);
    /* need to set customization points for Distributors plugable functions */
    icd_distributor__set_plugable_fn_ptr(that, &icd_module_get_plugable_fns);

    ICD_MEMSET_ZERO(&icd_module_plugable_fns, sizeof(icd_module_plugable_fns));
    snprintf(buf, sizeof(buf), "PriorityGroupsDefault");
    result = init_icd_plugable_fns(&icd_module_plugable_fns, buf, data);

    ICD_MEMSET_ZERO(&icd_module_agent_plugable_fns, sizeof(icd_module_agent_plugable_fns));
    snprintf(buf, sizeof(buf), "PriorityGroupsAgentDefault");
    result = init_icd_plugable_fns(&icd_module_agent_plugable_fns, buf, data);

    ICD_MEMSET_ZERO(&icd_module_customer_plugable_fns, sizeof(icd_module_customer_plugable_fns));
    snprintf(buf, sizeof(buf), "PriorityGroupsCustomerDefault");
    result = init_icd_plugable_fns(&icd_module_customer_plugable_fns, buf, data);

    /* Now customize plugable interface points for each of the above function ptr strucs */
    plugable_fns = &icd_module_agent_plugable_fns;
    result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_agent__standard_state_call_end, NULL);
    result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_agent__standard_cleanup_caller);
    result = icd_plugable__set_state_bridged_fn(plugable_fns, icd_module__agent_state_bridged, NULL);

/*
    result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_module__standard_state_call_end, NULL);
    result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_module__standard_cleanup_caller);
*/
    plugable_fns = &icd_module_customer_plugable_fns;
    result = icd_plugable__set_state_call_end_fn(plugable_fns, icd_customer__standard_state_call_end, NULL);
    result = icd_plugable__set_cleanup_caller_fn(plugable_fns, icd_customer__standard_cleanup_caller);
    result = icd_plugable__set_state_bridged_fn(plugable_fns, icd_module__customer_state_bridged, NULL);

    result = icd_plugable__set_state_associate_failed_fn(plugable_fns, icd_module__state_associate_failed, NULL);
    result = icd_plugable__set_state_channel_failed_fn(plugable_fns, icd_module__state_channel_failed, NULL);
    result = icd_plugable__set_state_bridge_failed_fn(plugable_fns, icd_module__state_bridge_failed, NULL);

    icd_distributor__create_thread(that);
    cw_verbose(VERBOSE_PREFIX_3 "ICD Distributor[%s] Initialized !\n", name);

    return ICD_SUCCESS;
}

int icd_module_command_agtpri(int fd, int argc, char **argv)
{
    static char *help[2] = { "help", "agtpri" };

    if (argc >= 2) {
        cw_cli(fd, "\n");
        cw_cli(fd, "\n");
        cw_cli(fd, "ICD Module loaded a icd command interface \n");

        cw_cli(fd, "\n");
        cw_cli(fd, "\n");
    } else
        icd_command_help(fd, 2, help);

    return 0;
}

icd_plugable_fn *icd_module_get_plugable_fns(icd_caller * that)
{
    icd_plugable_fn *plugable_fns = NULL;
    char *dist_name = NULL;

    assert(that != NULL);

    if (icd_caller__get_active_member(that) == NULL) {
        /* always return our standard plugable interface when no distributor */
        plugable_fns = &icd_module_plugable_fns;

    } else {
        dist_name =
            vh_read(icd_distributor__get_params(icd_member__get_distributor(icd_caller__get_active_member(that))),
            "dist");
        /* sanity check did we get called with our distributor dist="agent.priority.group" 
           if  (strcmp(dist_name, ICD_MODULE) == 0) {
           }
         */
        plugable_fns = icd_plugable_fn_list__fetch_fns(icd_caller__get_plugable_fns_list(that), dist_name);
        if (plugable_fns == NULL) {
            /* if we fetched the caller specific plugab_fnlist for this dist and found 
             * no caller specfic plugable defined 
             * then we just go with our static local interface for agent or customer
             */
            if (icd_caller__has_role(that, ICD_AGENT_ROLE))
                plugable_fns = &icd_module_agent_plugable_fns;
            else
                plugable_fns = &icd_module_customer_plugable_fns;
        }

    }

    if (plugable_fns == NULL) {
        cw_log(LOG_ERROR, "Caller %d [%s] has no plugable fn aborting ala crash\n", icd_caller__get_id(that),
            icd_caller__get_name(that));
    } else {
        if (icd_debug)
            cw_log(LOG_DEBUG, "\nCaller id[%d] [%s] using icd_module_plugable_fns[%s] ready_fn[%p] for Dist[%s]\n",
                icd_caller__get_id(that), icd_caller__get_name(that), icd_plugable__get_name(plugable_fns),
                plugable_fns->state_ready_fn, dist_name);
    }
    assert(plugable_fns != NULL);
    return plugable_fns;

}

/*
static icd_status icd_module__init_agent(icd_agent *that,icd_config *data); 
static icd_status icd_module__init_customer(icd_customer *that, icd_config *data);

static icd_status icd_module__init_caller(icd_caller *that,icd_config *data) {
    assert(that != NULL);
    icd_caller *caller = (icd_caller *)that;
    icd_plugable_fn *plugable_fns =NULL;
    icd_list_iterator *iter;
    
    return ICD_SUCCESS;
}

static icd_status icd_module__init_agent(icd_agent *that,icd_config *data) {
    assert(that != NULL);

    icd_caller *caller = (icd_caller *)that;
    icd_plugable_fn *plugable_fns =NULL;
    icd_list_iterator *iter;

    
    return ICD_SUCCESS;
}

static icd_status icd_module__init_customer(icd_customer *that, icd_config *data) {
    assert(that != NULL);

    return ICD_SUCCESS;
}        
*/
static icd_status link_callers_via_pop_customer_ring_agent_priority_groups(icd_distributor * dist, void *extra)
{
    icd_member *customer_member = NULL;
    icd_member *agent_member = NULL;
    icd_caller *customer_caller = NULL;
    icd_caller *agent_caller = NULL;
    int cust_id;
    int agent_id;
    int first_agent_id = -1;
    icd_status result;

    char *tmp_str;
    int agent_priority = 0;
    char agent_priority_str[3];
    int match_found = 1;
    int match_count = 0;

    icd_list_iterator *iter;

    assert(dist != NULL);
    assert(dist->customers != NULL);
    assert(dist->agents != NULL);

    if (!icd_member_list__has_members(dist->customers) || !icd_member_list__has_members(dist->agents)) {
        return ICD_ENOTFOUND;
    }

    customer_member = icd_member_list__pop(dist->customers);
    customer_caller = icd_member__get_caller(customer_member);
    if (customer_member == NULL || customer_caller == NULL) {
        cw_log(LOG_ERROR, "ICD Distributor %s could not retrieve customer from list\n",
            icd_distributor__get_name(dist));
        return ICD_ERESOURCE;
    }
    tmp_str = icd_caller__get_param(customer_caller, "agent_priority");
    if (tmp_str != NULL) {
        agent_priority = atoi(tmp_str);
    }
    agent_priority++;
    snprintf(agent_priority_str, 3, "%d", agent_priority);
    icd_caller__set_param_string(customer_caller, "agent_priority", agent_priority_str);
    tmp_str = icd_caller__get_param(customer_caller, "agent_priority");

    result = icd_member__distribute(customer_member);
    if (result != ICD_SUCCESS) {
        /* Some other distributor got to this customer first */
        result = ICD_SUCCESS;   /* does this matter */
        return result;
    }
    cust_id = icd_caller__get_id(customer_caller);
    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD AgentPriorityDist %s found customer[%s] agent_priority[%d] from list\n",
            icd_distributor__get_name(dist), icd_caller__get_name(customer_caller), agent_priority);

    iter = icd_distributor__get_agent_iterator(dist);
    while (match_found == 1 && icd_list_iterator__has_more(iter)) {
        //agent_member = icd_member_list__pop(dist->agents);
        agent_member = (icd_member *) icd_list_iterator__next(iter);
        if (agent_member == NULL) {
            cw_log(LOG_ERROR, "ICD Distributor %s could not pop agent member from list\n",
                icd_distributor__get_name(dist));
            return ICD_ERESOURCE;
        }
        agent_caller = icd_member__get_caller(agent_member);
        if (agent_caller == NULL) {
            cw_log(LOG_ERROR, "ICD Distributor %s could not cast agent caller from member\n",
                icd_distributor__get_name(dist));
            return ICD_ERESOURCE;
        }
        if (icd_caller__get_priority(agent_caller) > agent_priority) {
            match_found = 0;
            /* only assign agents with a priority less than or equal to the last groups of agents 
             * that we attempted to bridge this customer caller
             * if this agent is the wrong priority just stuff him back on the dist and get outa here
             */
            //icd_distributor__pushback_agent(dist, agent_member);
            if (match_count == 0) {
                /* found no agents to call, stick customer back on list & either timeout or get new agents */
                icd_distributor__pushback_customer(dist, customer_member);
            }
            cw_log(LOG_WARNING, "ICD AgentPriorityDist[%s] need priority[%d] found Agent[%s] WRONG Priority[%d]\n",
                icd_distributor__get_name(dist), agent_priority, icd_caller__get_name(agent_caller)
                , icd_caller__get_priority(agent_caller));
            return ICD_EEXISTS;
        }

        if (icd_debug)
            cw_log(LOG_DEBUG, "ICD AgentPriorityDist %s found Agent Caller[%s] priority[%d] from list\n",
                icd_distributor__get_name(dist), icd_caller__get_name(agent_caller),
                icd_caller__get_priority(agent_caller));

        agent_id = icd_caller__get_id(agent_caller);

        /* Make sure we haven't looped through all agents already */
        if (agent_id == first_agent_id) {
            icd_distributor__pushback_agent(dist, agent_member);
            if (match_count == 0) {
                /* found no agents to call, stick customer back on list & either timeout or get new agents */
                icd_distributor__pushback_customer(dist, customer_member);
            }
            return ICD_ENOTFOUND;
        }
        if (first_agent_id == -1) {
            first_agent_id = agent_id;
        }

        /* ok this guy is good to go what about other dists they domt seem to check the state 
         * when they pop callers, so if thes eguy is in differentq do we have an issue 
         */
        result = icd_member__distribute(agent_member);
        if (result != ICD_SUCCESS) {
            return result;
        }

        result = icd_caller__join_callers(customer_caller, agent_caller);

        /* Figure out who the bridger is, and who the bridgee is */
        result = icd_distributor__select_bridger(agent_caller, customer_caller);

        cw_verbose(VERBOSE_PREFIX_3 "Distributor[%s] Link CustomerID[%d] to AgentID[%d]\n",
            icd_distributor__get_name(dist), cust_id, agent_id);
        if (icd_caller__has_role(customer_caller, ICD_BRIDGER_ROLE)) {
            result = icd_caller__bridge(customer_caller);
        } else if (icd_caller__has_role(agent_caller, ICD_BRIDGER_ROLE)) {
            result = icd_caller__bridge(agent_caller);
        } else {
            cw_log(LOG_ERROR, "ICD Distributor %s found no bridger responsible to bridge call\n",
                icd_distributor__get_name(dist));
            icd_distributor__pushback_agent(dist, agent_member);
            icd_distributor__pushback_customer(dist, customer_member);
            return ICD_EGENERAL;
        }

        icd_caller__dump_debug(agent_caller);

        match_count++;

        if (icd_caller__get_onhook(agent_caller) != 0)
            icd_distributor__remove_caller(dist, agent_caller); /*off hook is always a winner */
        /* 
           icd_member_list__remove_member_by_element(dist->agents, agent_member);
           icd_list__remove_by_element((icd_list *)dist->agents, agent_member);
         */
        break;

    }                           /* while poping agents with priority <= current agent_priority as the customer */

    icd_caller__dump_debug(customer_caller);
    //icd_caller__dump_debug(agent_caller);

    return ICD_SUCCESS;
}

/* agent reacting to the bridged state */
static int icd_module__agent_state_bridged(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_caller *associate;
    icd_list_iterator *iter;
    struct cw_channel *chan;
    struct cw_channel *chan_associate;
    icd_status result;
    icd_status final_result;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

    switch (icd_caller__get_roles(that)) {
    case ICD_BRIDGER_ROLE:
        /* agent pops self off the dist, check role for right list */
        icd_distributor__remove_caller(icd_member__get_distributor(icd_caller__get_active_member(that)), that);
        /*now we need to remove any associations that are not who we bridged to) */
        chan = icd_caller__get_channel(that);
        iter = icd_list__get_iterator((icd_list *) (icd_caller__get_associations(that)));
        while (icd_list_iterator__has_more(iter)) {
            associate = (icd_caller *) icd_list_iterator__next(iter);
            /* ok this a qualified guy BUT we may have a race in ring N associates 
             * he might bridged to someone else & his state maybe bridged
             * we now gota see if we bridged to him by chk the as bridge chan name
             * maybe want to x-pollenate this guys params with their icd_caller id
             */
            switch (icd_caller__get_bridge_technology(that)) {
            case ICD_BRIDGE_STANDARD:
                chan_associate = icd_caller__get_channel(associate);
                if (chan && chan_associate && chan->name != cw_bridged_channel(chan_associate)->name) {
                    result = icd_caller__unlink_from_caller(that, associate);
                    if (result != ICD_SUCCESS)
                        final_result = result;
                }
                break;
            case ICD_BRIDGE_CONFERENCE:
                if (icd_caller__get_conference(that) != icd_caller__get_conference(associate)) {
                    result = icd_caller__unlink_from_caller(that, associate);
                    if (result != ICD_SUCCESS)
                        final_result = result;
                }
                break;
            }                   /*switch on bridge_tech */
        }                       /* while iter has more */
        destroy_icd_list_iterator(&iter);
        break;

    case ICD_BRIDGEE_ROLE:
        cw_log(LOG_ERROR, "Caller id[%d] [%s] in role[ICD_AGENT_ROLE] cant be bridgee using [%s]  \n",
            icd_caller__get_id(that), icd_caller__get_name(that), module_name);

        break;
    }                           /* switch on icd_caller__get_roles */

    icd_caller__set_start_now(that);
    icd_caller__set_callcount(that, -1);        /* increments the count */
    return 0;
}

/* customer reacting to the bridged state */
static int icd_module__customer_state_bridged(icd_event * event, void *extra)
{
    icd_caller *that;
    icd_caller *associate;
    icd_list_iterator *iter;

    struct cw_channel *chan;
    struct cw_channel *chan_associate;
    int link_count = 0;
    icd_status result;
    icd_status final_result;

    assert(event != NULL);
    that = (icd_caller *) icd_event__get_source(event);
    assert(that != NULL);

    switch (icd_caller__get_roles(that)) {
    case ICD_BRIDGER_ROLE:
        cw_log(LOG_ERROR, "Caller id[%d] [%s] in role[ICD_CUSTOMER_ROLE] cant be bridger using [%s]  \n",
            icd_caller__get_id(that), icd_caller__get_name(that), module_name);
        break;

    case ICD_BRIDGEE_ROLE:

        /*now we need to remove any associations that are not who we bridged to) */
        chan = icd_caller__get_channel(that);
        iter = icd_list__get_iterator((icd_list *) (icd_caller__get_associations(that)));
        while (icd_list_iterator__has_more(iter)) {
            associate = (icd_caller *) icd_list_iterator__next(iter);
            link_count = icd_list__count((icd_list *) (icd_caller__get_associations(associate)));
            /* ok this a qualified guy BUT we may have a race in ring N associates 
             * he might bridged to someone else & his state maybe bridged
             * we now gota see if we bridged to him by chk the as bridge chan name
             * maybe want to x-pollenate this guys params with their icd_caller id
             */
            switch (icd_caller__get_bridge_technology(that)) {
            case ICD_BRIDGE_STANDARD:
                chan_associate = icd_caller__get_channel(associate);
                if (chan && chan_associate && chan->name != cw_bridged_channel(chan_associate)->name) {
                    result = icd_caller__unlink_from_caller(that, associate);
                    if (result != ICD_SUCCESS)
                        final_result = result;
                    else
                        link_count--;
                }
                /* check if this associate has any other associations 
                 *  if so leave them alone if not then ask em to hang up
                 */
                /* hangup any OnHook callers stop em ringing TC lock the list */
                /* the READY state */
                if (!icd_caller__get_onhook(associate) && link_count == 0) {
                    icd_bridge__safe_hangup(associate);
                    icd_caller__reset_pushback(associate);      /* dont want agent on top */
                    /* ready -> icd_distributor__add_agent remove & re-add in sort sequence */
                    icd_caller__set_state(associate, ICD_CALLER_STATE_READY);
                }

                break;
            case ICD_BRIDGE_CONFERENCE:
                if (icd_caller__get_conference(that) != icd_caller__get_conference(associate)) {
                    result = icd_caller__unlink_from_caller(that, associate);
                    if (result != ICD_SUCCESS) {
                        final_result = result;
                    }
                }
                break;
            }                   /*switch on bridge_tech */
        }                       /* while iter has more */
        destroy_icd_list_iterator(&iter);
        break;
    }                           /* switch on icd_caller__get_roles */

    icd_caller__set_start_now(that);
    icd_caller__set_callcount(that, -1);        /* increments the count */
    return 0;
}

static int icd_module__state_bridge_failed(icd_event * event, void *extra)
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
    icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_BRIDGE_FAILED);

    return icd_module__pushback_and_ready_on_fail(event, extra);

}

static int icd_module__state_channel_failed(icd_event * event, void *extra)
{
    assert(event != NULL);

    return icd_module__limited_ready_state_on_fail(event, extra);
}

static int icd_module__state_associate_failed(icd_event * event, void *extra)
{
    assert(event != NULL);
    /*
       that = (icd_caller *)icd_event__get_source(event);
       assert(that != NULL);
     */

    return icd_module__pushback_and_ready_on_fail(event, extra);
}

static int icd_module__pushback_and_ready_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;
    int *value = 0;

    assert(event != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

    /* increment the count of the number of failures */
    value = (int *) extra;

    //(*value)++;

    icd_caller__set_pushback(that);
    icd_caller__set_state(that, ICD_CALLER_STATE_READY);

    /* No veto allowed here */
    return 0;
}

static int icd_module__limited_ready_state_on_fail(icd_event * event, void *extra)
{
    icd_caller *that;
    int *value;

    assert(event != NULL);
    assert(extra != NULL);

    that = icd_event__get_source(event);
    assert(that != NULL);

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
    /* that->cleanup_caller_fn(that); standard is in icd_agent/customer.c */
    /* No veto allowed here */
    return 0;
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

