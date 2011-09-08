/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Trivial application to send a TIFF file as a FAX
 * 
 * Copyright (C) 2003, 2005, Steve Underwood
 *
 * Steve Underwood <steveu@coppice.org>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
#include <spandsp.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_txfax.c $", "$Revision: 5391 $")

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/app.h"
#include "callweaver/dsp.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/translate.h"
#include "callweaver/manager.h"

static char *tdesc = "Trivial FAX Transmit Application";

static void *fax_app;
static const char *fax_name = "TxFAX";
static const char *fax_synopsis = "Send a file as a FAX";
static const char *fax_syntax = "TxFAX(filename[, caller][, debug])";
static const char *fax_descrip = 
"Send a given TIFF file to the channel as a FAX.\n"
"The \"caller\" option makes the application behave as a calling machine,\n"
"rather than the answering machine. The default behaviour is to behave as\n"
"an answering machine.\n"
"Uses LOCALSTATIONID to identify itself to the remote end.\n"
"     LOCALHEADERINFO to generate a header line on each page.\n"
"     FAX_DISABLE_V17\n"
"     FAX_DISABLE_ECM\n"
"Sets REMOTESTATIONID to the remote end's identity.\n"
"     FAXPAGES to the number of pages received.\n"
"     FAXBITRATE to the transmition rate.\n"
"     FAXRESOLUTION to the resolution.\n"
"     PHASEESTATUS to the phase E result status.\n"
"     PHASEESTRING to the phase E result string.\n"
"Returns -1 when the user hangs up, or if the file does not exist.\n"
"Returns 0 otherwise.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

#define MAX_BLOCK_SIZE 240
#define ready_to_talk(chan) ((!chan ||  cw_check_hangup(chan))  ?  0  :  1)

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
/*- End of function --------------------------------------------------------*/

/* Return a monotonically increasing time, in microseconds */
static uint64_t nowis(void)
{
    int64_t now;
#ifndef HAVE_POSIX_TIMERS
    struct timeval tv;

    gettimeofday(&tv, NULL);
    now = tv.tv_sec*1000000LL + tv.tv_usec;
#else
    struct timespec ts;
    
    if (clock_gettime(CLOCK_MONOTONIC, &ts))
        cw_log(LOG_WARNING, "clock_gettime returned %s\n", strerror(errno));
    now = ts.tv_sec*1000000LL + ts.tv_nsec/1000;
#endif
    return now;
}
/*- End of function --------------------------------------------------------*/

static void *faxgen_alloc(struct cw_channel *chan, void *params)
{
    cw_log(LOG_DEBUG, "Allocating fax generator\n");
    return params;
}
/*- End of function --------------------------------------------------------*/

static void faxgen_release(struct cw_channel *chan, void *data)
{
    cw_log(LOG_DEBUG, "Releasing fax generator\n");
    return;
}
/*- End of function --------------------------------------------------------*/

static int faxgen_generate(struct cw_channel *chan, void *data, int samples)
{
    int len;
    fax_state_t *fax;
    struct cw_frame outf;

    uint8_t __buf[sizeof(uint16_t)*MAX_BLOCK_SIZE + 2*CW_FRIENDLY_OFFSET];
    uint8_t *buf = __buf + CW_FRIENDLY_OFFSET;
    
    fax = (fax_state_t*) data;

    samples = (samples <= MAX_BLOCK_SIZE)  ?  samples  :  MAX_BLOCK_SIZE;
    if ((len = fax_tx(fax, (int16_t *) &buf[CW_FRIENDLY_OFFSET], samples)) > 0)
    {
        cw_fr_init_ex(&outf, CW_FRAME_VOICE, CW_FORMAT_SLINEAR, "FAX");
        outf.datalen = len*sizeof(int16_t);
        outf.samples = len;
        outf.data = &buf[CW_FRIENDLY_OFFSET];
        outf.offset = CW_FRIENDLY_OFFSET;

        if (cw_write(chan, &outf) < 0)
            cw_log(LOG_WARNING, "Unable to write frame to channel; %s\n", strerror(errno));
    }

    return 0;
}
/*- End of function --------------------------------------------------------*/

struct cw_generator faxgen = 
{
    alloc:      faxgen_alloc,
    release:    faxgen_release,
    generate:   faxgen_generate,
};

static int phase_d_handler(t30_state_t *s, void *user_data, int result)
{
    struct cw_channel *chan;
    t30_stats_t t;
    
    chan = (struct cw_channel *) user_data;
    if (result)
    {
        t30_get_transfer_statistics(s, &t);
        cw_log(LOG_DEBUG, "==============================================================================\n");
        cw_log(LOG_DEBUG, "Pages transferred:  %i\n", t.pages_tx);
        cw_log(LOG_DEBUG, "Image size:         %i x %i\n", t.width, t.length);
        cw_log(LOG_DEBUG, "Image resolution    %i x %i\n", t.x_resolution, t.y_resolution);
        cw_log(LOG_DEBUG, "Transfer Rate:      %i\n", t.bit_rate);
        cw_log(LOG_DEBUG, "Bad rows            %i\n", t.bad_rows);
        cw_log(LOG_DEBUG, "Longest bad row run %i\n", t.longest_bad_row_run);
        cw_log(LOG_DEBUG, "Compression type    %s\n", t4_encoding_to_str(t.encoding));
        cw_log(LOG_DEBUG, "Image size (bytes)  %i\n", t.image_size);
        cw_log(LOG_DEBUG, "==============================================================================\n");
    }
    return T30_ERR_OK;
}
/*- End of function --------------------------------------------------------*/

static void phase_e_handler(t30_state_t *s, void *user_data, int result)
{
    struct cw_channel *chan;
    char buf[128];
    t30_stats_t t;
    const char *tx_ident;
    const char *rx_ident;

    chan = (struct cw_channel *) user_data;
    t30_get_transfer_statistics(s, &t);
    
    tx_ident = t30_get_tx_ident(s);
    if (tx_ident == NULL)
        tx_ident = "";
    rx_ident = t30_get_rx_ident(s);
    if (rx_ident == NULL)
        rx_ident = "";
    pbx_builtin_setvar_helper(chan, "REMOTESTATIONID", rx_ident);
    snprintf(buf, sizeof(buf), "%d", t.pages_tx);
    pbx_builtin_setvar_helper(chan, "FAXPAGES", buf);
    snprintf(buf, sizeof(buf), "%d", t.y_resolution);
    pbx_builtin_setvar_helper(chan, "FAXRESOLUTION", buf);
    snprintf(buf, sizeof(buf), "%d", t.bit_rate);
    pbx_builtin_setvar_helper(chan, "FAXBITRATE", buf);
    snprintf(buf, sizeof(buf), "%d", result);
    pbx_builtin_setvar_helper(chan, "PHASEESTATUS", buf);
    snprintf(buf, sizeof(buf), "%s", t30_completion_code_to_str(result));
    pbx_builtin_setvar_helper(chan, "PHASEESTRING", buf);

    cw_log(LOG_DEBUG, "==============================================================================\n");
    if (result == T30_ERR_OK)
    {
        cw_log(LOG_DEBUG, "Fax successfully sent.\n");
        cw_log(LOG_DEBUG, "Remote station id: %s\n", rx_ident);
        cw_log(LOG_DEBUG, "Local station id:  %s\n", tx_ident);
        cw_log(LOG_DEBUG, "Pages transferred: %i\n", t.pages_tx);
        cw_log(LOG_DEBUG, "Image resolution:  %i x %i\n", t.x_resolution, t.y_resolution);
        cw_log(LOG_DEBUG, "Transfer Rate:     %i\n", t.bit_rate);
        manager_event(EVENT_FLAG_CALL,
                      "FaxSent", "Channel: %s\nExten: %s\nCallerID: %s\nRemoteStationID: %s\nLocalStationID: %s\nPagesTransferred: %i\nResolution: %i\nTransferRate: %i\nFileName: %s\n",
                      chan->name,
                      chan->exten,
                      (chan->cid.cid_num)  ?  chan->cid.cid_num  :  "",
                      rx_ident,
                      tx_ident,
                      t.pages_tx,
                      t.y_resolution,
                      t.bit_rate,
                      s->rx_file);
    }
    else
    {
        cw_log(LOG_DEBUG, "Fax send not successful - result (%d) %s.\n", result, t30_completion_code_to_str(result));
    }
    cw_log(LOG_DEBUG, "==============================================================================\n");
}
/*- End of function --------------------------------------------------------*/

static int t38_tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count)
{
    struct cw_frame outf;
    struct cw_channel *chan;

    chan = (struct cw_channel *) user_data;

    cw_fr_init_ex(&outf, CW_FRAME_MODEM, CW_MODEM_T38, "FAX");
    outf.datalen = len;
    outf.data = (char *) buf;
    outf.tx_copies = count;
    if (cw_write(chan, &outf) < 0)
        cw_log(LOG_WARNING, "Unable to write frame to channel; %s\n", strerror(errno));
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int fax_set_common(struct cw_channel *chan, t30_state_t *t30, const char *file, int calling_party, int verbose)
{
    char *x;

    x = pbx_builtin_getvar_helper(chan, "LOCALSTATIONID");
    if (x  &&  x[0])
        t30_set_tx_ident(t30, x);
    x = pbx_builtin_getvar_helper(chan, "LOCALSUBADDRESS");
    if (x  &&  x[0])
        t30_set_tx_sub_address(t30, x);
    x = pbx_builtin_getvar_helper(chan, "LOCALHEADERINFO");
    if (x  &&  x[0])
        t30_set_tx_page_header_info(t30, x);
    t30_set_tx_file(t30, file, -1, -1);

    x = pbx_builtin_getvar_helper(chan, "FAX_DISABLE_V17");
    if (x  &&  x[0])
        t30_set_supported_modems(t30, T30_SUPPORT_V29 | T30_SUPPORT_V27TER);

    x = pbx_builtin_getvar_helper(chan, "FAX_DISABLE_ECM");
    if (x  &&  x[0])
    {
        t30_set_ecm_capability(t30, FALSE);
        t30_set_supported_compressions(t30,
                                       T30_SUPPORT_T4_1D_COMPRESSION
                                     | T30_SUPPORT_T4_2D_COMPRESSION);
        cw_log(LOG_DEBUG, "Disabling ECM mode\n");
    }
    else 
    {
        t30_set_ecm_capability(t30, TRUE);
        t30_set_supported_compressions(t30,
                                       T30_SUPPORT_T4_1D_COMPRESSION
                                     | T30_SUPPORT_T4_2D_COMPRESSION
                                     | T30_SUPPORT_T6_COMPRESSION);
    }

    t30_set_supported_image_sizes(t30,
                                  T30_SUPPORT_US_LETTER_LENGTH
                                | T30_SUPPORT_US_LEGAL_LENGTH
                                | T30_SUPPORT_UNLIMITED_LENGTH
                                | T30_SUPPORT_215MM_WIDTH
                                | T30_SUPPORT_255MM_WIDTH
                                | T30_SUPPORT_303MM_WIDTH);
    t30_set_supported_resolutions(t30,
                                  T30_SUPPORT_STANDARD_RESOLUTION
                                | T30_SUPPORT_FINE_RESOLUTION
                                | T30_SUPPORT_SUPERFINE_RESOLUTION
                                | T30_SUPPORT_R8_RESOLUTION
                                | T30_SUPPORT_R16_RESOLUTION);

    //t30_set_phase_b_handler(t30, phase_b_handler, chan);
    //t30_set_phase_d_handler(t30, phase_d_handler, chan);
    t30_set_phase_e_handler(t30, phase_e_handler, chan);
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int fax_t38(struct cw_channel *chan, t38_terminal_state_t *t38, const char *file, int calling_party, int verbose)
{
    char *x;
    struct cw_frame *inf = NULL;
    int ready = 1;
    int res = 0;
    uint64_t now;
    uint64_t passage;
    t30_state_t *t30;
    t38_core_state_t *t38_core;

    memset(t38, 0, sizeof(*t38));

    if (t38_terminal_init(t38, calling_party, t38_tx_packet_handler, chan) == NULL)
    {
        cw_log(LOG_WARNING, "Unable to start T.38 termination.\n");
        return -1;
    }
    t30 = t38_terminal_get_t30_state(t38);
    t38_core = t38_terminal_get_t38_core_state(t38);
    span_log_set_message_handler(&t38->logging, span_message);
    span_log_set_message_handler(&t30->logging, span_message);
    span_log_set_message_handler(&t38_core->logging, span_message);

    if (verbose)
    {
        span_log_set_level(&t38->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
        span_log_set_level(&t30->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
        span_log_set_level(&t38_core->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
    }

    fax_set_common(chan, t30, file, calling_party, verbose);

    passage = nowis();

    t38_terminal_set_tep_mode(t38, TRUE);
    while (ready  &&  ready_to_talk(chan))
    {
        if (chan->t38_status != T38_NEGOTIATED)
            break;

        if ((res = cw_waitfor(chan, 20)) < 0)
        {
            ready = 0;
            break;
        }

        now = nowis();
        t38_terminal_send_timeout(t38, (now - passage)/125);
        passage = now;
        /* End application when T38/T30 has finished */
        if (!t30_call_active(t30)) 
            break;

        if ((inf = cw_read(chan)) == NULL)
        {
            ready = 0;
            break;
        }

        if (inf->frametype == CW_FRAME_MODEM  &&  inf->subclass == CW_MODEM_T38)
            t38_core_rx_ifp_packet(t38_core, inf->data, inf->datalen, inf->seq_no);

        cw_fr_free(inf);
    }

    return ready;
}
/*- End of function --------------------------------------------------------*/

static int fax_audio(struct cw_channel *chan, fax_state_t *fax, const char *file, int calling_party, int verbose)
{
    char *x;
    struct cw_frame *inf = NULL;
    struct cw_frame outf;
    int ready = 1;
    int samples = 0;
    int res = 0;
    int len = 0;
    int generator_mode = 0;
    uint64_t begin = 0;
    uint64_t received_frames = 0;
    uint8_t __buf[sizeof(uint16_t)*MAX_BLOCK_SIZE + 2*CW_FRIENDLY_OFFSET];
    uint8_t *buf = __buf + CW_FRIENDLY_OFFSET;
#if 0
    struct cw_frame *dspf = NULL;
    struct cw_dsp *dsp = NULL;
#endif
    uint64_t voice_frames;
    t30_state_t *t30;

    memset(fax, 0, sizeof(*fax));
    if (fax_init(fax, calling_party) == NULL)
    {
        cw_log(LOG_WARNING, "Unable to start FAX\n");
        return -1;
    }
    t30 = fax_get_t30_state(fax);
    fax_set_transmit_on_idle(fax, TRUE);
    span_log_set_message_handler(&fax->logging, span_message);
    span_log_set_message_handler(&t30->logging, span_message);
    if (verbose)
    {
        span_log_set_level(&fax->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
        span_log_set_level(&t30->logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
    }

    fax_set_common(chan, t30, file, calling_party, verbose);
    fax_set_transmit_on_idle(fax, TRUE);

    if (calling_party)
    {
        voice_frames = 0;
    }
    else
    {
#if 0
        /* Initializing the DSP */
        if ((dsp = cw_dsp_new()) == NULL)
        {
            cw_log(LOG_WARNING, "Unable to allocate DSP!\n");
        }
        else
        {
            cw_dsp_set_threshold(dsp, 256); 
            cw_dsp_set_features(dsp, DSP_FEATURE_DTMF_DETECT | DSP_FEATURE_FAX_CNG_DETECT);
            cw_dsp_digitmode(dsp, DSP_DIGITMODE_DTMF | DSP_DIGITMODE_RELAXDTMF);
        }
#endif
        voice_frames = 1;
    }

    /* This is the main loop */
    begin = nowis();
    while (ready  &&  ready_to_talk(chan))
    {
        if (chan->t38_status == T38_NEGOTIATED)
            break;

        if ((res = cw_waitfor(chan, 20)) < 0)
        {
            ready = 0;
            break;
        }

        if (!t30_call_active(t30))
            break;

        if ((inf = cw_read(chan)) == NULL)
        {
            ready = 0;
            break;
        }

        /* We got a frame */
        if (inf->frametype == CW_FRAME_VOICE)
        {
#if 0
            if (dsp)
            {
                if ((dspf = cw_frdup(inf)))
                    dspf = cw_dsp_process(chan, dsp, dspf);

                if (dspf)
                {
                    if (dspf->frametype == CW_FRAME_DTMF)
                    {
                        if (dspf->subclass == 'f')
                        {
                            cw_log(LOG_DEBUG, "Fax detected in RxFax !!!\n");
                            cw_app_request_t38(chan);
                            /* Prevent any further attempts to negotiate T.38 */
                            cw_dsp_free(dsp);
                            dsp = NULL;
                    
                        }
                    }
                    cw_fr_free(dspf);
                    dspf = NULL;
                }
            }
#else
            if (voice_frames)
            {
                /* Wait a little before trying to switch to T.38, as some things don't seem
                   to like entirely missing the audio. */
                if (++voice_frames == 100)
                {
                    cw_log(LOG_DEBUG, "Requesting T.38 negotation in RxFax !!!\n");
                    cw_app_request_t38(chan);
                    voice_frames = 0;
                }
            }
#endif
            received_frames++;

            if (fax_rx(fax, inf->data, inf->samples))
                break;

            samples = (inf->samples <= MAX_BLOCK_SIZE)  ?  inf->samples  :  MAX_BLOCK_SIZE;
            if ((len = fax_tx(fax, (int16_t *) &buf[CW_FRIENDLY_OFFSET], samples)) > 0)
            {
                cw_fr_init_ex(&outf, CW_FRAME_VOICE, CW_FORMAT_SLINEAR, "FAX");
                outf.datalen = len*sizeof(int16_t);
                outf.samples = len;
                outf.data = &buf[CW_FRIENDLY_OFFSET];
                outf.offset = CW_FRIENDLY_OFFSET;

                if (cw_write(chan, &outf) < 0)
                {
                    cw_log(LOG_WARNING, "Unable to write frame to channel; %s\n", strerror(errno));
                    break;
                }
            }
        }
        else
        {
            if ((nowis() - begin) > 1000000)
            {
                if (received_frames < 20)
                {
                    /* Just to be sure we have had no frames ... */
                    cw_log(LOG_NOTICE, "Switching to generator mode\n");
                    generator_mode = 1;
                    break;
                }
            }
        }
        cw_fr_free(inf);
        inf = NULL;
    }

    if (inf)
    {
        cw_fr_free(inf);
        inf = NULL;
    }
    if (generator_mode)
    {
        /* This is activated when we don't receive any frame for X seconds (see above)... */
#if 0
        if (dsp)
            cw_dsp_reset(dsp);
#endif
        cw_generator_activate(chan, &faxgen, fax);

        while (ready  &&  ready_to_talk(chan))
        {
            if (chan->t38_status == T38_NEGOTIATED)
                break;

            if ((res = cw_waitfor(chan, 20)) < 0)
            {
                ready = 0;
                break;
            }

            if (!t30_call_active(t30))
                break;

            if ((inf = cw_read(chan)) == NULL)
            {
                ready = 0;
                break;
            }

            /* We got a frame */
            if (inf->frametype == CW_FRAME_VOICE)
            {
#if 0
                if (dsp)
                {
                    if ((dspf = cw_frdup(inf)))
                        dspf = cw_dsp_process(chan, dsp, dspf);

                    if (dspf)
                    {
                        if (dspf->frametype == CW_FRAME_DTMF)
                        {
                            if (dspf->subclass == 'f')
                            {
                                cw_log(LOG_DEBUG, "Fax detected in RxFax !!!\n");
                                cw_app_request_t38(chan);
                                /* Prevent any further attempts to negotiate T.38 */
                                cw_dsp_free(dsp);
                                dsp = NULL;
                            }
                        }
                        cw_fr_free(dspf);
                        dspf = NULL;
                    }
                }
#else
                if (voice_frames)
                {
                    if (++voice_frames == 100)
                    {
                        cw_log(LOG_DEBUG, "Requesting T.38 negotation in RxFax !!!\n");
                        cw_app_request_t38(chan);
                        voice_frames = 0;
                    }
                }
#endif
                if (fax_rx(fax, inf->data, inf->samples))
                {
                    ready = 0;
                    break;
                }
            }

            cw_fr_free(inf);
            inf = NULL;
        }

        if (inf)
        {
            cw_fr_free(inf);
            inf = NULL;
        }
        cw_generator_deactivate(chan);
    }
#if 0
    if (dsp)
        cw_dsp_free(dsp);
#endif
    return ready;
}
/*- End of function --------------------------------------------------------*/

static int fax_exec(struct cw_channel *chan, int argc, char **argv)
{
    fax_state_t fax;
    t38_terminal_state_t t38;
    t30_state_t *t30;

    const char *file_name;
    int res = 0;
    int ready;

    int calling_party;
    int verbose;
    
    struct localuser *u;

    int original_read_fmt;
    int original_write_fmt;

    /* Basic initial checkings */

    if (chan == NULL)
    {
        cw_log(LOG_WARNING, "Fax transmit channel is NULL. Giving up.\n");
        return -1;
    }

    /* Make sure they are initialized to zero */
    memset(&fax, 0, sizeof(fax));
    memset(&t38, 0, sizeof(t38));

    if (argc < 1)
    {
        cw_log(LOG_ERROR, "Syntax: %s\n", fax_syntax);
        return -1;
    }

    /* Resetting channel variables related to T38 */
    
    pbx_builtin_setvar_helper(chan, "REMOTESTATIONID", "");
    pbx_builtin_setvar_helper(chan, "FAXPAGES", "");
    pbx_builtin_setvar_helper(chan, "FAXRESOLUTION", "");
    pbx_builtin_setvar_helper(chan, "FAXBITRATE", "");
    pbx_builtin_setvar_helper(chan, "PHASEESTATUS", "");
    pbx_builtin_setvar_helper(chan, "PHASEESTRING", "");

    /* Parsing parameters */
    
    calling_party = FALSE;
    verbose = FALSE;

    file_name = argv[0];

    while (argv++, --argc)
    {
        if (strcmp(argv[0], "caller") == 0)
        {
            calling_party = TRUE;
        }
        else if (strcmp(argv[0], "debug") == 0)
        {
            verbose = TRUE;
        }
        else if (strcmp(argv[0], "start") == 0)
        {
            /* TODO: handle this */
        }
        else if (strcmp(argv[0], "end") == 0)
        {
            /* TODO: handle this */
        }
    }

    /* Done parsing */

    LOCAL_USER_ADD(u);

    if (chan->_state != CW_STATE_UP)
    {
        /* Shouldn't need this, but checking to see if channel is already answered
         * Theoretically the PBX should already have answered before running the app */
        res = cw_answer(chan);
        if (!res)
        {
            cw_log(LOG_DEBUG, "Could not answer channel '%s'\n", chan->name);
            //LOCAL_USER_REMOVE(u);
            //return res;
        }
    }

    /* Setting read and write formats */
    
    original_read_fmt = chan->readformat;
    if (original_read_fmt != CW_FORMAT_SLINEAR)
    {
        res = cw_set_read_format(chan, CW_FORMAT_SLINEAR);
        if (res < 0)
        {
            cw_log(LOG_WARNING, "Unable to set to linear read mode, giving up\n");
            LOCAL_USER_REMOVE(u);
            return -1;
        }
    }

    original_write_fmt = chan->writeformat;
    if (original_write_fmt != CW_FORMAT_SLINEAR)
    {
        res = cw_set_write_format(chan, CW_FORMAT_SLINEAR);
        if (res < 0)
        {
            cw_log(LOG_WARNING, "Unable to set to linear write mode, giving up\n");
            res = cw_set_read_format(chan, original_read_fmt);
            if (res)
                cw_log(LOG_WARNING, "Unable to restore read format on '%s'\n", chan->name);
            LOCAL_USER_REMOVE(u);
            return -1;
        }
    }

    /* This is the main loop */
    ready = TRUE;        
    if (chan->t38_status == T38_NEGOTIATED)
        t30 = t38_terminal_get_t30_state(&t38);
    else
        t30 = fax_get_t30_state(&fax);
    while (ready  &&  ready_to_talk(chan))
    {
        if (ready  &&  chan->t38_status != T38_NEGOTIATED)
        {
            t30 = fax_get_t30_state(&fax);
            ready = fax_audio(chan, &fax, file_name, calling_party, verbose);
        }
        if (ready  &&  chan->t38_status == T38_NEGOTIATED)
        {
            t30 = t38_terminal_get_t30_state(&t38);
            ready = fax_t38(chan, &t38, file_name, calling_party, verbose);
        }
        if (chan->t38_status != T38_NEGOTIATING)
            ready = FALSE; // 1 loop is enough. This could be useful if we want to turn from udptl to RTP later.
    }

    t30_terminate(t30);

    fax_release(&fax);
    t38_terminal_release(&t38);

    /* Restoring initial channel formats. */
    if (original_read_fmt != CW_FORMAT_SLINEAR)
    {
        if ((res = cw_set_read_format(chan, original_read_fmt)))
            cw_log(LOG_WARNING, "Unable to restore read format on '%s'\n", chan->name);
    }
    if (original_write_fmt != CW_FORMAT_SLINEAR)
    {
        if ((res = cw_set_write_format(chan, original_write_fmt)))
            cw_log(LOG_WARNING, "Unable to restore write format on '%s'\n", chan->name);
    }

    return ready;
}
/*- End of function --------------------------------------------------------*/

int unload_module(void)
{
    int res = 0;

    STANDARD_HANGUP_LOCALUSERS;
    res |= cw_unregister_application(fax_app);
    return res;
}
/*- End of function --------------------------------------------------------*/

int load_module(void)
{
    fax_app = cw_register_application(fax_name, fax_exec, fax_synopsis, fax_syntax, fax_descrip);
    return 0;
}
/*- End of function --------------------------------------------------------*/

char *description(void)
{
    return tdesc;
}
/*- End of function --------------------------------------------------------*/

int usecount(void)
{
    int res;

    STANDARD_USECOUNT(res);
    return res;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
