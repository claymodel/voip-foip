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
 *  \brief icd_listeners.c - collection of listeners
 * 
 * The icd_listeners module provides a collection of listeners. Each module
 * in ICD provides an interface for adding listeners to each instance. As
 * events occur on the instance, it sends the events through its collection
 * of listeners, which have the opportunity to veto the event if desired.
 *
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

/*===== Public APIs =====*/

#include "callweaver/icd/icd_listeners.h"

/*===== Private Types and APIs =====*/

typedef struct icd_listener_node icd_listener_node;
struct icd_listener_node {
    icd_listener_node *next;
    void *listener;
    int (*lstn_fn) (void *listener, icd_event * event, void *extra);
    icd_memory *memory;
    void *extra;
};

struct icd_listeners {
    icd_listener_node *first;
    icd_listener_node *last;
    icd_memory *memory;
    int allocated;
};

static icd_listener_node *create_icd_listener_node(void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra);

static icd_status destroy_icd_listener_node(icd_listener_node ** nodep);

/*===== Public Implementations  =====*/

/***** Init - Destroyers *****/

/* Constructor for a listeners object. */
icd_listeners *create_icd_listeners(void)
{
    icd_listeners *listeners;
    icd_status result;

    /* make a new listeners collection from scratch */
    ICD_MALLOC(listeners, sizeof(icd_listeners));

    if (listeners == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Listeners collection\n");
        return NULL;
    }
    listeners->allocated = 1;
    result = init_icd_listeners(listeners);
    if (result != ICD_SUCCESS) {
        ICD_FREE(listeners);
        return NULL;
    }

    return listeners;
}

/* Remove the listener and clear its pointer. */
icd_status destroy_icd_listeners(icd_listeners ** listenersp)
{
    int clear_result;

    assert(listenersp != NULL);
    assert(*listenersp != NULL);

    if ((clear_result = icd_listeners__clear(*listenersp)) != ICD_SUCCESS) {
        return clear_result;
    }
    if ((*listenersp)->allocated) {
        ICD_FREE((*listenersp));
        *listenersp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize the icd_listeners structure. */
icd_status init_icd_listeners(icd_listeners * that)
{
    assert(that != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_listeners));
    return ICD_SUCCESS;
}

/* Clear out the icd_listeners structure. Reinitialize before using again. */
icd_status icd_listeners__clear(icd_listeners * that)
{
    icd_listener_node *curr;
    icd_listener_node *next;

    assert(that != NULL);

    curr = that->first;
    while (curr != NULL) {
        next = curr->next;
        icd_listeners__remove(that, curr->listener);
        curr = next;
    }
    that->first = NULL;
    that->last = NULL;
    return ICD_SUCCESS;
}

/***** Actions *****/

/* Add a listener to the collection. */
icd_status icd_listeners__add(icd_listeners * that, void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    icd_listener_node *newnode;
    icd_listener_node *prevnode;

    assert(that != NULL);
    assert(lstn_fn != NULL);

    newnode = create_icd_listener_node(listener, lstn_fn, extra);
    if (newnode == NULL) {
        return ICD_ERESOURCE;
    }

    if (that->first == NULL) {
        that->first = newnode;
        that->last = newnode;
        return ICD_SUCCESS;
    }
    prevnode = that->last;
    prevnode->next = newnode;
    that->last = newnode;
    return ICD_SUCCESS;
}

/* Remove the listener from this collection. */
icd_status icd_listeners__remove(icd_listeners * that, void *listener)
{
    icd_listener_node *curr;
    icd_listener_node *prev;
    icd_status result;

    assert(that != NULL);

    curr = that->first;
    prev = NULL;
    while (curr != NULL) {
        if (curr->listener == listener) {
            if (prev == NULL) {
                that->first = curr->next;
            } else {
                prev->next = curr->next;
            }
            if (that->last == curr) {
                that->last = prev;
            }
            result = destroy_icd_listener_node(&curr);
            return result;
        }
        prev = curr;
        curr = prev->next;
    }

    return ICD_ENOTFOUND;
}

/* Notify all listeners that an event has occured. */
int icd_listeners__notify(icd_listeners * that, icd_event * event)
{
    icd_listener_node *curr;

    assert(that != NULL);

    /* Call manager_event here ??? */

    curr = that->first;
    while (curr != NULL) {
        if (curr->lstn_fn(curr->listener, event, curr->extra) != 0) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

/* Print the contents of the listener collection. */
icd_status icd_listeners__dump(icd_listeners * that, int fd)
{
    return ICD_SUCCESS;
}

/*===== Private Implementations  =====*/

static icd_listener_node *create_icd_listener_node(void *listener, int (*lstn_fn) (void *listener,
        icd_event * event, void *extra), void *extra)
{
    icd_listener_node *newnode;

    /* make a new list from scratch */
    ICD_MALLOC(newnode, sizeof(icd_listener_node));
    if (newnode == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Listener Node\n");
        return NULL;
    }

    newnode->listener = listener;
    newnode->lstn_fn = lstn_fn;
    newnode->extra = extra;
    return newnode;
}

static icd_status destroy_icd_listener_node(icd_listener_node ** nodep)
{
    assert(nodep != NULL);
    assert((*nodep) != NULL);

    ICD_FREE((*nodep));
    (*nodep) = NULL;
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

