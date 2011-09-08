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
 * \brief icd_metalist.c - a list of lists
 * 
 * The icd_metalist module provides a list of lists. Each element stored in
 * this list is itself a list. It allows access to the contained lists by
 * name as well as through the mechanisms of icd_list. It also has its own
 * typesafe iterator that returns icd_list structures.
 *
 * The icd_metalist is castable to an icd_list (since it is an extension
 * of it) so the API of icd_list is usable on icd_metalist. See that
 * section for more of the actions that are possible with icd_metalist
 * via a cast.
 *
 * Because an icd_metalist is itself an icd_list (which is what icd_metalist
 * contains), you could create arbitrarily deep trees by keeping icd_metalists
 * in another icd_metalist.
 *
 * Internally, icd_metalist sets its insertion point function to icd_list's
 * icd_list__insert_ordered() function, passing the icd_list__cmp_name_order()
 * comparison function to it to use to determine the sort order. This shows an
 * example of how you can keep list elements in an arbitrary sort order.
 *
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_metalist.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_list_private.h"

/*===== Private types and APIs =====*/

struct icd_metalist {
    icd_list list;
    icd_memory *memory;
    int allocated;
};

/***** Constructors and Destructors *****/

/* Constructor for a list of lists. */
icd_metalist *create_icd_metalist(icd_config * data)
{
    icd_metalist *list;
    icd_status result;

    assert(data != NULL);
    ICD_MALLOC(list, sizeof(icd_metalist));

    if (list == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Metalist\n");
        return NULL;
    }
    result = init_icd_metalist(list, data);
    if (result != ICD_SUCCESS) {
        ICD_FREE(list);
        return NULL;
    }
    list->allocated = 1;
    return list;
}

/* Destructor for a list of lists. */
icd_status destroy_icd_metalist(icd_metalist ** listp)
{
    icd_status clear_result;

    assert(listp != NULL);
    assert((*listp) != NULL);

    clear_result = icd_metalist__clear(*listp);

    if ((clear_result = icd_metalist__clear(*listp)) != ICD_SUCCESS) {
        return clear_result;
    }

    if ((*listp)->allocated) {
        ((icd_list *) (*listp))->state = ICD_LIST_STATE_DESTROYED;
        ICD_FREE((*listp));
        *listp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize previously created metalist */
icd_status init_icd_metalist(icd_metalist * that, icd_config * data)
{
    icd_status retval;
    icd_list_node *(*ins_fn) (icd_list * that, void *new_elem, void *extra);
    void *extra;

    assert(that != NULL);
    assert(data != NULL);
    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_metalist));

    retval = init_icd_list((icd_list *) that, data);
    if (retval == ICD_SUCCESS) {
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
            extra = icd_list__cmp_name_order;
        }
        retval = icd_list__set_node_insert_func((icd_list *) that, ins_fn, extra);
    }
    return retval;
}

/* Clear the metalist so that it needs to be reinitialized to be reused. */
icd_status icd_metalist__clear(icd_metalist * that)
{
    assert(that != NULL);

    return icd_list__clear((icd_list *) that);
}

/* Adds a list to the metalist, returns success or failure. */
icd_status icd_metalist__add_list(icd_metalist * that, icd_list * new_list)
{
    icd_status retval;

    assert(that != NULL);

    retval = icd_list__push((icd_list *) that, new_list);
    return retval;
}

/* Retrieves a list from the metalist when given a name. */
icd_list *icd_metalist__fetch_list(icd_metalist * that, char *name)
{
    assert(that != NULL);

    return icd_list__fetch((icd_list *) that, name, icd_list__by_name);
}

/* Removes a list from the metalist when given a name, returns success or failure. */
icd_status icd_metalist__remove_list(icd_metalist * that, char *name)
{
    assert(that != NULL);

    return icd_list__drop_node((icd_list *) that, name, icd_list__by_name);
}

/* Removes a list from the metalist when given the object itself, returns success or failure. */
icd_status icd_metalist__remove_list_by_element(icd_metalist * that, icd_list * target)
{
    assert(that != NULL);

    return icd_list__remove_by_element((icd_list *) that, target);
}

/* Prints the contents of the metalist to the given file descriptor. */
icd_status icd_metalist__dump(icd_metalist * that, int fd)
{
    icd_status ret = ICD_SUCCESS;

    assert(that != NULL);
    return ret;

}

/**** Iterator functions ****/

/* Returns the next element from the list. */
icd_list *icd_metalist_iterator__next(icd_list_iterator * that)
{
    assert(that != NULL);

    return (icd_list *) icd_list_iterator__next(that);
}

/*===== Private Implementations  =====*/

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

