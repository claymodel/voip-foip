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
 *  \brief icd_list.c - generic thread-safe list
 * 
 * This is an implementation of a thread-safe list.
 *
 * This implementation implements the list as a combination of singly
 * linked list and array.
 *
 * It has two structures of importance, the node and the list. The node is
 * a forward pointer for a singly linked list as well as a pointer to
 * whatever the node is supposed to contain. That contained element is called
 * the payload. The node also has a state field which keeps track of the
 * state of the node, as well as a (currently unused) flags field.
 *
 * The list is the structure which will most commonly be used by external
 * modules. The main part of the list consists of 4 pointers to nodes. They
 * are: the first node in the list, the last node in the list (to help with
 * FIFO insertions), a pointer to a cache of node structures that the list
 * can use, and a pointer to the first unused node in the cache.
 *
 * When a list is created, an array of node structures is created at the same
 * time and the *cache pointer is pointed to it. All of the nodes in the
 * array belong to either the list of free nodes whose head is kept in
 * *first_free, or to the list of nodes in use within the icd_list, whose head
 * and tail are kept in *head and *tail. You can tell which list any node
 * belongs to by examining its state.
 *
 * By default, the list will be kept in FIFO order though you can change it
 * to anything you want.
 *
 * The implementation of icd_list__insert_ordered() is initially a naive
 * implementation that walks the list on every insert. If that turns out to
 * be too slow, we can implement an array of pointers to each of the
 * payloads and do a binary search on them. Eventually, if we find this is
 * a big enough bottleneck, we could even keep the list in a balanced tree
 * instead of a singly linked list. That would be overkill at this point,
 * though.
 *
 * The list structure also holds a number of function pointers. These are
 * for providing behaviour (*key_fn and *ins_fn) as well as providing
 * notification hooks for events (add_fn, del_fn, clr_fn, dstry_fn).
 * Many of these function pointers take an extra parameter, and those
 * too are stored in the list structure.
 *
 * Because it can be locked for insertions and removals, the list structure
 * holds an cw_lock structure.
 *
 * In addition, it holds an icd_listeners structure to provide listener
 * behaviour to events on the list. A list also has an optional name, a
 * category, a state, a size (which is the size of the node structure array)
 * and a count of the elements in the list.
 *
 * Eventually, we'll probably want to make the list resize itself when it
 * runs out of structure nodes. For now, we'll settle for a warning message
 * suggesting the user either increase the default size or implement the
 * resizing behaviour.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_listeners.h"
#include "callweaver/icd/icd_event.h"
#include "callweaver/icd/icd_memory.h"

/*===== Public APIs =====*/

#include "callweaver/icd/icd_list.h"

/*===== Private Types and APIs =====*/

#include "callweaver/icd/icd_list_private.h"

char *icd_list_state_strings[] = {
    "ICD_LIST_STATE_CREATED", "ICD_LIST_STATE_INITIALIZED",
    "ICD_LIST_STATE_CLEARED", "ICD_LIST_STATE_DESTROYED",
    "ICD_LIST_STATE_L CW_STANDARD"
};

/* Note that none of these functions locks the list. */

/* Initialize the fields in a node */
static void icd_list__init_node(icd_list_node * node);

/* Returns a node to a list's cache */
static void icd_list__free_node(icd_list * list, icd_list_node * node);

/* Function for finding node by payload */
int icd_list__identify_payload(void *key, void *payload);

static icd_module module_id = ICD_LIST;

/*===== Public Implementations  =====*/

/***** Init - Destroyers *****/

/* Create and initializes a list object.*/
icd_list *create_icd_list(icd_config * data)
{
    icd_list *list;
    icd_status result;
    icd_memory *memory;

    /* make a new list from scratch */
    /*   check for passed in memory manager, use it if so */
    if ((memory = icd_config__get_value(data, "memory")) != NULL) {
        ICD_CALLOC(list, icd_list)
            list->memory = memory;
    } else {
        ICD_OBJECT_CALLOC(list, icd_list)
            list->created_as_object = 1;
    }
    if (list == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD List\n");
        return NULL;
    }
    list->allocated = 1;
    list->state = ICD_LIST_STATE_CREATED;
    result = init_icd_list(list, data);
    if (result != ICD_SUCCESS) {
        ICD_OBJECT_FREE(list);
        return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, list, list->name, module_id, ICD_EVENT_CREATE, NULL,
        list->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_list(&list);
        return NULL;
    }
    return list;
}

/* Destroyer of a list object. Returns SUCCESS, ELOCK, EVETO.
 * Note that having nodes left may be a valid result if a listener or hook
 * has vetoed the list destruction.
 */
icd_status destroy_icd_list(icd_list ** listp)
{
    icd_status vetoed;
    int clear_result;

    assert(listp != NULL);
    assert((*listp) != NULL);

    /* First, notify event hooks and listeners */
    vetoed =
        icd_event_factory__notify(event_factory, *listp, (*listp)->name, module_id, ICD_EVENT_DESTROY, NULL,
        (*listp)->listeners, NULL, (*listp)->dstry_fn, (*listp)->dstry_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Destruction of ICD List %s has been vetoed\n", icd_list__get_name(*listp));
        return ICD_EVETO;
    }

    /* Next, clear the list of any nodes and free dependent structures. */
    if ((clear_result = icd_list__clear(*listp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Finally, destroy the list itself if from the heap */
    if ((*listp)->allocated) {
        (*listp)->state = ICD_LIST_STATE_DESTROYED;
        if ((*listp)->created_as_object) {
            ICD_OBJECT_FREE(*listp);
        } else {
            ICD_FREE(*listp);
        }
    }
    return ICD_SUCCESS;
}

/* Initialize a list object */
icd_status init_icd_list(icd_list * that, icd_config * data)
{
    icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
    void *extra;
    int x;
    icd_status vetoed;

    assert(that != NULL);

    cw_mutex_init(&(that->lock));
    that->category = ICD_LIST_CAT_UNKNOWN;
    that->ins_fn = icd_list__insert_fifo;
    /* TBD - create_icd_listeners() should take an icd_memory * argument */
    that->listeners = create_icd_listeners();
    if (data != NULL) {
        icd_config__set_raw(data, "memory", that->memory);
        /* TBD - make config functions use the memory manager on key "memory" if present */
        that->name = icd_config__get_strdup(data, "name", "");
        that->size = icd_config__get_int_value(data, "size", 50);
        /* Set the dump function */
        that->dump_fn = icd_config__get_any_value(data, "dump", icd_list__standard_dump);

        /* Set sort order */
        ins_fn = (icd_list_node * (*)(icd_list * that, void *new_elem, void *extra))
            icd_config__get_value(data, "insert.function");
        extra = icd_config__get_value(data, "insert.extra");
        /* set default order, checks for another name for extra parameter */
        if (ins_fn == NULL) {
            ins_fn = icd_list__insert_fifo;
            extra = NULL;
        } else if (extra == NULL) {
            extra = icd_config__get_value(data, "compare.function");
        }
        icd_list__set_node_insert_func(that, ins_fn, extra);

        /* Set key function if any */
        icd_list__set_key_check_func(that, icd_config__get_value(data, "key.function"));

        /* Set notify function. This incantation will assign the correct function
           pointer (or a dummy one if not specified) and extra parameter (or NULL
           if not specified) */
        icd_list__set_add_node_notify_func(that, icd_config__get_any_value(data, "add.notify.function",
                icd_list__dummy_notify_hook), icd_config__get_value(data, "add.notify.extra"));
        icd_list__set_remove_node_notify_func(that, icd_config__get_any_value(data, "remove.notify.function",
                icd_list__dummy_notify_hook), icd_config__get_value(data, "remove.notify.extra"));
        icd_list__set_clear_list_notify_func(that, icd_config__get_any_value(data, "clear.notify.function",
                icd_list__dummy_notify_hook), icd_config__get_value(data, "clear.notify.extra"));
        icd_list__set_destroy_list_notify_func(that, icd_config__get_any_value(data, "destroy.notify.function",
                icd_list__dummy_notify_hook), icd_config__get_value(data, "destroy.notify.extra"));
    } else {

        that->name = ICD_SUBSTRDUP(that->memory, "");
        that->size = 50;
        that->dump_fn = icd_list__standard_dump;
        ins_fn = icd_list__insert_fifo;
        extra = NULL;
        icd_list__set_node_insert_func(that, ins_fn, extra);
        icd_list__set_key_check_func(that, NULL);
        icd_list__set_add_node_notify_func(that, icd_list__dummy_notify_hook, NULL);
        icd_list__set_remove_node_notify_func(that, icd_list__dummy_notify_hook, NULL);
        icd_list__set_clear_list_notify_func(that, icd_list__dummy_notify_hook, NULL);
        icd_list__set_destroy_list_notify_func(that, icd_list__dummy_notify_hook, NULL);
    }

    /* Create and initialize the cache of nodes */
    ICD_SUBMULTICALLOC(that->cache, that->size, icd_list_node);
    //that->cache = (icd_list_node *) malloc (sizeof(icd_list_node) * that->size);
    if (that->cache == NULL) {
        cw_log(LOG_ERROR, "No memory available to create an ICD List cache\n");
        return ICD_ERESOURCE;
    }
    /* Initialize the free list to the first element in cache */
    that->first_free = that->cache;
    for (x = 0; x < that->size; x++) {
        icd_list__init_node(that->cache + x);
        if (x > 0) {
            icd_list__free_node(that, that->cache + x);
        }
    }
    that->state = ICD_LIST_STATE_INITIALIZED;
    /*ATTENTION! */
    /* this code fails on dynamic dist gotta fix it */

    vetoed = icd_event__generate(ICD_EVENT_INIT, NULL);

    if (vetoed == ICD_EVETO) {
        icd_list__clear(that);
        return ICD_EVETO;
    }

    return ICD_SUCCESS;
}

/* Clear the list so that it has to be reinitialized to be reused.
   Returns SUCCESS, ELOCK, EVETO. Note that having nodes left may be a
   valid result if a listener or hook has vetoed the list destruction.*/
icd_status icd_list__clear(icd_list * that)
{
    icd_status vetoed;
    int count;
    int x;
    void *result;

    assert(that != NULL);

    /* Special case to deal with previously cleared */
    if (that->state == ICD_LIST_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    assert(that->state == ICD_LIST_STATE_INITIALIZED);
    assert(that->cache != NULL);

    /* Notify event hooks and listeners */
    vetoed = icd_event__notify(ICD_EVENT_CLEAR, NULL, that->clr_fn, that->clr_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_WARNING, "Clearing of ICD List %s has been vetoed\n", icd_list__get_name(that));
        return ICD_EVETO;
    }

    /* No veto? Then we won't be needing listeners any more */
    if (that->listeners != NULL) {
        destroy_icd_listeners(&(that->listeners));
    }

    /* Get rid of each node on the list. */
    count = icd_list__count(that);
    for (x = 0; x < count; x++) {
        result = icd_list__pop(that);
    }
    count = icd_list__count(that);
    if (count != 0) {
        cw_log(LOG_WARNING, "Was unable to clear ICD List %s\n", icd_list__get_name(that));
        return ICD_EGENERAL;
    }

    /* Finally, get rid of the cache and mutex and set the state. */
    if (icd_list__lock(that) == ICD_SUCCESS) {
        that->state = ICD_LIST_STATE_CLEARED;
        ICD_SUBFREE(that->cache);
        if (that->name != NULL) {
            ICD_SUBFREE(that->name);
            that->name = NULL;
        }
        icd_list__unlock(that);
        cw_mutex_destroy(&(that->lock));
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD List %s in order to clear it\n", icd_list__get_name(that));
    return ICD_ELOCK;
}

/***** Actions *****/

/* Add an element onto the list. Use the ins_fn to figure out where. */
icd_status icd_list__push(icd_list * that, void *element)
{
    icd_list_node *new_node;
    icd_list_node *prev_node;
    icd_status vetoed;

    assert(that != NULL);
    assert(element != NULL);
    assert(that->ins_fn != NULL);

    /* First, notify event hooks and listeners */
    vetoed = icd_event__notify(ICD_EVENT_PUSH, element, that->add_fn, that->add_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Adding Node to ICD List %s has been vetoed\n", icd_list__get_name(that));
        return ICD_EVETO;
    }

    if (icd_list__lock(that) == ICD_SUCCESS) {
        /* Get insertion point into the list */
        /* List must be locked or the insertion point could be out of date */
        prev_node = that->ins_fn(that, element, that->ins_fn_extra);

        new_node = icd_list__get_node(that);
        if (new_node == NULL) {
            cw_log(LOG_WARNING, "No room in ICD List %s to push an element\n", icd_list__get_name(that));
            icd_list__unlock(that);
            return ICD_ERESOURCE;
        }
        new_node->payload = element;
        if (prev_node == NULL) {
            /* Use the head of the list */
            new_node->next = that->head;
            that->head = new_node;
        } else {
            new_node->next = prev_node->next;
            prev_node->next = new_node;
        }
        if (that->tail == prev_node) {
            that->tail = new_node;
        }
        that->count++;
        icd_list__unlock(that);
        return ICD_SUCCESS;
    }
    cw_log(LOG_WARNING, "Unable to get a lock on ICD List %s in order to push onto it\n",
        icd_list__get_name(that));
    return ICD_ELOCK;
}

/* Pop the top node off of the list and returns its payload.
   Returns NULL if an error occurs, the list is empty, there is a veto,
   or the payload is null. */
void *icd_list__pop(icd_list * that)
{
    void *retval;

    retval = icd_list__pop_locked(that);
    if(retval){
           icd_list__unlock(that);
    }    
    return retval;

}

/* Pop the top node off of the list and returns its payload.
   Returns NULL if an error occurs, the list is empty, there is a veto,
   or the payload is null. If payload is not null - List is locked*/
void *icd_list__pop_locked(icd_list * that)
{
    icd_list_node *node;
    void *retval;
    icd_status vetoed;

    assert(that != NULL);

    node = that->head;
    if (node == NULL) {
        return NULL;
    }

    /* Notify event hooks and listeners */
    vetoed = icd_event__notify(ICD_EVENT_POP, node->payload, that->del_fn, that->del_fn_extra);
    if (vetoed == ICD_EVETO) {
        cw_log(LOG_NOTICE, "Removing Node from ICD List %s has been vetoed\n", icd_list__get_name(that));
        return NULL;
    }

    if (icd_list__lock(that) == ICD_SUCCESS) {
    /*In the meantime node could become invalid */
        node = that->head;
        if (node == NULL) {
           icd_list__unlock(that);
           return NULL;
        }
        that->head = node->next;
        that->count--;
        retval = node->payload;
        if (that->head == NULL) {
            that->tail = NULL;
        }
        icd_list__free_node(that, node);
        if(!retval) {
		icd_list__unlock(that);
	}
        return retval;
    }

    /* Do we need the return to differentiate between this null, the
       empty list null, and the empty element null? Perhaps this function
       needs an icd_status * parameter. */
    cw_log(LOG_WARNING, "Unable to get a lock on ICD List %s in order to pop off of it\n",
        icd_list__get_name(that));
    return NULL;

}
/* Returns the payload of the top node of the list without altering the list.
   Returns NULL if an error occurs, the list is empty, or the payload is null. */
void *icd_list__peek(icd_list * that)
{
    icd_list_node *node;

    assert(that != NULL);

    node = that->head;
    if (node == NULL) {
        return NULL;
    }

    return node->payload;
}

/* Merge the elements from another list into this one. If any of the nodes fail
   to be pushed on to the list, then the return status is the reason it couldn't
   be. Otherwise, the function returns success. Note that this function does not
   alter the nodes from the other list in any way. */
icd_status icd_list__merge(icd_list * that, icd_list * other)
{
    icd_list_iterator *iter;
    void *element;
    icd_status result;
    icd_status final_result = ICD_SUCCESS;

    assert(that != NULL);
    assert(other != NULL);

    iter = icd_list__get_iterator(other);
    if (iter == NULL) {
        return ICD_ERESOURCE;
    }
    while (icd_list_iterator__has_more(iter)) {
        element = icd_list_iterator__next(iter);
        result = icd_list__push(that, element);
        if (result != ICD_SUCCESS) {
            final_result = result;
        }
    }
    destroy_icd_list_iterator(&iter);

    return final_result;
}

/* Returns the number of slots allocated for elements in this list.*/
int icd_list__size(icd_list * that)
{
    assert(that != NULL);

    return that->size;
}

/* Returns the number of elements currently in the list.  */
int icd_list__count(icd_list * that)
{
    assert(that != NULL);

    return that->count;
}

/* Uses key_fn (see icd_list__set_key_check_func) to find the first payload
   in the list that matches the key value. */
void *icd_list__find(icd_list * that, void *key)
{
    assert(that != NULL);
    assert(that->key_fn != NULL);

    return icd_list__fetch(that, key, that->key_fn);
}

/* Returns the position in the list of a particular payload, -1 if not found. */
int icd_list__position(icd_list * that, void *target)
{
    int count;
    icd_list_iterator *iter;
    icd_list_node *node;
    void *payload;

    assert(that != NULL);

    if (icd_list__lock(that) != ICD_SUCCESS) {
       return -1;
    }
    iter = icd_list__get_node_iterator(that);
    if (iter == NULL) {
        icd_list__unlock(that);
        return -1;
    }
    count = 0;
    while (icd_list_iterator__has_more_nolock(iter)) {
        node = icd_list_iterator__next(iter);
        payload = icd_list__get_payload(node);
        if (payload == target) {
            destroy_icd_list_iterator(&iter);
            icd_list__unlock(that);
            return count;
        }
        count++;
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock(that);
    return -1;
}

/* Uses key_fn to find a node to remove based on its key value. */
icd_status icd_list__remove(icd_list * that, void *key)
{
    assert(that != NULL);
    assert(that->key_fn != NULL);

    return icd_list__drop_node(that, key, that->key_fn);
}

/* Removes a node based on its payload. */
icd_status icd_list__remove_by_element(icd_list * that, void *payload)
{
    assert(that != NULL);

    return icd_list__drop_node(that, payload, icd_list__identify_payload);
}

/* Print out a copy of the list */
icd_status icd_list__dump(icd_list * that, int verbosity, int fd)
{
    assert(that != NULL);
    assert(that->dump_fn != NULL);

    return that->dump_fn(that, verbosity, fd, that->dump_fn_extra);
}

/***** Node behaviours *****/

/* Get the structure the list is holding out of a node. */
void *icd_list__get_payload(icd_list_node * that)
{
    assert(that != NULL);

    return that->payload;
}

/***** Getters and Setters *****/

/* Sets the name of the list */
icd_status icd_list__set_name(icd_list * that, char *name)
{
    assert(that != NULL);

    if (that->name != NULL) {
        ICD_SUBFREE(that->name);
    }
    that->name = strdup(name);
    return ICD_SUCCESS;
}

/* Gets the name of the list or the empty string if none. */
char *icd_list__get_name(icd_list * that)
{
    assert(that != NULL);

    if (that->name == NULL) {
        return "";
    }
    return that->name;
}

/* Sets the memory manager for the list */
icd_status icd_list__set_memory(icd_list * that, icd_memory * memory_manager)
{
    assert(that != NULL);
    assert(that->created_as_object == 0);

    that->memory = memory_manager;
    return ICD_SUCCESS;
}

/* Gets the memory manager for the list */
icd_memory *icd_list__get_memory(icd_list * that)
{
    assert(that != NULL);

    return that->memory;
}

/***** Callback Setters *****/

/* Allows you to provide a function that does inserts into the list.
 * The extra parameter will be passed into the function on each call.
 */
icd_status icd_list__set_node_insert_func(icd_list * that, icd_list_node * (*ins_fn) (icd_list * that,
        void *new_elem, void *extra), void *extra)
{
    assert(that != NULL);

    that->ins_fn = ins_fn;
    that->ins_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Allows you to provide a function that can compare a node in the list to a key value. */
icd_status icd_list__set_key_check_func(icd_list * that, int (*key_fn) (void *key, void *payload))
{
    assert(that != NULL);

    that->key_fn = key_fn;
    return ICD_SUCCESS;
}

/* Allows you to provide a function which gets called when a node is added to the list.
 * The extra parameter will be passed into the function on each call.
 */
icd_status icd_list__set_add_node_notify_func(icd_list * that, int (*add_fn) (icd_event * that, void *extra),
    void *extra)
{
    assert(that != NULL);

    that->add_fn = add_fn;
    that->add_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Allows you to provide a function which gets called when a node is removed from the list.
 * The extra parameter will be passed into the function on each call.
 */
icd_status icd_list__set_remove_node_notify_func(icd_list * that, int (*del_fn) (icd_event * that, void *extra),
    void *extra)
{
    assert(that != NULL);

    that->del_fn = del_fn;
    that->del_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Allows you to provide a function which gets called when the list is cleared.
 * The extra parameter will be passed into the function on each call.
 */
icd_status icd_list__set_clear_list_notify_func(icd_list * that, int (*clr_fn) (icd_event * that, void *extra),
    void *extra)
{
    assert(that != NULL);

    that->clr_fn = clr_fn;
    that->clr_fn_extra = extra;
    return ICD_SUCCESS;
}

/* Allows you to provide a function which gets called when the list is destroyed.
 * The extra parameter will be passed into the function on each call.
 */
icd_status icd_list__set_destroy_list_notify_func(icd_list * that, int (*dstry_fn) (icd_event * that, void *extra),
    void *extra)
{
    assert(that != NULL);

    that->dstry_fn = dstry_fn;
    that->dstry_fn_extra = extra;
    return ICD_SUCCESS;
}

icd_status icd_list__set_dump_func(icd_list * that, icd_status(*dump_fn) (icd_list *, int verbosity, int fd,
        void *extra), void *extra)
{
    assert(that != NULL);

    that->dump_fn = dump_fn;
    that->dump_fn_extra = extra;
    return ICD_SUCCESS;
}

/***** Locking *****/

/* Lock the list. If the mutex is of type errorcheck we should check EDEADLK
   in case the thread locks an object twice. If we decide to use
   trylocks, we need to check for EBUSY. EINVAL indicates the mutex wasn't initialized.*/
icd_status icd_list__lock(icd_list * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_LIST_STATE_CLEARED || that->state == ICD_LIST_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
   
   if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] try to lock\n", that->name);
    retval = cw_mutex_lock(&that->lock);
    if (retval == 0) {
       if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] locked\n", that->name);
        return ICD_SUCCESS;
    }
    if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] lock failed\n", that->name);
    return ICD_ELOCK;
}

/* Unlock the list. Check error codes EINVAL (mutex not initialized) and EPERM. */
icd_status icd_list__unlock(icd_list * that)
{
    int retval;

    assert(that != NULL);

    if (that->state == ICD_LIST_STATE_DESTROYED) {
        return ICD_ERESOURCE;
    }
    if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] try to unlock\n", that->name);
    retval = cw_mutex_unlock(&that->lock);
    if (retval == 0) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] unlocked\n", that->name);
        return ICD_SUCCESS;
    }
    if (icd_debug)
            cw_log(LOG_DEBUG, "List [%s] unlock failed\n", that->name);
    return ICD_ELOCK;
}

/**** Listeners ****/

icd_status icd_list__add_listener(icd_list * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__add(that->listeners, listener, lstn_fn, extra);
    return ICD_SUCCESS;
}

icd_status icd_list__remove_listener(icd_list * that, void *listener)
{
    assert(that != NULL);
    assert(that->listeners != NULL);

    icd_listeners__remove(that->listeners, listener);
    return ICD_SUCCESS;
}

/**** Iterator functions ****/

/* Returns an iterator to the list. Iterates in list order. */
icd_list_iterator *icd_list__get_iterator(icd_list * that)
{
    icd_list_iterator *iter;

    assert(that != NULL);
    ICD_SUBCALLOC(iter, icd_list_iterator);
    if (iter == NULL) {
        cw_log(LOG_ERROR, "No memory available to create an iterator on ICD list %s\n", icd_list__get_name(that));
        return NULL;
    }
    memset(iter, 0, sizeof(icd_list_iterator));

    iter->parent = that;
    iter->type = ICD_LIST_ITERTYPE_PAYLOAD;
    iter->next = that->head;
    return iter;
}

/* Returns an iterator of the nodes on the list. Iterates in list order. */
icd_list_iterator *icd_list__get_node_iterator(icd_list * that)
{
    icd_list_iterator *iter;

    assert(that != NULL);

    iter = icd_list__get_iterator(that);
    if (iter != NULL) {
        iter->type = ICD_LIST_ITERTYPE_NODE;
    }
    return iter;
}

/* Queue Iterator destructor. */
icd_status destroy_icd_list_iterator(icd_list_iterator ** iterp)
{
    assert(iterp != NULL);
    assert((*iterp) != NULL);
    ICD_SUBFREE(*iterp);
    return ICD_SUCCESS;
}

/* Indicates whether there are more elements in the list. */
int icd_list_iterator__has_more(icd_list_iterator * that)
{
    assert(that != NULL);
    if (!that->next || !that->parent) {
        return 0;
    }
    cw_mutex_lock(&that->parent->lock);
    if (that && that->next && that->next->state && that->next->state != ICD_NODE_STATE_USED && that->curr
        && that->curr->state && that->curr->state == ICD_NODE_STATE_USED) {
        that->next = that->curr->next;
    }
    cw_mutex_unlock(&that->parent->lock);
    return (that->next != NULL);
}

/* Indicates whether there are more elements in the list. */
int icd_list_iterator__has_more_nolock(icd_list_iterator * that)
{
    assert(that != NULL);
    if (!that->next || !that->parent) {
        return 0;
    }
    if (that && that->next && that->next->state && that->next->state != ICD_NODE_STATE_USED && that->curr
        && that->curr->state && that->curr->state == ICD_NODE_STATE_USED) {
        that->next = that->curr->next;
    }
    return (that->next != NULL);
}
/* Returns the next element from the list. */
void *icd_list_iterator__next(icd_list_iterator * that)
{
    assert(that != NULL);

    /* We should have more smarts in here to deal with new and removed nodes.
       This will handle the current node being deleted, but gets non-
       deterministic if much more happens. */
    if (that->next == NULL) {
        if (that->curr == NULL) {
            return NULL;
        } else {
            that->next = that->curr->next;
        }
    }
    if (that->next->state != ICD_NODE_STATE_USED && that->curr != NULL && that->curr->state == ICD_NODE_STATE_USED) {
        that->next = that->curr->next;
    }
    that->curr = that->next;
    if (that->curr != NULL) {
        that->next = that->curr->next;
    }
    if (that->curr == NULL) {
        return NULL;
    }
    if (that->type == ICD_LIST_ITERTYPE_PAYLOAD) {
        return that->curr->payload;
    }
    return that->curr;
}

/**** Predefined Pluggable Behaviours ****/

/* This does precisely nothing */
int icd_list__dummy_notify_hook(icd_event * event, void *extra)
{
    return 0;
}

/* Return the last element on the list, so that the new element
   can be placed after it */
icd_list_node *icd_list__insert_fifo(icd_list * that, void *new_elem, void *a)
{
    assert(that != NULL);

    return that->tail;
}

/* Return NULL to indicate the new element goes at the head of the list */
icd_list_node *icd_list__insert_lifo(icd_list * that, void *new_elem, void *a)
{
    assert(that != NULL);

    return NULL;
}

/* Eventually, this should calculate a node to insert at and iterate to there */
icd_list_node *icd_list__insert_random(icd_list * that, void *new_elem, void *seed)
{
    assert(that != NULL);

    return NULL;
}

/* Insert in order determined by cmp_fn. Remember the list is locked at
   this point. This implementation is naive and can easily be improved. */
icd_list_node *icd_list__insert_ordered(icd_list * that, void *new_elem, void *cmp_fn)
{
    int count;
    icd_list_iterator *iter;
    icd_list_node *node = NULL;
    icd_list_node *lastnode = NULL;
    int (*compare_fn) (void *, void *);

    assert(that != NULL);
    assert(cmp_fn != NULL);

    compare_fn = (int (*)(void *, void *)) (cmp_fn);
    count = icd_list__count(that);
    if (count == 0) {
        return NULL;
    }

    iter = icd_list__get_node_iterator(that);
    if (iter == NULL) {
        return NULL;
    }
    while (icd_list_iterator__has_more(iter)) {
        lastnode = node;
        node = icd_list_iterator__next(iter);
        if (node == NULL) {
            destroy_icd_list_iterator(&iter);
            return lastnode;
        }
        if (compare_fn(node->payload, new_elem) > 0) {
            destroy_icd_list_iterator(&iter);
            return lastnode;
        }
    }
    destroy_icd_list_iterator(&iter);

    return that->tail;
}

/* Standard function for printing out a copy of the list */
icd_status icd_list__standard_dump(icd_list * list, int verbosity, int fd, void *extra)
{
    icd_list_iterator *iter;
    void *element;
    int skip_opening;

    assert(list != NULL);

    if (extra == NULL) {
        skip_opening = 0;
    } else {
        skip_opening = *((int *) extra);
    }
    if (skip_opening == 0) {
        cw_cli(fd, "\nDumping icd_list {\n");
    }
    cw_cli(fd, "      name=%s\n", icd_list__get_name(list));
    cw_cli(fd, "     count=%d\n", list->count);
    cw_cli(fd, "      size=%d\n", list->size);
    cw_cli(fd, "     state=%d\n", list->state);
    cw_cli(fd, "  category=%d\n", list->category);
    if (verbosity > 2) {
        cw_cli(fd, "      head=%p\n", list->head);
        cw_cli(fd, "      tail=%p\n", list->tail);
        cw_cli(fd, "     cache=%p\n", list->cache);
        cw_cli(fd, "first_free=%p\n", list->first_free);
        cw_cli(fd, " listeners=%p\n", list->listeners);
        cw_cli(fd, "     flags=%u\n", list->flags);
        cw_cli(fd, "    key_fn=%p\n", list->key_fn);
        cw_cli(fd, "    ins_fn=%p\n", list->ins_fn);
        cw_cli(fd, "    add_fn=%p\n", list->add_fn);
        cw_cli(fd, "    del_fn=%p\n", list->del_fn);
        cw_cli(fd, "    clr_fn=%p\n", list->clr_fn);
        cw_cli(fd, "  dstry_fn=%p\n", list->dstry_fn);
        cw_cli(fd, "   dump_fn=%p\n", list->dump_fn);
    }
    if (verbosity > 3) {
        cw_cli(fd, " ins_extra=%p\n", list->ins_fn_extra);
        cw_cli(fd, " add_extra=%p\n", list->add_fn_extra);
        cw_cli(fd, " del_extra=%p\n", list->del_fn_extra);
        cw_cli(fd, " clr_extra=%p\n", list->clr_fn_extra);
        cw_cli(fd, "dstry_xtra=%p\n", list->dstry_fn_extra);
        cw_cli(fd, " dump_xtra=%p\n", list->dump_fn_extra);
    }

    if (skip_opening == 0 && verbosity > 1) {
        cw_cli(fd, "    nodes {\n");
        iter = icd_list__get_iterator(list);
        if (iter == NULL) {
            return ICD_ERESOURCE;
        }
        while (icd_list_iterator__has_more(iter)) {
            element = icd_list_iterator__next(iter);
            cw_cli(fd, "       payload=%p", element);
        }
        destroy_icd_list_iterator(&iter);
        cw_cli(fd, "    }\n");
    }
    if (skip_opening == 0) {
        cw_cli(fd, "}\n");
    }
    return ICD_SUCCESS;
}

/***** Comparison functions ("void *" are all "icd_list *") *****/

int icd_list__cmp_name_order(void *arg1, void *arg2)
{
    icd_list *list1;
    icd_list *list2;

    assert(arg1 != NULL);
    assert(arg2 != NULL);

    list1 = (icd_list *) arg1;
    list2 = (icd_list *) arg2;

    return strcmp(list1->name, list2->name);
}

int icd_list__cmp_name_reverse_order(void *arg1, void *arg2)
{
    return icd_list__cmp_name_order(arg2, arg1);
}

/***** Key functions *****/

int icd_list__by_name(void *key, void *list)
{

    if (list == NULL) {
        return 0;
    }
    return (strcmp(((icd_list *) list)->name, (char *) key) == 0);
}

/*===== Protected Implementations =====*/

/* Fetches the first node that a callback function returns true for. */
icd_list_node *icd_list__fetch_node(icd_list * that, void *key, int (*match_fn) (void *key, void *payload))
{
    icd_list_iterator *iter;
    icd_list_node *node;
    void *payload;

    assert(that != NULL);
    assert(match_fn != NULL);

    if (icd_list__lock(that) != ICD_SUCCESS) {
            return NULL;
    }
    iter = icd_list__get_node_iterator(that);
    if (iter == NULL) {
        icd_list__unlock(that);
        return NULL;
    }
    while (icd_list_iterator__has_more_nolock(iter)) {
        node = icd_list_iterator__next(iter);
        payload = icd_list__get_payload(node);
        if (match_fn(key, payload)) {
            destroy_icd_list_iterator(&iter);
            icd_list__unlock(that);
            return node;
        }
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock(that);
    return NULL;
}

/* Fetches the first payload that a callback function returns true for. */
void *icd_list__fetch(icd_list * that, void *key, int (*match_fn) (void *key, void *payload))
{
    icd_list_node *node;

    assert(that != NULL);
    assert(match_fn != NULL);

    node = icd_list__fetch_node(that, key, match_fn);
    if (node == NULL) {
        return NULL;
    }
    return icd_list__get_payload(node);
}

/* Removes the first node that a callback function returns true for */
icd_status icd_list__drop_node(icd_list * that, void *key, int (*match_fn) (void *key, void *payload))
{
    icd_list_iterator *iter;
    icd_list_node *node;
    icd_list_node *prevnode = NULL;
    void *payload;
    icd_status vetoed;

    assert(that != NULL);
    assert(match_fn != NULL);

    if (icd_list__lock(that) != ICD_SUCCESS) {
            return ICD_ELOCK;
    }
    iter = icd_list__get_node_iterator((icd_list *) that);
    if (iter == NULL) {
        icd_list__unlock(that);
        return ICD_ERESOURCE;
    }
    while (icd_list_iterator__has_more_nolock(iter)) {
        node = icd_list_iterator__next(iter);
        payload = icd_list__get_payload(node);
        if (match_fn(key, payload)) {
            destroy_icd_list_iterator(&iter);
            vetoed = icd_event__notify(ICD_EVENT_REMOVE, node->payload, that->del_fn, that->del_fn_extra);
            if (vetoed == ICD_EVETO) {
                cw_log(LOG_NOTICE, "Removal of Node from ICD List %s has been vetoed\n", icd_list__get_name(that));
                icd_list__unlock(that);
                return ICD_EVETO;
            }

            if (that->head == node) {
                    that->head = node->next;
            }
            if (that->tail == node) {
                    that->tail = prevnode;
            }
            if (prevnode != NULL) {
                    prevnode->next = node->next;
            }
            that->count--;
            icd_list__free_node(that, node);
            icd_list__unlock(that);
            return ICD_SUCCESS;
        }
        prevnode = node;
    }
    destroy_icd_list_iterator(&iter);
    icd_list__unlock(that);
    return ICD_ENOTFOUND;
}

/* Gets an unused node out of the list's cache, returns NULL if fresh out */
icd_list_node *icd_list__get_node(icd_list * list)
{
    icd_list_node *node;

    assert(list != NULL);

    node = list->first_free;
    if (node == NULL) {
        cw_log(LOG_WARNING,
            "Out of nodes to store element in ICD List.\nEither create "
            "the list %s with a larger size, or implement resizing in icd_list.c\n", icd_list__get_name(list));
        return NULL;
    }
    list->first_free = node->next;
    node->state = ICD_NODE_STATE_USED;
    return node;
}

/*===== Private Implementations  =====*/

/* Initialize the fields in a node */
static void icd_list__init_node(icd_list_node * node)
{

    assert(node != NULL);

    memset(node, 0, sizeof(icd_list_node));
    node->state = ICD_NODE_STATE_ALLOCATED;
}

/* Returns a node to a list's cache */
static void icd_list__free_node(icd_list * list, icd_list_node * node)
{

    assert(list != NULL);
    assert(node != NULL);

    node->state = ICD_NODE_STATE_FREE;
    node->next = list->first_free;
    list->first_free = node;
}

/* To find a node by its payload */
int icd_list__identify_payload(void *key, void *payload)
{
    return (key == payload);
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

