/*
 * CallWeaver Bluetooth Channel
 *
 * Author: Theo Zourzouvillys <theo@adaptive-it.co.uk>
 * Adaptive Linux Solutions <http://www.adaptive-it.co.uk>
 * Copyright (C) 2004 Adaptive Linux Solutions
 * 
 * Modifications by Chris Halls <halls@debian.org>
 *
 * callweaver'ized by Rico Gloeckner <mc+callweaver@ukeer.de>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 * TODO: clean up
 * TODO: add configure.ac correctly
 * TODO: own pin handler
 * TODO: maybe outsource some handling
 * TODO: documentation
 * TODO: 
 */

#include "chan_bluetooth.h"

/* ---------------------------------- */

/*! Bluetooth: Convert role enum into ascii string 
 * @param role role enum
 * @return role as ascii text (string)
 */

static const char *
role2str(blt_role_t role)
{
  switch (role) {
    case BLT_ROLE_HS:
      return "HS";
    case BLT_ROLE_AG:
      return "AG";
    case BLT_ROLE_NONE:
      return "??";
  }
  return ""; // fix compiler stupid warning.
}

/*! Bluetooth: convert status enum into ascii string 
 * @param status status enum
 * @return status as ascii text (string)
 */
static const char *
status2str(blt_status_t status)
{
  switch (status) {
    case BLT_STATUS_DOWN:
      return "Down";
    case BLT_STATUS_CONNECTING:
      return "Connecting";
    case BLT_STATUS_NEGOTIATING:
      return "Negotiating";
    case BLT_STATUS_READY:
      return "Ready";
    case BLT_STATUS_RINGING:
      return "Ringing";
    case BLT_STATUS_IN_CALL:
      return "InCall";
  };
  return "Unknown";
}

/*! Bluetooth: return last socket error 
 * @param fd fd to operate on
 * @return pointer to rc of getsockopt
 */
static int sock_err(int fd)
{
  int ret;
  socklen_t len = (socklen_t) sizeof(ret);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &len);
  return ret;
}


/*! Bluetooth: Parse clip
 * @param str 
 * @param number
 * @param nuber_len 
 * @param name
 * @param name_len
 * @param type
 * @return  currently unused
 */
static int parse_clip(const char * str, char *number, int number_len, char * name, int name_len, int *type)
{
  const char *c = str;
  const char *start;
  int length;
  char typestr[256];

  memset(number, 0, number_len);
  memset(name, 0, name_len);
  *type = 0;

  number[0] = '\0';
  name[0] = '\0';
  while(*c && *c != '"')
    c++;
  c++;
  start = c;
  while(*c && *c != '"')
    c++;
  length = c - start < number_len ? c - start : number_len;
  strncpy(number, start, length);
  number[length] = '\0';
  c++;
  while(*c && *c != ',')
    c++;
  c++;
  start = c;
  while(*c && *c != ',')
    c++;
  length = c - start < number_len ? c - start : number_len;
  strncpy(typestr, start, length);
  typestr[length] = '\0';
  *type = atoi(typestr);
  c++;
  while(*c && *c != ',')
    c++;
  c++;
  while(*c && *c != ',')
    c++;
  c++;
  while(*c && *c != '"')
    c++;
  c++;
  start = c;
  while(*c && *c != '"')
    c++;
  length = c - start < number_len ? c - start : number_len;
  strncpy(name, start, length);
  name[length] = '\0';

  return(1);
}


/*! Bluetooth: parse callsetup indication
 * @param str
 * @param name
 * @param name_len
 * @return
 */
static const char *
parse_cind(const char * str, char * name, int name_len)
{
  int c = 0;

  memset(name, 0, name_len);

  while (*str) {
    if (*str == '(') {
      if (++c == 1 && *(str+1) == '"') {
        const char * start = str + 2;
        int len = 0;
        str += 2;
        while (*str && *str != '"') {
          len++;
          str++;
        }
        if (len == 0)
          return NULL;
        strncpy(name, start, (len > name_len) ? name_len : len);
      }
    } else if (*str == ')')
      c--;
    else if (c == 0 && *str == ',')
      return str + 1;
    str++;
  }
  return NULL;
}


/*! Bluetooth: Set callsetup indication
 * @param dev
 * @param indicator
 * @param val
 * @return void
 */
static void
set_cind(blt_dev_t * dev, int indicator, int val)
{

  cw_log(LOG_DEBUG, "CIND %d set to %d\n", indicator, val);

  if (indicator == dev->callsetup_pos) {

    // call progress
    dev->callsetup = val;

    switch (val) {
      case 3:
        // Outgoing ringing
        if (dev->owner && dev->role == BLT_ROLE_AG)
          cw_queue_control(dev->owner, CW_CONTROL_RINGING);
        break;
      case 2:
        break;
      case 1:
        break;
      case 0:
        if (dev->owner && dev->role == BLT_ROLE_AG && dev->call == 0)
          cw_queue_control(dev->owner, CW_CONTROL_CONGESTION);
        break;
    }

  } else if (indicator == dev->service_pos) {

    // Signal

    if (val == 0)
      cw_log(LOG_NOTICE, "Audio Gateway %s lost signal\n", dev->name);
    else if (dev->service == 0 && val > 0)
      cw_log(LOG_NOTICE, "Audio Gateway %s got signal\n", dev->name);

    dev->service = val;

  } else if (indicator == dev->call_pos) {

    // Call
    dev->call = val;

    if (dev->owner) {
      if (val == 1) {
      	sco_start(dev, -1);
        cw_queue_control(dev->owner, CW_CONTROL_ANSWER);
      } else if (val == 0)
        cw_queue_control(dev->owner, CW_CONTROL_HANGUP);
    }
  }
}


/*! ???
 * @param ring
 * @param data
 * @param circular_len
 * @param pos
 * @param data_len
 * @return always 0
 */
static int set_buffer(char * ring, char * data, int circular_len, int * pos, int data_len)
{
  int start_pos = *(pos);
  int done = 0;
  int copy;

  while (data_len) {
    // Set can_do to the most we can do in this copy.

    copy = MIN(circular_len - start_pos, data_len);
    memcpy(ring + start_pos, data + done, copy);
    done += copy;
    start_pos += copy;
    data_len -= copy;

    if (start_pos == circular_len) {
      start_pos = 0;
    }
  }
  *(pos) = start_pos;
  return 0;
}

/*! ???
 * @param dest
 * @param ring
 * @param ring_size
 * @param head
 * @param to_copy Size of buffer to copy
 * @return always 0
 */
static int get_buffer(char * dst, char * ring, int ring_size, int * head, int to_copy)
{
  int copy;

  // |1|2|3|4|5|6|7|8|9|
  //      |-----|

  while (to_copy) {

    // Set can_do to the most we can do in this copy.
    copy = MIN(ring_size - *head, to_copy);

#if __BYTE_ORDER == __LITTLE_ENDIAN
    memcpy(dst, ring + *head, copy);
#else
    cw_swapcopy_samples(dst, ring+*head, copy/2);
#endif
    memset(ring+*head, 0, copy);
    dst += copy;
    *head += copy;
    to_copy -= copy;

    if (*head == ring_size ) {
	    *head = 0;
    }
  }
  return 0;
}

/**
 * Handle SCO audio sync.
 *
 * If we are the MASTER, then we control the timing,
 * in 48 byte chunks.  If we're the SLAVE, we send
 * as and when we recieve a packet.
 *
 * Because of packet/timing nessecity, we 
 * start up a thread when we're passing audio, so
 * that things are timed exactly right.
 *
 * sco_thread() is the function that handles it.
 *
 */
static void *
sco_thread(void * data)
{
  blt_dev_t * dev = (blt_dev_t*)data;
  int res;
  struct pollfd pfd[2];
  int in_pos = 0;
  int out_pos = 0;
  char c = 1;
  int sending;
  char buf[1024];
  int len;

  // Avoid deadlock in odd circumstances

  cw_log(LOG_WARNING, "SCO thread started on fd %d, pid %d\n", dev->sco, getpid());

  if (fcntl(dev->sco_pipe[1], F_SETFL, O_RDWR|O_NONBLOCK)) {
	  cw_log(LOG_WARNING, "fcntl failed on sco_pipe\n");
  }

  // dev->status = BLT_STATUS_IN_CALL;
  // cw_queue_control(dev->owner, AST_CONTROL_ANSWER);
  // Set buffer to silence, just incase.

  cw_mutex_lock(&(dev->sco_lock));

  memset(dev->sco_buf_in, 0, BUFLEN);
  memset(dev->sco_buf_out, 0, BUFLEN);

  dev->sco_pos_in  = 0;
  dev->sco_pos_out = 0;
  dev->sco_pos_inrcv = 0;
  dev->wakeread = 1;

  cw_mutex_unlock(&(dev->sco_lock));

  while (1) {

    cw_mutex_lock(&(dev->sco_lock));

    if (dev->sco_running != 1) {
      cw_log(LOG_DEBUG, "SCO stopped.\n");
      break;
    }

    pfd[0].fd = dev->sco;
    pfd[0].events = POLLIN;

    pfd[1].fd = dev->sco_pipe[1];
    pfd[1].events = POLLIN;

    cw_mutex_unlock(&(dev->sco_lock));

    res = poll(pfd, 2, 50);

    if (res == -1 && errno != EINTR) {
      cw_log(LOG_DEBUG, "SCO poll() error\n");
      break;
    }

    if (res == 0)
      continue;


    if (pfd[0].revents & POLLIN) {

      len = read(dev->sco, buf, 48);

      if (len) {
        cw_mutex_lock(&(dev->lock));

        if (dev->owner && dev->owner->_state == CW_STATE_UP) {
		cw_mutex_lock(&(dev->sco_lock));
		set_buffer(dev->sco_buf_in,  buf, BUFLEN, &in_pos,  len);
		dev->sco_pos_inrcv = in_pos;

		get_buffer(buf, dev->sco_buf_out, BUFLEN, &out_pos, len);
		if (write(dev->sco, buf, len) != len)
			cw_log(LOG_WARNING, "Wrote <48 to sco\n");

		if (dev->wakeread) {
			/* blt_read has caught up. Kick it */
			dev->wakeread = 0;
			if(write(dev->sco_pipe[1], &c, 1) != 1)
				cw_log(LOG_WARNING, "write to kick sco_pipe failed\n");
		}
		cw_mutex_unlock(&(dev->sco_lock));
	}
        cw_mutex_unlock(&(dev->lock));
      }

    } else if (pfd[0].revents) {

      int e = sock_err(pfd[0].fd);
      cw_log(LOG_ERROR, "SCO connection error: %s (errno %d)\n", strerror(e), e);
      break;

    } else if (pfd[1].revents & POLLIN) {

      int len;

      len = read(pfd[1].fd, &c, 1);
      sending = (sending) ? 0 : 1;

      cw_mutex_unlock(&(dev->sco_lock));

    } else if (pfd[1].revents) {

      int e = sock_err(pfd[1].fd);
      cw_log(LOG_ERROR, "SCO pipe connection event %d on pipe[1]=%d: %s (errno %d)\n", pfd[1].revents, pfd[1].fd, strerror(e), e);
      break;

    } else {
      cw_log(LOG_NOTICE, "Unhandled poll output\n");
      cw_mutex_unlock(&(dev->sco_lock));
    }

  }

  cw_mutex_lock(&(dev->lock));
  close(dev->sco);
  dev->sco = -1;
  dev->sco_running = -1;
  
  memset(dev->sco_buf_in, 0, BUFLEN);
  memset(dev->sco_buf_out, 0, BUFLEN);

  dev->sco_pos_in  = 0;
  dev->sco_pos_out = 0;
  dev->sco_pos_inrcv = 0;
  
  cw_mutex_unlock(&(dev->sco_lock));
  if (dev->owner)
    cw_queue_control(dev->owner, CW_CONTROL_HANGUP);
  cw_mutex_unlock(&(dev->lock));
  cw_log(LOG_DEBUG, "SCO thread stopped\n");
  return NULL;
}

/*! Bluetooth: Start SCO thread.  Must be called with dev->lock */
static int
sco_start(blt_dev_t * dev, int fd)
{

  if (dev->sco_pipe[1] <= 0) {
    cw_log(LOG_ERROR, "SCO pipe[1] == %d\n", dev->sco_pipe[1]);
    return -1;
  }

  cw_mutex_lock(&(dev->sco_lock));

  if (dev->sco_running != -1) {
    cw_log(LOG_ERROR, "Tried to start SCO thread while already running\n");
    cw_mutex_unlock(&(dev->sco_lock));
    return -1;
  }

  if (dev->sco == -1) {
    if (fd > 0) {
      dev->sco = fd;
    } else if (sco_connect(dev) != 0) {
      cw_log(LOG_ERROR, "SCO fd invalid\n");
      cw_mutex_unlock(&(dev->sco_lock));
      return -1;
    }
  }

  dev->sco_running = 1;

  if (cw_pthread_create(&(dev->sco_thread), NULL, sco_thread, dev) < 0) {
    cw_log(LOG_ERROR, "Unable to start SCO thread.\n");
    dev->sco_running = -1;
    cw_mutex_unlock(&(dev->sco_lock));
    return -1;
  }

  cw_mutex_unlock(&(dev->sco_lock));

  return 0;
}


/*! Bluetooth: Stop SCO thread.  Must be called with dev->lock */
static int
sco_stop(blt_dev_t * dev)
{
  cw_mutex_lock(&(dev->sco_lock));
  if (dev->sco_running == 1)
    dev->sco_running = 0;
  else
    dev->sco_running = -1;
  dev->sco_sending = 0;
  cw_mutex_unlock(&(dev->sco_lock));
  return 0;
}

/* Bluetooth: Answer the call.  Call with lock held on device */
static int
answer(blt_dev_t * dev)
{
  if ( 
    (!dev->owner) || (dev->ready != 1) || 
    (dev->status != BLT_STATUS_READY && dev->status != BLT_STATUS_RINGING)
  ) {
    cw_log(LOG_ERROR, 
      "Attempt to answer() in invalid state (owner=%p, ready=%d, status=%s)\n", 
      dev->owner, dev->ready, status2str(dev->status));
    return -1;
  }

    // dev->sd = sco_connect(&local_bdaddr, &(dev->bdaddr), NULL, NULL, 0);
    // dev->status = BLT_STATUS_IN_CALL;
    // dev->owner->fds[0] = dev->sd;
    //  if we are answering (hitting button):
  cw_queue_control(dev->owner, CW_CONTROL_ANSWER);
    //  if CW signals us to answer:
    // cw_setstate(cw, AST_STATE_UP);

  /* Start SCO link */
  sco_start(dev, -1);
  return 0;
}


/*! Bluetooth: write to BLT Device
 * @param chan cw_channel
 * @param frame cw_frame
 */
static int 
blt_write(struct cw_channel *chan, struct cw_frame * frame)
{
  blt_dev_t *dev = chan->tech_pvt; 

  /* We do want Voice data only */
  if (frame->frametype != CW_FRAME_VOICE) {
    cw_log(LOG_WARNING, 
        "Don't know what to do with frame type '%d'\n", frame->frametype);
    return 0;
  }

  /* and we need our format already */
  if (!(frame->subclass & BLUETOOTH_FORMAT)) {
    static int fish = 5;
    if (fish) {
      cw_log(LOG_WARNING, 
          "Cannot handle frames in format %d\n", frame->subclass);
      fish--;
    }
    return 0;
  }

  /* Channel had already hang up on us */
  if (chan->_state != CW_STATE_UP) {
    return 0;
  }

  /* lock and prealloc buffer we use */
  cw_mutex_lock(&(dev->sco_lock));
  set_buffer(dev->sco_buf_out, frame->data, BUFLEN, &(dev->sco_pos_out), MIN(frame->datalen, BUFLEN));
  cw_mutex_unlock(&(dev->sco_lock));

  return 0;

}

/*! Bluetooth: Read from BLT device
 * @param chan cw_channel
 * @return a single cw_frame
 */
static struct cw_frame *
blt_read(struct cw_channel *chan)
{
  blt_dev_t *dev = chan->tech_pvt;
  char c = 1;
  int len;
  static int fish = 0;

  /* Some nice norms */
  cw_fr_init_ex(&dev->fr, CW_FRAME_VOICE, BLUETOOTH_FORMAT, BLT_CHAN_NAME);
  dev->fr.mallocd = CW_MALLOCD_DATA;

  read(dev->sco_pipe[0], &c, 1);
  cw_mutex_lock(&(dev->sco_lock));
  dev->sco_sending = 1;

  /* Buffer wrapped? */
  if (dev->sco_pos_inrcv < dev->sco_pos_in)
  {
    /* Yep. Read only till the end */
    len = BUFLEN - dev->sco_pos_in + dev->sco_pos_inrcv;
  }
  else
  {
    len = dev->sco_pos_inrcv - dev->sco_pos_in;
  }
  dev->fr.data = malloc(CW_FRIENDLY_OFFSET+len) + CW_FRIENDLY_OFFSET;

  get_buffer(dev->fr.data, dev->sco_buf_in, BUFLEN, &(dev->sco_pos_in), len);
  dev->wakeread = 1;
  cw_mutex_unlock(&(dev->sco_lock));
  if (fish) {
    unsigned char *x = dev->fr.data;
    cw_log(LOG_WARNING, "blt_read  %d: %02x %02x %02x %02x %02x %02x\n",
    dev->fr.datalen, x[0], x[1], x[2], x[3], x[4], x[5]);
    fish--;
  }

  dev->fr.samples = len/2;
  dev->fr.datalen = len;
  dev->fr.offset = CW_FRIENDLY_OFFSET;
  return &dev->fr;
}

/*! Bluetooth: Escape Any '"' in str.  Return malloc()ed string */
static char *
escape_str(char * str)
{
  char * ptr = str;
  char * pret;
  char * ret;
  int len = 0;

  while (*ptr) {
    if (*ptr == '"')
      len++;
    len++;
    ptr++;
  }

  ret = malloc(len + 1);
  pret = memset(ret, 0, len + 1);

  ptr = str;
  
  while (*ptr) {
    if (*ptr == '"')
      *pret++ = '\\';
    *pret++ = *ptr++;
  }

  return ret;
}

/*! Bluetooth: Ring a headset
 */
static int
ring_hs(blt_dev_t * dev)
{

  cw_mutex_lock(&(dev->lock));

  if (dev->owner == NULL) {
    cw_mutex_unlock(&(dev->lock));
    return 0;
  } 

  dev->ringing = 1;
  dev->status = BLT_STATUS_RINGING;

  send_atcmd(dev, "RING");

  dev->owner->rings++;

  // FIXME '"' needs to be escaped in ELIP.

  if (dev->clip && dev->owner->cid.cid_num)
    send_atcmd(dev, "+CLIP: \"%s\",129", dev->owner->cid.cid_num);

  if (dev->elip && dev->owner->cid.cid_name) {
    char * esc = escape_str(dev->owner->cid.cid_name);
    send_atcmd(dev, "*ELIP: \"%s\"", esc);
    free(esc);
  }

  cw_mutex_unlock(&(dev->lock));

  return 1;
}


/*! Bluetooth: Try to call a Headset
 * If the HS is already connected, then just send RING, otherwise, things get a
 * little more sticky.  We first have to find the channel for HS using SDP, 
 * then intiate the connection.  Once we've done that, we can start the call.
 */
static int
blt_call(struct cw_channel *chan, char * dest, int timeout)
{
  blt_dev_t * dev = chan->tech_pvt;

  /* channel isnt there, abort */
  if (
    (chan->_state != CW_STATE_DOWN) && 
    (chan->_state != CW_STATE_RESERVED)
  ) {
    cw_log(LOG_WARNING, 
        "blt_call called on %s, neither down nor reserved\n", chan->name);
    return -1;
  }

  cw_log(LOG_DEBUG, "Calling %s on %s [t: %d]\n", dest, chan->name, timeout);

  /* interface is already locked (wtf?) */
  if (cw_mutex_lock(&iface_lock)) {
    cw_log(LOG_ERROR, "Failed to get iface_lock.\n");
    return -1;
  }

  /* the device isnt there */
  if (dev->ready == 0) {
    cw_log(LOG_WARNING, "Tried to call a device not ready/connected.\n");
    cw_setstate(chan, CW_CONTROL_CONGESTION);
    cw_mutex_unlock(&iface_lock);
    return 0;
  }

  /* connect to to headset ... */
  if (dev->role == BLT_ROLE_HS) {
    send_atcmd(dev, "+CIEV: 3,1");
    dev->ring_timer = cw_sched_add(sched, 5000, CW_SCHED_CB(ring_hs), dev);
    ring_hs(dev);

    cw_setstate(chan, CW_STATE_RINGING);
    cw_queue_control(chan, CW_CONTROL_RINGING);

  /* ... or rather cellphone ... */
  } else if (dev->role == BLT_ROLE_AG) {
    send_atcmd(dev, "ATD%s;", dev->dnid);
  
  /* ... or unknown device.  Aiiiieee! Abort! */
  } else {
    cw_setstate(chan, CW_CONTROL_CONGESTION);
    cw_log(LOG_ERROR, "Unknown device role\n");

  }

  cw_mutex_unlock(&iface_lock);
  return 0;
}


/*! Bluetooth: Hangup channel
 */
static int 
blt_hangup(struct cw_channel *chan)
{
  blt_dev_t * dev = chan->tech_pvt;

  cw_log(LOG_DEBUG, "blt_hangup(%s)\n", chan->name);

  if (!chan->tech_pvt) {
    cw_log(LOG_WARNING, "Asked to hangup channel not connected\n");
    return 0;
  }

  if (cw_mutex_lock(&iface_lock)) {
    cw_log(LOG_ERROR, "Failed to get iface_lock\n");
    return 0;
  }

  cw_mutex_lock(&(dev->lock));

  sco_stop(dev);
  dev->sco_sending = 0;

  if (dev->role == BLT_ROLE_HS) {
    if (dev->ringing == 0) {
	    //
      // Actual call in progress
      send_atcmd(dev, "+CIEV: 2,0");
    } else {

      // Just ringing still
      if (dev->role == BLT_ROLE_HS)
        send_atcmd(dev, "+CIEV: 3,0");

      if (dev->ring_timer >= 0)
        cw_sched_del(sched, dev->ring_timer);

      dev->ring_timer = -1;
      dev->ringing = 0;
    } /* if dev->ringing */

  } else if (dev->role == BLT_ROLE_AG) {

    // Cancel call.
    send_atcmd(dev, "ATH");
    send_atcmd(dev, "AT+CHUP");
  } /* if dev->role */

  if (dev->status == BLT_STATUS_IN_CALL || dev->status == BLT_STATUS_RINGING)
    dev->status = BLT_STATUS_READY;

  chan->tech_pvt = NULL;
  dev->owner = NULL;
  cw_mutex_unlock(&(dev->lock));
  cw_setstate(chan, CW_STATE_DOWN);
  cw_mutex_unlock(&(iface_lock));

  return 0;
}

/*! Bluetooth: indicate call connection
 */
static int 
blt_indicate(struct cw_channel *c, int condition)
{
  cw_log(LOG_DEBUG, "blt_indicate (%d)\n", condition);

  switch(condition) {
    case CW_CONTROL_RINGING:
      return -1;
    default:
      cw_log(LOG_WARNING, "Don't know how to condition %d\n", condition);
      break;
  }
  return -1;
}

/*! Bluetooth: answer call */
static int
blt_answer(struct cw_channel *chan)
{
  blt_dev_t * dev = chan->tech_pvt;

  cw_mutex_lock(&dev->lock);

  cw_log(LOG_DEBUG, "Answering interface\n");

  if (chan->_state != CW_STATE_UP) {
    send_atcmd(dev, "+CIEV: 2,1");
    send_atcmd(dev, "+CIEV: 3,0");
    sco_start(dev, -1);
    cw_setstate(chan, CW_STATE_UP);
  }

  cw_mutex_unlock(&dev->lock);

  return 0;
}

/*! Bluetooth: create new channel */
static struct cw_channel *
blt_new(blt_dev_t *dev, int state, const char *context, const char *number)
{
  struct cw_channel *chan;
  char c = 0;

  if ((chan = cw_channel_alloc(1)) == NULL) {
    cw_log(LOG_WARNING, "Unable to allocate channel structure\n");
    return NULL;
  }

  snprintf(chan->name, sizeof(chan->name), "BLT/%s", dev->name);

  chan->nativeformats       = BLUETOOTH_FORMAT;
  chan->rawreadformat  = BLUETOOTH_FORMAT;
  chan->rawwriteformat = BLUETOOTH_FORMAT;
  chan->writeformat         = BLUETOOTH_FORMAT;
  chan->readformat          = BLUETOOTH_FORMAT;

  cw_setstate(chan, state);

  chan->type = BLT_CHAN_NAME;

  chan->tech_pvt = dev;
  chan->tech = &blt_tech;
  strncpy(chan->context, context, sizeof(chan->context)-1);
  strncpy(chan->exten,   number,  sizeof(chan->exten) - 1);
  if(0 == strcmp(number, "s")) {
    cw_set_callerid(chan, dev->cid_num, dev->cid_name, dev->cid_num);
  }
 
  chan->language[0] = '\0';

  chan->fds[0] = dev->sco_pipe[0];
  write(dev->sco_pipe[1], &c, 1);

  dev->owner = chan;

  cw_mutex_lock(&usecnt_lock);
  usecnt++;
  cw_mutex_unlock(&usecnt_lock);

  cw_update_use_count();

  if (state != CW_STATE_DOWN) {
    if (cw_pbx_start(chan)) {
      cw_log(LOG_WARNING, "Unable to start PBX on %s\n", chan->name);
      cw_hangup(chan);
    }
  }

  return chan;
}

static struct cw_channel *
blt_request(const char * type, int format, void * local_data, int *cause)
{
  char * data = (char*)local_data;
  int oldformat;
  blt_dev_t * dev = NULL;
  struct cw_channel * chan = NULL;
  char * number = data, * dname;

  dname = strsep(&number, "/");

  oldformat = format;

  format &= BLUETOOTH_FORMAT;

  if (!format) {
    cw_log(LOG_NOTICE, "Asked to get a channel of unsupported format '%d'\n", oldformat);
    return NULL;
  }

  cw_log(LOG_DEBUG, "Dialing '%s' via '%s'\n", number, dname);

  if (cw_mutex_lock(&iface_lock)) {
    cw_log(LOG_ERROR, "Unable to lock iface_list\n");
    return NULL;
  }

  dev = iface_head;

  while (dev) {
    if (strcmp(dev->name, dname) == 0) {
      cw_mutex_lock(&(dev->lock));
      if (!dev->ready) {
        cw_log(LOG_ERROR, "Device %s is not connected\n", dev->name);
        cw_mutex_unlock(&(dev->lock));
        cw_mutex_unlock(&iface_lock);
        return NULL;
      }
      break;
    }
    dev = dev->next;
  }

  cw_mutex_unlock(&iface_lock);

  if (!dev) {
    cw_log(LOG_WARNING, "Failed to find device named '%s'\n", dname);
    return NULL;
  }

  if (number && dev->role != BLT_ROLE_AG) {
    cw_log(LOG_WARNING, "Tried to send a call out on non AG\n");
    cw_mutex_unlock(&(dev->lock));
    return NULL;
  }

  if (dev->role == BLT_ROLE_AG)
    strncpy(dev->dnid, number, sizeof(dev->dnid) - 1);

  chan = blt_new(dev, CW_STATE_DOWN, dev->context, "s");

  cw_mutex_unlock(&(dev->lock));

  return chan;
}


/* ---- AT COMMAND SOCKET STUFF ---- */

/*! Bluetooth: send at cmd to BLT device */
static int
send_atcmd(blt_dev_t * dev, const char * fmt, ...)
{
  char buf[1024];
  va_list ap;
  int len;

  va_start(ap, fmt);
  len = vsnprintf(buf, 1023, fmt, ap);
  va_end(ap);

  if (option_verbose)
    cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s < %s\n", role2str(dev->role), 10, dev->name, buf);

  write(dev->rd, "\r\n", 2);
  len = write(dev->rd, buf, len);
  write(dev->rd, "\r\n", 2);
  return (len) ? 0 : -1;
}


static int
send_atcmd_ok(blt_dev_t * dev, const char * cmd)
{
  int len;
  strncpy(dev->last_ok_cmd, cmd, BLT_RDBUFF_MAX - 1);
  if (option_verbose)
    cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s < OK\n", role2str(dev->role), 10, dev->name);
  len = write(dev->rd, "\r\nOK\r\n", 6);
  return (len) ? 0 : -1;
}

static int
send_atcmd_error(blt_dev_t * dev)
{
  int len;

  if (option_verbose)
    cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s < ERROR\n", role2str(dev->role), 10, dev->name);

  write(dev->rd, "\r\n", 2);
  len = write(dev->rd, "ERROR", 5);
  write(dev->rd, "\r\n", 2);

  return (len) ? 0 : -1;
}


/* -- Handle negotiation when we're an AG -- */
static int
atcmd_brsf_set(blt_dev_t * dev, const char * arg, int len)
{
  cw_log(LOG_DEBUG, "Device Supports: %s\n", arg);
  dev->brsf = atoi(arg);
  send_atcmd(dev, "+BRSF: %d", 23);
  return 0;
}


/* Bluetooth Voice Recognition */
static int
atcmd_bvra_set(blt_dev_t * dev, const char * arg, int len)
{
  cw_log(LOG_WARNING, "+BVRA Not Yet Supported\n");
  // Do not do anything further
  return -1;
}

/* Clock */
static int
atcmd_cclk_read(blt_dev_t * dev)
{
  struct tm t, *tp;
  const time_t ti = time(0);
  tp = localtime_r(&ti, &t);
  send_atcmd(dev, "+CCLK: \"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"", 
                  (tp->tm_year % 100), (tp->tm_mon + 1), (tp->tm_mday),
                  tp->tm_hour, tp->tm_min, tp->tm_sec, ((tp->tm_gmtoff / 60) / 15));
  return 0;
}

/*! CHUP - Hangup Call */
static int
atcmd_chup_execute(blt_dev_t * dev, const char * data)
{
  if (!dev->owner) {
    cw_log(LOG_ERROR, "Request to hangup call when none in progress\n");
    return -1;
  }
  cw_log(LOG_DEBUG, "Hangup Call\n");
  cw_queue_control(dev->owner, CW_CONTROL_HANGUP);
  return 0;
}

/* CIND - Call Indicator */
static int
atcmd_cind_read(blt_dev_t * dev)
{
  send_atcmd(dev, "+CIND: 1,0,0");
  return 0;
}

static int
atcmd_cind_test(blt_dev_t * dev)
{
  send_atcmd(dev, "+CIND: (\"service\",(0,1)),(\"call\",(0,1)),(\"callsetup\",(0-4))");
  return 0;
}

/* CHLD - Call hold and multiparty */
static int
atcmd_chld_test(blt_dev_t * dev)
{
  send_atcmd(dev, "+CHLD: (0,1,2,3,4)");
  return 0;
}

/* Set Language */
static int
atcmd_clan_read(blt_dev_t * dev)
{
  send_atcmd(dev, "+CLAN: \"en\"");
  return 0;
}

/* Caller Id Presentation */
static int
atcmd_clip_set(blt_dev_t * dev, const char * arg, int len)
{
  dev->clip = atoi(arg);
  return 0;
}

/* Connected Line Identification Presentation */
static int
atcmd_colp_set(blt_dev_t * dev, const char * arg, int len)
{
  dev->colp = atoi(arg);
  return 0;
}

/* Call waiting event reporting */
static int
atcmd_ccwa_set(blt_dev_t * dev, const char * arg, int len)
{
  dev->ccwa = atoi(arg);
  return 0;
}


/* CMER - Mobile Equipment Event Reporting */
static int
atcmd_cmer_set(blt_dev_t * dev, const char * arg, int len)
{
  dev->ready = 1;
  dev->status = BLT_STATUS_READY;
  return 0;
}

/* PhoneBook Types:
 *
 *  - FD - SIM Fixed Dialing Phone Book
 *  - ME - ME Phone book
 *  - SM - SIM Phone Book
 *  - DC - ME dialled-calls list
 *  - RC - ME recieved-calls lisr
 *  - MC - ME missed-calls list
 *  - MV - ME Voice Activated Dialing List
 *  - HP - Hierachial Phone Book
 *  - BC - Own Business Card (PIN2 required)
 *
 */

/* Read Phone Book Entry */
static int
atcmd_cpbr_set(blt_dev_t * dev, const char * arg, int len)
{
  // XXX:T: Fix the phone book!
  // * Maybe add res_phonebook or something? */
  send_atcmd(dev, "+CPBR: %d,\"%s\",128,\"%s\"", atoi(arg), arg, arg);
  return 0;
}

/* Select Phone Book */
static int
atcmd_cpbs_set(blt_dev_t * dev, const char * arg, int len)
{
  // XXX:T: I guess we'll just accept any?
  return 0;
}

static int
atcmd_cscs_set(blt_dev_t * dev, const char * arg, int len)
{
  // XXX:T: Language
  return 0;
}

static int
atcmd_eips_set(blt_dev_t * dev, const char * arg, int len)
{
  cw_log(LOG_DEBUG, "Identify Presentation Set: %s=%s\n",
                         (*(arg) == 49) ? "ELIP" : "EOLP",
                         (*(arg+2) == 49) ? "ON" : "OFF");

  if (*(arg) == 49)
    dev->eolp = (*(arg+2) == 49) ? 1 : 0;
  else
    dev->elip = (*(arg+2) == 49) ? 1 : 0;

  return 0;
}

/* VGS - Speaker Volume Gain */
static int
atcmd_vgs_set(blt_dev_t * dev, const char * arg, int len)
{
  dev->gain_speaker = atoi(arg);
  return 0;
}

/* Dial */
static int
atcmd_dial_execute(blt_dev_t * dev, const char * data)
{
  char * number = NULL;

  /* Make sure there is a ';' at the end of the line */
  if (*(data + (strlen(data) - 1)) != ';') {
    cw_log(LOG_WARNING, "Can't dial non-voice right now: %s\n", data);
    return -1;
  }

  number = strndup(data, strlen(data) - 1);
  cw_log(LOG_NOTICE, "Dial: [%s]\n", number);

  send_atcmd(dev, "+CIEV: 2,1");
  send_atcmd(dev, "+CIEV: 3,0");

  sco_start(dev, -1);

  if (blt_new(dev, CW_STATE_UP, dev->context, number) == NULL) {
    sco_stop(dev);
  }

  free(number);

  return 0;
}

static int atcmd_bldn_execute(blt_dev_t * dev, const char *data)
{
	return atcmd_dial_execute(dev, "bldn;");
}

/* Answer */
static int
atcmd_answer_execute(blt_dev_t * dev, const char * data)
{

  if (!dev->ringing || !dev->owner) {
    cw_log(LOG_WARNING, "Can't answer non existant call\n");
    return -1;
  }

  dev->ringing = 0;

  if (dev->ring_timer >= 0)
    cw_sched_del(sched, dev->ring_timer);

  dev->ring_timer = -1;

  send_atcmd(dev, "+CIEV: 2,1");
  send_atcmd(dev, "+CIEV: 3,0");

  return answer(dev);
}

static int
ag_unsol_ciev(blt_dev_t * dev, const char * data)
{
  const char * orig = data;
  int indicator;
  int status;

  while (*(data) && *(data) == ' ')
    data++;

  if (*(data) == 0) {
    cw_log(LOG_WARNING, "Invalid value[1] for '+CIEV:%s'\n", orig);
    return -1;
  }

  indicator = *(data++) - 48;

  if (*(data++) != ',') {
    cw_log(LOG_WARNING, "Invalid value[2] for '+CIEV:%s'\n", orig);
    return -1;
  }

  if (*(data) == 0) {
    cw_log(LOG_WARNING, "Invalid value[3] for '+CIEV:%s'\n", orig);
    return -1;
  }

  status = *(data) - 48;

  set_cind(dev, indicator, status);

  return 0;
}

static int
ag_unsol_cind(blt_dev_t * dev, const char * data)
{

  while (*(data) && *(data) == ' ')
    data++;


  if (dev->cind == 0)
  {
    int pos = 1;
    char name[1024];

    while ((data = parse_cind(data, name, 1023)) != NULL) {
      cw_log(LOG_DEBUG, "CIND: %d=%s\n", pos, name);
      if (strcmp(name, "call") == 0)
        dev->call_pos = pos;
      else if (strcmp(name, "service") == 0)
        dev->service_pos = pos;
      else if (strcmp(name, "call_setup") == 0 || strcmp(name, "callsetup") == 0)
        dev->callsetup_pos = pos;
      pos++;
    }

    cw_log(LOG_DEBUG, "CIND: %d=%s\n", pos, name);

  } else {

    int pos = 1, len = 0;
    char val[128];
    const char * start = data;

    while (*data) {
      if (*data == ',') {
        memset(val, 0, 128);
        strncpy(val, start, len);
        set_cind(dev, pos, atoi(val));
        pos++;
        len = 0;
        data++;
        start = data;
        continue;
      }
      len++;
      data++;
    }

    memset(val, 0, 128);
    strncpy(val, start, len);
    cw_log(LOG_DEBUG, "CIND IND %d set to %d [%s]\n", pos, atoi(val), val);
  }

  return 0;
}

/*
 * handle an incoming call
 */
static int
ag_unsol_clip(blt_dev_t * dev, const char * data)
{
  const char * orig = data;
  char name[256];
  char number[64];
  int type;

  while (*(data) && *(data) == ' ')
    data++;

  if (*(data) == 0) {
    cw_log(LOG_WARNING, "Invalid value[1] for '+CLIP:%s'\n", orig);
    return -1;
  }

  parse_clip(data, number, sizeof(number)-1, name, sizeof(name)-1, &type);
  cw_log(LOG_NOTICE, "Parsed '+CLIP: %s' number='%s' type='%d' name='%s'\n", data, number, type, name);

  blt_new(dev, CW_STATE_RING, dev->context, "s");

  return 0;
}


/* -- Handle negotiation when we're a HS -- */
static void ag_unknown_response(blt_dev_t * dev, char * cmd)
{
  cw_log(LOG_DEBUG, "Got UNKN response: %s\n", cmd);

  // DELAYED
  // NO CARRIER

}

static void ag_cgmi_response(blt_dev_t * dev, char * cmd)
{
  // CGMM - Phone Model
  // CGMR - Phone Revision
  // CGSN - IMEI
  // AT*
  // VTS - send tone
  // CREG
  // CBC - BATTERY
  // CSQ - SIGANL
  // CSMS - SMS STUFFS
  //  CMGL
  //  CMGR
  //  CMGS
  // CSCA - sms CENTER NUMBER
  // CNMI - SMS INDICATION
  // ast_log(LOG_DEBUG, "Manufacturer: %s\n", cmd);
  dev->cb = ag_unknown_response;
}

static void ag_cgmi_valid_response(blt_dev_t * dev, char * cmd)
{
  // send_atcmd(dev, "AT+WS46?");
  // send_atcmd(dev, "AT+CRC=1");
  // send_atcmd(dev, "AT+CNUM");

  if (strcmp(cmd, "OK") == 0) {
    send_atcmd(dev, "AT+CGMI");
    dev->cb = ag_cgmi_response;
  } else {
    dev->cb = ag_unknown_response;
  }
}

static void ag_clip_response(blt_dev_t * dev, char * cmd)
{
  send_atcmd(dev, "AT+CGMI=?");
  dev->cb = ag_cgmi_valid_response;
}

static void ag_cmer_response(blt_dev_t * dev, char * cmd)
{
  dev->cb = ag_clip_response;
  dev->ready = 1;
  dev->status = BLT_STATUS_READY;
  send_atcmd(dev, "AT+CLIP=1");
}

static void ag_cind_status_response(blt_dev_t * dev, char * cmd)
{
  // XXX:T: Handle response.
  dev->cb = ag_cmer_response;
  send_atcmd(dev, "AT+CMER=3,0,0,1");
  // Initiase SCO link!
}

static void ag_cind_response(blt_dev_t * dev, char * cmd)
{
  dev->cb = ag_cind_status_response;
  dev->cind = 1;
  send_atcmd(dev, "AT+CIND?");
}

static void ag_brsf_response(blt_dev_t * dev, char * cmd)
{
  dev->cb = ag_cind_response;
  cw_log(LOG_DEBUG, "Bluetooth features: %s\n", cmd);
  dev->cind = 0;
  send_atcmd(dev, "AT+CIND=?");
}

/* ---------------------------------- */

static int
sdp_register(sdp_session_t * session)
{
  // XXX:T: Fix this horrible function so it makes some sense and is extensible!
  sdp_list_t *svclass_id, *pfseq, *apseq, *root;
  uuid_t root_uuid, svclass_uuid, ga_svclass_uuid, l2cap_uuid, rfcomm_uuid;
  sdp_profile_desc_t profile;
  sdp_list_t *aproto, *proto[2];
  sdp_record_t record;
  uint8_t u8 = rfcomm_channel_ag;
  uint8_t u8_hs = rfcomm_channel_hs;
  sdp_data_t *channel;
  int ret = 0;

  memset((void *)&record, 0, sizeof(sdp_record_t));
  record.handle = 0xffffffff;
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  root = sdp_list_append(0, &root_uuid);
  sdp_set_browse_groups(&record, root);

  // Register as an AG

  sdp_uuid16_create(&svclass_uuid, HANDSFREE_AUDIO_GW_SVCLASS_ID);
  svclass_id = sdp_list_append(0, &svclass_uuid);
  sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
  svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
  sdp_set_service_classes(&record, svclass_id);
  sdp_uuid16_create(&profile.uuid, 0x111f);
  profile.version = 0x0100;
  pfseq = sdp_list_append(0, &profile);

  sdp_set_profile_descs(&record, pfseq);

  sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
  proto[0] = sdp_list_append(0, &l2cap_uuid);
  apseq = sdp_list_append(0, proto[0]);

  sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
  proto[1] = sdp_list_append(0, &rfcomm_uuid);
  channel = sdp_data_alloc(SDP_UINT8, &u8);
  proto[1] = sdp_list_append(proto[1], channel);
  apseq = sdp_list_append(apseq, proto[1]);

  aproto = sdp_list_append(0, apseq);
  sdp_set_access_protos(&record, aproto);

  sdp_set_info_attr(&record, "Voice Gateway", 0, 0);

  if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
    cw_log(LOG_ERROR, "Service Record registration failed\n");
    ret = -1;
    goto end;
  }

  sdp_record_ag = record.handle;

  cw_log(LOG_NOTICE, "HeadsetAudioGateway service registered\n");

  sdp_data_free(channel);
  sdp_list_free(proto[0], 0);
  sdp_list_free(proto[1], 0);
  sdp_list_free(apseq, 0);
  sdp_list_free(aproto, 0);

  // -------------

  memset((void *)&record, 0, sizeof(sdp_record_t));
  record.handle = 0xffffffff;
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  root = sdp_list_append(0, &root_uuid);
  sdp_set_browse_groups(&record, root);

  // Register as an HS

  sdp_uuid16_create(&svclass_uuid, HANDSFREE_AUDIO_GW_SVCLASS_ID);
  svclass_id = sdp_list_append(0, &svclass_uuid);
  sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
  svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
  sdp_set_service_classes(&record, svclass_id);
  sdp_uuid16_create(&profile.uuid, 0x111e);
  profile.version = 0x0100;
  pfseq = sdp_list_append(0, &profile);
  sdp_set_profile_descs(&record, pfseq);

  sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
  proto[0] = sdp_list_append(0, &l2cap_uuid);
  apseq = sdp_list_append(0, proto[0]);

  sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
  proto[1] = sdp_list_append(0, &rfcomm_uuid);
  channel = sdp_data_alloc(SDP_UINT8, &u8_hs);
  proto[1] = sdp_list_append(proto[1], channel);
  apseq = sdp_list_append(apseq, proto[1]);

  aproto = sdp_list_append(0, apseq);
  sdp_set_access_protos(&record, aproto);
  sdp_set_info_attr(&record, "Voice Gateway", 0, 0);

  if (sdp_record_register(session, &record, SDP_RECORD_PERSIST) < 0) {
    cw_log(LOG_ERROR, "Service Record registration failed\n");
    ret = -1;
    goto end;
  }

  sdp_record_hs = record.handle;

  cw_log(LOG_NOTICE, "HeadsetAudioGateway service registered\n");

end:
  sdp_data_free(channel);
  sdp_list_free(proto[0], 0);
  sdp_list_free(proto[1], 0);
  sdp_list_free(apseq, 0);
  sdp_list_free(aproto, 0);

  return ret;
}

static int
rfcomm_listen(bdaddr_t * bdaddr, int channel)
{

  int sock = -1;
  struct sockaddr_rc loc_addr;
  int on = 1;

  if ((sock = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
    cw_log(LOG_ERROR, "Can't create socket: %s (errno: %d)\n", strerror(errno), errno);
    return -1;
  }

  loc_addr.rc_family = AF_BLUETOOTH;

  /* Local Interface Address */
  bacpy(&loc_addr.rc_bdaddr, bdaddr);

  /* Channel */
  loc_addr.rc_channel = channel;

  if (bind(sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) < 0) {
    cw_log(LOG_ERROR, "Can't bind socket: %s (errno: %d)\n", strerror(errno), errno);
    close(sock);
    return -1;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
    cw_log(LOG_ERROR, "Set socket SO_REUSEADDR option on failed: errno %d, %s", errno, strerror(errno));
    close(sock);
    return -1;
  }

  if (fcntl(sock, F_SETFL, O_RDWR|O_NONBLOCK) != 0)
    cw_log(LOG_ERROR, "Can't set RFCOMM socket to NBIO\n");

  if (listen(sock, 10) < 0) {
    cw_log(LOG_ERROR,"Can not listen on the socket. %s(%d)\n", strerror(errno), errno);
    close(sock);
    return -1;
  }

  cw_log(LOG_NOTICE, "Listening for RFCOMM channel %d connections on FD %d\n", channel, sock);

  return sock;
}


static int
sco_listen(bdaddr_t * bdaddr)
{
  int sock = -1;
  int on = 1;
  struct sockaddr_sco loc_addr;

  memset(&loc_addr, 0, sizeof(loc_addr));

  if ((sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO)) < 0) {
    cw_log(LOG_ERROR, "Can't create SCO socket: %s (errno: %d)\n", strerror(errno), errno);
    return -1;
  }

  loc_addr.sco_family = AF_BLUETOOTH;
  bacpy(&loc_addr.sco_bdaddr, BDADDR_ANY);

  if (bind(sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) < 0) {
    cw_log(LOG_ERROR, "Can't bind SCO socket: %s (errno: %d)\n", strerror(errno), errno);
    close(sock);
    return -1;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
    cw_log(LOG_ERROR, "Set SCO socket SO_REUSEADDR option on failed: errno %d, %s", errno, strerror(errno));
    close(sock);
    return -1;
  }

  if (fcntl(sock, F_SETFL, O_RDWR|O_NONBLOCK) != 0)
    cw_log(LOG_ERROR, "Can't set SCO socket to NBIO\n");

  if (listen(sock, 10) < 0) {
    cw_log(LOG_ERROR,"Can not listen on SCO socket: %s(%d)\n", strerror(errno), errno);
    close(sock);
    return -1;
  }

  cw_log(LOG_NOTICE, "Listening for SCO connections on FD %d\n", sock);

  return sock;
}

static int
rfcomm_connect(bdaddr_t * src, bdaddr_t * dst, int channel, int nbio)
{
  struct sockaddr_rc addr;
  int s;

  if ((s = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.rc_family = AF_BLUETOOTH;
  bacpy(&addr.rc_bdaddr, src);
  addr.rc_channel = 0;

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(s);
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.rc_family = AF_BLUETOOTH;
  bacpy(&addr.rc_bdaddr, dst);
  addr.rc_channel = channel;

  if (nbio) {
    if (fcntl(s, F_SETFL, O_RDWR|O_NONBLOCK) != 0)
      cw_log(LOG_ERROR, "Can't set RFCOMM socket to NBIO\n");
  }

  if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0 && (nbio != 1 || (errno != EAGAIN))) {
    close(s);
    return -1;
  }

  return s;
}

/* Must be called with dev->lock held */
static int
sco_connect(blt_dev_t * dev)
{
  struct sockaddr_sco addr;
  // struct sco_conninfo conn;
  // struct sco_options opts;
  // int size;
  // bdaddr_t * src = &local_bdaddr;

  int s;
  bdaddr_t * dst = &(dev->bdaddr);

  if (dev->sco != -1) {
    cw_log(LOG_ERROR, "SCO fd already open.\n");
    return -1;
  }

  if ((s = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO)) < 0) {
    cw_log(LOG_ERROR, "Can't create SCO socket(): %s\n", strerror(errno));
    return -1;
  }

  memset(&addr, 0, sizeof(addr));

  addr.sco_family = AF_BLUETOOTH;
  bacpy(&addr.sco_bdaddr, BDADDR_ANY);

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cw_log(LOG_ERROR, "Can't bind() SCO socket: %s\n", strerror(errno));
    close(s);
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sco_family = AF_BLUETOOTH;
  bacpy(&addr.sco_bdaddr, dst);

  if (fcntl(s, F_SETFL, O_RDWR|O_NONBLOCK) != 0)
    cw_log(LOG_ERROR, "Can't set SCO socket to NBIO\n");

  if ((connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) && (errno != EAGAIN)) {
    cw_log(LOG_ERROR, "Can't connect() SCO socket: %s (errno %d)\n", strerror(errno), errno);
    close(s);
    return -1;
  }

  cw_log(LOG_DEBUG, "SCO: %d\n", s);
  dev->sco = s;

  return 0;
}


/* Non blocking (async) outgoing bluetooth connection */
static int
try_connect(blt_dev_t * dev)
{
  int fd;
  cw_mutex_lock(&(dev->lock));

  if (dev->status != BLT_STATUS_CONNECTING && dev->status != BLT_STATUS_DOWN) {
    cw_mutex_unlock(&(dev->lock));
    return 0;
  }

  if (dev->rd != -1) {

    int ret;
    struct pollfd pfd;

    if (dev->status != BLT_STATUS_CONNECTING) {
      cw_mutex_unlock(&(dev->lock));
      dev->outgoing_id = -1;
      return 0;
    }

    // ret = connect(dev->rd, (struct sockaddr *)&(dev->addr), sizeof(struct sockaddr_rc)); // 

    pfd.fd = dev->rd;
    pfd.events = POLLIN | POLLOUT;

    ret = poll(&pfd, 1, 0);

    if (ret == -1) {
      close(dev->rd);
      dev->rd = -1;
      dev->status = BLT_STATUS_DOWN;
      dev->outgoing_id = cw_sched_add(sched, 1000, CW_SCHED_CB(try_connect), dev);
      cw_mutex_unlock(&(dev->lock));
      return 0;
    }

    if (ret > 0) {

      socklen_t len = (socklen_t) sizeof(ret);
      getsockopt(dev->rd, SOL_SOCKET, SO_ERROR, &ret, &len);

      if (ret == 0) {

        cw_log(LOG_NOTICE, "Initialised bluetooth link to device %s\n", dev->name); 

        dev->status = BLT_STATUS_NEGOTIATING;

        /* If this device is a AG, we initiate the negotiation. */

        if (dev->role == BLT_ROLE_AG) {
          dev->cb = ag_brsf_response;
          send_atcmd(dev, "AT+BRSF=23");
        }

        dev->outgoing_id = -1;
        cw_mutex_unlock(&(dev->lock));
        return 0;

      } else {

        if (ret != EHOSTDOWN)
          cw_log(LOG_NOTICE, "Connect to device %s failed: %s (errno %d)\n", dev->name, strerror(ret), ret);

        close(dev->rd);
        dev->rd = -1;
        dev->status = BLT_STATUS_DOWN;
        dev->outgoing_id = cw_sched_add(sched, (ret == EHOSTDOWN) ? 10000 : 2500, CW_SCHED_CB(try_connect), dev);
        cw_mutex_unlock(&(dev->lock));
        return 0;

      }

    }

    dev->outgoing_id = cw_sched_add(sched, 100, CW_SCHED_CB(try_connect), dev);
    cw_mutex_unlock(&(dev->lock));
    return 0;
  }

  fd = rfcomm_connect(&local_bdaddr, &(dev->bdaddr), dev->channel, 1);

  if (fd == -1) {
    cw_log(LOG_WARNING, "NBIO connect() to %s returned %d: %s\n", dev->name, errno, strerror(errno));
    dev->outgoing_id = cw_sched_add(sched, 5000, CW_SCHED_CB(try_connect), dev);
    cw_mutex_unlock(&(dev->lock));
    return 0;
  }

  dev->rd = fd;
  dev->status = BLT_STATUS_CONNECTING;
  dev->outgoing_id = cw_sched_add(sched, 100, CW_SCHED_CB(try_connect), dev);
  cw_mutex_unlock(&(dev->lock));
  return 0;
}


/* Called whenever a new command is recieved while we're the AG */


static int
process_rfcomm_cmd(blt_dev_t * dev, char * cmd)
{
  int i;
  char * fullcmd = cmd;

  if (option_verbose)
    cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s > %s\n", role2str(dev->role), 10, dev->name, cmd);

  /* Read the 'AT' from the start of the string */
  if (strncmp(cmd, "AT", 2)) {
    cw_log(LOG_WARNING, "Unknown command without 'AT': %s\n", cmd);
    send_atcmd_error(dev);
    return 0;
  }

  cmd += 2;

  // Don't forget 'AT' on its own is OK.

  if (strlen(cmd) == 0) {
    send_atcmd_ok(dev, fullcmd);
    return 0;
  }

  for (i = 0 ; i < ATCMD_LIST_LEN ; i++) {
    if (strncmp(atcmd_list[i].str, cmd, strlen(atcmd_list[i].str)) == 0) {
      char * pos = (cmd + strlen(atcmd_list[i].str));
      if ((strncmp(pos, "=?", 2) == 0) && (strlen(pos) == 2)) {
        /* TEST command */
        if (atcmd_list[i].test) {
          if (atcmd_list[i].test(dev) == 0)
            send_atcmd_ok(dev, fullcmd);
          else
            send_atcmd_error(dev);
        } else {
          send_atcmd_ok(dev, fullcmd);
        }
      } else if ((strncmp(pos, "?", 1) == 0) && (strlen(pos) == 1)) {
        /* READ command */
        if (atcmd_list[i].read) {
          if (atcmd_list[i].read(dev) == 0)
            send_atcmd_ok(dev, fullcmd);
          else
            send_atcmd_error(dev);
        } else {
          cw_log(LOG_WARNING, "AT Command: '%s' missing READ function\n", fullcmd);
          send_atcmd_error(dev);
        }
      } else if (strncmp(pos, "=", 1) == 0) {
        /* SET command */
        if (atcmd_list[i].set) {
          if (atcmd_list[i].set(dev, (pos + 1), (*(pos + 1)) ? strlen(pos + 1) : 0) == 0)
            send_atcmd_ok(dev, fullcmd);
          else
            send_atcmd_error(dev);
        } else {
          cw_log(LOG_WARNING, "AT Command: '%s' missing SET function\n", fullcmd);
          send_atcmd_error(dev);
        }
      } else {
        /* EXECUTE command */
        if (atcmd_list[i].execute) {
          if (atcmd_list[i].execute(dev, cmd + strlen(atcmd_list[i].str)) == 0)
            send_atcmd_ok(dev, fullcmd);
          else
            send_atcmd_error(dev);
        } else {
          cw_log(LOG_WARNING, "AT Command: '%s' missing EXECUTE function\n", fullcmd);
          send_atcmd_error(dev);
        }
      }
      return 0;
    }
  }

  cw_log(LOG_WARNING, "Unknown AT Command: '%s' (%s)\n", fullcmd, cmd);
  send_atcmd_error(dev);

  return 0;
}

/* Called when a socket is incoming */
static void
handle_incoming(int fd, blt_role_t role)
{
  blt_dev_t * dev;
  struct sockaddr_rc addr;
  socklen_t len = (socklen_t) sizeof(addr);

  // Got a new incoming socket.
  cw_log(LOG_DEBUG, "Incoming RFCOMM socket\n");

  cw_mutex_lock(&iface_lock);

  fd = accept(fd, (struct sockaddr*)&addr, &len);

  dev = iface_head;
  while (dev) {
    if (bacmp(&(dev->bdaddr), &addr.rc_bdaddr) == 0) {
      cw_log(LOG_DEBUG, "Connect from %s\n", dev->name);
      cw_mutex_lock(&(dev->lock));
      /* Kill any outstanding connect attempt. */
      if (dev->outgoing_id > -1) {
        cw_sched_del(sched, dev->outgoing_id);
        dev->outgoing_id = -1;
      }

      rd_close(dev, 0, 0);

      dev->status = BLT_STATUS_NEGOTIATING;
      dev->rd = fd;

      if (dev->role == BLT_ROLE_AG) {
        dev->cb = ag_brsf_response;
        send_atcmd(dev, "AT+BRSF=23");
      }

      cw_mutex_unlock(&(dev->lock));
      break;
    }
    dev = dev->next;
  }

  if (dev == NULL) {
    cw_log(LOG_WARNING, "Connect from unknown device\n");
    close(fd);
  }
  cw_mutex_unlock(&iface_lock);

  return;
}

static void
handle_incoming_sco(int master)
{

  blt_dev_t * dev;
  struct sockaddr_sco addr;
  struct sco_conninfo conn;
  struct sco_options opts;
  socklen_t len = (socklen_t) sizeof(addr);
  int fd;

  cw_log(LOG_DEBUG, "Incoming SCO socket\n");

  fd = accept(master, (struct sockaddr*)&addr, &len);

  if (fcntl(fd, F_SETFL, O_RDWR|O_NONBLOCK) != 0) {
    cw_log(LOG_ERROR, "Can't set SCO socket to NBIO\n");
    close(fd);
    return;
  }

  len = sizeof(conn);

  if (getsockopt(fd, SOL_SCO, SCO_CONNINFO, &conn, &len) < 0) {
    cw_log(LOG_ERROR, "Can't getsockopt SCO_CONNINFO on SCO socket: %s\n", strerror(errno));
    close(fd);
    return;
  }

  len = sizeof(opts);

  if (getsockopt(fd, SOL_SCO, SCO_OPTIONS, &opts, &len) < 0) {
    cw_log(LOG_ERROR, "Can't getsockopt SCO_OPTIONS on SCO socket: %s\n", strerror(errno));
    close(fd);
    return;
  }

  cw_mutex_lock(&iface_lock);
  dev = iface_head;
  while (dev) {
    if (bacmp(&(dev->bdaddr), &addr.sco_bdaddr) == 0) {
      cw_log(LOG_DEBUG, "SCO Connect from %s\n", dev->name);
      cw_mutex_lock(&(dev->lock));
      if (dev->sco_running != -1) {
        cw_log(LOG_ERROR, "Incoming SCO socket, but SCO thread already running.\n");
      } else {
        sco_start(dev, fd);
      }
      cw_mutex_unlock(&(dev->lock));
      break;
    }
    dev = dev->next;
  }

  cw_mutex_unlock(&iface_lock);

  if (dev == NULL) {
    cw_log(LOG_WARNING, "SCO Connect from unknown device\n");
    close(fd);
  } else {
    // XXX:T: We need to handle the fact we might have an outgoing connection attempt in progress.
    cw_log(LOG_DEBUG, "SCO: %d, HCIHandle=%d, MUT=%d\n", fd, conn.hci_handle, opts.mtu);
  }

  return;
}

/* Called when there is data waiting on a socket */
static int
handle_rd_data(blt_dev_t * dev)
{
  char c;
  int ret;

  while ((ret = read(dev->rd, &c, 1)) == 1) {

    // log_buf[i++] = c;

    if (dev->role == BLT_ROLE_HS) {

      if (c == '\r') {
        ret = process_rfcomm_cmd(dev, dev->rd_buff);
        dev->rd_buff_pos = 0;
        memset(dev->rd_buff, 0, BLT_RDBUFF_MAX);
        return ret;
      }

      if (dev->rd_buff_pos >= BLT_RDBUFF_MAX)
        return 0;

      dev->rd_buff[dev->rd_buff_pos++] = c;

    } else if (dev->role == BLT_ROLE_AG) {

      switch (dev->state) {

        case BLT_STATE_WANT_R:
          if (c == '\r') {
            dev->state = BLT_STATE_WANT_N;
          } else if (c == '+') {
            dev->state = BLT_STATE_WANT_CMD;
            dev->rd_buff[dev->rd_buff_pos++] = '+';
          } else {
            cw_log(LOG_ERROR, "Device %s: Expected '\\r', got %d. state=BLT_STATE_WANT_R\n", dev->name, c);
            return -1;
          }
          break;

        case BLT_STATE_WANT_N:
          if (c == '\n')
            dev->state = BLT_STATE_WANT_CMD;
          else {
            cw_log(LOG_ERROR, "Device %s: Expected '\\n', got %d. state=BLT_STATE_WANT_N\n", dev->name, c);
            return -1;
          }
          break;

        case BLT_STATE_WANT_CMD:
          if (c == '\r')
            dev->state = BLT_STATE_WANT_N2;
          else {
            if (dev->rd_buff_pos >= BLT_RDBUFF_MAX) {
              cw_log(LOG_ERROR, "Device %s: Buffer exceeded\n", dev->name);
              return -1;
            }
            dev->rd_buff[dev->rd_buff_pos++] = c;
          }
          break;

        case BLT_STATE_WANT_N2:
          if (c == '\n') {

            dev->state = BLT_STATE_WANT_R;

            if (dev->rd_buff[0] == '+') {
              int i;
              // find unsolicited
              for (i = 0 ; i < ATCMD_LIST_LEN ; i++) {
                if (strncmp(atcmd_list[i].str, dev->rd_buff, strlen(atcmd_list[i].str)) == 0) {
                  if (atcmd_list[i].unsolicited)
                    atcmd_list[i].unsolicited(dev, dev->rd_buff + strlen(atcmd_list[i].str) + 1);
                  else
                    cw_log(LOG_WARNING, "Device %s: Unhandled Unsolicited: %s\n", dev->name, dev->rd_buff);
                  break;
                }
              }

              if (option_verbose)
                cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s > %s\n", role2str(dev->role), 10, dev->name, dev->rd_buff);

              if (i == ATCMD_LIST_LEN)
                cw_log(LOG_DEBUG, "Device %s: Got unsolicited message: %s\n", dev->name, dev->rd_buff);

            } else {

              if (
                strcmp(dev->rd_buff, "OK") != 0 && 
                strcmp(dev->rd_buff, "CONNECT") != 0 &&
                strcmp(dev->rd_buff, "RING") != 0 &&
                strcmp(dev->rd_buff, "NO CARRIER") != 0 && 
                strcmp(dev->rd_buff, "ERROR") != 0 &&
                strcmp(dev->rd_buff, "NO DIALTONE") != 0 && 
                strcmp(dev->rd_buff, "BUSY") != 0 && 
                strcmp(dev->rd_buff, "NO ANSWER") != 0 && 
                strcmp(dev->rd_buff, "DELAYED") != 0
              ){
                // It must be a multiline error
                strncpy(dev->last_err_cmd, dev->rd_buff, 1023);
                if (option_verbose)
                  cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s > %s\n", role2str(dev->role), 10, dev->name, dev->rd_buff);
              } else if (dev->cb) {
                if (option_verbose)
                  cw_verbose(VERBOSE_PREFIX_1 "[%s] %*s > %s\n", role2str(dev->role), 10, dev->name, dev->rd_buff);
                dev->cb(dev, dev->rd_buff);
              } else {
                cw_log(LOG_ERROR, "Device %s: Data on socket in HS mode, but no callback\n", dev->name);
              }

            }

            dev->rd_buff_pos = 0;
            memset(dev->rd_buff, 0, BLT_RDBUFF_MAX);

          } else {

            cw_log(LOG_ERROR, "Device %s: Expected '\\n' got %d. state = BLT_STATE_WANT_N2:\n", dev->name, c);
            return -1;

          }

          break;

        default:
          cw_log(LOG_ERROR, "Device %s: Unknown device state %d\n", dev->name, dev->state);
          return -1;
      }
    }
  }
  return 0;
}

/* Close the devices RFCOMM socket, and SCO if it exists. Must hold dev->lock */
static void
rd_close(blt_dev_t * dev, int reconnect, int e)
{
  dev->ready = 0;

  if (dev->rd)
    close(dev->rd);

  dev->rd = -1;
  dev->status = BLT_STATUS_DOWN;
  sco_stop(dev);

  if (dev->owner) {
    cw_setstate(dev->owner, CW_STATE_DOWN);
    cw_queue_control(dev->owner, CW_CONTROL_HANGUP);
  }

  /* Schedule a reconnect */
  if (reconnect && dev->autoconnect) {
    dev->outgoing_id = cw_sched_add(sched, 5000, CW_SCHED_CB(try_connect), dev);

    if (monitor_thread == pthread_self()) {
      // Because we're not the monitor thread, we needd to inturrupt poll().
      pthread_kill(monitor_thread, SIGURG);
    }

    if (e)
      cw_log(LOG_NOTICE, "Device %s disconnected, scheduled reconnect in 5 seconds: %s (errno %d)\n", dev->name, strerror(e), e);
  } else if (e) {
    cw_log(LOG_NOTICE, "Device %s disconnected: %s (errno %d)\n", dev->name, strerror(e), e);
  }

  return;
}

/*
 * Remember that we can only add to the scheduler from
 * the do_monitor thread, as it calculates time to next one from
 * this loop.
 */

static void *
do_monitor(void * data)
{
#define SRV_SOCK_CNT 3

  int res = 0;
  blt_dev_t * dev;
  struct pollfd * pfds = malloc(sizeof(struct pollfd) * (ifcount + SRV_SOCK_CNT));

  /* -- We start off by trying to connect all of our devices (non blocking) -- */

  monitor_pid = getpid();

  if (cw_mutex_lock(&iface_lock)) {
    cw_log(LOG_ERROR, "Failed to get iface_lock.\n");
    return NULL;
  }

  dev = iface_head;
  while (dev) {

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, dev->sco_pipe) != 0) {
      cw_log(LOG_ERROR, "Failed to create socket pair: %s (errno %d)\n", strerror(errno), errno);
      cw_mutex_unlock(&iface_lock);
      return NULL;
    }

    if (dev->autoconnect && dev->status == BLT_STATUS_DOWN)
      dev->outgoing_id = cw_sched_add(sched, 1500, CW_SCHED_CB(try_connect), dev);
    dev = dev->next;
  }
  cw_mutex_unlock(&iface_lock);

  /* -- Now, Scan all sockets, and service scheduler -- */

  pfds[0].fd = rfcomm_sock_ag;
  pfds[0].events = POLLIN;

  pfds[1].fd = rfcomm_sock_hs;
  pfds[1].events = POLLIN;

  pfds[2].fd = sco_socket;
  pfds[2].events = POLLIN;

  while (1) {
    int cnt = SRV_SOCK_CNT;
    int i;

    /* -- Build pfds -- */

    if (cw_mutex_lock(&iface_lock)) {
      cw_log(LOG_ERROR, "Failed to get iface_lock.\n");
      return NULL;
    }
    dev = iface_head;
    while (dev) {
      cw_mutex_lock(&(dev->lock));
      if (dev->rd > 0 && ((dev->status != BLT_STATUS_DOWN) && (dev->status != BLT_STATUS_CONNECTING))) {
        pfds[cnt].fd = dev->rd;
        pfds[cnt].events = POLLIN;
        cnt++;
      }
      cw_mutex_unlock(&(dev->lock));
      dev = dev->next;
    }
    cw_mutex_unlock(&iface_lock);

    /* -- End Build pfds -- */

    res = 100;
    res = poll(pfds, cnt, MAX(100, MIN(100, res)));

    if (pfds[0].revents) {
      handle_incoming(rfcomm_sock_ag, BLT_ROLE_AG);
      res--;
    }

    if (pfds[1].revents) {
      handle_incoming(rfcomm_sock_hs, BLT_ROLE_HS);
      res--;
    }

    if (pfds[2].revents) {
      handle_incoming_sco(sco_socket);
      res--;
    }

    if (res == 0)
      continue;

    for (i = SRV_SOCK_CNT ; i < cnt ; i++) {

      /* Optimise a little bit */
      if (res == 0)
        break;
      else if (pfds[i].revents == 0)
        continue;

      /* -- Find the socket that has activity -- */

      if (cw_mutex_lock(&iface_lock)) {
        cw_log(LOG_ERROR, "Failed to get iface_lock.\n");
        return NULL;
      }

      dev = iface_head;

      while (dev) {
        if (pfds[i].fd == dev->rd) {
          cw_mutex_lock(&(dev->lock));
          if (pfds[i].revents & POLLIN) {
            if (handle_rd_data(dev) == -1) {
              rd_close(dev, 0, 0);
            }
          } else {
            rd_close(dev, 1, sock_err(dev->rd));
          }
          cw_mutex_unlock(&(dev->lock));
          res--;
          break;
        }
        dev = dev->next;
      }

      if (dev == NULL) {
        cw_log(LOG_ERROR, "Unhandled fd from poll()\n");
        close(pfds[i].fd);
      }

      cw_mutex_unlock(&iface_lock);

      /* -- End find socket with activity -- */
    }
  }
  return NULL;
}

static int
restart_monitor(void)
{

  if (monitor_thread == CW_PTHREADT_STOP)
    return 0;

  if (cw_mutex_lock(&monitor_lock)) {
    cw_log(LOG_WARNING, "Unable to lock monitor\n");
    return -1;
  }

  if (monitor_thread == pthread_self()) {
    cw_mutex_unlock(&monitor_lock);
    cw_log(LOG_WARNING, "Cannot kill myself\n");
    return -1;
  }

  if (monitor_thread != CW_PTHREADT_NULL) {

    /* Just signal it to be sure it wakes up */
    pthread_cancel(monitor_thread);
    pthread_kill(monitor_thread, SIGURG);
    cw_log(LOG_DEBUG, "Waiting for monitor thread to join...\n");
    pthread_join(monitor_thread, NULL);
    cw_log(LOG_DEBUG, "joined\n");

  } else {

    /* Start a new monitor */
    if (cw_pthread_create(&monitor_thread, NULL, do_monitor, NULL) < 0) {
      cw_mutex_unlock(&monitor_lock);
      cw_log(LOG_ERROR, "Unable to start monitor thread.\n");
      return -1;
    }
  }
  cw_mutex_unlock(&monitor_lock);
  return 0;
}

static int
blt_parse_config(void)
{
  struct cw_config * cfg;
  struct cw_variable * v;
  char * cat;

  cfg = cw_config_load(BLT_CONFIG_FILE);

  if (!cfg) {
    cw_log(LOG_NOTICE, "Unable to load Bluetooth config: %s.  Bluetooth disabled\n", BLT_CONFIG_FILE);
    return -1;
  }

  v = cw_variable_browse(cfg, "general");

  while (v) {
    if (!strcasecmp(v->name, "rfchannel_ag")) {
      rfcomm_channel_ag = atoi(v->value);
    } else if (!strcasecmp(v->name, "rfchannel_hs")) {
      rfcomm_channel_hs = atoi(v->value);
    } else if (!strcasecmp(v->name, "interface")) {
      hcidev_id = atoi(v->value);
    } else {
      cw_log(LOG_WARNING, "Unknown config key '%s' in section [general]\n", v->name);
    }
    v = v->next;
  }
  cat = cw_category_browse(cfg, NULL);

  while(cat) {

    char * str;

    if (strcasecmp(cat, "general")) {
      blt_dev_t * device = malloc(sizeof(blt_dev_t));
      memset(device, 0, sizeof(blt_dev_t));
      device->sco_running = -1;
      device->sco = -1;
      device->rd = -1;
      device->outgoing_id = -1;
      device->status = BLT_STATUS_DOWN;
      str2ba(cat, &(device->bdaddr));
      device->name = cw_variable_retrieve(cfg, cat, "name");

      str = cw_variable_retrieve(cfg, cat, "type");

      if (str == NULL) {
        cw_log(LOG_ERROR, "Device [%s] has no role.  Specify type=<HS/AG>\n", cat);
        return -1;
      } else if (strcasecmp(str, "HS") == 0)
        device->role = BLT_ROLE_HS;
      else if (strcasecmp(str, "AG") == 0) {
        device->role = BLT_ROLE_AG;
      } else {
        cw_log(LOG_ERROR, "Device [%s] has invalid role '%s'\n", cat, str);
        return -1;
      }

      /* XXX:T: Find channel to use using SDP.
       *        However, this needs to be non blocking, and I can't see
       *        anything in sdp_lib.h that will allow non blocking calls.
       */

      device->channel = 1;

      if ((str = cw_variable_retrieve(cfg, cat, "channel")) != NULL)
        device->channel = atoi(str);

      if ((str = cw_variable_retrieve(cfg, cat, "autoconnect")) != NULL)
        device->autoconnect = (strcasecmp(str, "yes") == 0 || strcmp(str, "1") == 0) ? 1 : 0;

      if ((str = cw_variable_retrieve(cfg, cat, "context")) != NULL)
	device->context = str;
      else
	device->context = "bluetooth";

      device->next = iface_head;
      iface_head = device;
      ifcount++;
    }

    cat = cw_category_browse(cfg, cat);
  }
  return 0;
}


static int
blt_show_peers(int fd, int argc, char *argv[])
{
  blt_dev_t * dev;

  if (cw_mutex_lock(&iface_lock)) {
    cw_log(LOG_ERROR, "Failed to get Iface lock\n");
    cw_cli(fd, "Failed to get iface lock\n");
    return RESULT_FAILURE;
  }

  dev = iface_head;

  cw_cli(fd, "BDAddr            Name       Role Status      A/C SCOCon/Fd/Th Sig\n");
  cw_cli(fd, "----------------- ---------- ---- ----------- --- ------------ ---\n");

  while (dev) {
    char b1[18];
    ba2str(&(dev->bdaddr), b1);
    cw_cli(fd, "%s %-10s %-4s %-11s %-3s %2d/%02d/%-6ld %s\n",
                b1, dev->name, (dev->role == BLT_ROLE_HS) ? "HS" : "AG", status2str(dev->status),
                (dev->autoconnect) ? "Yes" : "No",
                dev->sco_running,
                dev->sco,
                dev->sco_thread,
                (dev->role == BLT_ROLE_AG) ? (dev->service) ? "Yes" : "No" : "N/A"
            );
    dev = dev->next;
  }

  cw_mutex_unlock(&iface_lock);
  return RESULT_SUCCESS;
}

static int
blt_show_information(int fd, int argc, char *argv[])
{
  char b1[18];
  ba2str(&local_bdaddr, b1);
  cw_cli(fd, "-------------------------------------------\n");
  cw_cli(fd, "   Monitor PID : %d\n", monitor_pid);
  cw_cli(fd, "     RFCOMM AG : Channel %d, FD %d\n", rfcomm_channel_ag, rfcomm_sock_ag);
  cw_cli(fd, "     RFCOMM HS : Channel %d, FD %d\n", rfcomm_channel_hs, rfcomm_sock_hs);
  cw_cli(fd, "        Device : hci%d, MAC Address %s\n", hcidev_id, b1);
  cw_cli(fd, "-------------------------------------------\n");
  return RESULT_SUCCESS;
}

static int
blt_ag_sendcmd(int fd, int argc, char *argv[])
{
  blt_dev_t * dev;

  if (argc != 4)
    return RESULT_SHOWUSAGE;

  cw_mutex_lock(&iface_lock);
  dev = iface_head;
  while (dev) {
    if (!strcasecmp(argv[2], dev->name))
      break;
    dev = dev->next;
  }
  cw_mutex_unlock(&iface_lock);

  if (!dev) {
    cw_cli(fd, "Device '%s' does not exist\n", argv[2]);
    return RESULT_FAILURE;
  }

  if (dev->role != BLT_ROLE_AG) {
    cw_cli(fd, "Device '%s' is not an AudioGateway\n", argv[2]);
    return RESULT_FAILURE;
  }

  if (dev->status == BLT_STATUS_DOWN || dev->status == BLT_STATUS_NEGOTIATING) {
    cw_cli(fd, "Device '%s' is not connected\n", argv[2]);
    return RESULT_FAILURE;
  }

  if (*(argv[3] + strlen(argv[3]) - 1) == '.')
    *(argv[3] + strlen(argv[3]) - 1) = '?';

  cw_cli(fd, "Sending AT command to %s: %s\n", dev->name, argv[3]);

  cw_mutex_lock(&(dev->lock));
  send_atcmd(dev, argv[3]);
  cw_mutex_unlock(&(dev->lock));

  return RESULT_SUCCESS;
}

static char *
complete_device(char * line, char * word, int pos, int state, int rpos, blt_role_t role)
{
  blt_dev_t * dev;
  int which = 0;
  char *ret;

  if (pos != rpos)
    return NULL;

  cw_mutex_lock(&iface_lock);

  dev = iface_head;

  while (dev) {

    if ((dev->role == role) && (!strncasecmp(word, dev->name, strlen(word)))) {
      if (++which > state)
        break;
    }
    dev = dev->next;
  }

  if (dev)
    ret = strdup(dev->name);
  else
    ret = NULL;

  cw_mutex_unlock(&iface_lock);

  return ret;
}

static char *
complete_device_2_ag(char * line, char * word, int pos, int state)
{
  return complete_device(line, word, pos, state, 2, BLT_ROLE_AG);
}

static void remove_sdp_records(void)
{

  sdp_session_t * sdp;
  sdp_list_t * attr;
  sdp_record_t * rec;
  int res = -1;
  uint32_t range = 0x0000ffff;

  if (sdp_record_ag == -1 || sdp_record_hs == -1)
    return;

  cw_log(LOG_DEBUG, "Removing SDP records\n");

  sdp = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, SDP_RETRY_IF_BUSY);

  if (!sdp)
    return;

  attr = sdp_list_append(0, &range);
  rec = sdp_service_attr_req(sdp, sdp_record_ag, SDP_ATTR_REQ_RANGE, attr);
  sdp_list_free(attr, 0);

  if (rec)
    if (sdp_record_unregister(sdp, rec) == 0)
      res = 0;

  attr = sdp_list_append(0, &range);
  rec = sdp_service_attr_req(sdp, sdp_record_hs, SDP_ATTR_REQ_RANGE, attr);
  sdp_list_free(attr, 0);

  if (rec)
    if (sdp_record_unregister(sdp, rec) == 0)
      res = 0;

  sdp_close(sdp);

  if (res == 0)
    cw_log(LOG_NOTICE, "Removed SDP records\n");
  else
    cw_log(LOG_ERROR, "Failed to remove SDP records\n");

}

static int
__unload_module(void)
{

  cw_channel_unregister(&blt_tech);

  if (monitor_thread != CW_PTHREADT_NULL) {

    if (cw_mutex_lock(&monitor_lock)) {
        cw_log(LOG_WARNING, "Unable to lock the monitor\n");
        return -1;
    }
    if (monitor_thread && (monitor_thread != CW_PTHREADT_STOP) && (monitor_thread != CW_PTHREADT_NULL)) {
      pthread_cancel(monitor_thread);
      pthread_kill(monitor_thread, SIGURG);
      fprintf(stderr, "Waiting for monitor thread to join...\n");
      pthread_join(monitor_thread, NULL);
      fprintf(stderr, "joined\n");
    }
    monitor_thread = CW_PTHREADT_STOP;
    cw_mutex_unlock(&monitor_lock);

  }

  if (rfcomm_sock_ag != -1) {
    fprintf(stderr, "Close sock_ag %d\n", rfcomm_sock_ag);
    close(rfcomm_sock_ag);
    rfcomm_sock_ag = -1;
  }
  if (rfcomm_sock_hs != -1) {
    fprintf(stderr, "Close sock_hs %d\n", rfcomm_sock_hs);
    close(rfcomm_sock_hs);
    rfcomm_sock_hs = -1;
  }
  if (sco_socket != -1) {
    fprintf(stderr, "Close sock_sco %d\n", sco_socket);
    close(sco_socket);
    sco_socket = -1;
  }
  fprintf(stderr, "Removing sdp records\n");
  cw_unregister_atexit(remove_sdp_records);
  remove_sdp_records();
  return 0;
}

int
load_module()
{
  sdp_session_t * sess;
  int dd;
  uint16_t vs;

  hcidev_id = BLT_DEFAULT_HCI_DEV;

  if (blt_parse_config() != 0) {
    cw_log(LOG_ERROR, "Bluetooth configuration error.  Bluetooth Disabled\n");
    return unload_module();
  }

  dd  = hci_open_dev(hcidev_id);
  if (dd == -1) {
    cw_log(LOG_ERROR, "Unable to open interface hci%d: %s.\n", hcidev_id, strerror(errno));
    //let's make openpb.x.org accept wrong configurations without dying
    unload_module();
    return 0;
  }

  hci_read_voice_setting(dd, &vs, 1000);
  vs = htobs(vs);
  close(dd);

  if (vs != 0x0060) {
    cw_log(LOG_ERROR, "Bluetooth voice setting must be 0x0060, not 0x%04x\n", vs);
    unload_module();
    return 0;
  }

  if ((sched = sched_context_create()) == NULL) {
    cw_log(LOG_WARNING, "Unable to create schedule context\n");
    return -1;
  }

  memset(&local_bdaddr, 0, sizeof(local_bdaddr));

  hci_devba(hcidev_id, &local_bdaddr);

  /* --- Add SDP record --- */

  sess = sdp_connect(&local_bdaddr, BDADDR_LOCAL, SDP_RETRY_IF_BUSY);

  if ((rfcomm_sock_ag = rfcomm_listen(&local_bdaddr, rfcomm_channel_ag)) < 0) {
    unload_module();
    return -1;
  }

  if ((rfcomm_sock_hs = rfcomm_listen(&local_bdaddr, rfcomm_channel_hs)) < 0) {
    unload_module();
    return -1;
  }

  if ((sco_socket = sco_listen(&local_bdaddr)) < 0) {
    unload_module();
    return -1;
  }

  if (!sess) {
    cw_log(LOG_ERROR, "Failed to connect to SDP server: %s\n", 
      strerror(errno));
    unload_module();
    return -1;
  }

  if (sdp_register(sess) != 0) {
    cw_log(LOG_ERROR, "Failed to register HeadsetAudioGateway in SDP\n");
    unload_module();
    return -1;
  }

  sdp_close(sess);

  if (restart_monitor() != 0) {
    unload_module();
    return -1;
  }

  if (cw_channel_register(&blt_tech)) {
    cw_log(LOG_ERROR, "Unable to register channel class BLT\n");
    __unload_module();
    return -1;
  }

  cw_cli_register(&cli_show_information);
  cw_cli_register(&cli_show_peers);
  cw_cli_register(&cli_ag_sendcmd);

  cw_register_atexit(remove_sdp_records);

  cw_log(LOG_NOTICE, "Loaded Bluetooth support\n");

  return 0;
}

int
unload_module(void)
{
  cw_cli_unregister(&cli_ag_sendcmd);
  cw_cli_unregister(&cli_show_peers);
  cw_cli_unregister(&cli_show_information);
  return __unload_module();
}

int
usecount()
{
  // FIXME LOCAL_USER_DECL
  int res;
  cw_mutex_lock(&usecnt_lock);
  res = usecnt;
  cw_mutex_unlock(&usecnt_lock);
  return res;
}

char *description()
{
	return (char *) BLT_DESCRIPTION;
}
