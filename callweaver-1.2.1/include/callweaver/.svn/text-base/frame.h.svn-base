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
 * \brief CallWeaver internal frame definitions.
 */

#ifndef _CALLWEAVER_FRAME_H
#define _CALLWEAVER_FRAME_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>

struct cw_codec_pref
{
    char order[32];
};

/*! Data structure associated with a single frame of data */
/* A frame of data read used to communicate between 
   between channels and applications */
typedef struct cw_frame
{
    /*! Kind of frame */
    int frametype;
    /*! Subclass, frame dependent */
    int subclass;
    /*! Length of data */
    int datalen;
    /*! Number of samples in this frame */
    int samples;
    /*! The sample rate of this frame (usually 8000 Hz */
    int samplerate;
    /*! Was the data malloc'd?  i.e. should we free it when we discard the frame? */
    int mallocd;
    /*! How many bytes exist _before_ "data" that can be used if needed */
    int offset;
    /*! Optional source of frame for debugging */
    const char *src;
    /*! Pointer to actual data */
    void *data;
    /*! Global delivery time */        
    struct timeval delivery;
    /*! Next/Prev for linking stand alone frames */
    struct cw_frame *prev;
    /*! Next/Prev for linking stand alone frames */
    struct cw_frame *next;
    /*! Timing data flag */
    int has_timing_info;
    /*! Timestamp in milliseconds */
    long ts;
    /*! Length in milliseconds */
    long len;
    /*! Sequence number */
    int seq_no;
    /*! Number of copies to send (for redundant transmission of special data) */
    int tx_copies;
    /*! Allocated data space */
    uint8_t local_data[0];
} opvx_frame_t;

#define CW_FRIENDLY_OFFSET    64        /*! It's polite for a a new frame to
                          have this number of bytes for additional
                          headers.  */
#define CW_MIN_OFFSET         32        /*! Make sure we keep at least this much handy */

/*! Need the header be free'd? */
#define CW_MALLOCD_HDR        (1 << 0)
/*! Need the data be free'd? */
#define CW_MALLOCD_DATA       (1 << 1)
/*! Need the source be free'd? (haha!) */
#define CW_MALLOCD_SRC        (1 << 2)

/* Frame types */
/*! A DTMF digit, subclass is the digit */
#define CW_FRAME_DTMF         1
/*! Voice data, subclass is CW_FORMAT_* */
#define CW_FRAME_VOICE        2
/*! Video frame, maybe?? :) */
#define CW_FRAME_VIDEO        3
/*! A control frame, subclass is CW_CONTROL_* */
#define CW_FRAME_CONTROL      4
/*! An empty, useless frame */
#define CW_FRAME_NULL         5
/*! Inter CallWeaver Exchange private frame type */
#define CW_FRAME_IAX          6
/*! Text messages */
#define CW_FRAME_TEXT         7
/*! Image Frames */
#define CW_FRAME_IMAGE        8
/*! HTML Frame */
#define CW_FRAME_HTML         9
/*! Comfort noise frame (subclass is level of CNG in -dBov), 
    body may include zero or more 8-bit reflection coefficients */
#define CW_FRAME_CNG          10
/*! T.38, V.150 or other modem-over-IP data stream */
#define CW_FRAME_MODEM        11

/* MODEM subclasses */
/*! T.38 Fax-over-IP */
#define CW_MODEM_T38          1
/*! V.150 Modem-over-IP */
#define CW_MODEM_V150         2

/* HTML subclasses */
/*! Sending a URL */
#define CW_HTML_URL           1
/*! Data frame */
#define CW_HTML_DATA          2
/*! Beginning frame */
#define CW_HTML_BEGIN         4
/*! End frame */
#define CW_HTML_END           8
/*! Load is complete */
#define CW_HTML_LDCOMPLETE    16
/*! Peer is unable to support HTML */
#define CW_HTML_NOSUPPORT     17
/*! Send URL, and track */
#define CW_HTML_LINKURL       18
/*! No more HTML linkage */
#define CW_HTML_UNLINK        19
/*! Reject link request */
#define CW_HTML_LINKREJECT    20

/* Data formats for capabilities and frames alike */
#define CW_AUDIO_CODEC_MASK   0xFFFF

/*! G.723.1 compression */
#define CW_FORMAT_G723_1      (1 << 0)
/*! GSM compression */
#define CW_FORMAT_GSM         (1 << 1)
/*! Raw mu-law data (G.711) */
#define CW_FORMAT_ULAW        (1 << 2)
/*! Raw A-law data (G.711) */
#define CW_FORMAT_ALAW        (1 << 3)
/*! G.726 ADPCM at 32kbps) */
#define CW_FORMAT_G726        (1 << 4)
/*! IMA/DVI/Intel ADPCM */
#define CW_FORMAT_DVI_ADPCM   (1 << 5)
/*! Raw 16-bit Signed Linear (8000 Hz) PCM */
#define CW_FORMAT_SLINEAR     (1 << 6)
/*! LPC10, 180 samples/frame */
#define CW_FORMAT_LPC10       (1 << 7)
/*! G.729A audio */
#define CW_FORMAT_G729A       (1 << 8)
/*! SpeeX Free Compression */
#define CW_FORMAT_SPEEX       (1 << 9)
/*! iLBC Free Compression */
#define CW_FORMAT_ILBC        (1 << 10)
/*! Oki ADPCM */
#define CW_FORMAT_OKI_ADPCM   (1 << 11)
/*! G.722 */
#define CW_FORMAT_G722        (1 << 12)
/*! Maximum audio format */
#define CW_FORMAT_MAX_AUDIO   (1 << 15)
/*! JPEG Images */
#define CW_FORMAT_JPEG        (1 << 16)
/*! PNG Images */
#define CW_FORMAT_PNG         (1 << 17)
/*! H.261 Video */
#define CW_FORMAT_H261        (1 << 18)
/*! H.263 Video */
#define CW_FORMAT_H263        (1 << 19)
/*! H.263+ Video */
#define CW_FORMAT_H263_PLUS   (1 << 20)
/*! H.264 Video */
#define CW_FORMAT_H264        (1 << 21)
/*! Maximum video format */
#define CW_FORMAT_MAX_VIDEO   (1 << 24)

/* Control frame types */
/*! Stop playing indications */
#define CW_STATE_STOP_INDICATING  -1      
/*! Other end has hungup */
#define CW_CONTROL_HANGUP         1
/*! Local ring */
#define CW_CONTROL_RING           2
/*! Remote end is ringing */
#define CW_CONTROL_RINGING        3
/*! Remote end has answered */
#define CW_CONTROL_ANSWER         4
/*! Remote end is busy */
#define CW_CONTROL_BUSY           5
/*! Make it go off hook */
#define CW_CONTROL_TAKEOFFHOOK    6
/*! Line is off hook */
#define CW_CONTROL_OFFHOOK        7
/*! Congestion (circuits busy) */
#define CW_CONTROL_CONGESTION     8
/*! Flash hook */
#define CW_CONTROL_FLASH          9
/*! Wink */
#define CW_CONTROL_WINK           10
/*! Set a low-level option */
#define CW_CONTROL_OPTION         11
/*! Key Radio */
#define    CW_CONTROL_RADIO_KEY   12
/*! Un-Key Radio */
#define    CW_CONTROL_RADIO_UNKEY 13
/*! Indicate PROGRESS */
#define CW_CONTROL_PROGRESS       14
/*! Indicate CALL PROCEEDING */
#define CW_CONTROL_PROCEEDING     15
/*! Indicate call is placed on hold */
#define CW_CONTROL_HOLD           16
/*! Indicate call is left from hold */
#define CW_CONTROL_UNHOLD         17
/*! Indicate video frame update */
#define CW_CONTROL_VIDUPDATE      18

#define CW_SMOOTHER_FLAG_G729     (1 << 0)
#define CW_SMOOTHER_FLAG_BE       (1 << 1)

/* Option identifiers and flags */
#define CW_OPTION_FLAG_REQUEST    0
#define CW_OPTION_FLAG_ACCEPT     1
#define CW_OPTION_FLAG_REJECT     2
#define CW_OPTION_FLAG_QUERY      4
#define CW_OPTION_FLAG_ANSWER     5
#define CW_OPTION_FLAG_WTF        6

/* Verify touchtones by muting audio transmission 
    (and reception) and verify the tone is still present */
#define CW_OPTION_TONE_VERIFY     1        

/* Put a compatible channel into TDD (TTY for the hearing-impared) mode */
#define    CW_OPTION_TDD          2

/* Relax the parameters for DTMF reception (mainly for radio use) */
#define    CW_OPTION_RELAXDTMF    3

/* Set (or clear) Audio (Not-Clear) Mode */
#define    CW_OPTION_AUDIO_MODE   4

/* Set channel transmit gain */
/* Option data is a single signed char
   representing number of decibels (dB)
   to set gain to (on top of any gain
   specified in channel driver)
*/
#define CW_OPTION_TXGAIN          5

/* Set channel receive gain */
/* Option data is a single signed char
   representing number of decibels (dB)
   to set gain to (on top of any gain
   specified in channel driver)
*/
#define CW_OPTION_RXGAIN          6

struct cw_option_header {
    /* Always keep in network byte order */
#if __BYTE_ORDER == __BIG_ENDIAN
        u_int16_t flag:3;
        u_int16_t option:13;
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
        u_int16_t option:13;
        u_int16_t flag:3;
#else
#error Byte order not defined
#endif
#endif
        u_int8_t data[0];
};

/*  Requests a frame to be allocated */
/* 
 * \param source 
 * Request a frame be allocated.  source is an optional source of the frame, 
 * len is the requested length, or "0" if the caller will supply the buffer 
 */
#if 0 /* Unimplemented */
struct cw_frame *cw_fralloc(char *source, int len);
#endif

/*! Initialises a frame */
/*! 
 * \param fr Frame to initialise
 * Initialise the contenst of a frame to sane values.
 * no return.
 */
void cw_fr_init(struct cw_frame *fr);

void cw_fr_init_ex(struct cw_frame *fr,
                     int frame_type,
                     int sub_type,
                     const char *src);

/*! Frees a frame */
/*! 
 * \param fr Frame to free
 * Free a frame, and the memory it used if applicable
 * no return.
 */
void cw_fr_free(struct cw_frame *fr);

/*! Copies a frame */
/*! 
 * \param fr frame to act upon
 * Take a frame, and if it's not been malloc'd, make a malloc'd copy
 * and if the data hasn't been malloced then make the
 * data malloc'd.  If you need to store frames, say for queueing, then
 * you should call this function.
 * Returns a frame on success, NULL on error
 */
struct cw_frame *cw_frisolate(struct cw_frame *fr);

/*! Copies a frame */
/*! 
 * \param fr frame to copy
 * Dupliates a frame -- should only rarely be used, typically frisolate is good enough
 * Returns a frame on success, NULL on error
 */
struct cw_frame *cw_frdup(struct cw_frame *fr);

/*! Reads a frame from an fd */
/*! 
 * \param fd an opened fd to read from
 * Read a frame from a stream or packet fd, as written by fd_write
 * returns a frame on success, NULL on error
 */
struct cw_frame *cw_fr_fdread(int fd);

/*! Writes a frame to an fd */
/*! 
 * \param fd Which fd to write to
 * \param frame frame to write to the fd
 * Write a frame to an fd
 * Returns 0 on success, -1 on failure
 */
int cw_fr_fdwrite(int fd, struct cw_frame *frame);

/*! Sends a hangup to an fd */
/*! 
 * \param fd fd to write to
 * Send a hangup (NULL equivalent) on an fd
 * Returns 0 on success, -1 on failure
 */
int cw_fr_fdhangup(int fd);

void cw_swapcopy_samples(void *dst, const void *src, int samples);

/* Helpers for byteswapping native samples to/from 
   little-endian and big-endian. */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cw_frame_byteswap_le(fr) do { ; } while(0)
#define cw_frame_byteswap_be(fr) do { struct cw_frame *__f = (fr); cw_swapcopy_samples(__f->data, __f->data, __f->samples); } while(0)
#else
#define cw_frame_byteswap_le(fr) do { struct cw_frame *__f = (fr); cw_swapcopy_samples(__f->data, __f->data, __f->samples); } while(0)
#define cw_frame_byteswap_be(fr) do { ; } while(0)
#endif


/*! Get the name of a format */
/*!
 * \param format id of format
 * \return A static string containing the name of the format or "UNKN" if unknown.
 */
extern char* cw_getformatname(int format);

/*! Get the names of a set of formats */
/*!
 * \param buf a buffer for the output string
 * \param n size of buf (bytes)
 * \param format the format (combined IDs of codecs)
 * Prints a list of readable codec names corresponding to "format".
 * ex: for format=CW_FORMAT_GSM|CW_FORMAT_SPEEX|CW_FORMAT_ILBC it will return "0x602 (GSM|SPEEX|ILBC)"
 * \return The return value is buf.
 */
extern char* cw_getformatname_multiple(char *buf, size_t size, int format);

/*!
 * \param name string of format
 * Gets a format from a name.
 * This returns the form of the format in binary on success, 0 on error.
 */
extern int cw_getformatbyname(char *name);

/*! Get a name from a format */
/*!
 * \param codec codec number (1,2,4,8,16,etc.)
 * Gets a name from a format
 * This returns a static string identifying the format on success, 0 on error.
 */
extern char *cw_codec2str(int codec);

struct cw_smoother;

extern struct cw_format_list_s *cw_get_format_list_index(int index);
extern struct cw_format_list_s *cw_get_format_list(size_t *size);
extern struct cw_smoother *cw_smoother_new(int bytes);
extern void cw_smoother_set_flags(struct cw_smoother *smoother, int flags);
extern int cw_smoother_test_flag(struct cw_smoother *s, int flag);
extern int cw_smoother_get_flags(struct cw_smoother *smoother);
extern void cw_smoother_free(struct cw_smoother *s);
extern void cw_smoother_reset(struct cw_smoother *s, int bytes);
extern int __cw_smoother_feed(struct cw_smoother *s, struct cw_frame *f, int swap);
extern struct cw_frame *cw_smoother_read(struct cw_smoother *s);

extern int cw_codec_sample_rate(struct cw_frame *f);

#define cw_smoother_feed(s,f) __cw_smoother_feed(s, f, 0)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cw_smoother_feed_be(s,f) __cw_smoother_feed(s, f, 1)
#define cw_smoother_feed_le(s,f) __cw_smoother_feed(s, f, 0)
#else
#define cw_smoother_feed_be(s,f) __cw_smoother_feed(s, f, 0)
#define cw_smoother_feed_le(s,f) __cw_smoother_feed(s, f, 1)
#endif

extern void cw_frame_dump(char *name, struct cw_frame *f, char *prefix);

/* Initialize a codec preference to "no preference" */
extern void cw_codec_pref_init(struct cw_codec_pref *pref);

/* Codec located at  a particular place in the preference index */
extern int cw_codec_pref_index(struct cw_codec_pref *pref, int index);

/* Remove a codec from a preference list */
extern void cw_codec_pref_remove(struct cw_codec_pref *pref, int format);

/* Append a codec to a preference list, removing it first if it was already there */
extern int cw_codec_pref_append(struct cw_codec_pref *pref, int format);

/* Select the best format according to preference list from supplied options. 
   If "find_best" is non-zero then if nothing is found, the "Best" format of 
   the format list is selected, otherwise 0 is returned. */
extern int cw_codec_choose(struct cw_codec_pref *pref, int formats, int find_best);

/* Parse an "allow" or "deny" line and update the mask and pref if provided */
extern void cw_parse_allow_disallow(struct cw_codec_pref *pref, int *mask, const char *list, int allowing);

/* Dump codec preference list into a string */
extern int cw_codec_pref_string(struct cw_codec_pref *pref, char *buf, size_t size);

/* Shift a codec preference list up or down 65 bytes so that it becomes an ASCII string */
extern void cw_codec_pref_convert(struct cw_codec_pref *pref, char *buf, size_t size, int right);

/* Returns the number of samples contained in the frame */
extern int cw_codec_get_samples(struct cw_frame *f);

/* Returns the number of bytes for the number of samples of the given format */
extern int cw_codec_get_len(int format, int samples);

/* Gets duration in ms of interpolation frame for a format */
static inline int cw_codec_interp_len(int format) 
{ 
    return (format == CW_FORMAT_ILBC) ? 30 : 20;
}

/*!
  \brief Adjusts the volume of the audio samples contained in a frame.
  \param f The frame containing the samples (must be CW_FRAME_VOICE and CW_FORMAT_SLINEAR)
  \param adjustment The number of dB to adjust up or down.
  \return 0 for success, non-zero for an error
 */
int cw_frame_adjust_volume(struct cw_frame *f, int adjustment);

/*!
  \brief Sums two frames of audio samples.
  \param f1 The first frame (which will contain the result)
  \param f2 The second frame
  \return 0 for success, non-zero for an error

  The frames must be CW_FRAME_VOICE and must contain CW_FORMAT_SLINEAR samples,
  and must contain the same number of samples.
 */
int cw_frame_slinear_sum(struct cw_frame *f1, struct cw_frame *f2);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_FRAME_H */
