/*
 * (SCCP*)
 *
 * An implementation of Skinny Client Control Protocol (SCCP)
 *
 * Sergio Chersovani (mlists@c-net.it)
 *
 * Reworked, but based on chan_sccp code.
 * The original chan_sccp driver that was made by Zozo which itself was derived from the chan_skinny driver.
 * Modified by Jan Czmok and Julien Goodwin
 *
 * This program is free software and may be modified and
 * distributed under the terms of the GNU Public License.
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include "chan_sccp.h"
#include "sccp_indicate.h"
#include "sccp_device.h"
#include "sccp_channel.h"
#include "sccp_actions.h"
#include "sccp_utils.h"

void sccp_indicate_lock(sccp_channel_t * c, uint8_t state) {
	if (!c)
		return;
	cw_mutex_lock(&c->lock);
	sccp_indicate_nolock(c, state);
	cw_mutex_unlock(&c->lock);
}

void sccp_indicate_nolock(sccp_channel_t * c, uint8_t state) {
	sccp_device_t * d;
	sccp_line_t * l;
	uint8_t oldstate;

	if (!c) {
		cw_log(LOG_ERROR, "SCCP: No channel, nothing to indicate?\n");
		return;
	}
	if (!c->device) {
		cw_log(LOG_ERROR, "SCCP: The channel %d does not have a device\n",c->callid);
		return;
	}

	d = c->device;

	if (!c->line) {
		cw_log(LOG_ERROR, "SCCP: The channel %d does not have a line\n",c->callid);
		return;
	}

	l = c->line;

	/* all the check are ok. We can safely run all the dev functions with no more checks */
	sccp_log(10)(VERBOSE_PREFIX_3 "%s: Indicate SCCP state (%s) on call %s-%d\n",d->id, sccp_indicate2str(state), l->name, c->callid);

	oldstate = c->state;
	c->state = state;

	switch (state) {
	case SCCP_CHANNELSTATE_DOWN:
/*		sccp_cw_setstate(c, CW_STATE_DOWN); */
		break;
	case SCCP_CHANNELSTATE_OFFHOOK:
		/* clear all the display buffers */
		sccp_dev_cleardisplaynotify(d);
		sccp_dev_clearprompt(d, 0, 0);
		sccp_dev_set_mwi(d, l, 0);
		if (!d->mwioncall)
			sccp_dev_set_mwi(d, NULL, 0);
		sccp_dev_set_ringer(d, SKINNY_STATION_RINGOFF, l->instance, c->callid);
		sccp_dev_set_speaker(d, SKINNY_STATIONSPEAKER_ON);
//		sccp_dev_set_microphone(d, SKINNY_STATIONMIC_ON);
		sccp_dev_set_lamp(d, SKINNY_STIMULUS_LINE, l->instance, SKINNY_LAMP_ON);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_OFFHOOK);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_ENTER_NUMBER, 0);
		sccp_dev_set_keyset(d,l->instance,c->callid, KEYMODE_OFFHOOK);
		sccp_dev_set_cplane(l,1);
		sccp_dev_starttone(d, SKINNY_TONE_INSIDEDIALTONE, l->instance, c->callid, 0);
		sccp_cw_setstate(c, CW_STATE_OFFHOOK);
		/* for earlyrtp take a look at sccp_channel_newcall because we have no c->owner here */
		break;
	case SCCP_CHANNELSTATE_ONHOOK:
		if (c == d->active_channel)
			sccp_dev_set_speaker(d, SKINNY_STATIONSPEAKER_OFF);
		sccp_dev_clearprompt(d,l->instance, c->callid);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_ONHOOK);
		sccp_dev_set_keyset(d,l->instance,c->callid, KEYMODE_ONHOOK);
		if (oldstate != SCCP_CHANNELSTATE_CALLWAITING)
			sccp_dev_set_lamp(d, SKINNY_STIMULUS_LINE, l->instance, SKINNY_LAMP_OFF);
		if (c == d->active_channel)
			sccp_dev_stoptone(d, l->instance, c->callid);
		if (oldstate == SCCP_CHANNELSTATE_RINGIN)
			sccp_dev_set_ringer(d, SKINNY_STATION_RINGOFF, l->instance, c->callid);
		sccp_handle_time_date_req(d->session, NULL);
		c->state = SCCP_CHANNELSTATE_DOWN;
		sccp_cw_setstate(c, CW_STATE_DOWN);
		break;
	case SCCP_CHANNELSTATE_RINGOUT:
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_RINGOUT);
		sccp_channel_send_callinfo(c);
		if (d->earlyrtp ==  SCCP_CHANNELSTATE_RINGOUT && !c->rtp) {
			sccp_channel_openreceivechannel(c);
		}
		/* it will be emulated if the rtp audio stream is open */
		if (!c->rtp)
			sccp_dev_starttone(d, SKINNY_TONE_ALERTINGTONE, l->instance, c->callid, 0);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_RINGOUT);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_RING_OUT, 0);
		sccp_cw_setstate(c, CW_STATE_RING);
		break;
	case SCCP_CHANNELSTATE_RINGIN:
		/* clear all the display buffers */
		sccp_dev_cleardisplaynotify(d);
		sccp_dev_clearprompt(d, 0, 0);
		sccp_dev_set_mwi(d, l, 0);
		if (!d->mwioncall)
			sccp_dev_set_mwi(d, NULL, 0);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_RINGIN);
		sccp_channel_send_callinfo(c);
		sccp_dev_set_lamp(d, SKINNY_STIMULUS_LINE, l->instance, SKINNY_LAMP_BLINK);
		sccp_dev_set_ringer(d, c->ringermode, l->instance, c->callid);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_RINGIN);
		sccp_cw_setstate(c, CW_STATE_RINGING);
		break;
	case SCCP_CHANNELSTATE_CONNECTED:
		sccp_dev_set_ringer(d, SKINNY_STATION_RINGOFF, l->instance, c->callid);
		sccp_dev_set_speaker(d, SKINNY_STATIONSPEAKER_ON);
//		if (c->calltype == SKINNY_CALLTYPE_OUTBOUND)
		sccp_dev_stoptone(d, l->instance, c->callid);
		sccp_dev_set_lamp(d, SKINNY_STIMULUS_LINE, l->instance, SKINNY_LAMP_ON);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_CONNECTED);
		sccp_channel_send_callinfo(c);
		sccp_dev_set_cplane(l,1);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_CONNECTED);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_CONNECTED, 0);
		if (!c->rtp) {
			sccp_channel_openreceivechannel(c);
		}
		/* callweaver wants rtp open before CW_STATE_UP */
		sccp_cw_setstate(c, CW_STATE_UP);
		break;
	case SCCP_CHANNELSTATE_BUSY:
		/* it will be emulated if the rtp audio stream is open */
		if (!c->rtp)
			sccp_dev_starttone(d, SKINNY_TONE_LINEBUSYTONE, l->instance, c->callid, 0);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_BUSY, 0);
		sccp_cw_setstate(c, CW_STATE_BUSY);
		break;
	case SCCP_CHANNELSTATE_PROCEED:
		sccp_dev_stoptone(d, l->instance, c->callid);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_PROCEED);
		sccp_channel_send_callinfo(c);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_CALL_PROCEED, 0);
		if (!c->rtp) {
			sccp_channel_openreceivechannel(c);
		}
		/* sccp_cw_setstate(c, CW_STATE_UP); */
		break;
	case SCCP_CHANNELSTATE_HOLD:
		sccp_channel_closereceivechannel(c);
		sccp_handle_time_date_req(d->session, NULL);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_HOLD);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_ONHOLD);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_HOLD, 0);
//		sccp_dev_set_microphone(d, SKINNY_STATIONMIC_OFF);
		sccp_dev_set_speaker(d, SKINNY_STATIONSPEAKER_OFF);
		sccp_dev_set_lamp(d, SKINNY_STIMULUS_LINE, l->instance, SKINNY_LAMP_WINK);
		break;
	case SCCP_CHANNELSTATE_CONGESTION:
		/* it will be emulated if the rtp audio stream is open */
		if (!c->rtp)
			sccp_dev_starttone(d, SKINNY_TONE_REORDERTONE, l->instance, c->callid, 0);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_CONGESTION);
		break;
	case SCCP_CHANNELSTATE_CALLWAITING:
		if (GLOB(callwaiting_tone))
			sccp_dev_starttone(d, GLOB(callwaiting_tone), l->instance, c->callid, 0);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_RINGIN);
		sccp_channel_send_callinfo(c);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_CALL_WAITING, 0);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_RINGIN);
		sccp_cw_setstate(c, CW_STATE_RINGING);
		break;
	case SCCP_CHANNELSTATE_CALLTRANSFER:
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_TRANSFER, 0);
		/* sccp_channel_set_callstate(c, SKINNY_CALLSTATE_CALLTRANSFER); */
		break;
	case SCCP_CHANNELSTATE_CALLPARK:
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_CALLPARK);
		break;
	case SCCP_CHANNELSTATE_CALLREMOTEMULTILINE:
		break;
	case SCCP_CHANNELSTATE_INVALIDNUMBER:
		/* this is for the earlyrtp. The 7910 does not play tones if a rtp stream is open */
		if (c->rtp)
			sccp_channel_closereceivechannel(c);
		sccp_dev_starttone(d, SKINNY_TONE_REORDERTONE, l->instance, c->callid, 0);
		sccp_channel_set_callstate(c, SKINNY_CALLSTATE_INVALIDNUMBER);
		sccp_channel_send_callinfo(c);
		sccp_dev_displayprompt(d, l->instance, c->callid, SKINNY_DISP_UNKNOWN_NUMBER, 0);
		/* don't set CW_STATE_DOWN. we hangup only on onhook and endcall softkey */
		break;
	case SCCP_CHANNELSTATE_DIALING:
		sccp_dev_stoptone(d, l->instance, c->callid);
		sccp_channel_send_dialednumber(c);
		sccp_dev_set_keyset(d, l->instance, c->callid, KEYMODE_DIGITSFOLL);
		sccp_dev_clearprompt(d,l->instance, c->callid);
		if (d->earlyrtp == SCCP_CHANNELSTATE_DIALING && !c->rtp) {
			sccp_channel_openreceivechannel(c);
		}
		sccp_cw_setstate(c, CW_STATE_DIALING);
		break;
	}
	sccp_log(10)(VERBOSE_PREFIX_3 "%s: Finish to indicate state SCCP (%s), SKINNY (%s) on call %s-%d\n",d->id, sccp_indicate2str(state), sccp_callstate2str(c->callstate), l->name, c->callid);
	if (l->hints) {
		/* privacy stuff */
		if (c->private && state != SCCP_CHANNELSTATE_ONHOOK) {
			return;
		} else {
			sccp_hint_notify(c, NULL);
		}
	}
}

const char * sccp_indicate2str(uint8_t state) {
		switch(state) {
		case SCCP_CHANNELSTATE_DOWN:
				return "Down";
		case SCCP_CHANNELSTATE_OFFHOOK:
				return "OffHook";
		case SCCP_CHANNELSTATE_ONHOOK:
				return "OnHook";
		case SCCP_CHANNELSTATE_RINGOUT:
				return "RingOut";
		case SCCP_CHANNELSTATE_RINGIN:
				return "Ringing";
		case SCCP_CHANNELSTATE_CONNECTED:
				return "Connected";
		case SCCP_CHANNELSTATE_BUSY:
				return "Busy";
		case SCCP_CHANNELSTATE_PROCEED:
				return "Proceed";
		case SCCP_CHANNELSTATE_CONGESTION:
				return "Congestion";
		case SCCP_CHANNELSTATE_HOLD:
				return "Hold";
		case SCCP_CHANNELSTATE_CALLWAITING:
				return "CallWaiting";
		case SCCP_CHANNELSTATE_CALLTRANSFER:
				return "CallTransfer";
		case SCCP_CHANNELSTATE_CALLPARK:
				return "CallPark";
		case SCCP_CHANNELSTATE_CALLREMOTEMULTILINE:
				return "CallRemoteMultiline";
		case SCCP_CHANNELSTATE_INVALIDNUMBER:
				return "InvalidNumber";
		case SCCP_CHANNELSTATE_DIALING:
				return "Dialing";
		default:
				return "Unknown";
		}
}


const char * sccp_callstate2str(uint8_t state) {
		switch(state) {
		case SKINNY_CALLSTATE_OFFHOOK:
				return "OffHook";
		case SKINNY_CALLSTATE_ONHOOK:
				return "OnHook";
		case SKINNY_CALLSTATE_RINGOUT:
				return "RingOut";
		case SKINNY_CALLSTATE_RINGIN:
				return "Ringing";
		case SKINNY_CALLSTATE_CONNECTED:
				return "Connected";
		case SKINNY_CALLSTATE_BUSY:
				return "Busy";
		case SKINNY_CALLSTATE_PROCEED:
				return "Proceed";
		case SKINNY_CALLSTATE_CONGESTION:
				return "Congestion";
		case SKINNY_CALLSTATE_HOLD:
				return "Hold";
		case SKINNY_CALLSTATE_CALLWAITING:
				return "CallWaiting";
		case SKINNY_CALLSTATE_CALLTRANSFER:
				return "CallTransfer";
		case SKINNY_CALLSTATE_CALLPARK:
				return "CallPark";
		case SKINNY_CALLSTATE_CALLREMOTEMULTILINE:
				return "CallRemoteMultiline";
		case SKINNY_CALLSTATE_INVALIDNUMBER:
				return "InvalidNumber";
		default:
				return "Unknown";
		}
}
