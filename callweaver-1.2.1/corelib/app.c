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
 * \brief Convenient Application Routines
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/app.c $", "$Revision: 5374 $")

#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/file.h"
#include "callweaver/app.h"
#include "callweaver/dsp.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/utils.h"
#include "callweaver/lock.h"
#include "callweaver/indications.h"
#include "callweaver/linkedlists.h"

#define MAX_OTHER_FORMATS 10

static CW_LIST_HEAD_STATIC(groups, cw_group_info);


/* 
This function presents a dialtone and reads an extension into 'collect' 
which must be a pointer to a **pre-initilized** array of char having a 
size of 'size' suitable for writing to.  It will collect no more than the smaller 
of 'maxlen' or 'size' minus the original strlen() of collect digits.
*/
int cw_app_dtget(struct cw_channel *chan, const char *context, char *collect, size_t size, int maxlen, int timeout) 
{
	struct tone_zone_sound *ts;
	int res=0, x=0;

	if(maxlen > size)
		maxlen = size;
	
	if(!timeout && chan->pbx)
		timeout = chan->pbx->dtimeout;
	else if(!timeout)
		timeout = 5;
	
	ts = cw_get_indication_tone(chan->zone,"dial");
	if (ts && ts->data[0])
		res = cw_playtones_start(chan, 0, ts->data, 0);
	else 
		cw_log(LOG_NOTICE,"Huh....? no dial for indications?\n");
	
	for (x = strlen(collect); strlen(collect) < maxlen; ) {
		res = cw_waitfordigit(chan, timeout);
		if (!cw_ignore_pattern(context, collect))
			cw_playtones_stop(chan);
		if (res < 1)
			break;
		collect[x++] = res;
		if (!cw_matchmore_extension(chan, context, collect, 1, chan->cid.cid_num)) {
			if (collect[x-1] == '#') {
				/* Not a valid extension, ending in #, assume the # was to finish dialing */
				collect[x-1] = '\0';
			}
			break;
		}
	}
	if (res >= 0) {
		if (cw_exists_extension(chan, context, collect, 1, chan->cid.cid_num))
			res = 1;
		else
			res = 0;
	}
	return res;
}

int cw_app_getdata(struct cw_channel *c, char *prompt, char *s, int maxlen, int timeout)
{
	int res,to,fto;
	/* XXX Merge with full version? XXX */
	if (maxlen)
		s[0] = '\0';
	if (prompt) {
		res = cw_streamfile(c, prompt, c->language);
		if (res < 0)
			return res;
		}
	fto = c->pbx ? c->pbx->rtimeout * 1000 : 6000;
	to = c->pbx ? c->pbx->dtimeout * 1000 : 2000;

	if (timeout > 0)
		fto = to = timeout;
	if (timeout < 0)
		fto = to = 1000000000;
	res = cw_readstring(c, s, maxlen, to, fto, "#");
	return res;
} 


int cw_app_getdata_full(struct cw_channel *c, char *prompt, char *s, int maxlen, int timeout, int audiofd, int ctrlfd)
{
	int res,to,fto;
	if (prompt) {
		res = cw_streamfile(c, prompt, c->language);
		if (res < 0)
			return res;
	}
	fto = 6000;
	to = 2000;
	if (timeout > 0) 
		fto = to = timeout;
	if (timeout < 0) 
		fto = to = 1000000000;
	res = cw_readstring_full(c, s, maxlen, to, fto, "#", audiofd, ctrlfd);
	return res;
}

int cw_app_getvoice(struct cw_channel *c, char *dest, char *dstfmt, char *prompt, int silence, int maxsec)
{
	int res;
	struct cw_filestream *writer;
	int rfmt;
	int totalms=0, total;
	
	struct cw_frame *f;
	struct cw_dsp *sildet;
	/* Play prompt if requested */
	if (prompt) {
		res = cw_streamfile(c, prompt, c->language);
		if (res < 0)
			return res;
		res = cw_waitstream(c,"");
		if (res < 0)
			return res;
	}
	rfmt = c->readformat;
	res = cw_set_read_format(c, CW_FORMAT_SLINEAR);
	if (res < 0) {
		cw_log(LOG_WARNING, "Unable to set to linear mode, giving up\n");
		return -1;
	}
	sildet = cw_dsp_new();
	if (!sildet) {
		cw_log(LOG_WARNING, "Unable to create silence detector :(\n");
		return -1;
	}
	writer = cw_writefile(dest, dstfmt, "Voice file", 0, 0, 0666);
	if (!writer) {
		cw_log(LOG_WARNING, "Unable to open file '%s' in format '%s' for writing\n", dest, dstfmt);
		cw_dsp_free(sildet);
		return -1;
	}
	for(;;) {
		if ((res = cw_waitfor(c, 2000)) < 0) {
			cw_log(LOG_NOTICE, "Waitfor failed while recording file '%s' format '%s'\n", dest, dstfmt);
			break;
		}
		if (res) {
			f = cw_read(c);
			if (!f) {
				cw_log(LOG_NOTICE, "Hungup while recording file '%s' format '%s'\n", dest, dstfmt);
				break;
			}
			if ((f->frametype == CW_FRAME_DTMF) && (f->subclass == '#')) {
				/* Ended happily with DTMF */
				cw_fr_free(f);
				break;
			} else if (f->frametype == CW_FRAME_VOICE) {
				cw_dsp_silence(sildet, f, &total); 
				if (total > silence) {
					/* Ended happily with silence */
					cw_fr_free(f);
					break;
				}
				totalms += f->samples / 8;
				if (totalms > maxsec * 1000) {
					/* Ended happily with too much stuff */
					cw_log(LOG_NOTICE, "Constraining voice on '%s' to %d seconds\n", c->name, maxsec);
					cw_fr_free(f);
					break;
				}
				res = cw_writestream(writer, f);
				if (res < 0) {
					cw_log(LOG_WARNING, "Failed to write to stream at %s!\n", dest);
					cw_fr_free(f);
					break;
				}

			}
			cw_fr_free(f);
		}
	}
	res = cw_set_read_format(c, rfmt);
	if (res)
		cw_log(LOG_WARNING, "Unable to restore read format on '%s'\n", c->name);
	cw_dsp_free(sildet);
	cw_closestream(writer);
	return 0;
}

static int (*cw_has_request_t38_func)(const struct cw_channel *chan) = NULL;

void cw_install_t38_functions( int (*has_request_t38_func)(const struct cw_channel *chan) )
{
	cw_has_request_t38_func = has_request_t38_func;
}

void cw_uninstall_t38_functions(void)
{
	cw_has_request_t38_func = NULL;
}

int cw_app_request_t38(const struct cw_channel *chan)
{
    if (cw_has_request_t38_func)
		return cw_has_request_t38_func(chan);
    return 0;
}



static int (*cw_has_voicemail_func)(const char *mailbox, const char *folder) = NULL;
static int (*cw_messagecount_func)(const char *mailbox, int *newmsgs, int *oldmsgs) = NULL;

void cw_install_vm_functions(int (*has_voicemail_func)(const char *mailbox, const char *folder),
			      int (*messagecount_func)(const char *mailbox, int *newmsgs, int *oldmsgs))
{
	cw_has_voicemail_func = has_voicemail_func;
	cw_messagecount_func = messagecount_func;
}

void cw_uninstall_vm_functions(void)
{
	cw_has_voicemail_func = NULL;
	cw_messagecount_func = NULL;
}

int cw_app_has_voicemail(const char *mailbox, const char *folder)
{
	static int warned = 0;
	if (cw_has_voicemail_func)
		return cw_has_voicemail_func(mailbox, folder);

	if ((option_verbose > 2) && !warned) {
		cw_verbose(VERBOSE_PREFIX_3 "Message check requested for mailbox %s/folder %s but voicemail not loaded.\n", mailbox, folder ? folder : "INBOX");
		warned++;
	}
	return 0;
}


int cw_app_messagecount(const char *mailbox, int *newmsgs, int *oldmsgs)
{
	static int warned = 0;
	if (newmsgs)
		*newmsgs = 0;
	if (oldmsgs)
		*oldmsgs = 0;
	if (cw_messagecount_func)
		return cw_messagecount_func(mailbox, newmsgs, oldmsgs);

	if (!warned && (option_verbose > 2)) {
		warned++;
		cw_verbose(VERBOSE_PREFIX_3 "Message count requested for mailbox %s but voicemail not loaded.\n", mailbox);
	}

	return 0;
}

int cw_dtmf_stream(struct cw_channel *chan,struct cw_channel *peer,char *digits,int between) 
{
	char *ptr;
	int res = 0;
	struct cw_frame f;
	if (!between)
		between = 100;

	if (peer)
		res = cw_autoservice_start(peer);

	if (!res) {
		res = cw_waitfor(chan,100);
		if (res > -1) {
			for (ptr=digits; *ptr; ptr++) {
				if (*ptr == 'w') {
					res = cw_safe_sleep(chan, 500);
					if (res) 
						break;
					continue;
				}
                cw_fr_init_ex(&f, CW_FRAME_DTMF, *ptr, NULL);
				f.src = "cw_dtmf_stream";
				if (strchr("0123456789*#abcdABCD",*ptr) == NULL)
                {
					cw_log(LOG_WARNING, "Illegal DTMF character '%c' in string. (0-9*#aAbBcCdD allowed)\n",*ptr);
				}
                else
                {
					res = cw_write(chan, &f);
					if (res) 
						break;
					/* pause between digits */
					res = cw_safe_sleep(chan,between);
					if (res) 
						break;
				}
			}
		}
		if (peer)
			res = cw_autoservice_stop(peer);
	}
	return res;
}

struct linear_state {
	int fd;
	int autoclose;
	int allowoverride;
	int origwfmt;
};

static void linear_release(struct cw_channel *chan, void *params)
{
	struct linear_state *ls = params;
	
    if (ls->origwfmt && cw_set_write_format(chan, ls->origwfmt))
    {
		cw_log(LOG_WARNING, "Unable to restore channel '%s' to format '%d'\n", chan->name, ls->origwfmt);
	}
	if (ls->autoclose)
		close(ls->fd);
	free(params);
}

static int linear_generator(struct cw_channel *chan, void *data, int samples)
{
	struct cw_frame f;
	short buf[2048 + CW_FRIENDLY_OFFSET / 2];
	struct linear_state *ls = data;
	int res, len;

	len = samples*sizeof(int16_t);
	if (len > sizeof(buf) - CW_FRIENDLY_OFFSET)
    {
		cw_log(LOG_WARNING, "Can't generate %d bytes of data!\n" ,len);
		len = sizeof(buf) - CW_FRIENDLY_OFFSET;
	}
	memset(&f, 0, sizeof(f));
	res = read(ls->fd, buf + CW_FRIENDLY_OFFSET/2, len);
	if (res > 0)
    {
        cw_fr_init_ex(&f, CW_FRAME_VOICE, CW_FORMAT_SLINEAR, NULL);
		f.data = buf + CW_FRIENDLY_OFFSET/sizeof(int16_t);
		f.datalen = res;
		f.samples = res/sizeof(int16_t);
		f.offset = CW_FRIENDLY_OFFSET;
		cw_write(chan, &f);
		if (res == len)
			return 0;
	}
	return -1;
}

static void *linear_alloc(struct cw_channel *chan, void *params)
{
	struct linear_state *ls;
	/* In this case, params is already malloc'd */
	if (params) {
		ls = params;
		if (ls->allowoverride)
			cw_set_flag(chan, CW_FLAG_WRITE_INT);
		else
			cw_clear_flag(chan, CW_FLAG_WRITE_INT);
		ls->origwfmt = chan->writeformat;
		if (cw_set_write_format(chan, CW_FORMAT_SLINEAR)) {
			cw_log(LOG_WARNING, "Unable to set '%s' to linear format (write)\n", chan->name);
			free(ls);
			ls = params = NULL;
		}
	}
	return params;
}

static struct cw_generator linearstream = 
{
	alloc: linear_alloc,
	release: linear_release,
	generate: linear_generator,
};

int cw_linear_stream(struct cw_channel *chan, const char *filename, int fd, int allowoverride)
{
	struct linear_state *lin;
	char tmpf[256];
	int res = -1;
	int autoclose = 0;
	if (fd < 0) {
		if (cw_strlen_zero(filename))
			return -1;
		autoclose = 1;
		if (filename[0] == '/') 
			cw_copy_string(tmpf, filename, sizeof(tmpf));
		else
			snprintf(tmpf, sizeof(tmpf), "%s/%s/%s", (char *)cw_config_CW_VAR_DIR, "sounds", filename);
		fd = open(tmpf, O_RDONLY);
		if (fd < 0){
			cw_log(LOG_WARNING, "Unable to open file '%s': %s\n", tmpf, strerror(errno));
			return -1;
		}
	}
	lin = malloc(sizeof(struct linear_state));
	if (lin) {
		memset(lin, 0, sizeof(lin));
		lin->fd = fd;
		lin->allowoverride = allowoverride;
		lin->autoclose = autoclose;
		res = cw_generator_activate(chan, &linearstream, lin);
	}
	return res;
}

int cw_control_streamfile(struct cw_channel *chan, const char *file,
			   const char *fwd, const char *rev,
			   const char *stop, const char *pause,
			   const char *restart, int skipms) 
{
	long elapsed = 0, last_elapsed = 0;
	char *breaks = NULL;
	char *end = NULL;
	int blen = 2;
	int res;

	if (stop)
		blen += strlen(stop);
	if (pause)
		blen += strlen(pause);
	if (restart)
		blen += strlen(restart);

	if (blen > 2) {
		breaks = alloca(blen + 1);
		breaks[0] = '\0';
		if (stop)
			strcat(breaks, stop);
		if (pause)
			strcat(breaks, pause);
		if (restart)
			strcat(breaks, restart);
	}
	if (chan->_state != CW_STATE_UP)
		res = cw_answer(chan);

	if (chan)
		cw_stopstream(chan);

	if (file) {
		if ((end = strchr(file,':'))) {
			if (!strcasecmp(end, ":end")) {
				*end = '\0';
				end++;
			}
		}
	}

	for (;;) {
		struct timeval started = cw_tvnow();

		if (chan)
			cw_stopstream(chan);
		res = cw_streamfile(chan, file, chan->language);
		if (!res) {
			if (end) {
				cw_seekstream(chan->stream, 0, SEEK_END);
				end=NULL;
			}
			res = 1;
			if (elapsed) {
				cw_stream_fastforward(chan->stream, elapsed);
				last_elapsed = elapsed - 200;
			}
			if (res)
				res = cw_waitstream_fr(chan, breaks, fwd, rev, skipms);
			else
				break;
		}

		if (res < 1)
			break;

		/* We go at next loop if we got the restart char */
		if (restart && strchr(restart, res)) {
			cw_log(LOG_DEBUG, "we'll restart the stream here at next loop\n");
			elapsed=0; /* To make sure the next stream will start at beginning */
			continue;
		}

		if (pause != NULL && strchr(pause, res)) {
			elapsed = cw_tvdiff_ms(cw_tvnow(), started) + last_elapsed;
			for(;;) {
				if (chan)
					cw_stopstream(chan);
				res = cw_waitfordigit(chan, 1000);
				if (res == 0)
					continue;
				else if (res == -1 || strchr(pause, res) || (stop && strchr(stop, res)))
					break;
			}
			if (res == *pause) {
				res = 0;
				continue;
			}
		}
		if (res == -1)
			break;

		/* if we get one of our stop chars, return it to the calling function */
		if (stop && strchr(stop, res)) {
			/* res = 0; */
			break;
		}
	}
	if (chan)
		cw_stopstream(chan);

	return res;
}

int cw_play_and_wait(struct cw_channel *chan, const char *fn)
{
	int d;
	d = cw_streamfile(chan, fn, chan->language);
	if (d)
		return d;
	d = cw_waitstream(chan, CW_DIGIT_ANY);
	cw_stopstream(chan);
	return d;
}

static int global_silence_threshold = 128;
static int global_maxsilence = 0;

int cw_play_and_record(struct cw_channel *chan, const char *playfile, const char *recordfile, int maxtime, const char *fmt, int *duration, int silencethreshold, int maxsilence, const char *path)
{
	int d;
	char *fmts;
	char comment[256];
	int x, fmtcnt=1, res=-1,outmsg=0;
	struct cw_frame *f;
	struct cw_filestream *others[MAX_OTHER_FORMATS];
	char *sfmt[MAX_OTHER_FORMATS];
	char *stringp=NULL;
	time_t start, end;
	struct cw_dsp *sildet=NULL;   	/* silence detector dsp */
	int totalsilence = 0;
	int dspsilence = 0;
	int gotsilence = 0;		/* did we timeout for silence? */
	int rfmt=0;

	if (silencethreshold < 0)
		silencethreshold = global_silence_threshold;

	if (maxsilence < 0)
		maxsilence = global_maxsilence;

	/* barf if no pointer passed to store duration in */
	if (duration == NULL) {
		cw_log(LOG_WARNING, "Error play_and_record called without duration pointer\n");
		return -1;
	}

	cw_log(LOG_DEBUG,"play_and_record: %s, %s, '%s'\n", playfile ? playfile : "<None>", recordfile, fmt);
	snprintf(comment,sizeof(comment),"Playing %s, Recording to: %s on %s\n", playfile ? playfile : "<None>", recordfile, chan->name);

	if (playfile) {
		d = cw_play_and_wait(chan, playfile);
		if (d > -1)
			d = cw_streamfile(chan, "beep",chan->language);
		if (!d)
			d = cw_waitstream(chan,"");
		if (d < 0)
			return -1;
	}

	fmts = cw_strdupa(fmt);

	stringp=fmts;
	strsep(&stringp, "|,");
	cw_log(LOG_DEBUG,"Recording Formats: sfmts=%s\n", fmts);
	sfmt[0] = cw_strdupa(fmts);

	while((fmt = strsep(&stringp, "|,"))) {
		if (fmtcnt > MAX_OTHER_FORMATS - 1) {
			cw_log(LOG_WARNING, "Please increase MAX_OTHER_FORMATS in app_voicemail.c\n");
			break;
		}
		sfmt[fmtcnt++] = cw_strdupa(fmt);
	}

	time(&start);
	end=start;  /* pre-initialize end to be same as start in case we never get into loop */
	for (x=0;x<fmtcnt;x++) {
		others[x] = cw_writefile(recordfile, sfmt[x], comment, O_TRUNC, 0, 0700);
		cw_verbose( VERBOSE_PREFIX_3 "x=%d, open writing:  %s format: %s, %p\n", x, recordfile, sfmt[x], others[x]);

		if (!others[x]) {
			break;
		}
	}

	if (path)
		cw_unlock_path(path);


	
	if (maxsilence > 0) {
		sildet = cw_dsp_new(); /* Create the silence detector */
		if (!sildet) {
			cw_log(LOG_WARNING, "Unable to create silence detector :(\n");
			return -1;
		}
		cw_dsp_set_threshold(sildet, silencethreshold);
		rfmt = chan->readformat;
		res = cw_set_read_format(chan, CW_FORMAT_SLINEAR);
		if (res < 0) {
			cw_log(LOG_WARNING, "Unable to set to linear mode, giving up\n");
			cw_dsp_free(sildet);
			return -1;
		}
	}
	/* Request a video update */
	cw_indicate(chan, CW_CONTROL_VIDUPDATE);

	if (x == fmtcnt) {
	/* Loop forever, writing the packets we read to the writer(s), until
	   we read a # or get a hangup */
		f = NULL;
		for(;;) {
		 	res = cw_waitfor(chan, 2000);
			if (!res) {
				cw_log(LOG_DEBUG, "One waitfor failed, trying another\n");
				/* Try one more time in case of masq */
			 	res = cw_waitfor(chan, 2000);
				if (!res) {
					cw_log(LOG_WARNING, "No audio available on %s??\n", chan->name);
					res = -1;
				}
			}

			if (res < 0) {
				f = NULL;
				break;
			}
			f = cw_read(chan);
			if (!f)
				break;
			if (f->frametype == CW_FRAME_VOICE) {
				/* write each format */
				for (x=0;x<fmtcnt;x++) {
					res = cw_writestream(others[x], f);
				}

				/* Silence Detection */
				if (maxsilence > 0) {
					dspsilence = 0;
					cw_dsp_silence(sildet, f, &dspsilence);
					if (dspsilence)
						totalsilence = dspsilence;
					else
						totalsilence = 0;

					if (totalsilence > maxsilence) {
						/* Ended happily with silence */
						if (option_verbose > 2)
							cw_verbose( VERBOSE_PREFIX_3 "Recording automatically stopped after a silence of %d seconds\n", totalsilence/1000);
						cw_fr_free(f);
						gotsilence = 1;
						outmsg=2;
						break;
					}
				}
				/* Exit on any error */
				if (res) {
					cw_log(LOG_WARNING, "Error writing frame\n");
					cw_fr_free(f);
					break;
				}
			} else if (f->frametype == CW_FRAME_VIDEO) {
				/* Write only once */
				cw_writestream(others[0], f);
			} else if (f->frametype == CW_FRAME_DTMF) {
				if (f->subclass == '#') {
					if (option_verbose > 2)
						cw_verbose( VERBOSE_PREFIX_3 "User ended message by pressing %c\n", f->subclass);
					res = '#';
					outmsg = 2;
					cw_fr_free(f);
					break;
				}
				if (f->subclass == '0') {
				/* Check for a '0' during message recording also, in case caller wants operator */
					if (option_verbose > 2)
						cw_verbose(VERBOSE_PREFIX_3 "User cancelled by pressing %c\n", f->subclass);
					res = '0';
					outmsg = 0;
					cw_fr_free(f);
					break;
				}
			}
			if (maxtime) {
				time(&end);
				if (maxtime < (end - start)) {
					if (option_verbose > 2)
						cw_verbose( VERBOSE_PREFIX_3 "Took too long, cutting it short...\n");
					outmsg = 2;
					res = 't';
					cw_fr_free(f);
					break;
				}
			}
			cw_fr_free(f);
		}
		if (end == start) time(&end);
		if (!f) {
			if (option_verbose > 2)
				cw_verbose( VERBOSE_PREFIX_3 "User hung up\n");
			res = -1;
			outmsg=1;
		}
	} else {
		cw_log(LOG_WARNING, "Error creating writestream '%s', format '%s'\n", recordfile, sfmt[x]);
	}

	*duration = end - start;

	for (x=0;x<fmtcnt;x++) {
		if (!others[x])
			break;
		if (res > 0) {
			if (totalsilence)
				cw_stream_rewind(others[x], totalsilence-200);
			else
				cw_stream_rewind(others[x], 200);
		}
		cw_truncstream(others[x]);
		cw_closestream(others[x]);
	}
	if (rfmt) {
		if (cw_set_read_format(chan, rfmt)) {
			cw_log(LOG_WARNING, "Unable to restore format %s to channel '%s'\n", cw_getformatname(rfmt), chan->name);
		}
	}
	if (outmsg > 1) {
		/* Let them know recording is stopped */
		if(!cw_streamfile(chan, "auth-thankyou", chan->language))
			cw_waitstream(chan, "");
	}
	if (sildet)
		cw_dsp_free(sildet);
	return res;
}

int cw_play_and_prepend(struct cw_channel *chan, char *playfile, char *recordfile, int maxtime, char *fmt, int *duration, int beep, int silencethreshold, int maxsilence)
{
	int d = 0;
	char *fmts;
	char comment[256];
	int x, fmtcnt=1, res=-1,outmsg=0;
	struct cw_frame *f;
	struct cw_filestream *others[MAX_OTHER_FORMATS];
	struct cw_filestream *realfiles[MAX_OTHER_FORMATS];
	char *sfmt[MAX_OTHER_FORMATS];
	char *stringp=NULL;
	time_t start, end;
	struct cw_dsp *sildet;   	/* silence detector dsp */
	int totalsilence = 0;
	int dspsilence = 0;
	int gotsilence = 0;		/* did we timeout for silence? */
	int rfmt=0;	
	char prependfile[80];
	
	if (silencethreshold < 0)
		silencethreshold = global_silence_threshold;

	if (maxsilence < 0)
		maxsilence = global_maxsilence;

	/* barf if no pointer passed to store duration in */
	if (duration == NULL) {
		cw_log(LOG_WARNING, "Error play_and_prepend called without duration pointer\n");
		return -1;
	}

	cw_log(LOG_DEBUG,"play_and_prepend: %s, %s, '%s'\n", playfile ? playfile : "<None>", recordfile, fmt);
	snprintf(comment,sizeof(comment),"Playing %s, Recording to: %s on %s\n", playfile ? playfile : "<None>", recordfile, chan->name);

	if (playfile || beep) {	
		if (!beep)
			d = cw_play_and_wait(chan, playfile);
		if (d > -1)
			d = cw_streamfile(chan, "beep",chan->language);
		if (!d)
			d = cw_waitstream(chan,"");
		if (d < 0)
			return -1;
	}
	cw_copy_string(prependfile, recordfile, sizeof(prependfile));	
	strncat(prependfile, "-prepend", sizeof(prependfile) - strlen(prependfile) - 1);
			
	fmts = cw_strdupa(fmt);
	
	stringp=fmts;
	strsep(&stringp, "|,");
	cw_log(LOG_DEBUG,"Recording Formats: sfmts=%s\n", fmts);	
	sfmt[0] = cw_strdupa(fmts);
	
	while((fmt = strsep(&stringp, "|,"))) {
		if (fmtcnt > MAX_OTHER_FORMATS - 1) {
			cw_log(LOG_WARNING, "Please increase MAX_OTHER_FORMATS in app_voicemail.c\n");
			break;
		}
		sfmt[fmtcnt++] = cw_strdupa(fmt);
	}

	time(&start);
	end=start;  /* pre-initialize end to be same as start in case we never get into loop */
	for (x=0;x<fmtcnt;x++) {
		others[x] = cw_writefile(prependfile, sfmt[x], comment, O_TRUNC, 0, 0700);
		cw_verbose( VERBOSE_PREFIX_3 "x=%d, open writing:  %s format: %s, %p\n", x, prependfile, sfmt[x], others[x]);
		if (!others[x]) {
			break;
		}
	}
	
	sildet = cw_dsp_new(); /* Create the silence detector */
	if (!sildet) {
		cw_log(LOG_WARNING, "Unable to create silence detector :(\n");
		return -1;
	}
	cw_dsp_set_threshold(sildet, silencethreshold);

	if (maxsilence > 0) {
		rfmt = chan->readformat;
		res = cw_set_read_format(chan, CW_FORMAT_SLINEAR);
		if (res < 0) {
			cw_log(LOG_WARNING, "Unable to set to linear mode, giving up\n");
			return -1;
		}
	}
						
	if (x == fmtcnt) {
	/* Loop forever, writing the packets we read to the writer(s), until
	   we read a # or get a hangup */
		f = NULL;
		for(;;) {
		 	res = cw_waitfor(chan, 2000);
			if (!res) {
				cw_log(LOG_DEBUG, "One waitfor failed, trying another\n");
				/* Try one more time in case of masq */
			 	res = cw_waitfor(chan, 2000);
				if (!res) {
					cw_log(LOG_WARNING, "No audio available on %s??\n", chan->name);
					res = -1;
				}
			}
			
			if (res < 0) {
				f = NULL;
				break;
			}
			f = cw_read(chan);
			if (!f)
				break;
			if (f->frametype == CW_FRAME_VOICE) {
				/* write each format */
				for (x=0;x<fmtcnt;x++) {
					if (!others[x])
						break;
					res = cw_writestream(others[x], f);
				}
				
				/* Silence Detection */
				if (maxsilence > 0) {
					dspsilence = 0;
					cw_dsp_silence(sildet, f, &dspsilence);
					if (dspsilence)
						totalsilence = dspsilence;
					else
						totalsilence = 0;
					
					if (totalsilence > maxsilence) {
					/* Ended happily with silence */
					if (option_verbose > 2) 
						cw_verbose( VERBOSE_PREFIX_3 "Recording automatically stopped after a silence of %d seconds\n", totalsilence/1000);
					cw_fr_free(f);
					gotsilence = 1;
					outmsg=2;
					break;
					}
				}
				/* Exit on any error */
				if (res) {
					cw_log(LOG_WARNING, "Error writing frame\n");
					cw_fr_free(f);
					break;
				}
			} else if (f->frametype == CW_FRAME_VIDEO) {
				/* Write only once */
				cw_writestream(others[0], f);
			} else if (f->frametype == CW_FRAME_DTMF) {
				/* stop recording with any digit */
				if (option_verbose > 2) 
					cw_verbose( VERBOSE_PREFIX_3 "User ended message by pressing %c\n", f->subclass);
				res = 't';
				outmsg = 2;
				cw_fr_free(f);
				break;
			}
			if (maxtime) {
				time(&end);
				if (maxtime < (end - start)) {
					if (option_verbose > 2)
						cw_verbose( VERBOSE_PREFIX_3 "Took too long, cutting it short...\n");
					res = 't';
					outmsg=2;
					cw_fr_free(f);
					break;
				}
			}
			cw_fr_free(f);
		}
		if (end == start)
            time(&end);
		if (!f) {
			if (option_verbose > 2) 
				cw_verbose( VERBOSE_PREFIX_3 "User hung up\n");
			res = -1;
			outmsg=1;
#if 0
			/* delete all the prepend files */
			for (x=0;x<fmtcnt;x++) {
				if (!others[x])
					break;
				cw_closestream(others[x]);
				cw_filedelete(prependfile, sfmt[x]);
			}
#endif
		}
	} else {
		cw_log(LOG_WARNING, "Error creating writestream '%s', format '%s'\n", prependfile, sfmt[x]); 
	}
	*duration = end - start;
#if 0
	if (outmsg > 1) {
#else
	if (outmsg) {
#endif
		struct cw_frame *fr;
		for (x=0;x<fmtcnt;x++) {
			snprintf(comment, sizeof(comment), "Opening the real file %s.%s\n", recordfile, sfmt[x]);
			realfiles[x] = cw_readfile(recordfile, sfmt[x], comment, O_RDONLY, 0, 0);
			if (!others[x] || !realfiles[x])
				break;
			if (totalsilence)
				cw_stream_rewind(others[x], totalsilence-200);
			else
				cw_stream_rewind(others[x], 200);
			cw_truncstream(others[x]);
			/* add the original file too */
			while ((fr = cw_readframe(realfiles[x]))) {
				cw_writestream(others[x],fr);
			}
			cw_closestream(others[x]);
			cw_closestream(realfiles[x]);
			cw_filerename(prependfile, recordfile, sfmt[x]);
#if 0
			cw_verbose("Recording Format: sfmts=%s, prependfile %s, recordfile %s\n", sfmt[x],prependfile,recordfile);
#endif
			cw_filedelete(prependfile, sfmt[x]);
		}
	}
	if (rfmt) {
		if (cw_set_read_format(chan, rfmt)) {
			cw_log(LOG_WARNING, "Unable to restore format %s to channel '%s'\n", cw_getformatname(rfmt), chan->name);
		}
	}
	if (outmsg) {
		if (outmsg > 1) {
			/* Let them know it worked */
			cw_streamfile(chan, "auth-thankyou", chan->language);
			cw_waitstream(chan, "");
		}
	}	
	return res;
}

/* Channel group core functions */

int cw_app_group_split_group(char *data, char *group, int group_max, char *category, int category_max)
{
	int res=0;
	char tmp[256];
	char *grp=NULL, *cat=NULL;

	if (!cw_strlen_zero(data)) {
		cw_copy_string(tmp, data, sizeof(tmp));
		grp = tmp;
		cat = strchr(tmp, '@');
		if (cat) {
			*cat = '\0';
			cat++;
		}
	}

	if (!cw_strlen_zero(grp))
		cw_copy_string(group, grp, group_max);
	else
		res = -1;

	if (!cw_strlen_zero(cat))
		cw_copy_string(category, cat, category_max);

	return res;
}

int cw_app_group_set_channel(struct cw_channel *chan, char *data)
{
	int res = 0;
	char group[80] = "", category[80] = "";
	struct cw_group_info *gi = NULL;
	size_t len = 0;

	if (cw_app_group_split_group(data, group, sizeof(group), category, sizeof(category)))
		return -1;

	/* Calculate memory we will need if this is new */
	len = sizeof(*gi) + strlen(group) + 1;
	if (!cw_strlen_zero(category))
		len += strlen(category) + 1;

	CW_LIST_LOCK(&groups);
	CW_LIST_TRAVERSE(&groups, gi, list) {
		if (gi->chan == chan && !strcasecmp(gi->group, group) && (cw_strlen_zero(category) || (!cw_strlen_zero(gi->category) && !strcasecmp(gi->category, category))))
			break;
	}

	if (!gi && (gi = calloc(1, len))) {
		gi->chan = chan;
		gi->group = (char *) gi + sizeof(*gi);
		strcpy(gi->group, group);
		if (!cw_strlen_zero(category)) {
			gi->category = (char *) gi + sizeof(*gi) + strlen(group) + 1;
			strcpy(gi->category, category);
		}
		CW_LIST_INSERT_TAIL(&groups, gi, list);
	} else {
 		res = -1;
	}
 
	CW_LIST_UNLOCK(&groups);


	return res;
}

int cw_app_group_get_count(char *group, char *category)
{
	struct cw_group_info *gi = NULL;
	int count = 0;

	if (cw_strlen_zero(group))
		return 0;

	CW_LIST_LOCK(&groups);
	CW_LIST_TRAVERSE(&groups, gi, list) {
		if (!strcasecmp(gi->group, group) && (cw_strlen_zero(category) || !strcasecmp(gi->category, category)))
			count++;
	}
	CW_LIST_UNLOCK(&groups);

	return count;
}

int cw_app_group_match_get_count(char *groupmatch, char *category)
{
	struct cw_group_info *gi = NULL;
	regex_t regexbuf;
	int count = 0;

	if (cw_strlen_zero(groupmatch))
		return 0;

	/* if regex compilation fails, return zero matches */
	if (regcomp(&regexbuf, groupmatch, REG_EXTENDED | REG_NOSUB))
		return 0;

	CW_LIST_LOCK(&groups);
	CW_LIST_TRAVERSE(&groups, gi, list) {
		if (!regexec(&regexbuf, gi->group, 0, NULL, 0) && (cw_strlen_zero(category) || !strcasecmp(gi->category, category)))
			count++;
	}
	CW_LIST_UNLOCK(&groups);
	
	regfree(&regexbuf);

	return count;
}

int cw_app_group_discard(struct cw_channel *chan)
{
	struct cw_group_info *gi = NULL;

	CW_LIST_LOCK(&groups);
	CW_LIST_TRAVERSE_SAFE_BEGIN(&groups, gi, list) {
		if (gi->chan == chan) {
			CW_LIST_REMOVE_CURRENT(&groups, list);
			free(gi);
		}
	}
   CW_LIST_TRAVERSE_SAFE_END
	CW_LIST_UNLOCK(&groups);

	return 0;
}

int cw_app_group_list_lock(void)
{
	return CW_LIST_LOCK(&groups);
}

struct cw_group_info *cw_app_group_list_head(void)
{
	return CW_LIST_FIRST(&groups);
}

int cw_app_group_list_unlock(void)
{
	return CW_LIST_UNLOCK(&groups);
}


int cw_separate_app_args(char *buf, char delim, int max_args, char **argv)
{
	char *start;
	int argc;
	char c;

	if (option_debug && option_verbose > 6)
		cw_log(LOG_DEBUG, "delim='%c', args: %s\n", delim, buf);

	/* The last argv is reserved for NULL. This is required if you want
	 * to hand off an argv to exec(2) for example.
	 */
	max_args--;

	argc = 0;
	if (buf) {
		start = buf;
		do {
			char *next, *end;
			int parens, inquote;

			/* Skip leading white space */
			start = cw_skip_blanks(start);
			next = end = start;

			/* Find the end of this arg. Backslash removes any special
			 * meaning from the next character. Otherwise quotes
			 * enclose strings and parentheses (outside any quoted
			 * string) must balance.
			 */
			inquote = parens = 0;
			for (; *next; next++) {
				if (*next == '\\') {
					if (!*(++next)) break;
				} else if (*next == '"') {
					inquote = !inquote;
					continue;
				} else if (*next == '(')
					parens++;
				else if (*next == ')')
					parens--;
				else if (*next == delim && !parens && !inquote)
					break;

				*(end++) = *next;
			}

			/* Note whether we hit a delimiter or '\0' in case
			 * we're about to overwrite it
			 */
			c = *next;

			/* Terminate and backtrack trimming off trailing whitespace */
			*end = '\0';
			while (end > start && isspace(end[-1]))
				*(--end) = '\0';

			/* Save the arg and its length if wanted */
			argv[argc] = start;
#if 0
			if (argl) argl[argc] = end - start;
#endif
			argc++;

			start = next + 1;
		} while (c && argc < max_args);
	}

	if (argc == 1 && !argv[0][0])
		argc--;

	argv[argc] = NULL;

	if (option_debug && option_verbose > 5) {
		int i;
		cw_log(LOG_DEBUG, "argc: %d\n", argc);
		for (i=0; i<argc; i++)
			cw_log(LOG_DEBUG, "argv[%d]: %s\n", i, argv[i]);
	}

	return argc;
}


enum CW_LOCK_RESULT cw_lock_path(const char *path)
{
	char *s;
	char *fs;
	int res;
	int fd;
	time_t start;

	s = alloca(strlen(path) + 10);
	fs = alloca(strlen(path) + 20);

	snprintf(fs, strlen(path) + 19, "%s/.lock-%08lx", path, cw_random());
	fd = open(fs, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (fd < 0) {
		cw_log(LOG_ERROR,"Unable to create lock file '%s': %s\n", path, strerror(errno));
		return CW_LOCK_PATH_NOT_FOUND;
	}
	close(fd);

	snprintf(s, strlen(path) + 9, "%s/.lock", path);
	time(&start);
	while (((res = link(fs, s)) < 0) && (errno == EEXIST) && (time(NULL) - start < 5))
		usleep(1);

	unlink(fs);

	if (res) {
		cw_log(LOG_WARNING, "Failed to lock path '%s': %s\n", path, strerror(errno));
		return CW_LOCK_TIMEOUT;
	} else {
		unlink(fs);
		cw_log(LOG_DEBUG, "Locked path '%s'\n", path);
		return CW_LOCK_SUCCESS;
	}
}

int cw_unlock_path(const char *path)
{
	char *s;
	int res;

	s = alloca(strlen(path) + 10);
	snprintf(s, strlen(path) + 9, "%s/%s", path, ".lock");

	if ((res = unlink(s)))
		cw_log(LOG_ERROR, "Could not unlock path '%s': %s\n", path, strerror(errno));
	else
		cw_log(LOG_DEBUG, "Unlocked path '%s'\n", path);

	return res;
}

int cw_record_review(struct cw_channel *chan, const char *playfile, const char *recordfile, int maxtime, const char *fmt, int *duration, const char *path) 
{
	int silencethreshold = 128; 
	int maxsilence=0;
	int res = 0;
	int cmd = 0;
	int max_attempts = 3;
	int attempts = 0;
	int recorded = 0;
	int message_exists = 0;
	/* Note that urgent and private are for flagging messages as such in the future */

	/* barf if no pointer passed to store duration in */
	if (duration == NULL) {
		cw_log(LOG_WARNING, "Error cw_record_review called without duration pointer\n");
		return -1;
	}

	cmd = '3';	 /* Want to start by recording */

	while ((cmd >= 0) && (cmd != 't')) {
		switch (cmd) {
		case '1':
			if (!message_exists) {
				/* In this case, 1 is to record a message */
				cmd = '3';
				break;
			} else {
				cw_streamfile(chan, "vm-msgsaved", chan->language);
				cw_waitstream(chan, "");
				cmd = 't';
				return res;
			}
		case '2':
			/* Review */
			cw_verbose(VERBOSE_PREFIX_3 "Reviewing the recording\n");
			cw_streamfile(chan, recordfile, chan->language);
			cmd = cw_waitstream(chan, CW_DIGIT_ANY);
			break;
		case '3':
			message_exists = 0;
			/* Record */
			if (recorded == 1)
				cw_verbose(VERBOSE_PREFIX_3 "Re-recording\n");
			else	
				cw_verbose(VERBOSE_PREFIX_3 "Recording\n");
			recorded = 1;
			cmd = cw_play_and_record(chan, playfile, recordfile, maxtime, fmt, duration, silencethreshold, maxsilence, path);
			if (cmd == -1) {
			/* User has hung up, no options to give */
				return cmd;
			}
			if (cmd == '0') {
				break;
			} else if (cmd == '*') {
				break;
			} 
			else {
				/* If all is well, a message exists */
				message_exists = 1;
				cmd = 0;
			}
			break;
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '*':
		case '#':
			cmd = cw_play_and_wait(chan, "vm-sorry");
			break;
		default:
			if (message_exists) {
				cmd = cw_play_and_wait(chan, "vm-review");
			}
			else {
				cmd = cw_play_and_wait(chan, "vm-torerecord");
				if (!cmd)
					cmd = cw_waitfordigit(chan, 600);
			}
			
			if (!cmd)
				cmd = cw_waitfordigit(chan, 6000);
			if (!cmd) {
				attempts++;
			}
			if (attempts > max_attempts) {
				cmd = 't';
			}
		}
	}
	if (cmd == 't')
		cmd = 0;
	return cmd;
}

#define RES_UPONE (1 << 16)
#define RES_EXIT  (1 << 17)
#define RES_REPEAT (1 << 18)
#define RES_RESTART ((1 << 19) | RES_REPEAT)

static int cw_ivr_menu_run_internal(struct cw_channel *chan, struct cw_ivr_menu *menu, void *cbdata);
static int ivr_dispatch(struct cw_channel *chan, struct cw_ivr_option *option, char *exten, void *cbdata)
{
	int res;
	int (*ivr_func)(struct cw_channel *, void *);
	char *c;
	char *n;
	
	switch(option->action) {
	case CW_ACTION_UPONE:
		return RES_UPONE;
	case CW_ACTION_EXIT:
		return RES_EXIT | (((unsigned long)(option->adata)) & 0xffff);
	case CW_ACTION_REPEAT:
		return RES_REPEAT | (((unsigned long)(option->adata)) & 0xffff);
	case CW_ACTION_RESTART:
		return RES_RESTART ;
	case CW_ACTION_NOOP:
		return 0;
	case CW_ACTION_BACKGROUND:
		res = cw_streamfile(chan, (char *)option->adata, chan->language);
		if (!res) {
			res = cw_waitstream(chan, CW_DIGIT_ANY);
		} else {
			cw_log(LOG_NOTICE, "Unable to find file '%s'!\n", (char *)option->adata);
			res = 0;
		}
		return res;
	case CW_ACTION_PLAYBACK:
		res = cw_streamfile(chan, (char *)option->adata, chan->language);
		if (!res) {
			res = cw_waitstream(chan, "");
		} else {
			cw_log(LOG_NOTICE, "Unable to find file '%s'!\n", (char *)option->adata);
			res = 0;
		}
		return res;
	case CW_ACTION_MENU:
		res = cw_ivr_menu_run_internal(chan, (struct cw_ivr_menu *)option->adata, cbdata);
		/* Do not pass entry errors back up, treat as though it was an "UPONE" */
		if (res == -2)
			res = 0;
		return res;
	case CW_ACTION_WAITOPTION:
		res = cw_waitfordigit(chan, 1000 * (chan->pbx ? chan->pbx->rtimeout : 10));
		if (!res)
			return 't';
		return res;
	case CW_ACTION_CALLBACK:
		ivr_func = option->adata;
		res = ivr_func(chan, cbdata);
		return res;
	case CW_ACTION_TRANSFER:
		res = cw_parseable_goto(chan, option->adata);
		return 0;
	case CW_ACTION_PLAYLIST:
	case CW_ACTION_BACKLIST:
		res = 0;
		c = cw_strdupa(option->adata);
		while((n = strsep(&c, ";")))
			if ((res = cw_streamfile(chan, n, chan->language)) || (res = cw_waitstream(chan, (option->action == CW_ACTION_BACKLIST) ? CW_DIGIT_ANY : "")))
				break;
		cw_stopstream(chan);
		return res;
	default:
		cw_log(LOG_NOTICE, "Unknown dispatch function %d, ignoring!\n", option->action);
		return 0;
	};
	return -1;
}

static int option_exists(struct cw_ivr_menu *menu, char *option)
{
	int x;
	for (x=0;menu->options[x].option;x++)
		if (!strcasecmp(menu->options[x].option, option))
			return x;
	return -1;
}

static int option_matchmore(struct cw_ivr_menu *menu, char *option)
{
	int x;
	for (x=0;menu->options[x].option;x++)
		if ((!strncasecmp(menu->options[x].option, option, strlen(option))) && 
				(menu->options[x].option[strlen(option)]))
			return x;
	return -1;
}

static int read_newoption(struct cw_channel *chan, struct cw_ivr_menu *menu, char *exten, int maxexten)
{
	int res=0;
	int ms;
	while(option_matchmore(menu, exten)) {
		ms = chan->pbx ? chan->pbx->dtimeout : 5000;
		if (strlen(exten) >= maxexten - 1) 
			break;
		res = cw_waitfordigit(chan, ms);
		if (res < 1)
			break;
		exten[strlen(exten) + 1] = '\0';
		exten[strlen(exten)] = res;
	}
	return res > 0 ? 0 : res;
}

static int cw_ivr_menu_run_internal(struct cw_channel *chan, struct cw_ivr_menu *menu, void *cbdata)
{
	/* Execute an IVR menu structure */
	int res=0;
	int pos = 0;
	int retries = 0;
	char exten[CW_MAX_EXTENSION] = "s";
	if (option_exists(menu, "s") < 0) {
		strcpy(exten, "g");
		if (option_exists(menu, "g") < 0) {
			cw_log(LOG_WARNING, "No 's' nor 'g' extension in menu '%s'!\n", menu->title);
			return -1;
		}
	}
	while(!res) {
		while(menu->options[pos].option) {
			if (!strcasecmp(menu->options[pos].option, exten)) {
				res = ivr_dispatch(chan, menu->options + pos, exten, cbdata);
				cw_log(LOG_DEBUG, "IVR Dispatch of '%s' (pos %d) yields %d\n", exten, pos, res);
				if (res < 0)
					break;
				else if (res & RES_UPONE)
					return 0;
				else if (res & RES_EXIT)
					return res;
				else if (res & RES_REPEAT) {
					int maxretries = res & 0xffff;
					if ((res & RES_RESTART) == RES_RESTART) {
						retries = 0;
					} else
						retries++;
					if (!maxretries)
						maxretries = 3;
					if ((maxretries > 0) && (retries >= maxretries)) {
						cw_log(LOG_DEBUG, "Max retries %d exceeded\n", maxretries);
						return -2;
					} else {
						if (option_exists(menu, "g") > -1) 
							strcpy(exten, "g");
						else if (option_exists(menu, "s") > -1)
							strcpy(exten, "s");
					}
					pos=0;
					continue;
				} else if (res && strchr(CW_DIGIT_ANY, res)) {
					cw_log(LOG_DEBUG, "Got start of extension, %c\n", res);
					exten[1] = '\0';
					exten[0] = res;
					if ((res = read_newoption(chan, menu, exten, sizeof(exten))))
						break;
					if (option_exists(menu, exten) < 0) {
						if (option_exists(menu, "i")) {
							cw_log(LOG_DEBUG, "Invalid extension entered, going to 'i'!\n");
							strcpy(exten, "i");
							pos = 0;
							continue;
						} else {
							cw_log(LOG_DEBUG, "Aborting on invalid entry, with no 'i' option!\n");
							res = -2;
							break;
						}
					} else {
						cw_log(LOG_DEBUG, "New existing extension: %s\n", exten);
						pos = 0;
						continue;
					}
				}
			}
			pos++;
		}
		cw_log(LOG_DEBUG, "Stopping option '%s', res is %d\n", exten, res);
		pos = 0;
		if (!strcasecmp(exten, "s"))
			strcpy(exten, "g");
		else
			break;
	}
	return res;
}

int cw_ivr_menu_run(struct cw_channel *chan, struct cw_ivr_menu *menu, void *cbdata)
{
	int res;
	res = cw_ivr_menu_run_internal(chan, menu, cbdata);
	/* Hide internal coding */
	if (res > 0)
		res = 0;
	return res;
}
	
char *cw_read_textfile(const char *filename)
{
	int fd;
	char *output=NULL;
	struct stat filesize;
	int count=0;
	int res;
	if(stat(filename,&filesize)== -1){
		cw_log(LOG_WARNING,"Error can't stat %s\n", filename);
		return NULL;
	}
	count=filesize.st_size + 1;
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		cw_log(LOG_WARNING, "Cannot open file '%s' for reading: %s\n", filename, strerror(errno));
		return NULL;
	}
	output=(char *)malloc(count);
	if (output) {
		res = read(fd, output, count - 1);
		if (res == count - 1) {
			output[res] = '\0';
		} else {
			cw_log(LOG_WARNING, "Short read of %s (%d of %d): %s\n", filename, res, count -  1, strerror(errno));
			free(output);
			output = NULL;
		}
	} else 
		cw_log(LOG_WARNING, "Out of memory!\n");
	close(fd);
	return output;
}

int cw_parseoptions(const struct cw_option *options, struct cw_flags *flags, char **args, char *optstr)
{
	char *s;
	int curarg;
	int argloc;
	char *arg;
	int res = 0;

	flags->flags = 0;

	if (!optstr)
		return 0;

	s = optstr;
	while (*s) {
		curarg = *s & 0x7f;
		flags->flags |= options[curarg].flag;
		argloc = options[curarg].arg_index;
		s++;
		if (*s == '(') {
			/* Has argument */
			s++;
			arg = s;
			while (*s && (*s != ')')) s++;
			if (*s) {
				if (argloc)
					args[argloc - 1] = arg;
				*s = '\0';
				s++;
			} else {
				cw_log(LOG_WARNING, "Missing closing parenthesis for argument '%c' in string '%s'\n", curarg, arg);
				res = -1;
			}
		} else if (argloc)
			args[argloc - 1] = NULL;
	}
	return res;
}
