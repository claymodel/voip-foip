#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"

void 
sccp_sk_redial(sccp_device_t * d , sccp_line_t * l, sccp_channel_t * c)
{
  ast_log(LOG_DEBUG, "Redial Softkey.\n");

  if (!d->lastNumberLine)
    return;

  l = sccp_line_find_byid(d, d->lastNumberLine);

  if (l)
    sccp_line_dial(l, d->lastNumber);

}

void 
sccp_sk_newcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c)
{
  ast_log(LOG_DEBUG, "Starting new call....\n");

  if (!l)
    l = d->currentLine;

  c = sccp_dev_allocate_channel(d, d->currentLine, 1, NULL);

  if (!c) {
    ast_log(LOG_ERROR, "Failed to allocate channel\n");
    return;
  }

  sccp_dev_set_speaker(l->device, StationSpeakerOn);
  sccp_dev_statusprompt_set(l->device, c, NULL, 0);
  sccp_dev_set_keyset(l->device, c, KEYMODE_OFFHOOK);
  sccp_dev_set_sptone(l->device, "InsideDialTone");
  
}

void sccp_sk_endcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c)
{
  if (!c) {
    ast_log(LOG_WARNING, "Tried to EndCall while no call in progress.\n");
    return;
  }

  sccp_channel_endcall(c);

}


void 
sccp_sk_addline(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c)
{
  ast_log(LOG_DEBUG, "Adding Line\n");
}

void sccp_sk_dnd(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) 
{
  ast_mutex_lock(&d->lock);

    d->dnd = (d->dnd) ? 0 : 1;

    if (d->dnd)
      sccp_dev_statusprompt_set(d, NULL, "DND is Enabled", 0);
    else
      sccp_dev_statusprompt_set(d, NULL, "DND Turned Off", 5);

  ast_mutex_unlock(&d->lock);
}


void sccp_sk_back(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {}
void sccp_sk_dial(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {}
void sccp_sk_clear(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {}

void sccp_sk_answer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}
void sccp_sk_reject(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}
void sccp_sk_hold(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}
void sccp_sk_conference(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}
void sccp_sk_transfer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}
void sccp_sk_blindxfr(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c){}

