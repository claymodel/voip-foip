/*
 *
 * SCCP support for Asterisk.
 *   -- By Zozo
 */

#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include "sccp_sched.h"

void
sccp_handle_alarm(sccp_session_t * s, sccp_moo_t * r)
{
  ast_log(LOG_NOTICE, "Alarm Message: Severity: %d, %s [%d/%d]\n", 
                      r->msg.AlarmMessage.alarmSeverity,
                      r->msg.AlarmMessage.text,
                      r->msg.AlarmMessage.parm1,
                      r->msg.AlarmMessage.parm2);
}

void
sccp_handle_register(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_device_t * d;
  const value_string * v = device_types;
  char *mb, *cur, tmp[256];
  sccp_line_t * l;
  sccp_moo_t * r1;

  while(v->value) {
    if (v->key == r->msg.RegisterMessage.deviceType)
      break;
    v++;
  }

  ast_log(LOG_DEBUG, "Trying to register device %s, Instance: %d, Type: %s, Version: %d\n", 
                      r->msg.RegisterMessage.sId.deviceName, 
                      r->msg.RegisterMessage.sId.instance, 
                      (v) ? v->value : "Unknown",
                      (int)r->msg.RegisterMessage.protocolVer);

  ast_mutex_lock(&devicelock);

  d = devices;

  while (d) {
    ast_mutex_lock(&d->lock);
    if (!strcasecmp(r->msg.RegisterMessage.sId.deviceName, d->id)) {
      if (d->session) {
        // We still have an old session here.
        ast_log(LOG_WARNING, "Device %s tried to login while old session still active.\n", d->id);
        ast_mutex_unlock(&d->lock);
        d = NULL;
        break;
      }
      ast_log(LOG_DEBUG, "Allocating Device %p to session %p\n", d, s);
      s->device = d;
      d->type = r->msg.RegisterMessage.deviceType;
      d->session = s;
      ast_mutex_unlock(&d->lock);
      break;
    }
    ast_mutex_unlock(&d->lock);
    d = d->next;
  }

  ast_mutex_unlock(&devicelock);

  if (!d) {
    REQ(r1, RegisterRejectMessage);
    ast_log(LOG_ERROR, "Rejecting Device %s: Device not found\n", r1->msg.RegisterMessage.sId.deviceName);
    strncpy(r1->msg.RegisterRejectMessage.text, "Unknown Device", StationMaxDisplayTextSize);
    sccp_session_send(s, r1);
    return;
  }


  strncpy(tmp, d->autologin, sizeof(tmp));
  mb = tmp;
  while((cur = strsep(&mb, ","))) {
    if (strlen(cur)) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_1 "Auto logging into %s\n", cur);
      l = sccp_line_find_byname(cur);
      if (l)
        sccp_dev_attach_line(d, l);
      else
        ast_log(LOG_ERROR, "Failed to autolog %s into %s: Couldn't find line %s\n", d->id, cur, cur);
    }
  }

  sccp_dev_set_registered(d, RsProgress);
  d->currentLine = d->lines;

  REQ(r1, RegisterAckMessage);
  r1->msg.RegisterAckMessage.protocolVer = 0x3;
  r1->msg.RegisterAckMessage.keepAliveInterval = keepalive;
  r1->msg.RegisterAckMessage.secondaryKeepAliveInterval = keepalive;
  strncpy(r1->msg.RegisterAckMessage.dateTemplate, date_format, StationDateTemplateSize);
  sccp_dev_send(d, r1);

  /* 
      sccp_dev_check_mwi(s->device);
      if (s->device->dnd && (time(0) + 5) > s->device->lastCallEndTime)
        sccp_dev_statusprompt_set(s->device, NULL, "DND is Enabled", 0);
  */

  // We pretty much know what each device, but hey - lets do it for the heck of it anyway :)
  sccp_dev_sendmsg(d, CapabilitiesReqMessage);

}


// Handle the configuration of the device here.

void
sccp_handle_button_template_req(sccp_session_t * s, sccp_moo_t * r)
{
  int b = 0, i;
  sccp_moo_t * r1;
  const btnlist * list;
  sccp_device_t * d = s->device;
  sccp_speed_t * k = d->speed_dials;
  sccp_line_t * l  = d->lines;
  int lineInstance = 1, speedInstance = 1;

  ast_mutex_lock(&devicelock);
  ast_mutex_lock(&linelock);

  REQ(r1, ButtonTemplateMessage)
  r1->msg.ButtonTemplateMessage.buttonOffset = 0;

  for (i = 0; i < StationMaxButtonTemplateSize ; i++) {
    r1->msg.ButtonTemplateMessage.definition[i].instanceNumber   = 1;
    r1->msg.ButtonTemplateMessage.definition[i].buttonDefinition = 0xFF;
  }

  if (!d->buttonSet) {
    ast_log(LOG_WARNING, "Don't have a button layout, sending blank template.\n");
    sccp_dev_send(s->device, r1);
    ast_mutex_unlock(&linelock);
    ast_mutex_unlock(&devicelock);
    return;
  }

  list = d->buttonSet->buttons;

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "Configuring button template. buttonOffset=%d, buttonCount=%d, totalButtonCount=%d\n", 
      0, d->buttonSet->buttonCount, d->buttonSet->buttonCount);

  r1->msg.ButtonTemplateMessage.buttonCount = d->buttonSet->buttonCount;
  r1->msg.ButtonTemplateMessage.totalButtonCount = d->buttonSet->buttonCount;

  for (i = 0 ; i < d->buttonSet->buttonCount ; i++) {
    int inst = 1;

    r1->msg.ButtonTemplateMessage.definition[i].buttonDefinition = list->type;

    switch (list->type) {
      case BtLine:
        inst = lineInstance++;
        while (l) {
          if (l->device == s->device) {
            ast_log(LOG_DEBUG, "Line[%.2d] = LINE(%s)\n", b+1, l->name);
            l->instance = inst;
            l->dnState = TsOnHook;
            l = l->next;
            break;
          }
          l = l->next;
        }
        break;

      case BtSpeedDial:
        inst = speedInstance++;
        break;
      default:
        break;
    }

    r1->msg.ButtonTemplateMessage.definition[i].instanceNumber = inst;

    if (sccp_debug >= 3)
      ast_verbose(VERBOSE_PREFIX_3 "%d %X\n", inst, list->type);
    list++;
  }

  ast_mutex_unlock(&linelock);
  ast_mutex_unlock(&devicelock);
  sccp_dev_send(s->device, r1);
  sccp_dev_set_keyset(s->device, NULL, KEYMODE_ONHOOK);

  return;

  /*
  ast_log(LOG_DEBUG, "Configuring buttons for %s\n", s->device->id);
  */

  // If there are no lines, we don't add speed dials, either, as a device with
  // no lines but speed dials seems to break 3.3(4.1).  Makes sense, I guess anyway.

  if (b == 0) {
    ast_log(LOG_DEBUG, "No lines found, sending empty ButtonTemplate\n");
    return;
  }


  while (k) {
    ast_log(LOG_DEBUG, "Line[%.2d] = SPEEDDIAL(%s)\n", b+1, k->name);
    r1->msg.ButtonTemplateMessage.definition[b].instanceNumber   = (b + 1);
    r1->msg.ButtonTemplateMessage.definition[b].buttonDefinition = BtSpeedDial;
    k->index = (b + 1);
    b++;
    k = k->next;
  }


  ast_log(LOG_DEBUG, "buttonCount: %d\n", r1->msg.ButtonTemplateMessage.buttonCount);

  sccp_dev_send(s->device, r1);
}

void
sccp_handle_line_number(sccp_session_t * s, sccp_moo_t * r)
{
  int lineNumber = r->msg.LineStatReqMessage.lineNumber;
  sccp_line_t * llines;
  sccp_moo_t * r1;

  ast_log(LOG_DEBUG, "Configuring line number %d for device %s\n", lineNumber, s->device->id);

  ast_mutex_lock(&devicelock);
  llines = s->device->lines;
  while (llines) {
    if (llines->instance == lineNumber)
      break;
    llines = llines->next;
  }
  ast_mutex_unlock(&devicelock);

  REQ(r1, LineStatMessage)

  r1->msg.LineStatMessage.lineNumber = lineNumber;

  if (llines == NULL) {
    ast_log(LOG_ERROR, "SCCP device %s requested a line configuration for unknown line %d\n", s->device->id, lineNumber);
    // XXX:T: Maybe we should return something here to tell the thing to fuck off and die?
    sccp_dev_send(s->device, r1);
    return;
  }

  memcpy(r1->msg.LineStatMessage.lineDirNumber, llines->name, StationMaxDirnumSize);
  memcpy(r1->msg.LineStatMessage.lineFullyQualifiedDisplayName, llines->label, StationMaxNameSize);
  sccp_dev_send(s->device, r1);
}

void
sccp_handle_speed_dial_stat_req(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_speed_t * k = s->device->speed_dials;
  sccp_moo_t * r1;
  int wanted = r->msg.SpeedDialStatReqMessage.speedDialNumber;

  if (sccp_debug >= 3)
    ast_verbose(VERBOSE_PREFIX_3 "Speed Dial Request for Button %d\n", wanted);

  REQ(r1, SpeedDialStatMessage)
  r1->msg.SpeedDialStatMessage.speedDialNumber = wanted;

  if ((k = sccp_dev_speed_find_byindex(s->device, wanted)) != NULL) {
    strncpy(r1->msg.SpeedDialStatMessage.speedDialDirNumber, k->ext, StationMaxDirnumSize);
    strncpy(r1->msg.SpeedDialStatMessage.speedDialDisplayName, k->name, StationMaxNameSize);
  }

  sccp_dev_send(s->device, r1);
}

void
sccp_handle_stimulus(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_line_t * l;
  const value_string * v = deviceStimuli;
  int stimulus = r->msg.StimulusMessage.stimulus;
  int stimulusInstance = r->msg.StimulusMessage.stimulusInstance;

  while (v->value) {
    if (v->key == stimulus)
      break;
    v++;
  }

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Got {StimulusMessage} stimulus=%s(%d) stimulusInstance=%d\n", (v) ? v->value : "Unknown!", stimulus, stimulusInstance);

  switch (stimulus) {

    case BtLastNumberRedial:
      if (s->device->lastNumberLine) {
        ast_log(LOG_DEBUG, "Redialing %s, on lineInstance %d\n", s->device->lastNumber, s->device->lastNumberLine);
        sccp_line_dial(sccp_line_find_byid(s->device, s->device->lastNumberLine), s->device->lastNumber);
      } else {
        ast_log(LOG_DEBUG, "Can't redial without there being a number to redial!\n");
      }
      break;

    case BtLine:
      l = sccp_line_find_byid(s->device, stimulusInstance);
      sccp_device_select_line(s->device, l);
      break;

    default:
      ast_log(LOG_NOTICE, "Don't know how to deal with stimulus %d\n", stimulus);
      break;

  }
}

void
sccp_handle_offhook(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_channel_t * chan;
  sccp_line_t * l;

  if (!s->device->lines) {
    ast_log(LOG_NOTICE, "No lines registered on %s for take OffHook\n", s->device->id);
    sccp_dev_statusprompt_set(s->device, NULL, "No lines registered!", 0);
    sccp_dev_set_sptone(s->device, "BeepBonk");
    return;
  }

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Device d=%p s=%p s->d->s=%p Taken Offhook\n", s->device, s, s->device->session);

  // Check if there is a call comming in on our currently selected line.
  l = sccp_dev_get_activeline(s->device);

  chan = l->channels;
  while (chan) {
    if (chan->isRinging)
      break;
    chan = chan->next;
  }

  // s->device->currentLine->dnState = DsSeize;

  if (chan) {

    // Answer the ringing channel.
    ast_log(LOG_DEBUG, "Anwered Ringing Channel\n");
    s->device->active_channel = chan;
    sccp_dev_set_ringer(s->device, StationRingOff);
    chan->isRinging = 0;
    sccp_dev_set_keyset(s->device, chan, KEYMODE_CONNECTED);
    sccp_dev_set_speaker(l->device, StationSpeakerOn);
    ast_queue_control(chan->owner, AST_CONTROL_ANSWER, 0);
    sccp_channel_set_callstate(chan, TsOffHook);    
    sccp_channel_set_callstate(chan, TsConnected);    
    start_rtp(chan);
    ast_setstate(chan->owner, AST_STATE_UP);

  } else {

    if (s->device->currentLine->channels)
      chan = s->device->currentLine->channels;
    else
      chan = sccp_dev_allocate_channel(s->device, s->device->currentLine, 1, NULL);

    if (!chan) {
      ast_log(LOG_ERROR, "Failed to allocate SCCP channel.\n");
      return;
    }

  }

}

void
sccp_handle_onhook(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_channel_t * c;

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Put Onhook\n");

  if (!s->device->lines) {
    ast_log(LOG_NOTICE, "No lines registered on %s to put OnHook\n", s->device->id);
    sccp_dev_set_sptone(s->device, "NoTone");
    return;
  }

  // get the active channel
  c = sccp_device_active_channel(s->device);

  if (!c) {
    ast_log(LOG_ERROR, "Erp, tried to hangup when we didn't have an active channel?!\n");
    return;
  }

  if (!c->line) 
    ast_log(LOG_NOTICE, "Channel didn't have a parent on OnHook - Huuu?!\n");

  sccp_channel_endcall(c);

  return;
}

void
sccp_handle_headset(sccp_session_t * s, sccp_moo_t * r)
{
  // XXX:T: What should we do here?
}

void
sccp_handle_capabilities_res(sccp_session_t * s, sccp_moo_t * r)
{
  const value_string * v = codec_list;
  int i;
  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_1 "Device has %d Capabilities\n", r->msg.CapabilitiesResMessage.count);
  for (i = 0 ; i < r->msg.CapabilitiesResMessage.count ; i++) {
    v = codec_list;
    while (v->value) {
      if (v->key == r->msg.CapabilitiesResMessage.caps[i].payloadCapability)
        break;
      v++;
    }
    // XXX:T: Handle this - Probably need an asterisk -> sccp codec map.
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "CODEC: %d - %s\n", r->msg.CapabilitiesResMessage.caps[i].payloadCapability, (v) ? v->value : "Unknown");
  }
}


void
sccp_handle_soft_key_template_req(sccp_session_t * s, sccp_moo_t * r)
{
  int i = 0;
  const softkeytypes * v = button_labels;
  sccp_moo_t * r1;

  REQ(r1, SoftKeyTemplateResMessage)
  r1->msg.SoftKeyTemplateResMessage.softKeyOffset = 0;

  while (v->id) {
    ast_log(LOG_DEBUG, "Button(%d)[%2d] = %s\n", i, v->id, v->txt);
    strncpy(r1->msg.SoftKeyTemplateResMessage.definition[i].softKeyLabel, v->txt, 15);
    r1->msg.SoftKeyTemplateResMessage.definition[i].softKeyEvent = v->id;
    i++;
    v++;
  }

  r1->msg.SoftKeyTemplateResMessage.softKeyCount = (i+1);
  r1->msg.SoftKeyTemplateResMessage.totalSoftKeyCount = (i+1);
  sccp_dev_send(s->device, r1);
}

/*
 * XXX:T: This code currently only allows for a maximum of 32 SoftKeySets. We can
 *        extend this to up to 255, by using the offset field.
 */

void
sccp_handle_soft_key_set_req(sccp_session_t * s, sccp_moo_t * r)
{
  const softkey_modes * v = SoftKeyModes;
  int iKeySetCount = 0;
  sccp_moo_t * r1;

  REQ(r1, SoftKeySetResMessage)
  r1->msg.SoftKeySetResMessage.softKeySetOffset = 0;

  while (v && v->ptr) {
    const btndef * b = v->ptr;
    int c = 0;

    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "Set[%d] = ", v->setId);

    while (b && b->labelId != 0 ) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_1 "%d:%d ", c, b->labelId);
      if (b->labelId != -1)
        r1->msg.SoftKeySetResMessage.definition[v->setId].softKeyTemplateIndex[c] = b->labelId;
      c++;
      b++;
    }

    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "\n");
    v++;
    iKeySetCount++;
  };

  if (sccp_debug)
    ast_verbose( VERBOSE_PREFIX_3 "There are %d SoftKeySets.\n", iKeySetCount);
  
  r1->msg.SoftKeySetResMessage.softKeySetCount       = iKeySetCount;
  r1->msg.SoftKeySetResMessage.totalSoftKeySetCount  = iKeySetCount; // <<-- for now, but should be: iTotalKeySetCount;

  sccp_dev_send(s->device, r1);
}

void
sccp_handle_time_date_req(sccp_session_t * s, sccp_moo_t * req)
{
  time_t timer = time(0);
  struct tm * cmtime = localtime(&timer);
  sccp_moo_t * r1;
  REQ(r1, DefineTimeDate)
  r1->msg.DefineTimeDate.year      = cmtime->tm_year+1900;
  r1->msg.DefineTimeDate.month     = cmtime->tm_mon+1;
  r1->msg.DefineTimeDate.dayOfWeek = cmtime->tm_wday;
  r1->msg.DefineTimeDate.day       = cmtime->tm_mday;
  r1->msg.DefineTimeDate.hour      = cmtime->tm_hour;
  r1->msg.DefineTimeDate.minute    = cmtime->tm_min;
  r1->msg.DefineTimeDate.seconds   = cmtime->tm_sec;
  sccp_dev_send(s->device, r1);
}

void
sccp_handle_keypad_button(sccp_session_t * s, sccp_moo_t * r)
{
  struct ast_frame f = { AST_FRAME_DTMF, };
  int event = r->msg.KeypadButtonMessage.kpButton;
  char resp;

  sccp_channel_t * c = sccp_get_active_channel(s->device);

  if (!c) {
    ast_log(LOG_ERROR, "Device %s sent a Keypress, but there is no active channel!\n", s->device->id);
    return;
  }

  printf("Cisco Digit: %08x (%d)\n", event, event);

  // XXX:T: # working?
  if (event < 10)
    resp = '0' + event;
  else if (event == 14)
    resp = '*';
  else if (event == 15)
    resp = '#';
  else
    resp = '?';

  f.subclass = resp;

  ast_mutex_lock(&c->lock);
  ast_queue_frame(c->owner, &f, 0);
  ast_mutex_unlock(&c->lock);
}


void
sccp_handle_soft_key_event(sccp_session_t * s, sccp_moo_t * r)
{
  const softkeytypes * b = button_labels;
  sccp_channel_t * c = NULL;
  sccp_line_t * l = NULL;

  ast_log(LOG_DEBUG, "Got Softkey: keyEvent=%d lineInstance=%d callReference=%d\n",
    r->msg.SoftKeyEventMessage.softKeyEvent,
    r->msg.SoftKeyEventMessage.lineInstance,
    r->msg.SoftKeyEventMessage.callReference);

  while (b->id) {
    if (b->id == r->msg.SoftKeyEventMessage.softKeyEvent)
      break;
    b++;
  }

  if (!b->func) {
    ast_log(LOG_WARNING, "Don't know how to handle keypress %d\n", r->msg.SoftKeyEventMessage.softKeyEvent);
    return;
  }

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "Softkey %s (%d) has been pressed.\n", b->txt, b->id);

  if (r->msg.SoftKeyEventMessage.lineInstance)
    l = sccp_line_find_byid(s->device, r->msg.SoftKeyEventMessage.lineInstance);

  if (r->msg.SoftKeyEventMessage.callReference) {
    c = sccp_channel_find_byid(r->msg.SoftKeyEventMessage.callReference);
    if (c) {
      if (c->line->device != s->device)
        c = NULL;
    }
  }

  if (sccp_debug >= 3)
    ast_verbose(VERBOSE_PREFIX_3 "Calling func()\n");

  b->func(s->device, l, c);

  if (sccp_debug >= 3)
    ast_verbose(VERBOSE_PREFIX_3 "Returned from func()\n");

}

void
sccp_handle_open_recieve_channel_ack(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_channel_t * c = sccp_get_active_channel(s->device);
  struct sockaddr_in sin;
  struct in_addr in;

  in.s_addr = r->msg.OpenReceiveChannelAck.ipAddr;

  ast_log(LOG_DEBUG, "Got OpenChannel ACK.  Status: %d, RemoteIP: %s, Port: %d, PassThruId: %d\n", 
    r->msg.OpenReceiveChannelAck.orcStatus,
    inet_ntoa(in),
    r->msg.OpenReceiveChannelAck.portNumber,
    r->msg.OpenReceiveChannelAck.passThruPartyId
  );

  if (!c) {
    ast_log(LOG_ERROR, "Device %s sent OpenChannelAck, but there is no active channel!\n", s->device->id);
    return;
  }

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = r->msg.OpenReceiveChannelAck.ipAddr;
  sin.sin_port = htons(r->msg.OpenReceiveChannelAck.portNumber);

  if (c->rtp && sin.sin_port)
    ast_rtp_set_peer(c->rtp, &sin);

  printf("Peer RTP is at port %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

}

void
sccp_handle_version(sccp_session_t * s, sccp_moo_t * r)
{
  sccp_moo_t * r1;
  char * verStr = sccp_helper_getversionfor(s);

  REQ(r1, VersionMessage)
  strncpy(r1->msg.VersionMessage.requiredVersion, verStr, (StationMaxVersionSize-1));
  sccp_dev_send(s->device, r1);
}
