//#define DO_TRACE
/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Fax Channel Driver
 * 
 * Copyright (C) 2005 Anthony Minessale II
 *
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/tcp.h>

#include "callweaver/lock.h"
#include "callweaver/cli.h"
#include "callweaver/channel.h"
#include "callweaver/logger.h"
#include "callweaver/cwobj.h"
#include "callweaver/module.h"
#include "callweaver/lock.h"
#include "callweaver/pbx.h"
#include "callweaver/devicestate.h"
#include "faxmodem.h"

static const char desc[] = "Fax Modem Interface";
static const char type[] = "Fax";
static const char tdesc[] = "Fax Modem Interface";

#define CONFIGFILE "chan_fax.conf"
#define DSP_BUFFER_MAXSIZE 2048
#define SAMPLES 160
#define MS 20

static volatile int THREADCOUNT = 0;
CW_MUTEX_DEFINE_STATIC(threadcount_lock);

#define MAX_FAXMODEMS 512
#define TIMEOUT 30000

#define VBPREFIX "CHAN FAX: "
static volatile struct faxmodem FAXMODEM_POOL[MAX_FAXMODEMS] = {};
static int SOFT_TIMEOUT = TIMEOUT;
static int SOFT_MAX_FAXMODEMS = MAX_FAXMODEMS;
static int READY = 0;
char *DEVICE_PREFIX;
static char *CONTEXT = NULL;
static int VBLEVEL = 0;

static const char *TERMINATOR = "\r\n";

#define IO_READ "1"
#define IO_HUP "0"
#define IO_PROD "2"
#define IO_ANSWER "3"

/* some flags */
typedef enum {
	TFLAG_PBX = (1 << 0),
	TFLAG_OUTBOUND = (1 << 1),
	TFLAG_EVENT = (1 << 2),
	TFLAG_DATA = (1 << 3)
} TFLAGS;


/* Modem ringing strategy */
enum {
	RING_STRATEGY_FF,  /* First-free */
	RING_STRATEGY_RR,  /* Roundrobin */
} ring_strategy;
int rr_next;

/* The following object is where you can attach your technology-specific state data.
 * Add as many members as you like and the data will be available to you in all of the methods.
 *
 * In the 'requester' method you need to allocate both a channel and a private_object
 * In in the 'hangup' method you must detach and destroy the private_object but not the channel
 * that is done for you.  Somewhat of an asyncronous life cycle *shrug*
 */

struct private_object {
	CWOBJ_COMPONENTS(struct private_object);	/* Object Abstraction Stuff */
	unsigned int flags;							/* FLAGS */
	struct cw_frame frame;						/* Frame for Writing */
	short fdata[(SAMPLES * 2) + CW_FRIENDLY_OFFSET];
	int flen;
	struct cw_channel *owner;					/* Pointer to my owner (the abstract channel object) */
	volatile struct faxmodem *fm;
	int pipe[2];
#ifdef DO_TRACE
	int debug[2];
#endif
        char *cid_num;
        char *cid_name;

	/* If this is true it means we already sent a hangup-statusmessage
	 * to our user, so don't bother with it when hanging up
	 */
        int hangup_msg_sent; 

	/* Condition variable for signalling new data */
	cw_cond_t data_cond;

};



/* This object is a container for the list of private objects it is simple because of the CWOBJ stuff */
static struct private_object_container {
	CWOBJ_CONTAINER_COMPONENTS(struct private_object);
} private_object_list;

static int usecnt = 0;
CW_MUTEX_DEFINE_STATIC(usecnt_lock);
CW_MUTEX_DEFINE_STATIC(control_lock);
CW_MUTEX_DEFINE_STATIC(data_lock);

/********************CHANNEL METHOD PROTOTYPES*******************
 * You may or may not need all of these methods, remove any unnecessary functions/protos/mappings as needed.
 *
 */
static struct cw_channel *tech_requester(const char *type, int format, void *data, int *cause);
static int tech_devicestate(void *data);
static int tech_send_digit(struct cw_channel *self, char digit);
static int tech_call(struct cw_channel *self, char *dest, int timeout);
static int tech_hangup(struct cw_channel *self);
static int tech_answer(struct cw_channel *self);
static struct cw_frame *tech_read(struct cw_channel *self);
static struct cw_frame *tech_exception(struct cw_channel *self);
static int tech_write(struct cw_channel *self, struct cw_frame *frame);
static int tech_indicate(struct cw_channel *self, int condition);
static int tech_fixup(struct cw_channel *oldchan, struct cw_channel *newchan);
static int tech_send_html(struct cw_channel *self, int subclass, const char *data, int datalen);
static int tech_send_text(struct cw_channel *self, const char *text);
static int tech_send_image(struct cw_channel *self, struct cw_frame *frame);
static int tech_setoption(struct cw_channel *self, int option, void *data, int datalen);
static int tech_queryoption(struct cw_channel *self, int option, void *data, int *datalen);
static struct cw_channel *tech_bridged_channel(struct cw_channel *self, struct cw_channel *bridge);
static int tech_transfer(struct cw_channel *self, const char *newdest);
static int tech_write_video(struct cw_channel *self, struct cw_frame *frame);

/* Helper Function Prototypes */
static void tech_destroy(struct private_object *tech_pvt);
static int waitfor_socket(int fd, int timeout) ;
static volatile struct faxmodem *acquire_modem(int index);
static void tech_destroy(struct private_object *tech_pvt) ;
static struct cw_channel *channel_new(const char *type, int format, void *data, int *cause);
static void channel_destroy(struct cw_channel *chan);
static struct cw_channel *tech_requester(const char *type, int format, void *data, int *cause);
static int tech_devicestate(void *data);
static int tech_send_digit(struct cw_channel *self, char digit);
static int dsp_buffer_size(int bitrate, struct timeval tv, int lastsize);
static void *faxmodem_media_thread(void *obj);
static void launch_faxmodem_media_thread(struct private_object *tech_pvt) ;
static struct cw_frame *tech_read(struct cw_channel *self);
static int tech_write(struct cw_channel *self, struct cw_frame *frame);
static int tech_write_video(struct cw_channel *self, struct cw_frame *frame);
static struct cw_frame *tech_exception(struct cw_channel *self);
static int tech_fixup(struct cw_channel *oldchan, struct cw_channel *newchan);
static int tech_send_html(struct cw_channel *self, int subclass, const char *data, int datalen);
static int tech_send_text(struct cw_channel *self, const char *text);
static int tech_send_image(struct cw_channel *self, struct cw_frame *frame) ;
static int tech_setoption(struct cw_channel *self, int option, void *data, int datalen);
static int tech_queryoption(struct cw_channel *self, int option, void *data, int *datalen);
static struct cw_channel *tech_bridged_channel(struct cw_channel *self, struct cw_channel *bridge);
static int tech_transfer(struct cw_channel *self, const char *newdest);
static int tech_bridge(struct cw_channel *chan_a, struct cw_channel *chan_b, int flags, struct cw_frame **outframe, struct cw_channel **recent_chan, int timeoutms);
static int control_handler(struct faxmodem *fm, int op, const char *num);
static void *faxmodem_thread(void *obj);
static void launch_faxmodem_thread(volatile struct faxmodem *fm) ;
static void activate_fax_modems(void);
static void deactivate_fax_modems(void);


/********************************************************************************
 * Constant structure for mapping local methods to the core interface.
 * This structure only needs to contain the methods the channel requires to operate
 * Not every channel needs all of them defined.
 */

static const struct cw_channel_tech technology = {
	.type = type,
	.description = tdesc,
	.capabilities = -1,
	.requester = tech_requester,
	.devicestate = tech_devicestate,
	.send_digit = tech_send_digit,
	.call = tech_call,
	.bridge = tech_bridge,
	.hangup = tech_hangup,
	.answer = tech_answer,
	.transfer = tech_transfer,
	.write_video = tech_write_video,
	.read = tech_read,
	.write = tech_write,
	.exception = tech_exception,
	.indicate = tech_indicate,
	.fixup = tech_fixup,
	.send_html = tech_send_html,
	.send_text = tech_send_text,
	.send_image = tech_send_image,
	.setoption = tech_setoption,
	.queryoption = tech_queryoption,
	.bridged_channel = tech_bridged_channel,
	.transfer = tech_transfer,
};

/***************** Helper functions ****************/

static void inline threadcount_inc(void)
{
	cw_mutex_lock(&threadcount_lock);
	THREADCOUNT++;
	cw_mutex_unlock(&threadcount_lock);
}

static void inline threadcount_dec(void)
{
	cw_mutex_lock(&threadcount_lock);
	THREADCOUNT--;
	cw_mutex_unlock(&threadcount_lock);
}

static void set_context(char *context)
{
	if (CONTEXT) {
		free(CONTEXT);
		CONTEXT = NULL;
	}

	if (context) {
		CONTEXT = strdup(context);
	}
}

static void set_vblevel(int level) 
{
	if (level > -1) {
		VBLEVEL = level;
	}
}


static int waitfor_socket(int fd, int timeout) 
{
	struct pollfd pfds[1];
	int res;

	memset(&pfds[0], 0, sizeof(pfds[0]));
	pfds[0].fd = fd;
	pfds[0].events = POLLIN | POLLERR;

	res = poll(pfds, 1, timeout);
	if (res == -1 && errno == EINTR)  /* System call interrupted */
	    return 0;

	
	if ((pfds[0].revents & POLLERR)) {
		res = -1;
	} else if((pfds[0].revents & POLLIN)) {
		res = 1;
	}

	return res;
}


static volatile struct faxmodem *acquire_modem(int index)
{
        volatile struct faxmodem *fm = NULL;

	cw_mutex_lock(&control_lock);
	if (index) {
		fm = &FAXMODEM_POOL[index];
	} else if (ring_strategy == RING_STRATEGY_FF) {
	    for (; rr_next < SOFT_MAX_FAXMODEMS; rr_next++) {
		    cw_verbose(VBPREFIX  "acquire considering: %d\n", rr_next);
		    cw_verbose(VBPREFIX  "%d state: %d\n", rr_next, FAXMODEM_POOL[rr_next].state);
			if ( FAXMODEM_POOL[rr_next].state == FAXMODEM_STATE_ONHOOK) {
				fm = &FAXMODEM_POOL[rr_next];
				break;
			}
	    }
	    rr_next++;
	    if (rr_next >= SOFT_MAX_FAXMODEMS)
		rr_next = 0;
	} else {
		int x;
		for(x = 0; x < SOFT_MAX_FAXMODEMS; x++) {
			if ( FAXMODEM_POOL[x].state == FAXMODEM_STATE_ONHOOK) {
				fm = &FAXMODEM_POOL[x];
				break;
			}
		}
	}
	
	if (fm && fm->state != FAXMODEM_STATE_ONHOOK) {
		cw_log(LOG_ERROR, "Modem %s In Use!\n", fm->devlink);
		fm = NULL;
	} 

	if (fm) {
		fm->state = FAXMODEM_STATE_ACQUIRED;
	} else {
		cw_log(LOG_ERROR, "No Modems Available!\n");
	}

	cw_mutex_unlock(&control_lock);

	return fm;
}


/* tech_destroy() Wipes out a private object 
 * If you allocated any memory to pointer members of this structure 
 * you must be sure to free it properly.
 */
static void tech_destroy(struct private_object *tech_pvt) 
{
	
	struct cw_channel *chan;
	CWOBJ_CONTAINER_UNLINK(&private_object_list, tech_pvt);
	if (tech_pvt && (chan = tech_pvt->owner)) {
		chan->tech_pvt = NULL;
		if (! cw_test_flag(tech_pvt, TFLAG_PBX)) {
			cw_hangup(chan);
		} else {
			cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
		}
	}
	if (tech_pvt->pipe[0] > -1) {
		close(tech_pvt->pipe[0]);
	}
	if (tech_pvt->pipe[1] > -1) {
		close(tech_pvt->pipe[1]);
	}

	if (tech_pvt->cid_name) free(tech_pvt->cid_name);
	if (tech_pvt->cid_num) free(tech_pvt->cid_num);

	free(tech_pvt);	
	cw_mutex_lock(&usecnt_lock);
	usecnt--;
	if (usecnt < 0) {
		usecnt = 0;
	}
	cw_mutex_unlock(&usecnt_lock);
}


/* channel_new() make a new channel and fit it with a private object */
static struct cw_channel *channel_new(const char *type, int format, void *data, int *cause)
{
	struct private_object *tech_pvt;
	struct cw_channel *chan = NULL;
	int myformat = CW_FORMAT_SLINEAR;


	if (!(tech_pvt = malloc(sizeof(struct private_object)))) {
		cw_log(LOG_ERROR, "Can't allocate a private structure.\n");
	} else {
		memset(tech_pvt, 0, sizeof(struct private_object));
		if (!(chan = cw_channel_alloc(1))) {
			free(tech_pvt);
			cw_log(LOG_ERROR, "Can't allocate a channel.\n");
		} else {
			cw_cond_init(&tech_pvt->data_cond, 0);
			chan->tech_pvt = tech_pvt;	
			chan->nativeformats = myformat;
			chan->type = type;
			snprintf(chan->name, sizeof(chan->name), "%s/%s-%04lx", chan->type, (char *)data, cw_random() & 0xffff);
			chan->writeformat = chan->rawwriteformat = chan->readformat = myformat;
			chan->_state = CW_STATE_RINGING;
			chan->_softhangup = 0;
			chan->tech = &technology;

			cw_fr_init_ex(&tech_pvt->frame, CW_FRAME_VOICE, myformat, NULL);
			tech_pvt->frame.offset = CW_FRIENDLY_OFFSET;
			tech_pvt->frame.data = tech_pvt->fdata + CW_FRIENDLY_OFFSET;

			tech_pvt->owner = chan;
			CWOBJ_CONTAINER_LINK(&private_object_list, tech_pvt);
			cw_mutex_lock(&usecnt_lock);
			usecnt++;
			cw_mutex_unlock(&usecnt_lock);
		}
	}
	
	return chan;
}

/* Destroy the channel since we didn't use it */
void channel_destroy(struct cw_channel *chan)
{
	CWOBJ_CONTAINER_UNLINK(&private_object_list, chan->tech_pvt);
	free(chan->tech_pvt);
	chan->tech_pvt = 0;

	cw_mutex_lock(&usecnt_lock);
	usecnt++;
	cw_mutex_unlock(&usecnt_lock);

	cw_hangup(chan);
}


/********************CHANNEL METHOD LIBRARY********************
 * This is the actual functions described by the prototypes above.
 *
 */

/*--- tech_requester: parse 'data' a url-like destination string, allocate a channel and a private structure
 * and return the newly-setup channel.
 */
static struct cw_channel *tech_requester(const char *type, int format, void *data, int *cause)
{
	struct cw_channel *chan = NULL;

	if (!(chan = channel_new(type, format, data, cause))) {
		cw_log(LOG_ERROR, "Can't allocate a channel\n");
	} else {
		char *mydata = cw_strdupa(data);
		int index = 0;
		struct private_object *tech_pvt;
		volatile struct faxmodem *fm;
		char *num = NULL, *did = NULL;

		num = mydata;
		if ((did = strchr(mydata, '/'))) {
			*did = '\0';
			did++;
			index = atoi(num);
		} else {
			did = mydata;
		}

		tech_pvt = chan->tech_pvt;

		if ((fm = acquire_modem(index))) {
			tech_pvt->fm = fm;
			fm->user_data = chan;
			tech_pvt->pipe[0] = tech_pvt->pipe[1] = -1;
			pipe(tech_pvt->pipe);
			chan->fds[0] = tech_pvt->pipe[0];
			fm->psock = tech_pvt->pipe[1];
			cw_copy_string((char*)fm->digits, did, 
					 sizeof(fm->digits));
		} else {
			cw_log(LOG_ERROR, "FAILURE ACQUIRING MODEM!\n");
			channel_destroy(chan);
			return NULL;
		}

		cw_set_flag(tech_pvt, TFLAG_PBX); /* so we know we dont have to free the channel ourselves */		
	}

	return chan;
}

/*--- tech_devicestate: Part of the device state notification system ---*/
static int tech_devicestate(void *data)
{
	/* return one of.........
	 * CW_DEVICE_UNKNOWN
	 * CW_DEVICE_NOT_INUSE
	 * CW_DEVICE_INUSE
	 * CW_DEVICE_BUSY
	 * CW_DEVICE_INVALID
	 * CW_DEVICE_UNAVAILABLE
	 */

	int res = CW_DEVICE_UNKNOWN;

	return res;
}



/*--- tech_senddigit: Send a DTMF character */
static int tech_send_digit(struct cw_channel *self, char digit)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_call: Initiate a call on my channel 
 * 'dest' has been passed telling you where to call
 * but you may already have that information from the requester method
 * not sure why it sends it twice, maybe it changed since then *shrug*
 * You also have timeout (in ms) so you can tell how long the caller
 * is willing to wait for the call to be complete.
 */

static int tech_call(struct cw_channel *self, char *dest, int timeout)
{
	struct private_object *tech_pvt;
	//struct faxmodem *fm;
	int res = 0;

	tech_pvt = self->tech_pvt;
	tech_pvt->fm->state = FAXMODEM_STATE_RINGING;

	/* Remember callerid so we can send it to our user.
	 * Without this the callerid is wrong unless the 'o' option is used
	 * in Dial() */
	if (tech_pvt->cid_name) 
	    free(tech_pvt->cid_name);
	if (tech_pvt->cid_num) 
	    free(tech_pvt->cid_num);

	if (self->cid.cid_name)
	    tech_pvt->cid_name = strdup(self->cid.cid_name);
	else
	    tech_pvt->cid_name = 0;

	if (self->cid.cid_num)
	    tech_pvt->cid_num = strdup(self->cid.cid_num);
	else
	    tech_pvt->cid_num = 0;
	    
	launch_faxmodem_media_thread(tech_pvt);

	return res;
}

/*--- tech_hangup: end a call on my channel 
 * Now is your chance to tear down and free the private object
 * from the channel it's about to be freed so you must do so now
 * or the object is lost.  Well I guess you could tag it for reuse
 * or for destruction and let a monitor thread deal with it too.
 * during the load_module routine you have every right to start up
 * your own fancy schmancy bunch of threads or whatever else 
 * you want to do.
 */
static int tech_hangup(struct cw_channel *self)
{
	struct private_object *tech_pvt = self->tech_pvt;
	int res = 0;

	self->tech_pvt = NULL;

	if (tech_pvt) {
#ifdef DO_TRACE
		close(tech_pvt->debug[0]);
		close(tech_pvt->debug[1]);
#endif
		if (!tech_pvt->hangup_msg_sent)
		    cw_cli(tech_pvt->fm->master, "NO CARRIER%s", TERMINATOR);

		tech_pvt->fm->state = FAXMODEM_STATE_ONHOOK;
		t31_call_event((t31_state_t *)&tech_pvt->fm->t31_state, 
			       AT_CALL_EVENT_HANGUP);
		tech_pvt->fm->psock = -1;
		tech_pvt->fm->user_data = NULL;
		tech_pvt->owner = NULL;
		tech_destroy(tech_pvt);
	}
	
	return res;
}

static int dsp_buffer_size(int bitrate, struct timeval tv, int lastsize)
{
	int us;
	double cleared;

	if (!lastsize) return 0;	// the buffer has been idle
	us = cw_tvdiff_ms(cw_tvnow(), tv);
	if (us <= 0) return 0;	// no time has passed
	cleared = ((double) bitrate * ((double) us / 1000000)) / 8;
	return cleared >= lastsize ? 0 : lastsize - cleared;
}

static void *faxmodem_media_thread(void *obj)
{
	struct private_object *tech_pvt = obj;
	volatile struct faxmodem *fm = tech_pvt->fm;
	struct timeval last = {0,0}, lastdtedata = {0,0}, now = {0,0}, reference = {0,0};
	int ms = 0;
	int avail, lastmodembufsize = 0, flowoff = 0;
	char modembuf[DSP_BUFFER_MAXSIZE];
	char buf[80];
	time_t noww;
	struct timespec abstime;
	int gotlen = 0;
	short *frame_data = tech_pvt->fdata + CW_FRIENDLY_OFFSET;
	struct pollfd pfds[1];
	memset(&pfds[0], 0, sizeof(pfds[0]));
	pfds[0].fd = tech_pvt->pipe[1];
	pfds[0].events = POLLIN | POLLERR;
	

	threadcount_inc();
	if (VBLEVEL > 1) {
		cw_verbose(VBPREFIX  "MEDIA THREAD ON %s\n", fm->devlink);
	}
	gettimeofday(&last, NULL);


	if (fm->state == FAXMODEM_STATE_RINGING) {
		time(&noww);
		cw_cli(fm->master, "%s", TERMINATOR);
		strftime(buf, sizeof(buf), "DATE=%m%d", localtime(&noww));
		cw_cli(fm->master, "%s%s", buf, TERMINATOR);
		strftime(buf, sizeof(buf), "TIME=%H%M", localtime(&noww));
		cw_cli(fm->master, "%s%s", buf, TERMINATOR);
		cw_cli(fm->master, "NAME=%s%s", tech_pvt->cid_name, TERMINATOR);
		cw_cli(fm->master, "NMBR=%s%s", tech_pvt->cid_num, TERMINATOR);
		cw_cli(fm->master, "NDID=%s%s", fm->digits, TERMINATOR);
		t31_call_event((t31_state_t*)&fm->t31_state, 
			       AT_CALL_EVENT_ALERTING);
	}

	while (fm->state == FAXMODEM_STATE_RINGING) {
		gettimeofday(&now, NULL);
		ms = cw_tvdiff_ms(now, last);

		if (ms % 5000 == 0) {
			t31_call_event((t31_state_t*)&fm->t31_state,
				       AT_CALL_EVENT_ALERTING);
		}
		
		usleep(100000);
		sched_yield();
	}

	if (tech_pvt->fm->state == FAXMODEM_STATE_ANSWERED) {
		t31_call_event((t31_state_t*)&fm->t31_state, 
			       AT_CALL_EVENT_ANSWERED);
		tech_pvt->fm->state = FAXMODEM_STATE_CONNECTED;
		cw_setstate(tech_pvt->owner, CW_STATE_UP);
	} else if (tech_pvt->fm->state == FAXMODEM_STATE_CONNECTED) {
		t31_call_event((t31_state_t*)&tech_pvt->fm->t31_state, 
			       AT_CALL_EVENT_CONNECTED);
	} else {
	        threadcount_dec();
		return NULL;
	}

	gettimeofday(&reference, NULL);	
	while (fm->state == FAXMODEM_STATE_CONNECTED) {
		tech_pvt->flen = 0;
		do {
			gotlen = t31_tx((t31_state_t*)&fm->t31_state, 
					frame_data + tech_pvt->flen, SAMPLES - tech_pvt->flen);
			tech_pvt->flen += gotlen;
		} while (tech_pvt->flen < SAMPLES && gotlen > 0);
			
		if (!tech_pvt->flen) {
			tech_pvt->flen = SAMPLES;
			memset(frame_data, 0, SAMPLES * 2);
		}
		tech_pvt->frame.samples = tech_pvt->flen;
		tech_pvt->frame.datalen = tech_pvt->frame.samples * 2;
		write(tech_pvt->pipe[1], IO_READ, 1);

#ifdef DO_TRACE
		write(tech_pvt->debug[1], frame_data, tech_pvt->flen * 2);
#endif

		reference = cw_tvadd(reference, cw_tv(0, MS * 1000));
		while ((ms = cw_tvdiff_ms(reference, cw_tvnow())) > 0) {
			abstime.tv_sec = time(0) + 1;
			abstime.tv_nsec = 0;

			cw_mutex_lock(&data_lock);
			cw_cond_timedwait(&tech_pvt->data_cond, &data_lock,
					    &abstime);
			cw_mutex_unlock(&data_lock);

			if (cw_test_flag(tech_pvt, TFLAG_DATA)) {
				cw_clear_flag(tech_pvt, TFLAG_DATA);
				break;
			}
		}
		
		gettimeofday(&now, NULL);
	
		avail = DSP_BUFFER_MAXSIZE - dsp_buffer_size(fm->t31_state.bit_rate, lastdtedata, lastmodembufsize);
		if (flowoff && avail >= DSP_BUFFER_MAXSIZE / 2) {
			char xon[1];
			xon[0] = 0x11;
			write(fm->psock, xon, 1);
			flowoff = 0;
			if (VBLEVEL > 1) {
				cw_verbose(VBPREFIX "%s XON, %d bytes available\n", fm->devlink, avail);
			}
		}
		if (cw_test_flag(fm, TFLAG_EVENT) && !flowoff) {
			ssize_t len;
			cw_clear_flag(fm, TFLAG_EVENT);
			do {
				len = read(fm->psock, modembuf, avail);
				if (len > 0) {
					t31_at_rx((t31_state_t*)&fm->t31_state,
						  modembuf, len);
					avail -= len;
				}
			} while (len > 0 && avail > 0);
			lastmodembufsize = DSP_BUFFER_MAXSIZE - avail;
			lastdtedata = now;
			if (!avail) {
				char xoff[1];
				xoff[0] = 0x13;
				write(fm->psock, xoff, 1);
				flowoff = 1;
				if (VBLEVEL > 1) {
					cw_verbose(VBPREFIX "%s XOFF\n", fm->devlink);
				}
			}
		}

		usleep(100);
		sched_yield();
	}
	if (VBLEVEL > 1) {
		cw_verbose(VBPREFIX  "MEDIA THREAD OFF %s\n", fm->devlink);
	}
	threadcount_dec();
	return NULL;
}

static void launch_faxmodem_media_thread(struct private_object *tech_pvt) 
{
	pthread_attr_t attr;
	int result = 0;
	pthread_t thread;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, faxmodem_media_thread, tech_pvt);
	result = pthread_attr_destroy(&attr);
}



/*--- tech_answer: answer a call on my channel
 * if being 'answered' means anything special to your channel
 * now is your chance to do it!
 */
static int tech_answer(struct cw_channel *self)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;

	if (VBLEVEL > 1) {
		cw_verbose(VBPREFIX  "Connected %s\n", tech_pvt->fm->devlink);
	}
	tech_pvt->fm->state = FAXMODEM_STATE_CONNECTED;
	launch_faxmodem_media_thread(tech_pvt);

	
	return res;
}


/*--- tech_read: Read an audio frame from my channel.
 * You need to read data from your channel and convert/transfer the
 * data into a newly allocated struct cw_frame object
 */
static struct cw_frame *tech_read(struct cw_channel *self)
{
	struct private_object *tech_pvt;
	int res;
	char cmd[2];

	tech_pvt = self->tech_pvt;
	res = read(tech_pvt->pipe[0], cmd, sizeof(cmd));	
	
	if (res < 0 || !strcmp(cmd, IO_HUP)) {
		cw_softhangup(tech_pvt->owner, CW_SOFTHANGUP_EXPLICIT);
		return NULL;
	}

	if (res < 0 || !strcmp(cmd, IO_ANSWER)) {
		struct cw_frame ans = {CW_FRAME_CONTROL, CW_CONTROL_ANSWER};
		launch_faxmodem_media_thread(tech_pvt);
		return cw_frdup(&ans);
	}

	return &tech_pvt->frame;
}

/*--- tech_write: Write an audio frame to my channel
 * Yep, this is the opposite of tech_read, you need to examine
 * a frame and transfer the data to your technology's audio stream.
 * You do not have any responsibility to destroy this frame and you should
 * consider it to be read-only.
 */

static int tech_write(struct cw_channel *self, struct cw_frame *frame)
{
	struct private_object *tech_pvt;
	int res = 0;
	//int gotlen;

	if (frame->frametype != CW_FRAME_VOICE) {
		return 0;
	}

	tech_pvt = self->tech_pvt;

#ifdef DO_TRACE
	write(tech_pvt->debug[0], frame->data, frame->datalen);
#endif

	res = t31_rx((t31_state_t*)&tech_pvt->fm->t31_state,
		     frame->data, frame->samples);
	
	/* Signal new data to media thread */
	cw_mutex_lock(&data_lock);
	cw_set_flag(tech_pvt, TFLAG_DATA);
	cw_cond_signal(&tech_pvt->data_cond);
	cw_mutex_unlock(&data_lock);
	
	//write(tech_pvt->pipe[0], IO_PROD, 1);


	return 0;
}

/*--- tech_write_video: Write a video frame to my channel ---*/
static int tech_write_video(struct cw_channel *self, struct cw_frame *frame)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_exception: Read an exception audio frame from my channel ---*/
static struct cw_frame *tech_exception(struct cw_channel *self)
{
	struct private_object *tech_pvt;
	struct cw_frame *new_frame = NULL;

	tech_pvt = self->tech_pvt;
	return new_frame;
}

/*--- tech_indicate: Indicaate a condition to my channel ---*/
static int tech_indicate(struct cw_channel *self, int condition)
{
	struct private_object *tech_pvt;
	int res = 0;
	int hangup = 0;

	tech_pvt = self->tech_pvt;
        if (VBLEVEL > 1) {
                cw_verbose(VBPREFIX  "Indication %d on %s\n", condition,
			     self->name);
        }

	switch(condition) {
	case CW_CONTROL_RINGING:
	case CW_CONTROL_ANSWER:
	case CW_CONTROL_PROGRESS:
	    hangup = 0;
	    break;
	case CW_CONTROL_BUSY:
	case CW_CONTROL_CONGESTION:
	    cw_cli(tech_pvt->fm->master, "BUSY%s", TERMINATOR);
	    hangup = 1;
	    break;
	default:
	    if (VBLEVEL > 1)
                cw_verbose(VBPREFIX  "UNKNOWN Indication %d on %s\n", 
			     condition,
                             self->name);
	}

	if (hangup) {
	    if (VBLEVEL > 1) {
                cw_verbose(VBPREFIX  "Hanging up because of indication %d "
			     "on %s\n", condition, self->name);
	    }	    
	    tech_pvt->hangup_msg_sent = 1;
	    cw_softhangup(self, CW_SOFTHANGUP_EXPLICIT);
	}
	return res;
}

/*--- tech_fixup: add any finishing touches to my channel if it is masqueraded---*/
static int tech_fixup(struct cw_channel *oldchan, struct cw_channel *newchan)
{
	int res = 0;

	return res;
}

/*--- tech_send_html: Send html data on my channel ---*/
static int tech_send_html(struct cw_channel *self, int subclass, const char *data, int datalen)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_send_text: Send plain text data on my channel ---*/
static int tech_send_text(struct cw_channel *self, const char *text)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_send_image: Send image data on my channel ---*/
static int tech_send_image(struct cw_channel *self, struct cw_frame *frame) 
{
	struct private_object *tech_pvt;

	int res = 0;
	tech_pvt = self->tech_pvt;
	return res;
}


/*--- tech_setoption: set options on my channel ---*/
static int tech_setoption(struct cw_channel *self, int option, void *data, int datalen)
{
	struct private_object *tech_pvt;

	int res = 0;
	tech_pvt = self->tech_pvt;
	return res;

}

/*--- tech_queryoption: get options from my channel ---*/
static int tech_queryoption(struct cw_channel *self, int option, void *data, int *datalen)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_bridged_channel: return a pointer to a channel that may be bridged to our channel. ---*/
static struct cw_channel *tech_bridged_channel(struct cw_channel *self, struct cw_channel *bridge)
{
	struct private_object *tech_pvt;  
	//struct cw_channel *chan = NULL;

	tech_pvt = self->tech_pvt;
	return self->_bridge;
}


/*--- tech_transfer: Technology-specific code executed to peform a transfer. ---*/
static int tech_transfer(struct cw_channel *self, const char *newdest)
{
	struct private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_bridge:  Technology-specific code executed to natively bridge 2 of our channels ---*/
static int tech_bridge(struct cw_channel *chan_a, struct cw_channel *chan_b, int flags, struct cw_frame **outframe, struct cw_channel **recent_chan, int timeoutms)
{
	int res = 0;

	return res;
}


static int control_handler(struct faxmodem *fm, int op, const char *num)
{
	int res = 0;

	if (VBLEVEL > 1) {
		cw_verbose(VBPREFIX  "Control Handler %s [op = %d]\n", fm->devlink, op);
	}
	cw_mutex_lock(&control_lock);
	if (fm->state == FAXMODEM_STATE_INIT) {
		fm->state = FAXMODEM_STATE_ONHOOK;
	}

	do {
	    if (op == AT_MODEM_CONTROL_CALL) {
		    struct cw_channel *chan = NULL;
		    struct private_object *tech_pvt;
		    int cause;
		    
		    if (fm->state != FAXMODEM_STATE_ONHOOK) {
			cw_log(LOG_ERROR, "Invalid State! [%s]\n", faxmodem_state2name(fm->state));
			res = -1;
			break;
		    }
		    if (!(chan = channel_new(type, CW_FORMAT_SLINEAR, (char *) num, &cause))) {
			cw_log(LOG_ERROR, "Can't allocate a channel\n");
			res = -1;
			break;
		    } else {
			tech_pvt = chan->tech_pvt;
			fm->user_data = chan;
			faxmodem_set_flag(fm, FAXMODEM_FLAG_ATDT);
			cw_copy_string(fm->digits, num, sizeof(fm->digits));
			tech_pvt->fm = fm;
			cw_copy_string(chan->context, CONTEXT, sizeof(chan->context));
			cw_copy_string(chan->exten, fm->digits, sizeof(chan->exten));
			cw_set_flag(tech_pvt, TFLAG_OUTBOUND);
			tech_pvt->pipe[0] = tech_pvt->pipe[1] = -1;
			pipe(tech_pvt->pipe);
			chan->fds[0] = tech_pvt->pipe[0];
			fm->psock = tech_pvt->pipe[1];
			fm->state = FAXMODEM_STATE_CALLING;
			if (cw_pbx_start(chan)) {
			    cw_log(LOG_WARNING, "Unable to start PBX on %s\n", chan->name);
			    cw_hangup(chan);
			}
#ifdef DOTRACE
			tech_pvt->debug[0] = open("/tmp/cap-in.raw", O_WRONLY|O_CREAT, 00660);
			tech_pvt->debug[1] = open("/tmp/cap-out.raw", O_WRONLY|O_CREAT, 00660);
#endif
			if (VBLEVEL > 1) {
			    cw_verbose(VBPREFIX  "Call Started %s %s@%s\n", chan->name, chan->exten, chan->context);
			}
		    }
	    } else if (op == AT_MODEM_CONTROL_ANSWER) { 
		if (fm->state != FAXMODEM_STATE_RINGING) {
		    cw_log(LOG_ERROR, "Invalid State! [%s]\n", faxmodem_state2name(fm->state));
		    res = -1;
		    break;
		}
		if (VBLEVEL > 1) {
		    cw_verbose(VBPREFIX  "Answered %s", fm->devlink);
		}
		fm->state = FAXMODEM_STATE_ANSWERED;
	    } else if (op == AT_MODEM_CONTROL_HANGUP) {
		if (fm->psock > -1) {
		    if (fm->user_data) {
			struct cw_channel *chan = fm->user_data;
			cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
			write(fm->psock, IO_HUP, 1);
		    }
		} else {
		    fm->state = FAXMODEM_STATE_ONHOOK;
		}
		
		t31_call_event(&fm->t31_state, AT_CALL_EVENT_HANGUP);
		
	    }
	} while (0);
	
	cw_mutex_unlock(&control_lock);
	return res;
}


static void *faxmodem_thread(void *obj)
{
	struct faxmodem *fm = obj;
	int res;
	char buf[1024], tmp[80];

	cw_mutex_lock(&control_lock);
	faxmodem_init(fm, control_handler, DEVICE_PREFIX);
	cw_mutex_unlock(&control_lock);

	threadcount_inc();


	while (faxmodem_test_flag(fm, FAXMODEM_FLAG_RUNNING)) {
	        res = waitfor_socket(fm->master, 1000);
		if (res == 0) {
		    /* Timeout occured, so we try again */
		    continue;
		} else if (res < 0) {
			cw_log(LOG_WARNING, "Bad Read on master [%s]\n", fm->devlink);
			break;
		} else if (!faxmodem_test_flag(fm, FAXMODEM_FLAG_RUNNING))
		    break;

		cw_set_flag(fm, TFLAG_EVENT);
		res = read(fm->master, buf, sizeof(buf));
		t31_at_rx(&fm->t31_state, buf, res);
		memset(tmp, 0, sizeof(tmp));

		/* Copy the AT command for debugging */
		if(strstr(buf, "AT") || strstr(buf, "at")) {
			int x;
			int l = res < (sizeof(tmp)-1) ? res : sizeof(tmp)-1;
			strncpy(tmp, buf, l);
			for(x = 0; x < l; x++) {
				if(tmp[x] == '\r' || tmp[x] == '\n') {
					tmp[x] = '\0';
				}
			}
			if (!cw_strlen_zero(tmp) && VBLEVEL > 0) {
				cw_verbose(VBPREFIX  "Command on %s [%s]\n", fm->devlink, tmp);
			}
		}
	}

	if (VBLEVEL > 1) {
		cw_verbose(VBPREFIX  "Thread ended for %s\n", fm->devlink);
	}
	threadcount_dec();
	faxmodem_close(fm);

	return NULL;
}

static void launch_faxmodem_thread(volatile struct faxmodem *fm) 
{
	pthread_attr_t attr;
	int result = 0;
	pthread_t thread;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, faxmodem_thread, 
				     (void*)fm);
	result = pthread_attr_destroy(&attr);
}

static void activate_fax_modems(void)
{
	int max = SOFT_MAX_FAXMODEMS;
	int x;

	cw_mutex_lock(&control_lock);
	memset((void*)FAXMODEM_POOL, 0, MAX_FAXMODEMS);
	for(x = 0; x < max; x++) {
		if (VBLEVEL > 1) {
			cw_verbose(VBPREFIX  "Starting Fax Modem SLOT %d\n", x);
		}
		launch_faxmodem_thread(&FAXMODEM_POOL[x]);
	}
	cw_mutex_unlock(&control_lock);
}


static void deactivate_fax_modems(void)
{
	int max = SOFT_MAX_FAXMODEMS;
	int x;
	
	cw_mutex_lock(&control_lock);
	for(x = 0; x < max; x++) {
		if (VBLEVEL > 1) {
			cw_verbose(VBPREFIX  "Stopping Fax Modem SLOT %d\n", x);
		}
		faxmodem_close(&FAXMODEM_POOL[x]);
		unlink((char*)FAXMODEM_POOL[x].devlink);
	}
	/* Wait for Threads to die */
	while (THREADCOUNT) {
		usleep(1000);
		sched_yield();
	}

	cw_mutex_unlock(&control_lock);

}

static int parse_config(int reload) {
	struct cw_config *cfg;
	char *entry;
	struct cw_variable *v;

	if ((cfg = cw_config_load(CONFIGFILE))) {
		READY++;
		for (entry = cw_category_browse(cfg, NULL); entry != NULL; entry = cw_category_browse(cfg, entry)) {
			if (!strcasecmp(entry, "settings")) {
				for (v = cw_variable_browse(cfg, entry); v ; v = v->next) {
					if (!strcasecmp(v->name, "modems")) {
						SOFT_MAX_FAXMODEMS = atoi(v->value);
					} else if (!strcasecmp(v->name, "timeout-ms")) {
						SOFT_TIMEOUT = atoi(v->value);
					} else if (!strcasecmp(v->name, "trap-seg")) {
						cw_log(LOG_WARNING, "trap-seg is deprecated - remove it from your chan_fax.conf");
					} else if (!strcasecmp(v->name, "context")) {
						set_context(v->value);
					} else if (!strcasecmp(v->name, "vblevel")) {
						set_vblevel(atoi(v->value));
					} else if (!strcasecmp(v->name, "device-prefix")) {
						free(DEVICE_PREFIX);
					        DEVICE_PREFIX = strdup(v->value);
					} else if (!strcasecmp(v->name, "ring-strategy")) {
					    if (!strcasecmp(v->value, "roundrobin"))
						ring_strategy = RING_STRATEGY_RR;
					    else
						ring_strategy = RING_STRATEGY_FF;

					}
				}
			}
		}
		if (!CONTEXT) {
			set_context("chan_fax");
		}
		cw_config_destroy(cfg);
	}

	return READY;
}


static int chan_fax_cli(int fd, int argc, char *argv[]) 
{
	if (argc > 1) {
		if (!strcasecmp(argv[1], "status")) {
			int x;
			cw_mutex_lock(&control_lock);
			for(x = 0; x < SOFT_MAX_FAXMODEMS; x++) {
				cw_cli(fd, "SLOT %d %s [%s]\n", x, FAXMODEM_POOL[x].devlink, faxmodem_state2name(FAXMODEM_POOL[x].state));
			}
			cw_mutex_unlock(&control_lock);
		} else if (!strcasecmp(argv[1], "vblevel")) {
			if(argc > 2) {
				set_vblevel(atoi(argv[2]));
			}
			cw_cli(fd, "vblevel = %d\n", VBLEVEL);
		}

	} else {
		cw_cli(fd, "Usage: fax [status]\n");
	}
	return 0;
}


static struct cw_cli_entry  cli_chan_fax = { { "fax", NULL }, chan_fax_cli, "Fax Channel", "Fax Channel" };

/******************************* CORE INTERFACE ********************************************
 * These are module-specific interface functions that are common to every module
 * To be used to initilize/de-initilize, reload and track the use count of a loadable module. 
 */

static void graceful_unload(void)
{
	if(READY) {
		deactivate_fax_modems();
	}
	faxmodem_clear_logger();
	set_context(NULL);
	CWOBJ_CONTAINER_DESTROY(&private_object_list);
	cw_channel_unregister(&technology);
	cw_cli_unregister(&cli_chan_fax);

	free(DEVICE_PREFIX);
}


int load_module()
{
	CWOBJ_CONTAINER_INIT(&private_object_list);

	DEVICE_PREFIX = strdup("/dev/FAX");
	
	if(!parse_config(0)) {
		return -1;
	}

	if (VBLEVEL > 1) {
		faxmodem_set_logger((faxmodem_logger_t) cw_log, __LOG_ERROR, __LOG_WARNING, __LOG_NOTICE);
	}
	cw_register_atexit(graceful_unload);
	activate_fax_modems();

	if (cw_channel_register(&technology)) {
		cw_log(LOG_ERROR, "Unable to register channel class %s\n", type);
		return -1;
	}
	cw_cli_register(&cli_chan_fax);

	return 0;
}

int reload()
{
	//parse_config(0);
	return 0;
}

int unload_module()
{
	graceful_unload();
	return 0;
}

int usecount()
{
	int res;
	cw_mutex_lock(&usecnt_lock);
	res = usecnt;
	cw_mutex_unlock(&usecnt_lock);
	return res;
}

/* returns a descriptive string to the system console */
char *description()
{
	return (char *) desc;
}

