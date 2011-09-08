/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Fax detection application for all channel types. 
 * Copyright (C) 2004-2005, Newman Telecom, Inc. and Newman Ventures, Inc.
 *
 * Justin Newman <jnewman@newmantelecom.com>
 *
 * We would like to thank Newman Ventures, Inc. for funding this
 * project.
 * Newman Ventures <info@newmanventures.com>
 *
 * Modified and ported to callweaver.org by
 * Massimo CtRiX Cetra <devel@navynet.it>
 * Thanks to Navynet SRL for funding this project
 *
 * Portions Copyright:
 * Copyright (C) 2001, Linux Support Services, Inc.
 * Copyright (C) 2004, Digium, Inc.
 *
 * Matthew Fredrickson <creslin@linux-support.net>
 * Mark Spencer <markster@digium.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "callweaver.h"

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/translate.h"
#include "callweaver/dsp.h"
#include "callweaver/indications.h"
#include "callweaver/utils.h"

static char *tdesc = "Fax detection application";

static void *detectfax_app;
static char *detectfax_name = "FaxDetect";
static char *detectfax_synopsis = "Detects fax sounds on all channel types (IAX and SIP too)";
static char *detectfax_syntax = "FaxDetect([waitdur[, tonestr[, options[, sildur[, mindur[, maxdur]]]]]])";
static char *detectfax_descrip = 
"Parameters:\n"
"      waitdur:  Maximum number of seconds to wait (default=4)\n"
"      tonestr:  Indication to be used while detecting (example: ring)\n"
"      options:\n"
"        'n':     Do not auto-answer if the call is not already answered\n"
"        'x':     Reception of DTMF digits terminates FaxDetect even if the\n"
"                 digits do not match an extension in the current context)\n"
"        'd':     Ignore any DTMF digits received\n"
"        'D':     Wait for a '#' signalling the end of a DTMF extension number\n"
"                 (the '#' is not included in the number) rather than exiting\n"
"                 as soon as the collected DTMF matches an extension in the\n"
"                 current context\n"
"        'f':     Disable fax detection\n"
"        't':     Disable talk detection\n"
"        'j':     Set variables (see below) and exit rather than jumping to the\n"
"                 'fax', 'talk' or DTMF extension\n"
"        sildur:  Silence ms after mindur/maxdur before aborting (default=1000)\n"
"        mindur:  Minimum non-silence ms needed (default=100)\n"
"        maxdur:  Maximum non-silence ms allowed (default=0/forever)\n"
"\n"
"This application listens for fax tones for waitdur seconds of time.\n"
"Audio is only monitored in the receive direction.\n"
"It can play optional ringtone indicated by tonestr. \n"
"It can be interrupted by digits or non-silence (talk or fax tones).\n"
"if d option is specified, a single digit interrupts and must be the \n"
"start of a valid extension.\n"
"If D option is specified (overrides d), the application will wait for\n"
"the user to enter digits terminated by a # and jumps to the corresponding\n"
"extension, if it exists.\n"
"If fax is detected (tones or T38 invite), it will jump to the 'fax' extension.\n"
"If a period of non-silence greater than 'mindur' ms, yet less than 'maxdur' ms\n"
"is followed by silence at least 'sildur' ms then the application jumps\n"
"to the 'talk' extension.\n"
"If all undetected, control will continue at the next priority.\n"
"\n"
"The application, upon exit, will set the folloging channel variables: \n"
"   - DTMF_DETECTED : set to 1 when a DTMF digit would have caused a jump.\n"
"   - TALK_DETECTED : set to 1 when talk has been detected.\n"
"   - FAX_DETECTED  : set to 1 when fax tones detected.\n"
"   - FAXEXTEN      : original dialplan extension of this application\n"
"   - DTMF_DID      : digits dialled before exit (excluding any terminating '#')\n"
"\n"
"Returns -1 on hangup, and 0 on successful completion with no exit conditions.\n";

#define CALLERID_FIELD cid.cid_num

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int detectfax_exec(struct cw_channel *chan, int argc, char **argv)
{
    int res = 0;
    struct localuser *u;
    char dtmf_did[256] = "\0";
    char *tonestr = NULL;
    int totalsilence;            // working vars
    int ms_silence = 0;
    int ms_talk = 0;        // working vars
    struct cw_frame *fr = NULL;
    struct cw_frame *fr2 = NULL;
    struct cw_frame *fr3 = NULL;
    int speaking = 0;
    int talkdetection_started = 0;
    struct timeval start_talk = {0, 0};
    struct timeval start_silence = {0, 0};
    struct timeval now = {0, 0};
    int waitdur;
    int sildur;
    int mindur;
    int maxdur;
    int skipanswer = 0;
    int noextneeded = 0;
    int ignoredtmf = 0;
    int ignorefax = 0;
    int ignoretalk = 0;
    int ignorejump = 0;
    int longdtmf = 0;
    int origrformat = 0;
    int origwformat = 0;
    int features = 0;
    time_t timeout = 0;
    struct cw_dsp *dsp = NULL;
    
    pbx_builtin_setvar_helper(chan, "FAX_DETECTED", "0");
    pbx_builtin_setvar_helper(chan, "FAXEXTEN", "unknown");
    pbx_builtin_setvar_helper(chan, "DTMF_DETECTED", "0");
    pbx_builtin_setvar_helper(chan, "TALK_DETECTED", "0");
        pbx_builtin_setvar_helper(chan, "DTMF_DID", "");

	if (argc > 6) {
		cw_log(LOG_ERROR, "Syntax: %s\n", detectfax_syntax);
		return -1;
	}

	waitdur = (argc > 0 ? atoi(argv[0]) : 4);
	if (waitdur <= 0) waitdur = 4;

	tonestr = (argc > 1 && argv[1][0] ? argv[1] : NULL);

	if (argc > 2) {
		for (; argv[2][0]; argv[2]++) {
			switch (argv[2][0]) {
				case 'n': skipanswer = 1; break;
				case 'x': noextneeded = 1; break;
				case 'd': ignoredtmf = 1; break;
				case 'D': ignoredtmf = 0; longdtmf = 1; break;
				case 'f': ignorefax = 1; break;
				case 't': ignoretalk = 1; break;
				case 'j': ignorejump = 1; break;
			}
		}
	}

	sildur = (argc > 3 ? atoi(argv[3]) : 1000);
	if (sildur <= 0) sildur = 1000;

	mindur = (argc > 4 ? atoi(argv[4]) : 100);
	if (mindur <= 0) mindur = 100;

	maxdur = (argc > 5 ? atoi(argv[5]) : -1);
	if (maxdur <= 0) maxdur = -1;

    cw_log(LOG_DEBUG, "Preparing detect of fax (waitdur=%dms, sildur=%dms, mindur=%dms, maxdur=%dms)\n", 
                        waitdur, sildur, mindur, maxdur);
                        
    LOCAL_USER_ADD(u);
    if (chan->_state != CW_STATE_UP  &&  !skipanswer)
    {
        /* Otherwise answer unless we're supposed to send this while on-hook */
        res = cw_answer(chan);
    }
    if (!res)
    {
        origrformat = chan->readformat;
        if ((res = cw_set_read_format(chan, CW_FORMAT_SLINEAR))) 
            cw_log(LOG_WARNING, "Unable to set read format to linear!\n");
        origwformat = chan->writeformat;
        if ((res = cw_set_write_format(chan, CW_FORMAT_SLINEAR))) 
            cw_log(LOG_WARNING, "Unable to set write format to linear!\n");
    }
    if (!(dsp = cw_dsp_new()))
    {
        cw_log(LOG_WARNING, "Unable to allocate DSP!\n");
        res = -1;
    }
    
    if (dsp)
    {    
        if (!ignoretalk)
            ; /* features |= DSP_FEATURE_SILENCE_SUPPRESS; */
        if (!ignorefax)
            features |= DSP_FEATURE_FAX_CNG_DETECT;

        features |= DSP_FEATURE_DTMF_DETECT;
            
        cw_dsp_set_threshold(dsp, 256); 
        cw_dsp_set_features(dsp, features | DSP_DIGITMODE_RELAXDTMF);
        cw_dsp_digitmode(dsp, DSP_DIGITMODE_DTMF);
    }

    if (tonestr)
    {
        struct tone_zone_sound *ts;

        ts = cw_get_indication_tone(chan->zone, tonestr);
        if (ts && ts->data[0])
            res = cw_playtones_start(chan, 0, ts->data, 0);
        else
            res = cw_playtones_start(chan, 0, tonestr, 0);
        if (res)
            cw_log(LOG_NOTICE,"Unable to start playtones\n");
    }

    if (!res)
    {
        if (waitdur > 0)
            timeout = time(NULL) + (time_t) waitdur;

        while (cw_waitfor(chan, -1) > -1)
        {
            if (waitdur > 0 && time(NULL) > timeout)
            {
                res = 0;
                break;
            }

            fr = cw_read(chan);
            if (!fr) {
                cw_log(LOG_DEBUG, "Got hangup\n");
                res = -1;
                break;
            }

            /* Check for a T38 switchover */
            if (chan->t38_status == T38_NEGOTIATED  &&  !ignorefax)
            {
                cw_log(LOG_DEBUG, "Fax detected on %s. T38 switchover completed.\n", chan->name);
                pbx_builtin_setvar_helper(chan, "FAX_DETECTED", "1");
                pbx_builtin_setvar_helper(chan,"FAXEXTEN",chan->exten);
                if (!ignorejump)
                {
                    if (strcmp(chan->exten, "fax"))
                    {
                        cw_log(LOG_NOTICE, "Redirecting %s to fax extension [T38]\n", chan->name);
                        if (cw_exists_extension(chan, chan->context, "fax", 1, chan->CALLERID_FIELD))
                        {
                            /* Save the DID/DNIS when we transfer the fax call to a "fax" extension */
                            strncpy(chan->exten, "fax", sizeof(chan->exten)-1);
                            chan->priority = 0;                                    
                        }
                        else
                            cw_log(LOG_WARNING, "Fax detected, but no fax extension\n");
                    }
                    else
                        cw_log(LOG_WARNING, "Already in a fax extension, not redirecting\n");
                }
                res = 0;
                cw_fr_free(fr);
                break;
            }


            fr2 = cw_dsp_process(chan, dsp, fr);
            if (!fr2)
            {
                cw_log(LOG_WARNING, "Bad DSP received (what happened?)\n");
                fr2 = fr;
            } 

            if (fr2->frametype == CW_FRAME_DTMF)
            {
                if (fr2->subclass == 'f'  &&  !ignorefax)
                {
                    // Fax tone -- Handle and return NULL 
                    cw_log(LOG_DEBUG, "Fax detected on %s\n", chan->name);
                    pbx_builtin_setvar_helper(chan, "FAX_DETECTED", "1");
                    pbx_builtin_setvar_helper(chan,"FAXEXTEN",chan->exten);
                    if (!ignorejump)
                    {
                        if (strcmp(chan->exten, "fax"))
                        {
                            cw_log(LOG_NOTICE, "Redirecting %s to fax extension [DTMF]\n", chan->name);
                            if (cw_exists_extension(chan, chan->context, "fax", 1, chan->CALLERID_FIELD))
                            {
                                // Save the DID/DNIS when we transfer the fax call to a "fax" extension
                                strncpy(chan->exten, "fax", sizeof(chan->exten)-1);
                                chan->priority = 0;
                            }
                            else
                                cw_log(LOG_WARNING, "Fax detected, but no fax extension\n");
                        }
                        else
                            cw_log(LOG_WARNING, "Already in a fax extension, not redirecting\n");
                    }
                    res = 0;
                    cw_fr_free(fr);
                    break;
                }
                else if (!ignoredtmf)
                {
                    char t[2];

                    t[0] = fr2->subclass;
                    t[1] = '\0';
                    cw_log(LOG_DEBUG, "DTMF detected on %s: %s\n", chan->name,t);
                    if ((noextneeded || cw_canmatch_extension(chan, chan->context, t, 1, chan->CALLERID_FIELD))
                        && !longdtmf)
                    {
                        // They entered a valid extension, or might be anyhow 
                        pbx_builtin_setvar_helper(chan, "DTMF_DETECTED", "1");
                        if (noextneeded)
                        {
                            cw_log(LOG_NOTICE, "DTMF received (not matching to exten)\n");
                            res = 0;
                        }
                        else
                        {
                            cw_log(LOG_NOTICE, "DTMF received (matching to exten)\n");
                            res = fr2->subclass;
                        }
                        cw_fr_free(fr);
                        break;
                    }
                    else
                    {
                        if (strcmp(t, "#")  ||  !longdtmf)
                        {
                            strncat(dtmf_did, t, sizeof(dtmf_did) - strlen(dtmf_did) - 1);
                        }
                        else
                        {
                            pbx_builtin_setvar_helper(chan, "DTMF_DETECTED", "1");
                            pbx_builtin_setvar_helper(chan, "DTMF_DID", dtmf_did);
                            if (!ignorejump && cw_canmatch_extension(chan, chan->context, dtmf_did, 1, chan->CALLERID_FIELD))
                            {
                                strncpy(chan->exten, dtmf_did, sizeof(chan->exten) - 1);
                                chan->priority = 0;
                            }
                            res = 0;
                            cw_fr_free(fr);
                            break;
                        }
                        cw_log(LOG_DEBUG, "Valid extension requested and DTMF did not match [%s]\n",t);
                    }
                }
            }
            else if ((fr->frametype == CW_FRAME_VOICE) && (fr->subclass == CW_FORMAT_SLINEAR)  &&  !ignoretalk)
            {

                // Let's do echo
                if (!tonestr)
                {
                    //The following piece of code enables this application
                    //to send empty frames. This solves fax detection problem
                    //When a fax gets in with RTP. 
                    //The CNG is detected, faxdetect gotos to fax extension
                    //and if we are on a SIP channel the T38 switchover is done.
                    if ((fr3 = cw_frdup(fr)))
                    {
                        memset(fr3->data, 0, fr3->datalen);
                        cw_write(chan, fr3);
                        cw_fr_free(fr3);
                    }
                }


                res = cw_dsp_silence(dsp, fr, &totalsilence);
                if (res)
                {
                    // There is silence on the line.
                    gettimeofday(&now, NULL);
                    if (talkdetection_started && speaking)
                    {
                        // 1st iteration
                        ms_talk =  (now.tv_sec  - start_talk.tv_sec ) * 1000;
                        ms_talk += (now.tv_usec - start_talk.tv_usec) / 1000;
                    }
                    if (speaking)
                        gettimeofday(&start_silence, NULL);

                    ms_silence =  (now.tv_sec  - start_silence.tv_sec ) * 1000;
                    ms_silence += (now.tv_usec - start_silence.tv_usec) / 1000;

                    //cw_log(LOG_WARNING,"[%5d,%5d,%5d] MS_TALK: %6d MS_SILENCE %6d\n", 
                    //    mindur,maxdur,sildur, ms_talk, ms_silence);
                    speaking=0;

                    if (ms_silence >= sildur)
                    {
                        if ((ms_talk >= mindur)  &&  ((maxdur < 0)  ||  (ms_talk < maxdur)))
                        {
                            // TALK Has been detected
                            char ms_str[64];

                            snprintf(ms_str, sizeof(ms_str), "%d", ms_talk);
                            pbx_builtin_setvar_helper(chan, "TALK_DETECTED", ms_str);
                            if (!ignorejump)
                            {
                                cw_log(LOG_NOTICE, "Redirecting %s to talk extension\n", chan->name);
                                if (cw_exists_extension(chan, chan->context, "talk", 1, chan->CALLERID_FIELD))
                                {
                                    strncpy(chan->exten, "talk", sizeof(chan->exten) - 1);
                                    chan->priority = 0;
                                }
                                else
                                    cw_log(LOG_WARNING, "Talk detected, but no talk extension\n");
                            }
                            else
                            {
                                cw_log(LOG_NOTICE, "Talk detected.\n");
                            }
                            res = 0;
                            cw_fr_free(fr);
                            break;
                        }
                    }
                }
                else
                {
                    if (!talkdetection_started)
                        talkdetection_started = 1;
                    if (!speaking)
                    {
                        gettimeofday(&start_talk, NULL);
                        cw_log(LOG_DEBUG,"Start of voice token\n");
                    }
                    speaking = 1;
                }
            }
            cw_fr_free(fr);
        }
    }
    else
        cw_log(LOG_WARNING, "Could not answer channel '%s'\n", chan->name);
    
    if (res > -1)
    {
        if (origrformat && cw_set_read_format(chan, origrformat))
        {
            cw_log(LOG_WARNING, "Failed to restore read format for %s to %s\n", 
                     chan->name, cw_getformatname(origrformat));
        }
        if (origwformat && cw_set_write_format(chan, origwformat))
        {
            cw_log(LOG_WARNING, "Failed to restore write format for %s to %s\n", 
                     chan->name, cw_getformatname(origwformat));
        }
    }
    
    if (dsp)
        cw_dsp_free(dsp);

    if (tonestr)
        cw_playtones_stop(chan);
    
    LOCAL_USER_REMOVE(u);
    
    return res;
}

int unload_module(void)
{
    int res = 0;
    STANDARD_HANGUP_LOCALUSERS;
    res |= cw_unregister_application(detectfax_app);
    return res;
}

int load_module(void)
{
    detectfax_app = cw_register_application(detectfax_name, detectfax_exec, detectfax_synopsis, detectfax_syntax, detectfax_descrip);
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

