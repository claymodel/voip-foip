/*
 * abstract_jb: common implementation-independent jitterbuffer stuff
 *
 * Copyright (C) 2005, Attractel OOD
 *
 * Contributors:
 * Slav Klenov <slav@securax.org>
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
 * \brief Common implementation-independent jitterbuffer stuff.
 * 
 */

#ifndef _CALLWEAVER_GENERIC_JB_H_
#define _CALLWEAVER_GENERIC_JB_H_

#include <stdio.h>
#include <sys/time.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct cw_channel;
struct cw_frame;


/* Configuration flags */
#define CW_GENERIC_JB_ENABLED         (1 << 0)
#define CW_GENERIC_JB_FORCED          (1 << 1)
#define CW_GENERIC_JB_LOG             (1 << 2)

#define CW_GENERIC_JB_IMPL_NAME_SIZE 12

    /* How much too late a request for a frame may be accepted */
#define JB_LATENESS_TOLERANCE 0.25

/*!
 * \brief General jitterbuffer configuration.
 */
struct cw_jb_conf
{
    /*! \brief Combination of the CW_GENERIC_JB_ENABLED, CW_GENERIC_JB_FORCED and CW_GENERIC_JB_LOG flags. */
    unsigned int flags;
    /*! \brief Minimum size of the jitterbuffer implementation. */
    long min_size;
    /*! \brief Max size of the jitterbuffer implementation. */
    long max_size;
    /*! \brief Resynchronization threshold of the jitterbuffer implementation. */
     long resync_threshold;
    /*! \brief Timing compensation (in ms) */
    long timing_compensation;
    /*! \brief Name of the jitterbuffer implementation to be used. */
     char impl[CW_GENERIC_JB_IMPL_NAME_SIZE];
};


/* Jitterbuffer configuration property names */
#define CW_GENERIC_JB_CONF_PREFIX "jb-"
#define CW_GENERIC_JB_CONF_ENABLE "enable"
#define CW_GENERIC_JB_CONF_FORCE "force"
#define CW_GENERIC_JB_CONF_MIN_SIZE "min-size"
#define CW_GENERIC_JB_CONF_MAX_SIZE "max-size"
#define CW_GENERIC_JB_CONF_RESYNCH_THRESHOLD "resynch-threshold"
#define CW_GENERIC_JB_CONF_TIMING_COMP "timing-compensation"
#define CW_GENERIC_JB_CONF_IMPL "impl"
#define CW_GENERIC_JB_CONF_LOG "log"


struct cw_jb_impl;


/*!
 * \brief General jitterbuffer state.
 */
struct cw_jb
{
    /*! \brief Jitterbuffer configuration. */
    struct cw_jb_conf conf;
    /*! \brief Jitterbuffer implementation to be used. */
    struct cw_jb_impl *impl;
    /*! \brief Jitterbuffer object, passed to the implementation. */
    void *jbobj;
    /*! \brief The time the jitterbuffer was created. */
    struct timeval timebase;
    /*! \brief The time the next frame should be played. */
    long next;
    /*! \brief Voice format of the last frame in. */
    int last_format;
    /*! \brief File for frame timestamp tracing. */
    FILE *logfile;
    /*! \brief Jitterbuffer internal state flags. */
    unsigned int flags;
};

typedef struct cw_jb_info
{
    /* statistics */
    long frames_in;         /* number of frames input to the jitterbuffer.*/
    long frames_out;        /* number of frames output from the jitterbuffer.*/
    long frames_late;       /* number of frames which were too late, and dropped.*/
    long frames_lost;       /* number of missing frames.*/
    long frames_dropped;    /* number of frames dropped (shrinkage) */
    long frames_ooo;        /* number of frames received out-of-order */
    long frames_cur;        /* number of frames presently in jb, awaiting delivery.*/
    long jitter;            /* jitter measured within current history interval*/
    long min;               /* minimum lateness within current history interval */
    long current;           /* the present jitterbuffer adjustment */
    long target;            /* the target jitterbuffer adjustment */
    long losspct;           /* recent lost frame percentage (* 1000) */
    long next_voice_ts;     /* the ts of the next frame to be read from the jb - in receiver's time */
    long last_voice_ms;     /* the duration of the last voice frame */
    long silence_begin_ts;  /* the time of the last CNG frame, when in silence */
    long last_adjustment;   /* the time of the last adjustment */
    long last_delay;        /* the last now added to history */
    long cnt_delay_discont; /* the count of discontinuous delays */
    long resync_offset;     /* the amount to offset ts to support resyncs */
    long cnt_contig_interp; /* the number of contiguous interp frames returned */
    /* These are used by the SpeakUp JB */
    long delay;             /* Current delay due the jitterbuffer */
    long delay_target;      /* The delay where we want to grow to */
    long frames_received;   /* Number of frames received by the jitterbuffer */
    long frames_dropped_twice;  /* Number of frames that were dropped because this timestamp was already in the jitterbuffer */
    short silence;          /* If we are in silence 1-yes 0-no */
    long iqr;               /* Inter Quartile Range of current history, if the squareroot is taken it is a good estimate of jitter */
} cw_jb_info;

typedef struct jb_frame
{
    void *data;             /* the frame data */
    long ts;                /* the relative delivery time expected */
    long ms;                /* the time covered by this frame, in sec/8000 */
    int  type;              /* the type of frame */
    struct jb_frame *next, *prev;
    int codec;              /* codec of this frame, undefined if nonvoice */
} jb_frame;


/*!
 * \brief Checks the need of a jb use in a generic bridge.
 * \param c0 first bridged channel.
 * \param c1 second bridged channel.
 *
 * Called from cw_generic_bridge() when two channels are entering in a bridge.
 * The function checks the need of a jitterbuffer, depending on both channel's
 * configuration and technology properties. As a result, this function sets
 * appropriate internal jb flags to the channels, determining further behaviour
 * of the bridged jitterbuffers.
 */
void cw_jb_do_usecheck(struct cw_channel *c0, struct cw_channel *c1);


/*!
 * \brief Calculates the time, left to the closest delivery moment in a bridge.
 * \param c0 first bridged channel.
 * \param c1 second bridged channel.
 * \param time_left bridge time limit, or -1 if not set.
 *
 * Called from cw_generic_bridge() to determine the maximum time to wait for
 * activity in cw_waitfor_n() call. If neihter of the channels is using jb,
 * this function returns the time limit passed.
 *
 * \return maximum time to wait.
 */
int cw_jb_get_when_to_wakeup(struct cw_channel *c0, struct cw_channel *c1, int time_left);


/*!
 * \brief Puts a frame into a channel jitterbuffer.
 * \param chan channel.
 * \param frame frame.
 * \param codec the codec in use.
 *
 * Called from cw_generic_bridge() to put a frame into a channel's jitterbuffer.
 * The function will successfuly enqueue a frame if and only if:
 * 1. the channel is using a jitterbuffer (as determined by cw_jb_do_usecheck()),
 * 2. the frame's type is CW_FRAME_VOICE,
 * 3. the frame has timing info set and has length >= 2 ms,
 * 4. there is no some internal error happened (like failed memory allocation).
 * Frames, successfuly queued, should be delivered by the channel's jitterbuffer,
 * when their delivery time has came.
 * Frames, not successfuly queued, should be delivered immediately.
 * Dropped by the jb implementation frames are considered successfuly enqueued as
 * far as they should not be delivered at all.
 *
 * \return zero if the frame was queued, -1 if not.
 */
int cw_jb_put(struct cw_channel *chan, struct cw_frame *f, int codec);


/*!
 * \brief Deliver the queued frames that should be delivered now for both channels.
 * \param c0 first bridged channel.
 * \param c1 second bridged channel.
 *
 * Called from cw_generic_bridge() to deliver any frames, that should be delivered
 * for the moment of invocation. Does nothing if neihter of the channels is using jb
 * or has any frames currently queued in. The function delivers frames usig cw_write()
 * each of the channels.
 */
void cw_jb_get_and_deliver(struct cw_channel *c0, struct cw_channel *c1);


/*!
 * \brief Destroys jitterbuffer on a channel.
 * \param chan channel.
 *
 * Called from cw_channel_free() when a channel is destroyed.
 */
void cw_jb_destroy(struct cw_channel *chan);

/*!
 * \brief Sets default jitterbuffer data.
 * \param conf The jb configuration structure to be populated.
 *
 * This is called to populate the private jitter-buffer configuration with
 * default configuration data.
 * returns nothing.
 */
void cw_jb_default_config(struct cw_jb_conf *conf);


/*!
 * \brief Sets jitterbuffer configuration property.
 * \param conf configuration to store the property in.
 * \param varname property name.
 * \param value property value.
 *
 * Called from a channel driver to build a jitterbuffer configuration tipically when
 * reading a configuration file. It is not neccessary for a channel driver to know
 * each of the jb configuration property names. The jitterbuffer itself knows them.
 * The channel driver can pass each config var it reads through this function. It will
 * return 0 if the variable was consumed from the jb conf.
 *
 * \return zero if the property was set to the configuration, -1 if not.
 */
int cw_jb_read_conf(struct cw_jb_conf *conf, char *varname, char *value);


/*!
 * \brief Configures a jitterbuffer on a channel.
 * \param chan channel to configure.
 * \param conf configuration to apply.
 *
 * Called from a channel driver when a channel is created and its jitterbuffer needs
 * to be configured.
 */
void cw_jb_configure(struct cw_channel *chan, struct cw_jb_conf *conf);


/*!
 * \brief Copies a channel's jitterbuffer configuration.
 * \param chan channel.
 * \param conf destination.
 */
void cw_jb_get_config(struct cw_channel *chan, struct cw_jb_conf *conf);

/*!
 * \brief Get jitter buffer stats
 * \param chan channel.
 * \param info destination stats structure.
 */
void cw_jb_get_info(struct cw_channel *chan, cw_jb_info *info);

/*!
 * \brief Check if jitterbuffer is active
 * \param chan channel.
 */
int cw_jb_is_active(struct cw_channel *chan);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
