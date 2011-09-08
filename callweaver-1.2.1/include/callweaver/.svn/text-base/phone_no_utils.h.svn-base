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

/*
 * CallerID (and other GR30) Generation support 
 * Includes code and algorithms from the Zapata library.
 */

#ifndef _CALLWEAVER_PHONE_NO_UTILS_H
#define _CALLWEAVER_PHONE_NO_UTILS_H

/*! Shrink a phone number in place to just digits (more accurately it just removes ()'s, .'s, and -'s... */
/*!
 * \param n The number to be stripped/shrunk
 */
extern void cw_shrink_phone_number(char *n);

/*! Check if a string consists only of digits.  Returns non-zero if so */
/*!
 * \param n number to be checked.
 * \return 0 if n is a number, 1 if it's not.
 */
extern int cw_isphonenumber(const char *n);

/* Various defines and bits for handling PRI- and SS7-type restriction */

#define CW_PRES_NUMBER_TYPE				0x03
#define CW_PRES_USER_NUMBER_UNSCREENED			0x00
#define CW_PRES_USER_NUMBER_PASSED_SCREEN		0x01
#define CW_PRES_USER_NUMBER_FAILED_SCREEN		0x02
#define CW_PRES_NETWORK_NUMBER				0x03

#define CW_PRES_RESTRICTION				0x60
#define CW_PRES_ALLOWED				0x00
#define CW_PRES_RESTRICTED				0x20
#define CW_PRES_UNAVAILABLE				0x40
#define CW_PRES_RESERVED				0x60

#define CW_PRES_ALLOWED_USER_NUMBER_NOT_SCREENED \
	CW_PRES_USER_NUMBER_UNSCREENED + CW_PRES_ALLOWED

#define CW_PRES_ALLOWED_USER_NUMBER_PASSED_SCREEN \
	CW_PRES_USER_NUMBER_PASSED_SCREEN + CW_PRES_ALLOWED

#define CW_PRES_ALLOWED_USER_NUMBER_FAILED_SCREEN \
	CW_PRES_USER_NUMBER_FAILED_SCREEN + CW_PRES_ALLOWED

#define CW_PRES_ALLOWED_NETWORK_NUMBER	\
	CW_PRES_NETWORK_NUMBER + CW_PRES_ALLOWED

#define CW_PRES_PROHIB_USER_NUMBER_NOT_SCREENED \
	CW_PRES_USER_NUMBER_UNSCREENED + CW_PRES_RESTRICTED

#define CW_PRES_PROHIB_USER_NUMBER_PASSED_SCREEN \
	CW_PRES_USER_NUMBER_PASSED_SCREEN + CW_PRES_RESTRICTED

#define CW_PRES_PROHIB_USER_NUMBER_FAILED_SCREEN \
	CW_PRES_USER_NUMBER_FAILED_SCREEN + CW_PRES_RESTRICTED

#define CW_PRES_PROHIB_NETWORK_NUMBER \
	CW_PRES_NETWORK_NUMBER + CW_PRES_RESTRICTED

#define CW_PRES_NUMBER_NOT_AVAILABLE \
	CW_PRES_NETWORK_NUMBER + CW_PRES_UNAVAILABLE

extern int cw_parse_caller_presentation(const char *data);

extern const char *cw_describe_caller_presentation(int data);

extern int cw_callerid_split(const char *src, char *name, int namelen, char *num, int numlen);

extern char *cw_callerid_merge(char *buf, int bufsiz, const char *name, const char *num, const char *unknown);

/*! Destructively parse inbuf into name and location (or number) */
extern int cw_callerid_parse(char *instr, char **name, char **location);

#endif /* _CALLWEAVER_PHONE_NO_UTILS_H */
