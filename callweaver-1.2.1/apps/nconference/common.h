/*
 * app_nconference
 *
 * NConference
 * A channel independent conference application for CallWeaver
 *
 * Copyright (C) 2002, 2003 Navynet SRL
 * http://www.navynet.it
 *
 * Massimo "CtRiX" Cetra - ctrix (at) navynet.it
 *
 * This program may be modified and distributed under the 
 * terms of the GNU Public License V2.
 *
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#ifndef _NCONFERENCE_COMMON_H
#define NCONFERENCE_COMMON_H

/* callweaver includes */
#include "callweaver.h"

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/config.h"
#include "callweaver/app.h"
#include "callweaver/dsp.h"
#include "callweaver/musiconhold.h"
#include "callweaver/manager.h"
#include "callweaver/options.h"
#include "callweaver/cli.h"
#include "callweaver/say.h"
#include "callweaver/utils.h"
#include "callweaver/translate.h"
#include "callweaver/frame.h"
#include "callweaver/features.h"

/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>


extern cw_mutex_t conflist_lock;


// debug logging level
#define APP_NCONFERENCE_DEBUG	0

#if (APP_NCONFERENCE_DEBUG == 0)
#define CW_CONF_DEBUG 	LOG_NOTICE
#else
#define CW_CONF_DEBUG 	LOG_NOTICE
#endif



#define APP_CONFERENCE_NAME     "NConference"
#define APP_CONFERENCE_MANID	"NConference-"

//
// feature defines
//
#define ENABLE_VAD		1
#define ENABLE AUTOGAIN		0	// Not used yet

// sample information for CW_FORMAT_SLINEAR format
#define CW_CONF_SAMPLE_RATE_8K 	8000
#define CW_CONF_SAMPLE_RATE_16K 	16000
#define CW_CONF_SAMPLE_RATE 		CW_CONF_SAMPLE_RATE_8K

// Time to wait while reading a channel
#define CW_CONF_WAITFOR_TIME 40 

// Time to destroy empty conferences (seconds)
#define CW_CONF_DESTROY_TIME 300

// -----------------------------------------------
#define CW_CONF_SKIP_MS_AFTER_VOICE_DETECTION 	210
#define CW_CONF_SKIP_MS_WHEN_SILENT     		90

#define CW_CONF_CBUFFER_8K_SIZE 3072


// Timelog functions


#if 1

#define TIMELOG(func,min,message) \
	do { \
		struct timeval t1, t2; \
		int diff; \
		gettimeofday(&t1,NULL); \
		func; \
		gettimeofday(&t2,NULL); \
		if((diff = usecdiff(&t2, &t1)) > min) \
			cw_log( CW_CONF_DEBUG, "TimeLog: %s: %d ms\n", message, diff); \
	} while (0)

#else

#define TIMELOG(func,min,message) func

#endif

#endif // _NCONFERENCE_COMMON_H
