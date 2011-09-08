
/*
 * A geek is not a true geek until he knows his nbio sockets.
 *   -- Zozo
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <asterisk/logger.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "chan_sccp.h"
#include "sccp_line.h"
#include "sccp_sched.h"
#include "sccp_socket.h"

int sccp_descriptor = -1;
ast_mutex_t socketlock = AST_MUTEX_INITIALIZER;

static void 
sccp_read_data(sccp_session_t * s)
{
  int length, readlen;
  char * input;

  /* called while we have sessionlock */
  ast_mutex_lock(&s->lock);

  if (ioctl(s->fd, FIONREAD, &length) == -1) {
    ast_log(LOG_WARNING, "FIONREAD ioctl failed: %s\n", strerror(errno));
    close(s->fd);
    s->fd = -1;
    ast_mutex_unlock(&s->lock);
    return;
  }

  if (!length && errno == EINTR) {
    ast_mutex_unlock(&s->lock);
    return;
  }

  if (!length) {
    ast_log(LOG_WARNING, "No length in read: %s (errno %d)\n", strerror(errno), errno);
    close(s->fd);
    s->fd = -1;
    ast_mutex_unlock(&s->lock);
    return;
  }

  input = malloc(length + 1);
  memset(input, 0, length+1);

  if ((readlen = read(s->fd, input, length)) < 0) {
    ast_log(LOG_WARNING, "read() returned %s\n", strerror(errno));
    close(s->fd);
    s->fd = -1;
    ast_mutex_unlock(&s->lock);
    return;
  }

  if (readlen != length) {
    ast_log(LOG_WARNING, "read() returned %d, wanted %d: %s\n", readlen, length, strerror(errno));
    close(s->fd);
    s->fd = -1;
    ast_mutex_unlock(&s->lock);
    return;
  }

  s->buffer_size += length;
  s->buffer = realloc(s->buffer, s->buffer_size);
  memcpy((s->buffer + (s->buffer_size - length)), input, length);

  ast_mutex_unlock(&s->lock);
}

static void destroy_session(sccp_session_t * s)
{
  sccp_session_t *cur, *prev = NULL;
  sccp_line_t * l = NULL;

  ast_mutex_lock(&devicelock);

  ast_log(LOG_DEBUG, "Killing Session (%p), Device: (%p)\n", s, s->device);

  // We want to remove any channels, then lines.

  if (s->device) {
    ast_mutex_lock(&s->device->lock);
    l = s->device->lines;
    while (l) {
      sccp_line_kill(l);
      l = l->next;
    }
    ast_mutex_unlock(&s->device->lock);
  }

  cur = sessions;
  while(cur) {
    if (cur == s)
      break;
    prev = cur;
    cur = cur->next;
  }
  if (cur) {
    // sccp_line_t * l;
    if (prev)
      prev->next = cur->next;
    else
      sessions = cur->next;
    if (s->fd > -1)
      close(s->fd);
    //if (s->device) {
    //  s->device->type = 0;
    //  s->device->registered = 0;
    //  s->device->currentLine = NULL;
    //  l = s->device->lines;
    //  s->device->lines = NULL;
    //  while (l) {
    //    sccp_line_t * tl;
    //    l->device = NULL;
    //    l->instance = -1;
    //    l->dnState = TsOnHook;
    //    tl = l->next;
    //    l->next = NULL;
    //    l = tl;
    //  }
    // }

    if (s->device)
      s->device->session = NULL;
    ast_sched_add(sccp_sched, 500, sccp_sched_delsession, s);

  } else {
    ast_log(LOG_WARNING, "Trying to delete non-existant session %p?\n", s);
  }
  ast_mutex_unlock(&devicelock);
}


static void 
sccp_accept_connection(void)
{
  /* called without sessionlock */
  struct sockaddr_in incoming;
  sccp_session_t * s;
  int dummy = 1, new_socket;
  unsigned int length = sizeof(struct sockaddr_in);

  if ((new_socket = accept(sccp_descriptor, (struct sockaddr *)&incoming, &length)) < 0) {
    ast_log(LOG_ERROR, "Error accepting new socket %s\n", strerror(errno));
     return;
  }

  if (ioctl(new_socket, FIONBIO, &dummy) < 0) {
    ast_log(LOG_ERROR, "Couldn't set socket to non-blocking\n");
    new_socket = close(new_socket);
    return;
  }

  s = malloc(sizeof(struct sccp_session));
  memset(s, 0, sizeof(sccp_session_t));

  ast_mutex_init(&s->lock);

  s->in_addr = strdup(inet_ntoa(incoming.sin_addr));

  ast_log(LOG_DEBUG, "Accepted connection from %s\n", s->in_addr);

  s->fd = new_socket;

  ast_mutex_lock(&sessionlock);
    s->next   = sessions;
    sessions  = s;
  ast_mutex_unlock(&sessionlock);

}

/* Called with mutex lock */
static sccp_moo_t *
sccp_process_data(sccp_session_t * s)
{
  int packSize = 0;
  void * newptr = NULL;
  sccp_moo_t * m = NULL;

  if (s->buffer_size == 0)
    return NULL;

  memcpy(&packSize, s->buffer, 4);

  packSize += 8;

  if ((packSize) > s->buffer_size)
    return NULL; // Not enough data, yet.

  m = malloc(sizeof(sccp_moo_t));
  memcpy(m, s->buffer, packSize);

  s->buffer_size -= packSize;

  if (s->buffer_size) {
    newptr = malloc(s->buffer_size);
    memcpy(newptr, (s->buffer + packSize), s->buffer_size);
  }

  s->buffer = newptr;

  return m;
}

void *
socket_thread(void * ignore)
{
  fd_set fset;
  sccp_session_t * s;
  sccp_moo_t * m;
  struct timeval tv;
  sigset_t sigs;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGTERM);
  sigaddset(&sigs, SIGINT);
  sigaddset(&sigs, SIGPIPE);
  sigaddset(&sigs, SIGWINCH);
  sigaddset(&sigs, SIGURG);
  pthread_sigmask(SIG_BLOCK, &sigs, NULL);

  while (1) {

    pthread_testcancel();

    ast_mutex_lock(&socketlock);

    tv.tv_sec = 0;
    tv.tv_usec = 100;

    FD_ZERO(&fset);
    FD_SET(sccp_descriptor, &fset);

    ast_mutex_lock(&sessionlock);
    s = sessions;
    while (s) {
      sccp_session_t * st;
      if (s->fd <= 0) {
        st = s->next;
        destroy_session(s);
        s = st;
        continue;
      } else {
        FD_SET(s->fd, &fset);
      }
      s = s->next;
    }
    ast_mutex_unlock(&sessionlock);

    if (select(FD_SETSIZE, &fset, 0, 0, &tv) == -1) {
      ast_log(LOG_ERROR, "SCCP select() returned -1. errno: %s\n", strerror(errno));
      ast_mutex_unlock(&socketlock);
      continue;
    }

    if (sccp_descriptor > 0 && FD_ISSET(sccp_descriptor, &fset)) {
      sccp_accept_connection();
    }

    ast_mutex_lock(&sessionlock);
    s = sessions;
    while (s) {
      int again = 0;
      if ( (s->fd > 0) && FD_ISSET(s->fd, &fset)) {
        sccp_read_data(s);
        do {
          again = 0;
          m = sccp_process_data(s);
          if (m) {
            again = 1;
            if (!handle_message(m, s)) {
              close(s->fd);
              s->fd = -1;
            }
          }
        } while (again);
      }
      s = s->next;
    }
    ast_mutex_unlock(&sessionlock);

    ast_mutex_unlock(&socketlock);
  }  
  return NULL;
}
