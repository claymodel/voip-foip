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
 *  \brief icd_list_private.h  - generic thread-safe list
 *
 * ICD_LIST Private Parts
 *
 * If you are not directly inheriting from icd_list, you should pay no
 * attention to the man behind the curtain. Nothing to see here. Return
 * to your homes.
 *
 * Still here? That must mean you need details about the internals of the
 * icd_list structure. Be careful, as you will now have to keep up with
 * any changes to the internal structure. You've been warned.
 *
 */

#ifndef ICD_LIST_PRIVATE_H
#define ICD_LIST_PRIVATE_H

#include "callweaver/lock.h"
#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        ICD_LIST_STATE_CREATED, ICD_LIST_STATE_INITIALIZED,
        ICD_LIST_STATE_CLEARED, ICD_LIST_STATE_DESTROYED,
        ICD_LIST_STATE_LAST_STANDARD
    } icd_list_state;

    extern char *icd_list_state_strings[];

    typedef enum {
        ICD_NODE_STATE_ALLOCATED, ICD_NODE_STATE_FREE, ICD_NODE_STATE_USED,
        ICD_NODE_STATE_LAST_STANDARD
    } icd_node_state;

    typedef enum {
        ICD_LIST_ITERTYPE_PAYLOAD, ICD_LIST_ITERTYPE_NODE, ICD_LIST_ITERTYPE_LAST_STANDARD
    } icd_iterator_type;

    struct icd_list_node {
        icd_list_node *next;
        void *payload;
        icd_node_state state;
        unsigned int flags;
    };

    struct icd_list_iterator {
        icd_list *parent;
        icd_list_node *prev;
        icd_list_node *curr;
        icd_list_node *next;
        icd_iterator_type type;
    };

    struct icd_list {
        char *name;
        icd_list_node *head;
        icd_list_node *tail;
        icd_list_node *cache;
        icd_list_node *first_free;
        icd_list_category category;
        int count;
        int size;
        icd_list_state state;
        unsigned int flags;
        icd_memory *memory;
        int created_as_object;
        int allocated;
        int (*key_fn) (void *key, void *payload);
        icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
        int (*add_fn) (icd_event * that, void *extra);
        int (*del_fn) (icd_event * that, void *extra);
        int (*clr_fn) (icd_event * that, void *extra);
        int (*dstry_fn) (icd_event * that, void *extra);
          icd_status(*dump_fn) (icd_list * that, int verbosity, int fd, void *extra);
        void *ins_fn_extra;
        void *add_fn_extra;
        void *del_fn_extra;
        void *clr_fn_extra;
        void *dstry_fn_extra;
        void *dump_fn_extra;
        icd_listeners *listeners;
        cw_mutex_t lock;
    };

/* Methods available for use of subtypes */

/* Retrieves the first node whose payload the match_fn returns true for */
    icd_list_node *icd_list__fetch_node(icd_list * that, void *key, int (*match_fn) (void *key, void *payload));

/* Fetches the first payload that a callback function returns true for. */
    void *icd_list__fetch(icd_list * that, void *key, int (*match_fn) (void *key, void *payload));

/* Removes the first node that a callback function returns true for */
    icd_status icd_list__drop_node(icd_list * that, void *key, int (*match_fn) (void *key, void *payload));

/* Gets an unused node out of the list's cache */
    icd_list_node *icd_list__get_node(icd_list * list);

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

