/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * Modifications by Carlos Antunes <cmantunes@gmail.com>
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

/*
 * Synthetic frame generation definitions.
 * This only needs to be included by callweaver/channel.h
 */

#ifndef _CALLWEAVER_GENERATOR_H
#define _CALLWEAVER_GENERATOR_H

#define GENERATOR_WAIT_ITERATIONS 100

/*! Data structure used to register new generator */
struct cw_generator {
	void *(*alloc)(struct cw_channel *chan, void *params);
	void (*release)(struct cw_channel *chan, void *data);
	int (*generate)(struct cw_channel *chan, void *data, int samples);
};

/*! Requests sent to generator thread */
enum cw_generator_requests {
	/* this really means 'no request' and MUST be zero*/
	gen_req_null = 0,
	/* deactivate current generator if any and activate new one */
	gen_req_activate,
	/* deactivate current generator, if any */
	gen_req_deactivate,
	/* deactivate current generator if any and terminate gen thread */
	gen_req_shutdown
};

/*! Generator channel data */
struct cw_generator_channel_data {

	/*! Generator data structures mutex (those in here below).
	 * To avoid deadlocks, never acquire channel lock when
	 * generator data lock is acquired */
	cw_mutex_t lock;

	/*! Generator thread for this channel */
	pthread_t *pgenerator_thread;

	/*! Non-zero if generator is currently active; zero otherwise */
	int gen_is_active;

	/*! Generator request condition gets signaled after
	 * changing gen_req to new value */
	cw_cond_t gen_req_cond;

	/*! New generator request available flag */
	enum cw_generator_requests gen_req;

	/*! New generator data */
	void *gen_data;

	/*! How many samples to generate each time*/
	int gen_samp;

	/*! How is our codec supposed to be? What's the sample frequency ? */
	int samples_per_second;

	/*! New generator function */
	int (*gen_func)(struct cw_channel *chan, void *gen_data, int gen_samp);

	/*! What to call to free (release) gen_data */
	void (*gen_free)(struct cw_channel *chan, void *gen_data);
};

/*! Activate a given generator */
int cw_generator_activate(struct cw_channel *chan, struct cw_generator *gen, void *params);

/*! Deactive an active generator */
void cw_generator_deactivate(struct cw_channel *chan);

/*! Is generator active on channel? */
inline int cw_generator_is_active(struct cw_channel *chan);

/*! Is the caller of this function running in the generator thread? */
inline int cw_generator_is_self(struct cw_channel *chan);

#endif /* _CALLWEAVER_GENERATOR_H */

