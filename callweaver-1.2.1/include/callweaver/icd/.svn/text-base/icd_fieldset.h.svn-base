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
 *  \brief icd_fieldset.h  -  a set of extension fields
 *
 * The icd_fieldset module holds a set of parameters that can be used for any
 * purpose within an ICD module. Most of the structures in ICD hold a
 * icd_fieldset pointer.
 *
 * Note that this makes no attempt to be typesafe. That may prove to be
 * problem eventually, although it may just work out.
 *
 */

#ifndef ICD_FIELDSET_H
#define ICD_FIELDSET_H
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/voidhash.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Init - Destroyer for icd_fieldset *****/

    icd_fieldset *create_icd_fieldset(char *name);
    icd_status destroy_icd_fieldset(icd_fieldset ** fieldsetp);
    icd_status init_icd_fieldset(icd_fieldset * that, char *name);
    icd_status icd_fieldset__clear(icd_fieldset * that);

/***** Actions *****/

    void *icd_fieldset__get_value(icd_fieldset * that, char *key);
    icd_status icd_fieldset__set_value(icd_fieldset * that, char *key, void *setting);
    icd_status icd_fieldset__set_if_new(icd_fieldset * that, char *key, void *setting);
    char *icd_fieldset__get_strdup(icd_fieldset * that, char *key, char *default_str);
    icd_status icd_fieldset__strncpy(icd_fieldset * that, char *key, char *target, int maxchars);
    int icd_fieldset__get_int_value(icd_fieldset * that, char *key, int default_int);
    void *icd_fieldset__get_any_value(icd_fieldset * that, char *key, void *default_any);
    icd_fieldset *icd_fieldset__get_subset(icd_fieldset * that, char *begin_key);
    icd_status icd_fieldset__remove_key(icd_fieldset * that, char *key);
    icd_status icd_fieldset__remove_value(icd_fieldset * that, void *value);
    icd_status icd_fieldset__remove_all(icd_fieldset * that);
    icd_status icd_fieldset__parse(icd_fieldset * that, char *line, char delim);

    icd_fieldset_iterator *icd_fieldset__get_key_iterator(icd_fieldset * that);
    int icd_fieldset_iterator__has_more(icd_fieldset_iterator * that);
    char *icd_fieldset_iterator__next(icd_fieldset_iterator * that);
    icd_status destroy_icd_fieldset_iterator(icd_fieldset_iterator ** that);

/***** Shared helper functions *****/

    char *correct_null_str(char *str);

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

