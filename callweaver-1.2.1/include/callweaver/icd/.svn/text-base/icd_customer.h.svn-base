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
 *  \brief  icd_customer.h  -  a customer call
 *
 * The icd_customer module represents a call that needs to be serviced by an
 * internal entity, aka an icd_agent. Although typically this will actually
 * be a customer of the company, it could be any call that needs servicing.
 *
 * This module is for the most part implemented as icd_customer, which
 * icd_customer is castable as. Only things specific to the customer side
 * of a call will be here.
 *
 */

#ifndef ICD_CUSTOMER_H
#define ICD_CUSTOMER_H

#include <time.h>
#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    icd_status icd_customer_load_module(icd_config * config);
/***** Init - Destroyer *****/

/* Create an customer. data is a config obj */
    icd_customer *create_icd_customer(icd_config * data);

/* Destroy a customer, freeing its memory and cleaning up after it. */
    icd_status destroy_icd_customer(icd_customer ** custp);

/* Initialize a previously created customer. */
    icd_status init_icd_customer(icd_customer * that, icd_config * data);

/* Clear a customer so it needs to be reinitialized to be reused. */
    icd_status icd_customer__clear(icd_customer * that);

/***** Locking *****/

/* Lock the entire customer */
    icd_status icd_customer__lock(icd_customer * that);

/* Unlock the entire customer */
    icd_status icd_customer__unlock(icd_customer * that);

/**** Listeners ****/

    icd_status icd_customer__add_listener(icd_customer * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_customer__remove_listener(icd_customer * that, void *listener);

/**** Standard Customer state action functions ****/
    icd_plugable_fn *icd_customer_get_plugable_fns(icd_caller * that);

/* These are all copies of the icd_caller_standard_state[icd_caller_state] function from icd_caller.c
 * if you find that you need unique State actions based on the role of the caller then
 * uncomment these proto types and implement the function in icd_customer.c 
 * then in init_icd_customer register the local function using icd_caller__set_state_[icd_caller_state]_fn
 * for example if you want to over ride the icd_[ICD_ROLE]_standard_state_call_end function then use
 * icd_caller__set_state_call_end_fn
 * (icd_caller *that, int (*end_fn)(icd_event *that, void *extra), void *extra)  
 * 
 * or using icd_config__get_any_value
 *     that->state_call_end_fn = 
 *     icd_config__get_any_value(data, "state.call.end.function", icd_caller__standard_state_call_end);

*/
/* Standard function for conferneces 
int icd_customer__standard_state_conference(icd_event *event, void *extra);
*/

/* Standard function for reacting to the ready state */
    int icd_customer__standard_state_ready(icd_event * event, void *extra);

/* Standard function for reacting to the distribute state 
int icd_customer__standard_state_distribute(icd_event *event, void *extra);
*/

/* Standard function for reacting to the get channels and bridge state 
int icd_customer__standard_state_get_channels(icd_event *event, void *extra);
*/

/* Standard function for reacting to the bridged state 
int icd_customer__standard_state_bridged(icd_event *event, void *extra);
*/

/* Standard function for reacting to the call end state */
    int icd_customer__standard_state_call_end(icd_event * event, void *extra);

/* Standard function for reacting to the bridge failed state */
    int icd_customer__standard_state_bridge_failed(icd_event * event, void *extra);

/* Standard function for reacting to the channel failed state 
int icd_customer__standard_state_channel_failed(icd_event *event, void *extra);
*/

/* Standard function for reacting to the associate failed state 
int icd_customer__standard_state_associate_failed(icd_event *event, void *extra);
*/

/* Standard function for reacting to the suspend state 
int icd_customer__standard_state_suspend(icd_event *event, void *extra);
*/

/* Standard behaviours called from inside the caller state action functions */

/* Standard function to prepare a caller object for entering a distributor 
icd_status icd_customer__standard_prepare_caller(icd_caller *caller);
*/

/* Standard function for cleaning up a caller after a bridge ends or fails */
    icd_status icd_customer__standard_cleanup_caller(icd_caller * caller);

/* Standard function for starting the bridging process on a caller 
icd_status icd_customer__standard_launch_caller(icd_caller *caller);
*/
    icd_customer *icd_customer__generate_queued_call(char *id, char *queuename, char *dialstr, char *vars,
        char delim, void *plug);

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

