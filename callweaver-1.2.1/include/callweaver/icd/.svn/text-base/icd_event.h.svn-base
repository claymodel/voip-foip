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
 *  \brief icd_event.h  -  Event Framework
 *
 * This module has two major components, the icd_event_factory and the
 * icd_event. Between them, they give all the functions required to provide 
 * an event handling framework.
 *
 * The factory is used to generate events. icd_event has no create method
 * and instead relies on the factory to create new instances and initialize
 * them. If you want to change the events used by the system, you'll have to
 * write your own event factory.
 *
 * The factory does more than just create events. It can also contain 
 * listeners that are told when any event created by the factory is fired.
 *
 * Events have a very simple lifecycle. They are created, fired (which involves
 * passing them to all notify functions and listeners appropriate to the event), 
 * and then destroyed. They encapsulate the module that the event originated
 * in, the event type, the source structure which threw the event, a message 
 * string for the event, and any other information that the source structure 
 * wants to pass along. It is assumed that the listener that is interested in
 * the event will know how to decode the extra information.
 *
 * The four functions you are most likely to need are actually macros:
 *   - icd_event__generate(event_id, extra) for simple events
 *   - icd_event__generate_with_msg(event_id, msg, extra) for a custom message
 *   - icd_event__notify(event_id, extra, hook_fn, hook_extra) for an
 *         event that also has to call a notify function
 *   - icd_event__notify_with_msg(event_id, msg, extra, hook_fn, hook_extra)
 *         same as notify, but with a custom message
 */

#ifndef ICD_EVENT_H
#define ICD_EVENT_H
#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* You don't have to use this factory, but all of ICD does */
    extern icd_event_factory *event_factory;

/* This is the registry for events, associating events names with event_id's.*/
    extern icd_fieldset *events;

/* This is the registry for modules, associating module names with module_ids.*/
    extern icd_fieldset *modules;

/***** Init - Destroyer for icd_event_factory *****/

/* Create an event factory. */
    icd_event_factory *create_icd_event_factory(char *name);

/* Destroy an event factory, freeing its memory and cleaning up after it. */
    icd_status destroy_icd_event_factory(icd_event_factory ** factoryp);

/* Initialize an event factory */
    icd_status init_icd_event_factory(icd_event_factory * that, char *name);

/* Clear an event factory */
    icd_status icd_event_factory__clear(icd_event_factory * that);

/***** Actions *****/

/* Create a new event from this factory. */
    icd_event *icd_event_factory__make(icd_event_factory * that, void *src, char *src_name, int mod_id,
        int event_id, char *msg, icd_listeners * targets, void *extra);

/* Macro that expands to create event in the most common way within ICD.
   factory is "event_factory", src is "that", src_name is "that->name", 
   listeners is "that->listeners", mod_id is the variable module_id 
   each ICD module */
#define icd_event__make(event_id, msg, extra) icd_event_factory__make(event_factory, that, that->name, module_id, event_id, msg, that->listeners, extra)

/* Translates event ID to string based on factory strings */
    char *icd_event_factory__to_string(icd_event_factory * factory, icd_event_type event_id);

/* Macro __to_string() that assumes factory is "event_factory" */
#define icd_event__to_string(event_id) icd_event_factory__to_string(event_factory, event_id)

/* Create an event, fire it, and destroy it. This is the standard behaviour. */
    icd_status icd_event_factory__generate(icd_event_factory * that, void *src, char *src_name, int mod_id,
        int event_id, char *msg, icd_listeners * targets, void *extra);

/* Macro that simplifies call to __generate() */
#define icd_event__generate(event_id, extra) icd_event_factory__generate(event_factory, that, that->name, module_id, event_id, NULL, that->listeners, extra)

/* Macro that simplifies call to __generate() and sets the message string. */
#define icd_event__generate_with_msg(event_id, msg, extra) icd_event_factory__generate(event_factory, that, that->name, module_id, event_id, msg, that->listeners, extra)

    icd_status icd_event_factory__notify(icd_event_factory * that, void *src, char *src_name, int mod_id,
        int event_id, char *msg, icd_listeners * targets, void *extra, int (*hook_fn) (icd_event * event,
            void *extra), void *hook_extra);

#define icd_event__notify(event_id, extra, hook_fn, hook_extra) icd_event_factory__notify(event_factory, that, that->name, module_id, event_id, NULL, that->listeners, extra, hook_fn, hook_extra)

#define icd_event__notify_with_message(event_id, msg, extra, hook_fn, hook_extra) icd_event_factory__notify(event_factory, that, that->name, module_id, event_id, msg, that->listeners, extra, hook_fn, hook_extra)

/**** Listener functions ****/

    icd_status icd_event_factory__add_listener(icd_event_factory * that, void *listener,
        int (*lstn_fn) (void *listener, icd_event * event, void *extra), void *extra);

    icd_status icd_event_factory__remove_listener(icd_event_factory * that, void *listener);

/***** Destroyer for icd_event *****/

/* Destroy an event, freeing its memory and cleaning up after it. */
    icd_status destroy_icd_event(icd_event ** eventp);

/* Initialize an event */
    icd_status init_icd_event(icd_event * that, icd_event_factory * factory, void *src, char *src_name, int mod_id,
        int event_id, char *msg, icd_listeners * targets, void *extra);

/* Clear an event */
    icd_status icd_event__clear(icd_event * that);

/***** Actions *****/

/* Fire the event, alerting all listeners and watching for vetos. */
    icd_status icd_event__fire(icd_event * that);

/* Generate a standard message for an event in a buffer you provide */
    char *icd_event__standard_msg(icd_event * event, char *buf, int bufsize);

/***** Getters and Setters *****/
/* add/register new modules or events */
    int icd_event_factory__add_module(char *name);
    int icd_event_factory__add_event(char *name);

/* Sets the source of the event */
    icd_status icd_event__set_source(icd_event * that, void *src);

/* Gets the source of the event */
    void *icd_event__get_source(icd_event * that);

    icd_status icd_event__set_name(icd_event * that, void *src);
    char *icd_event__get_name(icd_event * that);

/* Sets the module_id of the event */
    icd_status icd_event__set_module_id(icd_event * that, int mod_id);

/* Gets the module_id of the event */
    int icd_event__get_module_id(icd_event * that);

/* Sets the event_id of the event */
    icd_status icd_event__set_event_id(icd_event * that, int event_id);

/* Gets the event_id of the event */
    int icd_event__get_event_id(icd_event * that);

/* Sets the message string of the event */
    icd_status icd_event__set_message(icd_event * that, char *msg);

/* Gets the message string of the event */
    char *icd_event__get_message(icd_event * that);

/* Sets the extra data on the event */
    icd_status icd_event__set_extra(icd_event * that, void *extra);

/* Gets the extra data on the event */
    void *icd_event__get_extra(icd_event * that);

/* Sets the target listeners of the event */
    icd_status icd_event__set_listeners(icd_event * that, icd_listeners * targets);

/* Gets the target listeners of the event */
    icd_listeners *icd_event__get_listeners(icd_event * that);

/***** Helper Functions *****/

/* Translates the module id into a suitable string for printing */
    char *icd_module__to_string(icd_module mod_id);

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

