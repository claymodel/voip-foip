/*
 *
 * SCCP support for Asterisk.
 *   -- By Zozo
 */

#include "chan_sccp.h"
#include "sccp_cli.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include <asterisk/cli.h>

char * TsCallStatusText[MAX_TsCallStatusText] = {
  "--",
  "OffHook",
  "OnHook",
  "RingOut",
  "RingIn",
  "Connected",
  "Busy",
  "InUse",
  "Hold",
  "CWaiting",
  "CTransfer",
  "CPark",
  "CProceed",
  "RemoteUse"
};

static char * 
complete_device(char *line, char *word, int pos, int state)
{
  sccp_device_t * d;
  int which = 0;
  char * ret;

  if (pos > 3)
    return NULL;

  ast_mutex_lock(&devicelock);
  d = devices;
  while(d) {
    if (!strncasecmp(word, d->id, strlen(word))) {
      if (++which > state)
        break;
    }
    d = d->next;
  }
  ret = d ? strdup(d->id) : NULL;

  ast_mutex_unlock(&devicelock);

  return ret;
}

static char * 
complete_intercom(char *line, char *word, int pos, int state)
{
  sccp_intercom_t * i;
  int which = 0;
  char * ret;

  if (pos > 3)
    return NULL;

  ast_mutex_lock(&intercomlock);
  i = intercoms;
  while(i) {
    if (!strncasecmp(word, i->id, strlen(word))) {
      if (++which > state)
        break;
    }
    i = i->next;
  }
  ret = i ? strdup(i->id) : NULL;

  ast_mutex_unlock(&intercomlock);

  return ret;
}


/* ------------------------------------------------------------ */

static int sccp_reset_restart(int fd, int argc, char * argv[])
{
  sccp_moo_t * r;
  sccp_device_t * d;

  if (argc != 3)
    return RESULT_SHOWUSAGE;

  d = sccp_dev_find_byid(argv[2]);

  if (!d) {
    ast_cli(fd, "Can't find device %s\n", argv[2]);
    return RESULT_SUCCESS;
  }

  REQ(r, Reset)
  r->msg.Reset.resetType = (!strcasecmp(argv[1], "reset")) ? DEVICE_RESET : DEVICE_RESTART;
  sccp_dev_send(d, r);

  ast_cli(fd, "Reset device %s\n", argv[2]);
  return RESULT_SUCCESS;

}

static struct ast_cli_entry cli_reset = {
  { "sccp", "reset", NULL },
  sccp_reset_restart,
  "Reset an SCCP device",
  "Usage: sccp reset <deviceId>\n",
  complete_device
};

static struct ast_cli_entry cli_restart = {
  { "sccp", "restart", NULL },
  sccp_reset_restart,
  "Reset an SCCP device",
  "Usage: sccp restart <deviceId>\n",
  complete_device
};

/* ------------------------------------------------------------ */

static int 
sccp_show_channel(int fd, int argc, char * argv[])
{
  sccp_channel_t * c;

  ast_cli(fd, "ID    LINE       DEVICE           TR CALLED     \n");
  ast_cli(fd, "===== ========== ================ == ========== \n");

  ast_mutex_lock(&chanlock);
  c = chans;
  while(c) {
    ast_cli(fd, "%.5d %-10s %-16s %c%c %-10s\n", 
      c->callid, 
      c->line->name,
      c->line->device->description, 
     (c->isOutgoing) ? 'O' : 'I', 
     (c->isRinging) ? 'R' : '-', 
      c->calledPartyNumber);
    c = c->lnext;
  }
  ast_mutex_unlock(&chanlock);
  return RESULT_SUCCESS;
}

static struct ast_cli_entry cli_show_channels = {
  { "sccp", "show", "channels", NULL },
  sccp_show_channel,
  "Show all SCCP channels",
  "Usage: sccp show channel\n",
  // complete_line
};

/* ------------------------------------------------------------ */

static int 
sccp_show_devices(int fd, int argc, char * argv[])
{
  sccp_device_t * d;

  ast_cli(fd, "NAME             ADDRESS         MAC             \n");
  ast_cli(fd, "================ =============== ================\n");

  ast_mutex_lock(&devicelock);
  d = devices;
  while(d) {
    ast_cli(fd, "%-16s %-15s %-16s\n",// %-10s %-16s %c%c %-10s\n", 
      d->description, 
      (d->session) ? d->session->in_addr : "--",
      d->id
    ); 
    d = d->next;
  }
  ast_mutex_unlock(&devicelock);
  return RESULT_SUCCESS;
}

static struct ast_cli_entry cli_show_devices = {
  { "sccp", "show", "devices", NULL },
  sccp_show_devices,
  "Show all SCCP Devices",
  "Usage: sccp show devices\n",
  complete_device
};

/* ------------------------------------------------------------ */

static int 
sccp_show_intercoms(int fd, int argc, char * argv[])
{
  sccp_intercom_t * i;
  sccp_device_t ** id;

  if (argc == 4) {

    ast_mutex_lock(&intercomlock);
    i = intercoms;
    while(i) {
      if (!strcasecmp(argv[3], i->id))
        break;
      i = i->next;
    }
    ast_mutex_unlock(&intercomlock);

    if (i == NULL) {
      ast_cli(fd, "Failed to find intercom channel with name '%s'\n", argv[3]);
      return RESULT_SUCCESS;
    }

    ast_cli(fd, "INTERCOM CHANNEL\n");
    ast_cli(fd, "================\n");
    ast_cli(fd, " Identifier : %s\n", argv[3]);
    ast_cli(fd, "Description : %s\n", argv[3]);

    ast_cli(fd, "\n");

    ast_cli(fd, "MEMBERS\n");
    ast_cli(fd, "=======\n\n");

    id = i->devices;
    while (*id) {
      ast_cli(fd, " * %s\n", (*id)->id);
      id++;
    }

    ast_cli(fd, "\n");

  } else {

    ast_cli(fd, "NAME             DESCRIPTION\n");
    ast_cli(fd, "================ ===============\n");

    ast_mutex_lock(&intercomlock);
    i = intercoms;
    while(i) {
      ast_cli(fd, "%-16s %s\n", i->id, i->description);
      i = i->next;
    }
    ast_mutex_unlock(&intercomlock);

    ast_cli(fd, "\n Do sccp show intercom <name> to show members of an intercom channel\n\n");


  }

  return RESULT_SUCCESS;
}

static struct ast_cli_entry cli_show_intercoms = {
  { "sccp", "show", "intercoms", NULL },
  sccp_show_intercoms,
  "Show all SCCP Inetrcom Lines",
  "Usage: sccp show intercoms\n",
  complete_intercom,
};

/* ------------------------------------------------------------ */

static int sccp_show_lines(int fd, int argc, char * argv[])
{
  sccp_line_t * l;

  ast_cli(fd, "NAME       DEVICE           DN STATE     MWI    CODEC\n");
  ast_cli(fd, "========== ================ ============ ====== ===========\n");

  ast_mutex_lock(&linelock);
  l = lines;

  if (l)
    ast_mutex_lock(&l->lock);

  while (l) {
    int i;
    sccp_line_t * t;
    char * codecName = "--";

    if (l->activeChannel) {
      for ( i = 0; i < 32 ; i++ )
        if (l->activeChannel->owner->nativeformats == (1 << i))
          codecName = ast_codec2str(i);
    }

    ast_cli(fd, "%-10s %-16s %-12s %-6s %-11.11s\n", 
      l->name, 
      (l->device) ? l->device->description : "--",
      (l->dnState > MAX_TsCallStatusText) ? "*ERROR*" : TsCallStatusText[l->dnState],
      (l->device) ? (l->hasMessages) ? "ON" : "OFF" : "--",
      (l->device) ? codecName : "--");

    if (l->lnext)
      ast_mutex_lock(&(l->lnext->lock));

    t = l->lnext;
    ast_mutex_unlock(&l->lock);
    l = t;
  }

  ast_mutex_unlock(&linelock);
  return RESULT_SUCCESS;
}

static char sccp_show_lines_usage [] =
 "Usage: sccp show lines\n";

static struct ast_cli_entry
cli_show_lines =
{ 
  { "sccp", "show", "lines", NULL }, 
  sccp_show_lines, 
  "Show SCCP Lines",
  sccp_show_lines_usage,
};




void sccp_register_cli(void)
{
  ast_cli_register(&cli_show_intercoms);
  ast_cli_register(&cli_show_channels);
  ast_cli_register(&cli_show_devices);
  ast_cli_register(&cli_show_lines);
  ast_cli_register(&cli_restart);
  ast_cli_register(&cli_reset);
}
