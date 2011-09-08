
#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_channel.h"
#include "sccp_cli.h"
#include "sccp_line.h"
#include "sccp_sched.h"
#include "sccp_socket.h"
#include <ctype.h>
#include <unistd.h>
#include <asterisk/pbx.h>
#include <asterisk/callerid.h>

/* Socket Stuff */
static struct sockaddr_in   bindaddr;
struct hostent            * hp;
static char                 __ourhost[256];
struct in_addr              __ourip;
char                        date_format[6] = "D-M-Y";
char                        default_context[AST_MAX_EXTENSION] = "default";

extern int sccp_descriptor;

static pthread_t accept_t;

/* Yes, these are all that the phone supports (except it's own 'Wideband 256k') */
int capability = AST_FORMAT_ULAW | AST_FORMAT_ALAW | AST_FORMAT_G729A;
int sccp_debug = 10;
int keepalive  = SCCP_KEEPALIVE;

static struct io_context *io;
static pthread_t monitor_thread = 0;

/* The scheduler */
struct sched_context * sccp_sched;

/* Lists */
sccp_device_t  * devices    = NULL;
sccp_session_t * sessions   = NULL;
sccp_line_t    * lines      = NULL;
sccp_channel_t * chans      = NULL;
sccp_intercom_t * intercoms = NULL;
/* ... and their locks ... */
ast_mutex_t      intercomlock = AST_MUTEX_INITIALIZER;
ast_mutex_t      sessionlock  = AST_MUTEX_INITIALIZER;
ast_mutex_t      devicelock   = AST_MUTEX_INITIALIZER;
ast_mutex_t      linelock     = AST_MUTEX_INITIALIZER;
ast_mutex_t      chanlock     = AST_MUTEX_INITIALIZER;
ast_mutex_t      monlock      = AST_MUTEX_INITIALIZER;

/* Keep track of when we're in use. */
int              usecnt       = 0;
ast_mutex_t      usecnt_lock  = AST_MUTEX_INITIALIZER;


/* ---------------------------------------------------- */

static struct ast_channel *sccp_request(char *type, int format, void *data) {

  // type = SCCP
  // format = <codec>
  // data == line to call
  // sccp_pvt_t * = NULL;
  sccp_line_t * l = NULL;
  sccp_channel_t * chan = NULL;
  sccp_intercom_t * i = NULL;

  // XXX:T: Is this actually needed?
  if (!data) {
    ast_log(LOG_NOTICE, "Attempt to call SCCP/ failed\n");
    return NULL;
  }

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_1 "SCCP trying to call %s, format %d, data, %s\n", type, format, (char*)data);

  l = sccp_line_find_byname((char*)data);

  if (l) {
    if (!l->device) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_1 "SCCP/%s isn't currently registered anywhere.\n", l->name);
      return NULL;
    }
    // XXX:T: Make sure they've not reached maxcalls.

    // Allocate a new SCCP channel.
    chan = sccp_dev_allocate_channel(l->device, l, 0, NULL);

    // pvt = malloc(sizeof(sccp_pvt_t));
    // memset(pvt, 0, sizeof(sccp_pvt_t));
    // pvt->chan = chan;
  }

  i = sccp_intercom_find_byname((char*)data);

  if ( (!l) && (!i) ) {
    ast_log(LOG_NOTICE, "Can't find SCCP/%s: Unknown Line or Intercom\n", (char*)data);
    return NULL;
  } else if (i) {

    ast_log(LOG_WARNING, "Intercom not yet supported\n");
    return NULL;

    // pvt = malloc(sizeof(sccp_pvt_t));
    // memset(pvt, 0, sizeof(sccp_pvt_t));

  }

  return chan->owner;
}

static int sccp_devicestate(void *data) {
  ast_log(LOG_NOTICE, "Can't devicestate()\n");
  return AST_DEVICE_UNKNOWN;
}

int handle_message(sccp_moo_t * r, sccp_session_t * s)
{
  if ( (!s->device) && (r->messageId != RegisterMessage && r->messageId != AlarmMessage && r->messageId != KeepAliveMessage)) {
    ast_log(LOG_WARNING, "Client sent %s without first registering.\n", sccpmsg2str(r->messageId));
    free(r);
    return 0;
  }

//  if (r->messageId != KeepAliveMessage) {
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_2 " >> Got message %s\n", sccpmsg2str(r->messageId));
//  }

  switch (r->messageId) {

    case AlarmMessage:
      sccp_handle_alarm(s, r);
      break;
    case RegisterMessage:
      sccp_handle_register(s, r);
      break;
    case VersionReqMessage:
      sccp_handle_version(s, r);
      break;
    case CapabilitiesResMessage:
      sccp_handle_capabilities_res(s, r);
      break;
    case ButtonTemplateReqMessage:
      sccp_handle_button_template_req(s, r);
      break;
    case SoftKeyTemplateReqMessage:
      sccp_handle_soft_key_template_req(s, r);
      break;
    case SoftKeySetReqMessage:
      sccp_handle_soft_key_set_req(s, r);
      break;
    case LineStatReqMessage:
      sccp_handle_line_number(s, r);
      break;
    case SpeedDialStatReqMessage:
      sccp_handle_speed_dial_stat_req(s, r);
      break;
    case StimulusMessage:
      sccp_handle_stimulus(s, r);
      break;
    case OffHookMessage:
      sccp_handle_offhook(s, r);
      break;
    case OnHookMessage:
      sccp_handle_onhook(s, r);
      break;
    case HeadsetStatusMessage:
      sccp_handle_headset(s, r);
      break;
    case TimeDateReqMessage:
      sccp_handle_time_date_req(s, r);
      break;
    case KeypadButtonMessage:
      sccp_handle_keypad_button(s, r);
      break;
    case SoftKeyEventMessage:
      sccp_handle_soft_key_event(s, r);
      break;
    case KeepAliveMessage:
      s->lastKeepAlive = time(0);
      sccp_session_sendmsg(s, KeepAliveAckMessage);
      sccp_dev_check_mwi(s->device);
      break;
    case IpPortMessage:
      s->rtpPort = r->msg.IpPortMessage.rtpMediaPort;
      break;
    case OpenReceiveChannelAck:
      sccp_handle_open_recieve_channel_ack(s, r);
      break;
    case unknownClientMessage2:
      if (s->device != NULL)
        sccp_dev_set_registered(s->device, RsOK);
      break;
    default:
      ast_log(LOG_WARNING, "Unhandled SCCP Message: %d - %s\n", r->messageId, sccpmsg2str(r->messageId));
  }

  free(r);
  return 1;
}

/* ---------------------------------------------------- */



static sccp_line_t *
build_line(struct ast_config * cfg, char * cat)
{
  struct ast_variable * v = ast_variable_browse(cfg, cat);
  sccp_line_t * l = NULL;

  l = malloc(sizeof(sccp_line_t)); // ...
  memset(l, 0, sizeof(sccp_line_t));
  ast_log(LOG_DEBUG, "Allocated an SCCP line.\n");
  l->instance = -1;

  strncpy(l->name, cat, sizeof(l->name));
  strncpy(l->context, default_context, AST_MAX_EXTENSION);
  strncpy(l->callerid, cat, sizeof(l->callerid));

  while(v) {
    if (!strcasecmp(v->name, "id")) {
      strncpy(l->id, v->value, sizeof(l->id));
    } else if (!strcasecmp(v->name, "pin")) {
      strncpy(l->pin, v->value, sizeof(l->pin));
    } else if (!strcasecmp(v->name, "")) {
      strncpy(l->label, v->value, sizeof(l->label));
    } else if (!strcasecmp(v->name, "description")) {
      strncpy(l->description, v->value, sizeof(l->description));
    } else if (!strcasecmp(v->name, "context")) {
      strncpy(l->context, v->value, sizeof(l->context));
    } else if (!strcasecmp(v->name, "callerid")) {
      strncpy(l->callerid, v->value, sizeof(l->callerid));
    } else if (!strcasecmp(v->name, "mailbox")) {
      strncpy(l->mailbox, v->value, sizeof(l->mailbox));
    }
    v = v->next;
  }
  return l;
}

static sccp_device_t *
build_device(struct ast_config * cfg, char * cat)
{
  struct ast_variable * v = ast_variable_browse(cfg, cat);
  sccp_device_t * d;

  d = malloc(sizeof(sccp_device_t)); // ...
  memset(d, 0, sizeof(sccp_device_t));
  ast_log(LOG_DEBUG, "Allocated an SCCP device.\n");

  strncpy(d->id, cat, StationMaxDeviceNameSize - 1);
  strncpy(d->description, cat, StationMaxDeviceNameSize - 1);
  d->type = 7960;

  while(v) {
    if (!strcasecmp(v->name, "type")) {
      const button_modes * bs = default_layouts;
      d->type = atoi(v->value);    
      while (bs->type) {
        if (!strcasecmp(v->value, bs->type))
          break;
         bs++;
      }
      if (bs->buttons)
        d->buttonSet = bs;
      else
        ast_log(LOG_ERROR, "Don't know how the buttons on model '%s' are (report as bug)\n", v->value);

    } else if (!strcasecmp(v->name, "autologin")) {

      strncpy(d->autologin, ast_variable_retrieve(cfg, cat, "autologin"), SCCP_MAX_AUTOLOGIN);

    } else if (!strcasecmp(v->name, "description")) {

      strncpy(d->description, v->value, StationMaxDeviceNameSize - 1);

    } else if (!strcasecmp(v->name, "speeddial")) {

      char * splitter = v->value, * exten = NULL, * name = NULL;
      sccp_speed_t *  k = malloc(sizeof(sccp_speed_t)); // ...
      ast_log(LOG_DEBUG, "Allocated an SCCP speed dial key.\n");

      memset(k, 0, sizeof(sccp_speed_t));
      k->index = 0;

      if (strchr(splitter, ',')) {
        exten = strsep(&splitter, ",");
        name  = splitter;
      }

      strncpy(k->name, splitter, 40);
      strncpy(k->ext, exten, 24);
      k->next = d->speed_dials;
      d->speed_dials = k;

    }
    v = v->next;
  }

  return d;
}

static 
int reload_config(void)
{
  struct ast_config   * cfg;
  struct ast_variable * v;
  char                * cat = NULL;
  sccp_device_t      ** id;
  int                   oldport = ntohs(bindaddr.sin_port),
                        on      = 1,
                        c       = 0;

  if (gethostname(__ourhost, sizeof(__ourhost))) {
    ast_log(LOG_WARNING, "Unable to get hostname, SCCP disabled\n");
    return 0;
  }

  memset(&bindaddr, 0, sizeof(bindaddr));
  hp = gethostbyname(__ourhost);
  if (!hp) {
    ast_log(LOG_WARNING, "Unable to get our IP address, SCCP disabled\n");
    return 0;
  }
  memcpy(&__ourip, hp->h_addr, sizeof(__ourip));
  bindaddr.sin_port   = ntohs(DEFAULT_SCCP_PORT);
  bindaddr.sin_family = AF_INET;

  cfg = ast_load("sccp.conf");
  if (!cfg) {
    ast_log(LOG_NOTICE, "Unable to load config file sccp.conf, SCCP disabled\n");
    return 0;
  }

  v = ast_variable_browse(cfg, "general");
  while (v) {
    if (!strcasecmp(v->name, "keepalive")) {
      keepalive = atoi(v->value);
    } else if (!strcasecmp(v->name, "context")) {
      strncpy(default_context, v->value, AST_MAX_EXTENSION);
    } else if (!strcasecmp(v->name, "dateformat")) {
      strncpy (date_format, v->value, 5);
    } else {
      ast_log(LOG_WARNING, "Unknown Key in [general]: %s\n", v->name);
    }
    v = v->next;
  }

  cat = ast_category_browse(cfg, cat);

  while (cat) {

    if (strcasecmp(cat, "general") != 0) {

      if (ast_variable_retrieve(cfg, cat, "type") != NULL) {
        sccp_device_t * d = build_device(cfg, cat);

        if (d) {
          if (option_verbose > 2)
            ast_verbose(VERBOSE_PREFIX_3 "Added device '%s'\n", d->id);
          ast_mutex_lock(&devicelock);
          d->next = devices;
          devices = d;
          ast_mutex_unlock(&devicelock);
        }

      } else if (ast_variable_retrieve(cfg, cat, "id") != NULL) {
        sccp_line_t * l = build_line(cfg, cat);
        if (l) {
          if (option_verbose > 2)
            ast_verbose(VERBOSE_PREFIX_3 "Added line '%s'\n", l->name);
          ast_mutex_lock(&linelock);
          l->lnext = lines;
          lines = l;
          ast_mutex_unlock(&linelock);
        }

      } else if (ast_variable_retrieve(cfg, cat, "device") != NULL) {

        sccp_intercom_t * i = malloc(sizeof(sccp_intercom_t));
        sccp_device_t * d = NULL;

        memset(i, 0, sizeof(sccp_intercom_t));

        if (ast_variable_retrieve(cfg, cat, "description"))
          strncpy(i->description, ast_variable_retrieve(cfg, cat, "description"), 23);

        strncpy(i->id, cat, 23);

        c = 0;

        /* Loop 1: count how many devices are in this intercom. */
        v = ast_variable_browse(cfg, cat);
        while (v) {
          if (!strcasecmp(v->name, "device")) {
            if (sccp_dev_find_byid(v->value))
              c++;
          }
          v = v->next;
        }

        if (c == 0)
          break;

        c++;

        i->devices = (sccp_device_t **) malloc(sizeof(sccp_device_t*) * c);
        id = memset(i->devices, 0, (sizeof(sccp_device_t*) * c));

        /* Loop 2: Actually allocate them */
        v = ast_variable_browse(cfg, cat);
        while (v) {
          if (!strcasecmp(v->name, "device"))  {
            d = sccp_dev_find_byid(v->value);
            if (d) {
              ast_log(LOG_DEBUG, "Added device %s to intercom line %s\n", v->value, cat);
              *id = d;
              id++;
            } else {
              ast_log(LOG_ERROR, "Warning, failed to find device %s for intercom line %s\n", v->value, cat);
            }
          }
          v = v->next;
        }

        ast_mutex_lock(&intercomlock);
        i->next = intercoms;
        intercoms = i;
        ast_mutex_unlock(&intercomlock);

      } else {
        ast_log(LOG_ERROR, "[%s] in sccp.conf must either have a type = <phoneType>, or id = <lineId>.\n", cat);
      }

    }
    cat = ast_category_browse(cfg, cat);
  }

  if ((sccp_descriptor > -1) && (ntohs(bindaddr.sin_port) != oldport)) {
    close(sccp_descriptor);
    sccp_descriptor = -1;
  }

  if (sccp_descriptor < 0) {

    sccp_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    on = 1;
    setsockopt(sccp_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (sccp_descriptor < 0) {

      ast_log(LOG_WARNING, "Unable to create SCCP socket: %s\n", strerror(errno));

    } else {
      if (bind(sccp_descriptor, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
        ast_log(LOG_WARNING, "Failed to bind to %s:%d: %s!\n", 
                             inet_ntoa(bindaddr.sin_addr), ntohs(bindaddr.sin_port),
                             strerror(errno));
        close(sccp_descriptor);
        sccp_descriptor = -1;
        return 0;
      } else {
        ast_log(LOG_NOTICE, "SCCP channel driver up and running on %s:%d\n", 
                            inet_ntoa(bindaddr.sin_addr), ntohs(bindaddr.sin_port));
      }

      if (listen(sccp_descriptor, DEFAULT_SCCP_BACKLOG)) {
        ast_log(LOG_WARNING, "Failed to start listening to %s:%d: %s\n",
                             inet_ntoa(bindaddr.sin_addr), ntohs(bindaddr.sin_port),
                             strerror(errno));
        close(sccp_descriptor);
        sccp_descriptor = -1;
        return 0;
      }

      if (sccp_debug)
        ast_log(LOG_NOTICE, "SCCP listening on %s:%d\n", inet_ntoa(bindaddr.sin_addr), ntohs(bindaddr.sin_port));
      pthread_create(&accept_t,NULL, socket_thread, NULL);
    }
  }

  ast_destroy(cfg);
  return 0;
}

static char *descrip =
"  SetCalledParty(\"Name\" <ext>) sets the name and number of the called party for use with chan_sccp\n";

static int
setcalledparty_exec(struct ast_channel *chan, void *data)
{
  char tmp[256] = "";
  char * num, * name;
  sccp_channel_t * c = chan->pvt->pvt;

  if (strcasecmp(chan->type, "SCCP") != 0)
    return 0;

  if (!data)
    return 0;

  strncpy(tmp, (char *)data, sizeof(tmp) - 1);
  ast_callerid_parse(tmp, &name, &num);

  if (name)
    strncpy(c->calledPartyName, name, 39);
  if (num)
    strncpy(c->calledPartyNumber, num, 23);

  return 0;
}

static void *do_monitor(void *data)
{
  int res = 1000;
  for(;;) {
    pthread_testcancel();
    res = ast_sched_wait(sccp_sched);
    if ((res < 0) || (res > 1000))
      res = 1000;
    res = ast_io_wait(io, res);
    ast_mutex_lock(&monlock);
    if (res >= 0)
      ast_sched_runq(sccp_sched);
    ast_mutex_unlock(&monlock);
  }
  return NULL;
}


static int restart_monitor(void)
{

  /* If we're supposed to be stopped -- stay stopped */
  if (monitor_thread == -2)
    return 0;

  if (ast_mutex_lock(&monlock)) {
    ast_log(LOG_WARNING, "Unable to lock monitor\n");
    return -1;
  }

  if (monitor_thread == pthread_self()) {
    ast_mutex_unlock(&monlock);
    ast_log(LOG_WARNING, "Cannot kill myself\n");
    return -1;
  }
  if (monitor_thread) {
     /* Wake up the thread */
    pthread_kill(monitor_thread, SIGURG);
  } else {
    /* Start a new monitor */
    if (pthread_create(&monitor_thread, NULL, do_monitor, NULL) < 0) {
      ast_mutex_unlock(&monlock);
      ast_log(LOG_ERROR, "Unable to start monitor thread.\n");
      return -1;
    }
  }
  ast_mutex_unlock(&monlock);
  return 0;
}

int load_module()
{

  sccp_sched = sched_context_create();
  if (!sccp_sched)
    ast_log(LOG_WARNING, "Unable to create schedule context\n");

  io = io_context_create();
  if (!io)
    ast_log(LOG_WARNING, "Unable to create I/O context\n");


  if (!reload_config()) {
    if (ast_channel_register_ex("SCCP", "SCCP", capability, sccp_request, sccp_devicestate)) {
      ast_log(LOG_ERROR, "Unable to register channel class SCCP\n");
      return -1;
    }
  }

  sccp_register_cli();
  ast_register_application("SetCalledParty", setcalledparty_exec, "Sets the name of the called party", descrip);
  restart_monitor();

  ast_sched_add(sccp_sched, (keepalive * 2) * 1000, sccp_sched_keepalive, NULL);

  return 0;
}

int reload(void)
{
  // XXX:T: reload_config();
  return 0;
}


int unload_module()
{
  ast_channel_unregister("SCCP");

  if (!ast_mutex_lock(&monlock)) {
    if (monitor_thread) {
      pthread_cancel(monitor_thread);
      pthread_kill(monitor_thread, SIGURG);
      pthread_join(monitor_thread, NULL);
    }
    monitor_thread = -2;
    ast_mutex_unlock(&monlock);
  } else {
    ast_log(LOG_WARNING, "Unable to lock the monitor\n");
    return -1;
  }

  return 0;
}

int usecount()
{
  int res;
  ast_mutex_lock(&usecnt_lock);
  res = usecnt;
  ast_mutex_unlock(&usecnt_lock);
  return res;
}

char *key() { return ASTERISK_GPL_KEY; }

char *description()
{
  return "Skinny Client Control Protocol (SCCP)";
}

