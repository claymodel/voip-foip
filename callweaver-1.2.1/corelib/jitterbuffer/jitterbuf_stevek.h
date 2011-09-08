/*
 * jitterbuf: an application-independent jitterbuffer
 *
 * Copyrights:
 * Copyright (C) 2004-2005, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 */

#ifndef _JITTERBUF_H_
#define _JITTERBUF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <callweaver/generic_jb.h>

/* configuration constants */
	/* Number of historical timestamps to use in calculating jitter and drift */
#define JB_HISTORY_SZ		500	
	/* what percentage of timestamps should we drop from the history when we examine it;
	 * this might eventually be something made configurable */
#define JB_HISTORY_DROPPCT	3
	/* the maximum droppct we can handle (say it was configurable). */
#define JB_HISTORY_DROPPCT_MAX	4
	/* the size of the buffer we use to keep the top and botton timestamps for dropping */
#define JB_HISTORY_MAXBUF_SZ	JB_HISTORY_SZ * JB_HISTORY_DROPPCT_MAX / 100 
	/* amount of additional jitterbuffer adjustment  */
#define JB_TARGET_EXTRA 40
	/* ms between growing and shrinking; may not be honored if jitterbuffer runs out of space */
#define JB_ADJUST_DELAY 40

/* return codes */
#define JB_OK		0
#define JB_EMPTY	1
#define JB_NOFRAME	2
#define JB_INTERP	3
#define JB_DROP		4
#define JB_SCHED	5

/* frame types */
#define JB_TYPE_CONTROL	0
#define JB_TYPE_VOICE	1
#define JB_TYPE_VIDEO	2  /* reserved */
#define JB_TYPE_SILENCE	3


typedef struct jb_conf {
	/* settings */
	long max_jitterbuf;	/* defines a hard clamp to use in setting the jitter buffer delay */
	long min_jitterbuf;     /* Minimum / Initial JB size */
 	long resync_threshold;  /* the jb will resync when delay increases to (2 * jitter) + this param */
	long max_contig_interp; /* the max interp frames to return in a row */
} jb_conf;

typedef struct jitterbuf {
	jb_conf conf;
	cw_jb_info info;

	/* history */
	long history[JB_HISTORY_SZ];   		/* history */
	int  hist_ptr;				/* points to index in history for next entry */
	long hist_maxbuf[JB_HISTORY_MAXBUF_SZ];	/* a sorted buffer of the max delays (highest first) */
	long hist_minbuf[JB_HISTORY_MAXBUF_SZ];	/* a sorted buffer of the min delays (lowest first) */
	int  hist_maxbuf_valid;			/* are the "maxbuf"/minbuf valid? */

	int force_resync;                       /* force a resync of this jb */

	jb_frame *frames; 		/* queued frames */
	jb_frame *free; 		/* free frames (avoid malloc?) */
} jitterbuf;


/* new jitterbuf */
jitterbuf *		jb_new(void);

/* destroy jitterbuf */
void			jb_destroy(jitterbuf *jb);

/* reset jitterbuf */
/* NOTE:  The jitterbuffer should be empty before you call this, otherwise
 * you will leak queued frames, and some internal structures */
void			jb_reset(jitterbuf *jb);

/* queue a frame data=frame data, timings (in ms): ms=length of frame (for voice), ts=ts (sender's time) 
 * now=now (in receiver's time) return value is one of 
 * JB_OK: Frame added. Last call to jb_next() still valid
 * JB_DROP: Drop this frame immediately
 * JB_SCHED: Frame added. Call jb_next() to get a new time for the next frame
 */
int 			jb_put(jitterbuf *jb, void *data, int type, long ms, long ts, long now);

/* get a frame for time now (receiver's time)  return value is one of
 * JB_OK:  You've got frame!
 * JB_DROP: Here's an audio frame you should just drop.  Ask me again for this time..
 * JB_NOFRAME: There's no frame scheduled for this time.
 * JB_INTERP: Please interpolate an interpl-length frame for this time (either we need to grow, or there was a lost frame) 
 * JB_EMPTY: The jb is empty.
 */
int			jb_get(jitterbuf *jb, jb_frame *frame, long now, long interpl);

/* unconditionally get frames from jitterbuf until empty */
int jb_getall(jitterbuf *jb, jb_frame *frameout);

/* when is the next frame due out, in receiver's time (0=EMPTY) 
 * This value may change as frames are added (esp non-audio frames) */
long			jb_next(jitterbuf *jb);

/* get jitterbuf info: only "statistics" may be valid */
int			jb_getinfo(jitterbuf *jb, cw_jb_info *stats);

/* set jitterbuf conf */
int			jb_setconf(jitterbuf *jb, jb_conf *conf);

typedef 		void (*jb_output_function_t)(const char *fmt, ...);
void 			jb_setoutput(jb_output_function_t err, jb_output_function_t warn, jb_output_function_t dbg);

#ifdef __cplusplus
}
#endif


#endif
