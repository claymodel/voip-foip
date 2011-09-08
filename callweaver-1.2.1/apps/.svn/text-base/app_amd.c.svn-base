/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2003 - 2006, Aheeva Technology.
 *
 * Claude Klimos (claude.klimos@aheeva.com)
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
 *
 */

/*! \file
 *
 * \brief Answering machine detection
 *
 * \author Claude Klimos (claude.klimos@aheeva.com)
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/branches/rel/1.2/apps/app_amd.c $", "$Revision: $")

#include "callweaver/module.h"
#include "callweaver/lock.h"
#include "callweaver/channel.h"
#include "callweaver/dsp.h"
#include "callweaver/pbx.h"
#include "callweaver/config.h"
#include "callweaver/app.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"

static char *tdesc = "Answering Machine Detection";

static void *app;
static char *name = "AMD";
static char *synopsis = "Attempts to detect answering machines";
static char *syntax = "AMD([initialSilence],[greeting],[afterGreetingSilence],[totalAnalysisTime]\n"
"      ,[minimumWordLength],[betweenWordsSilence],[maximumNumberOfWords]\n"
"      ,[silenceThreshold],[|maximumWordLength])\n";
static char *descrip =
"  AMD([initialSilence],[greeting],[afterGreetingSilence],[totalAnalysisTime]\n"
"      ,[minimumWordLength],[betweenWordsSilence],[maximumNumberOfWords]\n"
"      ,[silenceThreshold],[|maximumWordLength])\n"
"  This application attempts to detect answering machines at the beginning\n"
"  of outbound calls.  Simply call this application after the call\n"
"  has been answered (outbound only, of course).\n"
"  When loaded, AMD reads amd.conf and uses the parameters specified as\n"
"  default values. Those default values get overwritten when calling AMD\n"
"  with parameters.\n"
"- 'initialSilence' is the maximum silence duration before the greeting. If\n"
"   exceeded then MACHINE.\n"
"- 'greeting' is the maximum length of a greeting. If exceeded then MACHINE.\n"
"- 'afterGreetingSilence' is the silence after detecting a greeting.\n"
"   If exceeded then HUMAN.\n"
"- 'totalAnalysisTime' is the maximum time allowed for the algorithm to decide\n"
"   on a HUMAN or MACHINE.\n"
"- 'minimumWordLength'is the minimum duration of Voice to considered as a word.\n"
"- 'betweenWordsSilence' is the minimum duration of silence after a word to \n"
"   consider the audio that follows as a new word.\n"
"- 'maximumNumberOfWords'is the maximum number of words in the greeting. \n"
"   If exceeded then MACHINE.\n"
"- 'silenceThreshold' is the silence threshold.\n"
"- 'maximumWordLength' is the maximum duration of a word to accept. If exceeded then MACHINE\n"
"This application sets the following channel variables upon completion:\n"
"    AMDSTATUS - This is the status of the answering machine detection.\n"
"                Possible values are:\n"
"                MACHINE | HUMAN | NOTSURE | HANGUP\n"
"    AMDCAUSE - Indicates the cause that led to the conclusion.\n"
"               Possible values are:\n"
"               TOOLONG-<%d total_time>\n"
"               INITIALSILENCE-<%d silenceDuration>-<%d initialSilence>\n"
"               HUMAN-<%d silenceDuration>-<%d afterGreetingSilence>\n"
"               MAXWORDS-<%d wordsCount>-<%d maximumNumberOfWords>\n"
"               LONGGREETING-<%d voiceDuration>-<%d greeting>\n"
"               MAXWORDLENGTH-<%d consecutiveVoiceDuration>\n";

#define STATE_IN_WORD       1
#define STATE_IN_SILENCE    2

/* Some default values for the algorithm parameters. These defaults will be overwritten from amd.conf */
static int dfltInitialSilence       = 2500;
static int dfltGreeting             = 1500;
static int dfltAfterGreetingSilence = 800;
static int dfltTotalAnalysisTime    = 5000;
static int dfltMinimumWordLength    = 100;
static int dfltBetweenWordsSilence  = 50;
static int dfltMaximumNumberOfWords = 3;
static int dfltSilenceThreshold     = 256;
static int dfltMaximumWordLength    = 5000; /* Setting this to a large default so it is not used unless specify it in the configs or command line */

/* Set to the lowest ms value provided in amd.conf or application parameters */
static int dfltMaxWaitTimeForFrame  = 50;

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int amd_exec(struct cw_channel *chan, int argc, char **argv) {
	struct localuser *u;
	int res = 0;
	struct cw_frame *f = NULL;
	struct cw_dsp *silenceDetector = NULL;
	int dspsilence = 0, readFormat, framelength = 0;
	int inInitialSilence = 1;
	int inGreeting = 0;
	int voiceDuration = 0;
	int silenceDuration = 0;
	int iTotalTime = 0;
	int iWordsCount = 0;
	int currentState = STATE_IN_WORD;
	int previousState = STATE_IN_SILENCE;
	int consecutiveVoiceDuration = 0;
	char amdCause[256] = "", amdStatus[256] = "";

	/* Lets set the initial values of the variables that will control the algorithm.
	   The initial values are the default ones. If they are passed as arguments
	   when invoking the application, then the default values will be overwritten
	   by the ones passed as parameters. */
	int initialSilence       = dfltInitialSilence;
	int greeting             = dfltGreeting;
	int afterGreetingSilence = dfltAfterGreetingSilence;
	int totalAnalysisTime    = dfltTotalAnalysisTime;
	int minimumWordLength    = dfltMinimumWordLength;
	int betweenWordsSilence  = dfltBetweenWordsSilence;
	int maximumNumberOfWords = dfltMaximumNumberOfWords;
	int silenceThreshold     = dfltSilenceThreshold;
	int maximumWordLength    = dfltMaximumWordLength;
	int maxWaitTimeForFrame  = dfltMaxWaitTimeForFrame;

	cw_verbose(VERBOSE_PREFIX_3 "AMD: %s %s %s (Fmt: %d)\n", chan->name ,chan->cid.cid_ani, chan->cid.cid_rdnis, chan->readformat);
//[initialSilence],[greeting],[afterGreetingSilence],[totalAnalysisTime]\n"
//"      ,[minimumWordLength],[betweenWordsSilence],[maximumNumberOfWords]\n"
//"      ,[silenceThreshold],[|maximumWordLength]	
	
	if (argc == 1)
		initialSilence = atoi(argv[0]);
	if (argc == 2)
		greeting = atoi(argv[1]);
	if (argc == 3)
		afterGreetingSilence = atoi(argv[2]);
	if (argc == 4)
		totalAnalysisTime = atoi(argv[3]);
	if (argc == 5)
		minimumWordLength = atoi(argv[4]);
	if (argc == 6)
		betweenWordsSilence = atoi(argv[5]);
	if (argc == 7)
		maximumNumberOfWords = atoi(argv[6]);
	if (argc == 8)
		silenceThreshold = atoi(argv[7]);
	if (argc == 9)
		maximumWordLength = atoi(argv[8]);
	LOCAL_USER_ADD(u);

	/* Find lowest ms value, that will be max wait time for a frame */
	if (maxWaitTimeForFrame > initialSilence)
		maxWaitTimeForFrame = initialSilence;
	if (maxWaitTimeForFrame > greeting)
		maxWaitTimeForFrame = greeting;
	if (maxWaitTimeForFrame > afterGreetingSilence)
		maxWaitTimeForFrame = afterGreetingSilence;
	if (maxWaitTimeForFrame > totalAnalysisTime)
		maxWaitTimeForFrame = totalAnalysisTime;
	if (maxWaitTimeForFrame > minimumWordLength)
		maxWaitTimeForFrame = minimumWordLength;
	if (maxWaitTimeForFrame > betweenWordsSilence)
		maxWaitTimeForFrame = betweenWordsSilence;

	/* Now we're ready to roll! */
	cw_verbose(VERBOSE_PREFIX_3 "AMD: initialSilence [%d] greeting [%d] afterGreetingSilence [%d] "
		"totalAnalysisTime [%d] minimumWordLength [%d] betweenWordsSilence [%d] maximumNumberOfWords [%d] silenceThreshold [%d] maximumWordLength [%d] \n",
				initialSilence, greeting, afterGreetingSilence, totalAnalysisTime,
				minimumWordLength, betweenWordsSilence, maximumNumberOfWords, silenceThreshold, maximumWordLength);

	/* Set read format to signed linear so we get signed linear frames in */
	readFormat = chan->readformat;
	if (cw_set_read_format(chan, CW_FORMAT_SLINEAR) < 0 ) {
		cw_log(LOG_WARNING, "AMD: Channel [%s]. Unable to set to linear mode, giving up\n", chan->name );
		pbx_builtin_setvar_helper(chan , "AMDSTATUS", "");
		pbx_builtin_setvar_helper(chan , "AMDCAUSE", "");
		LOCAL_USER_REMOVE(u);
		return;
	}

	/* Create a new DSP that will detect the silence */
	if (!(silenceDetector = cw_dsp_new())) {
		cw_log(LOG_WARNING, "AMD: Channel [%s]. Unable to create silence detector :(\n", chan->name );
		pbx_builtin_setvar_helper(chan , "AMDSTATUS", "");
		pbx_builtin_setvar_helper(chan , "AMDCAUSE", "");
		LOCAL_USER_REMOVE(u);
		return;
	}

	/* Set silence threshold to specified value */
	cw_dsp_set_threshold(silenceDetector, silenceThreshold);

	/* Now we go into a loop waiting for frames from the channel */
	while ((res = cw_waitfor(chan, 2 * maxWaitTimeForFrame)) > -1) {

		/* If we fail to read in a frame, that means they hung up */
		if (!(f = cw_read(chan))) {
			cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. HANGUP\n", chan->name);
			cw_log(LOG_DEBUG, "Got hangup\n");
			strcpy(amdStatus, "HANGUP");
			break;
		}

		if (f->frametype == CW_FRAME_VOICE || f->frametype == CW_FRAME_NULL || f->frametype == CW_FRAME_CNG) {
			/* If the total time exceeds the analysis time then give up as we are not too sure */
			if (f->frametype == CW_FRAME_VOICE)
				framelength = (cw_codec_get_samples(f) / DEFAULT_SAMPLES_PER_MS);
			else
				framelength += 2 * maxWaitTimeForFrame;

			iTotalTime += framelength;
			if (iTotalTime >= totalAnalysisTime) {
				cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Too long...\n", chan->name );
				cw_fr_free(f);
				strcpy(amdStatus , "NOTSURE");
				sprintf(amdCause , "TOOLONG-%d", iTotalTime);
				break;
			}

			/* Feed the frame of audio into the silence detector and see if we get a result */
			if (f->frametype != CW_FRAME_VOICE)
				dspsilence += 2 * maxWaitTimeForFrame;
			else {
				dspsilence = 0;
				cw_dsp_silence(silenceDetector, f, &dspsilence);
			}

			if (dspsilence > 0) {
				silenceDuration = dspsilence;
				
				if (silenceDuration >= betweenWordsSilence) {
					if (currentState != STATE_IN_SILENCE ) {
						previousState = currentState;
						cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Changed state to STATE_IN_SILENCE\n", chan->name);
					}
					/* Find words less than word duration */
					if (consecutiveVoiceDuration < minimumWordLength && consecutiveVoiceDuration > 0){
						cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Short Word Duration: %d\n", chan->name, consecutiveVoiceDuration);
					}
					currentState  = STATE_IN_SILENCE;
					consecutiveVoiceDuration = 0;
				}

				if (inInitialSilence == 1  && silenceDuration >= initialSilence) {
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. ANSWERING MACHINE: silenceDuration:%d initialSilence:%d\n",
						chan->name, silenceDuration, initialSilence);
					cw_fr_free(f);
					strcpy(amdStatus , "MACHINE");
					sprintf(amdCause , "INITIALSILENCE-%d-%d", silenceDuration, initialSilence);
					res = 1;
					break;
				}
				
				if (silenceDuration >= afterGreetingSilence  &&  inGreeting == 1) {
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. HUMAN: silenceDuration:%d afterGreetingSilence:%d\n",
						chan->name, silenceDuration, afterGreetingSilence);
					cw_fr_free(f);
					strcpy(amdStatus , "HUMAN");
					sprintf(amdCause , "HUMAN-%d-%d", silenceDuration, afterGreetingSilence);
					res = 1;
					break;
				}
				
			} else {
				consecutiveVoiceDuration += framelength;
				voiceDuration += framelength;

				/* If I have enough consecutive voice to say that I am in a Word, I can only increment the
				   number of words if my previous state was Silence, which means that I moved into a word. */
				if (consecutiveVoiceDuration >= minimumWordLength && currentState == STATE_IN_SILENCE) {
					iWordsCount++;
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Word detected. iWordsCount:%d\n", chan->name, iWordsCount);
					previousState = currentState;
					currentState = STATE_IN_WORD;
				}
				if (consecutiveVoiceDuration >= maximumWordLength){
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Maximum Word Length detected. [%d]\n", chan->name, consecutiveVoiceDuration);
					cw_fr_free(f);
					strcpy(amdStatus , "MACHINE");
					sprintf(amdCause , "MAXWORDLENGTH-%d", consecutiveVoiceDuration);
					break;
				}
				if (iWordsCount >= maximumNumberOfWords) {
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. ANSWERING MACHINE: iWordsCount:%d\n", chan->name, iWordsCount);
					cw_fr_free(f);
					strcpy(amdStatus , "MACHINE");
					sprintf(amdCause , "MAXWORDS-%d-%d", iWordsCount, maximumNumberOfWords);
					res = 1;
					break;
				}

				if (inGreeting == 1 && voiceDuration >= greeting) {
					cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. ANSWERING MACHINE: voiceDuration:%d greeting:%d\n", chan->name, voiceDuration, greeting);
					cw_fr_free(f);
					strcpy(amdStatus , "MACHINE");
					sprintf(amdCause , "LONGGREETING-%d-%d", voiceDuration, greeting);
					res = 1;
					break;
				}

				if (voiceDuration >= minimumWordLength ) {
					if (silenceDuration > 0)
						cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Detected Talk, previous silence duration: %d\n", chan->name, silenceDuration);
					silenceDuration = 0;
				}
				if (consecutiveVoiceDuration >= minimumWordLength && inGreeting == 0) {
					/* Only go in here once to change the greeting flag when we detect the 1st word */
					if (silenceDuration > 0)
						cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Before Greeting Time:  silenceDuration: %d voiceDuration: %d\n", chan->name, silenceDuration, voiceDuration);
					inInitialSilence = 0;
					inGreeting = 1;
				}
				
			}
		}
		cw_fr_free(f);
	}
	
	if (!res) {
		/* It took too long to get a frame back. Giving up. */
		cw_verbose(VERBOSE_PREFIX_3 "AMD: Channel [%s]. Too long...\n", chan->name);
		strcpy(amdStatus , "NOTSURE");
		sprintf(amdCause , "TOOLONG-%d", iTotalTime);
	}

	/* Set the status and cause on the channel */
	pbx_builtin_setvar_helper(chan , "AMDSTATUS" , amdStatus);
	pbx_builtin_setvar_helper(chan , "AMDCAUSE" , amdCause);

	/* Restore channel read format */
	if (readFormat && cw_set_read_format(chan, readFormat))
		cw_log(LOG_WARNING, "AMD: Unable to restore read format on '%s'\n", chan->name);

	/* Free the DSP used to detect silence */
	cw_dsp_free(silenceDetector);

	LOCAL_USER_REMOVE(u);
	return;
}

static int load_config(int reload)
{
	struct cw_config *cfg = NULL;
	char *cat = NULL;
	struct cw_variable *var = NULL;

	if (!(cfg = cw_config_load("amd.conf"))) {
		cw_log(LOG_ERROR, "Configuration file amd.conf missing.\n");
		return -1;
	}

	cat = cw_category_browse(cfg, NULL);

	while (cat) {
		if (!strcasecmp(cat, "general") ) {
			var = cw_variable_browse(cfg, cat);
			while (var) {
				if (!strcasecmp(var->name, "initial_silence")) {
					dfltInitialSilence = atoi(var->value);
				} else if (!strcasecmp(var->name, "greeting")) {
					dfltGreeting = atoi(var->value);
				} else if (!strcasecmp(var->name, "after_greeting_silence")) {
					dfltAfterGreetingSilence = atoi(var->value);
				} else if (!strcasecmp(var->name, "silence_threshold")) {
					dfltSilenceThreshold = atoi(var->value);
				} else if (!strcasecmp(var->name, "total_analysis_time")) {
					dfltTotalAnalysisTime = atoi(var->value);
				} else if (!strcasecmp(var->name, "min_word_length")) {
					dfltMinimumWordLength = atoi(var->value);
				} else if (!strcasecmp(var->name, "between_words_silence")) {
					dfltBetweenWordsSilence = atoi(var->value);
				} else if (!strcasecmp(var->name, "maximum_number_of_words")) {
					dfltMaximumNumberOfWords = atoi(var->value);
				} else if (!strcasecmp(var->name, "maximum_word_length")) {
					dfltMaximumWordLength = atoi(var->value);

				} else {
					cw_log(LOG_WARNING, "%s: Cat:%s. Unknown keyword %s at line %d of amd.conf\n",
						app, cat, var->name, var->lineno);
				}
				var = var->next;
			}
		}
		cat = cw_category_browse(cfg, cat);
	}

	cw_config_destroy(cfg);

	cw_verbose(VERBOSE_PREFIX_3 "AMD defaults: initialSilence [%d] greeting [%d] afterGreetingSilence [%d] "
		"totalAnalysisTime [%d] minimumWordLength [%d] betweenWordsSilence [%d] maximumNumberOfWords [%d] silenceThreshold [%d] maximumWordLength [%d]\n",
		dfltInitialSilence, dfltGreeting, dfltAfterGreetingSilence, dfltTotalAnalysisTime,
		dfltMinimumWordLength, dfltBetweenWordsSilence, dfltMaximumNumberOfWords, dfltSilenceThreshold, dfltMaximumWordLength);

	return 0;
}

int unload_module(void)
{
	return cw_unregister_application(app);
}

int load_module(void)
{
	app = cw_register_application(name, amd_exec, synopsis, syntax, descrip);
	
	return 0;
}

int reload(void)
{
	return (load_config(1));
}

int usecount(void)
{
        int res;
        STANDARD_USECOUNT(res);
        return res;
}

char *description(void)
{               
        return tdesc;
}
