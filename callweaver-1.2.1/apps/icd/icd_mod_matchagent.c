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
  *  \brief icd_mod_matchagent.c - module that matches a customer to a defined agent in a queue
  */
  
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  


#include "callweaver/icd/icd_module_api.h"

#include "callweaver/icd/icd_member_list.h"


extern icd_fieldset *agents;
extern int icd_verbose;
icd_status link_callers_via_pop_customer_match_agent(icd_distributor * dist, void *extra);
                                           
static void *icd_distributor__match_agent_run(void *that) 
{ 
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
		while(icd_distributor__get_added_callers_number(dist) > 0){
			icd_distributor__reset_added_callers_number(dist);
			if (icd_distributor__customers_pending(dist) && icd_distributor__agents_pending(dist)) { 
				result = icd_distributor__unlock(dist); 
				/* func ptr to the icd_distributor__link_callers_via_?? note may also come from custom 
				* function eg from icd_mod_?? installed using icd_distributor__set_link_callers_fn 
				*/ 
				if (icd_verbose > 4) 
				cw_verbose(VERBOSE_PREFIX_3 "Distributor__run [%s] link_fn[%p]  \n",  
					icd_distributor__get_name(dist), dist->link_fn); 
				result = dist->link_fn(dist, dist->link_fn_extra);   
				result = icd_distributor__lock(dist); 
			}
		} 
		/* All distributor links are now created wait for changes of customer or agent list  */      
		cw_cond_wait(&(dist->wakeup), &(dist->lock)); /* wait until signal received */ 
		result = icd_distributor__unlock(dist); 
		if (icd_verbose > 4) 
			cw_verbose(VERBOSE_PREFIX_3 "Distributor__run [%s] wait  \n",  icd_distributor__get_name(dist));             
		} else { 
	            /* TBD - Make paused thread work better.  
	             *        - Use pthread_cond_wait() 
	             *        - Use same or different condition variable?  
	             */ 
 	        }
	        /* Play nice */ 
	        sched_yield(); 
	    } 
	    /* Do any cleanup here */ 
	    return NULL; 
} 

static icd_status init_icd_distributor_match_agent(icd_distributor * that, char *name, icd_config * data)
{

    assert(that != NULL);
    assert(data != NULL);
    strncpy(that->name, name, sizeof(that->name));
    icd_distributor__set_config_params(that, data);
    icd_distributor__create_lists(that, data);
    icd_list__set_node_insert_func((icd_list *) that->agents, icd_list__insert_fifo, NULL);
    icd_distributor__set_link_callers_fn(that, link_callers_via_pop_customer_match_agent, NULL);
	icd_distributor__set_run_fn(that, icd_distributor__match_agent_run);  
    icd_distributor__create_thread(that);

    cw_verbose(VERBOSE_PREFIX_3 "Registered ICD Distributor[%s] Initialized !\n", name);
    return ICD_SUCCESS;
}

int icd_module_load(icd_config_registry * registry)
{

    assert(registry != NULL);
    icd_config_registry__register_ptr(registry, "dist", "matchagent", init_icd_distributor_match_agent);

    icd_config_registry__register_ptr(registry, "dist.link", "popcustmatchagent",
      link_callers_via_pop_customer_match_agent  );

    cw_verbose(VERBOSE_PREFIX_3 "Registered ICD Module[MatchAgent]!\n");

    return 0;
}

int icd_module_unload(void)
{
    /*TODO didnt get this far */
    cw_verbose(VERBOSE_PREFIX_3 "Unloaded ICD Module[MatchAgent]!\n");
    return 0;

}

/* Match the customers "identifier" to the agents "identifier".
 * the "identifier" is set to what ever value is required typical use case
 * agent identifier=agent_id and the customer identifier = agent_id
 * exten => 9001,1,icd_customer(identifier=123|name=roundrobin-customer|queue=roundrobin_q|moh=ringing)
 * exten => 8001,1,icd_agent(identifier=123|agent=101|noauth=1|queue=test_q
 * */
icd_status link_callers_via_pop_customer_match_agent(icd_distributor * dist, void *extra)
{
    icd_member *customer = NULL;
    icd_member *agent = NULL;
    icd_caller *customer_caller = NULL;
    icd_caller *agent_caller = NULL;
    char *tmp_str;
    icd_list_iterator *iter;
    icd_status result;
    int LinkFound;

    assert(dist != NULL);
    assert(dist->customers != NULL);
    assert(dist->agents != NULL);
    if (!icd_member_list__has_members(dist->customers)) {
        return ICD_ENOTFOUND;
    }
    if (!icd_member_list__has_members(dist->agents)) {
        return ICD_ENOTFOUND;
    }

    /* Go through all the customers looking for a match */
    /* For each customer on the list, try to find the match agent. 
     * Return after all customer has been pass looked over once */
	while(1){
		LinkFound = 0;
		result = icd_member_list__lock(dist->customers);
		iter = icd_distributor__get_customer_iterator(dist);
		while (icd_list_iterator__has_more_nolock(iter)) {
			customer = (icd_member *) icd_list_iterator__next(iter);
			if (customer != NULL) {
				customer_caller = icd_member__get_caller(customer);
			}    
			if (customer == NULL || customer_caller == NULL) {
				cw_log(LOG_ERROR, "MatchAgent Distributor %s could not retrieve customer from list\n",
				icd_distributor__get_name(dist));
				continue;
			}
	
			tmp_str = icd_caller__get_param(customer_caller, "identifier");
	
			if (tmp_str == NULL) {
				cw_log(LOG_WARNING, "MatchAgent Distributor [%s] reports that customer [%s] has no identifier\n",
				icd_distributor__get_name(dist), icd_caller__get_name(customer_caller));
				continue;
			}
			agent_caller = (icd_caller *) icd_fieldset__get_value(agents, tmp_str);   
			if (agent_caller == NULL) {
				cw_log(LOG_WARNING, "MatchAgent Distributor [%s] reports that agent [%s] is not in ICD\n",
				icd_distributor__get_name(dist), tmp_str);
				continue;
			}
			if(icd_caller__get_state(agent_caller) != ICD_CALLER_STATE_READY){
				continue;
			}       
			icd_member_list__lock(dist->agents);		
			agent = icd_member_list__get_for_caller(dist->agents, agent_caller);
			if (agent == NULL ) {
			/*  cw_log(LOG_WARNING, "MatchAgent Distributor [%s] reports that agent [%s] is not in distributor\n",
                	icd_distributor__get_name(dist), tmp_str);
			*/                
				icd_member_list__unlock(dist->agents);		
				continue;
        		}
			if (icd_member__lock(agent) != ICD_SUCCESS){
				icd_member_list__unlock(dist->agents);		
				continue;
			}
			icd_member_list__unlock(dist->agents);
			result = icd_member__distribute(agent);
			if (result != ICD_SUCCESS) {
				cw_log(LOG_WARNING, "MatchAgent Distributor [%s] reports that cannot distribute agent [%s]\n",
				icd_distributor__get_name(dist), tmp_str);
				icd_member__unlock(agent);
				continue;
			}
			
			if(icd_caller__get_state(customer_caller) == ICD_CALLER_STATE_READY){
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
			}	
		
			if (result != ICD_SUCCESS) {
				icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);   
				icd_member__unlock(agent);
				continue;
			}
			
			result = icd_member__distribute(customer);
			if (result != ICD_SUCCESS) {
				icd_caller__unlock(customer_caller);
				icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
				icd_member__unlock(agent);
				continue;
			}
			LinkFound = 1;
			icd_member_list__unlock(dist->customers);
			result = icd_caller__join_callers(customer_caller, agent_caller);
			if (result != ICD_SUCCESS) {
				icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
				icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
				icd_caller__unlock(customer_caller);
				icd_member__unlock(agent);
			}
			else {

			/* Figure out who the bridger is, and who the bridgee is */
				result = icd_distributor__select_bridger(agent_caller, customer_caller);

				cw_verbose(VERBOSE_PREFIX_3 "MatchAgent Distributor [%s] Link CustomerID[%d] to AgentID[%d]\n",
				icd_distributor__get_name(dist), icd_caller__get_id(customer_caller), icd_caller__get_id(agent_caller));
				if (icd_caller__has_role(customer_caller, ICD_BRIDGER_ROLE)) {
					result = icd_caller__bridge(customer_caller);
				} else if (icd_caller__has_role(agent_caller, ICD_BRIDGER_ROLE)) {
					result = icd_caller__bridge(agent_caller);
				} else {
					cw_log(LOG_ERROR, "MatchAgent Distributor %s found no bridger responsible to bridge call\n",
					icd_distributor__get_name(dist));
					icd_caller__set_state(agent_caller, ICD_CALLER_STATE_READY);
					icd_caller__set_state(customer_caller, ICD_CALLER_STATE_READY);
					icd_caller__unlock(customer_caller);
					icd_member__unlock(agent);
					continue;
				}
			}
			if( LinkFound) {
				icd_caller__unlock(customer_caller);
				icd_member__unlock(agent);
				break;
			}
      		} /*  while (icd_list_iterator__has_more(iter)) */
		if(! LinkFound) {
			result = icd_member_list__unlock(dist->customers);
		}
		destroy_icd_list_iterator(&iter);
		if(! LinkFound) {
			break;
		}    
	}
	return ICD_SUCCESS;
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


