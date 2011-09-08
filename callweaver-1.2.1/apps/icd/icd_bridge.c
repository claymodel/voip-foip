/*
 * ICD - Intelligent Call Distributor 
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com>
 * Additions, Changes and Support by Tim R. Clark <tclark at shaw dot ca>
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o., Piotr Figurny and Michal Bielicki
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2005, Digium, Inc.
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
  * \brief icd_bridge.c - direct bridging taken mostly out of the asterisk pbx
  */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_globals.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_command.h"

#ifndef  CW_FLAG_NONATIVE
#define  CW_FLAG_NONATIVE (1 << 7)
#endif


extern icd_fieldset *agents;

/* shamelessly borrowed from real callweaver and slowly moprhed to our needs*/
int ok_exit_noagent(icd_caller * caller);
int ok_exit(icd_caller * caller, char digit);
int no_agent(icd_caller * caller, icd_queue * queue);    
static int say_position(icd_caller *that, int override, int waiting);

int icd_bridge_call(icd_caller *bridger, icd_caller *bridgee)
{
    struct cw_channel *chan,*peer;
    struct cw_bridge_config bridge_config;
    int res = 0;

    cw_log(LOG_WARNING, "icd_bridge_call in progress...\n");
    chan = (cw_channel *) icd_caller__get_channel(bridger);
    peer = (cw_channel *) icd_caller__get_channel(bridgee);

    if (chan == NULL || peer == NULL) {
        cw_log(LOG_WARNING, "Bridge failed not enough channels\n");
        icd_caller__set_state(bridger, ICD_CALLER_STATE_CHANNEL_FAILED);
        icd_caller__set_state(bridgee, ICD_CALLER_STATE_CHANNEL_FAILED);
        return -1;
    }
    /* assume the bridge other wise we need to use 2 threads/call (control & pbx) threads for each caller */
    icd_caller__set_state(bridger, ICD_CALLER_STATE_BRIDGED);
    icd_caller__set_state(bridgee, ICD_CALLER_STATE_BRIDGED);

    memset(&bridge_config, 0, sizeof(struct cw_bridge_config));
    cw_set_flag(&(bridge_config.features_caller),  CW_FEATURE_DISCONNECT |  CW_FEATURE_REDIRECT);
 
    /* shutdown any generators such as moh on the various channels */
    icd_caller__stop_waiting(bridger);
    icd_caller__stop_waiting(bridgee);
    icd_caller__add_flag(bridger, ICD_ENTLOCK_FLAG);
    icd_caller__add_flag(bridgee, ICD_ENTLOCK_FLAG);

    cw_set_flag(chan,  CW_FLAG_NONATIVE);
    res = cw_bridge_call(chan, peer, &bridge_config);
    cw_clear_flag(chan,  CW_FLAG_NONATIVE);

    /* 
       Exit bridge loop chan==bridger, peer==bridgee. 1) hungup 2) DTMF * 3)error has occured.  
       At this point only 1 or the other will be left alive 
       set both to ICD_CALLER_STATE_CALL_END and let them sort it out 
       as long as the state is still ICD_CALLER_STATE_BRIDGED
       otherwise something else has already changed it.
     */

    icd_caller__clear_flag(bridger, ICD_ENTLOCK_FLAG);
    icd_caller__clear_flag(bridgee, ICD_ENTLOCK_FLAG);

    if (bridgee && icd_caller__get_state(bridgee) == ICD_CALLER_STATE_BRIDGED)
        icd_caller__set_state(bridgee, ICD_CALLER_STATE_CALL_END);
    if (bridger && icd_caller__get_state(bridger) == ICD_CALLER_STATE_BRIDGED)
        icd_caller__set_state(bridger, ICD_CALLER_STATE_CALL_END);

    return res;
}

int icd_bridge__wait_call_agent(icd_caller * that)
{
    int res = 0;
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);
    int result = 0;

    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD Caller waiting is ID[%d] \n", icd_caller__get_id(that));

    icd_caller__start_waiting(that);
    for (;;) {
        /* This is the wait loop for agent callers */

        /* To Do add the logic to pipe in annoucements, time the caller out of the q, or exit if no agents  */
        for (;;) {
            if (!(icd_caller__get_state(that) == ICD_CALLER_STATE_READY)) {
                /*state change occured, we are just a bridgee waiting to get bridged continue waiting */
            if (icd_debug)
              cw_log(LOG_DEBUG, "ICD Agent waiting ID[%d] changed state to [%s] \n", icd_caller__get_id(that), icd_caller__get_state_string(that));
                if (!(icd_caller__has_role(that, ICD_BRIDGEE_ROLE)
                        && icd_caller__get_state(that) == ICD_CALLER_STATE_DISTRIBUTING)) {
                    result = -1;
                    res = 0;
                    break;
                }
            }
            /* Wait some milliseconds before checking again */
            res = cw_waitfordigit(chan, 200);
            if (res) {
                break;
            }

        }

        /* If they hungup, return immediately */
        if (res < 0) {
            /* default_moh_stop(that); */
            icd_caller__stop_waiting(that);
            if (icd_caller__get_onhook(that)){
	      icd_bridge__safe_hangup(that);
              icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
	    }
	    else{
//              cw_softhangup(chan ,  CW_SOFTHANGUP_EXPLICIT);
//              cw_hangup(chan);
//              icd_caller__set_channel(that, NULL);
              icd_caller__set_state(that, ICD_CALLER_STATE_SUSPEND);
	    }  
            break;
        }

        if (result < 0) {
            /* state change occured, time to go to next state */
            break;
        }

        if (!res) {
            break;
        }
    }
    return res;
}

int icd_bridge__wait_call_customer(icd_caller * that)
{
    int res = 0;
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);
    icd_queue *queue;
    char *chimefile;
    int result = 0;
    time_t now, start;
    int avgholdmins;
    static int waitms = 100;
    int noagent_wait_timeout = 20;       /* number of sec to wait when no agents in the q */
    char *noagent_time; 
    int agent_count;
    
    noagent_time = icd_caller__get_param(that, "noagent_timeout");
    if(noagent_time){
       noagent_wait_timeout= atoi(noagent_time);
    }   
    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD Caller waiting is ID[%d] \n", icd_caller__get_id(that));
    icd_caller__start_waiting(that);
    start = icd_caller__get_start(that);
    for (;;) {
        /* This is the wait loop for callers */

        /* pipe in annoucements, time the caller out of the q, or exit if no agents  */
        for (;;) {
            if (!(icd_caller__get_state(that) == ICD_CALLER_STATE_READY)) {
                /*state change occured, we are just a bridgee waiting to get bridged continue waiting */
                if (!(icd_caller__has_role(that, ICD_BRIDGEE_ROLE)
                        && (/*icd_caller__get_state(that) == ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE
                            || */ 
			   icd_caller__get_state(that) == ICD_CALLER_STATE_DISTRIBUTING))
                    ) {
                    result = -1;
                    res = 0;
                    break;
                }
            }
            /* Wait some milliseconds before checking again */
            res = cw_waitfordigit(chan, waitms);
            /* -1 hangup detected, or DTMF digits #-35,*-42 0-48,1-49, ..9-57 or t-116 timeout */
            if (res != 0)
                break;

            //member = icd_caller__get_memberships_peek(that);
            //queue = icd_member__get_queue(member);
            queue = icd_member__get_queue(icd_caller__get_memberships_peek(that));

            /* If no agent (agent count =0) is serving this queue then exit to the priority +101 ext 
             * should we check pending agents in the dist or just use the q ?
             icd_distributor__agents_pending(icd_queue__get_distributor(queue));
             */
	    /* We take into account only agents that are in queue and logged in  */
            
            /* Matchagent special case */
            if (no_agent(that, queue)){
                res = 1;
                break;
            }

            agent_count=icd_queue__agent_active_count(queue);
            if (agent_count == 0) {
                time(&now);
                if ((now - start) >= noagent_wait_timeout) {
                    res = 1;
                    break;
                }
            }

            /* If wait timed out, break out, & set res = ext that handle timeouts */
            if (icd_queue__get_wait_timeout(queue)) {
                time(&now);
                if ((now - start) >= icd_queue__get_wait_timeout(queue)) {
                    /*set res to [t]imeout ext in context for ok_exit, the ext for wait timeouts   */
                    res = 116;  /* 116-t or we could just use 85-T */
                    break;
                }
            }
            /* Round hold time to nearest minute */

            avgholdmins = icd_queue__get_holdannounce_holdtime(queue);

            /* Make a position announcement, if enabled */
            if (icd_queue__get_holdannounce_cycle(queue) && icd_caller__get_entertained(that) == ICD_ENTERTAIN_MOH) {
                res = say_position(that, 0, 1);
            }
            if (res < 0)
                break;

            chimefile = icd_queue__chime(queue, that);
            if (chimefile != NULL) {
                if (strcmp(chimefile, "skip")) {        /* not skip */
                    if (!strcmp(chimefile, "announce_pos")) {
                        res = say_position(that, 1, 1);
                        if (res < 0)
                            break;
                    } else if (!strcmp(chimefile, "announce_pos_time")) {
                        res = say_position(that, 2, 1);
                        if (res < 0)
                            break;
                    } else {
                        res = icd_caller__play_sound_file(that, chimefile);
                        if (res < 0)
                            break;
                    }

                }
                time(&now);
                icd_caller__set_chimenext(that, now + icd_queue__get_chime_freq(queue));
            }

        }

        /* If they hungup, return immediately */
        if (res < 0) {
            /* Record this abandoned call */
            if (icd_verbose > 2)
                cw_log(LOG_WARNING, "Caller %s [%d] disconnected while waiting their turn\n",
                    icd_caller__get_name(that), icd_caller__get_id(that));
                icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: HANGUP\r\nID: %d\r\n"
                "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan"); 
            res = -1;

            /* prolly should update a counter of callers that abandoned need to lock q & update */

            icd_bridge__safe_hangup(that);
            icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
            break;
        }

        if (result < 0) {
            /* state change occured, time to go to next state */
            icd_caller__stop_waiting(that);
	    break;

        }

        if (!res) {
            break;
        }
        if (res == 1){ 
          cw_log(LOG_WARNING, "Caller exit while waiting turn in line no agents available customer id[%d] [%s] \n", icd_caller__get_id(that), icd_caller__get_name(that));
	  if(ok_exit_noagent(that)){
                icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: NoAgentTimeout\r\nID: %d\r\n"
                "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan"); 
            icd_caller__stop_waiting(that);
            icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);     /* todo new exit code */
            break;
          }
	  else{
            /*set res to [t]imeout ext in context for ok_exit, the ext for wait timeouts   */
	    /* it will be serviced like regular timeout   */
            res = 116;  /* 116-t or we could just use 85-T */
            icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: NoAgentTimeout\r\nID: %d\r\n"
                "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\nExtension: NoExtension\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan"); 
	    break;
	  }
	}     

        if (ok_exit(that, res)) {
           icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: Exit\r\nID: %d\r\n"
                "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\nExtension: %d\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan",res); 
            cw_log(LOG_WARNING, "Caller exit to exten[%d] while waiting turn in line\n", res);
            icd_caller__stop_waiting(that);
            icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);     /* todo new exit code */
            break;
        }

    }
    if(res == 116){ /*Timeout  */
        if (ok_exit(that, res)) {
           icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: Timeout\r\nID: %d\r\n"
                "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\nExtension: %d\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan",res); 
            cw_log(LOG_WARNING, "Caller exit to exten[%d] while waiting turn in line\n", res);
	}  
	else {
        icd_manager_send_message("Module: ICD_BRIDGE\r\nEvent: Timeout\r\nID: %d\r\n"
               "CallerName: %s\r\nChannelName: %s\r\nUniqueid: %s\r\nExtension: NoExtension\r\n",  
	             icd_caller__get_id(that), icd_caller__get_name(that), chan ? chan->name: "nochan", chan ? chan->uniqueid : "nochan"); 
	}    
        icd_caller__stop_waiting(that);
        icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);     /* todo new exit code */
    }    
    return res;
}

int icd_bridge_wait_ack(icd_caller * that)
{
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);
    int max_wait_ack = 60;      /* TBD stick this in the agent config */
    int result = 0;
    int res = 0;
    time_t start_time;
    time_t now;

    assert(chan != NULL);
    if (icd_debug)
        cw_log(LOG_DEBUG, "ICD Agent waiting for acknowledgment is ID  %d\n", icd_caller__get_id(that));
    
    time(&start_time);	
    max_wait_ack = icd_caller__get_timeout(that)/1000; /*converting from milisec to seconds */	
/*Prepare to extern ackowledgement */    
    icd_caller__clear_flag(that, ICD_ACK_EXTERN_FLAG);
    icd_caller__stop_waiting(that);
    
    res = icd_bridge__play_sound_file(chan, "queue-callswaiting"); 

	/* This is the wait loop for agents that requirement an acknowledgement  b4 we bridge the call */
	if(!res){
      for (;;) {
            if (!(icd_caller__get_state(that) == ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE)) {
                result = -1;
                break;
            }
            if(!icd_caller_list__has_callers(icd_caller__get_associations(that))){
                icd_caller__start_waiting(that);
	        result = -1;
		break;
	    }
           /*Check for extern acknowledgement */    
            if (icd_caller__has_flag(that, ICD_ACK_EXTERN_FLAG)){
	        res = 1;
	        break;
            }
	    
	    /* Wait a second before checking again */
            res = cw_waitfordigit(chan, 200);

            if (res) {
                break;
            }

            time(&now);
            if ((now - start_time) > max_wait_ack){
                result = -1;
                break;
	    }	
      }
	}

    if (result < 0) {
//           icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
    	if (chan->stream) { 
	            cw_stopstream(chan);       /* cut off any files that are playing */ 
    	}
        return result;
     }
        /* If they hungup, return immediately */
    if (res < 0) {
       icd_bridge__safe_hangup(that);
//            icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
        return -1;   
     }
        /* ok we got some type of dtmf, ok to bridge */
    if (res) {
    	         if (chan->stream) { 
	            cw_stopstream(chan);       /* cut off any files that are playing */ 
	         } 
    			return 0;
	}
/* Never get here - for compiler's sake */	 
   if (chan->stream) { 
	            cw_stopstream(chan);       /* cut off any files that are playing */ 
   	}
    return 0;	 
}

//! An callweaver-specific method to translate text strings into a channel
/*!
 * There are two ways to get a channel. The first uses the channel string,
 * which divvies up into a protocol (type) and data that is specific for the
 * particular type. The second method to get a channel is to identify the
 * context/priority/extension and use a local channel to feed back from the
 * dialplan. So far, we have only implemented the channel string.
 * /param chanstring a string in the form "type/data"
 * /param context a context as defined by the dialplan
 * /param priority the priority that identifies lines in the dialplan
 * /param extension the extension in the dialplan to look at
 * /return an callweaver channel or NULL on failure
 * /todo implement the local channel method for getting a channel
 */
struct cw_channel *icd_bridge_get_callweaver_channel(char *chanstring, char *context, char *priority,
    char *extension)
{
    struct cw_channel *chan = NULL;
    int cause;
    char *type;
    char *data;

    /* The channel string method */
    if (chan == NULL && chanstring != NULL) {
        type = strdup(chanstring);
        data = strchr(type, '/');
        if (data != NULL) {
            *data = '\0';
            data++;
            /* BCA - Is ULAW the only format we support? */
            chan = cw_request(type,  CW_FORMAT_ALAW, data, &cause);

        }
        ICD_STD_FREE(type);
    }
    /* Did it work? */
    if (chan != NULL) {
        return chan;
    }
    /* Nope. Figure out why and report. */
    if (type != NULL && data != NULL) {
        cw_log(LOG_NOTICE, "ICD REQUEST: Unable to request channel %s/%s\n", type, (char *) data);
    } else if (chanstring != NULL) {
        cw_log(LOG_NOTICE, "ICD REQUEST: Channel '%s' not specified in type/data format\n", chanstring);
    } else {
        cw_log(LOG_NOTICE, "ICD REQUEST: Local Channel creation not yet supported\n");
    }
    return NULL;
}

//! Bring up an callweaver channel
/*!
 * Ask the low levels of callweaver to ring a channel
 * until it comes up or we don't care any more.
 *
 * \param caller the caller object
 * \param chanstring the address to ring
 * \param timeout how long in milliseconds to wait before timing out
 * \return callweaver channel state
 */
int icd_bridge_dial_callweaver_channel(icd_caller * that, char *chanstring, int timeout)
{
    //struct cw_channel *newchan;
    struct cw_channel *chan;
    struct cw_frame *f = NULL;
    char *caller_id;
    char *addr;
    int result;
    int state = CW_CONTROL_HANGUP;
//    int assoc;
    icd_caller_state caller_state;

    int module_id = ICD_CALLER; /* keep event macro happy and icd_bridge is not realy a module */

    assert(that != NULL);

    chan = icd_caller__get_channel(that);
    if (chan == NULL) {
        return state;
    }

    caller_id = icd_caller__get_caller_id(that);
    if (caller_id != NULL && strlen(caller_id) > 0) {
        /* cw_set_callerid(chan, caller_id, 1); v1.0 */
        cw_set_callerid(chan, caller_id, NULL, caller_id);
    }

    addr = NULL;
    if (chanstring != NULL && strlen(chanstring) > 0) {
        addr = strchr(chanstring, '/');
        if (addr != NULL) {
            addr++;
        }
    }
    if (addr == NULL) {
        cw_log(LOG_WARNING, "ICD REQUEST: Could not identify address in channel [%s]\n", chanstring);
        return state;
    }

    /* cw_call() starts ringing and returns immediately with timeout (0) or error. */

    result = cw_call(chan, addr, 0);
    if (result < 0) {
        if (chanstring != NULL) {
            cw_log(LOG_WARNING, "ICD REQUEST: Unable to ring channel %s\n", chanstring);
        } else {
            cw_log(LOG_WARNING, "ICD REQUEST: Unable to ring local channel\n");
        }
        return state;
    }
    	 
//    assoc = icd_caller_list__has_callers(icd_caller__get_associations(that));
    caller_state = icd_caller__get_state(that);
    /* While we haven't timed out and we still have no channel up */
    while (timeout && (chan->_state != CW_STATE_UP)) {
        result = cw_waitfor(chan, timeout);
        /* Something is not cool */
        if (result < 0) {
            break;
        }
        /* Timed out, so we are done trying */
        if (result == 0) {
            break;
        }
        /* -1 means go forever */
        if (timeout > -1) {
            /* result holds the number of milliseconds remaining */
            timeout = result;
            if (timeout < 0) {
                timeout = 0;
            }
        }
        /* make sure the guy this is for still is there to take it */
        /* BCA - this has never worked and the algorithm can't. How can we make this check?
           if(peer && req_state >= 0 && (icd_caller__get_state(peer) != req_state)) {
           cw_log(LOG_WARNING,"lost my peer while dialing, nevermind!");
           state = CW_CONTROL_HANGUP;
           result = 0;
           if (f != NULL) {
           cw_fr_free(f);
           }
           break;
           }             */

//	 if(assoc)
//	  if(!icd_caller_list__has_callers(icd_caller__get_associations(that)))   
//	   { 
//	      return 1; 
//	   }
	  if(caller_state  != icd_caller__get_state(that)){
	      return 1; 
	   }
	  
        /* Read a frame from the channel and process it */
        f = cw_read(chan);
        if (f == NULL) {
            state = CW_CONTROL_HANGUP;
            result = 0;
            break;
        }
        if (f->frametype == CW_FRAME_CONTROL) {
            if (f->subclass == CW_CONTROL_RINGING) {
                state = f->subclass;
            } else if ((f->subclass == CW_CONTROL_BUSY) || (f->subclass == CW_CONTROL_CONGESTION)) {
                state = f->subclass;
                cw_fr_free(f);
                break;
            } else if (f->subclass == CW_CONTROL_ANSWER) {
                /* This is what we are hoping for */
                state = f->subclass;
                cw_fr_free(f);
                break;
            }
            /* else who cares */

        }
        cw_fr_free(f);
    }

    /* write the call detail records, if the caller hangs up while were dialing them */

    /* this doesnt work anyway lets make it independant responsibility to activate CDR -Tony
       if (result <= 0) {
       if (!chan->cdr) {
       chan->cdr = cw_cdr_alloc();
       }
       if (chan->cdr) {
       cw_cdr_init(chan->cdr, chan);
       cw_cdr_setapp(chan->cdr, "Dial", chanstring);
       cw_cdr_update(chan);
       cw_cdr_start(chan->cdr);
       cw_cdr_end(chan->cdr);
       //If the cause wasn't handled properly
       if (cw_cdr_disposition(chan->cdr, chan->hangupcause)) {
       cw_cdr_failed(chan->cdr);
       }
       } else {
       cw_log(LOG_WARNING, "Unable to create Call Detail Record\n");
       }
       icd_bridge__safe_hangup(that);
       }
     */
    if (chan->_state == CW_STATE_UP) {

/*  TC experiment to masq b4 bridge        
newchan = cw_channel_alloc(0);
snprintf(newchan->name, sizeof (newchan->name), "ChanGrab/%s",chan->name);
newchan->readformat = chan->readformat;
newchan->writeformat = chan->writeformat;
cw_channel_masquerade(newchan, chan);               
icd_bridge__safe_hangup(that);
icd_caller__set_channel(that, newchan);
*/

        result =
            icd_event_factory__generate(event_factory, that, icd_caller__get_name(that), module_id,
            ICD_EVENT_CHANNEL_UP, NULL, icd_caller__get_listeners(that), NULL);

        return  CW_CONTROL_ANSWER;
    }
    return state;
}

void icd_bridge__remasq(icd_caller * caller)
{
    struct cw_channel *oldchan = NULL;
    struct cw_channel *newchan = NULL;
    struct cw_frame *f;

    assert(caller != NULL);
    oldchan = (cw_channel *) icd_caller__get_channel(caller);
    if (!oldchan)               /* nothing to do */
        return;
    icd_caller__add_flag(caller, ICD_NOHANGUP_FLAG);
    newchan = cw_channel_alloc(0);
    strncpy(newchan->name, oldchan->name, sizeof(newchan->name));
    newchan->readformat = oldchan->readformat;
    newchan->writeformat = oldchan->writeformat;
    cw_channel_masquerade(newchan, oldchan);
    f = cw_read(newchan);
    if (f)
        cw_fr_free(f);

    if (oldchan) {
        cw_stopstream(oldchan);
        cw_generator_deactivate(oldchan);
        cw_clear_flag(oldchan,  CW_FLAG_BLOCKING);
        cw_softhangup(oldchan,  CW_SOFTHANGUP_EXPLICIT);
        if (icd_caller__owns_channel(caller)) {
            cw_hangup(oldchan);
        }
        oldchan = NULL;
    }
    icd_caller__set_channel(caller, newchan);

}

void icd_bridge__parse_ubf(icd_caller * caller, icd_unbridge_flag ubf)
{

    switch (ubf) {
    case ICD_UNBRIDGE_HANGUP_AGENT:
        if (icd_caller__has_role(caller, ICD_AGENT_ROLE))
            icd_bridge__safe_hangup(caller);
        else if (icd_caller__has_role(caller, ICD_CUSTOMER_ROLE)) {
            icd_bridge__remasq(caller);
            icd_caller__set_state(caller, ICD_CALLER_STATE_READY);
        }
        break;
    case ICD_UNBRIDGE_HANGUP_CUSTOMER:
        if (icd_caller__has_role(caller, ICD_CUSTOMER_ROLE))
            icd_bridge__safe_hangup(caller);
        break;
    case ICD_UNBRIDGE_HANGUP_ALL:
        icd_bridge__safe_hangup(caller);
        break;
    case ICD_UNBRIDGE_HANGUP_NONE:
        icd_bridge__remasq(caller);
        icd_caller__set_state(caller, ICD_CALLER_STATE_READY);
        break;
    }

}

void icd_bridge__unbridge_caller(icd_caller * caller, icd_unbridge_flag ubf)
{
    struct cw_channel *chan;
    icd_caller *associate;
    icd_caller_list *associations;
    icd_list_iterator *iter;

    if (!icd_caller__has_role(caller, ICD_BRIDGER_ROLE))
        return;

    chan = icd_caller__get_channel(caller);
    associations = icd_caller__get_associations(caller);

    iter = icd_list__get_iterator((icd_list *) associations);
    if (!icd_list_iterator__has_more(iter)) {
        /* no associates, shoudn't be bridged.....(I hope) */
        destroy_icd_list_iterator(&iter);
        return;
    }

    while (icd_list_iterator__has_more(iter)) {
        associate = (icd_caller *) icd_list_iterator__next(iter);
        icd_bridge__parse_ubf(associate, ubf);
    }
    destroy_icd_list_iterator(&iter);

    icd_bridge__parse_ubf(caller, ubf);

}

void icd_bridge__safe_hangup(icd_caller * caller)
{
    struct cw_channel *oldchan = NULL;
    struct cw_channel *newchan = NULL;
    struct cw_frame *f;

    assert(caller != NULL);
    if (icd_caller__has_flag(caller, ICD_NOHANGUP_FLAG)) {
        return;
    }

    oldchan = (cw_channel *) icd_caller__get_channel(caller);
    if (!oldchan || cw_test_flag(oldchan,  CW_FLAG_ZOMBIE))    /* nothing to do */
        return;

    if (icd_debug)
        cw_log(LOG_DEBUG, "Caller id[%d] [%s]Hangup called on chan[%s]\n", icd_caller__get_id(caller),
            icd_caller__get_name(caller), oldchan->name);

    /* 
       Here we demand ownership of the channel, 
       giving us the ability to safely destroy it.
       %TC Danger race condition here oldchan may have gone aways
       we should lock the oldchan while be do the masquerade
       Why do we masq if we already icd_caller__owns_channel(caller) ???
       if (!cw_mutex_trylock(&oldchan->lock)) {
     */

    newchan = cw_channel_alloc(0);
    if (newchan) {
       cw_mutex_lock(&oldchan->lock);
       strncpy(newchan->name, oldchan->name, sizeof(newchan->name));
       newchan->readformat = oldchan->readformat;
       newchan->writeformat = oldchan->writeformat;
       cw_mutex_unlock(&oldchan->lock);
       /*note masq will blindly lock both channels, does not check if the channels are still there ? */
       if (!cw_channel_masquerade(newchan, oldchan)) {
           f = cw_read(newchan);
           if (f)
             cw_fr_free(f);
       }

       cw_softhangup(newchan,  CW_SOFTHANGUP_EXPLICIT);
       cw_hangup(newchan);
       newchan = NULL;
    }

    if (oldchan) {
        cw_clear_flag(oldchan,  CW_FLAG_BLOCKING);
        cw_softhangup(oldchan,  CW_SOFTHANGUP_EXPLICIT);
        if (icd_caller__owns_channel(caller)) {
            cw_hangup(oldchan);
        }
        oldchan = NULL;
    }
    icd_caller__set_channel(caller, NULL);
}

int ok_exit_noagent(icd_caller * that)
{
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);

    /* See if there is a special busy priority in this context for this queue */
    if (chan != NULL
        && cw_exists_extension(chan, chan->context, chan->exten, chan->priority + 101, chan->cid.cid_num)) {
        chan->priority += 100;
        if (icd_verbose > 2)
            cw_log(LOG_WARNING, "Caller %s [%d] has no agents to service call exit to busy priority\n",
                icd_caller__get_name(that), icd_caller__get_id(that));
        return 1;
    }

    return 0;

}

int ok_exit(icd_caller * that, char digit)
{
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);
    char *context;
    char tmp[2];

    context = (char *) icd_caller__get_param(that, "context");
    if (context == NULL)
        context = chan->context;

    tmp[0] = digit;
    tmp[1] = '\0';

    if (chan != NULL && cw_exists_extension(chan, context, tmp, 1, chan->cid.cid_num)) {
        cw_log(LOG_WARNING, "Found digit exit context[%s] exten[%s]\n", context, tmp);
        /* possible race here caller might hangup while we figure things out */
        if (!cw_mutex_trylock(&chan->lock)) {
            strncpy(chan->context, context, sizeof(chan->context) - 1);
            strncpy(chan->exten, tmp, sizeof(chan->exten) - 1);
            chan->priority = 0;
            cw_mutex_unlock(&chan->lock);
            return 1;
        } else
            return 0;
    }
    return 0;
}

int no_agent(icd_caller * caller, icd_queue * queue){

    char * tmp_str;
    icd_caller *agent_caller = NULL;

    tmp_str = icd_caller__get_param(caller, "identifier");
    
    if (tmp_str != NULL) {
       agent_caller = (icd_caller *) icd_fieldset__get_value(agents, tmp_str);
           
       if (agent_caller == NULL) {
            return 1;
        }
        if(icd_caller__get_state(agent_caller) == ICD_CALLER_STATE_INITIALIZED
           || icd_caller__get_state(agent_caller) == ICD_CALLER_STATE_SUSPEND){
            return 1;
        }   
    }

    return 0;
}

static int say_position(icd_caller * that, int override, int waiting)
{
    struct cw_channel *chan = (cw_channel *) icd_caller__get_channel(that);
    icd_member *member;
    icd_queue *queue;
    int res = 0, avgholdmins = 0;
    time_t now;
    int x = 0, pos = 0;
    if(waiting)
        icd_caller__stop_waiting(that);

    for (x = 0; x < 1; x++) {   /* do {once} with break ability */
        /* Check to see if this is ludicrous -- if we just announced position, don't do it again */
        time(&now);
        if ((now - icd_caller__get_last_announce_time(that)) < 15) {
            res = 1;
            break;
        }

        /* peek at the Head Queue for this caller, Customer only have 1 q they can belong to */
        member = icd_caller__get_memberships_peek(that);
        queue = icd_member__get_queue(member);
        avgholdmins = icd_queue__get_holdannounce_holdtime(queue);

        /*
           if (queue == NULL) {
           cw_log(LOG_NOTICE, "Caller %d [%s] asked to say Position but does not have an active Queue \n",
           icd_caller__get_id(that), icd_caller__get_name(that));
           return -1
           }
         */

        /* If either our position has changed, or we are over the freq timer, say position */

        if ((icd_caller__get_last_announce_position(that) == icd_caller__get_position(that, member))
            && (((now - icd_caller__get_last_announce_time(that)) < icd_queue__get_holdannounce_frequency(queue))
                || override)) {
            res = 1;
            break;
        }
        
        /* Say we're next, if we are */
        if (icd_caller__get_position(that, member) == 1) {
            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_next(queue));
            if (res < 0) {
                res = 0;
                break;
            }

        } else {
            /* its actually possible we're bridged right now if it's a conference ..
               so we gotta make sure we are not already popped off the queue before
               being comfortable saying this number cos it may be -1
             */
            pos = icd_caller__get_position(that, member);
            if (pos < 1) {
                res = 0;
                break;
            }
            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_thereare(queue));
            if (res < 0) {
                res = 0;
                break;
            }
            res = cw_say_number(chan, pos,  CW_DIGIT_ANY, chan->language, (char *) NULL);      /* Needs gender */
            if (res < 0) {
                res = 0;
                break;
            }
            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_calls(queue));
            if (res < 0) {
                res = 0;
                break;
            }
        }

        /* If the hold time is >1 min, if it's enabled, and if it's not
           supposed to be only once and we have already said it, say it */

        if (avgholdmins > 1 && (((icd_queue__get_holdannounce_cycle(queue))
                    && ((!(icd_queue__get_holdannounce_cycle(queue) == 1)
                            && icd_caller__get_last_announce_position(that)))) || override > 1)) {

            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_holdtime(queue));
            if (res < 0) {
                res = 0;
                break;
            }
            res = cw_say_number(chan, avgholdmins,  CW_DIGIT_ANY, chan->language, (char *) NULL);
            if (res < 0) {
                res = 0;
                break;
            }

            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_minutes(queue));
            if (res < 0) {
                res = 0;
                break;
            }

        }
    }

    /* Set our last_pos indicators */
    if (!res) {                 /* res == 1 means dont do anything !res means it did something */
        icd_caller__set_last_announce_position(that, icd_caller__get_position(that, member));
        icd_caller__set_last_announce_time(that);
        pos = icd_caller__get_position(that, member);
        if (pos > 0)
            res = icd_caller__play_sound_file(that, icd_queue__get_holdannounce_sound_thanks(queue));
        cw_verbose(VERBOSE_PREFIX_3 "Hold time for %s in queue %s is %d minutes\n",
                    chan->name ? chan->name : "caller", icd_queue__get_name(queue), avgholdmins);
    }

    if(waiting)
        icd_caller__start_waiting(that);
    return 0;
}

int icd_safe_sleep(struct cw_channel *chan, int ms)
{
    struct cw_frame *f;

    while (ms > 0) {
        ms = cw_waitfor(chan, ms);
        if (ms < 0)
            return -1;
        if (ms > 0) {
            f = cw_read(chan);
            if (!f)
                return -1;
            cw_fr_free(f);
        }
    }
    return 0;
}

/* check the hangup status of the caller's chan (frontended for future cross compatibility) */
int icd_bridge__check_hangup(icd_caller * that)
{
    struct cw_channel *chan = NULL;

    assert(that != NULL);

    chan = (cw_channel *) icd_caller__get_channel(that);

    if (chan != NULL && chan->name != NULL) {
         if (cw_check_hangup(chan) == 0)  
	               return 0; 
	    } 
	    icd_caller__set_channel(that, NULL); 
    return 1;
}

int icd_bridge__play_sound_file(struct cw_channel *chan, char *file)
{
    int res = 0;

    if (chan == NULL || !file || !strlen(file))
        res = -1;
    else {
        res = cw_streamfile(chan, file, chan->language);
        if (!res) {
            res = cw_waitstream(chan,  CW_DIGIT_ANY);
        }
    }

    return res;
}

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

