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
 *  \brief icd_types.h  -   Defines the types available in the ICD package
 */

#ifndef ICD_TYPES_H
#define ICD_TYPES_H

/* FD for log */
#define ICD_STDOUT 1
#define ICD_STDERR 2

#define ICD_STRING_LEN 255

/* CallWeaver specific pieces */
typedef struct cw_channel cw_channel;

/* Infrastructure pieces */
typedef struct icd_listeners icd_listeners;
typedef struct icd_event_factory icd_event_factory;
typedef struct icd_event icd_event;
typedef struct icd_fieldset icd_fieldset;
typedef struct icd_fieldset_iterator icd_fieldset_iterator;
typedef struct icd_config icd_config;
typedef struct icd_config_registry icd_config_registry;
typedef struct icd_fieldset_iterator icd_config_iterator;
typedef struct icd_list icd_list;
typedef struct icd_list_node icd_list_node;
typedef struct icd_list_iterator icd_list_iterator;
typedef struct icd_metalist icd_metalist;

/* Call distribution specific pieces */
typedef struct icd_caller icd_caller;
typedef struct icd_caller_holdinfo icd_caller_holdinfo;
typedef struct icd_customer icd_customer;
typedef struct icd_agent icd_agent;
typedef struct icd_distributor icd_distributor;
typedef struct icd_customer_list icd_customer_list;
typedef struct icd_agent_list icd_agent_list;
typedef struct icd_distributor_list icd_distributor_list;
typedef struct icd_caller_list icd_caller_list;
typedef struct icd_queue icd_queue;
typedef struct icd_queue_holdannounce icd_queue_holdannounce;
typedef struct icd_queue_chimeinfo icd_queue_chimeinfo;
typedef struct icd_list_iterator icd_queue_iterator;
typedef struct icd_member icd_member;
typedef struct icd_member_list icd_member_list;
typedef struct icd_conference icd_conference;

/* caller / distributor plugable behavior pieces */
typedef struct icd_loadable_object icd_loadable_object;
typedef struct icd_plugable_fn_list icd_plugable_fn_list;
typedef struct icd_plugable_fn icd_plugable_fn;

typedef struct icd_caller_group_list icd_caller_group_list;
typedef struct icd_caller_group icd_caller_group;

/* ways to bridge calls */

typedef enum {
    ICD_BRIDGE_STANDARD,
    ICD_BRIDGE_CONFERENCE
} icd_bridge_technology;

/* ways to unbridge calls */
typedef enum {
    ICD_UNBRIDGE_HANGUP_ALL,
    ICD_UNBRIDGE_HANGUP_NONE,
    ICD_UNBRIDGE_HANGUP_AGENT,
    ICD_UNBRIDGE_HANGUP_CUSTOMER
} icd_unbridge_flag;

/* ICD Modules */
typedef enum {
    APP_ICD, ICD_QUEUE, ICD_DISTRIBUTOR, ICD_DISTRIBUTOR_LIST, ICD_CALLER,
    ICD_CALLER_LIST, ICD_AGENT, ICD_CUSTOMER, ICD_MEMBER, ICD_MEMBER_LIST,
    ICD_BRIDGE, ICD_CONFERENCE, ICD_LISTENERS, ICD_EVENT, ICD_FIELDSET,
    ICD_CONFIG, ICD_LIST, ICD_METALIST, ICD_PLUGABLE_FN, ICD_PLUGABLE_FN_LIST,
    ICD_MODULE_LAST_STANDARD
} icd_module;

#define ICD_MAX_MODULES 50

/* extern int  *icd_event_modules[]; these are now local icd_event.c */
extern char *icd_module_strings[];      /* see icd_event.c DONT forget to update when chg enum icd_module! */

/* Event types. are generic and used by icd objects and by icd states*/
typedef enum {
    ICD_EVENT_TEST, ICD_EVENT_CREATE, ICD_EVENT_INIT, ICD_EVENT_CLEAR,
    ICD_EVENT_DESTROY, ICD_EVENT_PUSH, ICD_EVENT_POP, ICD_EVENT_FIRE,
    ICD_EVENT_PUSHBACK, ICD_EVENT_STATECHANGE, ICD_EVENT_ADD,
    ICD_EVENT_REMOVE, ICD_EVENT_READY, ICD_EVENT_CHANNEL_UP,
    ICD_EVENT_GET_CHANNELS, ICD_EVENT_GOT_CHANNELS, ICD_EVENT_DISTRIBUTE, ICD_EVENT_DISTRIBUTED,
    ICD_EVENT_LINK, ICD_EVENT_UNLINK, ICD_EVENT_BRIDGE, ICD_EVENT_BRIDGED,
    ICD_EVENT_BRIDGE_FAILED, ICD_EVENT_CHANNEL_FAILED, ICD_EVENT_ASSOC_FAILED,
    ICD_EVENT_BRIDGE_END, ICD_EVENT_SUSPEND, ICD_EVENT_SETFIELD,
    ICD_EVENT_AUTHENTICATE, ICD_EVENT_ASSIGN, ICD_EVENT_CONFERENCE, ICD_EVENT_LAST_STANDARD
} icd_event_type;

#define ICD_MAX_EVENTS 100

/*extern int  *icd_event_ids[]; these are now local icd_event.c*/
extern char *icd_event_strings[];       /* see icd_event.c DONT forget to update when chg enum icd_event_type!! */

/* Return codes from functions */
typedef enum {
    ICD_SUCCESS, ICD_EGENERAL, ICD_ELOCK, ICD_ESTATE,
    ICD_EVETO, ICD_ERESOURCE, ICD_ENOTFOUND, ICD_EEXISTS
} icd_status;

/* States that an icd_caller can be in */
typedef enum {
    ICD_CALLER_STATE_CREATED, ICD_CALLER_STATE_INITIALIZED,
    ICD_CALLER_STATE_CLEARED, ICD_CALLER_STATE_DESTROYED,
    ICD_CALLER_STATE_READY, ICD_CALLER_STATE_DISTRIBUTING,
    ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE, ICD_CALLER_STATE_BRIDGED,
    ICD_CALLER_STATE_BRIDGE_FAILED, ICD_CALLER_STATE_CHANNEL_FAILED,
    ICD_CALLER_STATE_ASSOCIATE_FAILED, ICD_CALLER_STATE_CALL_END,
    ICD_CALLER_STATE_SUSPEND, ICD_CALLER_STATE_CONFERENCED
} icd_caller_state;

extern char *icd_caller_state_strings[];        /* Kept in icd_caller.c */

/* Status of bridging */
typedef enum { BRIDGE_UP, BRIDGE_CLEANUP, BRIDGE_DOWN } icd_bridge_status;

extern char *icd_bridge_status_strings[];       /* Kept in icd_caller.c */

/* Categories of lists that are available */
typedef enum {
    ICD_LIST_CAT_UNKNOWN, ICD_LIST_CAT_CALLER, ICD_LIST_CAT_MEMBER,
    ICD_LIST_CAT_DISTRIBUTOR, ICD_LIST_CAT_QUEUE, ICD_LIST_CAT_LAST_STANDARD
} icd_list_category;

/* Flags that a caller can have */
typedef enum {
    ICD_NOHANGUP_FLAG = 1,
    ICD_MONITOR_FLAG = 2,
    ICD_CONF_MEMBER_FLAG = 4,
    ICD_ORPHAN_FLAG = 8,
    ICD_ENTLOCK_FLAG = 16,
    ICD_ACK_EXTERN_FLAG = 32
} icd_flag;

/* Roles that a caller can have */
typedef enum {
    ICD_AGENT_ROLE = 1,
    ICD_CUSTOMER_ROLE = 2,
    ICD_BRIDGER_ROLE = 4,
    ICD_BRIDGEE_ROLE = 8,
    ICD_LOOPER_ROLE = 16,
    ICD_CLONER_ROLE = 32,
    ICD_CLONE_ROLE = 64,
    ICD_INVALID_ROLE = 128
} icd_role;

/* Ways to entertain a caller 1-moh, 2-ringing */
typedef enum {
    ICD_ENTERTAIN_NONE, ICD_ENTERTAIN_MOH, ICD_ENTERTAIN_RING
} icd_entertain;

/* States that a thread can be in */
typedef enum {
    ICD_THREAD_STATE_UNINITIALIZED, ICD_THREAD_STATE_PAUSED,
    ICD_THREAD_STATE_RUNNING, ICD_THREAD_STATE_FINISHED
} icd_thread_state;

/*plugable interface points */
typedef enum {
    add_queue_notify, remove_queue_notify, add_dist_notify, remove_dist_notify, change_state_notify,
    assign_channel_notify, link_caller_notify, bridge_caller_notify, icd_caller_dummy_notify_hook,
    state_ready_function, state_distribute_function, state_get_channels_function,
    state_bridged_function, state_bridge_failed_function, state_channel_failed_function,
    state_associate_failed_function, state_call_end_function, state_suspend_function,
    state_conference_function, prepare_caller_function, cleanup_caller_function,
    launch_caller_function, add_queue_extra, remove_queue_extra,
    add_dist_extra, remove_dist_extra, change_state_extra,
    assign_channel_extra, link_caller_extra, bridge_caller_extra,
    authenticate_notify_extra, state_ready_extra, state_distribute_extra, state_get_channels_extra,
    state_bridged_extra, state_bridge_failed_extra, state_channel_failed_extra,
    state_associate_failed_extra, state_call_end_extra, state_suspend_extra
} icd_plugable_fn_names;

#define ICD_PLUGABLE_STANDARD icd_plugable_fn

extern char *icd_thread_state_strings[];        /* Kept in icd_caller.c */

extern char *cw_state_strings[];       /* Kept in icd_caller.c */

/* Generic list of void pointers to stuff several things into 1 obj */

struct icd_voidlist {
    void *a;
    void *b;
    void *c;
    void *d;
};

typedef struct icd_voidlist icd_voidlist;

#ifdef USE_APR_GLOBAL           /* unwise */
#include <icd_apr.h>
#define ICD_STD_MALLOC(bytes) icd_apr__malloc(bytes)
#define ICD_STD_FREE(obj) icd_apr__free(obj)
#else
#define ICD_STD_MALLOC(bytes) malloc(bytes)
#define ICD_STD_FREE(obj) free(obj)
#endif

#ifdef USE_APR
#include <icd_apr.h>
typedef apr_pool_t icd_memory;

#define ICD_MALLOC(obj,size) {\
 apr_pool_t *pool;\
 pool=icd_apr__new_subpool();\
 obj = icd_apr__submalloc(pool,size);\
 if(obj) {\
   memset (obj, 0, size);\
   obj->memory = pool;\
  }\
}

#define ICD_FREE(obj) {\
 apr_pool_t *pool = obj->memory;\
 icd_apr__destroy_subpool(pool);\
 obj = NULL;\
}

#define ICD_NEW_POOL(name) icd_memory *name = icd_apr__new_subpool();
#define ICD_POOL_MALLOC(name,size) icd_apr__submalloc(name,size)
#define ICD_POOL_FREE(name) {}
#define ICD_POOL_CLEAR(name) {icd_apr__clear_subpool(name);}
#define ICD_POOL_DESTROY(name) {icd_apr__destroy_subpool(name);}
#define ICD_MEMSET_ZERO(name,size) memset(name,0,size)
#define ICD_INNER_MALLOC(name,bytes) icd_apr__submalloc(name->memory,size)
#define ICD_INNER_FREE(name) {}

#define ICD_INIT     icd_apr__init()
#define ICD_UNINIT   icd_apr__destroy()

#else
typedef void icd_memory;

#define ICD_MALLOC(obj,size) {\
 obj = malloc(size);\
 memset (obj, 0, size);\
}
#define ICD_FREE(obj) {\
  free(obj);\
}
#define ICD_INIT                /*  */
#define ICD_UNINIT              /*  */

#define ICD_NEW_POOL(name) void *name;
#define ICD_POOL_MALLOC(name,size) name = malloc(size);
#define ICD_POOL_FREE(name) free(name)
#define ICD_POOL_DESTROY(name) {}
#define ICD_POOL_CLEAR(name) {}
#define ICD_MEMSET_ZERO(name,size) memset(name,0,size)
#define ICD_INNER_MALLOC(name,bytes) malloc(size)
#define ICD_INNER_FREE(name) free(name)
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

