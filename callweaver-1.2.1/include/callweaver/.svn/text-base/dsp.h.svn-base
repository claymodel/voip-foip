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
 * \brief Convenient Signal Processing routines
 */

#ifndef _CALLWEAVER_DSP_H
#define _CALLWEAVER_DSP_H

#define DSP_FEATURE_SILENCE_SUPPRESS    (1 << 0)
#define DSP_FEATURE_BUSY_DETECT         (1 << 1)
#define DSP_FEATURE_DTMF_DETECT         (1 << 3)
#define DSP_FEATURE_FAX_CNG_DETECT      (1 << 4)
#define DSP_FEATURE_FAX_CED_DETECT      (1 << 5)

#define    DSP_DIGITMODE_DTMF       0               /*! \brief Detect DTMF digits */
#define DSP_DIGITMODE_MF            1               /*! \brief Detect MF digits */

#define DSP_DIGITMODE_NOQUELCH      (1 << 8)        /*! \brief Do not quelch DTMF from in-band */
#define DSP_DIGITMODE_MUTECONF      (1 << 9)        /*! \brief Mute conference */
#define DSP_DIGITMODE_MUTEMAX       (1 << 10)       /*! \brief Delay audio by a frame to try to extra quelch */
#define DSP_DIGITMODE_RELAXDTMF     (1 << 11)       /*! \brief "Radio" mode (relaxed DTMF) */

#define DSP_PROGRESS_TALK           (1 << 16)       /*! \brief Enable talk detection */
#define DSP_PROGRESS_RINGING        (1 << 17)       /*! \brief Enable calling tone detection */
#define DSP_PROGRESS_BUSY           (1 << 18)       /*! \brief Enable busy tone detection */
#define DSP_PROGRESS_CONGESTION     (1 << 19)       /*! \brief Enable congestion tone detection */
#define DSP_FEATURE_CALL_PROGRESS   (DSP_PROGRESS_TALK | DSP_PROGRESS_RINGING | DSP_PROGRESS_BUSY | DSP_PROGRESS_CONGESTION)

#define DSP_TONE_STATE_SILENCE      0
#define DSP_TONE_STATE_RINGING      1
#define DSP_TONE_STATE_DIALTONE     2
#define DSP_TONE_STATE_TALKING      3
#define DSP_TONE_STATE_BUSY         4
#define DSP_TONE_STATE_SPECIAL1     5
#define DSP_TONE_STATE_SPECIAL2     6
#define DSP_TONE_STATE_SPECIAL3     7
#define DSP_TONE_STATE_HUNGUP       8

struct cw_dsp;

struct cw_dsp *cw_dsp_new(void);

void cw_dsp_free(struct cw_dsp *dsp);

/*! \brief Set threshold value for silence */
void cw_dsp_set_threshold(struct cw_dsp *dsp, int threshold);

/*! \brief Set number of required cadences for busy */
void cw_dsp_set_busy_count(struct cw_dsp *dsp, int cadences);

/*! \brief Set expected lengths of the busy tone */
void cw_dsp_set_busy_pattern(struct cw_dsp *dsp, int tonelength, int quietlength);

/*! \brief Scans for progress indication in audio */
int cw_dsp_call_progress(struct cw_dsp *dsp, struct cw_frame *inf);

/*! \brief Set zone for doing progress detection */
int cw_dsp_set_call_progress_zone(struct cw_dsp *dsp, char *zone);

/*! \brief Return CW_FRAME_NULL frames when there is silence, CW_FRAME_BUSY on 
   busies, and call progress, all dependent upon which features are enabled */
struct cw_frame *cw_dsp_process(struct cw_channel *chan, struct cw_dsp *dsp, struct cw_frame *inf);

/*! \brief Return non-zero if this is silence.  Updates "totalsilence" with the total
   number of seconds of silence  */
int cw_dsp_silence(struct cw_dsp *dsp, struct cw_frame *f, int *totalsilence);

/*! \brief Return non-zero if historically this should be a busy, request that
  cw_dsp_silence has already been called */
int cw_dsp_busydetect(struct cw_dsp *dsp);

/*! \brief Reset total silence count */
void cw_dsp_reset(struct cw_dsp *dsp);

/*! \brief Reset DTMF detector */
void cw_dsp_digitreset(struct cw_dsp *dsp);

/*! \brief Select feature set */
void cw_dsp_set_features(struct cw_dsp *dsp, int features);

/*! \brief Set digit mode */
int cw_dsp_digitmode(struct cw_dsp *dsp, int digitmode);

#endif /* _CALLWEAVER_DSP_H */
