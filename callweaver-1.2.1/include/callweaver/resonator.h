/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2005, Carlos Antunes
 *
 * Carlos Antunes <cmantunes@gmail.com>
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
 *
 * Digital Resonator - to produce low cost sinewaves
 *
 */

#ifndef _CALLWEAVER_RESONATOR_H
#define _CALLWEAVER_RESONATOR_H

/* A digital resonator is a fast, efficient way of producing sinewaves */
struct digital_resonator {
	int16_t k;		/* x(n) = k * x(n-1) - x(n-2) */
	int16_t xnm1;		/* xnm1 = x(n-1) */
	int16_t xnm2;		/* xnm2 = x(n-2) */
	uint16_t cur_freq;	/* frequency currently being generated */
	int16_t cur_ampl;	/* current signal amplitude */
	uint16_t cur_samp;	/* current sampling frequency */
	int16_t upper_limit;	/* samples upper limit */
	int16_t lower_limit;	/* samples lower limit */
};

/* Initial paramaters for the digital resonator */
void digital_resonator_init(struct digital_resonator *dr, uint16_t requested_frequency, int16_t requested_amplitude, uint16_t sampling_frequency);

/* Reinitialization paramaters for the digital resonator */
void digital_resonator_reinit(struct digital_resonator *dr, uint16_t new_frequency, int16_t new_amplitude);

/* How we get samples after resonator is initialized */
inline int16_t digital_resonator_get_sample(struct digital_resonator *dr);

#endif /* CALLWEAVER_RESONATOR_H */
