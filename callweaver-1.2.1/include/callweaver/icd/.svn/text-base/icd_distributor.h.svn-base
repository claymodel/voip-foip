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
 *  \brief icd_distributor.h  -  a strategy to assign a call to an agent
 *
 * The icd_distributor module holds a set of all agents and customers that can
 * talk to one another, as well as a thread of execution that can bridges the
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

#ifndef ICD_DISTRIBUTOR_H
#define ICD_DISTRIBUTOR_H
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/voidhash.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Init - Destroyer *****/

/* Create a distributor. data is a parsable string of parameters. */
    icd_distributor *create_icd_distributor(char *name, icd_config * data);

/* Destroy a distributor. */
    icd_status destroy_icd_distributor(icd_distributor ** distp);

/* Initialize a distributor. */
    icd_status init_icd_distributor(icd_distributor * distributor, char *name, icd_config * data);

/* Initialize a round robin distributor. */
    icd_status init_icd_distributor_round_robin(icd_distributor * that, char *name, icd_config * data);

/* Initialize a least recent distributor. */
    icd_status init_icd_distributor_least_recent_agent(icd_distributor * that, char *name, icd_config * data);

/* Initialize a most recent distributor. */
    icd_status init_icd_distributor_most_recent_agent(icd_distributor * that, char *name, icd_config * data);

/* Initialize an agent priority distributor. */
    icd_status init_icd_distributor_agent_priority(icd_distributor * that, char *name, icd_config * data);

/* Initialize a least calls distributor. */
    icd_status init_icd_distributor_least_calls_agent(icd_distributor * that, char *name, icd_config * data);

/* Initialize a random distributor. */
    icd_status init_icd_distributor_random(icd_distributor * that, char *name, icd_config * data);

/* Initialize a random distributor. */
    icd_status init_icd_distributor_ringall(icd_distributor * that, char *name, icd_config * data);

/* Clear a distributor */
    icd_status icd_distributor__clear(icd_distributor * that);

/***** Actions *****/

/* Adds an agent to the existing agent list in this distributor. */
    icd_status icd_distributor__add_agent(icd_distributor * that, icd_member * new_agent);

/* Pushes an agent back onto the top of this distributor. */
    icd_status icd_distributor__pushback_agent(icd_distributor * that, icd_member * new_agent);

/* Merges the agents in an agent list into the one contained in this distributor. */
    icd_status icd_distributor__add_agent_list(icd_distributor * that, icd_member_list * new_list);

/* Gives back the number of icd_agents waiting for a connection. */
    int icd_distributor__agents_pending(icd_distributor * that);

/* Gives the position of this agent in the list. */
    int icd_distributor__agent_position(icd_distributor * that, icd_agent * target);

/* Removes a specific agent from the distributor's list of available agents. */
    icd_status icd_distributor__remove_agent(icd_distributor * that, icd_agent * target);

/* Adds a customer to the existing customer list in this distributor. */
    icd_status icd_distributor__add_customer(icd_distributor * that, icd_member * new_cust);

/* Pushes a customer back onto the top of this distributor. */
    icd_status icd_distributor__pushback_customer(icd_distributor * that, icd_member * new_customer);

/* Merges the customers in a customer list into the one contained in this distributor. */
    icd_status icd_distributor__add_customer_list(icd_distributor * that, icd_member_list * new_list);

/* Gives back the number of icd_customers waiting for a connection. */
    int icd_distributor__customers_pending(icd_distributor * that);

/* Gives the position of this customer in the list. */
    int icd_distributor__customer_position(icd_distributor * that, icd_customer * target);

/* Removes a specific customer from the distributor's list of available agents. */
    icd_status icd_distributor__remove_customer(icd_distributor * that, icd_customer * target);

/* Print out a copy of the distributor. */
    icd_status icd_distributor__dump(icd_distributor * that, int verbosity, int fd);

/* Start the distributor thread. */
    icd_status icd_distributor__start_distributing(icd_distributor * that);

/* Pause the distributor thread. */
    icd_status icd_distributor__pause_distributing(icd_distributor * that);

/* Stop the distributor thread for good. */
    icd_status icd_distributor__stop_distributing(icd_distributor * that);

/* Select which of two callers to make the bridger and which is bridgee */
    icd_status icd_distributor__select_bridger(icd_caller * primary, icd_caller * secondary);

/***** Getters and Setters *****/

/* Sets the agent list to use in this distributor. */
    icd_status icd_distributor__set_agent_list(icd_distributor * that, icd_member_list * agents);

/* Gets the agent list used in this distributor. */
    icd_member_list *icd_distributor__get_agent_list(icd_distributor * that);

/* Sets the customer list to use in this distributor. */
    icd_status icd_distributor__set_customer_list(icd_distributor * that, icd_member_list * customers);

/* Gets the customer list used in this distributor. */
    icd_member_list *icd_distributor__get_customer_list(icd_distributor * that);

/* Sets the name of this distributor. */
    icd_status icd_distributor__set_name(icd_distributor * that, char *name);

/* Gets the number of callers added since last link function call. */
    unsigned int icd_distributor__get_added_callers_number(icd_distributor * that);

/*  Resets the number of callers added since last link function call.  */
    void icd_distributor__reset_added_callers_number(icd_distributor * that);

/* Gets the name of this distributor. */
    char *icd_distributor__get_name(icd_distributor * that);

/* Sets the name of this distributor. */
    icd_status icd_distributor__set_agentsfile(icd_distributor * that, char *agentsfile);

/* Gets the name of this distributor. */
    char *icd_distributor__get_agentsfile(icd_distributor * that);

/* TBD - should return icd_fieldset */
    void_hash_table *icd_distributor__get_params(icd_distributor * that);

/* get the plugable function if it exists */

    void *icd_distributor__get_plugable_fn_ptr(icd_distributor * that);
    icd_plugable_fn *icd_distributor__get_plugable_fn(icd_distributor * that, icd_caller * caller);

/**** Callback Setters ****/
/* set the plugable function to find the struc of plugable functions */
    icd_status icd_distributor__set_plugable_fn_ptr(icd_distributor * that,
        icd_plugable_fn * (*get_plugable_fn) (icd_caller *));

    icd_status icd_distributor__set_link_callers_fn(icd_distributor * that, icd_status(*link_fn) (icd_distributor *,
            void *extra), void *extra);

    icd_status icd_distributor__set_dump_func(icd_distributor * that, icd_status(*dump_fn) (icd_distributor *,
            int verbosity, int fd, void *extra), void *extra);

/**** Listeners ****/

/* Lock the distributor */
    icd_status icd_distributor__lock(icd_distributor * that);

/* Unlock the distributor. */
    icd_status icd_distributor__unlock(icd_distributor * that);

/**** Listeners ****/

    icd_status icd_distributor__add_listener(icd_distributor * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_distributor__remove_listener(icd_distributor * that, void *listener);

/**** Iterator functions ****/

    icd_list_iterator *icd_distributor__get_customer_iterator(icd_distributor * that);

    icd_list_iterator *icd_distributor__get_agent_iterator(icd_distributor * that);

/***** Predefined Pluggable Actions *****/

/* Pop the top agent and customer, and link them. */
    icd_status icd_distributor__link_callers_via_pop(icd_distributor *, void *);

/* Pop the top agent and customer and link them, then add agent back to list */
    icd_status icd_distributor__link_callers_via_pop_and_push(icd_distributor * dist, void *extra);

/* Pop the top customer and link to all free agents (who are all bridgers). */
    icd_status icd_distributor__link_callers_via_ringall(icd_distributor *, void *);

/* Standard distributor dump function */
    icd_status icd_distributor__standard_dump(icd_distributor * dist, int verbosity, int fd, void *extra);

/* get a pointer to a named parameter */
    char *icd_distributor__get_string_value(icd_distributor *, char *);

/* fill in the values of the struct*/
    void icd_distributor__populate_data(icd_distributor * that, char *data, char delim);

    icd_status icd_distributor__remove_caller(icd_distributor * that, icd_caller * that_caller);
    void *icd_distributor__standard_run(void *that);
    icd_status icd_distributor__set_run_fn(icd_distributor * that, void *(*run_fn) (void *that));
    icd_status icd_distributor__set_default_run_fn(icd_distributor * that);

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

