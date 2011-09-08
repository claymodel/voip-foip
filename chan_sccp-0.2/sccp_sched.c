#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include "sccp_sched.h"
#include <unistd.h>

int
sccp_sched_delsession(void * data)
{
  sccp_session_t * s = data;
  sccp_line_t * l = NULL;

  ast_mutex_lock(&sessionlock);

  ast_log(LOG_DEBUG, "Removing session %p, device %p\n", s, s->device);

  if (s->device != NULL) {

    ast_mutex_lock(&s->device->lock);

    l = s->device->lines;

    ast_mutex_lock(&l->lock);

    if (l->channelCount != 0) {
      ast_log(LOG_DEBUG, "sccp_sched_delsession still has %d active channels, not free()'ing session yet.\n", l->channelCount);
      ast_mutex_unlock(&l->lock);
      ast_mutex_unlock(&s->device->lock);
      ast_mutex_unlock(&sessionlock);
      return 1000;
    }

    if (s == s->device->session)
      s->device->session = NULL;

    ast_mutex_unlock(&l->lock);
    ast_mutex_unlock(&s->device->lock);

  }

  if (s->in_addr)
    free(s->in_addr);
  free(s);

  ast_mutex_unlock(&sessionlock);

  return 0;
}

int
sccp_sched_keepalive(void * data)
{
  sccp_session_t * s;
  ast_mutex_lock(&sessionlock);
  s = sessions;
  while (s) { 
    if ( (s->fd > 0) && (s->device != NULL) && (time(0) > (s->lastKeepAlive + (keepalive*2))) ) {
      // dead client - kill them if we can.
      ast_log(LOG_WARNING, "Dead SCCP client: %s (%p/%p)\n", s->device->id, s, s->device);
      ast_mutex_lock(&s->lock);
      close(s->fd);
      s->fd = -1;
      ast_mutex_unlock(&s->lock);
    }
    s = s->next;
  }
  ast_mutex_unlock(&sessionlock);
  return ((keepalive * 2) * 1000);
}
