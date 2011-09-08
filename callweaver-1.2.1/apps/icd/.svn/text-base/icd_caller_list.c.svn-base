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
 * \brief icd_caller_list.c - a list of callers
 *
 * The icd_caller_list is a typical icd_list with a few extensions that are
 * specific to keeping a list of icd_callers. It sets some default attributes
 * for the callers it holds, such as the moh, announce, and default distributor
 * for the callers to belong to.
 *
 * In addition, it keeps track of a few events fired off by individual
 * icd_caller elements and amalgamates them into one place. So if you want
 * to be notified when any icd_caller in this list is bridged, you
 * would register yourself with icd_caller_list__set_assigned_agent_notify_fn().
 *
 */
 
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_list_private.h"
#include "callweaver/icd/icd_caller.h"

/*===== Private APIs, Types, and Variables =====*/

static icd_module module_id = ICD_CALLER_LIST;

struct icd_caller_list {
    icd_list list;
    int (*state_fn) (icd_event * event, void *extra);
    int (*chan_fn) (icd_event * event, void *extra);
    int (*link_fn) (icd_event * event, void *extra);
    int (*bridge_fn) (icd_event * event, void *extra);
    int (*authn_fn) (icd_event * event, void *extra);
    void *state_fn_extra;
    void *chan_fn_extra;
    void *link_fn_extra;
    void *bridge_fn_extra;
    void *authn_fn_extra;
    icd_memory *memory;
    int allocated;

};

static const int skipconst = 1;

static int icd_caller_list__identify_name(void *key, void *payload);

/***** Init - Destroyer *****/

/* Constructor for a caller list. */
icd_caller_list *create_icd_caller_list(char *name, icd_config * data)
{
    icd_caller_list *list;
    icd_status result;

    assert(data != NULL);
    ICD_MALLOC(list, sizeof(icd_caller_list));
    if (list == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Caller List\n");
        return NULL;
    }
    list->allocated = 1;
    result = init_icd_caller_list(list, name, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(list);
        return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, list, ((icd_list *) list)->name, module_id, ICD_EVENT_CREATE,
        NULL, ((icd_list *) list)->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_caller_list(&list);
        return NULL;
    }
    return list;
}

/* Destructor for a caller list. */
icd_status destroy_icd_caller_list(icd_caller_list ** listp)
{
    icd_status clear_result;
    icd_status vetoed;
    icd_list *that;

    assert(listp != NULL);
    assert((*listp) != NULL);

    /* First, notify event hooks and listeners */
    that = (icd_list *) (*listp);
    vetoed = icd_event__notify(ICD_EVENT_DESTROY, NULL, that->dstry_fn, that->dstry_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD Caller List %s has been vetoed\n",
            icd_caller_list__get_name(*listp));
        return ICD_EVETO;
    }

    if ((clear_result = icd_caller_list__clear(*listp)) != ICD_SUCCESS) {
        return clear_result;
    }

    if ((*listp)->allocated) {
        that->state = ICD_LIST_STATE_DESTROYED;
        ICD_FREE((*listp));
        *listp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize previously created caller list */
icd_status init_icd_caller_list(icd_caller_list * that, char *name, icd_config * data)
{
    icd_status retval;
    icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
    void *extra;

    assert(that != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_caller_list));

    retval = init_icd_list((icd_list *) that, data);
    icd_list__set_name((icd_list *) that, name);
    if (data != NULL) {
        icd_list__set_dump_func((icd_list *) that, icd_caller_list__standard_dump, NULL);
        icd_list__set_key_check_func((icd_list *) that, icd_caller_list__identify_name);
        /* Override default sort order */
        ins_fn = (icd_list_node * (*)(icd_list * that, void *new_elem, void *extra))
            icd_config__get_value(data, "insert_function");
        extra = icd_config__get_value(data, "insert_extra");
        if (ins_fn == NULL) {
            ins_fn = icd_list__insert_ordered;
        }
        if (extra == NULL) {
            extra = icd_config__get_value(data, "compare.function");
        }
        if (extra == NULL) {
            /* use default fifo cant use icd_caller__cmp_blah bcus this family of funcs take a membership */
            extra = icd_list__insert_fifo;
        }
        retval = icd_list__set_node_insert_func((icd_list *) that, ins_fn, extra);
        retval =
            icd_caller_list__set_state_change_notify_fn(that, icd_config__get_any_value(data,
                "statechange.function", icd_caller_list__dummy_notify), icd_config__get_any_value(data,
                "statechange.extra", NULL));
        retval =
            icd_caller_list__set_channel_attached_notify_fn(that, icd_config__get_any_value(data, "attach.function",
                icd_caller_list__dummy_notify), icd_config__get_any_value(data, "attach.extra", NULL));
        retval =
            icd_caller_list__set_linked_notify_fn(that, icd_config__get_any_value(data, "linked.function",
                icd_caller_list__dummy_notify), icd_config__get_any_value(data, "linked.extra", NULL));
        retval =
            icd_caller_list__set_bridged_notify_fn(that, icd_config__get_any_value(data, "bridged.function",
                icd_caller_list__dummy_notify), icd_config__get_any_value(data, "bridged.extra", NULL));
        retval =
            icd_caller_list__set_authenticate_notify_fn(that, icd_config__get_any_value(data,
                "authenticate.function", icd_caller_list__dummy_notify), icd_config__get_any_value(data,
                "authenticate.extra", NULL));
    } else {
        icd_list__set_dump_func((icd_list *) that, icd_caller_list__standard_dump, NULL);
        icd_list__set_key_check_func((icd_list *) that, icd_caller_list__identify_name);
        ins_fn = icd_list__insert_ordered;
        /* use default fifo cant use icd_caller__cmp_blah bcus this family of funcs take a membership */
        extra = icd_list__insert_fifo;
        retval = icd_list__set_node_insert_func((icd_list *) that, ins_fn, extra);
        retval = icd_caller_list__set_state_change_notify_fn(that, icd_caller_list__dummy_notify, NULL);
        retval = icd_caller_list__set_channel_attached_notify_fn(that, icd_caller_list__dummy_notify, NULL);
        retval = icd_caller_list__set_linked_notify_fn(that, icd_caller_list__dummy_notify, NULL);
        retval = icd_caller_list__set_bridged_notify_fn(that, icd_caller_list__dummy_notify, NULL);
        retval = icd_caller_list__set_authenticate_notify_fn(that, icd_caller_list__dummy_notify, NULL);
    }
    return retval;
}

/* Clear the caller list so that it needs to be reinitialized to be reused. */
icd_status icd_caller_list__clear(icd_caller_list * that)
{
    assert(that != NULL);

    return icd_list__clear((icd_list *) that);
}

/***** Actions *****/

/* Adds a caller to the list, returns success or failure (typesafe wrapper). */
icd_status icd_caller_list__push(icd_caller_list * that, icd_caller * new_call)
{
    icd_status retval;

    assert(that != NULL);
    assert(new_call != NULL);

    retval = icd_list__push((icd_list *) that, new_call);
    return retval;
}

/* Retrieves a caller from the list, returns null if there are none (typesafe wrapper). */
icd_caller *icd_caller_list__pop(icd_caller_list * that)
{
    icd_caller *caller;

    assert(that != NULL);

    caller = (icd_caller *) icd_list__pop((icd_list *) that);
    return caller;
}

/* Returns 1 if there are calls in the list, 0 otherwise. */
int icd_caller_list__has_callers(icd_caller_list * that)
{
    int count;

    assert(that != NULL);

    count = icd_list__count((icd_list *) that);
    return count > 0;
}

/* Returns the numerical position of the caller from the head of the list (0 based), -1 if not present */
int icd_caller_list__caller_position(icd_caller_list * that, icd_caller * target)
{
    assert(that != NULL);
    assert(target != NULL);

    return icd_list__position((icd_list *) that, target);
}

/* Prints the contents of the caller structures to the given file descriptor. */
icd_status icd_caller_list__dump(icd_caller_list * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(((icd_list *) that)->dump_fn != NULL);

    return ((icd_list *) that)->dump_fn((icd_list *) that, verbosity, fd, ((icd_list *) that)->dump_fn_extra);
    return ICD_SUCCESS;
}

/* Retrieves a caller from the list when given an id. */
icd_caller *icd_caller_list__fetch_caller(icd_caller_list * that, char *id)
{
    icd_caller *caller;

    assert(that != NULL);
    assert(id != NULL);

    caller = (icd_caller *) icd_list__fetch((icd_list *) that, id, icd_caller_list__identify_name);
    return caller;
}

/* Removes a caller from the list when given an id, returns success or failure. */
icd_status icd_caller_list__remove_caller(icd_caller_list * that, char *id)
{
    assert(that != NULL);
    assert(id != NULL);

    return icd_list__remove((icd_list *) that, id);
}

/* Removes a caller from the list when given the object itself, returns success or failure. */
icd_status icd_caller_list__remove_caller_by_element(icd_caller_list * that, icd_caller * target)
{
    assert(that != NULL);
    assert(target != NULL);

    return icd_list__remove_by_element((icd_list *) that, target);
}

/***** Getters and Setters *****/

/* Set the list name */
icd_status icd_caller_list__set_name(icd_caller_list * that, char *name)
{
    icd_status retval;

    retval = icd_list__set_name((icd_list *) that, name);
    return retval;
}

/* Get the list name */
char *icd_caller_list__get_name(icd_caller_list * that)
{
    char *name;

    name = icd_list__get_name((icd_list *) that);
    return name;
}

/***** Callback Setters *****/

/* Rollup function for notifying state change of any icd_caller in list */
icd_status icd_caller_list__set_state_change_notify_fn(icd_caller_list * that, int (*state_fn) (icd_event * event,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->state_fn = state_fn;
    that->state_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Rollup function for notifying channel attachment of any icd_caller in list */
icd_status icd_caller_list__set_channel_attached_notify_fn(icd_caller_list * that,
    int (*chan_fn) (icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    that->chan_fn = chan_fn;
    that->chan_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Rollup function for notifying that an icd_caller in list has been linked to another caller */
icd_status icd_caller_list__set_linked_notify_fn(icd_caller_list * that, int (*link_fn) (icd_event * event,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->link_fn = link_fn;
    that->link_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Rollup function for notifying that an icd_caller in list has been bridged */
icd_status icd_caller_list__set_bridged_notify_fn(icd_caller_list * that, int (*bridge_fn) (icd_event * event,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->bridge_fn = bridge_fn;
    that->bridge_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Rollup function for notifying that one of the icd_caller in list has tried to authenticate */
icd_status icd_caller_list__set_authenticate_notify_fn(icd_caller_list * that, int (*authn_fn) (icd_event * event,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->authn_fn = authn_fn;
    that->authn_fn_extra = extra;
    return ICD_SUCCESS;
}

/***** Locking *****/

/* Lock the entire caller list. */
icd_status icd_caller_list__lock(icd_caller_list * that)
{
    assert(that != NULL);

    return icd_list__lock((icd_list *) that);
}

/* Unlock the entire caller list */
icd_status icd_caller_list__unlock(icd_caller_list * that)
{
    assert(that != NULL);

    return icd_list__unlock((icd_list *) that);
}

/**** Listener functions ****/

icd_status icd_caller_list__add_listener(icd_caller_list * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    return icd_list__add_listener((icd_list *) that, listener, lstn_fn, extra);
}

icd_status icd_caller_list__remove_listener(icd_caller_list * that, void *listener)
{
    assert(that != NULL);

    return icd_list__remove_listener((icd_list *) that, listener);
}

/**** Predefined Hooks ****/

int icd_caller_list__dummy_notify(icd_event * event, void *extra)
{
    return 0;
}

/* Standard caller list dump function */
icd_status icd_caller_list__standard_dump(icd_list * list, int verbosity, int fd, void *extra)
{
    icd_caller_list *call_list;
    icd_list_iterator *iter;
    icd_caller *caller;

    assert(list != NULL);
    assert(list->dump_fn != NULL);

    call_list = (icd_caller_list *) list;

    //cw_cli(fd,"\nDumping icd_caller list {\n");
    //icd_list__standard_dump(list, verbosity, fd, ((void *)&skipconst));
    //cw_cli(fd,"       moh=%s\n", icd_caller_list__get_moh(call_list));
    //cw_cli(fd,"   context=%s\n", icd_caller_list__get_context(call_list));
    //cw_cli(fd,"  announce=%s\n", icd_caller_list__get_announce(call_list));

    /* TBD Print these as well (though don't descend on dist)
       icd_distributor *dist;
       int (*state_fn)(icd_caller *caller, int oldstate, int newstate);
       int (*chan_fn)(icd_caller *caller, cw_channel *chan);
       int (*link_fn)(icd_caller *caller, icd_caller *associate);
       int (*bridge_fn)(icd_caller *caller, icd_caller *bridged_to);
       int (*authn_fn)(icd_caller *caller, int id);
       int allocated;
     */

    if (verbosity > 1) {
        cw_cli(fd, "    caller {\n");
        iter = icd_list__get_iterator(list);
        if (iter == NULL) {
            return ICD_ERESOURCE;
        }
        while (icd_list_iterator__has_more(iter)) {
            caller = (icd_caller *) icd_list_iterator__next(iter);
            icd_caller__dump(caller, verbosity - 1, fd);
        }
        destroy_icd_list_iterator(&iter);
        cw_cli(fd, "    }\n");
    } else {
        iter = icd_list__get_iterator(list);
        if (iter == NULL) {
            return ICD_ERESOURCE;
        }
        while (icd_list_iterator__has_more(iter)) {
            caller = (icd_caller *) icd_list_iterator__next(iter);
            icd_caller__dump_debug_fd(caller, fd, "      ");
        }
        destroy_icd_list_iterator(&iter);
    }

    return ICD_SUCCESS;
}

/*===== Private API Implementations =====*/

static int icd_caller_list__identify_name(void *key, void *payload)
{
    icd_caller *caller;
    char *name;

    caller = (icd_caller *) payload;
    name = icd_caller__get_name(caller);

    return (strcmp(name, key) == 0);

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

