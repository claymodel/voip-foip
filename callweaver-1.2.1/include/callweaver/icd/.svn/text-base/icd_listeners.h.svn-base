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
 *  \brief icd_listeners.h  -  collection of listeners
 *
 * The icd_listeners module provides a collection of listeners. Each module
 * in ICD provides an interface for adding listeners to each instance. As
 * events occur on the instance, it sends the events through its collection
 * of listeners, which have the opportunity to veto the event if desired.
 */

#ifndef ICD_LISTENERS_H
#define ICD_LISTENERS_H

#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constructors and Destructors *****/

/* Constructor for a listeners object. Parent is the object generating
   events, the one that will hold this collection of listeners. */
    icd_listeners *create_icd_listeners(void);

/* Remove the listener and clear its pointer. */
    icd_status destroy_icd_listeners(icd_listeners ** listenersp);

/* Initialize the icd_listeners structure. */
    icd_status init_icd_listeners(icd_listeners * that);

/* Clear out the icd_listeners structure. Reinitialize before using again. */
    icd_status icd_listeners__clear(icd_listeners * that);

/***** Actions *****/

/* Add a listener to the collection. */
    icd_status icd_listeners__add(icd_listeners * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

/* Remove the listener from this collection. */
    icd_status icd_listeners__remove(icd_listeners * that, void *listener);

/* Notify all listeners that an event has occured. */
    int icd_listeners__notify(icd_listeners * that, icd_event * event);

/* Print the contents of the listener collection. */
    icd_status icd_listeners__dump(icd_listeners * that, int fd);

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

