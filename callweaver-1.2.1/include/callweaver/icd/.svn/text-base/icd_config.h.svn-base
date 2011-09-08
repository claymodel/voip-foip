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
 *  \brief  icd_config.h  -  a table of configuration parameters
 *
 * The icd_config module holds a set of parameters that can be used to
 * initialize any of the icd structures. Each module will have its own set
 * of key values that it can pull out of an icd_config object. In addition,
 * this module holds an icd_config_registry structure, which allows the
 * dynamic translation of a key setting into any type of void * value.
 *
 * Note that this makes no attempt to be typesafe, but that should be ok.
 * It is hard to imagine multiple threads wanting to alter the same
 * set of configuration parameters. If this assumption turns out to be
 * false, the code will need to be reworked.
 *
 */

#ifndef ICD_CONFIG_H
#define ICD_CONFIG_H
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/voidhash.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Init - Destroyer for icd_config *****/

    icd_config *create_icd_config(icd_config_registry * registry, char *name);
    icd_status destroy_icd_config(icd_config ** configp);
    icd_status init_icd_config(icd_config * that, icd_config_registry * registry, char *name);
    icd_status icd_config__clear(icd_config * that);

    void *icd_config__get_value(icd_config * that, char *key);
    icd_status icd_config__set_value(icd_config * that, char *key, char *setting);
    icd_status icd_config__set_raw(icd_config * that, char *key, void *data);
    icd_status icd_config__set_if_new(icd_config * that, char *key, char *setting);
    icd_status icd_config__parse(icd_config * that, char *line, char delim);
    void *icd_config__get_param(icd_config * that, char *name);
    char *icd_config__get_strdup(icd_config * that, char *key, char *default_str);
    icd_status icd_config__strncpy(icd_config * that, char *key, char *target, int maxchars);
    int icd_config__get_int_value(icd_config * that, char *key, int default_int);
    void *icd_config__get_any_value(icd_config * that, char *key, void *default_any);
    icd_config *icd_config__get_subset(icd_config * that, char *begin_key);
    icd_config_registry *icd_config__get_registry(icd_config * that, char *key);

    icd_config_iterator *icd_config__get_key_iterator(icd_config * that);
    int icd_config_iterator__has_more(icd_config_iterator * that);
    char *icd_config_iterator__next(icd_config_iterator * that);
    icd_status destroy_icd_config_iterator(icd_config_iterator ** that);

/***** Init - Destroyer for icd_config_registry *****/

    icd_config_registry *create_icd_config_registry(char *name);
    icd_status destroy_icd_config_registry(icd_config_registry ** regp);
    icd_status init_icd_config_registry(icd_config_registry * that, char *name);
    icd_status icd_config_registry__clear(icd_config_registry * that);

    icd_status icd_config_registry__register(icd_config_registry * that, char *key);
    icd_status icd_config_registry__register_ptr(icd_config_registry * that, char *key, char *keysetting,
        void *value);

    icd_status icd_config_registry__set_validate(icd_config_registry * that, int validate);
    int icd_config_registry__get_validate(icd_config_registry * that);

    icd_config_iterator *icd_config__get_registered_keys_iterator(icd_config_registry * that);

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

