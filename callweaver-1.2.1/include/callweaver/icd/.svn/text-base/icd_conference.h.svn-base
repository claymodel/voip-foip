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
 
#if !defined(ICD_CONFERENCE_H)
#define ICD_CONFERENCE_H

#include ZAPTEL_H

struct icd_conference
{
    char name[ICD_STRING_LEN];
    char pin[ICD_STRING_LEN];
    int fd;
    int usecount;
    time_t start;
    icd_caller *owner;
    int is_agent_conf;
    struct zt_confinfo ztc;
    icd_memory *memory;
};

int icd_conference__set_global_usage(int value);
int icd_conference__get_global_usage(void);
icd_status icd_conference__clear(icd_caller * that);
icd_conference *icd_conference__new(char *name);
int icd_conference__usecount(icd_conference * conf);
icd_status icd_conference__associate(icd_caller * that, icd_conference * conf, int owner);
icd_status icd_conference__join(icd_caller * that);
icd_status icd_conference__register(char *name, icd_conference * conf);
icd_status icd_conference__deregister(char *name);
icd_conference *icd_conference__locate(char *name);
vh_keylist *icd_conference__list(void);
void icd_conference__init_registry(void);
void icd_conference__destroy_registry(void);
icd_caller *icd_conference__get_owner(icd_conference * conf);
void icd_conference__lock(icd_conference * conf, char *pin);
char *icd_conference__key(icd_conference * conf);

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

