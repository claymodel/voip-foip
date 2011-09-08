/*
 * vISDN channel driver for CallWeaver
 *
 * Copyright (C) 2004-2005 Daniele Orlandi
 *
 * Authors: Daniele "Vihai" Orlandi <daniele@orlandi.com>
 *
 * This program is free software and may be modified and distributed
 * under the terms and conditions of the GNU General Public License.
 *
 */

#ifdef HAVE_CONFIG_H
 #include "confdefs.h"
#endif

#include <libq931/list.h>

#include "callweaver/channel.h"

static const char visdn_channeltype[] = "VISDN";
static const char visdn_description[] = "VISDN Channel Driver for CallWeaver.org";

struct visdn_suspended_call
{
	struct list_head node;

	struct cw_channel *cw_chan;
	struct q931_channel *q931_chan;

	char call_identity[10];
	int call_identity_len;

	time_t old_when_to_hangup;
};

struct visdn_chan {
	struct cw_channel *cw_chan;
	struct q931_call *q931_call;
	struct visdn_suspended_call *suspended_call;

	char visdn_chanid[30];
	int is_voice;
	int channel_fd;

	char calling_number[21];
	int sending_complete;

	int may_send_digits;
	char queued_digits[21];
};

static int visdn_call(struct cw_channel*, char *, int);
static struct cw_frame *visdn_exception(struct cw_channel *);
static int visdn_hangup(struct cw_channel*);
static int visdn_answer(struct cw_channel*);
static struct cw_frame *visdn_read(struct cw_channel*);
static int visdn_write(struct cw_channel*, struct cw_frame *);
static int visdn_indicate(struct cw_channel*, int);
static int visdn_transfer(struct cw_channel*, const char *);
static int visdn_fixup(struct cw_channel*, struct cw_channel *);
static int visdn_send_digit(struct cw_channel*, char);
static int visdn_sendtext(struct cw_channel*, const char *);
static int visdn_bridge(struct cw_channel*, struct cw_channel*, int, struct cw_frame **, struct cw_channel **, int);
static int visdn_setoption(struct cw_channel*, int, void *, int);
static struct cw_channel *visdn_request(const char *, int, void *, int *);

static const struct cw_channel_tech visdn_tech = {
	.type = visdn_channeltype,
	.description = visdn_description,
	.exception = visdn_exception,
	.call = visdn_call,
	.hangup = visdn_hangup,
	.answer = visdn_answer,
	.read = visdn_read,
	.write = visdn_write,
	.indicate = visdn_indicate,
	.transfer = visdn_transfer,
	.fixup = visdn_fixup,
	.send_digit = visdn_send_digit,
	.send_text = visdn_sendtext,
	.bridge = visdn_bridge,
	.setoption = visdn_setoption,
	.capabilities = CW_FORMAT_ALAW,
	.requester = visdn_request,
};

