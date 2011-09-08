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
 
 
#ifndef ICD_PLUGABLE_FN_H
#define ICD_PLUGABLE_FN_H

#include "callweaver/icd/icd_types.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct icd_plugable_fn {
        int id;                 /* Unique key for this function ptrs struc */
        char name[ICD_STRING_LEN];      /* A name to refer to this group of fn ptrs usually the dist strategy */

        /* run func , sadistic yes */
        void *(*run_fn) (void *ptr);

        /* Callback Setters */
        int (*auth_fn) (icd_caller *, void *);
        int (*authn_fn) (icd_event * that, void *extra);
        void *authn_fn_extra;

        /*  - Notification callbacks */
        int (*addq_fn) (icd_event * that, void *extra);
        void *addq_fn_extra;

        int (*delq_fn) (icd_event * that, void *extra);
        void *delq_fn_extra;

        int (*adddist_fn) (icd_event * that, void *extra);
        void *adddist_fn_extra;

        int (*deldist_fn) (icd_event * that, void *extra);
        void *deldist_fn_extra;

        int (*chan_fn) (icd_event * that, void *extra);
        void *chan_fn_extra;

        int (*link_fn) (icd_event * that, void *extra);
        void *link_fn_extra;

        int (*bridge_fn) (icd_event * that, void *extra);
        void *bridge_fn_extra;

        int (*state_fn) (icd_event * that, void *extra);
        void *state_fn_extra;

        /*  - State functions */
        int (*state_ready_fn) (icd_event * that, void *extra);
        void *state_ready_fn_extra;

        int (*state_distribute_fn) (icd_event * that, void *extra);
        void *state_distribute_fn_extra;

        int (*state_get_channels_fn) (icd_event * that, void *extra);
        void *state_get_channels_fn_extra;

        int (*state_conference_fn) (icd_event * that, void *extra);
        void *state_conference_fn_extra;

        int (*state_bridged_fn) (icd_event * that, void *extra);
        void *state_bridged_fn_extra;

        int (*state_bridge_failed_fn) (icd_event * that, void *extra);
        void *state_bridge_failed_fn_extra;

        int (*state_channel_failed_fn) (icd_event * that, void *extra);
        void *state_channel_failed_fn_extra;

        int (*state_associate_failed_fn) (icd_event * that, void *extra);
        void *state_associate_failed_fn_extra;

        int (*state_call_end_fn) (icd_event * that, void *extra);
        void *state_call_end_fn_extra;

        int (*state_suspend_fn) (icd_event * that, void *extra);
        void *state_suspend_fn_extra;

        /* Actions within state changes */
        int (*start_waiting_fn) (icd_caller * caller);
        int (*stop_waiting_fn) (icd_caller * caller);
          icd_status(*prepare_caller_fn) (icd_caller * caller);
          icd_status(*cleanup_caller_fn) (icd_caller * caller);
          icd_status(*launch_caller_fn) (icd_caller * caller);

        int allocated;          /* Is this structure on the heap? */
    };

    extern icd_fieldset *queues;

    icd_plugable_fn *create_icd_plugable_fns(icd_config * data, char *name);

/* Initialize a previously created plugable_fns */
    icd_status init_icd_plugable_fns(icd_plugable_fn * that, char *name, icd_config * data);

    void icd_plugable__create_standard_fns(icd_plugable_fn_list * that, icd_config * data);

    icd_status icd_plugable__clear_fns(icd_plugable_fn * that);

    icd_status destroy_icd_plugable_fn(icd_plugable_fn ** plugable_fnsp);

    char *icd_plugable__get_name(icd_plugable_fn * plugable_fns);

    icd_plugable_fn icd_plugable_get_fns(icd_caller * that);

/***** Callback Setters Predefined Pluggable Actions ****/
/* set get_plugable_fn in the caller, distributor or the mebership struc's */
    icd_status icd_plugable__set_fn(void *that, icd_plugable_fn * (*get_plugable_fn) (icd_caller *));

    icd_status icd_plugable__set_authenticate_fn(icd_plugable_fn * that, int (*auth_fn) (icd_caller *, void *));

/* Sets the added to queue notification hook to a particular function. */
    icd_status icd_plugable__set_added_to_queue_notify_fn(icd_plugable_fn * that, int (*addq_fn) (icd_event * that,
            void *extra), void *extra);

/* Sets the removed from queue notification hook to a particular function. */
    icd_status icd_plugable__set_removed_from_queue_notify_fn(icd_plugable_fn * that,
        int (*delq_fn) (icd_event * that, void *extra), void *extra);

/* Sets the added to distributor notification hook to a particular function. */
    icd_status icd_plugable__set_added_to_distributor_notify_fn(icd_plugable_fn * that,
        int (*adddist_fn) (icd_event * that, void *extra), void *extra);

/* Sets the removed from distributor notification hook to a particular function. */
    icd_status icd_plugable__set_removed_from_distributor_notify_fn(icd_plugable_fn * that,
        int (*deldist_fn) (icd_event * that, void *extra), void *extra);

/* Sets the call_state_change notification hook to a particular function. */
    icd_status icd_plugable__set_state_change_notify_fn(icd_plugable_fn * that, int (*state_fn) (icd_event * that,
            void *extra), void *extra);

/* Sets the channel assigned notification hook to a particular function. */
    icd_status icd_plugable__set_channel_assigned_notify_fn(icd_plugable_fn * that,
        int (*chan_fn) (icd_event * that, void *extra), void *extra);

/* Sets the linked to caller notification hook to a particular function. */
    icd_status icd_plugable__set_linked_notify_fn(icd_plugable_fn * that, int (*link_fn) (icd_event * that,
            void *extra), void *extra);

/* Sets the bridged notification hook to a particular function. */
    icd_status icd_plugable__set_bridged_notify_fn(icd_plugable_fn * that, int (*bridge_fn) (icd_event * that,
            void *extra), void *extra);

/* Sets the function to be called once authentication has successfully completed. */
    icd_status icd_plugable__set_authenticate_notify_fn(icd_plugable_fn * that, int (*authn_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_ready_fn(icd_plugable_fn * that, int (*ready_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_distribute_fn(icd_plugable_fn * that, int (*dist_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_get_channels_fn(icd_plugable_fn * that, int (*chan_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_bridged_fn(icd_plugable_fn * that, int (*bridged_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_conference_fn(icd_plugable_fn * that,
        int (*state_conference_fn) (icd_event * that, void *extra), void *extra);

    icd_status icd_plugable__set_state_call_end_fn(icd_plugable_fn * that, int (*end_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_bridge_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_channel_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_associate_failed_fn(icd_plugable_fn * that, int (*fail_fn) (icd_event * that,
            void *extra), void *extra);

    icd_status icd_plugable__set_state_suspend_fn(icd_plugable_fn * that, int (*suspend_fn) (icd_event * that,
            void *extra), void *extra);

/* Set up the pluggable actions from within the caller thread */
/*These are not available as plugables in the distributor since no active member is defined */

    icd_status icd_plugable__set_start_waiting_fn(icd_plugable_fn * that, int (*start_fn) (icd_caller * caller));
    icd_status icd_plugable__set_stop_waiting_fn(icd_plugable_fn * that, int (*stop_fn) (icd_caller * caller));

    icd_status icd_plugable__set_prepare_caller_fn(icd_plugable_fn * that,
        icd_status(*prepare_fn) (icd_caller * caller));

    icd_status icd_plugable__set_cleanup_caller_fn(icd_plugable_fn * that,
        icd_status(*cleanup_fn) (icd_caller * caller));

    icd_status icd_plugable__set_launch_caller_fn(icd_plugable_fn * that,
        icd_status(*launch_fn) (icd_caller * caller));

    icd_memory *memory;

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

