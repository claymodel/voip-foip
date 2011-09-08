/*
 * Asterisk - SCCP Support (By Zozo)
 * ---------------------------------------------------------------------------
 * sccp_helper.c - SCCP Helper Functions
 * 
 * $Id: sccp_helper.c,v 1.2 2003/11/22 19:18:34 zozo.anlx.net Exp $
 *
 */

#include "chan_sccp.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include <unistd.h>
#include <asterisk/app.h>

const char * sccpmsg2str(int e)
{
  const value_string * v = message_list;
  while (v)
  {
    if (v->key == e)
      return v->value;
    v++;
  }
  return "Unknown";
}

int sccp_line_hasmessages(sccp_line_t * l)
{
  int newmsgs, oldmsgs;
  int totalmsgs = 0;
  char tmp[AST_MAX_EXTENSION] = "", * mb, * cur;
  strncpy(tmp, l->mailbox, sizeof(tmp));
  mb = tmp;

  while((cur = strsep(&mb, ","))) {
    if (strlen(cur)) {
      if (sccp_debug > 2)
        ast_verbose(VERBOSE_PREFIX_3 "Checking mailbox: %s\n", cur);
      ast_app_messagecount(cur, &newmsgs, &oldmsgs);
      totalmsgs += newmsgs;
    }
  }
  return totalmsgs;
}

sccp_intercom_t *
sccp_intercom_find_byname(char * name) 
{
  sccp_intercom_t * i = NULL;

  ast_mutex_lock(&intercomlock);
  i = intercoms;
  while(i) {
    if (!strcasecmp(name, i->id))
      break;
    i = i->next;
  }
  ast_mutex_unlock(&intercomlock);
  return i;
}

char *
sccp_helper_getversionfor(sccp_session_t * s)
{
  return "P002F202";
}
