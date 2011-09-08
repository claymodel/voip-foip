/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
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
 * \brief Call Detail Record API 
 */

#ifndef _CALLWEAVER_CDR_H
#define _CALLWEAVER_CDR_H

#include <sys/time.h>
#define CW_CDR_FLAG_KEEP_VARS			(1 << 0)
#define CW_CDR_FLAG_POSTED			(1 << 1)
#define CW_CDR_FLAG_LOCKED			(1 << 2)
#define CW_CDR_FLAG_CHILD			(1 << 3)
#define CW_CDR_FLAG_POST_DISABLED		(1 << 4)

#define CW_CDR_NOANSWER			(1 << 0)
#define CW_CDR_BUSY				(1 << 1)
#define CW_CDR_ANSWERED			(1 << 2)
#define CW_CDR_FAILED				(1 << 3)

/*! AMA Flags */
#define CW_CDR_OMIT				(1)
#define CW_CDR_BILLING				(2)
#define CW_CDR_DOCUMENTATION			(3)

#define CW_MAX_USER_FIELD			256
#define CW_MAX_ACCOUNT_CODE			20

/* Include channel.h after relevant declarations it will need */
#include "callweaver/channel.h"

struct cw_channel;

/*! Responsible for call detail data */
struct cw_cdr {
	/*! Caller*ID with text */
	char clid[CW_MAX_EXTENSION];
	/*! Caller*ID number */
	char src[CW_MAX_EXTENSION];		
	/*! Destination extension */
	char dst[CW_MAX_EXTENSION];		
	/*! Destination context */
	char dcontext[CW_MAX_EXTENSION];	
	
	char channel[CW_MAX_EXTENSION];
	/*! Destination channel if appropriate */
	char dstchannel[CW_MAX_EXTENSION];	
	/*! Last application if appropriate */
	char lastapp[CW_MAX_EXTENSION];	
	/*! Last application data */
	char lastdata[CW_MAX_EXTENSION];	
	
	struct timeval start;
	
	struct timeval answer;
	
	struct timeval end;
	/*! Total time in system, in seconds */
	int duration;				
	/*! Total time call is up, in seconds */
	int billsec;				
	/*! What happened to the call */
	int disposition;			
	/*! What flags to use */
	int amaflags;				
	/*! What account number to use */
	char accountcode[CW_MAX_ACCOUNT_CODE];			
	/*! flags */
	unsigned int flags;				
	/* Unique Channel Identifier */
	char uniqueid[32];
	/* User field */
	char userfield[CW_MAX_USER_FIELD];

	/* A linked list for variables */
	struct varshead varshead;

	struct cw_cdr *next;
};

extern void cw_cdr_getvar(struct cw_cdr *cdr, const char *name, char **ret, char *workspace, int workspacelen, int recur);
extern int cw_cdr_setvar(struct cw_cdr *cdr, const char *name, const char *value, int recur);
extern int cw_cdr_serialize_variables(struct cw_cdr *cdr, char *buf, size_t size, char delim, char sep, int recur);
extern void cw_cdr_free_vars(struct cw_cdr *cdr, int recur);
extern int cw_cdr_copy_vars(struct cw_cdr *to_cdr, struct cw_cdr *from_cdr);

typedef int (*cw_cdrbe)(struct cw_cdr *cdr);

/*! \brief Allocate a CDR record 
 * Returns a malloc'd cw_cdr structure, returns NULL on error (malloc failure)
 */
extern struct cw_cdr *cw_cdr_alloc(void);

/*! \brief Duplicate a record 
 * Returns a malloc'd cw_cdr structure, returns NULL on error (malloc failure)
 */
extern struct cw_cdr *cw_cdr_dup(struct cw_cdr *cdr);

/*! \brief Free a CDR record 
 * \param cdr cw_cdr structure to free
 * Returns nothing important
 */
extern void cw_cdr_free(struct cw_cdr *cdr);

/*! \brief Initialize based on a channel
 * \param cdr Call Detail Record to use for channel
 * \param chan Channel to bind CDR with
 * Initializes a CDR and associates it with a particular channel
 * Return is negligible.  (returns 0 by default)
 */
extern int cw_cdr_init(struct cw_cdr *cdr, struct cw_channel *chan);

/*! Initialize based on a channel */
/*! 
 * \param cdr Call Detail Record to use for channel
 * \param chan Channel to bind CDR with
 * Initializes a CDR and associates it with a particular channel
 * Return is negligible.  (returns 0 by default)
 */
extern int cw_cdr_setcid(struct cw_cdr *cdr, struct cw_channel *chan);

/*! Register a CDR handling engine */
/*!
 * \param name name associated with the particular CDR handler
 * \param desc description of the CDR handler
 * \param be function pointer to a CDR handler
 * Used to register a Call Detail Record handler.
 * Returns -1 on error, 0 on success.
 */
extern int cw_cdr_register(char *name, char *desc, cw_cdrbe be);

/*! Unregister a CDR handling engine */
/*!
 * \param name name of CDR handler to unregister
 * Unregisters a CDR by it's name
 */
extern void cw_cdr_unregister(char *name);

/*! Start a call */
/*!
 * \param cdr the cdr you wish to associate with the call
 * Starts all CDR stuff necessary for monitoring a call
 * Returns nothing important
 */
extern void cw_cdr_start(struct cw_cdr *cdr);

/*! Answer a call */
/*!
 * \param cdr the cdr you wish to associate with the call
 * Starts all CDR stuff necessary for doing CDR when answering a call
 */
extern void cw_cdr_answer(struct cw_cdr *cdr);

/*! Busy a call */
/*!
 * \param cdr the cdr you wish to associate with the call
 * Returns nothing important
 */
extern void cw_cdr_busy(struct cw_cdr *cdr);

/*! Fail a call */
/*!
 * \param cdr the cdr you wish to associate with the call
 * Returns nothing important
 */
extern void cw_cdr_failed(struct cw_cdr *cdr);

/*! Save the result of the call based on the CW_CAUSE_* */
/*!
 * \param cdr the cdr you wish to associate with the call
 * Returns nothing important
 * \param cause the CW_CAUSE_*
 */
extern int cw_cdr_disposition(struct cw_cdr *cdr, int cause);
	
/*! End a call */
/*!
 * \param cdr the cdr you have associated the call with
 * Registers the end of call time in the cdr structure.
 * Returns nothing important
 */
extern void cw_cdr_end(struct cw_cdr *cdr);

/*! Detaches the detail record for posting (and freeing) either now or at a
 * later time in bulk with other records during batch mode operation */
/*! 
 * \param cdr Which CDR to detach from the channel thread
 * Prevents the channel thread from blocking on the CDR handling
 * Returns nothing
 */
extern void cw_cdr_detach(struct cw_cdr *cdr);

/*! Spawns (possibly) a new thread to submit a batch of CDRs to the backend engines */
/*!
 * \param shutdown Whether or not we are shutting down
 * Blocks the callweaver shutdown procedures until the CDR data is submitted.
 * Returns nothing
 */
extern void cw_cdr_submit_batch(int shutdown);

/*! Set the destination channel, if there was one */
/*!
 * \param cdr Which cdr it's applied to
 * Sets the destination channel the CDR is applied to
 * Returns nothing
 */
extern void cw_cdr_setdestchan(struct cw_cdr *cdr, char *chan);

/*! Set the last executed application */
/*!
 * \param cdr which cdr to act upon
 * \param app the name of the app you wish to change it to
 * \param data the data you want in the data field of app you set it to
 * Changes the value of the last executed app
 * Returns nothing
 */
extern void cw_cdr_setapp(struct cw_cdr *cdr, const char *app, const char *data);

/*! Convert a string to a detail record AMA flag */
/*!
 * \param flag string form of flag
 * Converts the string form of the flag to the binary form.
 * Returns the binary form of the flag
 */
extern int cw_cdr_amaflags2int(const char *flag);

/*! Disposition to a string */
/*!
 * \param flag input binary form
 * Converts the binary form of a disposition to string form.
 * Returns a pointer to the string form
 */
extern char *cw_cdr_disp2str(int disposition);

/*! Reset the detail record, optionally posting it first */
/*!
 * \param cdr which cdr to act upon
 * \param flags |CW_CDR_FLAG_POSTED whether or not to post the cdr first before resetting it
 *              |CW_CDR_FLAG_LOCKED whether or not to reset locked CDR's
 */
extern void cw_cdr_reset(struct cw_cdr *cdr, int flags);

/*! Flags to a string */
/*!
 * \param flags binary flag
 * Converts binary flags to string flags
 * Returns string with flag name
 */
extern char *cw_cdr_flags2str(int flags);

extern int cw_cdr_setaccount(struct cw_channel *chan, const char *account);
extern int cw_cdr_setamaflags(struct cw_channel *chan, const char *amaflags);


extern int cw_cdr_setuserfield(struct cw_channel *chan, const char *userfield);
extern int cw_cdr_appenduserfield(struct cw_channel *chan, const char *userfield);


/* Update CDR on a channel */
extern int cw_cdr_update(struct cw_channel *chan);


extern int cw_default_amaflags;

extern int cw_end_cdr_before_h_exten;

extern char cw_default_accountcode[CW_MAX_ACCOUNT_CODE];

extern struct cw_cdr *cw_cdr_append(struct cw_cdr *cdr, struct cw_cdr *newcdr);

/*! Reload the configuration file cdr.conf and start/stop CDR scheduling thread */
extern void cw_cdr_engine_reload(void);

/*! Load the configuration file cdr.conf and possibly start the CDR scheduling thread */
extern int cw_cdr_engine_init(void);

/*! Submit any remaining CDRs and prepare for shutdown */
extern void cw_cdr_engine_term(void);

#endif /* _CALLWEAVER_CDR_H */
