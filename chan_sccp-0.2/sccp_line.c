#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_pbx.h"
#include "sccp_line.h"
#include "sccp_channel.h"
#include <asterisk/callerid.h>

sccp_line_t *
sccp_line_find_byname(char * line)
{
  sccp_line_t * l;
  ast_mutex_lock(&linelock);
  l = lines;
  while(l && strcasecmp(l->name, line) != 0) {
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 " --*> %s\n", l->name);
    l = l->lnext;
  }
  ast_mutex_unlock(&linelock);
  return l;
}

sccp_line_t *
sccp_line_find_byid(sccp_device_t * d, int instance)
{
  sccp_line_t * l;
  ast_mutex_lock(&d->lock);
  l = d->lines;
  while (l) {
    if (l->instance == instance)
      break;
    l = l->next;
  }
  ast_mutex_unlock(&d->lock);
  return l;
}

void
sccp_line_dial(sccp_line_t * l, char * num)
{
  ast_log(LOG_DEBUG, "Trying to dial %s on line %s\n", num, l->name);
  sccp_dev_allocate_channel(l->device, l, 1, num);
}

/* Kills a line's channels. */
/* Called with a lock on l->lock */
void
sccp_line_kill(sccp_line_t * l)
{
  sccp_channel_t * c = l->channels;
  while (c) {
    sccp_channel_t * t = c->next;
    sccp_channel_endcall(c);
    c = t;
  }
  // Now wait until l->channel has no activeChannel, in scheduler.
}
