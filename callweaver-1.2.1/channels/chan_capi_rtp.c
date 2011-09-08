/*
 * (CAPI*)
 *
 * An implementation of Common ISDN API 2.0 for CallWeaver
 *
 * Copyright (C) 2006 Cytronics & Melware
 *
 * Armin Schindler <armin@melware.de>
 * 
 * This program is free software and may be modified and 
 * distributed under the terms of the GNU Public License.
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "callweaver/lock.h"
#include "callweaver/frame.h"
#include "callweaver/channel.h"
#include "callweaver/logger.h"
#include "callweaver/module.h"
#include "callweaver/pbx.h"
#include "callweaver/config.h"
#include "callweaver/options.h"
#include "callweaver/features.h"
#include "callweaver/utils.h"
#include "callweaver/rtp.h"
#include "callweaver/strings.h"
#include "callweaver/chan_capi20.h"
#include "callweaver/chan_capi.h"
#include "callweaver/chan_capi_rtp.h"

/* RTP settings / NCPI RTP struct */

static unsigned char NCPI_voice_over_ip_alaw[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len Ulaw    Alaw     */
  "\x04\x00\x00\x08\x08"
/* Len Alaw     */
  "\x02\x08\x08"
/* Len UlawLen Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x00\x04\x03\x00\xa0\x00\x08\x04\x03\x00\xa0\x00";

static unsigned char NCPI_voice_over_ip_ulaw[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len Ulaw    Alaw     */
  "\x04\x00\x00\x08\x08"
/* Len Ulaw     */
  "\x02\x00\x00"
/* Len UlawLen Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x00\x04\x03\x00\xa0\x00\x08\x04\x03\x00\xa0\x00";

static unsigned char NCPI_voice_over_ip_gsm[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len GSM     Alaw     */
  "\x04\x03\x03\x08\x08"
/* Len GSM      */
  "\x02\x03\x03"
/* Len GSM Len Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x03\x04\x0f\x00\xa0\x00\x08\x04\x00\x00\xa0\x00";

static unsigned char NCPI_voice_over_ip_g723[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len G723    Alaw     */
  "\x04\x04\x04\x08\x08"
/* Len G723     */
  "\x02\x04\x04"
/* Len G723Len Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x04\x04\x01\x00\xa0\x00\x08\x04\x00\x00\xa0\x00";

static unsigned char NCPI_voice_over_ip_g726[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len G726    Alaw     */
  "\x04\x02\x02\x08\x08"
/* Len G726     */
  "\x02\x02\x02"
/* Len G726Len Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x02\x04\x0f\x00\xa0\x00\x08\x04\x00\x00\xa0\x00";

static unsigned char NCPI_voice_over_ip_g729[] =
/* Len Options          */
  "\x27\x00\x00\x00\x00"
/* Len Filt */
  "\x00"
/* Len Tem PT  Seq     Timestamp       SSRC             */
  "\x0c\x80\x00\x00\x00\x00\x00\x00\x00\x12\x34\x56\x78"
/* Len G729    Alaw     */
  "\x04\x12\x12\x08\x08"
/* Len G729     */
  "\x02\x12\x12"
/* Len G729Len Opts    IntervalAlawLen Opts    Interval */
  "\x0c\x12\x04\x0f\x00\xa0\x00\x08\x04\x00\x00\xa0\x00";


/*
 * return NCPI for chosen RTP codec
 */
_cstruct capi_rtp_ncpi(struct capi_pvt *i)
{
	_cstruct ncpi = NULL;

	if ((i) && (i->owner) &&
	    (i->bproto == CC_BPROTO_RTP)) {
		switch(i->codec) {
		case CW_FORMAT_ALAW:
			ncpi = NCPI_voice_over_ip_alaw;
			break;
		case CW_FORMAT_ULAW:
			ncpi = NCPI_voice_over_ip_ulaw;
			break;
		case CW_FORMAT_GSM:
			ncpi = NCPI_voice_over_ip_gsm;
			break;
		case CW_FORMAT_G723_1:
			ncpi = NCPI_voice_over_ip_g723;
			break;
		case CW_FORMAT_G726:
			ncpi = NCPI_voice_over_ip_g726;
			break;
		case CW_FORMAT_G729A:
			ncpi = NCPI_voice_over_ip_g729;
			break;
		default:
			cc_log(LOG_ERROR, "%s: format %s(%d) invalid.\n",
				i->vname, cw_getformatname(i->codec), i->codec);
			break;
		}
	}

	return ncpi;
}

/*
 * create rtp for capi interface
 */
int capi_alloc_rtp(struct capi_pvt *i)
{
	struct cw_hostent ahp;
	struct hostent *hp;
	struct in_addr addr;
	struct sockaddr_in us;
	char temp[MAXHOSTNAMELEN];

	hp = cw_gethostbyname("localhost", &ahp);
	memcpy(&addr, hp->h_addr, sizeof(addr));

	if (!(i->rtp = cw_rtp_new_with_bindaddr(NULL, NULL, 0, 0, addr))) {
		cc_log(LOG_ERROR, "%s: unable to alloc rtp.\n", i->vname);
		return 1;
	}
	cw_rtp_get_us(i->rtp, &us);
	cw_rtp_set_peer(i->rtp, &us);
	cc_verbose(2, 1, VERBOSE_PREFIX_4 "%s: alloc rtp socket on %s:%d\n",
		i->vname,
		cw_inet_ntoa(temp, sizeof(temp), us.sin_addr),
		ntohs(us.sin_port));
	i->timestamp = 0;
	return 0;
}

/*
 * write rtp for a channel
 */
int capi_write_rtp(struct cw_channel *c, struct cw_frame *f)
{
	struct capi_pvt *i = CC_CHANNEL_PVT(c);
	_cmsg CMSG;
	struct sockaddr_in us;
	int len;
	unsigned int uslen;
	unsigned int *rtpheader;
	unsigned char buf[256];

	uslen = sizeof(us);

	if (!(i->rtp)) {
		cc_log(LOG_ERROR, "rtp struct is NULL\n");
		return -1;
	}

	cw_rtp_get_us(i->rtp, &us);
	cw_rtp_set_peer(i->rtp, &us);
	if (cw_rtp_write(i->rtp, f) != 0) {
		cc_verbose(3, 0, VERBOSE_PREFIX_2 "%s: rtp_write error, dropping packet.\n",
			i->vname);
		return 0;
	}

	while(1) {
		len = recvfrom(cw_rtp_fd(i->rtp), buf, sizeof(buf),
			0, (struct sockaddr *)&us, &uslen);
		if (len <= 0)
			break;

		rtpheader = (unsigned int *)buf;
		
		rtpheader[1] = htonl(i->timestamp);
		i->timestamp += CAPI_MAX_B3_BLOCK_SIZE;
			
		if (len > (CAPI_MAX_B3_BLOCK_SIZE + RTP_HEADER_SIZE)) {
			cc_verbose(4, 0, VERBOSE_PREFIX_4 "%s: rtp write data: frame too big (len = %d).\n",
				i->vname, len);
			continue;
		}
		if (i->B3q >= CAPI_MAX_B3_BLOCKS) {
			cc_verbose(3, 1, VERBOSE_PREFIX_4 "%s: B3q is full, dropping packet.\n",
				i->vname);
			continue;
		}
		cc_mutex_lock(&i->lock);
		i->B3q++;
		cc_mutex_unlock(&i->lock);

		i->send_buffer_handle++;

		cc_verbose(6, 1, VERBOSE_PREFIX_4 "%s: RTP write for NCCI=%#x len=%d(%d) %s ts=%x\n",
			i->vname, i->NCCI, len, f->datalen, cw_getformatname(f->subclass),
			i->timestamp);

		DATA_B3_REQ_HEADER(&CMSG, capi_ApplID, get_capi_MessageNumber(), 0);
		DATA_B3_REQ_NCCI(&CMSG) = i->NCCI;
		DATA_B3_REQ_FLAGS(&CMSG) = 0;
		DATA_B3_REQ_DATAHANDLE(&CMSG) = i->send_buffer_handle;
		DATA_B3_REQ_DATALENGTH(&CMSG) = len;
		DATA_B3_REQ_DATA(&CMSG) = (buf);

		_capi_put_cmsg(&CMSG);
	}

	return 0;
}

/*
 * read data b3 in RTP mode
 */
struct cw_frame *capi_read_rtp(struct capi_pvt *i, unsigned char *buf, int len)
{
	struct cw_frame *f;
	struct sockaddr_in us;

	if (!(i->owner))
		return NULL;

	if (!(i->rtp)) {
		cc_log(LOG_ERROR, "rtp struct is NULL\n");
		return NULL;
	}

	cw_rtp_get_us(i->rtp, &us);
	cw_rtp_set_peer(i->rtp, &us);

	if (len != sendto(cw_rtp_fd(i->rtp), buf, len, 0, (struct sockaddr *)&us, sizeof(us))) {
		cc_verbose(4, 1, VERBOSE_PREFIX_3 "%s: RTP sendto error\n",
			i->vname);
		return NULL;
	}

	if ((f = cw_rtp_read(i->rtp))) {
		if (f->frametype != CW_FRAME_VOICE) {
			cc_verbose(3, 1, VERBOSE_PREFIX_3 "%s: DATA_B3_IND RTP (len=%d) non voice type=%d\n",
				i->vname, len, f->frametype);
			return NULL;
		}
		cc_verbose(6, 1, VERBOSE_PREFIX_4 "%s: DATA_B3_IND RTP NCCI=%#x len=%d %s (read/write=%d/%d)\n",
			i->vname, i->NCCI, len, cw_getformatname(f->subclass),
			i->owner->readformat, i->owner->writeformat);
		if (i->owner->nativeformats != f->subclass) {
			cc_verbose(3, 1, VERBOSE_PREFIX_3 "%s: DATA_B3_IND RTP nativeformats=%d, but subclass=%d\n",
				i->vname, i->owner->nativeformats, f->subclass);
			i->owner->nativeformats = f->subclass;
			cw_set_read_format(i->owner, i->owner->readformat);
			cw_set_write_format(i->owner, i->owner->writeformat);
		}
	}
	return f;
}

/*
 * eval RTP profile
 */
void voice_over_ip_profile(struct cc_capi_controller *cp)
{
	MESSAGE_EXCHANGE_ERROR error;
	_cmsg CMSG;
	struct timeval tv;
	unsigned char fac[4] = "\x03\x02\x00\x00";
	int waitcount = 200;
	unsigned short info = 0;
	unsigned int payload1, payload2;

	FACILITY_REQ_HEADER(&CMSG, capi_ApplID, get_capi_MessageNumber(), 0);
	FACILITY_REQ_CONTROLLER(&CMSG) = cp->controller;
	FACILITY_REQ_FACILITYSELECTOR(&CMSG) = FACILITYSELECTOR_VOICE_OVER_IP;
	FACILITY_REQ_FACILITYREQUESTPARAMETER(&CMSG) = (_cstruct)&fac;
	_capi_put_cmsg(&CMSG);

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	while (waitcount) {
		error = capi20_waitformessage(capi_ApplID, &tv);
		error = capi_get_cmsg(&CMSG, capi_ApplID); 
		if (error == 0) {
			if (IS_FACILITY_CONF(&CMSG)) {
				info = 1;
				break;
			}
		}
		usleep(20000);
		waitcount--;
	} 
	if (!info) {
		cc_log(LOG_WARNING, "did not receive FACILITY_CONF\n");
		return;
	}

	/* parse profile */
	if (FACILITY_CONF_FACILITYSELECTOR(&CMSG) != FACILITYSELECTOR_VOICE_OVER_IP) {
		cc_log(LOG_WARNING, "unexpected FACILITY_SELECTOR = %#x\n",
			FACILITY_CONF_FACILITYSELECTOR(&CMSG));
		return;
	}
	if ((info = FACILITY_CONF_INFO(&CMSG)) != 0x0000) {
		cc_verbose(3, 0, VERBOSE_PREFIX_4 "FACILITY_CONF INFO = %#x, RTP not used.\n",
			info);
		return;

	}
	if (FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[0] < 13) {
		cc_log(LOG_WARNING, "conf parameter too short %d, RTP not used.\n",
			FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[0]);
		return;
	}
	info = read_capi_word(&(FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[1]));
	if (info != 0x0002) {
		cc_verbose(3, 0, VERBOSE_PREFIX_4 "FACILITY_CONF wrong parameter (0x%04x), RTP not used.\n",
			info);
		return;
	}
	info = read_capi_word(&(FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[4]));
	payload1 = read_capi_dword(&(FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[6]));
	payload2 = read_capi_dword(&(FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&CMSG)[10]));
	cc_verbose(3, 0, VERBOSE_PREFIX_4 "RTP payload options 0x%04x 0x%08x 0x%08x\n",
		info, payload1, payload2);

	cc_verbose(3, 0, VERBOSE_PREFIX_4 "RTP codec: ");
	if (payload1 & 0x00000100) {
		cp->rtpcodec |= CW_FORMAT_ALAW;
		cc_verbose(3, 0, "G.711-alaw ");
	}
	if (payload1 & 0x00000001) {
		cp->rtpcodec |= CW_FORMAT_ULAW;
		cc_verbose(3, 0, "G.711-ulaw ");
	}
	if (payload1 & 0x00000008) {
		cp->rtpcodec |= CW_FORMAT_GSM;
		cc_verbose(3, 0, "GSM ");
	}
	if (payload1 & 0x00000010) {
		cp->rtpcodec |= CW_FORMAT_G723_1;
		cc_verbose(3, 0, "G.723.1 ");
	}
	if (payload1 & 0x00000004) {
		cp->rtpcodec |= CW_FORMAT_G726;
		cc_verbose(3, 0, "G.726 ");
	}
	if (payload1 & 0x00040000) {
		cp->rtpcodec |= CW_FORMAT_G729A;
		cc_verbose(3, 0, "G.729");
	}
	cc_verbose(3, 0, "\n");
}

