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
 * \brief icd_distributor_list.c - list of distributor strategies
 *
 * The icd_distributor_list module keeps a collection of distributors, and so
 * for simplicity it is an extension of icd_list. In addition, though, it knows
 * how to manipulate distributors. So you could keep code in the distributor
 * list that allows one distributor to steal an agent or customer from another.
 * It also has the ability to assign an agent or customer to a particular
 * distributor, although we will likely use other mechanisms to make this
 * connection most of the time.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include "callweaver/icd/icd_distributor_list.h"
#include "callweaver/icd/icd_types.h"

/***** Constructors and Destructors *****/

/* Constructor for a list object. */
icd_distributor_list *init_icd_distributor_list(int size)
{
    icd_distributor_list *list;

    return list;
}

/* Destructor for a list object. */
icd_status destroy_icd_distributor_list(icd_distributor_list ** listp)
{
    icd_status ret;

    return ret;
}

/***** Behaviours *****/

/* Add an element onto the list. */
icd_status icd_distributor_list__push(icd_distributor_list * that, icd_distributor * dist)
{
    icd_status ret;

    return ret;
}

/* Pop the top element off of the list. */
icd_distributor *icd_distributor_list__pop(icd_distributor_list * that)
{
    icd_distributor *dist;

    return dist;
}

/* Print our a copy of the list */
icd_status icd_distributor_list__dump(icd_distributor_list * that, int fd)
{
    icd_status ret;

    return ret;

}

/***** Locking *****/

/* Lock the list */
icd_status icd_distributor_list__lock(icd_distributor_list * that)
{
    icd_status ret;

    return ret;
}

/* Unlock the list */
icd_status icd_distributor_list__unlock(icd_distributor_list * that)
{
    icd_status ret;

    return ret;
}

/**** Listeners ****/

icd_status icd_distributor_list__add_listener(icd_distributor_list * that, void *listener,
    int (*lstn_fn) (icd_distributor_list * that, void *msg, void *extra), void *extra)
{
    icd_status ret;

    return ret;
}

icd_status icd_distributor_list__remove_listener(icd_distributor_list * that, void *listener)
{
    icd_status ret;

    return ret;
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

