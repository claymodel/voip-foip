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
 * \brief A-Law to Signed linear conversion
 */

#ifndef _CALLWEAVER_ALAW_H
#define _CALLWEAVER_ALAW_H

/*! Init the ulaw conversion stuff */
/*!
 * To init the ulaw to slinear conversion stuff, this needs to be run.
 */
extern void cw_alaw_init(void);

/*! converts signed linear to alaw */
extern uint8_t __cw_lin2a[8192];

/*! converts alaw to signed linear */
extern int16_t __cw_alaw[256];

#define CW_LIN2A(a) (__cw_lin2a[((unsigned short)(a)) >> 3])
#define CW_ALAW(a) (__cw_alaw[(int)(a)])

#endif /* _CALLWEAVER_ALAW_H */
