/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2007, Eris Associates Limited, UK
 *
 * Mike Jagdis <mjagdis@eris-associates.co.uk>
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
 * \brief Caller ID (and other GR30) Generation support 
 * Includes code and algorithms from the spandsp library.
 *
 */

/*!
 * \page Caller ID names and numbers
 *
 * Caller ID names are currently 8 bit characters, probably
 * ISO8859-1, depending on what your channel drivers handle.
 *
 * IAX2 and SIP caller ID names are UTF-8
 * On ISDN Caller ID names are 7 bit, Almost ASCII
 * (See http://www.zytrax.com/tech/ia5.html )
 *
 * \note CallWeaver does not currently support SIP UTF-8 caller ID names or caller IDs.
 *
 * \par See also
 * 	\arg \ref callerid.c
 * 	\arg \ref callerid.h
 *	\arg \ref Def_CallerPres
 */
#ifndef _CALLWEAVER_CALLERID_H
#define _CALLWEAVER_CALLERID_H

#include <spandsp.h>

#include "callweaver/channel.h"

/*! Maximum length (in samples) of a buffer used for caller ID data.
 *  32000 is 4s - which should be enough for more than 20 DTMF digits
 */
#define MAX_CALLERID_SIZE 32000


/*! \brief Generates an FSK/DTMF encoded caller ID stream in [ua]law format suitable for transmission.
 *
 * \param sig         Signalling type to use -- one of ADSI_STANDARD_* from spandsp/adsi.h.
 * \param outbuf      Buffer to use.  "outbuf" must be at least MAX_CALLERID_SIZE bytes long
 *                    if you want to be sure you don't have an overrun.
 * \param outlen      Length of "outbuf" available for use.
 * \param pres        Caller ID presentation flags (see include/callweaver/phone_no_utils.h)
 * \param number      Caller number. Use NULL for no number.
 * \param name        Caller name. Use NULL for no name.
 * \param callwaiting Whether this is at the start of a call or is a callwaiting announcement.
 * \param codec       Either CW_FORMAT_ULAW or CW_FORMAT_ALAW.
 *
 * This function creates a stream of caller ID (a callerid spill) data in ulaw or alaw format.
 *
 * \return It returns the size (in bytes) of the data.
 */
extern int cw_callerid_generate(int sig, uint8_t *outbuf, int outlen, int pres, char *number, char *name, int callwaiting, int codec);

/*! \brief Parse the caller ID decoded from an adsi_rx decoded FSK/DTMF stream and apply it to the channel
 *
 * \param adsi       The SpanDSP adsi_rx_state_t used to decode the data stream.
 * \param chan       The CallWeaver channel the caller ID applies to.
 * \param msg        The message buffer containing the decoded data stream.
 * \param len        The length of data in the message buffer.
 *
 * \return 0 if caller ID was successfully decoded, -1 otherwise
 *
 * Note: This function is never called directly but should be called via a SpanDSP adsi_rx
 * callback function that knows the correct adsi and chan arguments.
 * See channels/chan_zap.c for an example.
 */
extern int callerid_get(adsi_rx_state_t *adsi, struct cw_channel *chan, const uint8_t *msg, int len);

/*! \brief Generates an FSK encoded message waiting indication in [ua]law format
 *         suitable for transmission.
 *
 * \param outbuf      Buffer to use.  "outbuf" must be at least MAX_CALLERID_SIZE bytes long
 *                    if you want to be sure you don't have an overrun.
 * \param outlen      Length of "outbuf" available for use.
 * \param active      Whether the message waiting indicator should be active or not.
 * \param mdmf        True if an MDMF indication should be used, FALSE for SDMF.
 * \param codec       Either CW_FORMAT_ULAW or CW_FORMAT_ALAW.
 *
 * \return It returns the size (in bytes) of the data.
 */
extern int vmwi_generate(uint8_t *outbuf, int outlen, int active, int mdmf, int codec);

/*! \brief Generate a CAS (CPE Alert Signal) tone, optionally preceeded by SAS (Subscriber Alert Signal)
 *
 * \param outbuf      Buffer to use.  Must be at least 2400 bytes unless no SAS is desired.
 * \param outlen      Length of "outbuf" available for use.
 * \param sendsas     Non-zero if CAS should be preceeded by SAS.
 * \param codec       Either CW_FORMAT_ULAW or CW_FORMAT_ALAW.
 *
 * \return Returns -1 on error (if len is less than 2400), 0 on success.
 */
extern int cw_gen_cas(uint8_t *outbuf, int outlen, int sendsas, int codec);

	/* A 2100Hz monotone (commonly known as a fax tone) may be used to cause
	 * most switches (with the possible exceptio of some ancient pre-fax
	 * switches maybe?) to disable echo cancelling and provide a clean channel.
	 */
/*! \brief Generate a 2100Hz monotone (or "fax tone") to request downstream
 *         echo cancelling is turned off.
 *
 * \param outbuf      Buffer to use.
 * \param outlen      Length of "outbuf" to use.
 * \param codec       Either CW_FORMAT_ULAW or CW_FORMAT_ALAW.
 */
extern int cw_gen_ecdisa(uint8_t *outbuf, int outlen, int codec);


/*
 * Caller*ID and other GR-30 compatible generation
 * routines (used by ADSI for example)
 */

extern int mate_generate(uint8_t *outbuf, int outlen, const char *msg, int codec);

#define	TDD_SAMPLES_PER_CHAR	2700

struct tdd_state;

extern int tdd_generate(struct tdd_state *tdd, uint8_t *outbuf, int outlen, const char *str, int codec);

extern int tdd_feed(struct tdd_state *tdd, uint8_t *xlaw, int len, int codec);

extern struct tdd_state *tdd_new(void);

extern void tdd_free(struct tdd_state *tdd);


#endif /* _CALLWEAVER_CALLERID_H */
