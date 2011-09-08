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
 *
 * \brief Channel Management
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
#include <spandsp.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/channel.c $", "$Revision: 5390 $")

#include "callweaver/pbx.h"
#include "callweaver/frame.h"
#include "callweaver/sched.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/musiconhold.h"
#include "callweaver/logger.h"
#include "callweaver/say.h"
#include "callweaver/file.h"
#include "callweaver/cli.h"
#include "callweaver/translate.h"
#include "callweaver/manager.h"
#include "callweaver/chanvars.h"
#include "callweaver/linkedlists.h"
#include "callweaver/indications.h"
#include "callweaver/monitor.h"
#include "callweaver/causes.h"
#include "callweaver/phone_no_utils.h"
#include "callweaver/utils.h"
#include "callweaver/lock.h"
#include "callweaver/app.h"
#include "callweaver/transcap.h"
#include "callweaver/devicestate.h"

/* uncomment if you have problems with 'monitoring' synchronized files */
#if 0
#define MONITOR_CONSTANT_DELAY
#define MONITOR_DELAY	150 * 8		/* 150 ms of MONITORING DELAY */
#endif

/*
 * Prevent new channel allocation if shutting down.
 */
static int shutting_down = 0;

static int uniqueint = 0;

unsigned long global_fin = 0, global_fout = 0;

/* XXX Lock appropriately in more functions XXX */

struct chanlist {
	const struct cw_channel_tech *tech;
	struct chanlist *next;
};

static struct chanlist *backends = NULL;

/*
 * the list of channels we have
 */
static struct cw_channel *channels = NULL;

/* Protect the channel list, both backends and channels.
 */
CW_MUTEX_DEFINE_STATIC(chlock);

const struct cw_cause
{
	int cause;
	const char *desc;
} causes[] =
{
	{ CW_CAUSE_UNALLOCATED, "Unallocated (unassigned) number" },
	{ CW_CAUSE_NO_ROUTE_TRANSIT_NET, "No route to specified transmit network" },
	{ CW_CAUSE_NO_ROUTE_DESTINATION, "No route to destination" },
	{ CW_CAUSE_CHANNEL_UNACCEPTABLE, "Channel unacceptable" },
	{ CW_CAUSE_CALL_AWARDED_DELIVERED, "Call awarded and being delivered in an established channel" },
	{ CW_CAUSE_NORMAL_CLEARING, "Normal Clearing" },
	{ CW_CAUSE_USER_BUSY, "User busy" },
	{ CW_CAUSE_NO_USER_RESPONSE, "No user responding" },
	{ CW_CAUSE_NO_ANSWER, "User alerting, no answer" },
	{ CW_CAUSE_CALL_REJECTED, "Call Rejected" },
	{ CW_CAUSE_NUMBER_CHANGED, "Number changed" },
	{ CW_CAUSE_DESTINATION_OUT_OF_ORDER, "Destination out of order" },
	{ CW_CAUSE_INVALID_NUMBER_FORMAT, "Invalid number format" },
	{ CW_CAUSE_FACILITY_REJECTED, "Facility rejected" },
	{ CW_CAUSE_RESPONSE_TO_STATUS_ENQUIRY, "Response to STATus ENQuiry" },
	{ CW_CAUSE_NORMAL_UNSPECIFIED, "Normal, unspecified" },
	{ CW_CAUSE_NORMAL_CIRCUIT_CONGESTION, "Circuit/channel congestion" },
	{ CW_CAUSE_NETWORK_OUT_OF_ORDER, "Network out of order" },
	{ CW_CAUSE_NORMAL_TEMPORARY_FAILURE, "Temporary failure" },
	{ CW_CAUSE_SWITCH_CONGESTION, "Switching equipment congestion" },
	{ CW_CAUSE_ACCESS_INFO_DISCARDED, "Access information discarded" },
	{ CW_CAUSE_REQUESTED_CHAN_UNAVAIL, "Requested channel not available" },
	{ CW_CAUSE_PRE_EMPTED, "Pre-empted" },
	{ CW_CAUSE_FACILITY_NOT_SUBSCRIBED, "Facility not subscribed" },
	{ CW_CAUSE_OUTGOING_CALL_BARRED, "Outgoing call barred" },
	{ CW_CAUSE_INCOMING_CALL_BARRED, "Incoming call barred" },
	{ CW_CAUSE_BEARERCAPABILITY_NOTAUTH, "Bearer capability not authorized" },
	{ CW_CAUSE_BEARERCAPABILITY_NOTAVAIL, "Bearer capability not available" },
	{ CW_CAUSE_BEARERCAPABILITY_NOTIMPL, "Bearer capability not implemented" },
	{ CW_CAUSE_CHAN_NOT_IMPLEMENTED, "Channel not implemented" },
	{ CW_CAUSE_FACILITY_NOT_IMPLEMENTED, "Facility not implemented" },
	{ CW_CAUSE_INVALID_CALL_REFERENCE, "Invalid call reference value" },
	{ CW_CAUSE_INCOMPATIBLE_DESTINATION, "Incompatible destination" },
	{ CW_CAUSE_INVALID_MSG_UNSPECIFIED, "Invalid message unspecified" },
	{ CW_CAUSE_MANDATORY_IE_MISSING, "Mandatory information element is missing" },
	{ CW_CAUSE_MESSAGE_TYPE_NONEXIST, "Message type nonexist." },
	{ CW_CAUSE_WRONG_MESSAGE, "Wrong message" },
	{ CW_CAUSE_IE_NONEXIST, "Info. element nonexist or not implemented" },
	{ CW_CAUSE_INVALID_IE_CONTENTS, "Invalid information element contents" },
	{ CW_CAUSE_WRONG_CALL_STATE, "Message not compatible with call state" },
	{ CW_CAUSE_RECOVERY_ON_TIMER_EXPIRE, "Recover on timer expiry" },
	{ CW_CAUSE_MANDATORY_IE_LENGTH_ERROR, "Mandatory IE length error" },
	{ CW_CAUSE_PROTOCOL_ERROR, "Protocol error, unspecified" },
	{ CW_CAUSE_INTERWORKING, "Interworking, unspecified" },
};

/* Control frame types */
const struct cw_control
{
	int control;
	const char *desc;
} controles[] =
{
	{CW_CONTROL_HANGUP, "Other end has hungup"},
	{CW_CONTROL_RING, "Local ring"},
	{CW_CONTROL_RINGING, "Remote end is ringing"},
	{CW_CONTROL_ANSWER,"Remote end has answered"},
	{CW_CONTROL_BUSY, "Remote end is busy"},
	{CW_CONTROL_TAKEOFFHOOK, "Make it go off hook"},
	{CW_CONTROL_OFFHOOK, "Line is off hook"},
	{CW_CONTROL_CONGESTION, "Congestion (circuits busy"},
	{CW_CONTROL_FLASH, "Flash hook"},
	{CW_CONTROL_WINK, "Wink"},
	{CW_CONTROL_OPTION, "Set a low-level option"},
	{CW_CONTROL_RADIO_KEY, "Key Radio"},
	{CW_CONTROL_RADIO_UNKEY, "Un-Key Radio"},
	{CW_CONTROL_PROGRESS, "Indicate PROGRESS"},
	{CW_CONTROL_PROCEEDING, "Indicate CALL PROCEEDING"},
	{CW_CONTROL_HOLD, "Indicate call is placed on hold"},
	{CW_CONTROL_UNHOLD, "Indicate call is left from hold"},
	{CW_CONTROL_VIDUPDATE, "Indicate video frame update"},
};

/* this code is broken - if someone knows how to rewrite the list traversal, please tell */
struct cw_variable *cw_channeltype_list(void)
{
#if 0
	struct chanlist *cl;
	struct cw_variable *var = NULL;
	struct cw_variable *prev = NULL;
#endif

	cw_log(LOG_WARNING, "cw_channeltype_list() called (probably by res_snmp.so). This is not implemented yet.\n");
	return NULL;

/*	CW_LIST_TRAVERSE(&backends, cl, list) {  <-- original line from asterisk */

#if 0
	CW_LIST_TRAVERSE(&backends, cl, next)
    {
		if (prev)
        {
			if ((prev->next = cw_variable_new(cl->tech->type, cl->tech->description)))
				prev = prev->next;
		}
        else
        {
			var = cw_variable_new(cl->tech->type, cl->tech->description);
			prev = var;
		}
	}
	return var;
#endif
}

static int show_channeltypes(int fd, int argc, char *argv[])
{
#define FORMAT  "%-10.10s  %-30.30s %-12.12s %-12.12s %-12.12s\n"
	struct chanlist *cl;
	cw_cli(fd, FORMAT, "Type", "Description",       "Devicestate", "Indications", "Transfer");
	cw_cli(fd, FORMAT, "----------", "-----------", "-----------", "-----------", "--------");
	if (cw_mutex_lock(&chlock)) {
		cw_log(LOG_WARNING, "Unable to lock channel list\n");
		return -1;
	}
	cl = backends;
	while (cl) {
		cw_cli(fd, FORMAT, cl->tech->type, cl->tech->description, 
			(cl->tech->devicestate) ? "yes" : "no", 
			(cl->tech->indicate) ? "yes" : "no",
			(cl->tech->transfer) ? "yes" : "no");
		cl = cl->next;
	}
	cw_mutex_unlock(&chlock);
	return RESULT_SUCCESS;

#undef FORMAT

}

static char show_channeltypes_usage[] = 
"Usage: show channeltypes\n"
"       Shows available channel types registered in your CallWeaver server.\n";

static struct cw_cli_entry cli_show_channeltypes = 
	{ { "show", "channeltypes", NULL }, show_channeltypes, "Show available channel types", show_channeltypes_usage };

/*--- cw_check_hangup: Checks to see if a channel is needing hang up */
int cw_check_hangup(struct cw_channel *chan)
{
	time_t	myt;

	/* if soft hangup flag, return true */
	if (chan->_softhangup) 
		return 1;
	/* if no technology private data, return true */
	if (!chan->tech_pvt) 
		return 1;
	/* if no hangup scheduled, just return here */
	if (!chan->whentohangup) 
		return 0;
	time(&myt); /* get current time */
	/* return, if not yet */
	if (chan->whentohangup > myt) 
		return 0;
	chan->_softhangup |= CW_SOFTHANGUP_TIMEOUT;
	return 1;
}

static int cw_check_hangup_locked(struct cw_channel *chan)
{
	int res;
	cw_mutex_lock(&chan->lock);
	res = cw_check_hangup(chan);
	cw_mutex_unlock(&chan->lock);
	return res;
}

/*--- cw_begin_shutdown: Initiate system shutdown */
void cw_begin_shutdown(int hangup)
{
	struct cw_channel *c;
	
    shutting_down = 1;
	if (hangup)
    {
		cw_mutex_lock(&chlock);
		c = channels;
		while (c)
        {
			cw_softhangup(c, CW_SOFTHANGUP_SHUTDOWN);
			c = c->next;
		}
		cw_mutex_unlock(&chlock);
	}
}

/*--- cw_active_channels: returns number of active/allocated channels */
int cw_active_channels(void)
{
	struct cw_channel *c;
	int cnt = 0;
	
    cw_mutex_lock(&chlock);
	c = channels;
	while (c)
    {
		cnt++;
		c = c->next;
	}
	cw_mutex_unlock(&chlock);
	return cnt;
}

/*--- cw_cancel_shutdown: Cancel a shutdown in progress */
void cw_cancel_shutdown(void)
{
	shutting_down = 0;
}

/*--- cw_shutting_down: Returns non-zero if CallWeaver is being shut down */
int cw_shutting_down(void)
{
	return shutting_down;
}

/*--- cw_channel_setwhentohangup: Set when to hangup channel */
void cw_channel_setwhentohangup(struct cw_channel *chan, time_t offset)
{
	time_t	myt;
	struct cw_frame fr = { CW_FRAME_NULL, };

	time(&myt);
	if (offset)
		chan->whentohangup = myt + offset;
	else
		chan->whentohangup = 0;
	cw_queue_frame(chan, &fr);
	return;
}
/*--- cw_channel_cmpwhentohangup: Compare a offset with when to hangup channel */
int cw_channel_cmpwhentohangup(struct cw_channel *chan, time_t offset)
{
	time_t whentohangup;

	if (chan->whentohangup == 0) {
		if (offset == 0)
			return (0);
		else
			return (-1);
	} else { 
		if (offset == 0)
			return (1);
		else {
			whentohangup = offset + time (NULL);
			if (chan->whentohangup < whentohangup)
				return (1);
			else if (chan->whentohangup == whentohangup)
				return (0);
			else
				return (-1);
		}
	}
}

/*--- cw_channel_register: Register a new telephony channel in CallWeaver */
int cw_channel_register(const struct cw_channel_tech *tech)
{
	struct chanlist *chan;

	cw_mutex_lock(&chlock);

	chan = backends;
	while (chan) {
		if (!strcasecmp(tech->type, chan->tech->type)) {
			cw_log(LOG_WARNING, "Already have a handler for type '%s'\n", tech->type);
			cw_mutex_unlock(&chlock);
			return -1;
		}
		chan = chan->next;
	}

	chan = malloc(sizeof(*chan));
	if (!chan) {
		cw_log(LOG_WARNING, "Out of memory\n");
		cw_mutex_unlock(&chlock);
		return -1;
	}
	chan->tech = tech;
	chan->next = backends;
	backends = chan;

	if (option_debug)
		cw_log(LOG_DEBUG, "Registered handler for '%s' (%s)\n", chan->tech->type, chan->tech->description);

	if (option_verbose > 1)
		cw_verbose(VERBOSE_PREFIX_2 "Registered channel type '%s' (%s)\n", chan->tech->type,
			    chan->tech->description);

	cw_mutex_unlock(&chlock);
	return 0;
}

void cw_channel_unregister(const struct cw_channel_tech *tech)
{
	struct chanlist *chan, *last=NULL;

	if (option_debug)
		cw_log(LOG_DEBUG, "Unregistering channel type '%s'\n", tech->type);

	cw_mutex_lock(&chlock);

	chan = backends;
	while (chan) {
		if (chan->tech == tech) {
			if (last)
				last->next = chan->next;
			else
				backends = backends->next;
			free(chan);
			cw_mutex_unlock(&chlock);

			if (option_verbose > 1)
				cw_verbose( VERBOSE_PREFIX_2 "Unregistered channel type '%s'\n", tech->type);

			return;
		}
		last = chan;
		chan = chan->next;
	}

	cw_mutex_unlock(&chlock);
}

const struct cw_channel_tech *cw_get_channel_tech(const char *name)
{
	struct chanlist *chanls;

	if (cw_mutex_lock(&chlock)) {
		cw_log(LOG_WARNING, "Unable to lock channel tech list\n");
		return NULL;
	}

	for (chanls = backends; chanls; chanls = chanls->next) {
		if (strcasecmp(name, chanls->tech->type))
			continue;

		cw_mutex_unlock(&chlock);
		return chanls->tech;
	}

	cw_mutex_unlock(&chlock);
	return NULL;
}

/*--- cw_cause2str: Gives the string form of a given hangup cause */
const char *cw_cause2str(int cause)
{
	int x;

	for (x=0; x < sizeof(causes) / sizeof(causes[0]); x++) 
		if (causes[x].cause == cause)
			return causes[x].desc;

	return "Unknown";
}

/*--- cw_control2str: Gives the string form of a given control frame */
const char *cw_control2str(int control)
{
	int x;

	for (x=0; x < sizeof(controles) / sizeof(controles[0]); x++) 
		if (controles[x].control == control)
			return controles[x].desc;

	return "Unknown";
}

/*--- cw_state2str: Gives the string form of a given channel state */
char *cw_state2str(int state)
{
	/* XXX Not reentrant XXX */
	static char localtmp[256];
	switch(state) {
	case CW_STATE_DOWN:
		return "Down";
	case CW_STATE_RESERVED:
		return "Rsrvd";
	case CW_STATE_OFFHOOK:
		return "OffHook";
	case CW_STATE_DIALING:
		return "Dialing";
	case CW_STATE_RING:
		return "Ring";
	case CW_STATE_RINGING:
		return "Ringing";
	case CW_STATE_UP:
		return "Up";
	case CW_STATE_BUSY:
		return "Busy";
	default:
		snprintf(localtmp, sizeof(localtmp), "Unknown (%d)\n", state);
		return localtmp;
	}
}

/*--- cw_transfercapability2str: Gives the string form of a given transfer capability */
char *cw_transfercapability2str(int transfercapability)
{
	switch(transfercapability) {
	case CW_TRANS_CAP_SPEECH:
		return "SPEECH";
	case CW_TRANS_CAP_DIGITAL:
		return "DIGITAL";
	case CW_TRANS_CAP_RESTRICTED_DIGITAL:
		return "RESTRICTED_DIGITAL";
	case CW_TRANS_CAP_3_1K_AUDIO:
		return "3K1AUDIO";
	case CW_TRANS_CAP_DIGITAL_W_TONES:
		return "DIGITAL_W_TONES";
	case CW_TRANS_CAP_VIDEO:
		return "VIDEO";
	default:
		return "UNKNOWN";
	}
}

/*--- cw_best_codec: Pick the best codec */
int cw_best_codec(int fmts)
{
	/* This just our opinion, expressed in code.  We are asked to choose
	   the best codec to use, given no information */
	int x;
	static const int prefs[] = 
	{
		/* Okay, ulaw is used by all telephony equipment, so start with it */
		CW_FORMAT_ULAW,
		/* Unless of course, you're a European, so then prefer ALAW */
		CW_FORMAT_ALAW,
		/* Okay, well, signed linear is easy to translate into other stuff */
		CW_FORMAT_SLINEAR,
		/* G.726 is standard ADPCM */
		CW_FORMAT_G726,
		/* ADPCM has great sound quality and is still pretty easy to translate */
		CW_FORMAT_DVI_ADPCM,
		/* Okay, we're down to vocoders now, so pick GSM because it's small and easier to
		   translate and sounds pretty good */
		CW_FORMAT_GSM,
		/* iLBC is not too bad */
		CW_FORMAT_ILBC,
		/* Speex is free, but computationally more expensive than GSM */
		CW_FORMAT_SPEEX,
		/* Ick, LPC10 sounds terrible, but at least we have code for it, if you're tacky enough
		   to use it */
		CW_FORMAT_LPC10,
		/* G.729a is faster than 723 and slightly less expensive */
		CW_FORMAT_G729A,
		/* Down to G.723.1 which is proprietary but at least designed for voice */
		CW_FORMAT_G723_1,
        CW_FORMAT_OKI_ADPCM,
	};

	/* Find the first prefered codec in the format given */
	for (x = 0;  x < (sizeof(prefs)/sizeof(prefs[0]));  x++)
	{
		if (fmts & prefs[x])
			return prefs[x];
	}
	cw_log(LOG_WARNING, "Don't know any of 0x%x formats\n", fmts);
	return 0;
}

static const struct cw_channel_tech null_tech =
{
	.type = "NULL",
	.description = "Null channel (should not see this)",
};

/*--- cw_channel_alloc: Create a new channel structure */
struct cw_channel *cw_channel_alloc(int needqueue)
{
	struct cw_channel *tmp;
	int x;
	int flags;
	struct varshead *headp;        
	        
	/* If shutting down, don't allocate any new channels */
	if (shutting_down)
        {
		cw_log(LOG_NOTICE, "Refusing channel allocation due to active shutdown\n");
		return NULL;
	}

	if ((tmp = malloc(sizeof(*tmp))) == NULL)
	{
		cw_log(LOG_ERROR, "Channel allocation failed: Out of memory\n");
		return NULL;
	}
	memset(tmp, 0, sizeof(*tmp));

	if ((tmp->sched = sched_manual_context_create()) == NULL)
        {
		cw_log(LOG_ERROR, "Channel allocation failed: Unable to create schedule context\n");
		free(tmp);
		return NULL;
	}
	
	for (x = 0;  x < CW_MAX_FDS - 1;  x++)
		tmp->fds[x] = -1;

	if (needqueue)
        {
		if (pipe(tmp->alertpipe))
                {
			cw_log(LOG_WARNING, "Channel allocation failed: Can't create alert pipe!\n");
			free(tmp);
			return NULL;
		}
    	flags = fcntl(tmp->alertpipe[0], F_GETFL);
		fcntl(tmp->alertpipe[0], F_SETFL, flags | O_NONBLOCK);
		flags = fcntl(tmp->alertpipe[1], F_GETFL);
		fcntl(tmp->alertpipe[1], F_SETFL, flags | O_NONBLOCK);
	}
        else 
	{
    	/* Make sure we've got it done right if they don't */
		tmp->alertpipe[0] = tmp->alertpipe[1] = -1;
        }
	/* Init channel generator data struct lock */
	cw_mutex_init(&tmp->gcd.lock);

	/* Always watch the alertpipe */
	tmp->fds[CW_MAX_FDS-1] = tmp->alertpipe[0];
	strcpy(tmp->name, "**Unknown**");
	/* Initial state */
	tmp->_state = CW_STATE_DOWN;
	tmp->appl = NULL;
	tmp->fin = global_fin;
	tmp->fout = global_fout;
//	snprintf(tmp->uniqueid, sizeof(tmp->uniqueid), "%li.%d", (long) time(NULL), uniqueint++); 
	if (cw_strlen_zero(cw_config_CW_SYSTEM_NAME))
		 snprintf(tmp->uniqueid, sizeof(tmp->uniqueid), "%li.%d", (long) time(NULL), uniqueint++);
	else
		 snprintf(tmp->uniqueid, sizeof(tmp->uniqueid), "%s-%li.%d", cw_config_CW_SYSTEM_NAME, (long) time(NULL), uniqueint++);
	headp = &tmp->varshead;
	cw_mutex_init(&tmp->lock);
	CW_LIST_HEAD_INIT_NOLOCK(headp);
	strcpy(tmp->context, "default");
	cw_copy_string(tmp->language, defaultlanguage, sizeof(tmp->language));
	strcpy(tmp->exten, "s");
	tmp->priority = 1;
	tmp->amaflags = cw_default_amaflags;
	cw_copy_string(tmp->accountcode, cw_default_accountcode, sizeof(tmp->accountcode));

	tmp->tech = &null_tech;
	tmp->t38_status = T38_STATUS_UNKNOWN;

        tmp->gen_samples = 160;
        tmp->samples_per_second = 8000;

	cw_mutex_lock(&chlock);
	tmp->next = channels;
	channels = tmp;

	cw_mutex_unlock(&chlock);
	return tmp;
}

/* Sets the channel codec samples per seconds */
void cw_channel_set_samples_per_second( struct cw_channel *tmp, int sps )
{
    //TODO should we check for sps to be correct ? (ie 8000, 16000 etccc)
    //TODO: Should we stop the generator and reactivate it or simply check that it's not running ?
    tmp->samples_per_second = sps;
}

/* Sets the channel generator samples per iteration */
void cw_channel_set_generator_samples( struct cw_channel *tmp, int samp )
{
    //TODO: Should we stop the generator and reactivate it or simply check that it's not running ?
    tmp->gen_samples = samp;
}

/* Sets the channel t38 status */
void cw_channel_perform_set_t38_status( struct cw_channel *tmp, t38_status_t status, const char *file, int line )
{
    if ( !tmp ) {
	cw_log(LOG_WARNING,"cw_channel_set_t38_status called with NULL channel at %s:%d\n", file, line);
	return;
    }
    if (option_debug > 5)
        cw_log(LOG_DEBUG,"Setting t38 status to %d for %s at %s:%d\n", status, tmp->name, file, line);
    tmp->t38_status = status;
}

/* Gets the channel t38 status */
t38_status_t cw_channel_get_t38_status( struct cw_channel *tmp ) 
{
    if ( !tmp )
	return T38_STATUS_UNKNOWN;
    else
	return tmp->t38_status;
}



/*--- cw_queue_frame: Queue an outgoing media frame */
int cw_queue_frame(struct cw_channel *chan, struct cw_frame *fin)
{
	struct cw_frame *f;
	struct cw_frame *prev, *cur;
	int blah = 1;
	int qlen = 0;

	/* Build us a copy and free the original one */
	if ((f = cw_frdup(fin)) == NULL)
	{
		cw_log(LOG_WARNING, "Unable to duplicate frame\n");
		return -1;
	}
	cw_mutex_lock(&chan->lock);
	prev = NULL;
	for (cur = chan->readq;  cur;  cur = cur->next)
	{
		if ((cur->frametype == CW_FRAME_CONTROL) && (cur->subclass == CW_CONTROL_HANGUP))
    		{
			/* Don't bother actually queueing anything after a hangup */
			cw_fr_free(f);
			cw_mutex_unlock(&chan->lock);
			return 0;
		}
		prev = cur;
		qlen++;
	}
	/* Allow up to 96 voice frames outstanding, and up to 128 total frames */
	if (((fin->frametype == CW_FRAME_VOICE) && (qlen > 96)) || (qlen  > 128))
	{
		if (fin->frametype != CW_FRAME_VOICE)
    		{
			cw_log(LOG_ERROR, "Dropping non-voice (type %d) frame for %s due to long queue length\n", fin->frametype, chan->name);
		}
    		else
    		{	
			cw_log(LOG_WARNING, "Dropping voice frame for %s due to exceptionally long queue\n", chan->name);
		}
		cw_fr_free(f);
		cw_mutex_unlock(&chan->lock);
		return 0;
	}
	if (prev)
		prev->next = f;
	else
		chan->readq = f;

	if (chan->alertpipe[1] > -1)
	{
	    if (write(chan->alertpipe[1], &blah, sizeof(blah)) != sizeof(blah))
		cw_log(LOG_WARNING, 
			    "Unable to write to alert pipe on %s, frametype/subclass %d/%d (qlen = %d): %s!\n",
			    chan->name, 
			    f->frametype, 
			    f->subclass, 
			    qlen, 
			    strerror(errno)
			);
	} else if (cw_test_flag(chan, CW_FLAG_BLOCKING)) {
		pthread_kill(chan->blocker, SIGURG);
	}
	cw_mutex_unlock(&chan->lock);
	return 0;
}

/*--- cw_queue_hangup: Queue a hangup frame for channel */
int cw_queue_hangup(struct cw_channel *chan)
{
	struct cw_frame f = { CW_FRAME_CONTROL, CW_CONTROL_HANGUP };
	/* Yeah, let's not change a lock-critical value without locking */
	if (!cw_mutex_trylock(&chan->lock)) {	
		chan->_softhangup |= CW_SOFTHANGUP_DEV;
		cw_mutex_unlock(&chan->lock);
	}
	return cw_queue_frame(chan, &f);
}

/*--- cw_queue_control: Queue a control frame */
int cw_queue_control(struct cw_channel *chan, int control)
{
	struct cw_frame f = { CW_FRAME_CONTROL, };
	f.subclass = control;
	return cw_queue_frame(chan, &f);
}

/*--- cw_channel_defer_dtmf: Set defer DTMF flag on channel */
int cw_channel_defer_dtmf(struct cw_channel *chan)
{
	int pre = 0;

	if (chan) {
		pre = cw_test_flag(chan, CW_FLAG_DEFER_DTMF);
		cw_set_flag(chan, CW_FLAG_DEFER_DTMF);
	}
	return pre;
}

/*--- cw_channel_undefer_dtmf: Unset defer DTMF flag on channel */
void cw_channel_undefer_dtmf(struct cw_channel *chan)
{
	if (chan)
		cw_clear_flag(chan, CW_FLAG_DEFER_DTMF);
}

/*
 * Helper function to find channels. It supports these modes:
 *
 * prev != NULL : get channel next in list after prev
 * name != NULL : get channel with matching name
 * name != NULL && namelen != 0 : get channel whose name starts with prefix
 * exten != NULL : get channel whose exten or proc_exten matches
 * context != NULL && exten != NULL : get channel whose context or proc_context
 *                                    
 * It returns with the channel's lock held. If getting the individual lock fails,
 * unlock and retry quickly up to 10 times, then give up.
 * 
 * XXX Note that this code has cost O(N) because of the need to verify
 * that the object is still on the global list.
 *
 * XXX also note that accessing fields (e.g. c->name in cw_log())
 * can only be done with the lock held or someone could delete the
 * object while we work on it. This causes some ugliness in the code.
 * Note that removing the first cw_log() may be harmful, as it would
 * shorten the retry period and possibly cause failures.
 * We should definitely go for a better scheme that is deadlock-free.
 */
static struct cw_channel *channel_find_locked(const struct cw_channel *prev,
					       const char *name, const int namelen,
					       const char *context, const char *exten)
{
	const char *msg = prev ? "deadlock" : "initial deadlock";
	int retries, done;
	struct cw_channel *c;

	for (retries = 0; retries < 10; retries++) {
		cw_mutex_lock(&chlock);
		for (c = channels; c; c = c->next) {
			if (!prev) {
				/* want head of list */
				if (!name && !exten)
					break;
				if (name) {
					/* want match by full name */
					if (!namelen) {
						if (!strcasecmp(c->name, name))
							break;
						else
							continue;
					}
					/* want match by name prefix */
					if (!strncasecmp(c->name, name, namelen))
						break;
				} else if (exten) {
					/* want match by context and exten */
					if (context && (strcasecmp(c->context, context) &&
							strcasecmp(c->proc_context, context)))
						continue;
					/* match by exten */
					if (strcasecmp(c->exten, exten) &&
					    strcasecmp(c->proc_exten, exten))
						continue;
					else
						break;
				}
			} else if (c == prev) { /* found, return c->next */
				c = c->next;
				break;
			}
		}
		/* exit if chan not found or mutex acquired successfully */
		done = (c == NULL) || (cw_mutex_trylock(&c->lock) == 0);
		/* this is slightly unsafe, as we _should_ hold the lock to access c->name */
		if (!done && c)
			cw_log(LOG_DEBUG, "Avoiding %s for '%s'\n", msg, c->name);
		cw_mutex_unlock(&chlock);
		if (done)
			return c;
		usleep(1);
	}
	/*
 	 * c is surely not null, but we don't have the lock so cannot
	 * access c->name
	 */
	cw_log(LOG_DEBUG, "Avoided %s for '%p', %d retries!\n",
		msg, c, retries);

	return NULL;
}

/*--- cw_channel_walk_locked: Browse channels in use */
struct cw_channel *cw_channel_walk_locked(const struct cw_channel *prev)
{
	return channel_find_locked(prev, NULL, 0, NULL, NULL);
}

/*--- cw_get_channel_by_name_locked: Get channel by name and lock it */
struct cw_channel *cw_get_channel_by_name_locked(const char *name)
{
	return channel_find_locked(NULL, name, 0, NULL, NULL);
}

/*--- cw_get_channel_by_name_prefix_locked: Get channel by name prefix and lock it */
struct cw_channel *cw_get_channel_by_name_prefix_locked(const char *name, const int namelen)
{
	return channel_find_locked(NULL, name, namelen, NULL, NULL);
}

/*--- cw_walk_channel_by_name_prefix_locked: Get next channel by name prefix and lock it */
struct cw_channel *cw_walk_channel_by_name_prefix_locked(struct cw_channel *chan, const char *name, const int namelen)
{
	return channel_find_locked(chan, name, namelen, NULL, NULL);
}

/*--- cw_get_channel_by_exten_locked: Get channel by exten (and optionally context) and lock it */
struct cw_channel *cw_get_channel_by_exten_locked(const char *exten, const char *context)
{
	return channel_find_locked(NULL, NULL, 0, context, exten);
}

/*--- cw_safe_sleep_conditional: Wait, look for hangups and condition arg */
int cw_safe_sleep_conditional(	struct cw_channel *chan, int ms,
	int (*cond)(void*), void *data )
{
	struct cw_frame *f;

	while (ms > 0)
	{
		if (cond  &&  ((*cond)(data) == 0))
			return 0;
		ms = cw_waitfor(chan, ms);
		if (ms <0)
			return -1;
		if (ms > 0)
    		{
			f = cw_read(chan);
			if (!f)
				return -1;
			cw_fr_free(f);
		}
	}
	return 0;
}

/*--- cw_safe_sleep: Wait, look for hangups */
int cw_safe_sleep(struct cw_channel *chan, int ms)
{
	struct cw_frame *f;
	
	while (ms > 0)
	{
		ms = cw_waitfor(chan, ms);
		if (ms < 0)
			return -1;
		if (ms > 0)
    		{
			f = cw_read(chan);
			if (!f)
				return -1;
			cw_fr_free(f);
		}
	}
	return 0;
}

static void free_cid(struct cw_callerid *cid)
{
	if (cid->cid_dnid)
		free(cid->cid_dnid);
	if (cid->cid_num)
		free(cid->cid_num);	
	if (cid->cid_name)
		free(cid->cid_name);	
	if (cid->cid_ani)
		free(cid->cid_ani);
	if (cid->cid_rdnis)
		free(cid->cid_rdnis);
}

/*--- cw_channel_free: Free a channel structure */
void cw_channel_free(struct cw_channel *chan)
{
	struct cw_channel *last=NULL, *cur;
	int fd;
	struct cw_var_t *vardata;
	struct cw_frame *f, *fp;
	struct varshead *headp;
	char name[CW_CHANNEL_NAME];
	
	headp=&chan->varshead;
	
	cw_mutex_lock(&chlock);
	cur = channels;
	while (cur)
	{
	    	if (cur == chan)
    		{
			if (last)
				last->next = cur->next;
			else
				channels = cur->next;
			break;
		}
		last = cur;
		cur = cur->next;
	}
	if (!cur)
		cw_log(LOG_WARNING, "Unable to find channel in list\n");
	else {
		/* Lock and unlock the channel just to be sure nobody
		   has it locked still */
		cw_mutex_lock(&cur->lock);
		cw_mutex_unlock(&cur->lock);
	}
	if (chan->tech_pvt)
	{
		cw_log(LOG_WARNING, "Channel '%s' may not have been hung up properly\n", chan->name);
		free(chan->tech_pvt);
	}

	cw_copy_string(name, chan->name, sizeof(name));

	/* Stop generator thread */
	cw_generator_deactivate(chan);

	/* Stop monitoring */
	if (chan->monitor)
		chan->monitor->stop(chan, 0);

	/* If there is native format music-on-hold state, free it */
	if (chan->music_state)
		cw_moh_cleanup(chan);

	/* Free translators */
	if (chan->readtrans)
		cw_translator_free_path(chan->readtrans);
	if (chan->writetrans)
		cw_translator_free_path(chan->writetrans);
	if (chan->pbx) 
		cw_log(LOG_WARNING, "PBX may not have been terminated properly on '%s'\n", chan->name);
	free_cid(&chan->cid);
	cw_mutex_destroy(&chan->lock);
	/* Close pipes if appropriate */
	if ((fd = chan->alertpipe[0]) > -1)
		close(fd);
	if ((fd = chan->alertpipe[1]) > -1)
		close(fd);
	f = chan->readq;
	chan->readq = NULL;
	while (f)
	{
		fp = f;
		f = f->next;
		cw_fr_free(fp);
	}
	
	/* loop over the variables list, freeing all data and deleting list items */
	/* no need to lock the list, as the channel is already locked */
	
	while (!CW_LIST_EMPTY(headp))
	{
        /* List Deletion. */
	    vardata = CW_LIST_REMOVE_HEAD(headp, entries);
	    cw_var_delete(vardata);
	    /* Drop out of the group counting radar */
	    cw_app_group_discard(chan);
	}

	/* Destroy the jitterbuffer */
	cw_jb_destroy(chan);

	if (chan->cdr) {
		cw_cdr_free(chan->cdr);
		chan->cdr = NULL;
	}

	free(chan);
	cw_mutex_unlock(&chlock);

	cw_device_state_changed_literal(name);
}

/*--- cw_spy_get_frames: Get as many frames as BOTH parts can give. */
/** Get as many frames as BOTH parts can give.
 * This will make \c f0 point to a list of frames from the first queue and \c f1
 * point to a list of frames from the second queue. Both list will have the same
 * number of elements.
 *
 * After calling this function the queues will have released the specified
 * frames and it's the caller's responsibility to deallocate them.
 *
 * @param spy The (unlocked) spy.
 * @param f0 The start of the frames from the first queue. This can become NULL
 * if no frames were available in either queue.
 * @param f1 The start of the frames from the second queue. This can become NULL
 * if no frames were available in either queue.
 */
void cw_spy_get_frames(struct cw_channel_spy *spy, struct cw_frame **f0, struct cw_frame **f1)
{
    unsigned left, right;
    unsigned same;
    cw_mutex_lock(&spy->lock);
    left = spy->queue[0].count;
    right = spy->queue[1].count;
    same = left < right ? left : right;
    if (same == 0) {
        *f0 = *f1 = NULL;
    } else {
        int ii;
        struct cw_frame *f = spy->queue[0].head;
        for (ii = 1; ii < same; ++ii)
            f = f->next;
        *f0 = spy->queue[0].head;
        spy->queue[0].head = f->next;
        spy->queue[0].count -= same;
        if (spy->queue[0].count == 0)
            spy->queue[0].tail = NULL;
        f->next = NULL;
        f = spy->queue[1].head;
        for (ii = 1; ii < same; ++ii)
            f = f->next;
        *f1 = spy->queue[1].head;
        spy->queue[1].head = f->next;
        spy->queue[1].count -= same;
        if (spy->queue[1].count == 0)
            spy->queue[1].tail = NULL;
        f->next = NULL;
    }
    cw_mutex_unlock(&spy->lock);
}

static void cw_spy_detach(struct cw_channel *chan) 
{
	struct cw_channel_spy *chanspy;

	/* Marking the spies as done is sufficient.  Chanspy or spy users will get the picture. */
	for (chanspy = chan->spiers;  chanspy;  chanspy = chanspy->next)
	{
		if (chanspy->status == CHANSPY_RUNNING)
			chanspy->status = CHANSPY_DONE;
	}
	chan->spiers = NULL;
}

/*--- cw_softhangup_nolock: Softly hangup a channel, don't lock */
int cw_softhangup_nolock(struct cw_channel *chan, int cause)
{
	int res = 0;
	struct cw_frame f = { CW_FRAME_NULL };

	if (option_debug)
		cw_log(LOG_DEBUG, "Soft-Hanging up channel '%s'\n", chan->name);
	/* Inform channel driver that we need to be hung up, if it cares */
	chan->_softhangup |= cause;
	cw_queue_frame(chan, &f);
	/* Interrupt any poll call or such */
	if (cw_test_flag(chan, CW_FLAG_BLOCKING))
		pthread_kill(chan->blocker, SIGURG);
	return res;
}

/*--- cw_softhangup_nolock: Softly hangup a channel, lock */
int cw_softhangup(struct cw_channel *chan, int cause)
{
	int res;

	cw_mutex_lock(&chan->lock);
	res = cw_softhangup_nolock(chan, cause);
	cw_mutex_unlock(&chan->lock);
	return res;
}

/*--- cw_queue_spy_frame: Add a frame to a spy. */
/** Copy a frame for a spy.
 * @param spy Pointer to the (unlocked) spy.
 * @param f Pointer to the frame to duplicate.
 * @param pos 0 for the first queue, 1 for the second.
 */
void cw_queue_spy_frame(struct cw_channel_spy *spy, struct cw_frame *f, int pos)
{
	cw_mutex_lock(&spy->lock);
	if (spy->queue[pos].count > 1000) {
		struct cw_frame *headf = spy->queue[pos].head;
		struct cw_frame *tmpf = headf;
		cw_log(LOG_ERROR, "Too many frames queued at once, flushing cache.\n");
		/* Set the queue as empty and unlock.*/
		spy->queue[pos].head = spy->queue[pos].tail = NULL;
		spy->queue[pos].count = 0;
		cw_mutex_unlock(&spy->lock);
		/* Free the wasted frames */
		while (tmpf) {
			struct cw_frame *freef = tmpf;
			tmpf = tmpf->next;
			cw_fr_free(freef);
		}
		return;
	} else {
		struct cw_frame *tmpf = cw_frdup(f);
		if (!tmpf) {
			cw_log(LOG_WARNING, "Unable to duplicate frame\n");
		} else {
			++spy->queue[pos].count;
			if (spy->queue[pos].tail)
				spy->queue[pos].tail->next = tmpf;
			else
				spy->queue[pos].head = tmpf;
			spy->queue[pos].tail = tmpf;
		}
	}
	cw_mutex_unlock(&spy->lock);
}

static void free_translation(struct cw_channel *clone)
{
	if (clone->writetrans)
		cw_translator_free_path(clone->writetrans);
	if (clone->readtrans)
		cw_translator_free_path(clone->readtrans);
	clone->writetrans = NULL;
	clone->readtrans = NULL;
	clone->rawwriteformat = clone->nativeformats;
	clone->rawreadformat = clone->nativeformats;
}

/*--- cw_hangup: Hangup a channel */
int cw_hangup(struct cw_channel *chan)
{
	int res = 0;

	cw_generator_deactivate(chan);

	/* Don't actually hang up a channel that will masquerade as someone else, or
	   if someone is going to masquerade as us */
	cw_mutex_lock(&chan->lock);

	cw_spy_detach(chan);		/* get rid of spies */

	if (chan->masq)
	{
		if (cw_do_masquerade(chan)) 
			cw_log(LOG_WARNING, "Failed to perform masquerade\n");
	}

	if (chan->masq)
	{
		cw_log(LOG_WARNING, "%s getting hung up, but someone is trying to masq into us?!?\n", chan->name);
		cw_mutex_unlock(&chan->lock);
		return 0;
	}
	/* If this channel is one which will be masqueraded into something, 
	   mark it as a zombie already, so we know to free it later */
	if (chan->masqr)
	{
		cw_set_flag(chan, CW_FLAG_ZOMBIE);
		cw_mutex_unlock(&chan->lock);
		return 0;
	}
	free_translation(chan);
	if (chan->stream) 		/* Close audio stream */
		cw_closestream(chan->stream);
	if (chan->vstream)		/* Close video stream */
		cw_closestream(chan->vstream);
	if (chan->sched)
		sched_context_destroy(chan->sched);
	
	if (chan->cdr)
	{
        /* End the CDR if it hasn't already */ 
		cw_cdr_end(chan->cdr);
		cw_cdr_detach(chan->cdr);	/* Post and Free the CDR */ 
		chan->cdr = NULL;
	}
	if (cw_test_flag(chan, CW_FLAG_BLOCKING))
	{
		cw_log(LOG_WARNING, "Hard hangup called by thread %ld on %s, while fd "
					"is blocked by thread %ld in procedure %s!  Expect a failure\n",
					(long)pthread_self(), chan->name, (long)chan->blocker, chan->blockproc);
		CRASH;
	}
	if (!cw_test_flag(chan, CW_FLAG_ZOMBIE))
	{
		if (option_debug)
			cw_log(LOG_DEBUG, "Hanging up channel '%s'\n", chan->name);
		if (chan->tech->hangup)
			res = chan->tech->hangup(chan);
	}
	else
	{
		if (option_debug)
			cw_log(LOG_DEBUG, "Hanging up zombie '%s'\n", chan->name);
	}
			
	cw_mutex_unlock(&chan->lock);
	
	/** cw_generator_deactivate after mutex_unlock*/
	if (option_debug)
	    cw_log(LOG_DEBUG, "Generator : deactivate after channel unlock (hangup function)\n");
	cw_generator_deactivate(chan);

	manager_event(EVENT_FLAG_CALL, "Hangup", 
			"Channel: %s\r\n"
			"Uniqueid: %s\r\n"
			"Cause: %d\r\n"
			"Cause-txt: %s\r\n",
			chan->name, 
			chan->uniqueid, 
			chan->hangupcause,
			cw_cause2str(chan->hangupcause)
			);
	cw_channel_free(chan);
	return res;
}

int cw_answer(struct cw_channel *chan)
{
	int res = 0;

	cw_mutex_lock(&chan->lock);
	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(chan, CW_FLAG_ZOMBIE) || cw_check_hangup(chan))
	{
		cw_mutex_unlock(&chan->lock);
		return -1;
	}
	switch (chan->_state)
	{
    case CW_STATE_RINGING:
    case CW_STATE_RING:
		if (chan->tech->answer)
			res = chan->tech->answer(chan);
		cw_setstate(chan, CW_STATE_UP);
		if (chan->cdr)
			cw_cdr_answer(chan->cdr);
		cw_mutex_unlock(&chan->lock);
		return res;
    case CW_STATE_UP:
		if (chan->cdr)
			cw_cdr_answer(chan->cdr);
		break;
	}
	cw_mutex_unlock(&chan->lock);
	return 0;
}

/*--- cw_waitfor_n_fd: Wait for x amount of time on a file descriptor to have input.  
      Please note that this is used only by DUNDI, at current date (Tue Jul 24, 2007).
*/

int cw_waitfor_n_fd(int *fds, int n, int *ms, int *exception)
{
	struct timeval start = { 0 , 0 };
	int res;
	int x, y;
	int winner = -1;
	int spoint;
	struct pollfd *pfds;
	
	pfds = alloca(sizeof(struct pollfd) * n);
	if (*ms > 0)
		start = cw_tvnow();
	y = 0;
	for (x = 0;  x < n;  x++)
	{
		if (fds[x] > -1)
        {
			pfds[y].fd = fds[x];
			pfds[y].events = POLLIN | POLLPRI;
			y++;
		}
	}
	res = poll(pfds, y, *ms);
	if (res < 0)
	{
		/* Simulate a timeout if we were interrupted */
		*ms = (errno != EINTR)  ?  -1  :  0;
		return -1;
	}
	spoint = 0;
	for (x = 0;  x < n;  x++)
	{
	    	if (fds[x] > -1)
    		{
			if ((res = cw_fdisset(pfds, fds[x], y, &spoint)))
        		{
				winner = fds[x];
				if (exception)
					*exception = (res & POLLPRI)?  -1  :  0;
			}
		}
	}
	if (*ms > 0)
	{
		*ms -= cw_tvdiff_ms(cw_tvnow(), start);
		if (*ms < 0)
			*ms = 0;
	}
	return winner;
}

/*--- cw_waitfor_nanfds: Wait for x amount of time on a file descriptor to have input.  */
struct cw_channel *cw_waitfor_nandfds(struct cw_channel **c, int n, int *fds, int nfds, 
	int *exception, int *outfd, int *ms)
{
	struct timeval start = { 0 , 0 };
	struct pollfd *pfds;
	int res;
	long rms;
	int x, y, max;
	int spoint;
	time_t now = 0;
	long whentohangup = 0, havewhen = 0, diff;
	struct cw_channel *winner = NULL;

	pfds = alloca(sizeof(struct pollfd) * (n * CW_MAX_FDS + nfds));

	if (outfd)
		*outfd = -99999;
	if (exception)
		*exception = 0;
	
	/* Perform any pending masquerades */
	for (x = 0;  x < n;  x++)
	{
		cw_mutex_lock(&c[x]->lock);
		if (c[x]->whentohangup)
    		{
			if (!havewhen)
				time(&now);
			diff = c[x]->whentohangup - now;
			if (!havewhen || (diff < whentohangup))
        		{
				havewhen++;
				whentohangup = diff;
			}
		}
		if (c[x]->masq)
    		{
			if (cw_do_masquerade(c[x]))
        		{
				cw_log(LOG_WARNING, "Masquerade failed\n");
				*ms = -1;
				cw_mutex_unlock(&c[x]->lock);
				return NULL;
			}
		}
		cw_mutex_unlock(&c[x]->lock);
	}

	rms = *ms;
	
	if (havewhen)
	{
		if ((*ms < 0) || (whentohangup * 1000 < *ms))
			rms = whentohangup * 1000;
	}
	max = 0;
	for (x = 0;  x < n;  x++)
	{
		for (y = 0;  y < CW_MAX_FDS;  y++)
    		{
			if (c[x]->fds[y] > -1)
        		{
				pfds[max].fd = c[x]->fds[y];
				pfds[max].events = POLLIN | POLLPRI;
				pfds[max].revents = 0;
				max++;
			}
		}
		CHECK_BLOCKING(c[x]);
	}
	for (x = 0;  x < nfds;  x++)
	{
		if (fds[x] > -1)
    		{
			pfds[max].fd = fds[x];
			pfds[max].events = POLLIN | POLLPRI;
			pfds[max].revents = 0;
			max++;
		}
	}
	if (*ms > 0) 
		start = cw_tvnow();
	
	if (sizeof(int) == 4)
	{
		do
    		{
			int kbrms = rms;

			if (kbrms > 600000)
				kbrms = 600000;
			res = poll(pfds, max, kbrms);
			if (!res)
				rms -= kbrms;
		}
    	    while (!res  &&  (rms > 0));
	}
	else
	{
	    res = poll(pfds, max, rms);
	}
	
	if (res < 0)
	{
		for (x = 0;  x < n;  x++) 
			cw_clear_flag(c[x], CW_FLAG_BLOCKING);
		/* Simulate a timeout if we were interrupted */
		if (errno != EINTR)
    		{
			*ms = -1;
		}
    		else
    		{
		    /* Just an interrupt */
#if 0
		    *ms = 0;
#endif			
		}
		return NULL;
	}
	else
	{
       	/* If no fds signalled, then timeout. So set ms = 0
		   since we may not have an exact timeout. */
		if (res == 0)
			*ms = 0;
	}

	if (havewhen)
		time(&now);
		
	spoint = 0;
	for (x = 0;  x < n;  x++)
	{
		cw_clear_flag(c[x], CW_FLAG_BLOCKING);
		if (havewhen && c[x]->whentohangup && (now > c[x]->whentohangup))
    		{
			c[x]->_softhangup |= CW_SOFTHANGUP_TIMEOUT;
			if (!winner)
				winner = c[x];
		}
		for (y = 0;  y < CW_MAX_FDS;  y++)
    		{
			if (c[x]->fds[y] > -1)
        		{
				if ((res = cw_fdisset(pfds, c[x]->fds[y], max, &spoint)))
            			{
					if (res & POLLPRI)
						cw_set_flag(c[x], CW_FLAG_EXCEPTION);
					else
						cw_clear_flag(c[x], CW_FLAG_EXCEPTION);
					c[x]->fdno = y;
					winner = c[x];
				}
			}
		}
	}
	for (x = 0;  x < nfds;  x++)
	{
	    if (fds[x] > -1)
    	    {
			if ((res = cw_fdisset(pfds, fds[x], max, &spoint)))
        		{
				if (outfd)
					*outfd = fds[x];
				if (exception)
            			{	
					if (res & POLLPRI) 
						*exception = -1;
					else
						*exception = 0;
				}
				winner = NULL;
			}
	    }	
	}
	if (*ms > 0)
        {
		*ms -= cw_tvdiff_ms(cw_tvnow(), start);
		if (*ms < 0)
			*ms = 0;
	}
	return winner;
}

struct cw_channel *cw_waitfor_n(struct cw_channel **c, int n, int *ms)
{
	return cw_waitfor_nandfds(c, n, NULL, 0, NULL, NULL, ms);
}

int cw_waitfor(struct cw_channel *c, int ms)
{
	struct cw_channel *chan;
	int oldms = ms;

	chan = cw_waitfor_n(&c, 1, &ms);
	if (ms < 0)
	{
		if (oldms < 0)
			return 0;
		else
			return -1;
	}
	return ms;
}

int cw_waitfordigit(struct cw_channel *c, int ms)
{
	/* XXX Should I be merged with waitfordigit_full XXX */
	struct cw_frame *f;
	int result = 0;

	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(c, CW_FLAG_ZOMBIE) || cw_check_hangup(c)) 
		return -1;

	/* Wait for a digit, no more than ms milliseconds total. */
	while (ms && !result)
	{
		ms = cw_waitfor(c, ms);
		if (ms < 0) /* Error */
			result = -1; 
		else if (ms > 0)
    		{
			/* Read something */
			f = cw_read(c);
			if (f)
        		{
				if (f->frametype == CW_FRAME_DTMF) 
					result = f->subclass;
				cw_fr_free(f);
			}
        		else
        		{
				result = -1;
        		}
		}
	}
	return result;
}

int cw_waitfordigit_full(struct cw_channel *c, int ms, int audiofd, int cmdfd)
{
	struct cw_frame *f;
	struct cw_channel *rchan;
	int outfd;
	int res;

	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(c, CW_FLAG_ZOMBIE) || cw_check_hangup(c)) 
		return -1;
	/* Wait for a digit, no more than ms milliseconds total. */
	while (ms)
	{
		errno = 0;
		rchan = cw_waitfor_nandfds(&c, 1, &cmdfd, (cmdfd > -1) ? 1 : 0, NULL, &outfd, &ms);
		if ((!rchan) && (outfd < 0) && (ms))
    		{ 
			if (errno == 0 || errno == EINTR)
				continue;
			cw_log(LOG_WARNING, "Wait failed (%s)\n", strerror(errno));
			return -1;
		}
    		else if (outfd > -1)
    		{
			/* The FD we were watching has something waiting */
			return 1;
		}
    		else if (rchan)
    		{
			if ((f = cw_read(c)) == 0)
				return -1;
			switch (f->frametype)
        		{
			    case CW_FRAME_DTMF:
				res = f->subclass;
				cw_fr_free(f);
				return res;
			    case CW_FRAME_CONTROL:
				switch(f->subclass)
            			{
				    case CW_CONTROL_HANGUP:
					cw_fr_free(f);
					return -1;
				    case CW_CONTROL_RINGING:
				    case CW_CONTROL_ANSWER:
					/* Unimportant */
					break;
				default:
					cw_log(LOG_WARNING, "Unexpected control subclass '%d'\n", f->subclass);
				}
			    case CW_FRAME_VOICE:
				/* Write audio if appropriate */
				if (audiofd > -1)
					write(audiofd, f->data, f->datalen);
			}
			/* Ignore */
			cw_fr_free(f);
		}
	}
	return 0; /* Time is up */
}

struct cw_frame *cw_read(struct cw_channel *chan)
{
	struct cw_frame *f = NULL;
	int blah;
	int prestate;
	static struct cw_frame null_frame =
	{
		CW_FRAME_NULL,
	};
	
	cw_mutex_lock(&chan->lock);
	if (chan->masq)
	{
		if (cw_do_masquerade(chan))
    		{
			cw_log(LOG_WARNING, "Failed to perform masquerade\n");
			f = NULL;
		}
    		else
    		{
		    f =  &null_frame;
		}
    		cw_mutex_unlock(&chan->lock);
		return f;
	}

	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(chan, CW_FLAG_ZOMBIE)  ||  cw_check_hangup(chan))
	{
		cw_mutex_unlock(&chan->lock);
		cw_generator_deactivate(chan);
		return NULL;
	}
	prestate = chan->_state;

	if (!cw_test_flag(chan, CW_FLAG_DEFER_DTMF) && !cw_strlen_zero(chan->dtmfq))
	{
		/* We have DTMF that has been deferred.  Return it now */
    		cw_fr_init_ex(&chan->dtmff, CW_FRAME_DTMF, chan->dtmfq[0], NULL);
		/* Drop first digit */
		memmove(chan->dtmfq, chan->dtmfq + 1, sizeof(chan->dtmfq) - 1);
		cw_mutex_unlock(&chan->lock);
		return &chan->dtmff;
	}
	
	/* Read and ignore anything on the alertpipe, but read only
	   one sizeof(blah) per frame that we send from it */
	if (chan->alertpipe[0] > -1)
		read(chan->alertpipe[0], &blah, sizeof(blah));

	/* Check for pending read queue */
	if (chan->readq)
	{
		f = chan->readq;
		chan->readq = f->next;
		/* Interpret hangup and return NULL */
		if ((f->frametype == CW_FRAME_CONTROL)  &&  (f->subclass == CW_CONTROL_HANGUP))
    		{
			cw_fr_free(f);
			f = NULL;
		}
	}
	else
	{
		chan->blocker = pthread_self();
		if (cw_test_flag(chan, CW_FLAG_EXCEPTION))
    		{
			if (chan->tech->exception) 
			{
            		    f = chan->tech->exception(chan);
			}
        		else
        		{
				cw_log(LOG_WARNING, "Exception flag set on '%s', but no exception handler\n", chan->name);
				f = &null_frame;
			}
			/* Clear the exception flag */
			cw_clear_flag(chan, CW_FLAG_EXCEPTION);
		}
    		else
    		{
			if (chan->tech->read)
				f = chan->tech->read(chan);
			else
				cw_log(LOG_WARNING, "No read routine on channel %s\n", chan->name);
		}
	}

	if (f)
	{
		/* If the channel driver returned more than one frame, stuff the excess
		   into the readq for the next cw_read call */
		if (f->next)
    		{
        		/* We can safely assume the read queue is empty, or we wouldn't be here */
			chan->readq = f->next;
			f->next = NULL;
		}

    	if ((f->frametype == CW_FRAME_VOICE))
        {
    		if (!(f->subclass & chan->nativeformats))
        	{
    			/* This frame can't be from the current native formats -- drop it on the floor */
    			cw_log(LOG_NOTICE, "Dropping incompatible voice frame on %s of format %s since our native format has changed to %s\n", chan->name, cw_getformatname(f->subclass), cw_getformatname(chan->nativeformats));
    			cw_fr_free(f);
    			f = &null_frame;
    		}
        	else
        	{
    			struct cw_channel_spy *spying;

    			for (spying = chan->spiers;  spying;  spying = spying->next)
    				cw_queue_spy_frame(spying, f, 0);
    			if (chan->monitor && chan->monitor->read_stream)
            		{
#ifndef MONITOR_CONSTANT_DELAY
    				int jump = chan->outsmpl - chan->insmpl - 2 * f->samples;

    				if (jump >= 0)
                		{
    					if (cw_seekstream(chan->monitor->read_stream, jump + f->samples, SEEK_FORCECUR) == -1)
    						cw_log(LOG_WARNING, "Failed to perform seek in monitoring read stream, synchronization between the files may be broken\n");
    					chan->insmpl += jump + 2 * f->samples;
    				}
                		else
                		{
	    				chan->insmpl+= f->samples;
                		}
#else
    				int jump = chan->outsmpl - chan->insmpl;

	    			if (jump - MONITOR_DELAY >= 0)
                		{
			    		if (cw_seekstream(chan->monitor->read_stream, jump - f->samples, SEEK_FORCECUR) == -1)
    						cw_log(LOG_WARNING, "Failed to perform seek in monitoring read stream, synchronization between the files may be broken\n");
	    				chan->insmpl += jump;
		    		}
                		else
                		{
    					chan->insmpl += f->samples;
                		}
#endif
		    		if (cw_writestream(chan->monitor->read_stream, f) < 0)
			    		cw_log(LOG_WARNING, "Failed to write data to channel monitor read stream\n");
    			}
			
	    		if (chan->readtrans)
            		{
    				if ((f = cw_translate(chan->readtrans, f, 1)) == NULL)
	    				f = &null_frame;
		    	}
    	    }
	}
    }

    if (!f)
    {
    	/* Make sure we always return NULL in the future */
		chan->_softhangup |= CW_SOFTHANGUP_DEV;
		cw_generator_deactivate(chan);
		/* End the CDR if appropriate */
		if (chan->cdr)
			cw_cdr_end(chan->cdr);
    }
    else if (cw_test_flag(chan, CW_FLAG_DEFER_DTMF) && f->frametype == CW_FRAME_DTMF)
    {
		if (strlen(chan->dtmfq) < sizeof(chan->dtmfq) - 2)
			chan->dtmfq[strlen(chan->dtmfq)] = f->subclass;
		else
			cw_log(LOG_WARNING, "Dropping deferred DTMF digits on %s\n", chan->name);
		f = &null_frame;
    }
    else if ((f->frametype == CW_FRAME_CONTROL) && (f->subclass == CW_CONTROL_ANSWER))
    {
		if (prestate == CW_STATE_UP)
    		{
			cw_log(LOG_DEBUG, "Dropping duplicate answer!\n");
			f = &null_frame;
		}
		/* Answer the CDR */
		cw_setstate(chan, CW_STATE_UP);
		cw_cdr_answer(chan->cdr);
    }


	/* High bit prints debugging */
	if (chan->fin & 0x80000000)
		cw_frame_dump(chan->name, f, "<<");
	if ((chan->fin & 0x7fffffff) == 0x7fffffff)
		chan->fin &= 0x80000000;
	else
		chan->fin++;
		
	
	cw_mutex_unlock(&chan->lock);
	/** generator deactivate after channel unlock */
	if (f == NULL  &&  cw_generator_is_active(chan))
	{
	    if (option_debug)
	    	cw_log(LOG_DEBUG, "Generator not finished in previous deactivate attempt - trying deactivate after channel unlock (cw_read function)\n");
	    cw_generator_deactivate(chan);
	}	
	
	return f;
}

int cw_indicate(struct cw_channel *chan, int condition)
{
    int res = -1;

    /* Stop if we're a zombie or need a soft hangup */
    if (cw_test_flag(chan, CW_FLAG_ZOMBIE) || cw_check_hangup(chan)) 
    	return -1;
    cw_mutex_lock(&chan->lock);
    if (chan->tech->indicate)
    	res = chan->tech->indicate(chan, condition);
    cw_mutex_unlock(&chan->lock);
    if (!chan->tech->indicate  ||  res)
    {
	/*
	 * Device does not support (that) indication, lets fake
	 * it by doing our own tone generation. (PM2002)
	 */
	if (condition >= 0)
    	{
	    const struct tone_zone_sound *ts = NULL;
	    switch (condition)
    	    {
		case CW_CONTROL_RINGING:
		    ts = cw_get_indication_tone(chan->zone, "ring");
		    break;
		case CW_CONTROL_BUSY:
		    ts = cw_get_indication_tone(chan->zone, "busy");
		    break;
		case CW_CONTROL_CONGESTION:
		    ts = cw_get_indication_tone(chan->zone, "congestion");
		    break;
	    }
	    if (ts  &&  ts->data[0])
    	    {
			cw_log(LOG_DEBUG, "Driver for channel '%s' does not support indication %d, emulating it\n", chan->name, condition);
			cw_playtones_start(chan,0,ts->data, 1);
			res = 0;
	    }
            else if (condition == CW_CONTROL_PROGRESS)
            {
				/* cw_playtones_stop(chan); */
	    }
            else if (condition == CW_CONTROL_PROCEEDING)
            {
				/* Do nothing, really */
	    }
            else if (condition == CW_CONTROL_HOLD)
            {
				/* Do nothing.... */
	    }
            else if (condition == CW_CONTROL_UNHOLD)
            {
				/* Do nothing.... */
	    }
            else if (condition == CW_CONTROL_VIDUPDATE)
            {
				/* Do nothing.... */
	    }
            else
            {
				/* not handled */
				cw_log(LOG_WARNING, "Unable to handle indication %d for '%s'\n", condition, chan->name);
				res = -1;
	    }
	}
	else
        {
            cw_playtones_stop(chan);
        }
    }
    return res;
}

int cw_recvchar(struct cw_channel *chan, int timeout)
{
	int c;
	char *buf = cw_recvtext(chan, timeout);

	if (buf == NULL)
		return -1;	/* error or timeout */
	c = *(unsigned char *) buf;
	free(buf);
	return c;
}

char *cw_recvtext(struct cw_channel *chan, int timeout)
{
	int res, done = 0;
	char *buf = NULL;
	
	while (!done)
	{
		struct cw_frame *f;

		if (cw_check_hangup(chan))
			break;
		if ((res = cw_waitfor(chan, timeout)) <= 0) /* timeout or error */
			break;
		timeout = res;	/* update timeout */
		if ((f = cw_read(chan)) == NULL)
			break; /* no frame */
		if (f->frametype == CW_FRAME_CONTROL  &&  f->subclass == CW_CONTROL_HANGUP)
		{
        	done = 1;	/* force a break */
		}
    		else if (f->frametype == CW_FRAME_TEXT)
    		{
        	    /* what we want */
			buf = strndup((char *) f->data, f->datalen);	/* dup and break */
			done = 1;
		}
		cw_fr_free(f);
	}
	return buf;
}

int cw_sendtext(struct cw_channel *chan, const char *text)
{
	int res = 0;

	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(chan, CW_FLAG_ZOMBIE)  ||  cw_check_hangup(chan)) 
		return -1;
	CHECK_BLOCKING(chan);
	if (chan->tech->send_text)
		res = chan->tech->send_text(chan, text);
	cw_clear_flag(chan, CW_FLAG_BLOCKING);
	return res;
}

static int do_senddigit(struct cw_channel *chan, char digit)
{
	int res = -1;

	if (chan->tech->send_digit)
		res = chan->tech->send_digit(chan, digit);
	if (!chan->tech->send_digit || res < 0)
    {
		/*
		 * Device does not support DTMF tones, lets fake
		 * it by doing our own generation. (PM2002)
		 */
		static const char* dtmf_tones[] =
        {
			"!0/100,!0/100",	/* silence */
			"!941+1336/100,!0/100",	/* 0 */
			"!697+1209/100,!0/100",	/* 1 */
			"!697+1336/100,!0/100",	/* 2 */
			"!697+1477/100,!0/100",	/* 3 */
			"!770+1209/100,!0/100",	/* 4 */
			"!770+1336/100,!0/100",	/* 5 */
			"!770+1477/100,!0/100",	/* 6 */
			"!852+1209/100,!0/100",	/* 7 */
			"!852+1336/100,!0/100",	/* 8 */
			"!852+1477/100,!0/100",	/* 9 */
			"!697+1633/100,!0/100",	/* A */
			"!770+1633/100,!0/100",	/* B */
			"!852+1633/100,!0/100",	/* C */
			"!941+1633/100,!0/100",	/* D */
			"!941+1209/100,!0/100",	/* * */
			"!941+1477/100,!0/100"	/* # */
        };
		if (res == -2)
			cw_playtones_start(chan, 0, dtmf_tones[0], 0);
		else if (digit >= '0'  &&  digit <='9')
			cw_playtones_start(chan, 0, dtmf_tones[1 + digit - '0'], 0);
		else if (digit >= 'A' && digit <= 'D')
			cw_playtones_start(chan, 0, dtmf_tones[1 + digit - 'A' + 10], 0);
		else if (digit == '*')
			cw_playtones_start(chan, 0, dtmf_tones[15], 0);
		else if (digit == '#')
			cw_playtones_start(chan, 0, dtmf_tones[16], 0);
		else
        {
			/* not handled */
			cw_log(LOG_DEBUG, "Unable to generate DTMF tone '%c' for '%s'\n", digit, chan->name);
		}
	}
	return 0;
}

int cw_senddigit(struct cw_channel *chan, char digit)
{
	return do_senddigit(chan, digit);
}

int cw_prod(struct cw_channel *chan)
{
	struct cw_frame a = { CW_FRAME_VOICE };
	char nothing[128];

	/* Send an empty audio frame to get things moving */
	if (chan->_state != CW_STATE_UP)
    {
		cw_log(LOG_DEBUG, "Prodding channel '%s'\n", chan->name);
		a.subclass = chan->rawwriteformat;
		a.data = nothing + CW_FRIENDLY_OFFSET;
		a.src = "cw_prod";
		if (cw_write(chan, &a))
			cw_log(LOG_WARNING, "Prodding channel '%s' failed\n", chan->name);
	}
	return 0;
}

int cw_write_video(struct cw_channel *chan, struct cw_frame *fr)
{
	int res;

	if (!chan->tech->write_video)
		return 0;
	if ((res = cw_write(chan, fr)) == 0)
		res = 1;
	return res;
}

int cw_write(struct cw_channel *chan, struct cw_frame *fr)
{
	int res = -1;
	struct cw_frame *f = NULL;

	/* Stop if we're a zombie or need a soft hangup */
	cw_mutex_lock(&chan->lock);
	if (cw_test_flag(chan, CW_FLAG_ZOMBIE) || cw_check_hangup(chan))
    {
		cw_mutex_unlock(&chan->lock);
		return -1;
	}
	/* Handle any pending masquerades */
	if (chan->masq)
    {
		if (cw_do_masquerade(chan))
        {
			cw_log(LOG_WARNING, "Failed to perform masquerade\n");
			cw_mutex_unlock(&chan->lock);
			return -1;
		}
	}
	if (chan->masqr)
    {
		cw_mutex_unlock(&chan->lock);
		return 0;
	}

	/* A write by a non channel generator thread may or may not
	 * deactivate a running channel generator depending on
	 * whether the CW_FLAG_WRITE_INT is set or not for the
	 * channel. If CW_FLAG_WRITE_INT is set, channel generator
	 * is deactivated. Otherwise, the write is simply ignored. */
	if (!cw_generator_is_self(chan)  &&  cw_generator_is_active(chan))
	{
		/* We weren't called by the generator
		 * thread and channel generator is active */
		if (cw_test_flag(chan, CW_FLAG_WRITE_INT))
        {
			/* Deactivate generator */

			/** unlock & lock added - testing if this caused crashes*/
			cw_mutex_unlock(&chan->lock);
			if (option_debug)
			    cw_log(LOG_DEBUG, "trying deactivate generator with unlock/lock channel (cw_write function)\n");
			cw_generator_deactivate(chan);
			cw_mutex_lock(&chan->lock);
		}
        else
        {
			/* Write doesn't interrupt generator.
			 * Write gets ignored instead */
			cw_mutex_unlock(&chan->lock);
			return 0;
		}

	}

	/* High bit prints debugging */
	if (chan->fout & 0x80000000)
		cw_frame_dump(chan->name, fr, ">>");
#if 0
	/* CMANTUNES: Do we really need this CHECK_BLOCKING thing in here?
	 * I no longer think we do because we can now be reading and writing
	 * at the same time. Writing is no longer tied to reading as before */
	CHECK_BLOCKING(chan);
#endif
	switch (fr->frametype)
    {
	case CW_FRAME_CONTROL:
		/* XXX Interpret control frames XXX */
		cw_log(LOG_WARNING, "Don't know how to handle control frames yet\n");
        res = 1;
		break;
	case CW_FRAME_DTMF:
		cw_clear_flag(chan, CW_FLAG_BLOCKING);
		cw_mutex_unlock(&chan->lock);
		res = do_senddigit(chan,fr->subclass);
		cw_mutex_lock(&chan->lock);
		CHECK_BLOCKING(chan);
		break;
	case CW_FRAME_TEXT:
		if (chan->tech->send_text)
			res = chan->tech->send_text(chan, (char *) fr->data);
		else
			res = 0;
		break;
	case CW_FRAME_HTML:
		if (chan->tech->send_html)
			res = chan->tech->send_html(chan, fr->subclass, (char *) fr->data, fr->datalen);
		else
			res = 0;
		break;
	case CW_FRAME_VIDEO:
		/* XXX Handle translation of video codecs one day XXX */
		if (chan->tech->write_video)
			res = chan->tech->write_video(chan, fr);
		else
			res = 0;
		break;
	case CW_FRAME_MODEM:
		if (chan->tech->write)
			res = chan->tech->write(chan, fr);
		else
			res = 0;
		break;
	default:
		if (chan->tech->write)
        {
			if (chan->writetrans && fr->frametype == CW_FRAME_VOICE) 
				f = cw_translate(chan->writetrans, fr, 0);
			else
				f = fr;
			if (f)
            {
				/* CMANTUNES: Instead of writing directly here,
				 * we could insert frame into output queue and
				 * let the channel driver use a writer thread
				 * to actually write the stuff, for example. */

				if (f->frametype == CW_FRAME_VOICE) {
					struct cw_channel_spy *spying;
					for (spying = chan->spiers;  spying;  spying = spying->next)
						cw_queue_spy_frame(spying, f, 1);
				}

				if( chan->monitor && chan->monitor->write_stream &&
						f && ( f->frametype == CW_FRAME_VOICE ) ) {
#ifndef MONITOR_CONSTANT_DELAY
					int jump = chan->insmpl - chan->outsmpl - 2 * f->samples;
					if (jump >= 0)
                    {
						if (cw_seekstream(chan->monitor->write_stream, jump + f->samples, SEEK_FORCECUR) == -1)
							cw_log(LOG_WARNING, "Failed to perform seek in monitoring write stream, synchronization between the files may be broken\n");
						chan->outsmpl += jump + 2 * f->samples;
					}
                    else
						chan->outsmpl += f->samples;
#else
					int jump = chan->insmpl - chan->outsmpl;
					if (jump - MONITOR_DELAY >= 0)
                    {
						if (cw_seekstream(chan->monitor->write_stream, jump - f->samples, SEEK_FORCECUR) == -1)
							cw_log(LOG_WARNING, "Failed to perform seek in monitoring write stream, synchronization between the files may be broken\n");
						chan->outsmpl += jump;
					}
                    else
						chan->outsmpl += f->samples;
#endif
					if (cw_writestream(chan->monitor->write_stream, f) < 0)
						cw_log(LOG_WARNING, "Failed to write data to channel monitor write stream\n");
				}

				res = chan->tech->write(chan, f);
			}
            else
            {
				res = 0;
            }
		}
	}

	/* It's possible this is a translated frame */
	if (f && f->frametype == CW_FRAME_DTMF)
		cw_log(LOG_DTMF, "%s : %c\n", chan->name, f->subclass);
    else if (fr->frametype == CW_FRAME_DTMF)
		cw_log(LOG_DTMF, "%s : %c\n", chan->name, fr->subclass);

	if (f && (f != fr))
		cw_fr_free(f);
	cw_clear_flag(chan, CW_FLAG_BLOCKING);
	/* Consider a write failure to force a soft hangup */
	if (res < 0)
		chan->_softhangup |= CW_SOFTHANGUP_DEV;
	else
    {
		if ((chan->fout & 0x7fffffff) == 0x7fffffff)
			chan->fout &= 0x80000000;
		else
			chan->fout++;
	}
	cw_mutex_unlock(&chan->lock);
	return res;
}

static int set_format(
                        struct cw_channel *chan, 
                        int fmt, 
                        int *rawformat, 
                        int *format,
		        struct cw_trans_pvt **trans, 
                        const int direction
                     )
{
	int native;
	int res;
	
	native = chan->nativeformats;
	/* Find a translation path from the native format to one of the desired formats */
	if (!direction)
		/* reading */
		res = cw_translator_best_choice(&fmt, &native);
	else
		/* writing */
		res = cw_translator_best_choice(&native, &fmt);

	if (res < 0)
	{
		cw_log(LOG_WARNING, "Unable to find a codec translation path from %s to %s\n",
			cw_getformatname(native), cw_getformatname(fmt));
		return -1;
	}
	
	/* Now we have a good choice for both. */
	cw_mutex_lock(&chan->lock);

	if ((*rawformat == native) && (*format == fmt) && ((*rawformat == *format) || (*trans))) {
		/* the channel is already in these formats, so nothing to do */
		cw_mutex_unlock(&chan->lock);
		return 0;
	}

	*rawformat = native;
	/* User perspective is fmt */
	*format = fmt;
	/* Free any read translation we have right now */
	if (*trans)
		cw_translator_free_path(*trans);
	/* Build a translation path from the raw format to the desired format */
	if (!direction)
		/* reading */
		*trans = cw_translator_build_path(*format, 8000, *rawformat, 8000);
	else
		/* writing */
		*trans = cw_translator_build_path(*rawformat, 8000, *format, 8000);
	cw_mutex_unlock(&chan->lock);
	if (option_debug)
    {
		cw_log(LOG_DEBUG, "Set channel %s to %s format %s\n", chan->name,
			direction ? "write" : "read", cw_getformatname(fmt));
	}
    return 0;
}

int cw_set_read_format(struct cw_channel *chan, int fmt)
{
	return set_format(chan, fmt, &chan->rawreadformat, &chan->readformat,
			  &chan->readtrans, 0);
}

int cw_set_write_format(struct cw_channel *chan, int fmt)
{
	return set_format(chan, fmt, &chan->rawwriteformat, &chan->writeformat,
			  &chan->writetrans, 1);
}

struct cw_channel *__cw_request_and_dial(const char *type, int format, void *data, int timeout, int *outstate, const char *cid_num, const char *cid_name, struct outgoing_helper *oh)
{
	int state = 0;
	int cause = 0;
	struct cw_channel *chan;
	struct cw_frame *f;
	int res = 0;
	
	chan = cw_request(type, format, data, &cause);
	if (chan)
    {
		if (oh)
        {
			cw_set_variables(chan, oh->vars);
			cw_set_callerid(chan, oh->cid_num, oh->cid_name, oh->cid_num);
		}
		cw_set_callerid(chan, cid_num, cid_name, cid_num);

		if (!cw_call(chan, data, 0))
        {
			while (timeout  &&  (chan->_state != CW_STATE_UP))
            {
				if ((res = cw_waitfor(chan, timeout)) < 0)
                {
					/* Something not cool, or timed out */
					break;
				}
				/* If done, break out */
				if (!res)
					break;
				if (timeout > -1)
					timeout = res;
				if ((f = cw_read(chan)) == 0)
				{
					state = CW_CONTROL_HANGUP;
					res = 0;
					break;
				}
				if (f->frametype == CW_FRAME_CONTROL)
                {
					if (f->subclass == CW_CONTROL_RINGING)
                    {
						state = CW_CONTROL_RINGING;
                    }
					else if ((f->subclass == CW_CONTROL_BUSY)  ||  (f->subclass == CW_CONTROL_CONGESTION))
                    {
						state = f->subclass;
						cw_fr_free(f);
						break;
					}
                    else if (f->subclass == CW_CONTROL_ANSWER)
                    {
						state = f->subclass;
						cw_fr_free(f);
						break;
					}
                    else if (f->subclass == CW_CONTROL_PROGRESS)
                    {
						/* Ignore */
					}
                    else if (f->subclass == -1)
                    {
						/* Ignore -- just stopping indications */
					}
                    else
                    {
						cw_log(LOG_NOTICE, "Don't know what to do with control frame %d\n", f->subclass);
					}
				}
				cw_fr_free(f);
			}
		}
        else
			cw_log(LOG_NOTICE, "Unable to call channel %s/%s\n", type, (char *)data);
	}
    else
    {
		cw_log(LOG_NOTICE, "Unable to request channel %s/%s\n", type, (char *)data);
		switch(cause)
        {
		case CW_CAUSE_BUSY:
			state = CW_CONTROL_BUSY;
			break;
		case CW_CAUSE_CONGESTION:
			state = CW_CONTROL_CONGESTION;
			break;
		}
	}
	if (chan)
    {
		/* Final fixups */
		if (oh)
        {
			if (oh->context && *oh->context)
				cw_copy_string(chan->context, oh->context, sizeof(chan->context));
			if (oh->exten && *oh->exten)
				cw_copy_string(chan->exten, oh->exten, sizeof(chan->exten));
			if (oh->priority)	
				chan->priority = oh->priority;
		}
		if (chan->_state == CW_STATE_UP) 
			state = CW_CONTROL_ANSWER;
	}
	if (outstate)
		*outstate = state;
	if (chan  &&  res <= 0)
    {
		if (!chan->cdr)
        {
			chan->cdr = cw_cdr_alloc();
			if (chan->cdr)
				cw_cdr_init(chan->cdr, chan);
		}
		if (chan->cdr)
        {
			char tmp[256];
			snprintf(tmp, 256, "%s/%s", type, (char *)data);
			cw_cdr_setapp(chan->cdr,"Dial",tmp);
			cw_cdr_update(chan);
			cw_cdr_start(chan->cdr);
			cw_cdr_end(chan->cdr);
			/* If the cause wasn't handled properly */
			if (cw_cdr_disposition(chan->cdr,chan->hangupcause))
				cw_cdr_failed(chan->cdr);
		}
        else 
		{
        	cw_log(LOG_WARNING, "Unable to create Call Detail Record\n");
		}
        cw_hangup(chan);
		chan = NULL;
	}
	return chan;
}

struct cw_channel *cw_request_and_dial(const char *type, int format, void *data, int timeout, int *outstate, const char *cidnum, const char *cidname)
{
	return __cw_request_and_dial(type, format, data, timeout, outstate, cidnum, cidname, NULL);
}

struct cw_channel *cw_request(const char *type, int format, void *data, int *cause)
{
	struct chanlist *chan;
	struct cw_channel *c = NULL;
	int capabilities;
	int fmt;
	int res;
	int foo;

	if (!cause)
		cause = &foo;
	*cause = CW_CAUSE_NOTDEFINED;
	if (cw_mutex_lock(&chlock))
	{
		cw_log(LOG_WARNING, "Unable to lock channel list\n");
		return NULL;
	}

	chan = backends;
	while (chan)
	{
	    	if (!strcasecmp(type, chan->tech->type))
    		{
			capabilities = chan->tech->capabilities;
			fmt = format;
			res = cw_translator_best_choice(&fmt, &capabilities);
			if (res < 0)
        		{
				cw_log(LOG_WARNING, "No translator path exists for channel type %s (native %d) to %d\n", type, chan->tech->capabilities, format);
				cw_mutex_unlock(&chlock);
				return NULL;
			}
			cw_mutex_unlock(&chlock);
			if (chan->tech->requester)
				c = chan->tech->requester(type, capabilities, data, cause);
			if (c)
        		{
				if (c->_state == CW_STATE_DOWN)
            			{
					manager_event(EVENT_FLAG_CALL, "Newchannel",
					"Channel: %s\r\n"
					"State: %s\r\n"
					"CallerID: %s\r\n"
					"CallerIDName: %s\r\n"
					"Uniqueid: %s\r\n"
					"Type: %s\r\n"
					"Dialstring: %s\r\n",					
					c->name, cw_state2str(c->_state), c->cid.cid_num ? c->cid.cid_num : "<unknown>", c->cid.cid_name ? c->cid.cid_name : "<unknown>",c->uniqueid,type,(char *)data);
				}
			}
			return c;
		}
		chan = chan->next;
	}
	if (!chan)
	{
		cw_log(LOG_WARNING, "No channel type registered for '%s'\n", type);
		*cause = CW_CAUSE_NOSUCHDRIVER;
	}
	cw_mutex_unlock(&chlock);
	return c;
}

int cw_call(struct cw_channel *chan, char *addr, int timeout) 
{
	/* Place an outgoing call, but don't wait any longer than timeout ms before returning. 
	   If the remote end does not answer within the timeout, then do NOT hang up, but 
	   return anyway.  */
	int res = -1;

	/* Stop if we're a zombie or need a soft hangup */
	cw_mutex_lock(&chan->lock);
	if (!cw_test_flag(chan, CW_FLAG_ZOMBIE)  &&  !cw_check_hangup(chan))
	{
		if (chan->tech->call)
			res = chan->tech->call(chan, addr, timeout);
	}
    cw_mutex_unlock(&chan->lock);
	return res;
}

/*--- cw_transfer: Transfer a call to dest, if the channel supports transfer */
/*	called by app_transfer or the manager interface */
int cw_transfer(struct cw_channel *chan, char *dest) 
{
	int res = -1;

	/* Stop if we're a zombie or need a soft hangup */
	cw_mutex_lock(&chan->lock);
	if (!cw_test_flag(chan, CW_FLAG_ZOMBIE) && !cw_check_hangup(chan))
	{
		if (chan->tech->transfer)
    		{
			res = chan->tech->transfer(chan, dest);
			if (!res)
				res = 1;
		}
    		else
    		{
			res = 0;
    		}
	}
	cw_mutex_unlock(&chan->lock);
	return res;
}

int cw_readstring(struct cw_channel *c, char *s, int len, int timeout, int ftimeout, char *enders)
{
	int pos = 0;
	int to = ftimeout;
	int d;

	/* XXX Merge with full version? XXX */
	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(c, CW_FLAG_ZOMBIE)  ||  cw_check_hangup(c)) 
		return -1;
	if (!len)
		return -1;
	for (;;)
	{
		if (c->stream)
    		{
			d = cw_waitstream(c, CW_DIGIT_ANY);
			cw_stopstream(c);
			usleep(1000);
			if (!d)
				d = cw_waitfordigit(c, to);
		}
        else
    		{
			d = cw_waitfordigit(c, to);
		}
		if (d < 0)
			return -1;
		if (d == 0)
    		{
			s[pos]='\0';
			return 1;
		}
		if (!strchr(enders, d))
			s[pos++] = d;
		if (strchr(enders, d)  ||  (pos >= len))
    		{
			s[pos]='\0';
			return 0;
		}
		to = timeout;
	}
	/* Never reached */
	return 0;
}

int cw_readstring_full(struct cw_channel *c, char *s, int len, int timeout, int ftimeout, char *enders, int audiofd, int ctrlfd)
{
	int pos = 0;
	int to = ftimeout;
	int d;

	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(c, CW_FLAG_ZOMBIE)  ||  cw_check_hangup(c)) 
		return -1;
	if (!len)
		return -1;
	for (;;)
	{
		if (c->stream)
    		{
			d = cw_waitstream_full(c, CW_DIGIT_ANY, audiofd, ctrlfd);
			cw_stopstream(c);
			usleep(1000);
			if (!d)
				d = cw_waitfordigit_full(c, to, audiofd, ctrlfd);
		}
    		else
    		{
			d = cw_waitfordigit_full(c, to, audiofd, ctrlfd);
		}
		if (d < 0)
			return -1;
		if (d == 0)
    		{
			s[pos]='\0';
			return 1;
		}
		if (d == 1)
    		{
			s[pos]='\0';
			return 2;
		}
		if (!strchr(enders, d))
			s[pos++] = d;
		if (strchr(enders, d)  ||  (pos >= len))
    		{
			s[pos]='\0';
			return 0;
		}
		to = timeout;
	}
	/* Never reached */
	return 0;
}

int cw_channel_supports_html(struct cw_channel *chan)
{
	if (chan->tech->send_html)
		return 1;
	return 0;
}

int cw_channel_sendhtml(struct cw_channel *chan, int subclass, const char *data, int datalen)
{
	if (chan->tech->send_html)
		return chan->tech->send_html(chan, subclass, data, datalen);
	return -1;
}

int cw_channel_sendurl(struct cw_channel *chan, const char *url)
{
	if (chan->tech->send_html)
		return chan->tech->send_html(chan, CW_HTML_URL, url, strlen(url) + 1);
	return -1;
}

int cw_channel_make_compatible(struct cw_channel *chan, struct cw_channel *peer)
{
	int src;
	int dst;

	/* Set up translation from the chan to the peer */
	src = chan->nativeformats;
	dst = peer->nativeformats;
	if (cw_translator_best_choice(&dst, &src) < 0)
    {
		cw_log(LOG_WARNING, "No path to translate from %s(%d) to %s(%d)\n", chan->name, src, peer->name, dst);
		return -1;
	}

	/* if the best path is not 'pass through', then
	   transcoding is needed; if desired, force transcode path
	   to use SLINEAR between channels */
	if ((src != dst) && option_transcode_slin)
		dst = CW_FORMAT_SLINEAR;
	if (cw_set_read_format(chan, src) < 0)
    {
		cw_log(LOG_WARNING, "Unable to set read format on channel %s to %d\n", chan->name, dst);
		return -1;
	}
	if (cw_set_write_format(peer, src) < 0)
    {
		cw_log(LOG_WARNING, "Unable to set write format on channel %s to %d\n", peer->name, dst);
		return -1;
	}

	/* Set up translation from the peer to the chan */
	src = peer->nativeformats;
	dst = chan->nativeformats;
	if (cw_translator_best_choice(&dst, &src) < 0)
    {
		cw_log(LOG_WARNING, "No path to translate from %s(%d) to %s(%d)\n", peer->name, src, chan->name, dst);
		return -1;
	}
	/* if the best path is not 'pass through', then
	   transcoding is needed; if desired, force transcode path
	   to use SLINEAR between channels */
	if ((src != dst) && option_transcode_slin)
		dst = CW_FORMAT_SLINEAR;
	if (cw_set_read_format(peer, dst) < 0)
    {
		cw_log(LOG_WARNING, "Unable to set read format on channel %s to %d\n", peer->name, dst);
		return -1;
	}
	if (cw_set_write_format(chan, dst) < 0)
    {
		cw_log(LOG_WARNING, "Unable to set write format on channel %s to %d\n", chan->name, dst);
		return -1;
	}
	return 0;
}

int cw_channel_masquerade(struct cw_channel *original, struct cw_channel *clone)
{
    struct cw_frame null = { CW_FRAME_NULL, };
    int res = -1;

    if (original == clone)
    {
		cw_log(LOG_WARNING, "Can't masquerade channel '%s' into itself!\n", original->name);
		return -1;
    }

    cw_mutex_lock(&original->lock);
    while (cw_mutex_trylock(&clone->lock))
    {
		cw_mutex_unlock(&original->lock);
		usleep(1);
		cw_mutex_lock(&original->lock);
    }
    cw_log(LOG_DEBUG, "Planning to masquerade channel %s into the structure of %s\n",
    	clone->name, original->name);

    if (original->masq)
    {
	cw_log(LOG_WARNING, "%s is already going to masquerade as %s\n", 
    	original->masq->name, original->name);
    }
    else if (clone->masqr)
    {
		cw_log(LOG_WARNING, "%s is already going to masquerade as %s\n", 
			clone->name, clone->masqr->name);
    }
    else
    {
		original->masq = clone;
		clone->masqr = original;
		cw_queue_frame(original, &null);
		cw_queue_frame(clone, &null);
		cw_log(LOG_DEBUG, "Done planning to masquerade channel %s into the structure of %s\n", clone->name, original->name);
		res = 0;
    }
    cw_mutex_unlock(&clone->lock);
    cw_mutex_unlock(&original->lock);
    return res;
}

void cw_change_name(struct cw_channel *chan, char *newname)
{
	char tmp[256];
	cw_copy_string(tmp, chan->name, sizeof(tmp));
	cw_copy_string(chan->name, newname, sizeof(chan->name));
	manager_event(EVENT_FLAG_CALL, "Rename", "Oldname: %s\r\nNewname: %s\r\nUniqueid: %s\r\n", tmp, chan->name, chan->uniqueid);
}

void cw_channel_inherit_variables(const struct cw_channel *parent, struct cw_channel *child)
{
	struct cw_var_t *current, *newvar;
	char *varname;

	CW_LIST_TRAVERSE(&parent->varshead, current, entries)
    {
		int vartype = 0;

		varname = cw_var_full_name(current);
		if (!varname)
			continue;

		if (varname[0] == '_')
        {
			vartype = 1;
			if (varname[1] == '_')
				vartype = 2;
		}

		switch (vartype)
        {
		case 1:
			newvar = cw_var_assign(&varname[1], cw_var_value(current));
			if (newvar)
            {
				CW_LIST_INSERT_TAIL(&child->varshead, newvar, entries);
				if (option_debug)
					cw_log(LOG_DEBUG, "Copying soft-transferable variable %s.\n", cw_var_name(newvar));
			}
			break;
		case 2:
			newvar = cw_var_assign(cw_var_full_name(current), cw_var_value(current));
			if (newvar)
            {
				CW_LIST_INSERT_TAIL(&child->varshead, newvar, entries);
				if (option_debug)
					cw_log(LOG_DEBUG, "Copying hard-transferable variable %s.\n", cw_var_name(newvar));
			}
			break;
		default:
			if (option_debug)
				cw_log(LOG_DEBUG, "Not copying variable %s.\n", cw_var_name(current));
			break;
		}
	}
}

/* Clone channel variables from 'clone' channel into 'original' channel
   All variables except those related to app_groupcount are cloned
   Variables are actually _removed_ from 'clone' channel, presumably
   because it will subsequently be destroyed.
   Assumes locks will be in place on both channels when called.
*/
   
static void clone_variables(struct cw_channel *original, struct cw_channel *clone)
{

	/* Append variables from clone channel into original channel */
	/* XXX Is this always correct?  We have to in order to keep PROCS working XXX */
	if (CW_LIST_FIRST(&clone->varshead))
		CW_LIST_INSERT_TAIL(&original->varshead, CW_LIST_FIRST(&clone->varshead), entries);
}

/*--- cw_do_masquerade: Masquerade a channel */
/* Assumes channel will be locked when called */
int cw_do_masquerade(struct cw_channel *original)
{
	int x,i;
	int res=0;
	int origstate;
	struct cw_frame *cur, *prev;
	const struct cw_channel_tech *t;
	void *t_pvt;
	struct cw_callerid tmpcid;
	struct cw_channel *clone = original->masq;
	int rformat = original->readformat;
	int wformat = original->writeformat;
	char newn[100];
	char orig[100];
	char masqn[100];
	char zombn[100];

	if (option_debug > 3)
		cw_log(LOG_DEBUG, "Actually Masquerading %s(%d) into the structure of %s(%d)\n",
			clone->name, clone->_state, original->name, original->_state);

	/* XXX This is a seriously wacked out operation.  We're essentially putting the guts of
	   the clone channel into the original channel.  Start by killing off the original
	   channel's backend.   I'm not sure we're going to keep this function, because 
	   while the features are nice, the cost is very high in terms of pure nastiness. XXX */

	/* We need the clone's lock, too */
	cw_mutex_lock(&clone->lock);

	cw_log(LOG_DEBUG, "Got clone lock for masquerade on '%s' at %p\n", clone->name, &clone->lock);

	/* Having remembered the original read/write formats, we turn off any translation on either
	   one */
	free_translation(clone);
	free_translation(original);


	/* Unlink the masquerade */
	original->masq = NULL;
	clone->masqr = NULL;
	
	/* Save the original name */
	cw_copy_string(orig, original->name, sizeof(orig));
	/* Save the new name */
	cw_copy_string(newn, clone->name, sizeof(newn));
	/* Create the masq name */
	snprintf(masqn, sizeof(masqn), "%s<MASQ>", newn);
		
	/* Copy the name from the clone channel */
	cw_copy_string(original->name, newn, sizeof(original->name));

	/* Mangle the name of the clone channel */
	cw_copy_string(clone->name, masqn, sizeof(clone->name));
	
	/* Notify any managers of the change, first the masq then the other */
	manager_event(EVENT_FLAG_CALL, "Rename", "Oldname: %s\r\nNewname: %s\r\nUniqueid: %s\r\n", newn, masqn, clone->uniqueid);
	manager_event(EVENT_FLAG_CALL, "Rename", "Oldname: %s\r\nNewname: %s\r\nUniqueid: %s\r\n", orig, newn, original->uniqueid);

	/* Swap the technlogies */	
	t = original->tech;
	original->tech = clone->tech;
	clone->tech = t;

	t_pvt = original->tech_pvt;
	original->tech_pvt = clone->tech_pvt;
	clone->tech_pvt = t_pvt;

	/* Swap the readq's */
	cur = original->readq;
	original->readq = clone->readq;
	clone->readq = cur;

	/* Swap the alertpipes */
	for (i = 0;  i < 2;  i++)
    {
		x = original->alertpipe[i];
		original->alertpipe[i] = clone->alertpipe[i];
		clone->alertpipe[i] = x;
	}

	/* Swap the raw formats */
	x = original->rawreadformat;
	original->rawreadformat = clone->rawreadformat;
	clone->rawreadformat = x;
	x = original->rawwriteformat;
	original->rawwriteformat = clone->rawwriteformat;
	clone->rawwriteformat = x;

	/* Save any pending frames on both sides.  Start by counting
	 * how many we're going to need... */
	prev = NULL;
	cur = clone->readq;
	x = 0;
	while (cur)
    {
		x++;
		prev = cur;
		cur = cur->next;
	}
	/* If we had any, prepend them to the ones already in the queue, and 
	 * load up the alertpipe */
	if (prev)
    {
		prev->next = original->readq;
		original->readq = clone->readq;
		clone->readq = NULL;
		if (original->alertpipe[1] > -1)
        {
			for (i = 0;  i < x;  i++)
				write(original->alertpipe[1], &x, sizeof(x));
		}
	}
	clone->_softhangup = CW_SOFTHANGUP_DEV;


	/* And of course, so does our current state.  Note we need not
	   call cw_setstate since the event manager doesn't really consider
	   these separate.  We do this early so that the clone has the proper
	   state of the original channel. */
	origstate = original->_state;
	original->_state = clone->_state;
	clone->_state = origstate;

	if (clone->tech->fixup)
    {
		if ((res = clone->tech->fixup(original, clone)))
			cw_log(LOG_WARNING, "Fixup failed on channel %s, strange things may happen.\n", clone->name);
	}

	/* Start by disconnecting the original's physical side */
	if (clone->tech->hangup)
		res = clone->tech->hangup(clone);
	if (res)
    {
		cw_log(LOG_WARNING, "Hangup failed!  Strange things may happen!\n");
		cw_mutex_unlock(&clone->lock);
		return -1;
	}
	
	snprintf(zombn, sizeof(zombn), "%s<ZOMBIE>", orig);
	/* Mangle the name of the clone channel */
	cw_copy_string(clone->name, zombn, sizeof(clone->name));
	manager_event(EVENT_FLAG_CALL, "Rename", "Oldname: %s\r\nNewname: %s\r\nUniqueid: %s\r\n", masqn, zombn, clone->uniqueid);

	/* Update the type. */
	original->type = clone->type;
	t_pvt = original->monitor;
	original->monitor = clone->monitor;
	clone->monitor = t_pvt;
	
	/* Keep the same language.  */
	cw_copy_string(original->language, clone->language, sizeof(original->language));
	/* Copy the FD's */
	for (x = 0;  x < CW_MAX_FDS;  x++)
		original->fds[x] = clone->fds[x];
		
	/* Drop group from original */
	cw_app_group_discard(original);
	clone_variables(original, clone);
	CW_LIST_HEAD_INIT_NOLOCK(&clone->varshead);
	/* Presense of ADSI capable CPE follows clone */
	original->adsicpe = clone->adsicpe;
	/* Bridge remains the same */
	/* CDR fields remain the same */
	/* XXX What about blocking, softhangup, blocker, and lock and blockproc? XXX */
	/* Application and data remain the same */
	/* Clone exception  becomes real one, as with fdno */
	cw_copy_flags(original, clone, CW_FLAG_EXCEPTION);
	original->fdno = clone->fdno;
	/* Schedule context remains the same */
	/* Stream stuff stays the same */
	/* Keep the original state.  The fixup code will need to work with it most likely */

	/* Just swap the whole structures, nevermind the allocations, they'll work themselves
	   out. */
	tmpcid = original->cid;
	original->cid = clone->cid;
	clone->cid = tmpcid;
	
	/* Our native formats are different now */
	original->nativeformats = clone->nativeformats;
	
	/* Context, extension, priority, app data, jump table,  remain the same */
	/* pvt switches.  pbx stays the same, as does next */
	
	/* Set the write format */
	cw_set_write_format(original, wformat);

	/* Set the read format */
	cw_set_read_format(original, rformat);

	/* Copy the music class */
	cw_copy_string(original->musicclass, clone->musicclass, sizeof(original->musicclass));

	cw_log(LOG_DEBUG, "Putting channel %s in %d/%d formats\n", original->name, wformat, rformat);

	/* Okay.  Last thing is to let the channel driver know about all this mess, so he
	   can fix up everything as best as possible */
	if (original->tech->fixup)
    {
		res = original->tech->fixup(clone, original);
		if (res)
        {
			cw_log(LOG_WARNING, "Channel for type '%s' could not fixup channel %s\n",
				original->type, original->name);
			cw_mutex_unlock(&clone->lock);
			return -1;
		}
	}
    else
    {
		cw_log(LOG_WARNING, "Channel type '%s' does not have a fixup routine (for %s)!  Bad things may happen.\n",
                 original->type, original->name);
	}

	/* Now, at this point, the "clone" channel is totally F'd up.  We mark it as
	   a zombie so nothing tries to touch it.  If it's already been marked as a
	   zombie, then free it now (since it already is considered invalid). */
	if (cw_test_flag(clone, CW_FLAG_ZOMBIE))
    {
		cw_log(LOG_DEBUG, "Destroying channel clone '%s'\n", clone->name);
		cw_mutex_unlock(&clone->lock);
		cw_channel_free(clone);
		manager_event(EVENT_FLAG_CALL, "Hangup", 
			"Channel: %s\r\n"
			"Uniqueid: %s\r\n"
			"Cause: %d\r\n"
			"Cause-txt: %s\r\n",
			clone->name, 
			clone->uniqueid, 
			clone->hangupcause,
			cw_cause2str(clone->hangupcause)
			);
	}
    else
    {
		struct cw_frame null_frame = { CW_FRAME_NULL, };
		cw_log(LOG_DEBUG, "Released clone lock on '%s'\n", clone->name);
		cw_set_flag(clone, CW_FLAG_ZOMBIE);
		cw_queue_frame(clone, &null_frame);
		cw_mutex_unlock(&clone->lock);
	}
	
	/* Signal any blocker */
	if (cw_test_flag(original, CW_FLAG_BLOCKING))
		pthread_kill(original->blocker, SIGURG);
	cw_log(LOG_DEBUG, "Done Masquerading %s (%d)\n", original->name, original->_state);
	return 0;
}

void cw_set_callerid(struct cw_channel *chan, const char *callerid, const char *calleridname, const char *ani)
{
	if (callerid)
    {
		if (chan->cid.cid_num)
			free(chan->cid.cid_num);
		if (cw_strlen_zero(callerid))
			chan->cid.cid_num = NULL;
		else
			chan->cid.cid_num = strdup(callerid);
	}
	if (calleridname)
    {
		if (chan->cid.cid_name)
			free(chan->cid.cid_name);
		if (cw_strlen_zero(calleridname))
			chan->cid.cid_name = NULL;
		else
			chan->cid.cid_name = strdup(calleridname);
	}
	if (ani)
    {
		if (chan->cid.cid_ani)
			free(chan->cid.cid_ani);
		if (cw_strlen_zero(ani))
			chan->cid.cid_ani = NULL;
		else
			chan->cid.cid_ani = strdup(ani);
	}
	if (chan->cdr)
		cw_cdr_setcid(chan->cdr, chan);
	manager_event(EVENT_FLAG_CALL, "Newcallerid", 
				"Channel: %s\r\n"
				"CallerID: %s\r\n"
				"CallerIDName: %s\r\n"
				"Uniqueid: %s\r\n"
				"CID-CallingPres: %d (%s)\r\n",
				chan->name, chan->cid.cid_num ? 
				chan->cid.cid_num : "<Unknown>",
				chan->cid.cid_name ? 
				chan->cid.cid_name : "<Unknown>",
				chan->uniqueid,
				chan->cid.cid_pres,
				cw_describe_caller_presentation(chan->cid.cid_pres)
				);
}

int cw_setstate(struct cw_channel *chan, int state)
{
	int oldstate = chan->_state;

	if (oldstate == state)
		return 0;

	chan->_state = state;
	cw_device_state_changed_literal(chan->name);
	manager_event(EVENT_FLAG_CALL,
		      (oldstate == CW_STATE_DOWN) ? "Newchannel" : "Newstate",
		      "Channel: %s\r\n"
		      "State: %s\r\n"
		      "CallerID: %s\r\n"
		      "CallerIDName: %s\r\n"
		      "Uniqueid: %s\r\n",
		      chan->name, cw_state2str(chan->_state), 
		      chan->cid.cid_num ? chan->cid.cid_num : "<unknown>", 
		      chan->cid.cid_name ? chan->cid.cid_name : "<unknown>", 
		      chan->uniqueid);

	return 0;
}

/*--- Find bridged channel */
struct cw_channel *cw_bridged_channel(struct cw_channel *chan)
{
	struct cw_channel *bridged;

        if ( !chan ) return NULL;

	bridged = chan->_bridge;
	if (bridged && bridged->tech->bridged_channel) 
		bridged = bridged->tech->bridged_channel(chan, bridged);
	return bridged;
}

static void bridge_playfile(struct cw_channel *chan, struct cw_channel *peer, char *sound, int remain) 
{
	int res=0, min=0, sec=0,check=0;

	check = cw_autoservice_start(peer);
	if(check) 
		return;

	if (remain > 0)
    {
		if (remain / 60 > 1)
        {
			min = remain / 60;
			sec = remain % 60;
		}
        else
        {
			sec = remain;
		}
	}
	
	if (!strcmp(sound, "timeleft"))
    {
    	/* Queue support */
		res = cw_streamfile(chan, "vm-youhave", chan->language);
		res = cw_waitstream(chan, "");
		if (min)
        {
			res = cw_say_number(chan, min, CW_DIGIT_ANY, chan->language, (char *) NULL);
			res = cw_streamfile(chan, "queue-minutes", chan->language);
			res = cw_waitstream(chan, "");
		}
		if (sec)
        {
			res = cw_say_number(chan, sec, CW_DIGIT_ANY, chan->language, (char *) NULL);
			res = cw_streamfile(chan, "queue-seconds", chan->language);
			res = cw_waitstream(chan, "");
		}
	}
    else
    {
		res = cw_streamfile(chan, sound, chan->language);
		res = cw_waitstream(chan, "");
	}

	check = cw_autoservice_stop(peer);
}

static enum cw_bridge_result cw_generic_bridge(struct cw_channel *c0,
                                                   struct cw_channel *c1,
                                                   struct cw_bridge_config *config,
                                                   struct cw_frame **fo,
                                                   struct cw_channel **rc,
                                                   struct timeval bridge_end)
{
	/* Copy voice back and forth between the two channels. */
	struct cw_channel *cs[3];
	struct cw_frame *f;
	struct cw_channel *who = NULL;
	enum cw_bridge_result res = CW_BRIDGE_COMPLETE;
	int o0nativeformats;
	int o1nativeformats;
	int watch_c0_dtmf;
	int watch_c1_dtmf;
	void *pvt0, *pvt1;
	int to;
	
	/* Indicates whether a frame was queued into a jitterbuffer */
	int frame_put_in_jb;

	cs[0] = c0;
	cs[1] = c1;
	pvt0 = c0->tech_pvt;
	pvt1 = c1->tech_pvt;
	o0nativeformats = c0->nativeformats;
	o1nativeformats = c1->nativeformats;
	watch_c0_dtmf = config->flags & CW_BRIDGE_DTMF_CHANNEL_0;
	watch_c1_dtmf = config->flags & CW_BRIDGE_DTMF_CHANNEL_1;

	/* Check the need of a jitterbuffer for each channel */
	cw_jb_do_usecheck(c0, c1);

	for (;;) {

        /* We get the T38 status of the 2 channels. 
           If at least one is not in a NON t38 state, then retry 
           This will force another native bridge loop
            */
        int res1,res2;
        res1 = cw_channel_get_t38_status(c0);
        res2 = cw_channel_get_t38_status(c1);
        //cw_log(LOG_DEBUG,"genbridge res t38 = %d:%d [%d %d]\n",res1, res2, T38_STATUS_UNKNOWN, T38_OFFER_REJECTED);

        if ( res1!=res2 ) {
            //cw_log(LOG_DEBUG,"Stopping generic bridge because channels have different modes\n");
            usleep(100);
            return CW_BRIDGE_RETRY;
        }

/*
        if ( res1==T38_NEGOTIATED ) {
            cw_log(LOG_DEBUG,"Stopping generic bridge because channel 0 is t38 enabled ( %d != [%d,%d])\n", res1, T38_STATUS_UNKNOWN, T38_OFFER_REJECTED);
            return CW_BRIDGE_RETRY;
        }
        if ( res2==T38_NEGOTIATED ) {
            cw_log(LOG_DEBUG,"Stopping generic bridge because channel 1 is t38 enabled\n");
            return CW_BRIDGE_RETRY;
        }
*/

		if ((c0->tech_pvt != pvt0) || (c1->tech_pvt != pvt1) ||
		    (o0nativeformats != c0->nativeformats) ||
		    (o1nativeformats != c1->nativeformats)) {
			/* Check for Masquerade, codec changes, etc */
			res = CW_BRIDGE_RETRY;
			break;
		}

		if (bridge_end.tv_sec) {
			to = cw_tvdiff_ms(bridge_end, cw_tvnow());
			if (to <= 0) {
				if (config->timelimit)
					res = CW_BRIDGE_RETRY;
				else
					res = CW_BRIDGE_COMPLETE;
				break;
			}
		} else {
			to = -1;
        }

		/* Calculate the appropriate max sleep interval - 
		 * in general, this is the time,
		 * left to the closest jb delivery moment */
		to = cw_jb_get_when_to_wakeup(c0, c1, to);

		who = cw_waitfor_n(cs, 2, &to);
		if (!who) {
			if (!to) {
				res = CW_BRIDGE_RETRY;
				break;
			}

			/* No frame received within the specified timeout - 
			 * check if we have to deliver now */
			cw_jb_get_and_deliver(c0, c1);

			cw_log(LOG_DEBUG, "Nobody there, continuing...\n"); 
			if (c0->_softhangup == CW_SOFTHANGUP_UNBRIDGE || c1->_softhangup == CW_SOFTHANGUP_UNBRIDGE) {
				if (c0->_softhangup == CW_SOFTHANGUP_UNBRIDGE)
					c0->_softhangup = 0;
				if (c1->_softhangup == CW_SOFTHANGUP_UNBRIDGE)
					c1->_softhangup = 0;
				c0->_bridge = c1;
				c1->_bridge = c0;
			}
			continue;
		}
		f = cw_read(who);
		if (!f) {
			*fo = NULL;
			*rc = who;
			res = CW_BRIDGE_COMPLETE;
			cw_log(LOG_DEBUG, "Didn't get a frame from channel: %s\n",who->name);
			break;
		}

		/* Try add the frame info the who's bridged channel jitterbuff */
		frame_put_in_jb = !cw_jb_put((who == c0) ? c1 : c0, f, f->subclass);

		if ((f->frametype == CW_FRAME_CONTROL) && !(config->flags & CW_BRIDGE_IGNORE_SIGS)) {
			if ((f->subclass == CW_CONTROL_HOLD) || (f->subclass == CW_CONTROL_UNHOLD) ||
			    (f->subclass == CW_CONTROL_VIDUPDATE)) {
				cw_indicate(who == c0 ? c1 : c0, f->subclass);
			} else {
				*fo = f;
				*rc = who;
				res =  CW_BRIDGE_COMPLETE;
				cw_log(LOG_DEBUG, "Got a FRAME_CONTROL (%d) frame on channel %s\n", f->subclass, who->name);
				break;
			}
		}
		if ((f->frametype == CW_FRAME_VOICE) ||
		    (f->frametype == CW_FRAME_DTMF) ||
		    (f->frametype == CW_FRAME_VIDEO) || 
		    (f->frametype == CW_FRAME_IMAGE) ||
		    (f->frametype == CW_FRAME_HTML) ||
		    (f->frametype == CW_FRAME_MODEM) ||
		    (f->frametype == CW_FRAME_TEXT)) {

			if (f->frametype == CW_FRAME_DTMF) {
				if (((who == c0) && watch_c0_dtmf) ||
				    ((who == c1) && watch_c1_dtmf)) {
					*rc = who;
					*fo = f;
					res = CW_BRIDGE_COMPLETE;
					cw_log(LOG_DEBUG, "Got DTMF on channel (%s)\n", who->name);
					break;
                                } else if ( f->frametype == CW_FRAME_MODEM ) {
                                        cw_log(LOG_DEBUG, "Got MODEM frame on channel (%s). Exiting generic bridge.\n", who->name);
                                    /* If we got a t38 frame... exit this generic bridge */
                                        return CW_BRIDGE_RETRY;
				} else {
					goto tackygoto;
				}
			} else {
#if 0
				cw_log(LOG_DEBUG, "Read from %s\n", who->name);
				if (who == last) 
					cw_log(LOG_DEBUG, "Servicing channel %s twice in a row?\n", last->name);
				last = who;
#endif
tackygoto:
				/* Write immediately frames, not passed through jb */
				if (!frame_put_in_jb)
					cw_write((who == c0)  ?  c1  :  c0, f);
				
				/* Check if we have to deliver now */
				cw_jb_get_and_deliver(c0, c1);
			}
		}
		cw_fr_free(f);

		/* Swap who gets priority */
		cs[2] = cs[0];
		cs[0] = cs[1];
		cs[1] = cs[2];

	}
	return res;
}

/*--- cw_channel_bridge: Bridge two channels together */
enum cw_bridge_result cw_channel_bridge(struct cw_channel *c0, struct cw_channel *c1, struct cw_bridge_config *config, struct cw_frame **fo, struct cw_channel **rc) 
{
	struct cw_channel *who = NULL;
	enum cw_bridge_result res = CW_BRIDGE_COMPLETE;
	int nativefailed=0;
	int nativeretry=0;
	int firstpass;
	int o0nativeformats;
	int o1nativeformats;
	long time_left_ms=0;
	struct timeval nexteventts = { 0, };
	char caller_warning = 0;
	char callee_warning = 0;
	int to;

	if (c0->_bridge) {
		cw_log(LOG_WARNING, "%s is already in a bridge with %s\n", 
			c0->name, c0->_bridge->name);
		return -1;
	}
	if (c1->_bridge) {
		cw_log(LOG_WARNING, "%s is already in a bridge with %s\n", 
			c1->name, c1->_bridge->name);
		return -1;
	}
	
	/* Stop if we're a zombie or need a soft hangup */
	if (cw_test_flag(c0, CW_FLAG_ZOMBIE) || cw_check_hangup_locked(c0) ||
	    cw_test_flag(c1, CW_FLAG_ZOMBIE) || cw_check_hangup_locked(c1)) 
		return -1;

	*fo = NULL;
	firstpass = config->firstpass;
	config->firstpass = 0;

	if (cw_tvzero(config->start_time))
		config->start_time = cw_tvnow();
	time_left_ms = config->timelimit;

	caller_warning = cw_test_flag(&config->features_caller, CW_FEATURE_PLAY_WARNING);
	callee_warning = cw_test_flag(&config->features_callee, CW_FEATURE_PLAY_WARNING);

	if (config->start_sound && firstpass) {
		if (caller_warning)
			bridge_playfile(c0, c1, config->start_sound, time_left_ms / 1000);
		if (callee_warning)
			bridge_playfile(c1, c0, config->start_sound, time_left_ms / 1000);
	}

	/* Keep track of bridge */
	c0->_bridge = c1;
	c1->_bridge = c0;
	
	manager_event(EVENT_FLAG_CALL, "Link", 
		      "Channel1: %s\r\n"
		      "Channel2: %s\r\n"
		      "Uniqueid1: %s\r\n"
		      "Uniqueid2: %s\r\n"
		      "CallerID1: %s\r\n"
		      "CallerID2: %s\r\n",
		      c0->name, c1->name, c0->uniqueid, c1->uniqueid, c0->cid.cid_num, c1->cid.cid_num);
                                                                        
	o0nativeformats = c0->nativeformats;
	o1nativeformats = c1->nativeformats;

	if (config->timelimit) {
		nexteventts = cw_tvadd(config->start_time, cw_samp2tv(config->timelimit, 1000));
		if (caller_warning || callee_warning)
			nexteventts = cw_tvsub(nexteventts, cw_samp2tv(config->play_warning, 1000));
	}

	for (/* ever */;;) {
		to = -1;
		if (config->timelimit) {
			struct timeval now;
			now = cw_tvnow();
			to = cw_tvdiff_ms(nexteventts, now);
			if (to < 0)
				to = 0;
			time_left_ms = config->timelimit - cw_tvdiff_ms(now, config->start_time);
			if (time_left_ms < to)
				to = time_left_ms;

			if (time_left_ms <= 0) {
				if (caller_warning && config->end_sound)
					bridge_playfile(c0, c1, config->end_sound, 0);
				if (callee_warning && config->end_sound)
					bridge_playfile(c1, c0, config->end_sound, 0);
				*fo = NULL;
				if (who) 
					*rc = who;
				res = 0;
				break;
			}
			
			if (!to) {
				if (time_left_ms >= 5000) {
					if (caller_warning && config->warning_sound && config->play_warning)
						bridge_playfile(c0, c1, config->warning_sound, time_left_ms / 1000);
					if (callee_warning && config->warning_sound && config->play_warning)
						bridge_playfile(c1, c0, config->warning_sound, time_left_ms / 1000);
				}
				if (config->warning_freq) {
					nexteventts = cw_tvadd(nexteventts, cw_samp2tv(config->warning_freq, 1000));
				} else
					nexteventts = cw_tvadd(config->start_time, cw_samp2tv(config->timelimit, 1000));
			}
		}

		if (c0->_softhangup == CW_SOFTHANGUP_UNBRIDGE || c1->_softhangup == CW_SOFTHANGUP_UNBRIDGE) {
			if (c0->_softhangup == CW_SOFTHANGUP_UNBRIDGE)
				c0->_softhangup = 0;
			if (c1->_softhangup == CW_SOFTHANGUP_UNBRIDGE)
				c1->_softhangup = 0;
			c0->_bridge = c1;
			c1->_bridge = c0;
			cw_log(LOG_DEBUG, "Unbridge signal received. Ending native bridge.\n");
			continue;
		}
		
		/* Stop if we're a zombie or need a soft hangup */
		if (cw_test_flag(c0, CW_FLAG_ZOMBIE) || cw_check_hangup_locked(c0) ||
		    cw_test_flag(c1, CW_FLAG_ZOMBIE) || cw_check_hangup_locked(c1))
    		{
			*fo = NULL;
			if (who)
				*rc = who;
			res = 0;
			cw_log(LOG_DEBUG, "Bridge stops because we're zombie or need a soft hangup: c0=%s, c1=%s, flags: %s,%s,%s,%s\n",
				c0->name, c1->name,
				cw_test_flag(c0, CW_FLAG_ZOMBIE) ? "Yes" : "No",
				cw_check_hangup(c0) ? "Yes" : "No",
				cw_test_flag(c1, CW_FLAG_ZOMBIE) ? "Yes" : "No",
				cw_check_hangup(c1) ? "Yes" : "No");
			break;
		}

                nativeretry=0;

		if (c0->tech->bridge &&
		    (config->timelimit == 0) &&
		    (c0->tech->bridge == c1->tech->bridge) &&
		    !nativefailed && !c0->monitor && !c1->monitor && !c0->spiers && !c1->spiers)
    		{
			/* Looks like they share a bridge method */
			if (option_verbose > 2) 
				cw_log(LOG_DEBUG,"Attempting native bridge of %s and %s\n", c0->name, c1->name);
			cw_set_flag(c0, CW_FLAG_NBRIDGE);
			cw_set_flag(c1, CW_FLAG_NBRIDGE);
			if ((res = c0->tech->bridge(c0, c1, config->flags, fo, rc, to)) == CW_BRIDGE_COMPLETE)
        		{
				manager_event(EVENT_FLAG_CALL, "Unlink", 
					      "Channel1: %s\r\n"
					      "Channel2: %s\r\n"
					      "Uniqueid1: %s\r\n"
					      "Uniqueid2: %s\r\n"
					      "CallerID1: %s\r\n"
					      "CallerID2: %s\r\n",
					      c0->name, c1->name, c0->uniqueid, c1->uniqueid, c0->cid.cid_num, c1->cid.cid_num);
				cw_log(LOG_DEBUG, "Returning from native bridge, channels: %s, %s\n", c0->name, c1->name);

				cw_clear_flag(c0, CW_FLAG_NBRIDGE);
				cw_clear_flag(c1, CW_FLAG_NBRIDGE);

				if (c0->_softhangup == CW_SOFTHANGUP_UNBRIDGE || c1->_softhangup == CW_SOFTHANGUP_UNBRIDGE)
					continue;

				c0->_bridge = NULL;
				c1->_bridge = NULL;

				return res;
			}
        		else
        		{
				cw_clear_flag(c0, CW_FLAG_NBRIDGE);
				cw_clear_flag(c1, CW_FLAG_NBRIDGE);
			}
			
			if (res == CW_BRIDGE_RETRY)
				continue;

			switch (res)
        		{
			    case CW_BRIDGE_RETRY:
				break;
			    case CW_BRIDGE_RETRY_NATIVE:
                                nativeretry++;
				break;
			    default:
				cw_log(LOG_WARNING, "Private bridge between %s and %s failed\n", c0->name, c1->name);
				/* fallthrough */
			    case CW_BRIDGE_FAILED_NOWARN:
				nativefailed++;
				break;
			}
		}
	
		if (((c0->writeformat != c1->readformat) || (c0->readformat != c1->writeformat) ||
		    (c0->nativeformats != o0nativeformats) || (c1->nativeformats != o1nativeformats)) &&
		    !(cw_generator_is_active(c0) || cw_generator_is_active(c1)))
    		{
			if (cw_channel_make_compatible(c0, c1))
        		{
				cw_log(LOG_WARNING, "Can't make %s and %s compatible\n", c0->name, c1->name);
                                manager_event(EVENT_FLAG_CALL, "Unlink",
					      "Channel1: %s\r\n"
					      "Channel2: %s\r\n"
					      "Uniqueid1: %s\r\n"
					      "Uniqueid2: %s\r\n"
					      "CallerID1: %s\r\n"
					      "CallerID2: %s\r\n",
					      c0->name, c1->name, c0->uniqueid, c1->uniqueid, c0->cid.cid_num, c1->cid.cid_num);
				return CW_BRIDGE_FAILED;
			}
			o0nativeformats = c0->nativeformats;
			o1nativeformats = c1->nativeformats;
		}
		res = cw_generic_bridge(c0, c1, config, fo, rc, nexteventts);
		if (res != CW_BRIDGE_RETRY)
			break;
	}

	c0->_bridge = NULL;
	c1->_bridge = NULL;

	manager_event(EVENT_FLAG_CALL, "Unlink",
		      "Channel1: %s\r\n"
		      "Channel2: %s\r\n"
		      "Uniqueid1: %s\r\n"
		      "Uniqueid2: %s\r\n"
		      "CallerID1: %s\r\n"
		      "CallerID2: %s\r\n",
		      c0->name, c1->name, c0->uniqueid, c1->uniqueid, c0->cid.cid_num, c1->cid.cid_num);
	cw_log(LOG_DEBUG, "Bridge stops bridging channels %s and %s\n", c0->name, c1->name);

	return res;
}

/*--- cw_channel_setoption: Sets an option on a channel */
int cw_channel_setoption(struct cw_channel *chan, int option, void *data, int datalen, int block)
{
	int res;

	if (chan->tech->setoption)
	{
		res = chan->tech->setoption(chan, option, data, datalen);
		if (res < 0)
			return res;
	}
	else
	{
		errno = ENOSYS;
		return -1;
	}
	if (block)
	{
		/* XXX Implement blocking -- just wait for our option frame reply, discarding
		   intermediate packets. XXX */
		cw_log(LOG_ERROR, "XXX Blocking not implemented yet XXX\n");
		return -1;
	}
	return 0;
}

struct tonepair_def
{
    tone_gen_descriptor_t tone_desc;
};

struct tonepair_state
{
    tone_gen_state_t tone_state;
	int origwfmt;
	struct cw_frame f;
	uint8_t offset[CW_FRIENDLY_OFFSET];
	int16_t data[4000];
};

static void tonepair_release(struct cw_channel *chan, void *params)
{
	struct tonepair_state *ts = params;

	if (chan)
		cw_set_write_format(chan, ts->origwfmt);
	free(ts);
}

static void *tonepair_alloc(struct cw_channel *chan, void *params)
{
	struct tonepair_state *ts;
	struct tonepair_def *td = params;

	if ((ts = malloc(sizeof(*ts))) == NULL)
		return NULL;
	memset(ts, 0, sizeof(*ts));
	ts->origwfmt = chan->writeformat;
	if (cw_set_write_format(chan, CW_FORMAT_SLINEAR))
	{
		cw_log(LOG_WARNING, "Unable to set '%s' to signed linear format (write)\n", chan->name);
		tonepair_release(NULL, ts);
		return NULL;
	}
	tone_gen_init(&ts->tone_state, &td->tone_desc);
	cw_set_flag(chan, CW_FLAG_WRITE_INT);
	return ts;
}

static int tonepair_generate(struct cw_channel *chan, void *data, int samples)
{
	struct tonepair_state *ts = data;
	int len;
	int x;

	len = samples*sizeof(int16_t);
	if (len > sizeof(ts->data)/sizeof(int16_t) - 1)
	{
		cw_log(LOG_WARNING, "Can't generate that much data!\n");
		return -1;
	}
	memset(&ts->f, 0, sizeof(ts->f));
	cw_fr_init_ex(&ts->f, CW_FRAME_VOICE, CW_FORMAT_SLINEAR, NULL);
	ts->f.datalen = len;
	ts->f.samples = samples;
	ts->f.offset = CW_FRIENDLY_OFFSET;
	ts->f.data = ts->data;
        x = tone_gen(&ts->tone_state, ts->data, samples);
	cw_write(chan, &ts->f);
	if (x < samples)
	    return -1;
	return 0;
}

static struct cw_generator tonepair =
{
	alloc: tonepair_alloc,
	release: tonepair_release,
	generate: tonepair_generate,
};

int cw_tonepair_start(struct cw_channel *chan, int freq1, int freq2, int duration, int vol)
{

    struct tonepair_def d;

    if (vol >= 0)
	vol = -13;

    if (duration == 0)
    {
        make_tone_gen_descriptor(&d.tone_desc,
                                 freq1,
                                 vol,
                                 freq2,
                                 vol,
                                 1,
                                 0,
                                 0,
                                 0,
                                 1);
    }
    else
    {
        make_tone_gen_descriptor(&d.tone_desc,
                                 freq1,
                                 vol,
                                 freq2,
                                 vol,
                                 duration*8,
                                 0,
                                 0,
                                 0,
                                 0);
    }
    if (cw_generator_activate(chan, &tonepair, &d))
		return -1;
	return 0;
}

void cw_tonepair_stop(struct cw_channel *chan)
{
	cw_generator_deactivate(chan);
}

int cw_tonepair(struct cw_channel *chan, int freq1, int freq2, int duration, int vol)
{
	int res;

	if ((res = cw_tonepair_start(chan, freq1, freq2, duration, vol)))
		return res;

	/* Don't return to caller until after duration has passed */
	cw_safe_sleep(chan, duration);
        cw_tonepair_stop(chan);
        return 0;
}

cw_group_t cw_get_group(char *s)
{
	char *piece;
	char *c = NULL;
	int start = 0;
    int finish = 0;
    int x;
	cw_group_t group = 0;

	c = cw_strdupa(s);

    while ((piece = strsep(&c, ",")))
    {
	if (sscanf(piece, "%d-%d", &start, &finish) == 2)
    	{
		/* Range */
	}
        else if (sscanf(piece, "%d", &start))
        {
			/* Just one */
			finish = start;
	}
        else
        {
			cw_log(LOG_ERROR, "Syntax error parsing group configuration '%s' at '%s'. Ignoring.\n", s, piece);
			continue;
	}
	for (x = start;  x <= finish;  x++)
        {
			if ((x > 63)  ||  (x < 0))
				cw_log(LOG_WARNING, "Ignoring invalid group %d (maximum group is 63)\n", x);
        		else
				group |= ((cw_group_t) 1 << x);
	}
    }
    return group;
}

static int (*cw_moh_start_ptr)(struct cw_channel *, char *) = NULL;
static void (*cw_moh_stop_ptr)(struct cw_channel *) = NULL;
static void (*cw_moh_cleanup_ptr)(struct cw_channel *) = NULL;

void cw_install_music_functions(int (*start_ptr)(struct cw_channel *, char *),
								 void (*stop_ptr)(struct cw_channel *),
								 void (*cleanup_ptr)(struct cw_channel *)
								 ) 
{
	cw_moh_start_ptr = start_ptr;
	cw_moh_stop_ptr = stop_ptr;
	cw_moh_cleanup_ptr = cleanup_ptr;
}

void cw_uninstall_music_functions(void) 
{
	cw_moh_start_ptr = NULL;
	cw_moh_stop_ptr = NULL;
	cw_moh_cleanup_ptr = NULL;
}

/*! Turn on music on hold on a given channel */
int cw_moh_start(struct cw_channel *chan, char *mclass) 
{
	if (cw_moh_start_ptr)
		return cw_moh_start_ptr(chan, mclass);

	if (option_verbose > 2)
		cw_verbose(VERBOSE_PREFIX_3 "Music class %s requested but no musiconhold loaded.\n", mclass ? mclass : "default");
	
	return 0;
}

/*! Turn off music on hold on a given channel */
void cw_moh_stop(struct cw_channel *chan) 
{
	if(cw_moh_stop_ptr)
		cw_moh_stop_ptr(chan);
}

void cw_moh_cleanup(struct cw_channel *chan) 
{
	if(cw_moh_cleanup_ptr)
        cw_moh_cleanup_ptr(chan);
}

void cw_channels_init(void)
{
	cw_cli_register(&cli_show_channeltypes);
}

/*--- cw_print_group: Print call group and pickup group ---*/
char *cw_print_group(char *buf, int buflen, cw_group_t group) 
{
	unsigned int i;
	int first=1;
	char num[3];

	buf[0] = '\0';
	
	if (!group)	/* Return empty string if no group */
		return(buf);

	for (i=0; i<=63; i++) {	/* Max group is 63 */
		if (group & ((cw_group_t) 1 << i)) {
	   		if (!first) {
				strncat(buf, ", ", buflen);
			} else {
				first=0;
	  		}
			snprintf(num, sizeof(num), "%u", i);
			strncat(buf, num, buflen);
		}
	}
	return(buf);
}

void cw_set_variables(struct cw_channel *chan, struct cw_variable *vars)
{
	struct cw_variable *cur;

	for (cur = vars; cur; cur = cur->next)
		pbx_builtin_setvar_helper(chan, cur->name, cur->value);	
}

/* If you are calling carefulwrite, it is assumed that you are calling
   it on a file descriptor that _DOES_ have NONBLOCK set.  This way,
   there is only one system call made to do a write, unless we actually
   have a need to wait.  This way, we get better performance. */
int cw_carefulwrite(int fd, char *s, int len, int timeoutms)
{
	/* Try to write string, but wait no more than ms milliseconds before timing out */
	int res = 0, n;
	
	while (len) {
		while ((res = write(fd, s, len)) < 0 && errno == EINTR);
		if (res < 0) {
			if (errno != EAGAIN)
				break;
			res = 0;
		}

		len -= res;
		s += res;

		if (len) {
			struct pollfd pfd;

			pfd.fd = fd;
			pfd.events = POLLOUT;
			res = -1;
			while ((n = poll(&pfd, 1, timeoutms)) < 0 && errno == EINTR);
			if (n < 1)
				break;
		}
	}

	return res;
}

/*--- cw_spy_empty_queues: Quickly empty both queues and return the frames. */
/** Mark both queues as empty and return the frames.
 * @param spy The (unlocked) spy to empty.
 * @param f0 The pointer for the frames of the first queue.
 * @param f1 The pointer for the frames of the second queue.
 */
void cw_spy_empty_queues(struct cw_channel_spy *spy, struct cw_frame **f0, struct cw_frame **f1)
{
    cw_mutex_lock(&spy->lock);
    *f0 = spy->queue[0].head;
    *f1 = spy->queue[1].head;
    spy->queue[0].head = spy->queue[0].tail = NULL;
    spy->queue[1].head = spy->queue[1].tail = NULL;
    spy->queue[0].count = spy->queue[1].count = 0;
    cw_mutex_unlock(&spy->lock);
}

#if defined(DEBUG_CHANNEL_LOCKS)
int cw_channel_unlock(struct cw_channel *chan)
{
	int res = 0;

	if (option_debug > 2) 
		cw_log(LOG_DEBUG, "::::==== Unlocking CW channel %s\n", chan->name);
	
	if (!chan)
	{
		if (option_debug)
			cw_log(LOG_DEBUG, "::::==== Unlocking non-existing channel \n");
		return 0;
	}

	res = cw_mutex_unlock(&chan->lock);

	if (option_debug > 2)
	{
#ifdef DEBUG_THREADS
		int count = 0;
		
        if ((count = chan->lock.reentrancy))
			cw_log(LOG_DEBUG, ":::=== Still have %d locks (recursive)\n", count);
#endif
		if (!res)
    		{
			if (option_debug)
				cw_log(LOG_DEBUG, "::::==== Channel %s was unlocked\n", chan->name);
		}
        if (res == EINVAL)
    		{
			if (option_debug)
				cw_log(LOG_DEBUG, "::::==== Channel %s had no lock by this thread. Failed unlocking\n", chan->name);
		}
	}
	if (res == EPERM)
	{
		/* We had no lock, so okay any way*/
		if (option_debug > 3)
			cw_log(LOG_DEBUG, "::::==== Channel %s was not locked at all \n", chan->name);
		res = 0;
	}
	return res;
}

int cw_channel_lock(struct cw_channel *chan)
{
	int res;

	if (option_debug > 3)
		cw_log(LOG_DEBUG, "====:::: Locking CW channel %s\n", chan->name);

	res = cw_mutex_lock(&chan->lock);

	if (option_debug > 3)
	{
#ifdef DEBUG_THREADS
		int count = 0;
	
    	if ((count = chan->lock.reentrancy))
			cw_log(LOG_DEBUG, ":::=== Now have %d locks (recursive)\n", count);
#endif
		if (!res)
			cw_log(LOG_DEBUG, "::::==== Channel %s was locked\n", chan->name);
		if (res == EDEADLK)
    		{
		    /* We had no lock, so okey any way */
		    if (option_debug > 3)
			    cw_log(LOG_DEBUG, "::::==== Channel %s was not locked by us. Lock would cause deadlock.\n", chan->name);
		}
		if (res == EINVAL)
    		{
			if (option_debug > 3)
				cw_log(LOG_DEBUG, "::::==== Channel %s lock failed. No mutex.\n", chan->name);
		}
	}
	return res;
}

int cw_channel_trylock(struct cw_channel *chan)
{
	int res;

	if (option_debug > 2)
		cw_log(LOG_DEBUG, "====:::: Trying to lock CW channel %s\n", chan->name);

	res = cw_mutex_trylock(&chan->lock);

	if (option_debug > 2)
	{
#ifdef DEBUG_THREADS
		int count = 0;

		if ((count = chan->lock.reentrancy))
			cw_log(LOG_DEBUG, ":::=== Now have %d locks (recursive)\n", count);
#endif
		if (!res)
			cw_log(LOG_DEBUG, "::::==== Channel %s was locked\n", chan->name);
		if (res == EBUSY)
    		{
			/* We failed to lock */
			if (option_debug > 2)
				cw_log(LOG_DEBUG, "::::==== Channel %s failed to lock. Not waiting around...\n", chan->name);
		}
		if (res == EDEADLK)
    		{
			/* We had no lock, so okey any way*/
			if (option_debug > 2)
				cw_log(LOG_DEBUG, "::::==== Channel %s was not locked. Lock would cause deadlock.\n", chan->name);
		}
		if (res == EINVAL && option_debug > 2)
			cw_log(LOG_DEBUG, "::::==== Channel %s lock failed. No mutex.\n", chan->name);
	}
	return res;
}
#endif
