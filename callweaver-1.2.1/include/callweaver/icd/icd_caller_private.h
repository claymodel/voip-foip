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
 *  \brief  icd_caller_private.h  -  a call to distribute
 *
 * ICD_CALLER Private Parts
 *
 * If you are not directly inheriting from icd_caller, you should pay no
 * attention to the man behind the curtain. Nothing to see here. Return
 * to your homes.
 *
 * Still here? That must mean you need details about the internals of the
 * icd_caller structure. Be careful, as you will now have to keep up with
 * any changes to the internal structure. You've been warned.
 *
 *
 */

#ifndef ICD_CALLER_PRIVATE_H
#define ICD_CALLER_PRIVATE_H

#include "callweaver/lock.h"
#include "callweaver/channel.h"
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_conference.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_plugable_fn.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct icd_caller_holdinfo {
        int chimepos;
        time_t chimenext;
        int last_announce_position;
        time_t last_announce_time;
    };

    struct icd_caller {
        int id;                 /* Unique key for caller */
        char *name;             /* A name to refer to this caller by in dump() */
        struct cw_channel *chan;       /* The channel as provided by the PBX */
        void *owner;            /* Any structure representing the owner of a call */
        void *authorization;    /* Some sort of authorization token */
        void *authentication;   /* Some sort of authentication token. */
        icd_member_list *memberships;   /* The Member List of distributors you belong to */
        icd_caller_list *associations;  /* links agent to cust for bridge */
        icd_caller_group_list *group_list;
        int using_caller_thread;        /* nonzero when we are going to run in our own thread */
        int owns_channel;
        int require_pushback;   /* nonzero when we need to be pushed back onto a distributor */
        icd_member *active_member;      /* The member that identifies the active distributor */
        int onhook;             /* Whether caller follows onhook rules or offhook rules */
        int dynamic;            /* Is caller static in icd_agents.conf or created dynamically */
        icd_caller_state state; /* The state of the caller see icd_caller_state in icd_types.h */
        time_t caller_created;  /* Time this caller instance was created */
        time_t last_mod;        /* Time of the last alteration on this structure */
        time_t last_state_change;       /* time the state changed */
        time_t start_call;      /* Time that a call started, or 0 for no call yet. */
        int timeout;            /* How long in sec's to wait for an answer */
        int callcount;          /* How many calls has this caller handled in total? */
        int authenticated;      /* Are we authenticated? */
        int channel_fail_count;
        int bridge_fail_count;
        icd_queue_holdannounce *holdannounce;
        icd_caller_holdinfo holdinfo;
        /* TBD - convert to icd_fieldset
           icd_fieldset *params;
         */
        void_hash_table *params;
        /* saved in config */
        icd_conference *conference;
        icd_bridge_technology bridge_technology;
        int conf_fd;
        int conf_mode;
        int acknowledge_call;   /* does agent caller have to enter dtmf b4 we bridge to caller */
        int acknowledged;       /* if 'acknowledgecall' is it acknowledged for the current call */
        int priority;           /* sort flds __set_priority */
        int role;               /* bit mask Agent or Customer, Bridger or Bridgee */
        int flag;
        int allocated;          /* Is this structure on the heap? */
        icd_entertain entertained;      /* Type of entertainment NONE, MOH, or RING */
        char *chan_string;
        char *caller_id;
        /*func to find the struct from the List or what ever... of plugable funcs */
        icd_plugable_fn *(*get_plugable_fn) (icd_caller * caller);
        icd_plugable_fn_list *plugable_fns_list;        /*The List of plugable functions for each dist */

        icd_listeners *listeners;
          icd_status(*dump_fn) (icd_caller * caller, int verbosity, int fd, void *extra);
        void *dump_fn_extra;

        /* Threading and locking */
        cw_mutex_t lock;
        pthread_t thread;
        cw_cond_t wakeup;
        icd_thread_state thread_state;
    };

/* Methods available for use of subtypes */

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

