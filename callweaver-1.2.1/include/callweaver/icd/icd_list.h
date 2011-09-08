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
 *  \brief icd_list.h  - generic thread-safe list
 *
 * ICD_LIST
 *
 * The icd_list module provides an implementation of a list that allows you
 * to keep the elements in a user-determined order, and to push and pop
 * elements on and off the list in a thread safe manner.
 *
 * It also provides hooks so that your program can be notified when an event
 * on the list occurs, as well as listeners that can register to be notified
 * of events. The difference between the notification hooks and the listeners
 * is that the listeners are called for every event but there can be any number
 * of them. The notification hook is called only for its particular event, but
 * there can be only one (unless they choose to chain together).
 *
 * The list provides for a pluggable sort order by allowing you to replace the
 * method that identifies the insertion point in the list for a new node. A null
 * value means the top of the list, any other value identifies the node that will
 * precede the new node in the list. There are four standard behaviours provided
 * for you that you can use out of the box for this. They are fifo, lifo, random,
 * and an in-place-sort order based on a pluggable comparison function.
 *
 * Comparison functions are kept in the modules containing the structures they
 * will be comparing. In the icd_list module there are two comparison functions,
 * icd_list__cmp_name_order and icd_list__cmp_name_reverse_order, which are
 * used when storing lists in a list (ie. in icd_metalist).
 *
 * Note that the icd_list_node is a structure that keeps a position in the
 * list and also holds the contents of the element you are storing in the
 * list, and for the most part it is not used externally. The one place where
 * they are required is in the pluggable function that returns the insertion
 * point node. For that reason, a node iterator and a function to extract the
 * payload of the node are provided for the use of that pluggable function.
 *
 * When writing an insertion point function, there are a few things to keep in
 * mind. Always use the node iterator and make sure you return an icd_list node.
 * On entry into your function the list will be locked, so don't lock it again
 * or unlock it. Try not to try to lock some other entity inside your function
 * as that provides the potential for race conditions. If you do have to lock
 * something inside the function, make very sure nothing could already have
 * locked that something and then tried to lock this list.
 *
 */

#ifndef ICD_LIST_H
#define ICD_LIST_H

#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constructors and Destructors *****/

/* Constructor for a list object. Name is optional, use NULL if none. */
    icd_list *create_icd_list(icd_config * data);

/* Destructor for a list object. Automatically resets the list pointer
 * passed in to NULL. Only use this for objects created with
 * create_icd_list.
 */
    icd_status destroy_icd_list(icd_list ** listp);

/* Initializer for an already created list object. */
    icd_status init_icd_list(icd_list * that, icd_config * data);

/* Clear the list so it needs to be reinitialized to be reused. Note that
 * the objects that the nodes contain are not freed, as the list doesn't
 * know how to do that. So only use this if you are tracking the payloads
 * elsewhere. (Parallel to init_icd_list).
 */
    icd_status icd_list__clear(icd_list * that);

/***** Behaviours *****/

/* Add an element onto the list. */
    icd_status icd_list__push(icd_list * that, void *element);

/* Pop the top element off of the list. */
    void *icd_list__pop(icd_list * that);

/* Pop the top element off of the list -list is locked if retunr value is not null */
    void *icd_list__pop_locked(icd_list * that);

/* Look at the top element of the list without removing it. */
    void *icd_list__peek(icd_list * that);

/* Merge the elements from another list into this one. */
    icd_status icd_list__merge(icd_list * that, icd_list * other);

/* Returns the number of slots allocated for elements in this list. */
    int icd_list__size(icd_list * that);

/* Returns the number of elements currently in the list.  */
    int icd_list__count(icd_list * that);

/* Uses key_fn (see icd_list__set_key_check_func) to find the first payload
   in the list that matches the key value. */
    void *icd_list__find(icd_list * that, void *key);

/* Returns the position in the list of a particular payload, -1 if not found */
    int icd_list__position(icd_list * that, void *target);

/* Uses key_fn to find a node to remove based on its key value. */
    icd_status icd_list__remove(icd_list * that, void *key);

/* Removes a node based on its payload. */
    icd_status icd_list__remove_by_element(icd_list * that, void *payload);

/* Print out a copy of the list */
    icd_status icd_list__dump(icd_list * that, int verbosity, int fd);

/***** Node behaviours *****/

/* Get the structure that the list is holding out of a node. */
    void *icd_list__get_payload(icd_list_node * that);

/***** Getters and Setters *****/

/* Sets the name of the list */
    icd_status icd_list__set_name(icd_list * that, char *name);

/* Gets the name of the list */
    char *icd_list__get_name(icd_list * that);

/* Sets the memory manager for the list */
    icd_status icd_list__set_memory(icd_list * that, icd_memory * memory_manager);

/* Gets the memory manager for the list */
    icd_memory *icd_list__get_memory(icd_list * that);

/***** Callback Setters *****/

/* Allows you to provide a function that decides where to do inserts into the
 * list. The "extra" parameter will be passed into the function on each call.
 */
    icd_status icd_list__set_node_insert_func(icd_list * that, icd_list_node * (*ins_fn) (icd_list * that,
            void *new_elem, void *extra), void *extra);

/* Allows you to provide a function that can check a key value against a
   payload in the list, returning non-zero on a match. */
    icd_status icd_list__set_key_check_func(icd_list * that, int (*key_fn) (void *key, void *payload));

/* Allows you to provide a function which gets called when a node is added to
 * the list. The extra parameter will be passed into the function on each call.
 */
    icd_status icd_list__set_add_node_notify_func(icd_list * that, int (*add_fn) (icd_event * that, void *extra),
        void *extra);

/* Allows you to provide a function which gets called when a node is removed
 * from the list. The extra parameter will be passed into the function on each 
 * call.
 */
    icd_status icd_list__set_remove_node_notify_func(icd_list * that, int (*del_fn) (icd_event * that, void *extra),
        void *extra);

/* Allows you to provide a function which gets called when the list is cleared.
 * The extra parameter will be passed into the function on each call.
 */
    icd_status icd_list__set_clear_list_notify_func(icd_list * that, int (*clr_fn) (icd_event * that, void *extra),
        void *extra);

/* Allows you to provide a function which gets called when the list is destroyed.
 * The extra parameter will be passed into the function on each call.
 */
    icd_status icd_list__set_destroy_list_notify_func(icd_list * that, int (*dstry_fn) (icd_event * that,
            void *extra), void *extra);

/* Set the dump function for this list */
    icd_status icd_list__set_dump_func(icd_list * that, icd_status(*dump_fn) (icd_list * list, int verbosity,
            int fd, void *extra), void *extra);

/***** Locking *****/

/* Lock the list */
    icd_status icd_list__lock(icd_list * that);

/* Unlock the list */
    icd_status icd_list__unlock(icd_list * that);

/***** Listeners *****/

    icd_status icd_list__add_listener(icd_list * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_list__remove_listener(icd_list * that, void *listener);

/***** Iterator functions *****/

/* Returns an iterator to the list. Iterates in list order. */
    icd_list_iterator *icd_list__get_iterator(icd_list * that);

/* Returns an iterator of the nodes on the list. Iterates in list order. */
    icd_list_iterator *icd_list__get_node_iterator(icd_list * that);

/* Queue Iterator destructor. */
    icd_status destroy_icd_list_iterator(icd_list_iterator ** iterp);

/* Indicates whether there are more elements in the list. */
    int icd_list_iterator__has_more(icd_list_iterator * that);

/* Indicates whether there are more elements in the list. */
    int icd_list_iterator__has_more_nolock(icd_list_iterator * that);

/* Returns the next element from the list. */
    void *icd_list_iterator__next(icd_list_iterator * that);

/***** Predefined Pluggable Behaviours *****/

    int icd_list__dummy_notify_hook(icd_event * event, void *extra);

    icd_list_node *icd_list__insert_fifo(icd_list * that, void *new_elem, void *extra);
    icd_list_node *icd_list__insert_lifo(icd_list * that, void *new_elem, void *extra);
    icd_list_node *icd_list__insert_random(icd_list * that, void *new_elem, void *seed);
    icd_list_node *icd_list__insert_ordered(icd_list * that, void *new_elem, void *cmp_fn);

/* Standard caller list dump function */
    icd_status icd_list__standard_dump(icd_list * list, int verbosity, int fd, void *extra);

/***** Comparison functions ("void *" are all "icd_list *") *****/

    int icd_list__cmp_name_order(void *, void *);
    int icd_list__cmp_name_reverse_order(void *, void *);

/***** Key functions *****/

    int icd_list__by_name(void *key, void *list);

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

