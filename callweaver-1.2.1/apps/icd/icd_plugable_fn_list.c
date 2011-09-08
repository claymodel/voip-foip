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
 *  \brief icd_plugable_fn_list.c - a list of lists
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_plugable_fn.h"
#include "callweaver/icd/icd_plugable_fn_list.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_list_private.h"

static icd_module module_id = ICD_PLUGABLE_FN_LIST;

/*===== Private types and APIs =====*/

struct icd_plugable_fn_list {
    icd_list list;
    icd_memory *memory;
    int allocated;
};

static int icd_plugable_fn_list__identify_name(void *key, void *payload);

/***** Constructors and Destructors *****/

/* Constructor for a list of function ptrs. */
icd_plugable_fn_list *create_icd_plugable_fn_list(char *name, icd_config * data)
{
    icd_plugable_fn_list *list;
    icd_status result;

    assert(data != NULL);
    ICD_MALLOC(list, sizeof(icd_plugable_fn_list));

    if (list == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD plugable_fn_list \n");
        return NULL;
    }

    result = init_icd_plugable_fn_list(list, name, data);
    list->allocated = 1;

    if (result != ICD_SUCCESS) {
        ICD_FREE(list);
        return NULL;
    }

    /* This can't use the event macros because we have no "that" */
    result =
        icd_event_factory__generate(event_factory, list, ((icd_list *) list)->name, module_id, ICD_EVENT_CREATE,
        NULL, ((icd_list *) list)->listeners, NULL);
    if (result == ICD_EVETO) {
        destroy_icd_plugable_fn_list(&list);
        return NULL;
    }

    return list;
}

/* Destructor for a list of function ptrs. */
icd_status destroy_icd_plugable_fn_list(icd_plugable_fn_list ** listp)
{
    icd_status clear_result;

    assert(listp != NULL);
    assert((*listp) != NULL);

    clear_result = icd_plugable_fn_list__clear(*listp);

    if ((clear_result = icd_plugable_fn_list__clear(*listp)) != ICD_SUCCESS) {
        return clear_result;
    }

    if ((*listp)->allocated) {
        ((icd_list *) (*listp))->state = ICD_LIST_STATE_DESTROYED;
        ICD_FREE((*listp));
        *listp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize previously created list of function ptrs */
icd_status init_icd_plugable_fn_list(icd_plugable_fn_list * that, char *name, icd_config * data)
{
    icd_status retval;
    icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
    void *extra;

    assert(that != NULL);
    assert(data != NULL);
    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_plugable_fn_list));

    retval = init_icd_list((icd_list *) that, data);

    if (retval == ICD_SUCCESS) {
        icd_list__set_name((icd_list *) that, name);
        ins_fn = icd_list__insert_ordered;
        extra = icd_list__insert_fifo;
        retval = icd_list__set_node_insert_func((icd_list *) that, ins_fn, extra);
        icd_list__set_key_check_func((icd_list *) that, icd_plugable_fn_list__identify_name);
    }
    return retval;
}

/* Clear the list so that it needs to be reinitialized to be reused. */
icd_status icd_plugable_fn_list__clear(icd_plugable_fn_list * that)
{
    assert(that != NULL);

    return icd_list__clear((icd_list *) that);
}

/* Drops All Function pointers in the List */
icd_status icd_plugable_fn_remove_all_plugable_fns(icd_plugable_fn_list * that)
{
    icd_list_iterator *iter;
    icd_list *next;
    icd_plugable_fn *plugable_fns;

    //icd_status result;
    icd_status final_result;

    assert(that != NULL);

    final_result = ICD_SUCCESS;
    /* returns the list payload     
       plugable_fns = icd_list__peek((icd_list *)that);
       while (plugable_fns != NULL) {
       icd_plugable__clear_fns(&plugable_fns);
       icd_plugable_fn_list__remove_fns_by_element(that, );
       plugable_fns = icd_list__peek((icd_list *)that);
       }
     */
    iter = icd_list__get_iterator((icd_list *) (that));
    if (iter != NULL) {
        while (icd_list_iterator__has_more(iter)) {
            next = icd_list_iterator__next(iter);
            plugable_fns = (icd_plugable_fn *) next;
            icd_plugable__clear_fns(plugable_fns);
            destroy_icd_plugable_fn(&plugable_fns);
            icd_plugable_fn_list__remove_fns_by_element(that, next);
        }
        destroy_icd_list_iterator(&iter);
    }

    return final_result;
}

/* Adds a function ptrs to the list, returns success or failure. */
icd_status icd_plugable_fn_list__add_fns(icd_plugable_fn_list * that, icd_plugable_fn * new_fns)
{
    icd_status retval;

    assert(that != NULL);

    retval = icd_list__push((icd_list *) that, new_fns);
    return retval;
}

/* Retrieves a function ptr from the list when given a name. */
icd_plugable_fn *icd_plugable_fn_list__fetch_fns(icd_plugable_fn_list * that, char *name)
{
    assert(that != NULL);

    return (icd_plugable_fn *) icd_list__fetch((icd_list *) that, name, icd_plugable_fn_list__identify_name);
}

/* Removes a plugable function from the list when given a name, returns success or failure. */
icd_status icd_plugable_fn_list__remove_fns(icd_plugable_fn_list * that, char *id)
{
    assert(that != NULL);
    assert(id != NULL);

    return icd_list__remove((icd_list *) that, id);
}

/* Removes a list from the list when given the object itself, returns success or failure. */
icd_status icd_plugable_fn_list__remove_fns_by_element(icd_plugable_fn_list * that, icd_list * target)
{
    assert(that != NULL);

    return icd_list__remove_by_element((icd_list *) that, target);
}

/* Prints the contents of the list to the given file descriptor. */
icd_status icd_plugable_fn_list__dump(icd_plugable_fn_list * that, int fd)
{
    icd_status ret = ICD_SUCCESS;

    assert(that != NULL);
    return ret;

}

/* Return the number of plugable function sets is ok to have none  */
int icd_plugable_fn_list_count(icd_plugable_fn_list * that)
{

    return that ? icd_list__count((icd_list *) (that)) : 0;
}

/**** Iterator functions ****/

/* Returns the next element from the list. */
icd_plugable_fn *icd_plugable_fn_list_iterator__next(icd_list_iterator * that)
{
    assert(that != NULL);

    return (icd_plugable_fn *) icd_list_iterator__next(that);
}

/*===== Private Implementations  =====*/
static int icd_plugable_fn_list__identify_name(void *key, void *payload)
{
    icd_plugable_fn *plugable_fns;
    char *name;

    plugable_fns = (icd_plugable_fn *) payload;
    name = icd_plugable__get_name(plugable_fns);
    /*
       cw_log(LOG_NOTICE, "Find Funcs By Name/Key[%s, %s], readyfunc=%p\n",
       name,(char *)key,  plugable_fns->state_ready_fn );
     */
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

