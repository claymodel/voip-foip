/*
 * Asterisk - SCCP Support (By Zozo)
 * ---------------------------------------------------------------------------
 * sccp_device.c - SCCP Device Control
 *
 * $Id: sccp_device.c,v 1.2 2003/11/22 19:18:34 zozo.anlx.net Exp $
 *
 */

#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_channel.h"
#include "sccp_pbx.h"
#include <asterisk/pbx.h>
#include <unistd.h>

static int callCount = 1;
static ast_mutex_t callCountLock = AST_MUTEX_INITIALIZER;

sccp_device_t *
sccp_find_device(const char * name)
{
  sccp_device_t * d = NULL;
  ast_mutex_lock(&devicelock);
  d = devices;
  while (d) {
    if (!strcasecmp(d->id, name)) {
      ast_mutex_unlock(&devicelock);
      return d;
    }
    d++;
  }
  ast_mutex_unlock(&devicelock);
  return NULL;
}

int
sccp_dev_send(sccp_device_t * d, sccp_moo_t * r)
{
  return sccp_session_send(d->session, r);
}

int
sccp_session_send(sccp_session_t * s, sccp_moo_t * r)
{
  int res = 1;

  if (!s || s->fd <= 0) {
    ast_log(LOG_ERROR, "Tried to send packet over DOWN device.\n");
    free(r);
    return -1;
  }

  ast_mutex_lock(&s->lock);

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_2 "Sending Packet Type %s (%d bytes)\n", sccpmsg2str(r->messageId), r->length);

  res = write(s->fd, r, r->length + 8);

  if (res != r->length+8) {
    ast_log(LOG_WARNING, "Only managed to send %d bytes (out of %d): %s\n", res, r->length+8, strerror(errno));
    res = 0;
  }

  ast_mutex_unlock(&s->lock);
  free(r);
  return res;
}

void
sccp_session_sendmsg(sccp_session_t * s, sccp_message_t t)
{
  sccp_moo_t * r;
  REQCMD(r, t);
  sccp_session_send(s, r);
}

void
sccp_dev_sendmsg(sccp_device_t * d, sccp_message_t t)
{
  sccp_moo_t * r;
  REQCMD(r, t);
  sccp_dev_send(d, r);
}

void
sccp_dev_set_registered(sccp_device_t * d, int opt)
{
  int tmp;

  if (d->registrationState == opt)
    return;

  d->registrationState = opt;
  if (opt == RsOK) {
    tmp = d->currentKeySet;
    d->currentKeySet = -1;
    sccp_dev_set_keyset(d, NULL, tmp);
  }
  sccp_dev_statusprompt_set(d, NULL, "Registered with Asterisk PBX", 5);
}

sccp_channel_t *
sccp_get_active_channel(sccp_device_t * d)
{
  // XXX:T: Take active Line into account, here.
  return d->active_channel;
}

/* Sets device 'd' info the SoftKey mode specified by 'opt' */
// XXX:T: this should probably move to channel, not device.
void
sccp_dev_set_keyset(sccp_device_t * d, sccp_channel_t * c, int opt)
{
  sccp_moo_t * r;

  if (!d->session)
    return;

  if (d->currentKeySet == opt && d->currentKeySetLine == ((c) ? c->line->instance : 0))
    return;

  d->currentKeySet = opt;
  d->currentKeySetLine = (c) ? c->line->instance : 0;
  
  if (d->registrationState != RsOK) 
    return;

  REQ(r, SelectSoftKeysMessage)

  if (c) {
    r->msg.SelectSoftKeysMessage.lineInstance  = c->line->instance;
    r->msg.SelectSoftKeysMessage.callReference = c->callid;
  }

  r->msg.SelectSoftKeysMessage.softKeySetIndex = opt;

  // XXX:T: We might not want all buttons to be active?
  r->msg.SelectSoftKeysMessage.validKeyMask1 = 127;
  r->msg.SelectSoftKeysMessage.validKeyMask2 = 127;

  switch (opt) {
    case KEYMODE_ONHOOK:
      if (strlen(d->lastNumber) == 0)
        r->msg.SelectSoftKeysMessage.validKeyMask1 &= ~(1<<0);
  }

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "{SelectSoftKeysMessage} lineInstance=%d callReference=%d softKeySetIndex=%d validKeyMask=%d/%d\n",
      r->msg.SelectSoftKeysMessage.lineInstance,
      r->msg.SelectSoftKeysMessage.callReference,
      r->msg.SelectSoftKeysMessage.softKeySetIndex,
      r->msg.SelectSoftKeysMessage.validKeyMask1,
      r->msg.SelectSoftKeysMessage.validKeyMask2);

  sccp_dev_send(d, r);


  d->keyset = opt;
}


/* Turns the lamp on the handset of device 'd' into mode 'opt' */
// XXX:T: prehaps though should be sccp_line_set_mwi(l, 1) ?
void
sccp_dev_set_mwi(sccp_device_t * d, int line, int hasMail)
{
  sccp_moo_t * r;

  if (!d->session)
    return;

  REQ(r, SetLampMessage)
  r->msg.SetLampMessage.stimulus = 0xF; // 0xF == "SsVoiceMail"
  r->msg.SetLampMessage.stimulusInstance = line;
  r->msg.SetLampMessage.lampMode = (hasMail) ? 2 : 1;
  sccp_dev_send(d, r);

  // XXX:T Also set stimulusInstance to 0 to make the lamp
  // XXX:T on the handset flash.
}

/* Sets the ringermode on device 'd' to 'opt' */

void
sccp_dev_set_ringer(sccp_device_t * d, int opt)
{
  sccp_moo_t * r;

  if (!d->session)
    return;

  if (d->ringermode == opt)
    return;

  REQ(r, SetRingerMessage)
  r->msg.SetRingerMessage.ringMode = opt;
  sccp_dev_send(d, r);

  d->ringermode = opt;
}

void
sccp_dev_set_speaker(sccp_device_t * d, StationSpeakerMode mode)
{
  sccp_moo_t * r;
  if (!d->session)
    return;
  REQ(r, SetSpeakerModeMessage)
  r->msg.SetSpeakerModeMessage.speakerMode = mode;
  ast_verbose(VERBOSE_PREFIX_2 "{SetSpeakerModeMessage} speakerMode=%d\n", mode);
  sccp_dev_send(d, r);
}

void
sccp_dev_set_microphone(sccp_device_t * d, StationMicrophoneMode opt)
{
  sccp_moo_t * r;
  if (!d->session)
    return;
  REQ(r, SetMicroModeMessage)
  r->msg.SetMicroModeMessage.micMode = opt;
  ast_verbose(VERBOSE_PREFIX_2 "{SetMicroModeMessage} micMode=%d\n", opt);
  sccp_dev_send(d, r);
}

void
sccp_dev_set_cplane(sccp_device_t * d, int status, int line)
{

  if (!d->session)
    return;

  if (!status) {

    sccp_dev_sendmsg(d, DeactivateCallPlaneMessage);

  } else {

    sccp_moo_t * r;
    REQ(r, ActivateCallPlaneMessage)
    r->msg.ActivateCallPlaneMessage.lineInstance = line;
    sccp_dev_send(d, r);

  }

}

void sccp_dev_set_sptone_byid(sccp_device_t * d, int tone)
{
  sccp_moo_t * r;

  if (!d->session)
    return;

  if (d->currentTone == tone) {
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "Current tone (%d) is equiv to wanted tone (%d).  Ignoring.\n", d->currentTone, tone);
    return;
  }

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Sending tone %d\n", tone);

  if (tone > 0) {
    REQ(r, StartToneMessage)
    r->msg.StartToneMessage.tone = tone;
  } else {
    REQ(r, StopToneMessage)
  }

  sccp_dev_send(d, r);
  d->currentTone = tone;

}

void
sccp_dev_set_sptone(sccp_device_t * d, char * tstr)
{
  const value_string * v = tone_list;

  if (!d->session)
    return;

  if (tstr) {
    while (v->value) {
      if (!strcasecmp(v->value, tstr)) {
        sccp_dev_set_sptone_byid(d, v->key);
        return;
      }
      v++;
    }
  }
  sccp_dev_set_sptone_byid(d, 0);
}

void
sccp_dev_remove_channel(sccp_channel_t * c)
{
  sccp_channel_t * prev = NULL, * cur;

  ast_mutex_lock(&chanlock);

  cur = chans;

  while (cur) {
    if (cur == c)
      break;
    prev = cur;
    cur = cur->lnext;
  }

  if (cur) {

    if (prev) {
      prev->lnext = cur->lnext;
    } else {
      chans = cur->lnext;
    }

  } else {

    ast_log(LOG_WARNING, "Couldn't find channel to remove()\n");
    ast_mutex_unlock(&chanlock);
    return;

  }

  // XXX:T: Fixme!
  c->line->device->active_channel = NULL;
  c->line->channels      = NULL;
  c->line->activeChannel = NULL;
  c->line                = NULL;
  c->owner               = NULL;

  ast_log(LOG_DEBUG, "Removed channel from line.\n");

  free(c);

  ast_mutex_unlock(&chanlock);
  return;
}

/* Needs to be called with d->lock */
sccp_channel_t *
sccp_dev_allocate_channel(sccp_device_t * d, sccp_line_t * l, int outgoing, char * dial)
{
  sccp_channel_t * c = NULL;
  struct ast_channel * tmp = NULL;
  pthread_t t;
  int callId;

  if (!d->session) {
    ast_log(LOG_ERROR, "Tried to open channel on device without a session\n");
    return NULL;
  }

  // If there is no current line, then we can't make a call in, or out.
  if (!d->currentLine) {
    ast_log(LOG_ERROR, "Tried to open channel on a device with no selected line\n");
    return NULL;
  }

  if (l == NULL)
    l = d->currentLine;

  ast_mutex_lock(&callCountLock);
  callId = callCount++;
  ast_mutex_unlock(&callCountLock);

  c = malloc(sizeof(sccp_channel_t));
  memset(c, 0, sizeof(sccp_channel_t));
  c->callid = callId;
  c->line = l;

  ast_mutex_lock(&l->lock);
  l->channelCount++;
  ast_mutex_unlock(&l->lock);

  ast_log(LOG_DEBUG, "After: #Channel ->lnext = %p, c = %p, channels = %p\n", c->lnext, c, chans);
  tmp = sccp_new_channel(c, AST_STATE_OFFHOOK);
  ast_log(LOG_DEBUG, "New channel name is: %s\n", tmp->name);
  ast_log(LOG_DEBUG, "After: #Channel ->lnext = %p, c = %p, channels = %p\n", c->lnext, c, chans);

  ast_mutex_lock(&chanlock);
    c->lnext = chans;
    chans = c;
  ast_mutex_unlock(&chanlock);

  c->owner = tmp;

  c->next = l->channels;
  l->channels = c;
  l->activeChannel = c;

  if (outgoing) {

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    c->isOutgoing = 1;
    d->active_channel = c;

    ast_log(LOG_DEBUG, "After: #Channel ->lnext = %p, c = %p, channels = %p\n", c->lnext, c, chans);

    sccp_dev_set_speaker(d, StationSpeakerOn);
    sccp_channel_set_callstate(c, TsOffHook);
    sccp_dev_statusprompt_set(d, c, NULL, 0);
    sccp_dev_set_keyset(d, c, KEYMODE_OFFHOOK);
    sccp_dev_set_sptone(d, "InsideDialTone");

    if (dial) {

      strncpy(tmp->exten, dial, AST_MAX_EXTENSION);
      if (ast_pbx_start(tmp)) {
        ast_log(LOG_WARNING, "PBX exited non-zero\n");
        sccp_dev_statusprompt_set(l->device, c, "PBX Error", 10);
        sccp_dev_set_sptone(l->device, "ReorderTone");
        ast_indicate(tmp, AST_CONTROL_CONGESTION);
      }

  ast_log(LOG_DEBUG, "After: #Channel ->lnext = %p, c = %p, channels = %p\n", c->lnext, c, chans);

    } else if (pthread_create(&t, &attr, sccp_start_channel, tmp)) {
      ast_log(LOG_WARNING, "Unable to create switch thread: %s\n", strerror(errno));
      ast_hangup(tmp);
      free(c);
      return NULL;
    }

  } else {

    // It's an incoming call.

  }

  ast_log(LOG_DEBUG, "After: #Channel ->lnext = %p, c = %p, chans = %p\n", c->lnext, c, chans);

  return c;
}

void
sccp_dev_cleardisplay(sccp_device_t * d)
{
  sccp_dev_sendmsg(d, ClearDisplay);
}

sccp_channel_t *
sccp_device_active_channel(sccp_device_t * d)
{
  return d->active_channel;
}

void
sccp_dev_statusprompt_set(sccp_device_t * d, sccp_channel_t * c, char * msg, int timeout)
{
  sccp_moo_t * r;

  if (!d->session)
    return;

  if (msg == NULL) {

    REQ(r, ClearPromptStatusMessage)
    r->msg.ClearPromptStatusMessage.callReference = (c) ? c->callid : 0;
    r->msg.ClearPromptStatusMessage.lineInstance  = (c) ? c->line->instance : 0;

  } else {

    REQ(r, DisplayPromptStatusMessage)
    r->msg.DisplayPromptStatusMessage.messageTimeout = timeout;
    r->msg.DisplayPromptStatusMessage.callReference = (c) ? c->callid : 0;
    r->msg.DisplayPromptStatusMessage.lineInstance  = (c) ? c->line->instance : 0;
    strncpy(r->msg.DisplayPromptStatusMessage.promptMessage, msg, sizeof(r->msg.DisplayPromptStatusMessage.promptMessage) - 1);

  }

  sccp_dev_send(d, r);
}

sccp_speed_t *
sccp_dev_speed_find_byindex(sccp_device_t * d, int ind)
{
  sccp_speed_t * k = d->speed_dials;
  if (!k)
    return NULL;

  while (k && k->index != ind)
    k = k->next;

  return k;
}

int
sccp_dev_attach_line(sccp_device_t * d, sccp_line_t * l)
{

  if (!l) {
    ast_log(LOG_ERROR, "Attempted to add a NULL line to device %s\n", d->id);
    return 0;
  }

  if (l->device) {
    ast_log(LOG_WARNING, "Line %s is already logged in elsewhere.\n", l->name);
    return 0;
  }

  ast_log(LOG_DEBUG, "Attaching line %s to device %s\n", l->name, d->id);

  l->device = d;
  l->instance = -1;
  // l->dnState = DsIdle;
  l->next = d->lines;
  d->lines = l;

  return 1;
}

sccp_device_t *
sccp_dev_find_byid(char * name)
{
  sccp_device_t * d = devices;

  ast_mutex_lock(&devicelock);
  while (d) {
    if (!strcasecmp(d->id, name))
      break;
    d = d->next;
  }
  ast_mutex_unlock(&devicelock);
  return d;
}

sccp_line_t *
sccp_dev_get_activeline(sccp_device_t * d)
{
  return d->currentLine;
}

void
sccp_dev_check_mwi(sccp_device_t * d)
{
  sccp_line_t * l;
  int hasMessages = 0;

  ast_mutex_lock(&d->lock);
  l = d->lines;
  while (l) {
    if (sccp_line_hasmessages(l)) {
      sccp_dev_set_mwi(d, l->instance, 1);
      l->hasMessages = 1;
      hasMessages = 1;
      break;
    } else if (l->hasMessages) {
      sccp_dev_set_mwi(d, l->instance, 0);
      l->hasMessages = 0;
    }
    l = l->next;
  }

  if (hasMessages != d->hasMessages) {
    sccp_dev_set_mwi(d, 0, hasMessages);
    d->hasMessages = hasMessages;
  }

  ast_mutex_unlock(&d->lock);
}

void
sccp_device_select_line(sccp_device_t * d, sccp_line_t * wanted)
{
  sccp_line_t * current;
  sccp_channel_t * chan = NULL;

  current = sccp_dev_get_activeline(d);

  if (!current || wanted->device != d || current == wanted)
    return;

  // If the current line isn't in a call, and
  // neither is the target.
  if (current->channels == NULL && wanted->channels == NULL) {

    ast_log(LOG_DEBUG, "All lines seem to be inactive, SEIZEing selected line %s\n", wanted->name);
    d->currentLine = wanted;

    chan = sccp_dev_allocate_channel(d, NULL, 1, NULL);
    if (!chan) {
      ast_log(LOG_ERROR, "Failed to allocate SCCP channel.\n");
      return;
    }

  }

  // If the device is currently onhook, then we need to ...
  else if ( current->dnState > TsOnHook || wanted->dnState == TsOffHook) {

    ast_log(LOG_DEBUG, "Selecing line %s while using line %s\n", wanted->name, current->name);
    // (1) Put current call on hold
    // (2) Stop transmitting/recievening

  } else {

    // Otherwise, just select the callplane
    ast_log(LOG_WARNING, "Unknown status while trying to select line %s.  Current line is %s\n", wanted->name, current->name);

  }

}
