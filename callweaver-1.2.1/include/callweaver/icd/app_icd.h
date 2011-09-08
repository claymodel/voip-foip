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
 *  \brief app_icd.h - Main entry point of Intelligent Call Distribution
 * 
 * This module represents the Intelligent Call Distribution system. It
 * provides the public interface that other systems, particularly CallWeaver,
 * can use for interacting with the ICD system.
 *
 * The ICD system provides a mechanism for creating queues of callers, each
 * composed of a two lists of callers (customers and agents), and bridging
 * calls between the two lists using a completely configurable mechanism
 * called a distributor.
 *
 * The app_icd module will typically be controlled through CallWeaver as an
 * application, but that is not required. You can also control it through a
 * custom CallWeaver manager, for example. You could even interact with it from
 * one of the other apps. The functions available for that purpose are
 * identified here.
 *
 */

#ifndef APP_ICD_H
#define APP_ICD_H

#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    icd_status icd_module_load_dynamic_module(icd_config_registry * registry);
    icd_status icd_module_unload_dynamic_modules(void);

/***** CallWeaver-specific APIs *****/

/* These functions are the mechanism that CallWeaver uses to control ICD.
 * They are declared in module.h so there is no need to declare them a
 * second time here. They are provided for documentation purposes only.
 * If you wish to use these functions, include module.h in your file.
 *
 *   int load_module(void);
 *   int unload_module(void);
 *   int usecount(void);
 *   char *description(void);
 *   char *key();
 *   int reload(void);
 
 *
 *
*/

/* This is the registry for queues, associating queue names with objects. */
    extern icd_fieldset *queues;

/* This is the registry for agents, associating agent names with objects.*/
    extern icd_fieldset *agents;

/* This is the registry for customers, associating agent names with objects. */
    extern icd_fieldset *customers;

/* This is the module mask (icd.conf module_mask=) for what module events to show in the default icd cli.*/
    extern int module_mask[ICD_MAX_MODULES];

/* This is the event mask (icd.conf event_mask=)for what events to show in the default icd cli.*/
    extern int event_mask[ICD_MAX_EVENTS];
/***** Initialization Routines *****/

/* Initialize the ICD system so that it is ready for use but not configured. */
    icd_status init_app_icd(void);

/* Clean up all resources held by the ICD system. */
    icd_status cleanup_app_icd(void);

/* Reinitialization the ICD system, reloading config files */
    icd_status reload_app_icd(icd_module module);

/***** Behaviours *****/

icd_queue *app_icd__new_queue(icd_config * config);
icd_queue *app_icd__new_unconfigured_queue(char *name);
icd_queue *app_icd__get_queue(char *name);
icd_distributor *app_icd__get_distributor(char *name);

icd_customer *app_icd__new_customer_to_queue(icd_config * cust_config, char *queue);
icd_agent *app_icd__new_agent_to_queue(icd_config * agent_config, char *queue);
icd_customer *app_icd__new_customer_to_queues(icd_config * cust_config, char **queues, int qcount);
icd_agent *app_icd__new_agent_to_queues(icd_config * agent_config, char **queues, int qcount);

icd_status app_icd__add_customer_to_queue(icd_customer * cust, char *queue);
icd_status app_icd__add_agent_to_queue(icd_agent * agent, char *queue);

/* Configuration through conf files. If filename NULL use default. Use these as a pair. */
/*icd general system options */
icd_status app_icd__read_icd_config(char *icd_config_name);

/* conference options */
icd_status app_icd__read_conference_config(char *conference_config_name);

/*queues config */
icd_status app_icd__read_queue_config(icd_fieldset * queues, char *queue_config_name,
        icd_fieldset * outstanding_members);
/* agents config */
icd_status app_icd__read_agents_config(icd_fieldset * agents, char *agent_config_name, icd_fieldset * queues,
        icd_fieldset * outstanding_members);

/* Adds a customer to the ICD system */
int app_icd__customer_exec(struct cw_channel *chan, int argc, char **argv);

/* Starts up an agent in the ICD system */
int app_icd__agent_exec(struct cw_channel *chan, int argc, char **argv);

/* Logs out an agent in the ICD system */
int app_icd__logout_exec(struct cw_channel *chan, int argc, char **argv);

/* Add an agent as a member to a queue */
int app_icd__add_member_exec(struct cw_channel *chan, int argc, char **argv);

/* Remove an agent as a member of a queue */
int app_icd__remove_member_exec(struct cw_channel *chan, int argc, char **argv);

/* Agent callback login */
int app_icd__agent_callback_login(struct cw_channel *chan, int argc, char **argv);

/* Customer callback login */
int app_icd__customer_callback_login(struct cw_channel *chan, int argc, char **argv);

/* dunno where this belongs but useful every where : */
int icd_instr(char *bigstr, char *smallstr, char delimit);

/***** Getters and Setters *****/

icd_status app_icd__set_default_queue_file(char *filename);
char *app_icd__get_default_queue_file(void);

icd_status app_icd__set_default_agent_file(char *filename);
char *app_icd__get_default_agent_file(void);

/**** Listener functions ****/

icd_status app_icd__add_listener(icd_agent * that, void *listener, int (*lstn_fn) (icd_agent * that, void *msg,
            void *extra), void *extra);

icd_status app_icd__remove_listener(icd_agent * that, void *listener);

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

