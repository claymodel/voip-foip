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
 *  \brief icd_queue.h  -  a collection of cunstomers and agents
 *
 * The icd_queue module holds caller memberships, as well as a distributor.
 *
 * Each queue holds two collections of memberships, one for agents and one
 * for customers. It is responsible for putting the callers into the
 * distributor it holds at the appropriate time. Whereas the distributor
 * holds a caller membership only when the caller is ready to be distributed,
 * the queue holds it both before and after distribution.
 *
 */

#ifndef ICD_QUEUE_H
#define ICD_QUEUE_H

#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_member.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SOUND_FILE_SIZE 255

    struct icd_queue_holdannounce {
        int cycle;              /* which cycle to announce holdtime: 0=never, -1=every time, 1=only once */
        int frequency;          /* How often to announce their position if cycle is every time */
        int holdtime;           /* Current avg holdtime for this queue, based on recursive boxcar filter */
        char sound_next[SOUND_FILE_SIZE];       /* "Your call is now first in line" (def. hold.youarenext) */
        char sound_thereare[SOUND_FILE_SIZE];   /* "There are currently" (def. hold.thereare) */
        char sound_calls[SOUND_FILE_SIZE];      /* "calls waiting to speak to a representative." (def. hold.callswaiting) */
        char sound_holdtime[SOUND_FILE_SIZE];   /* "The current estimated total holdtime is" (def. hold.holdtime) */
        char sound_minutes[SOUND_FILE_SIZE];    /* "minutes." (def. queue-minutes) */
        char sound_thanks[SOUND_FILE_SIZE];     /* "Thank you for your patience." (def. hold.thanks) */
        int allocated;
    };

    struct icd_queue_chimeinfo {
        char chimedata[ICD_STRING_LEN];
        char *chimelist[ICD_STRING_LEN];
        int chimepos;
        int chimelistlen;
        int chime_freq;
        int chime_repeat_to;
        time_t chimenext;
    };

/* alloc/free/init */
    icd_queue *create_icd_queue(char *name, icd_config * config);
    icd_status destroy_icd_queue(icd_queue ** queuep);
    icd_status init_icd_queue(icd_queue * that, char *name, icd_config * config);
    icd_status icd_queue__clear(icd_queue * that);

/* operators */
    icd_status icd_queue__start_distributing(icd_queue * that);
    icd_status icd_queue__pause_distributing(icd_queue * that);
    icd_status icd_queue__stop_distributing(icd_queue * that);
    icd_status icd_queue__customer_join(icd_queue * that, icd_member * caller);
    icd_status icd_queue__customer_quit(icd_queue * that, icd_member * caller);
    icd_status icd_queue__customer_distribute(icd_queue * that, icd_member * caller);
    icd_status icd_queue__customer_pushback(icd_queue * that, icd_member * caller);
    icd_status icd_queue__agent_join(icd_queue * that, icd_member * caller);
    icd_status icd_queue__agent_quit(icd_queue * that, icd_member * caller);
    icd_status icd_queue__agent_distribute(icd_queue * that, icd_member * caller);
    icd_status icd_queue__agent_pushback(icd_queue * that, icd_member * caller);
    icd_status icd_queue__agent_dist_quit(icd_queue * that, icd_member * member);
	icd_status icd_queue__customer_dist_quit(icd_queue * that, icd_member * member);

/***** Getters and Setters *****/
    icd_status icd_queue__set_name(icd_queue * that, char *name);
    char *icd_queue__get_name(icd_queue * that);
    char *icd_queue__get_monitor_args(icd_queue * that);
    char * icd_queue__check_recording(icd_queue *that, icd_caller *caller);
    int icd_queue__get_agent_count(icd_queue * that);
    int icd_queue__get_customer_count(icd_queue * that);
    int icd_queue__get_agent_position(icd_queue * that, icd_agent * target);
    int icd_queue__get_customer_position(icd_queue * that, icd_customer * target);
    icd_status icd_queue__set_distributor(icd_queue * that, icd_distributor * dist);
    icd_distributor *icd_queue__get_distributor(icd_queue * that);
    icd_member_list *icd_queue__get_customers(icd_queue * that);
    icd_member_list *icd_queue__get_agents(icd_queue * that);

    icd_queue_holdannounce *icd_queue__get_holdannounce(icd_queue * that);
    int icd_queue__get_holdannounce_cycle(icd_queue * that);
    int icd_queue__get_holdannounce_frequency(icd_queue * that);
    int icd_queue__get_holdannounce_holdtime(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_next(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_thereare(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_calls(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_holdtime(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_minutes(icd_queue * that);
    char *icd_queue__get_holdannounce_sound_thanks(icd_queue * that);

    int icd_queue__get_wait_timeout(icd_queue * that);

/*Count of active agents - states other that INITIALIZED and SUSPEND  */ 
    int icd_queue__agent_active_count(icd_queue * that);


/* Print out a debug dump of the queue.*/
    icd_status icd_queue__dump(icd_queue * that, int verbosity, int fd);
    icd_status icd_queue__standard_dump(icd_queue * that, int verbosity, int fd, void *extra);

/* Print out key Info on Queues for cli UI */
    icd_status icd_queue__show(icd_queue * that, int verbosity, int fd);

/***** Locking *****/

/* Lock the entire queue */
    icd_status icd_queue__lock(icd_queue * that);

/* Unlock the entire queue */
    icd_status icd_queue__unlock(icd_queue * that);

/**** Listeners ****/

    icd_status icd_queue__add_listener(icd_queue * that, void *listener, int (*lstn_fn) (void *listener,
            icd_event * event, void *extra), void *extra);

    icd_status icd_queue__remove_listener(icd_queue * that, void *listener);

/****** Iterators for customers and agents in queue *****/

/* Returns an iterator of customer callers in the queue. */
    icd_list_iterator *icd_queue__get_customer_iterator(icd_queue * that);

/* Returns an iterator of agent callers in the queue. */
    icd_list_iterator *icd_queue__get_agent_iterator(icd_queue * that);

/* Queue Iterator destructor. */
    icd_status destroy_icd_queue_iterator(icd_queue_iterator ** iterp);

/* Indicates whether there are more callers in the queue. */
    int icd_queue_iterator__has_more(icd_queue_iterator * that);

/* Returns the next caller from the queue. */
    icd_caller *icd_queue_iterator__next(icd_queue_iterator * that);

    char *icd_queue__chime(icd_queue * queue, icd_caller * caller);
    int icd_queue__set_chime_freq(icd_queue * that, int freq);
    int icd_queue__get_chime_freq(icd_queue * that);
    int icd_queue__set_holdannounce_holdtime(icd_queue * that, int time);
    icd_status icd_queue__calc_holdtime(icd_queue * that);
    icd_status icd_queue__set_dump_func(icd_queue * that, icd_status(*dump_fn) (icd_queue *, int verbosity, int fd,
            void *extra), void *extra);

typedef enum {
    ICD_QUEUE_STATE_CREATED, ICD_QUEUE_STATE_INITIALIZED,
    ICD_QUEUE_STATE_CLEARED, ICD_QUEUE_STATE_DESTROYED,
    ICD_QUEUE_STATE_LAST_STANDARD
} icd_queue_state;



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

