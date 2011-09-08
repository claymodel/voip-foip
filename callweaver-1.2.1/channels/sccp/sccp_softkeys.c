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
#include "sccp_utils.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include "sccp_indicate.h"

#include "callweaver/utils.h"
#ifdef CS_SCCP_PICKUP
#include "callweaver/features.h"
#include "callweaver/callerid.h"
#endif
#include "callweaver/devicestate.h"

void sccp_sk_redial(sccp_device_t * d , sccp_line_t * l, sccp_channel_t * c) {
	sccp_log(10)(VERBOSE_PREFIX_3 "%s: Redial Softkey.\n",d->id);

	if (cw_strlen_zero(d->lastNumber)) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: No number to redial\n", d->id);
		return;
	}

	if (c) {
		if (c->state == SCCP_CHANNELSTATE_OFFHOOK) {
			/* we have a offhook channel */
			cw_mutex_lock(&c->lock);
			cw_copy_string(c->dialedNumber, d->lastNumber, sizeof(c->dialedNumber));
			sccp_log(10)(VERBOSE_PREFIX_3 "%s: Get ready to redial number %s\n", d->id, d->lastNumber);
			c->digittimeout = time(0)+1;
			cw_mutex_unlock(&c->lock);
		}
		/* here's a KEYMODE error. nothing to do */
		return;
	}
	if (!l)
		l = d->currentLine;
	sccp_channel_newcall(l, d->lastNumber);
}

void sccp_sk_newcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!l)
		l = d->currentLine;
	sccp_channel_newcall(l, NULL);
}

void sccp_sk_hold(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		sccp_dev_displayprompt(d, 0, 0, "No call to put on hold.",5);
		return;
	}
	sccp_channel_hold(c);
}

void sccp_sk_resume(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: No call to resume. Ignoring\n", d->id);
		return;
	}
	sccp_channel_resume(c);
}

void sccp_sk_transfer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: Transfer when there is no active call\n", d->id);
		return;
	}
	sccp_channel_transfer(c);

}

void sccp_sk_endcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: Endcall with no call in progress\n", d->id);
		return;
	}
	sccp_channel_endcall(c);
}


void sccp_sk_dnd(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	/* actually the line param is not used */
	sccp_line_t * l1 = NULL;
	
	if (!d->dndmode) {
		sccp_dev_displayprompt(d, 0, 0, SKINNY_DISP_DND " " SKINNY_DISP_SERVICE_IS_NOT_ACTIVE, 10);
		return;
	}
	
	cw_mutex_lock(&d->lock);
	d->dnd = (d->dnd) ? 0 : 1;
	if (d->dndmode == SCCP_DNDMODE_REJECT) {
		l1 = d->lines;
		while (l1) {
			sccp_log(10)(VERBOSE_PREFIX_3 "%s: Notify the dnd status (%s) to callweaver for line %s\n", d->id, d->dnd ? "on" : "off", l1->name);
			if (d->dnd)
 				sccp_hint_notify_linestate(l1, SCCP_DEVICESTATE_DND, NULL);
 			else
	 			sccp_hint_notify_linestate(l1, SCCP_DEVICESTATE_ONHOOK, NULL);
			l1 = l1->next_on_device;
		}
	}
	cw_mutex_unlock(&d->lock);
	sccp_dev_dbput(d);
	sccp_dev_check_displayprompt(d);
}


void sccp_sk_backspace(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	int len;
	if (!c)
		return;
	if (c->state != SCCP_CHANNELSTATE_DIALING)
		return;
	cw_mutex_lock(&c->lock);
	len = strlen(c->dialedNumber)-1;
	if (len >= 0)
		c->dialedNumber[len] = '\0';
	sccp_log(10)(VERBOSE_PREFIX_3 "%s: backspacing dial number %s\n", c->device->id, c->dialedNumber);
	cw_mutex_unlock(&c->lock);
}

void sccp_sk_answer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	sccp_channel_answer(c);
}

void sccp_sk_dirtrfr(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
/*	sccp_channel_dirtrfr(c); */
}

void sccp_sk_cfwdall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c || !c->owner) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: Call forward with no channel active\n", d->id);
		return;
	}
	if (c->state != SCCP_CHANNELSTATE_RINGOUT && c->state != SCCP_CHANNELSTATE_CONNECTED) {
		sccp_line_cfwd(l, SCCP_CFWD_NONE, NULL);
		return;
	}
	sccp_line_cfwd(l, SCCP_CFWD_ALL, c->dialedNumber);
}

void sccp_sk_cfwdbusy(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c || !c->owner) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: Call forward with no channel active\n", d->id);
		return;
	}
	if (c->state != SCCP_CHANNELSTATE_RINGOUT && c->state != SCCP_CHANNELSTATE_CONNECTED) {
		sccp_line_cfwd(l, SCCP_CFWD_NONE, NULL);
		return;
	}
	sccp_line_cfwd(l, SCCP_CFWD_BUSY, c->dialedNumber);

}

void sccp_sk_cfwdnoanswer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	sccp_log(10)(VERBOSE_PREFIX_3 "### CFwdNoAnswer Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_park(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
#ifdef CS_SCCP_PARK
	sccp_channel_park(c);
#else
	sccp_log(10)(VERBOSE_PREFIX_3 "### Native park was not compiled in\n");
#endif
}

void sccp_sk_trnsfvm(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!l->trnsfvm) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: TRANSVM pressed but not configured in sccp.conf\n", d->id);
		return;
	}
	if (!c || !c->owner) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: TRANSVM with no channel active\n", d->id);
		return;
	}
	if (c->state != SCCP_CHANNELSTATE_RINGIN && c->state != SCCP_CHANNELSTATE_CALLWAITING) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: TRANSVM pressed in no ringing state\n", d->id);
		return;
	}

	sccp_log(1)(VERBOSE_PREFIX_3 "%s: TRANSVM to %s\n", d->id, l->trnsfvm);
	cw_copy_string(c->owner->call_forward, l->trnsfvm, sizeof(c->owner->call_forward));
	cw_setstate(c->owner, CW_STATE_BUSY);
	cw_queue_control(c->owner, CW_CONTROL_BUSY);
}

void sccp_sk_private(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!d->private) {
		sccp_log(1)(VERBOSE_PREFIX_3 "%s: private function is not active on this device\n", d->id);
		return;
	}
	cw_mutex_lock(&c->lock);
	c->private = (c->private) ? 0 : 1;
	sccp_dev_displayprompt(d, c->line->instance, c->callid, SKINNY_DISP_PRIVATE, 0);
	sccp_log(1)(VERBOSE_PREFIX_3 "%s: Private %s on call %d\n", d->id, c->private ? "enabled" : "disabled", c->callid);
	cw_mutex_unlock(&c->lock);
}

void sccp_sk_gpickup(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
#ifndef CS_SCCP_PICKUP
	sccp_log(10)(VERBOSE_PREFIX_3 "### Native PICKUP was not compiled in\n");
#else
	struct cw_channel *ast, *original = NULL;

	if (!l)
		l = d->currentLine;
	if (!l)
		l = d->lines;
	if (!l)
		return;

	if (!l->pickupgroup) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: pickupgroup not configured in sccp.conf\n", d->id);
		return;
	}
	c = sccp_channel_find_bystate_on_line(l, SCCP_CHANNELSTATE_OFFHOOK);
	if (!c)
		c = sccp_channel_newcall(l, NULL);
	if (!c) {
			cw_log(LOG_ERROR, "%s: Can't allocate SCCP channel for line %s\n",d->id, l->name);
			return;
	}

	sccp_log(10)(VERBOSE_PREFIX_3 "%s: Starting the PICKUP stuff\n", d->id);
	/* convert the outgoing call in a incoming call */
	/* let's the startchannel thread go down */
	ast = c->owner;
	if (!ast) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: error: no channel to handle\n", d->id);
		/* let the channel goes down to the invalid number */
		return;
	}

	if (cw_pickup_call(ast)) {
		sccp_log(10)(VERBOSE_PREFIX_3 "%s: pickup error\n", d->id);
		/* let the channel goes down to the invalid number */
		return;
	}

	if (ast) {
		cw_mutex_lock(&ast->lock);
		original = ast->masqr;
		cw_mutex_unlock(&ast->lock);
	}

	sccp_log(10)(VERBOSE_PREFIX_3 "%s: Pickup the call from %s\n", d->id, original->name);
	
	/* need the lock for the pbx_startchannel */
	if (ast->pbx)
		cw_queue_hangup(ast);
	else
		cw_hangup(ast);

	if (original) {
		sccp_channel_set_callingparty(c, original->cid.cid_name, original->cid.cid_num);
	}
	cw_mutex_lock(&c->lock);
	c->calltype = SKINNY_CALLTYPE_INBOUND;
	sccp_indicate_nolock(c, SCCP_CHANNELSTATE_CONNECTED);
	cw_mutex_unlock(&c->lock);
#endif
}
