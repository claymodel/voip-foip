#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include <asterisk/callerid.h>

sccp_channel_t *
sccp_channel_find_byid(int id)
{
  sccp_channel_t * c = NULL;
  ast_mutex_lock(&chanlock);
  c = chans;
  while (c) {
    if (c->callid == id)
      break;
    c = c->lnext;
  }
  ast_mutex_unlock(&chanlock);
  return c;
}

void
sccp_channel_send_callinfo(sccp_channel_t * c)
{
  sccp_moo_t * r;
  char * name, * number;
  char tmp[256] = "";

  if (!c->owner) {
    ast_log(LOG_ERROR, "Call doesn't have an owner!\n");
    return;
  }

  if (c->isOutgoing) {

    if (c->line->callerid)
      strncpy(tmp, c->line->callerid, 253);
    else
      ast_log(LOG_WARNING, "Outgoing calls %s doesn't have CallerId\n", c->owner->name);

  } else {

    if (c->owner->callerid)
      strncpy(tmp, c->owner->callerid, 253);
    else
      ast_log(LOG_WARNING, "Incoming call %s doesn't have CallerId\n", c->owner->name);

  }
  ast_callerid_parse(tmp, &name, &number);

  REQ(r, CallInfoMessage);

  if (name)
    strncpy(r->msg.CallInfoMessage.callingPartyName, name, 39);
  if (number)
    strncpy(r->msg.CallInfoMessage.callingParty, number, 23);

  if (c->isOutgoing) {

    if (c->calledPartyName)
      strncpy(r->msg.CallInfoMessage.calledPartyName, c->calledPartyName, 39);
    if (c->calledPartyNumber)
      strncpy(r->msg.CallInfoMessage.calledParty, c->calledPartyNumber, 23);

  } else {

    strncpy(tmp, c->line->callerid, 235);
    ast_callerid_parse(tmp, &name, &number);
    if (name)
      strncpy(r->msg.CallInfoMessage.calledPartyName, name, 39);
    if (number)
      strncpy(r->msg.CallInfoMessage.calledParty, number, 23);

  }


  r->msg.CallInfoMessage.lineId   = c->line->instance;
  r->msg.CallInfoMessage.callRef  = c->callid;
  r->msg.CallInfoMessage.callType = (c->isOutgoing) ? 2 : 1; // XXX:T: Need to add 3, for Forarded call.

  sccp_dev_send(c->line->device, r);
}

// XXX:T: whoops - need to move to device.
void
sccp_channel_set_callstate(sccp_channel_t * c, StationDCallState state)
{
  sccp_moo_t * r;

  // XXX:T: Assert!

  REQ(r, CallStateMessage)
  r->msg.CallStateMessage.callState     = state;
  r->msg.CallStateMessage.lineInstance  = (c) ? c->line->instance : 0;
  r->msg.CallStateMessage.callReference = (c) ? c->callid : 0;

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "{CallStateMessage} callState=%s(%d), lineInstance=%d, callReference=%d\n", 
      TsCallStatusText[state],
      r->msg.CallStateMessage.callState,
      r->msg.CallStateMessage.lineInstance,
      r->msg.CallStateMessage.callReference);

  sccp_dev_send(c->line->device, r);

  if (c->line->instance)
    c->line->dnState = state;

}

void
sccp_channel_set_lamp(sccp_channel_t * c, int lampMode)
{
  sccp_moo_t * r;
  REQ(r, SetLampMessage)
  r->msg.SetLampMessage.stimulus = 0x9;
  r->msg.SetLampMessage.stimulusInstance = c->line->instance;
  r->msg.SetLampMessage.lampMode = lampMode;
  sccp_dev_send(c->line->device, r);
}

void
sccp_channel_connect(sccp_channel_t * c)
{
  sccp_moo_t * r;
  struct sockaddr_in sin;
  unsigned char m[3] = "";
  ast_rtp_get_us(c->rtp, &sin);

  REQ(r, OpenReceiveChannel)
  r->msg.OpenReceiveChannel.conferenceId = 0;
  r->msg.OpenReceiveChannel.passThruPartyId = 0;
  r->msg.OpenReceiveChannel.millisecondPacketSize = 20;
  r->msg.OpenReceiveChannel.payloadType = 0x4; // XXX:T: Hack!
  r->msg.OpenReceiveChannel.qualifierIn.vadValue = c->line->vad;
  sccp_dev_send(c->line->device, r);

  memset(m, 0, 4);
  memcpy(m, &__ourip.s_addr, 4);
  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_2 "Telling Endpoint to use %d.%d.%d.%d(%d):%d\n", m[0], m[1], m[2], m[3], ntohs(__ourip.s_addr), ntohs(sin.sin_port));

  REQ(r, StartMediaTransmission)
  r->msg.StartMediaTransmission.conferenceId = 0;
  r->msg.StartMediaTransmission.passThruPartyId = 0;
  r->msg.StartMediaTransmission.remoteIpAddr = __ourip.s_addr;
  r->msg.StartMediaTransmission.remotePortNumber = ntohs(sin.sin_port);
  r->msg.StartMediaTransmission.millisecondPacketSize = 20;
  r->msg.StartMediaTransmission.payloadType = 0x4; // XXX:T Cludge!
  r->msg.StartMediaTransmission.qualifierOut.precedenceValue = 0;
  r->msg.StartMediaTransmission.qualifierOut.ssValue = 0; // Silence supression
  r->msg.StartMediaTransmission.qualifierOut.maxFramesPerPacket = 95792;
  sccp_dev_send(c->line->device, r);
}

void
sccp_channel_disconnect(sccp_channel_t * c)
{
  sccp_moo_t * r;

  REQ(r, CloseReceiveChannel)
  sccp_dev_send(c->line->device, r);
  
  REQ(r, StopMediaTransmission)
  sccp_dev_send(c->line->device, r);

}

void
sccp_channel_endcall(sccp_channel_t * c)
{
  struct ast_frame f = { AST_FRAME_CONTROL, AST_CONTROL_HANGUP };

  ast_mutex_lock(&c->line->lock);
  ast_mutex_lock(&c->lock);

  // Hang up the current channel.
  sccp_dev_set_sptone(c->line->device, "NoTone");

  sccp_channel_set_callstate(c, TsOnHook);

  // Queue a request for asterisk to hangup this channel.
  ast_queue_frame(c->owner, &f, 0);

  if (!c->line) {
    ast_log(LOG_ERROR, "Channel %s doesn't have a line!\n", c->owner->name);
    ast_mutex_unlock(&c->lock);
    return;
  }

  sccp_dev_set_speaker(c->line->device, StationSpeakerOff);

  ast_mutex_unlock(&c->lock);
  ast_mutex_unlock(&c->line->lock);

  return;
}
