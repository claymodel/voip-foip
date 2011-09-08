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
 * \brief Say numbers and dates (maybe words one day too)
 */

#ifndef _CALLWEAVER_SAY_H
#define _CALLWEAVER_SAY_H

#include "callweaver/channel.h"
#include "callweaver/file.h"

#include <time.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* says a number
 * \param chan channel to say them number on
 * \param num number to say on the channel
 * \param ints which dtmf to interrupt on
 * \param lang language to speak the number
 * \param options set to 'f' for female, 'm' for male, 'c' for commune, 'n' for neuter, 'p' for plural
 * Vocally says a number on a given channel
 * Returns 0 on success, DTMF digit on interrupt, -1 on failure
 */
int cw_say_number(struct cw_channel *chan, int num, const char *ints, const char *lang, const char *options);

/* Same as above with audiofd for received audio and returns 1 on ctrlfd being readable */
int cw_say_number_full(struct cw_channel *chan, int num, const char *ints, const char *lang, const char *options, int audiofd, int ctrlfd);

/* says an enumeration
 * \param chan channel to say them enumeration on
 * \param num number to say on the channel
 * \param ints which dtmf to interrupt on
 * \param lang language to speak the enumeration
 * \param options set to 'f' for female, 'm' for male, 'c' for commune, 'n' for neuter, 'p' for plural
 * Vocally says a enumeration on a given channel (first, sencond, third, forth, thirtyfirst, hundredth, ....) 
 * especially useful for dates and messages. says 'last' if num equals to INT_MAX
 * Returns 0 on success, DTMF digit on interrupt, -1 on failure
 */
int cw_say_enumeration(struct cw_channel *chan, int num, const char *ints, const char *lang, const char *options);
int cw_say_enumeration_full(struct cw_channel *chan, int num, const char *ints, const char *lang, const char *options, int audiofd, int ctrlfd);

/* says digits
 * \param chan channel to act upon
 * \param num number to speak
 * \param ints which dtmf to interrupt on
 * \param lang language to speak
 * Vocally says digits of a given number
 * Returns 0 on success, dtmf if interrupted, -1 on failure
 */
int cw_say_digits(struct cw_channel *chan, int num, const char *ints, const char *lang);
int cw_say_digits_full(struct cw_channel *chan, int num, const char *ints, const char *lang, int audiofd, int ctrlfd);

/* says digits of a string
 * \param chan channel to act upon
 * \param num string to speak
 * \param ints which dtmf to interrupt on
 * \param lang language to speak in
 * Vocally says the digits of a given string
 * Returns 0 on success, dtmf if interrupted, -1 on failure
 */
int cw_say_digit_str(struct cw_channel *chan, const char *num, const char *ints, const char *lang);
int cw_say_digit_str_full(struct cw_channel *chan, const char *num, const char *ints, const char *lang, int audiofd, int ctrlfd);
int cw_say_character_str(struct cw_channel *chan, const char *num, const char *ints, const char *lang);
int cw_say_character_str_full(struct cw_channel *chan, const char *num, const char *ints, const char *lang, int audiofd, int ctrlfd);
int cw_say_phonetic_str(struct cw_channel *chan, const char *num, const char *ints, const char *lang);
int cw_say_phonetic_str_full(struct cw_channel *chan, const char *num, const char *ints, const char *lang, int audiofd, int ctrlfd);

int cw_say_datetime(struct cw_channel *chan, time_t t, const char *ints, const char *lang);

int cw_say_time(struct cw_channel *chan, time_t t, const char *ints, const char *lang);

int cw_say_date(struct cw_channel *chan, time_t t, const char *ints, const char *lang);

int cw_say_datetime_from_now(struct cw_channel *chan, time_t t, const char *ints, const char *lang);

int cw_say_date_with_format(struct cw_channel *chan, time_t t, const char *ints, const char *lang, const char *format, const char *timezone);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_SAY_H */
