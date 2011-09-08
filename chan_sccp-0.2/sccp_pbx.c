/*
 *
 * SCCP support for Asterisk.
 *   -- By Zozo
 */

#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_channel.h"
#include "sccp_pbx.h"

#include <asterisk/pbx.h>
#include <unistd.h>

/* Wait up to 16 seconds for first digit */
static int firstdigittimeout = 16000;
/* How long to wait for following digits */
static int gendigittimeout = 8000;
/* How long to wait for an extra digit, if there is an ambiguous match */
static int matchdigittimeout = 3000;

static struct ast_frame * sccp_rtp_read(sccp_channel_t * c)
{
  /* Retrieve audio/etc from channel.  Assumes c->lock is already held. */
  struct ast_frame * f = ast_rtp_read(c->rtp);
  if (c->owner) {
      /* We already hold the channel lock */
      if (f->frametype == AST_FRAME_VOICE) {
        if (f->subclass != c->owner->nativeformats) {
          ast_log(LOG_DEBUG, "Oooh, format changed to %d\n", f->subclass);
        c->owner->nativeformats = f->subclass;
        ast_set_read_format(c->owner, c->owner->readformat);
        ast_set_write_format(c->owner, c->owner->writeformat);
      }
    }
  }
  return f;
}


void start_rtp(sccp_channel_t * c)
{
  ast_mutex_lock(&c->lock);

  /* Allocate the RTP */
  c->rtp = ast_rtp_new(NULL, NULL, 1, 0);

  if (c->rtp && c->owner)
    c->owner->fds[0] = ast_rtp_fd(c->rtp);

//  if (c->rtp)
//    ast_rtp_setnat(c->rtp, c->nat);

  /* Create the RTP connections  */
  sccp_channel_connect(c);
  ast_mutex_unlock(&c->lock);

}

static int sccp_pbx_call(struct ast_channel *ast, char *dest, int timeout)
{

  sccp_line_t    * l;
  sccp_device_t  * d;
  sccp_session_t * s;
  sccp_channel_t * c;
  struct ast_frame f = { 0, };

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Asterisk request to call: %s\n", ast->name);

  c = ast->pvt->pvt;
  l = c->line;
  d = l->device;
  s = d->session;

  if (d->dnd) {
    ast_queue_control(ast, AST_CONTROL_BUSY, 0);
    return 0;
  }

  // BlinkBlinkBlink!
  sccp_channel_set_lamp(c, 0x5);

  //
  // Ring Ring, wakey wakey!
  //
  // XXX:T: atm, i've hardcoded to "InsideRing", however,
  // XXX:T: there is also OutsideRing (for external calls)
  // XXX:T: and FeatureRing (for voicemail, callback, etc)
  // XXX:T: I'll have to work out a sensible way to choose 
  // XXX:T: how to set it.
  //
  sccp_dev_set_ringer(d, StationInsideRing);

  // Send the fact we have an incoming call.
  // Hmm, odd. Cisco debug docs online don't mention the callstate.
  // Maybe they are just trying to confuzle us :-p
  sccp_channel_set_callstate(c, TsRingIn);

  // Send the call info.
  sccp_channel_send_callinfo(c);

  // DisplayNotifyMessage 
  // XXX:T : I don't think we actually need this, now.
  // sccp_channel_displaynotify(c);

  // Set Prompt to "Incoming Call"
  sccp_dev_statusprompt_set(d, c, "Incoming Call", 0);
  // sleep(1);

  // stationOutputSelectSoftKeys
  sccp_dev_set_keyset(d, c, KEYMODE_INCOMING);
  // sleep(1);

  // Send the callinfo.
  c->sentCall = 1;
  c->isRinging = 1;
  f.frametype = AST_FRAME_CONTROL;
  f.subclass = AST_CONTROL_RINGING;
  ast_queue_frame(c->owner, &f, 0);

  // Well, i guess we just wait for the user to answer now.  Trallla...
  return 0;
}


static int sccp_pbx_hangup(struct ast_channel * ast)
{
  sccp_line_t    * l;
  sccp_device_t  * d;
  sccp_session_t * s;
  sccp_channel_t * c;

  if (!ast->pvt->pvt) {
    ast_log(LOG_DEBUG, "Asked to hangup channel not connected\n");
    return 0;
  }

  c = ast->pvt->pvt;
  l = c->line;

  ast_mutex_lock(&l->lock);
  ast_mutex_lock(&c->lock);

  d = l->device;
  s = d->session;

  ast_log(LOG_DEBUG, "Request to hangup %s call by Asterisk - %s - %s (%d active channels on this line)\n", (c->isOutgoing) ? "outgoing" : "incoming", ast->name, d->id, l->channelCount);


  if ( ((!c->isOutgoing) && c->sentCall) || (c->isOutgoing) ) {

    if (!c->isOutgoing) {
      sccp_dev_set_ringer(d, StationRingOff);
    }

    // XXX:T: Check it's onhook, first?
    sccp_dev_set_sptone(d, NULL);

    // CLOSE_RECIEVE_CHANNEL_MESSAGE
    // STOP_MEDIA_TRANSMISSION_MESSAGE
    sccp_channel_disconnect(c);

    // SET_SPEAKER_MESSAGE  
    sccp_dev_set_speaker(d, StationSpeakerOff);

    // CALL_STATE_MESSAGE
    sccp_channel_set_callstate(c, TsOnHook);

    // CLEAR_PROMPT_STATUS_MESSAGE
    sccp_dev_statusprompt_set(d, c, NULL, 0);

    // SELECT_SOFT_KEYS_MESSAGE
    sccp_dev_set_keyset(d, c, KEYMODE_ONHOOK);

    // clear the tone again, without callRef.
    sccp_dev_set_sptone(d, "NoTone");

    d->lastCallEndTime = time(0);

    // Call prompt, without callRef.
    sccp_dev_statusprompt_set(d, NULL, "Call Ended", 5);

    // And select SoftKeySet without a callRef.
    sccp_dev_set_keyset(d, NULL, KEYMODE_ONHOOK);

  }

  if (c->line->dnState != TsOnHook) {
    sccp_dev_set_sptone(d, "ZipZip");
    sccp_dev_statusprompt_set(s->device, NULL,
			      "Call terminated by remote party.", 10);
  }

  ast->pvt->pvt     = NULL;

  if (c->rtp) {
    ast_rtp_destroy(c->rtp);
    c->rtp = NULL;
  }
  ast_log(LOG_DEBUG, "Channel = %p\n", chans);

  ast_mutex_unlock(&c->lock);

  sccp_dev_remove_channel(c);
  l->channelCount--;
  ast_mutex_unlock(&l->lock);
  return 0;
}

static int sccp_pbx_answer(struct ast_channel *ast)
{
  int res = 0;
  sccp_channel_t * c = ast->pvt->pvt;
  sccp_line_t * l    = c->line;

  if (!c->rtp) {
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "SCCP: Starting RTP\n");
    start_rtp(c);
  }

  if (sccp_debug)
    ast_verbose("Answered %s on %s@%s-%d\n", ast->name, l->name, l->device->id, c->callid);

  if (ast->_state != AST_STATE_UP) {
    ast_setstate(ast, AST_STATE_UP);
  }
  return res;
}


static struct ast_frame * sccp_pbx_read(struct ast_channel *ast)
{
  sccp_channel_t * c = ast->pvt->pvt;
  struct ast_frame *fr;
  ast_mutex_lock(&c->lock);
  fr = sccp_rtp_read(c);
  ast_mutex_unlock(&c->lock);
  return fr;
}

static int sccp_pbx_write(struct ast_channel *ast, struct ast_frame *frame)
{
  sccp_channel_t * c = ast->pvt->pvt;

  int res = 0;
  if (frame->frametype != AST_FRAME_VOICE) {
    if (frame->frametype == AST_FRAME_IMAGE)
      return 0;
    else {
      ast_log(LOG_WARNING, "Can't send %d type frames with SCP write\n", frame->frametype);
      return 0;
    }
  } else {
    if (!(frame->subclass & ast->nativeformats)) {
      ast_log(LOG_WARNING, "Asked to transmit frame type %d, while native formats is %d (read/write = %d/%d)\n",
              frame->subclass, ast->nativeformats, ast->readformat, ast->writeformat);
      return -1;
    }
  }
  if (c) {
    ast_mutex_lock(&c->lock);
    if (c->rtp) {
      res =  ast_rtp_write(c->rtp, frame);
    }
    ast_mutex_unlock(&c->lock);
  }
  return res;
}

static int sccp_pbx_indicate(struct ast_channel *ast, int ind)
{
  sccp_channel_t * c      = ast->pvt->pvt;
  sccp_line_t * l         = c->line;

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Asked to indicate '%d' (%s) condition on channel %s\n", ind, ast_state2str(ind), ast->name);

  switch(ind) {

    case AST_CONTROL_RINGING:
      sccp_dev_set_sptone(l->device, "AlertingTone");
      sccp_dev_statusprompt_set(l->device, ast->pvt->pvt, "Ringing Destination", 0);
      sccp_dev_set_keyset(l->device, ast->pvt->pvt, KEYMODE_PROGRESS);
      sccp_channel_set_callstate(c, TsRingOut);
      sccp_channel_send_callinfo(c);
      break;

    case AST_CONTROL_BUSY:
      sccp_dev_set_sptone(l->device, "LineBusyTone");
      sccp_dev_statusprompt_set(l->device, ast->pvt->pvt, "Destination Busy", 0);
      sccp_channel_set_callstate(c, TsBusy);
      break;

    case AST_CONTROL_CONGESTION:
      sccp_dev_set_sptone(l->device, "ReorderTone");
      sccp_dev_statusprompt_set(l->device, ast->pvt->pvt, "Network Congestion or Error", 0);
      sccp_channel_set_callstate(c, TsCongestion);
      break;

    case -1:
      sccp_dev_statusprompt_set(l->device, ast->pvt->pvt, "Connected", 0);
      sccp_dev_set_keyset(l->device, ast->pvt->pvt, KEYMODE_CONNECTED);
      sccp_channel_set_callstate(c, TsConnected);
      sccp_dev_set_sptone(l->device, "NoTone");
      break;

    default:
      ast_log(LOG_WARNING, "Don't know how to indicate condition %d\n", ind);
      return -1;
  }
  return 0;
}

static int sccp_pbx_fixup(struct ast_channel *oldchan, struct ast_channel *newchan)
{
  sccp_channel_t * c = newchan->pvt->pvt;
  ast_mutex_lock(&c->lock);
  ast_log(LOG_NOTICE, "sccp_pbx_fixup(%s, %s)\n", oldchan->name, newchan->name);
  if (c->owner != oldchan) {
    ast_log(LOG_WARNING, "old channel wasn't %p but was %p\n", oldchan, c->owner);
    ast_mutex_unlock(&c->lock);
    return -1;
  }
  c->owner = newchan;
  ast_mutex_unlock(&c->lock);
  return 0;
}

static int sccp_pbx_senddigit(struct ast_channel *ast, char digit)
{
  // XXX:T: Fixme
  return -1;
}

struct ast_channel *
sccp_new_channel(sccp_channel_t * c, int state)
{
  struct ast_channel * tmp;
  sccp_line_t * l = c->line;
  int fmt;

  ast_log(LOG_DEBUG, "Channels: %p\n", chans);
  tmp = ast_channel_alloc(1);
  ast_log(LOG_DEBUG, "Channels: %p\n", chans);

  if (tmp) {

    tmp->nativeformats = l->device->capability;

    if (!tmp->nativeformats)
      tmp->nativeformats = capability;

    fmt = ast_best_codec(tmp->nativeformats);

    snprintf(tmp->name, sizeof(tmp->name), "SCCP/%s-%08x", l->name, c->callid);
    ast_log(LOG_DEBUG, "PVT: %s\n", tmp->name);

    if (c->rtp)
      tmp->fds[0] = ast_rtp_fd(c->rtp);

    tmp->type = "SCCP";
    ast_setstate(tmp, state);

    if (state == AST_STATE_RING)
      tmp->rings = 1;

    tmp->writeformat         = fmt;
    tmp->readformat          = fmt;

    tmp->pvt->pvt            = c;
    tmp->pvt->rawwriteformat = fmt;
    tmp->pvt->rawreadformat  = fmt;

    tmp->pvt->answer         = sccp_pbx_answer;
    tmp->pvt->hangup         = sccp_pbx_hangup;
    tmp->pvt->call           = sccp_pbx_call;

    tmp->pvt->read           = sccp_pbx_read;
    tmp->pvt->write          = sccp_pbx_write;

    tmp->pvt->indicate       = sccp_pbx_indicate;
    tmp->pvt->fixup          = sccp_pbx_fixup;
    tmp->pvt->send_digit     = sccp_pbx_senddigit;

    // XXX: Bridge?
    // XXX: Transfer?

    c->owner = tmp;

    ast_mutex_lock(&usecnt_lock);
    usecnt++;
    ast_mutex_unlock(&usecnt_lock);

    ast_update_use_count();

    if (l->callerid)
      tmp->callerid = strdup(l->callerid);

    strncpy(tmp->context, l->context, sizeof(tmp->context)-1);

    // tmp->callgroup = l->callgroup;
    // tmp->pickupgroup = l->pickupgroup;
    // strncpy(tmp->call_forward, l->call_forward, sizeof(tmp->call_forward));
    // strncpy(tmp->exten,l->exten, sizeof(tmp->exten)-1);
    // if (strlen(l->callerid))
    //   tmp->callerid = strdup(l->callerid);

    strncpy(tmp->context, l->context, sizeof(tmp->context)-1);
    tmp->priority = 1;

    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "New channel context: %s\n", tmp->context);

  } else {
    ast_log(LOG_WARNING, "Unable to allocate channel structure\n");
  }
  return tmp;
}

void *
sccp_start_channel(void *data)
{
  struct ast_channel * chan = data;
  sccp_channel_t * c = chan->pvt->pvt;
  sccp_line_t * l = c->line;
  char digit;
  int len = 0, res;
  int timeout = firstdigittimeout;
  char exten[AST_MAX_EXTENSION] = "";

  if (sccp_debug)
    ast_verbose( VERBOSE_PREFIX_3 "(1)Starting simple switch on '%s@%s' (%d)\n", l->name, l->device->id, chan->blocking);

  memset(&exten, 0, AST_MAX_EXTENSION);

  sccp_dev_statusprompt_set(l->device, c, "Enter Number", 0);

  while(len < AST_MAX_EXTENSION - 1) {

    digit = ast_waitfordigit(chan, timeout);
    timeout = gendigittimeout;

    if (digit < 0) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_3 "ast_waitfordigit() returned < 0\n");
      ast_indicate(chan, -1);
      ast_hangup(chan);
      return NULL;
    } 

    exten[len++] = digit;
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_1 "Digit: %c (%s)\n", digit, exten);

    if (chan->_state != AST_STATE_DIALING) {
    sccp_dev_set_keyset(l->device, c, KEYMODE_DIALING);
      ast_setstate(chan, AST_STATE_DIALING);
    }

    if (ast_ignore_pattern(chan->context, exten)) {
      sccp_dev_set_sptone(l->device, "OutsideDialTone");
    } else {
      sccp_dev_set_sptone(l->device, "NoTone");
    }

    strncpy(l->device->lastNumber, exten, AST_MAX_EXTENSION);
    l->device->lastNumberLine = l->instance;

    if (ast_exists_extension(chan, chan->context, exten, 1, l->callerid)) {
      if (!digit || !ast_matchmore_extension(chan, chan->context, exten, 1, l->callerid)) {
        strncpy(chan->exten, exten, sizeof(chan->exten)-1);
        ast_setstate(chan, AST_STATE_RING);
        res = ast_pbx_run(chan);

        if (res) {
          ast_log(LOG_WARNING, "PBX exited non-zero\n");
          sccp_dev_statusprompt_set(l->device, c, "PBX Error", 0);
          sccp_dev_set_sptone(l->device, "ReorderTone");
          ast_indicate(chan, AST_CONTROL_CONGESTION);
        } else {
          return NULL;
        }

      } else {

        timeout = matchdigittimeout;

      }

    } else if (!ast_canmatch_extension(chan, chan->context, exten, 1, chan->callerid) && ((exten[0] != '*') || (strlen(exten) > 2))) {

      ast_log(LOG_WARNING, "Can't match [%s] from '%s' in context %s\n", exten, chan->callerid ? chan->callerid : "<Unknown Caller>", chan->context);
      sccp_dev_statusprompt_set(l->device, c, "Invalid Number", 0);
      sccp_dev_set_sptone(l->device, "ReorderTone");
      ast_indicate(chan, AST_CONTROL_CONGESTION);
      break;

    }

  } 

  res = ast_waitfor(chan, -1);
  ast_log(LOG_DEBUG, "ast_waitfor(chan) returned %d\n", res);
  ast_hangup(chan);
  return NULL;
}

