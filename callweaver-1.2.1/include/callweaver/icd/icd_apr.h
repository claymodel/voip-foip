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

#ifndef ICD_APR_H
#define ICD_APR_H
#include <apr_strings.h>
#include <apr_pools.h>
#include "callweaver/icd/icd_types.h"

void *icd_apr__malloc(size_t size);
void *icd_apr__calloc(size_t size);
void *icd_apr__submalloc(apr_pool_t * pool, size_t size);
void *icd_apr__subcalloc(apr_pool_t * pool, size_t size);
void *icd_apr__free(void *obj);
void icd_apr__destroy(void);
icd_status icd_apr__init(void);
apr_pool_t *icd_apr__new_subpool(void);
void icd_apr__destroy_subpool(apr_pool_t * pool);
void icd_apr__clear_subpool(apr_pool_t * pool);
char *icd_apr__strdup(char *str);
char *icd_apr__substrdup(apr_pool_t * pool, char *str);
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

