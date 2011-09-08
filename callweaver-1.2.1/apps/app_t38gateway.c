/*
 * vim:softtabstop=4:noexpandtab
 *
 * CallWeaver -- An open source telephony toolkit.
 *
 * Trivial application to act as a T.38 gateway
 * 
 * Copyright (C) 2005, Anthony Minessale II
 * Copyright (C) 2003, 2005, Steve Underwood
 *
 * Anthony Minessale II <anthmct@yahoo.com>
 * Steve Underwood <steveu@coppice.org>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
#include <spandsp.h>

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/causes.h"
#include "callweaver/dsp.h"
#include "callweaver/app.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/lock.h"

static char *tdesc = "T.38 Gateway Dialer Application";

static void *t38gateway_app;
static const char *t38gateway_name = "T38Gateway";
static const char *t38gateway_synopsis = "A PSTN <-> T.38 gateway";
static const char *t38gateway_syntax = "T38Gateway(dialstring[, timeout[, options]])";
static const char *t38gateway_descrip =
"Options:\n\n"
" h -- Hangup if the call was successful.\n\n"
" r -- Indicate 'ringing' to the caller.\n\n"
"Sets FAXPAGES to the number of pages transferred\n\n";


static int cw_check_hangup_locked(struct cw_channel *chan)
{
    int res;

    cw_mutex_lock(&chan->lock);
    res = cw_check_hangup(chan);
    cw_mutex_unlock(&chan->lock);
    return res;
}

#define clean_frame(f) if(f) {cw_fr_free(f); f = NULL;}
#define ALL_DONE(u,ret) {pbx_builtin_setvar_helper(chan, "DIALSTATUS", status); cw_indicate(chan, -1); LOCAL_USER_REMOVE(u) ; return ret;}

#define ready_to_talk(chan,peer) ((!chan  ||  !peer  ||  cw_check_hangup_locked(chan)  ||  cw_check_hangup_locked(peer))  ?  0  :  1)

#define DONE_WITH_ERROR -1
#define RUNNING 1
#define DONE 0

#define MAX_BLOCK_SIZE 240

STANDARD_LOCAL_USER;
LOCAL_USER_DECL;

static void span_message(int level, const char *msg)
{
    int cw_level;
    
    if (level == SPAN_LOG_ERROR)
        cw_level = __LOG_ERROR;
    else if (level == SPAN_LOG_WARNING)
        cw_level = __LOG_WARNING;
    else
        cw_level = __LOG_DEBUG;
    //cw_level = __LOG_WARNING;
    cw_log(cw_level, __FILE__, __LINE__, __PRETTY_FUNCTION__, msg);
}

/* cw_bridge_audio(chan,peer);
   this is a no-nonsense optionless bridge function that probably needs to grow a little.
   This function makes no attempt to perform a native bridge or do anything cool because it's
   main usage is for situations where you are doing a translated codec in a VOIP gateway
   where you simply want to forward the call elsewhere.
   This is my perception of what cw_channel_bridge may have looked like in the beginning ;)
*/
static int cw_bridge_frames(struct cw_channel *chan, struct cw_channel *peer)
{
    struct cw_channel *active = NULL;
    struct cw_channel *inactive = NULL;
    struct cw_channel *channels[2];
    struct cw_frame *f;
    struct cw_frame *fr2;
    int timeout = -1;
    int running = RUNNING;
    struct cw_dsp *dsp_cng = NULL;
    struct cw_dsp *dsp_ced = NULL;
    struct cw_dsp *dspx;

    if ((dsp_cng = cw_dsp_new()) == NULL)
    {
        cw_log(LOG_WARNING, "Unable to allocate DSP!\n");
    }
    else
    {
        cw_dsp_set_threshold(dsp_cng, 256); 
        cw_dsp_set_features(dsp_cng, DSP_FEATURE_DTMF_DETECT | DSP_FEATURE_FAX_CNG_DETECT);
        cw_dsp_digitmode(dsp_cng, DSP_DIGITMODE_DTMF | DSP_DIGITMODE_RELAXDTMF);
    }

    if ((dsp_ced = cw_dsp_new()) == NULL)
    {
        cw_log(LOG_WARNING, "Unable to allocate DSP!\n");
    }
    else
    {
        cw_dsp_set_threshold(dsp_ced, 256); 
        cw_dsp_set_features(dsp_ced, DSP_FEATURE_FAX_CED_DETECT);
    }

    channels[0] = chan;
    channels[1] = peer;

    while (running == RUNNING  &&  (running = ready_to_talk(channels[0], channels[1])))
    {
        //cw_log(LOG_NOTICE, "br: t38 status: [%d,%d]\n", chan->t38_status, peer->t38_status);

        if ((active = cw_waitfor_n(channels, 2, &timeout)))
        {
            inactive = (active == channels[0])  ?   channels[1]  :  channels[0];
            if ((f = cw_read(active)))
            {

                if (dsp_ced  ||  dsp_cng)
                    fr2 = cw_frdup(f);
                else
                    fr2 = NULL;

                f->tx_copies = 1; /* TODO: this is only needed because not everything sets the tx_copies field properly */

                if ((chan->t38_status == T38_NEGOTIATING)  ||  (peer->t38_status == T38_NEGOTIATING))
                {
                    /*  TODO 
                        This is a very BASIC method to mute a channel. It should be improved
                        and we should send EMPTY frames (not just avoid sending them) 
                    */
                    cw_log(LOG_DEBUG, "channels are muted.\n");
                }
                else
                {
                    cw_write(inactive, f);
                }
    
                clean_frame(f);
                channels[0] = inactive;
                channels[1] = active;

                if (active == chan)
                {
                    /* Look for FAX CNG tone */
                    dspx = dsp_cng;
                }
                else
                {
                    /* Look for FAX CED tone or V.21 preamble */
                    dspx = dsp_ced;
                }
                if (dspx  &&  fr2)
                {
                    if ((fr2 = cw_dsp_process(active, dspx, fr2)))
                    {
                        if (fr2->frametype == CW_FRAME_DTMF)
                        {
                            if (fr2->subclass == 'f'
                                ||
                                fr2->subclass == 'F')
                            {
                                cw_log(LOG_DEBUG, "FAX %s tone detected in T38 gateway!!!\n", (fr2->subclass == 'f')  ?  "CNG"  :  "CED");
                                cw_app_request_t38(chan);
                                /* Prevent any further attempts to negotiate T.38 */
                                if (dsp_cng)
                                {
                                    cw_dsp_free(dsp_cng);
                                    dsp_cng = NULL;
                                }
                                if (dsp_ced)
                                {
                                    cw_dsp_free(dsp_ced);
                                    dsp_ced = NULL;
                                }
                            }
                        }
                    }
                }
                if (f != fr2)
                {
                    if (fr2)
                        cw_fr_free(fr2);
                    fr2 = NULL;
                }
            }
            else
            {
                running = DONE;
            }
        }

        /* Check if we need to change to gateway operation */
        if ((chan->t38_status != T38_NEGOTIATING) 
            &&
            (peer->t38_status != T38_NEGOTIATING)
            &&
            (chan->t38_status != peer->t38_status))
        {
            cw_log(LOG_DEBUG, "Stop bridging frames. [ %d,%d]\n", chan->t38_status, peer->t38_status);
            running = RUNNING;
            break;
        }
    }

    if (dsp_cng)
        cw_dsp_free(dsp_cng);
    if (dsp_ced)
        cw_dsp_free(dsp_ced);

    return running;
}

static int t38_tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count)
{
    struct cw_frame outf;
    struct cw_channel *chan;

    chan = (struct cw_channel *) user_data;
    cw_fr_init_ex(&outf, CW_FRAME_MODEM, CW_MODEM_T38, "T38Gateway");
    outf.datalen = len;
    outf.data = (uint8_t *) buf;
    cw_log(LOG_DEBUG, "t38_tx_packet_handler: Sending %d copies of frame\n", count);
    outf.tx_copies = count;
    if (cw_write(chan, &outf) < 0)
        cw_log(LOG_WARNING, "Unable to write frame to channel; %s\n", strerror(errno));
    return 0;
}

static int cw_t38_gateway(struct cw_channel *chan, struct cw_channel *peer, int verbose)
{
    struct cw_channel *active = NULL;
    struct cw_channel *inactive = NULL;
    struct cw_channel *channels[2];
    struct cw_frame *f;
    struct cw_frame outf;
    int timeout = -1;
    int running = RUNNING;
    int original_read_fmt;
    int original_write_fmt;
    int res;
    int samples;
    int len;
    char *x;
    char text[128];
    t38_stats_t stats;
    t38_gateway_state_t t38_state;
    uint8_t __buf[sizeof(uint16_t)*MAX_BLOCK_SIZE + 2*CW_FRIENDLY_OFFSET];
    uint8_t *buf = __buf + CW_FRIENDLY_OFFSET;
    t38_core_state_t *t38_core;

    if (chan->t38_status == T38_NEGOTIATED)
    {
        channels[0] = chan;
        channels[1] = peer;
    }
    else
    {
        channels[0] = peer;
        channels[1] = chan;
    }

    original_read_fmt = channels[1]->readformat;
    original_write_fmt = channels[1]->writeformat;
    if (channels[1]->t38_status != T38_NEGOTIATED)
    {
        if (original_read_fmt != CW_FORMAT_SLINEAR)
        {
            if ((res = cw_set_read_format(channels[1], CW_FORMAT_SLINEAR)) < 0)
            {
                cw_log(LOG_WARNING, "Unable to set to linear read mode, giving up\n");
                return -1;
            }
        }
        if (original_write_fmt != CW_FORMAT_SLINEAR)
        {
            if ((res = cw_set_write_format(channels[1], CW_FORMAT_SLINEAR)) < 0)
            {
                cw_log(LOG_WARNING, "Unable to set to linear write mode, giving up\n");
                if ((res = cw_set_read_format(channels[1], original_read_fmt)))
                    cw_log(LOG_WARNING, "Unable to restore read format on '%s'\n", channels[1]->name);
                return -1;
            }
        }
    }

    if (t38_gateway_init(&t38_state, t38_tx_packet_handler, channels[0]) == NULL)
    {
        cw_log(LOG_WARNING, "Unable to start the T.38 gateway\n");
        return -1;
    }
    t38_gateway_set_transmit_on_idle(&t38_state, TRUE);
    t38_core = t38_gateway_get_t38_core_state(&t38_state);
    x = pbx_builtin_getvar_helper(chan, "FAX_DISABLE_V17");
    if (x  &&  x[0])
        t38_gateway_set_supported_modems(&t38_state, T30_SUPPORT_V29 | T30_SUPPORT_V27TER);
    else
        t38_gateway_set_supported_modems(&t38_state, T30_SUPPORT_V17 | T30_SUPPORT_V29 | T30_SUPPORT_V27TER);

    x = pbx_builtin_getvar_helper(chan, "FAX_DISABLE_ECM");
    if (x  &&  x[0])
    {
        t38_gateway_set_ecm_capability(&t38_state, FALSE);
        cw_log(LOG_DEBUG, "Disabling ECM mode\n");
    }
    else 
    {
        t38_gateway_set_ecm_capability(&t38_state, TRUE);
    }

    span_log_set_message_handler(&t38_state.logging, span_message);
    span_log_set_message_handler(&t38_core->logging, span_message);
    if (verbose)
    {
        span_log_set_level(&t38_state.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
        span_log_set_level(&t38_core->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
    }
    t38_set_t38_version(t38_core, 0);
    t38_gateway_set_ecm_capability(&t38_state, 1);

    while (running == RUNNING  &&  (running = ready_to_talk(channels[0], channels[1])))
    {
        //cw_log(LOG_NOTICE, "gw: t38status: [%d,%d]\n", chan->t38_status, peer->t38_status);
        if ((chan->t38_status == T38_NEGOTIATED) 
            &&
            (peer->t38_status == T38_NEGOTIATED))
        {
            cw_log(LOG_DEBUG, "Stop gateway-ing frames (both channels are in t38 mode). [ %d,%d]\n", chan->t38_status, peer->t38_status);
            running = RUNNING;
            break;
        }

        if ((active = cw_waitfor_n(channels, 2, &timeout)))
        {
            if (active == channels[0])
            {
                if ((f = cw_read(active)))
                {
                    t38_core_rx_ifp_packet(t38_core, f->data, f->datalen, f->seq_no);
                    clean_frame(f);
                }
                else
                {
                    running = DONE;
                }
            }
            else
            {
                if ((f = cw_read(active)))
                {
                    if (t38_gateway_rx(&t38_state, f->data, f->samples))
                    {
                        clean_frame(f);
                        break;
                    }
                    samples = (f->samples <= MAX_BLOCK_SIZE)  ?  f->samples  :  MAX_BLOCK_SIZE;

                    if ((len = t38_gateway_tx(&t38_state, (int16_t *) &buf[CW_FRIENDLY_OFFSET], samples)))
                    {
                        cw_fr_init_ex(&outf, CW_FRAME_VOICE, CW_FORMAT_SLINEAR, "T38Gateway");
                        outf.datalen = len*sizeof(int16_t);
                        outf.samples = len;
                        outf.data = &buf[CW_FRIENDLY_OFFSET];
                        outf.offset = CW_FRIENDLY_OFFSET;
                        if (cw_write(channels[1], &outf) < 0)
                        {
                            cw_log(LOG_WARNING, "Unable to write frame to channel; %s\n", strerror(errno));
                            clean_frame(f);
                            break;
                        }
                    }
                    clean_frame(f);
                }
                else
                {
                    running = DONE;
                }
                inactive = (active == channels[0])  ?   channels[1]  :  channels[0];
            }
        }
    }

    t38_gateway_get_transfer_statistics(&t38_state, &stats);
    snprintf(text, sizeof(text), "%d", stats.pages_transferred);
    pbx_builtin_setvar_helper(chan, "FAXPAGES", text);

    if (original_read_fmt != CW_FORMAT_SLINEAR)
    {
        if ((res = cw_set_read_format(channels[1], original_read_fmt)))
            cw_log(LOG_WARNING, "Unable to restore read format on '%s'\n", channels[1]->name);
    }
    if (original_write_fmt != CW_FORMAT_SLINEAR)
    {
        if ((res = cw_set_write_format(channels[1], original_write_fmt)))
            cw_log(LOG_WARNING, "Unable to restore write format on '%s'\n", channels[1]->name);
    }
    return running;
}

static int t38gateway_exec(struct cw_channel *chan, int argc, char **argv)
{
    int res = 0;
    struct localuser *u;
    char *dest = NULL;
    struct cw_channel *peer;
    int state = 0, ready = 0;
    int timeout;
    int format = chan->nativeformats;
    struct cw_frame *f;
    int verbose;
    char status[256];
    struct cw_channel *active = NULL;
    struct cw_channel *channels[2];
    
    if (argc < 1  ||  argc > 3  ||  !argv[0][0])
    {
        cw_log(LOG_ERROR, "Syntax: %s\n", t38gateway_syntax);
        return -1;
    }

    LOCAL_USER_ADD(u);

    verbose = TRUE;

    timeout = (argc > 1 && argv[1][0] ? atoi(argv[1]) * 1000 : 60000);

    if ((dest = strchr(argv[0], '/')))
    {
        int cause = 0;
        *dest = '\0';
        dest++;

        if (!(peer = cw_request(argv[0], format, dest, &cause)))
        {
            strncpy(status, "CHANUNAVAIL", sizeof(status) - 1); /* assume as default */
            cw_log(LOG_ERROR, "Error creating channel %s/%s\n", argv[0], dest);
            ALL_DONE(u, 0);
        }

        if (peer)
        {
            cw_channel_inherit_variables(chan, peer);
            peer->appl = "AppT38GW (Outgoing Line)";
            peer->whentohangup = 0;
            if (peer->cid.cid_num)
                free(peer->cid.cid_num);
            peer->cid.cid_num = NULL;
            if (peer->cid.cid_name)
                free(peer->cid.cid_name);
            peer->cid.cid_name = NULL;
            if (peer->cid.cid_ani)
                free(peer->cid.cid_ani);
            peer->cid.cid_ani = NULL;
            peer->transfercapability = chan->transfercapability;
            peer->adsicpe = chan->adsicpe;
            peer->cid.cid_tns = chan->cid.cid_tns;
            peer->cid.cid_ton = chan->cid.cid_ton;
            peer->cid.cid_pres = chan->cid.cid_pres;
            peer->cdrflags = chan->cdrflags;
            if (chan->cid.cid_rdnis)
                peer->cid.cid_rdnis = strdup(chan->cid.cid_rdnis);
            if (chan->cid.cid_num) 
                peer->cid.cid_num = strdup(chan->cid.cid_num);
            if (chan->cid.cid_name) 
                peer->cid.cid_name = strdup(chan->cid.cid_name);
            if (chan->cid.cid_ani) 
                peer->cid.cid_ani = strdup(chan->cid.cid_ani);
            cw_copy_string(peer->language, chan->language, sizeof(peer->language));
            cw_copy_string(peer->accountcode, chan->accountcode, sizeof(peer->accountcode));
            peer->cdrflags = chan->cdrflags;
            if (cw_strlen_zero(peer->musicclass))
                cw_copy_string(peer->musicclass, chan->musicclass, sizeof(peer->musicclass));        
        }
        if (argc > 2 && strchr(argv[2], 'r'))
            cw_indicate(chan, CW_CONTROL_RINGING);
    }
    else
    {
        cw_log(LOG_ERROR, "Error creating channel. Invalid name %s\n", argv[0]);
        ALL_DONE(u, 0);
    }
    if ((res = cw_call(peer, dest, 0)) < 0)
        ALL_DONE(u, -1); 
    strncpy(status, "CHANUNAVAIL", sizeof(status) - 1); /* assume as default */
    channels[0] = peer;
    channels[1] = chan;
    if (cw_channel_make_compatible(chan, peer))
    {
        cw_log(LOG_ERROR, "failed to make remote_channel %s/%s Compatible\n", argv[0], dest);
        ALL_DONE(u, -1);
    }
    /* While we haven't timed out and we still have no channel up */
    while (timeout  &&  (peer->_state != CW_STATE_UP))
    {
        active = cw_waitfor_n(channels, 2, &timeout);
        /* Timed out, so we are done trying */
        if (timeout == 0)
        {
            strncpy(status, "NOANSWER", sizeof(status) - 1);
            cw_log(LOG_NOTICE, "Timeout on peer\n");
            break;
        }
        if (active)
        {
            /* -1 means go forever */
            if (timeout > -1)
            {
                /* res holds the number of milliseconds remaining */
                if (timeout < 0)
                {
                    timeout = 0;
                    strncpy(status, "NOANSWER", sizeof(status) - 1);
                }
            }
            if (active == peer)
            {
                if ((f = cw_read(active)) == NULL)
                {
                    state = CW_CONTROL_HANGUP;
                    chan->hangupcause = peer->hangupcause;
                    res = 0;
                    break;
                }

                if (f->frametype == CW_FRAME_CONTROL)
                {
                    if (f->subclass == CW_CONTROL_PROCEEDING)
                    {
                        state = f->subclass;
                        cw_indicate(chan, CW_CONTROL_PROCEEDING);
                        cw_fr_free(f);
                        continue;
                    }
                    else if (f->subclass == CW_CONTROL_PROGRESS)
                    {
                        state = f->subclass;
                        cw_indicate(chan, CW_CONTROL_PROGRESS);
                        cw_fr_free(f);
                        continue;
                    }
                    else if (f->subclass == CW_CONTROL_RINGING)
                    {
                        state = f->subclass;
                        cw_indicate(chan, CW_CONTROL_RINGING);
                        cw_fr_free(f);
                        continue;
                    }
                    else if ((f->subclass == CW_CONTROL_BUSY)  ||  (f->subclass == CW_CONTROL_CONGESTION))
                    {
                        state = f->subclass;
                        chan->hangupcause = peer->hangupcause;
                        cw_fr_free(f);
                        break;
                    }
                    else if (f->subclass == CW_CONTROL_ANSWER)
                    {
                        /* This is what we are hoping for */
                        state = f->subclass;
                        cw_fr_free(f);
                        ready = 1;
                        break;
                    }
                    /* else who cares */
                }
                else if (f->frametype == CW_FRAME_VOICE)
                {
                    /* Pass on early media */
                    cw_write(chan, f);
                    cw_fr_free(f);
                    continue;
                }
            }
            else
            {
                /* orig channel reports something */
                if ((f = cw_read(active)) == NULL)
                {
                    state = CW_CONTROL_HANGUP;
                    cw_log(LOG_DEBUG, "Hangup from remote channel\n");
                    res = 0;
                    break;
                }
                if (f->frametype == CW_FRAME_CONTROL)
                {
                    if (f->subclass == CW_CONTROL_HANGUP)
                    {
                        state = f->subclass;
                        res = 0;
                        cw_fr_free(f);
                        break;
                    }
                }
            }
            cw_fr_free(f);
        }
    }

    res = 1;
    if (ready  &&  ready_to_talk(chan, peer))
    {
        cw_answer(chan);
        peer->appl = t38gateway_name;

        /* FIXME original patch removes the if line below - trying with it before removing it */
        if (argc > 2  &&  strchr(argv[2], 'r'))
            cw_indicate(chan, -1);

        cw_set_callerid(peer, chan->cid.cid_name, chan->cid.cid_num, chan->cid.cid_num);
        chan->hangupcause = CW_CAUSE_NORMAL_CLEARING;

        res = RUNNING;

        while (res == RUNNING)
        {
            if (res  &&  (chan->t38_status == peer->t38_status))
            {
                // Same on both sides, so just bridge 
                cw_log(LOG_DEBUG, "Bridging frames [ %d,%d]\n", chan->t38_status, peer->t38_status);
                res = cw_bridge_frames(chan, peer);
            }

            if ((res == RUNNING)
                &&
                ((chan->t38_status == T38_STATUS_UNKNOWN) || (peer->t38_status == T38_STATUS_UNKNOWN))
                &&
                (chan->t38_status != peer->t38_status))
            {
                // Different on each side, so gateway 
                cw_log(LOG_DEBUG, "Doing T.38 gateway [ %d,%d]\n", chan->t38_status, peer->t38_status);
                res = cw_t38_gateway(chan, peer, verbose);
            }
            cw_log(LOG_DEBUG," res = %d, RUNNING defined as %d, chan_Status [%d,%d] UNKNOWN set to %d ", res, RUNNING, chan->t38_status, peer->t38_status, T38_STATUS_UNKNOWN  );
        }
    }
    else
    {
        cw_log(LOG_DEBUG, "failed to get remote_channel %s/%s\n", argv[0], dest);
    }
    if (state == CW_CONTROL_ANSWER)
       strncpy(status, "ANSWER", sizeof(status) - 1);
    if (state == CW_CONTROL_BUSY)
        strncpy(status, "BUSY", sizeof(status) - 1);
    if (state == CW_CONTROL_CONGESTION)
        strncpy(status, "CONGESTION", sizeof(status) - 1);
    if (state == CW_CONTROL_HANGUP)
        strncpy(status, "CANCEL", sizeof(status) - 1);
    pbx_builtin_setvar_helper(chan, "DIALSTATUS", status);
    cw_log(LOG_NOTICE, "T38Gateway exit with %s\n", status);
    if (peer)
        cw_hangup(peer);

    /* Hangup if the call worked and you spec the h flag */
    ALL_DONE(u, (!res  &&  (argc > 2  &&  strchr(argv[2], 'h')))  ?  -1  :  0);
}

int unload_module(void)
{
    int res = 0;

    STANDARD_HANGUP_LOCALUSERS;
    res |= cw_unregister_application(t38gateway_app);
    return res;
}

int load_module(void)
{
    t38gateway_app = cw_register_application(t38gateway_name, t38gateway_exec, t38gateway_synopsis, t38gateway_syntax, t38gateway_descrip);
    return 0;
}

char *description(void)
{
    return tdesc;
}

int usecount(void)
{
    int res;

    STANDARD_USECOUNT(res);
    return res;
}
