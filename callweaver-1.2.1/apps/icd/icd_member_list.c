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
 *  \brief icd_member_list.c - list of member objects
 * 
 * The icd_member_list is a typical icd_list with a few extensions that are
 * specific to keeping a list of icd_members.
 *
 * In addition, it keeps track of a few events fired off by individual
 * icd_member elements and amalgamates them into one place.
 *
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_list_private.h"
#include "callweaver/icd/icd_member.h"
/* For dump function only, should be done from icd_member */
#include "callweaver/icd/icd_caller.h"

/*===== Private APIs, types, and variables =====*/

static icd_module module_id = ICD_MEMBER_LIST;

struct icd_member_list {
    icd_list list;
    icd_memory *memory;
    int allocated;
};

static const int skipconst = 1;

static int icd_member_list__identify_queue(void *key, void *payload);
static int icd_member_list__identify_distributor(void *key, void *payload);
static int icd_member_list__identify_caller(void *key, void *payload);
static int icd_member_list__identify_name(void *key, void *payload);

/*===== Public API Implementations =====*/

/***** Init - Destroyer *****/

/* Constructor for a member list. */
icd_member_list *create_icd_member_list(char *name, icd_config * data)
{
    icd_member_list *list;
    icd_status result;

    assert(data != NULL);
    ICD_MALLOC(list, sizeof(icd_member_list));

    if (list == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Member List\n");
        return NULL;
    }
    list->allocated = 1;

    result = init_icd_member_list(list, name, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(list);
        return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, list, ((icd_list *) list)->name, module_id, ICD_EVENT_CREATE,
        NULL, ((icd_list *) list)->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_member_list(&list);
        return NULL;
    }
    return list;
}

/* Destructor for a member list. */
icd_status destroy_icd_member_list(icd_member_list ** listp)
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
        cw_log(LOG_NOTICE, "Destruction of ICD Member List %s has been vetoed\n",
            icd_member_list__get_name(*listp));
        return ICD_EVETO;
    }

    if ((clear_result = icd_member_list__clear(*listp)) != ICD_SUCCESS) {
        return clear_result;
    }

    if ((*listp)->allocated) {
        that->state = ICD_LIST_STATE_DESTROYED;
        ICD_FREE((*listp));
        *listp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize previously created member list */
icd_status init_icd_member_list(icd_member_list * that, char *name, icd_config * data)
{
    icd_status retval;
    icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
    void *extra;

    assert(that != NULL);
    assert(data != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_member_list));
    retval = init_icd_list((icd_list *) that, data);
    if (retval == ICD_SUCCESS) {
        icd_list__set_name((icd_list *) that, name);
        icd_list__set_dump_func((icd_list *) that, icd_member_list__standard_dump, NULL);
        icd_list__set_key_check_func((icd_list *) that, icd_member_list__identify_name);
        /* Override default sort order */
        ins_fn = (icd_list_node * (*)(icd_list * that, void *new_elem, void *extra))
            icd_config__get_value(data, "insert_function");
        extra = icd_config__get_value(data, "insert_extra");
        if (ins_fn == NULL) {
            ins_fn = icd_list__insert_fifo;
        }
        if (extra == NULL) {
            extra = icd_config__get_value(data, "compare.function");
        }
        retval = icd_list__set_node_insert_func((icd_list *) that, ins_fn, extra);
    }
    return retval;
}

/* Clear the member list so that it needs to be reinitialized to be reused. */
icd_status icd_member_list__clear(icd_member_list * that)
{
    assert(that != NULL);

    return icd_list__clear((icd_list *) that);
}

/***** Actions *****/

/* Adds a member to the list, returns success or failure (typesafe wrapper). */
icd_status icd_member_list__push(icd_member_list * that, icd_member * new_member)
{
    assert(that != NULL);
    assert(new_member != NULL);

    return icd_list__push((icd_list *) that, new_member);
}

/* Retrieves a member from the list, returns null if there are none (typesafe wrapper). */
icd_member *icd_member_list__pop(icd_member_list * that)
{
    assert(that != NULL);

    return (icd_member *) icd_list__pop((icd_list *) that);
}

/* Retrieves a member from the list, returns null if there are none (typesafe wrapper). */
icd_member *icd_member_list__pop_locked(icd_member_list * that)
{
    assert(that != NULL);

    return (icd_member *) icd_list__pop_locked((icd_list *) that);
}
/* Pushback to the top of the list a formerly popped node. */
icd_status icd_member_list__pushback(icd_member_list * that, icd_member * new_member)
{
    icd_status vetoed;
    icd_list_node *new_node;
    icd_list *list;

    assert(that != NULL);
    assert(new_member != NULL);

    /* First, notify event hooks and listeners */
    list = (icd_list *) that;
    vetoed =
        icd_event_factory__notify(event_factory, that, icd_member_list__get_name(that), module_id,
        ICD_EVENT_PUSHBACK, NULL, list->listeners, new_member, list->add_fn, list->add_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Pushing Back to ICD Member List %s has been vetoed\n",
            icd_member_list__get_name(that));
        return ICD_EVETO;
    }

    if (icd_member_list__lock(that) == ICD_SUCCESS) {
        new_node = icd_list__get_node(list);
        if (new_node == NULL) {
            cw_log(LOG_WARNING, "No room in ICD Member List %s to push back an element\n",
                icd_member_list__get_name(that));
            icd_member_list__unlock(that);
            return ICD_ERESOURCE;
        }
        new_node->payload = new_member;

        new_node->next = list->head;
        list->head = new_node;

        if (list->tail == NULL) {
            list->tail = new_node;
        }
        list->count++;
        icd_member_list__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD Member List %s in order to push onto it\n",
        icd_member_list__get_name(that));
    return ICD_ELOCK;
}

/* Returns 1 if there are members in the list, 0 otherwise. */
int icd_member_list__has_members(icd_member_list * that)
{
    int count;

    assert(that != NULL);

    count = icd_list__count((icd_list *) that);
    return count > 0;
}

/* Returns the numerical position of the member from the head of the list (0 based) */
int icd_member_list__member_position(icd_member_list * that, icd_member * target)
{
    assert(that != NULL);
    assert(target != NULL);

    return icd_list__position((icd_list *) that, target);
}

/* Returns the member with the given queue, NULL if none. List is unchanged. */
icd_member *icd_member_list__get_for_queue(icd_member_list * that, icd_queue * queue)
{
    icd_member *member;

    assert(that != NULL);
    assert(queue != NULL);

    member = (icd_member *) icd_list__fetch((icd_list *) that, queue, icd_member_list__identify_queue);
    return member;
}

/* Returns the member with the given distributor, NULL if none. List is unchanged. */
icd_member *icd_member_list__get_for_distributor(icd_member_list * that, icd_distributor * dist)
{
    icd_member *member;

    assert(that != NULL);
    assert(dist != NULL);

    member = (icd_member *) icd_list__fetch((icd_list *) that, dist, icd_member_list__identify_distributor);
    return member;
}

/* Returns the member with the given caller, NULL if none. List is unchanged. */
icd_member *icd_member_list__get_for_caller(icd_member_list * that, icd_caller * caller)
{
    icd_member *member;

    assert(that != NULL);
    assert(caller != NULL);

    member = (icd_member *) icd_list__fetch((icd_list *) that, caller, icd_member_list__identify_caller);
    return member;
}

/* Prints the contents of the member structures to the given file descriptor. */
icd_status icd_member_list__dump(icd_member_list * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(((icd_list *) that)->dump_fn != NULL);

    return ((icd_list *) that)->dump_fn((icd_list *) that, verbosity, fd, ((icd_list *) that)->dump_fn_extra);
}

/* Removes a member from the list when given an id, returns success or failure. */
icd_status icd_member_list__remove_member(icd_member_list * that, char *id)
{
    assert(that != NULL);
    assert(id != NULL);

    return icd_list__remove((icd_list *) that, id);

}

/* Removes a member from the list when given the object itself, returns success or failure. */
icd_status icd_member_list__remove_member_by_element(icd_member_list * that, icd_member * target)
{
    assert(that != NULL);
    assert(target != NULL);

    return icd_list__remove_by_element((icd_list *) that, target);
}

/***** Getters and Setters *****/

/* Set the list name */
icd_status icd_member_list__set_name(icd_member_list * that, char *name)
{
    assert(that != NULL);

    return icd_list__set_name((icd_list *) that, name);
}

/* Get the list name */
char *icd_member_list__get_name(icd_member_list * that)
{
    assert(that != NULL);

    return icd_list__get_name((icd_list *) that);
}

/***** Locking *****/

/* Lock the entire member list. */
icd_status icd_member_list__lock(icd_member_list * that)
{
    assert(that != NULL);

    return icd_list__lock((icd_list *) that);
}

/* Unlock the entire member list */
icd_status icd_member_list__unlock(icd_member_list * that)
{
    assert(that != NULL);

    return icd_list__unlock((icd_list *) that);
}

/**** Listener functions ****/

icd_status icd_member_list__add_listener(icd_member_list * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);

    return icd_list__add_listener((icd_list *) that, listener, lstn_fn, extra);
}

icd_status icd_member_list__remove_listener(icd_member_list * that, void *listener)
{
    assert(that != NULL);

    return icd_list__remove_listener((icd_list *) that, listener);
}

/**** Predefined Hooks ****/

/* Standard member list dump function */
icd_status icd_member_list__standard_dump(icd_list * list, int verbosity, int fd, void *extra)
{
    icd_member_list *member_list;
    icd_list_iterator *iter;
    icd_member *member;
    icd_caller *caller;

    assert(list != NULL);
    assert(list->dump_fn != NULL);

    member_list = (icd_member_list *) list;

    /* temporary hack  
       replacing it means it will produce the same output just the right way

       <HACK>
     */

    iter = icd_list__get_iterator(list);
    if (iter == NULL) {
        return ICD_ERESOURCE;
    }

    while (icd_list_iterator__has_more(iter)) {
        member = (icd_member *) icd_list_iterator__next(iter);
        caller = icd_member__get_caller(member);
        icd_caller__dump(caller, 0, fd);
    }

    /* </HACK> */

    // here is what belongs here.... but not doing anything useful to debug yet, see above
    /*
       cw_cli(fd,"\nDumping icd_member list {\n");
       icd_list__standard_dump(list, verbosity, fd, ((void *)&skipconst));

       if (verbosity > 1) {
       cw_cli(fd,"    member {\n");
       iter = icd_list__get_iterator(list);
       if (iter == NULL) {
       return ICD_ERESOURCE;
       }
       while (icd_list_iterator__has_more(iter)) {
       member = (icd_member *)icd_list_iterator__next(iter);
       icd_member__dump(member, verbosity - 1, fd);
       }
       destroy_icd_list_iterator(&iter);
       cw_cli(fd,"    }\n");
       }
       cw_cli(fd,"}\n");
     */

    return ICD_SUCCESS;
}

/*===== Private Implementations =====*/

/* Returns true if the key (of type icd_queue) is the same as payload (of type icd_member) field "queue" */
static int icd_member_list__identify_queue(void *key, void *payload)
{
    icd_member *member;
    icd_queue *queue;

    member = (icd_member *) payload;
    queue = icd_member__get_queue(member);

    return (queue == key);
}

/* Returns true if the key (of type icd_distributor) is the same as payload (of type icd_member) field "distributor" */
static int icd_member_list__identify_distributor(void *key, void *payload)
{
    icd_member *member;
    icd_distributor *distributor;

    member = (icd_member *) payload;
    distributor = icd_member__get_distributor(member);

    return (distributor == key);
}

/* Returns true if the key (of type icd_caller) is the same as payload (of type icd_member) field "caller" */
static int icd_member_list__identify_caller(void *key, void *payload)
{
    icd_member *member;
    icd_caller *caller;

    member = (icd_member *) payload;
    caller = icd_member__get_caller(member);

    return (caller == key);
}

/* Returns true if the key (of type char *) is the same as payload (of type icd_member) field "name" */
static int icd_member_list__identify_name(void *key, void *payload)
{
    icd_member *member;
    char *name;

    member = (icd_member *) payload;
    name = icd_member__get_name(member);

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

