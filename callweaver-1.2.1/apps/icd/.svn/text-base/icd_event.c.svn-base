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

/* \file
 * \brief icd_event.h - event framework
 * 
 * The icd_event module provides an event framework. It has two parts - an
 * icd_event_factory which creates events and keeps track of listeners that
 * want to be alerted to events occuring, and the icd_event object itself.
 *
 * icd_event is unusual in the ICD system in that, like list iterators,
 * it has no create_ method and is instead generated from another object,
 * in this case the event factory. This is done so we can have a global
 * mechanism for keeping track of events that occur.
 *
 * There is a global event factory that you should use if you don't want
 * to create a custom version. All the standard ICD modules use it. If you
 * do write your own event factory, you should add any listeners that
 * register themselves with you to the global event factory, as well, so
 * that they can get all events that occur.
 *
 * The only event that the event factory sends to its listeners is the FIRE
 * event, which happens every time any event notifies its listeners that it
 * is being fired. If there are no listeners on the event factory, then the
 * FIRE event does not occur.
 *
 * The FIRE event is a vetoable event, so any listeners to the event
 * factory can stop an event from firing. This will filter back to the
 * event-generating source as a veto as well, just as if one of its
 * listeners had done the vetoing.
 *
 * The typical lifecycle of an event is icd_event_factory__make(),
 * icd_event__fire(), and destroy_icd_event(). Because so many of the events
 * that are generated follow this pattern, a function is provided that
 * can be used instead of calling those three functions yourself,
 * icd_event_factory__generate(). Another common pattern is to make the
 * event, call a notification hook, then fire and destroy the event object.
 * For this, the icd_event_factory_notify() function is provided.
 *
 * The icd_event_factory__make(), icd_event_factory__generate(), and
 * icd_event_factory__notify functions all have long and complicated sets of
 * parameters that are passed in. You can use these functions directly if you
 * like, but since the parameters that are passed are the same in so many
 * cases, some macros have been provided for you that pass in the most
 * common values you are going to want to use. These are:
 *
  *   1. icd_event__make(event_id, message, extra) - The three parameters
 *      listed are the only ones you need to provide when you call this.
 *      The other parameters to icd_event_factory__make() are provided for
 *      you automatically, as follows:
 *          - factory is "event_factory"
 *          - src is "that",
 *            NOTE: if the local function in which want to use an event macro has no 'that' as an argument
 *            you will need to add a var object 'that' in order to use the event macros and it must have
 *            a 'name' member , and a listeners member
 *          - src_name is "that->name"
 *          - listeners is "that->listeners"
 *          - mod_id is the static variable module_id in each ICD module
 *
 *   2. icd_event__generate(event_id, extra) - Calls icd_event_factory__generate
 *      with the following parameters provided
 *      for you -
 *      (event_factory, that, that->name, module_id, event_id, NULL, that->listeners, extra)
 *   3. icd_event__generate_with_msg(event_id, message, extra) - Exactly like
 *      icd_event__generate(), only the message variable is not NULL (meaning
 *      use the standard message) but passed in as a parameter.
 *
 *   4. icd_event__notify(event_id, extra, hook_fn, extra) - Calls icd_event_factory__notify
*/

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_listeners.h"
#include "callweaver/icd/icd_event.h"

/*===== Private types and APIs =====*/

static icd_module module_id = ICD_EVENT;

struct icd_event_factory {
    char name[ICD_STRING_LEN];
    icd_listeners *listeners;
    char **event_strings;
    icd_memory *memory;
    int allocated;
};

struct icd_event {
    icd_event_factory *factory;
    void *src;
    char *src_name;
    int mod_id;
    int event_id;
    char msg[1024];
    icd_listeners *target_listeners;
    void *extra;
    icd_memory *memory;
    int allocated;
};

int *icd_event_modules[ICD_MAX_MODULES];

/* The standard set of module strings. */
char *icd_module_strings[ICD_MAX_MODULES] = {
    "APP_ICD", "ICD_Queue", "ICD_Distributor", "ICD_Distributor_List",
    "ICD_Caller", "ICD_Caller_List", "ICD_Agent", "ICD_Customer",
    "ICD_Member", "ICD_Member_List", "ICD_Bridge", "ICD_Conference", "ICD_Listeners",
    "ICD_Event", "ICD_Fieldset", "ICD_Config", "ICD_List", "ICD_Meta_List",
    "ICD_PlugableFN", "ICD_PlugableFN_List", "ICD_Last_Standard",
    NULL
};

/* The standard set of event strings. NULL marks the end */
char *icd_event_strings[ICD_MAX_EVENTS] = {
    "Test", "Create", "Initialize", "Clear", "Destroy", "Push", "Pop", "Fire",
    "Pushback", "StateChange", "Add", "Remove", "Ready", "ChannelUp",
    "GetChannels", "GotChannels", "Distribute", "Distributed",
    "Link", "Unlink", "Bridge", "Bridged", "BridgeFailed",
    "ChannelFailed", "AssociateFailed", "BridgeEnded", "Suspend",
    "SetField", "Authenticate", "Assign", "Conference", "LastStandard", NULL
};
int *icd_event_ids[ICD_MAX_EVENTS];

icd_event_factory global_event_factory = { "global", NULL, icd_event_strings, 0 };
icd_event_factory *event_factory = &global_event_factory;

/* Sets the strings that represent each event */
icd_status icd_event_factory__set_event_strings(icd_event_factory * that, char **strs);

/* Gets the set of strings that represent the events */
char **icd_event_factory__get_event_strings(icd_event_factory * that);

/*===== Public APIs =====*/

/*=== Event Factory methods ===*/

/* Create an event factory. */
icd_event_factory *create_icd_event_factory(char *name)
{
    icd_event_factory *factory;
    icd_status result;

    ICD_MALLOC(factory, sizeof(icd_event_factory));
    if (factory == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Event Factory\n");
        return NULL;
    }
    factory->allocated = 1;
    result = init_icd_event_factory(factory, name);
    if (result != ICD_SUCCESS) {
        ICD_FREE(factory);
        return NULL;
    }

    return factory;
}

/* Destroy an event factory, freeing its memory and cleaning up after it. */
icd_status destroy_icd_event_factory(icd_event_factory ** factoryp)
{
    int clear_result;

    assert(factoryp != NULL);
    assert(*factoryp != NULL);

    if ((clear_result = icd_event_factory__clear(*factoryp)) != ICD_SUCCESS) {
        return clear_result;
    }
    if ((*factoryp)->allocated) {
        ICD_FREE((*factoryp));
        *factoryp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize an event factory */
icd_status init_icd_event_factory(icd_event_factory * that, char *name)
{
    assert(that != NULL);
    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_event_factory));
    strncpy(that->name, name, sizeof(that->name));
    that->listeners = create_icd_listeners();
    that->event_strings = icd_event_strings;
    that->allocated = 0;

    return ICD_SUCCESS;
}

/* Clear an event factory */
icd_status icd_event_factory__clear(icd_event_factory * that)
{
    icd_status result;

    assert(that != NULL);
    assert(that->listeners != NULL);

    result = destroy_icd_listeners(&(that->listeners));

    return result;
}

/***** Actions *****/
int icd_event_factory__add_module(char *name)
{
    int x;

    assert(name != NULL);
    for (x = 0; x < ICD_MAX_MODULES; x++) {
        if (icd_module_strings[x] == NULL) {
            icd_module_strings[x] = name;
            icd_module_strings[x + 1] = NULL;
            return x;
        }
    }

    return 0;
}

int icd_event_factory__add_event(char *name)
{
    int x;

    assert(name != NULL);
    for (x = 0; x < ICD_MAX_EVENTS; x++) {
        if (icd_event_strings[x] == NULL) {
            icd_event_strings[x] = name;
            icd_event_strings[x + 1] = NULL;
            return x;
        }
    }

    return 0;
}

/* Create a new event from this factory. */
icd_event *icd_event_factory__make(icd_event_factory * that, void *src, char *src_name, int mod_id, int event_id,
    char *msg, icd_listeners * targets, void *extra)
{
    icd_event *event;
    icd_status result;

    ICD_MALLOC(event, sizeof(icd_event));
    if (event == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Event\n");
        return NULL;
    }
    event->allocated = 1;
    result = init_icd_event(event, that, src, src_name, mod_id, event_id, msg, targets, extra);
    if (result != ICD_SUCCESS) {
        ICD_STD_FREE(event);
        return NULL;
    }

    return event;
}

/* Creates an event, fires it, and destroys it. This is the standard behaviour,
   but you can create your own with the other functions here. */
icd_status icd_event_factory__generate(icd_event_factory * that, void *src, char *src_name, int mod_id,
    int event_id, char *msg, icd_listeners * targets, void *extra)
{
    icd_event *event;
    icd_status result;

    assert(that != NULL);

    event = icd_event_factory__make(that, src, src_name, mod_id, event_id, msg, targets, extra);
    if (event == NULL) {
        return ICD_ERESOURCE;
    }
    result = icd_event__fire(event);
    destroy_icd_event(&event);
    return result;
}

/* Creates an event, notifies hook function, fires event, and destroys it.
   Note that extra is provided by the module firing the event, but the
   hook_extra is provided by the module that registeres the function. */
icd_status icd_event_factory__notify(icd_event_factory * that, void *src, char *src_name, int mod_id, int event_id,
    char *msg, icd_listeners * targets, void *extra, int (*hook_fn) (icd_event * event, void *extra),
    void *hook_extra)
{
    icd_event *event;
    icd_status result;
    int hook_result;

    assert(that != NULL);

    event = icd_event_factory__make(that, src, src_name, mod_id, event_id, msg, targets, extra);
    if (event == NULL) {
        return ICD_ERESOURCE;
    }
    if (hook_fn != NULL) {
        hook_result = hook_fn(event, hook_extra);
        if (hook_result != 0) {
            destroy_icd_event(&event);
            return ICD_EVETO;
        }
    }
    result = icd_event__fire(event);
    destroy_icd_event(&event);
    return result;
}

/* Translates the event id into a suitable string for printing based on the
   factory's set of translation strings. */
char *icd_event_factory__to_string(icd_event_factory * factory, icd_event_type event_id)
{
    int x;

    if (event_id < 0) {
        return "";
    }
    /* Check whether event is higher than strings we have available */
    for (x = 0; x < event_id; x++) {
        if (factory->event_strings[x] == NULL) {
            return "";
        }
    }
    /* It's all good. */
    return factory->event_strings[event_id];
}

/**** Listener functions ****/

icd_status icd_event_factory__add_listener(icd_event_factory * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    /* Initialize if we got the global event factory for the first time */
    if (that->listeners == NULL) {
        that->listeners = create_icd_listeners();
    }

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_event_factory__remove_listener(icd_event_factory * that, void *listener)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}

/*=== Event methods ===*/

/***** Initializers and Destroyers *****/

/* Destructor for an event. */
icd_status destroy_icd_event(icd_event ** eventp)
{
    int clear_result;

    assert(eventp != NULL);
    assert(*eventp != NULL);

    if ((clear_result = icd_event__clear(*eventp)) != ICD_SUCCESS) {
        return clear_result;
    }
    if ((*eventp)->allocated) {
        ICD_FREE((*eventp));
        *eventp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize previously created event */
icd_status init_icd_event(icd_event * that, icd_event_factory * factory, void *src, char *src_name, int mod_id,
    int event_id, char *msg, icd_listeners * targets, void *extra)
{
    char buf[200];

    assert(that != NULL);
    assert(factory != NULL);
    assert(src != NULL);

    that->factory = factory;
    that->src = src;
    that->src_name = src_name;
    that->mod_id = mod_id;
    that->event_id = event_id;

    if (msg != NULL)
        strncpy(that->msg, msg, sizeof(that->msg));
    else
        strncpy(that->msg, icd_event__standard_msg(that, buf, 200), sizeof(that->msg));

    that->target_listeners = targets;
    that->extra = extra;

    return ICD_SUCCESS;
}

/* Clear the event so that it needs to be reinitialized to be reused. */
icd_status icd_event__clear(icd_event * that)
{
    assert(that != NULL);

    that->factory = NULL;
    that->src = NULL;
    that->mod_id = -1;
    that->event_id = -1;
    that->target_listeners = NULL;
    that->extra = NULL;

    return ICD_SUCCESS;
}

/***** Actions *****/
/*
for (mod = APP_ICD; mod < ICD_MODULE_L CW_STANDARD; ++mod) {
        snprintf(tmp, sizeof(tmp), "%d",mod);
        icd_fieldset__strncpy(modules,tmp,icd_module_strings[mod], 10);
    };

    for (event = ICD_EVENT_TEST; event < ICD_EVENT_L CW_STANDARD; ++event) {
        snprintf(tmp, sizeof(tmp), "%d",event);
        icd_fieldset__strncpy(events,tmp,icd_event_strings[event], 10);
    };
*/
/* Lets each listener know that an event has occured. Returns
   ICD_EVETO if any of them has an issue. */
icd_status icd_event__fire(icd_event * that)
{
    icd_event *factory_event;
    char fire_msg[200];
    int fire_veto = 0;
    int event_veto;

    assert(that != NULL);

    if (that->target_listeners == NULL) {
        return ICD_SUCCESS;
    }
    if (that->factory != NULL && that->factory->listeners != NULL) {
        snprintf(fire_msg, sizeof(fire_msg), "Event %s in %s ==> [%s] %s",
            icd_event_factory__to_string(that->factory, that->event_id), icd_module__to_string(that->mod_id),
            that->src_name, that->msg);

        /* Can't use icd_event__make here because first 2 args are wrong */
        factory_event =
            icd_event_factory__make(that->factory, that->factory, that->factory->name, module_id, ICD_EVENT_FIRE,
            fire_msg, that->factory->listeners, that);
        /* pass the factory event object to the factory listeners  text of event is in factory_event->msg
         * the actual source event is the extra of the factory_event eg in your factory listener
         * icd_event *event=icd_event__get_extra(factory_event);
         */
        fire_veto = icd_listeners__notify(that->factory->listeners, factory_event);
        destroy_icd_event(&factory_event);

    }
    if (fire_veto) {
        return ICD_EVETO;
    }
    event_veto = icd_listeners__notify(that->target_listeners, that);
    if (event_veto) {
        return ICD_EVETO;
    }
    return ICD_SUCCESS;
}

/* Generate a standard message for an event in a buffer you provide */
char *icd_event__standard_msg(icd_event * that, char *buf, int bufsize)
{
    assert(that != NULL);
    assert(buf != NULL);

    memset(buf, 0, bufsize);
    strncpy(buf, icd_event_factory__to_string(that->factory, that->event_id), bufsize - 1);
    strncat(buf, " ", bufsize - strlen(buf) - 1);
    strncat(buf, icd_module__to_string(that->mod_id), bufsize - strlen(buf) - 1);
    if (that->src_name != NULL) {
        strncat(buf, " ", bufsize - strlen(buf) - 1);
        strncat(buf, that->src_name, bufsize - strlen(buf) - 1);
    }
    return buf;
}

/***** Getters and Setters *****/

/* Sets the source of the event */
icd_status icd_event__set_source(icd_event * that, void *src)
{
    assert(that != NULL);

    that->src = src;
    return ICD_SUCCESS;
}

/* Gets the source of the event */
void *icd_event__get_source(icd_event * that)
{
    assert(that != NULL);

    return that->src;
}

/* Sets the source name of the event */
icd_status icd_event__set_name(icd_event * that, void *src)
{
    assert(that != NULL);

    that->src_name = src;
    return ICD_SUCCESS;
}

/* Gets the source name of the event */
char *icd_event__get_name(icd_event * that)
{
    assert(that != NULL);

    return that->src_name;
}

/* Sets the mod_id of the event */
icd_status icd_event__set_module_id(icd_event * that, int mod_id)
{
    assert(that != NULL);

    that->mod_id = mod_id;
    return ICD_SUCCESS;
}

/* Gets the module_id of the event */
int icd_event__get_module_id(icd_event * that)
{
    assert(that != NULL);

    return that->mod_id;
}

/* Sets the event_id of the event */
icd_status icd_event__set_event_id(icd_event * that, int event_id)
{
    assert(that != NULL);

    that->event_id = event_id;
    return ICD_SUCCESS;
}

/* Gets the event_id of the event */
int icd_event__get_event_id(icd_event * that)
{
    assert(that != NULL);

    return that->event_id;
}

/* Sets the message string of the event */
icd_status icd_event__set_message(icd_event * that, char *msg)
{
    assert(that != NULL);

    if (that->msg != NULL) {
        ICD_STD_FREE(that->msg);
    }
    strncpy(that->msg, msg, sizeof(that->msg));

    return ICD_SUCCESS;
}

/* Gets the message string of the event */
char *icd_event__get_message(icd_event * that)
{
    assert(that != NULL);
    return that->msg;
}

/* Sets the extra data on the event */
icd_status icd_event__set_extra(icd_event * that, void *extra)
{
    assert(that != NULL);

    that->extra = extra;
    return ICD_SUCCESS;
}

/* Gets the extra data on the event */
void *icd_event__get_extra(icd_event * that)
{
    assert(that != NULL);

    return that->extra;
}

/* Sets the target listeners of the event */
icd_status icd_event__set_listeners(icd_event * that, icd_listeners * targets)
{
    assert(that != NULL);

    that->target_listeners = targets;
    return ICD_SUCCESS;
}

/* Gets the target listeners of the event */
icd_listeners *icd_event__get_listeners(icd_event * that)
{
    assert(that != NULL);

    return that->target_listeners;
}

/*===== Private Function Implementations =====*/

/***** Factory Getters and Setters *****/

/* Sets the strings to display for a particular event */
icd_status icd_event_factory__set_event_strings(icd_event_factory * that, char **strs)
{
    assert(that != NULL);

    that->event_strings = strs;
    return ICD_SUCCESS;
}

/* Gets the strings that represent events for this factory */
char **icd_event_factory__get_event_strings(icd_event_factory * that)
{
    assert(that != NULL);

    return that->event_strings;
}

/***** Help Functions *****/

/* Translates the module ID into a suitable string for printing */
char *icd_module__to_string(icd_module mod_id)
{
    int x;

    if (mod_id < 0) {
        return "";
    }
    /* Check whether module ID is higher than strings we have available */
    for (x = 0; x < mod_id; x++) {
        if (icd_module_strings[x] == NULL) {
            return "";
        }
    }
    /* It's all good. */
    return icd_module_strings[mod_id];
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

