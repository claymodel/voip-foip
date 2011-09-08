/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * res_jabber Application For CallWeaver
 *
 * Copyright (C) 2005, Anthony Minessale II
 *
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <loudmouth/loudmouth.h>
#include <assert.h>
#define PATCHED_MANAGER

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/manager.h"
#include "callweaver/musiconhold.h"
#include "callweaver/lock.h"
#include "callweaver/cli.h"

#define g_free_if_exists(ptr) if(ptr) {g_free(ptr); ptr=NULL;}

#define JABBER_MSG_SIZE 512
#define CW_FLAG_APP (1 << 30)
#define CW_FLAG_EXTEN (1 << 31)
#define JABBER_STRLEN 512
#define JABBER_ARRAY_LEN 50
#define JABBER_BODYLEN 2048

#define JABBER_MIN_MEDIA_PORT 18000
#define JABBER_MAX_MEDIA_PORT 19999

#define JABBER_HARD_TIMEOUT 0
#define JABBER_LINE_SEPERATOR "\n"
#define JABBER_RECORD_SEPERATOR "\n\n"
#define JABBER_DYNAMIC 

CW_MUTEX_DEFINE_STATIC(global_lock);
CW_MUTEX_DEFINE_STATIC(port_lock);
CW_MUTEX_DEFINE_STATIC(callid_lock);

static char *tdesc = "res_jabber";

static void *app;
static const char *name = "NextGen";
static const char *synopsis = "res_jabber";
static const char *syntax = "";
static const char *desc = "";

static char *configfile = "res_jabber.conf";

STANDARD_LOCAL_USER;
LOCAL_USER_DECL;


#define CHANSTATE_NEW "NEW"
#define CHANSTATE_ANSWER "ANSWER"
#define CHANSTATE_BUSY "BUSY"
typedef char * CHANSTATE;


typedef enum {
	MFLAG_EXISTS = (1 << 0),
	MFLAG_CONTENT = (1 << 1),
} MFLAGS;

typedef enum {
	Q_OUTBOUND,
	Q_INBOUND
} QT;

typedef enum {
	JFLAG_RUNNING = 				(1 <<  0),
	JFLAG_SHUTDOWN = 				(1 <<  1),
	JFLAG_AUTHED = 					(1 <<  2),
	JFLAG_MALLOC = 					(1 <<  3),
	JFLAG_ERROR = 					(1 <<  4),
	JFLAG_MAIN = 					(1 <<  5),
	JFLAG_SUB = 					(1 <<  6),
	JFLAG_RECEIVEMEDIA = 			(1 <<  7),
	JFLAG_FORWARDMEDIA = 			(1 <<  8),
} JFLAGS;

struct jabber_message_node {
	char *jabber_id;
	char *subject;
	char *body;
	struct jabber_message_node *next;
};

struct jabber_message {
    int callid;
    int mval;
	char subject[JABBER_STRLEN];
	char jabber_id[JABBER_STRLEN];
    char command[JABBER_STRLEN];
    char command_args[JABBER_STRLEN];
    char names[JABBER_STRLEN][JABBER_ARRAY_LEN];
    char values[JABBER_STRLEN][JABBER_ARRAY_LEN];
    char body[JABBER_BODYLEN];
    unsigned int flags;
    int last;
    struct woomera_message *next;
};

struct jabber_profile {
	unsigned int flags;
	struct cw_channel *chan;
	int timeout;
	time_t toolate;
	char *master;
	char *bridgeto;
	CHANSTATE chanstate;

	LmConnection *connection;
	GMainContext *context;

	gchar *server;
	gchar *login;
	gchar *passwd;
	gchar *resource;

	struct cw_frame *frame_queue;
	cw_mutex_t fr_qlock;

	struct jabber_message_node *ib_message_queue;
	cw_mutex_t ib_qlock;

	struct jabber_message_node *ob_message_queue;
	cw_mutex_t ob_qlock;

	int media_socket;
	struct sockaddr_in media_send_addr;
	struct sockaddr_in media_recv_addr;
	int callid;

	gchar *identifier;
};

struct jabber_profile global_profile;

static int MEDIA_PORT = JABBER_MIN_MEDIA_PORT;
static int CALLID = 1;

static struct {
	char *master;
	char *server;
	char *login;
	char *passwd;
	char *resource;
	char *media_ip;
	char *event_master;
} globals;


static int jabber_context_open(struct jabber_profile *profile);
static int jabber_context_close(struct jabber_profile *profile);
static int next_media_port(void);
static int next_callid(void);
static void media_close(int *socket);
static void authentication_cb (LmConnection *connection, gboolean result, gpointer ud);
static void connection_open_cb (LmConnection *connection, gboolean result, struct jabber_profile *profile);
static int jabber_profile_queue_frame(struct jabber_profile *profile, struct cw_frame *frame);
static struct cw_frame *jabber_profile_shift_frame(struct jabber_profile *profile);
static int jabber_message_node_push(struct jabber_profile *profile, struct jabber_message_node *node,  QT qt);
static struct jabber_message_node *jabber_message_node_shift(struct jabber_profile *profile, QT qt);
static struct jabber_message_node *jabber_message_node_unshift(struct jabber_profile *profile, struct jabber_message_node *node, QT qt);
static struct jabber_message_node *jabber_message_node_new(char *jabber_id, char *subject, char *fmt, ...);
static void free_jabber_message_node(struct jabber_message_node **node);
static LmHandlerResult handle_messages (LmMessageHandler *handler, LmConnection *connection, LmMessage *m, gpointer user_data);
static void jabber_disconnect(struct jabber_profile *profile);
static int jabber_connect(struct jabber_profile *profile);
static int check_outbound_message_queue(struct jabber_profile *profile);
static int jabber_message_parse(struct jabber_message_node *node, struct jabber_message *jmsg); 
static char *jabber_message_header(struct jabber_message *jmsg, char *key);
static void *jabber_thread(void *obj);
static void *media_receive_thread(void *obj);
static void launch_jabber_thread(struct jabber_profile *profile); 
static void launch_media_receive_thread(struct jabber_profile *profile);
static int parse_jabber_command_profile(struct jabber_profile *profile, struct jabber_message *jmsg);
static void profile_answer(struct jabber_profile *profile);
static void *jabber_pbx_session(void *obj);
#ifdef JABBER_DYNAMIC
static void launch_jabber_pbx_session(struct jabber_profile *profile); 
static struct jabber_profile *jabber_profile_new(void);
#endif
static void jabber_profile_init(struct jabber_profile *profile, char *resource, char *identifier, struct cw_channel *chan, unsigned int flags);
static void jabber_profile_destroy(struct jabber_profile *profile);
static int create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client);
static int parse_jabber_command_main(struct jabber_message *jmsg);
static int res_jabber_exec(struct cw_channel *chan, int argc, char **argv);
static void init_globals(int do_free); 
static int config_jabber(int reload); 
static void *cli_command_thread(void *cli_command);
static void launch_cli_thread(char *cli_command); 






#define jabber_message_node_printf(id, sub, fmt, ...) jabber_message_node_new(id, sub, fmt "Epoch: %ld\n\n", ##__VA_ARGS__, time(NULL))

#ifdef PATCHED_MANAGER
static int jabber_manager_event(int category, char *event, char *body)
{
	struct jabber_message_node *node;
	if ((node=jabber_message_node_printf(globals.event_master, "CALLWEAVER EVENT", "%s", body))) { 
		jabber_message_node_push(&global_profile, node, Q_OUTBOUND);
	}
	return 0;
}
#endif

static int next_callid(void)
{
	int callid;
	cw_mutex_lock(&callid_lock);
	/*******************LOCK*********************/
	callid = CALLID++;
	/*******************LOCK*********************/
	cw_mutex_unlock(&callid_lock);
	return callid;
}

static int next_media_port(void) 
{
	int port = MEDIA_PORT++;

	cw_mutex_lock(&port_lock);
	/*******************LOCK*********************/
	if (port >= JABBER_MAX_MEDIA_PORT) {
		MEDIA_PORT = JABBER_MIN_MEDIA_PORT;
	}
	/*******************LOCK*********************/
	cw_mutex_unlock(&port_lock);

	return port;
}

static void media_close(int *socket)
{
	if(*socket > -1) {
		close(*socket);
		*socket = -1;
	}
}

static void authentication_cb (LmConnection *connection, gboolean result, gpointer ud)
{
	struct jabber_profile *profile;
	struct jabber_message_node *node;

    profile = (struct jabber_profile *) ud;


	cw_log(LOG_DEBUG, "Auth: %d\n", result);

	if (result == TRUE) {
		LmMessage *m;
		
		m = lm_message_new_with_sub_type (NULL,
										  LM_MESSAGE_TYPE_PRESENCE,
										  LM_MESSAGE_SUB_TYPE_AVAILABLE);
		cw_log(LOG_DEBUG, ":: %s\n", lm_message_node_to_string (m->node));
		
		lm_connection_send (connection, m, NULL);
		lm_message_unref (m);
		cw_set_flag(profile, JFLAG_AUTHED);

		if (cw_test_flag(profile, JFLAG_MAIN)) {
			if ((node = jabber_message_node_printf(profile->master,
												   "Event",
												   "EVENT STARTUP\n"
												   "From: %s\n",
												   globals.login
												   ))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}
		}
	}
}

static void connection_open_cb (LmConnection *connection, gboolean result, struct jabber_profile *profile)
{
	cw_log(LOG_DEBUG, "Connected callback\n");
	lm_connection_authenticate (connection, profile->login, profile->passwd, profile->resource, authentication_cb, profile, FALSE,  NULL);
	cw_log(LOG_DEBUG, "Sent auth message\n");
}

static int jabber_profile_queue_frame(struct jabber_profile *profile, struct cw_frame *frame)
{
	struct cw_frame *fp;
	int cnt = 0;

	cw_mutex_lock(&profile->fr_qlock);
	/*******************LOCK*********************/
	for (fp = profile->frame_queue ; fp && fp->next ; fp = fp->next) { 
		cnt++;
	}
	
	cnt++;
	if (!fp) {
		profile->frame_queue = frame;
	} else {
		fp->next = frame;
	}
	/*******************LOCK*********************/
	cw_mutex_unlock(&profile->fr_qlock);
	return cnt;
}
						  
static struct cw_frame *jabber_profile_shift_frame(struct jabber_profile *profile)
{
	struct cw_frame *fp = NULL;

	cw_mutex_lock(&profile->fr_qlock);
	/*******************LOCK*********************/
	if (profile->frame_queue) {
        fp = profile->frame_queue;
        profile->frame_queue = fp->next;
        fp->next = NULL;
    }
	/*******************LOCK*********************/
	cw_mutex_unlock(&profile->fr_qlock);
	return fp;
}
						  


static int jabber_message_node_push(struct jabber_profile *profile, struct jabber_message_node *node,  QT qt)
{
	int cnt = 0;
	struct jabber_message_node **head, *np;
	cw_mutex_t *lock;

	switch(qt) {
	case Q_INBOUND:
		head = &profile->ib_message_queue;
		lock = &profile->ib_qlock;
		break;
	default:
		head = &profile->ob_message_queue;
		lock = &profile->ob_qlock;
		break;
	}

	cw_mutex_lock(lock);
	/*******************LOCK*********************/
	for (np = *head ; np && np->next ; np = np->next) { 
		cnt++;
	}
	
	cnt++;
	if (!np) {
		*head = node;
	} else {
		np->next = node;
	}
	/*******************LOCK*********************/
	cw_mutex_unlock(lock);


	return cnt;
}

static struct jabber_message_node *jabber_message_node_shift(struct jabber_profile *profile, QT qt)
{
	struct jabber_message_node **head, *ret = NULL;
	cw_mutex_t *lock;

	switch(qt) {
	case Q_INBOUND:
		head = &profile->ib_message_queue;
		lock = &profile->ib_qlock;
		break;
	default:
		head = &profile->ob_message_queue;
		lock = &profile->ob_qlock;
		break;
	}

	cw_mutex_lock(lock);
	/*******************LOCK*********************/
	if (*head) {
		ret = *head;
		*head = ret->next;
		ret->next = NULL;
	}
	/*******************LOCK*********************/
	cw_mutex_unlock(lock);

	return ret;
}

static struct jabber_message_node *jabber_message_node_unshift(struct jabber_profile *profile, struct jabber_message_node *node, QT qt)
{
	struct jabber_message_node **head, *ret = NULL;
	cw_mutex_t *lock;
	
	switch(qt) {
	case Q_INBOUND:
		head = &profile->ib_message_queue;
		lock = &profile->ib_qlock;
		break;
	default:
		head = &profile->ob_message_queue;
		lock = &profile->ob_qlock;
		break;
	}

	cw_mutex_lock(lock);
	/*******************LOCK*********************/
	node->next = *head;
	*head = node;
	/*******************LOCK*********************/
	cw_mutex_unlock(lock);

	return ret;
}

static struct jabber_message_node *jabber_message_node_new(char *jabber_id, char *subject, char *fmt, ...)
{
	struct jabber_message_node *node = NULL;
	char *data;
	va_list ap;
    va_start(ap, fmt);
	int result;

#ifdef SOLARIS
	data = (char *)malloc(10240);
	vsnprintf(data, 10240, fmt, ap);
#else
    result = vasprintf(&data, fmt, ap);
#endif	
	va_end(ap);

	if((node=malloc(sizeof(*node)))) {
		memset(node, 0, sizeof(*node));
		node->jabber_id = g_strdup(jabber_id);
		if(subject) {
			node->subject = g_strdup(subject);
		}
		node->body = data;
	}

	return node;

}

static void free_jabber_message_node(struct jabber_message_node **node)
{
	if (*node) {
		if ((*node)->jabber_id) {
			free((*node)->jabber_id);
		}
		if ((*node)->subject) {
			free((*node)->subject);
		}
		if ((*node)->body) {
			free((*node)->body);
		}
		free(*node);
		*node = NULL;
	}
}

static LmHandlerResult handle_messages (LmMessageHandler *handler, LmConnection *connection, LmMessage *m, gpointer user_data)
{
	LmMessageNode *lmnode;
	struct jabber_message_node *node;
	char *from;
	char *body;
	struct jabber_profile *profile;
	struct cw_frame fr = {CW_FRAME_NULL};
	struct cw_frame *frx;

	profile = (struct jabber_profile *) user_data;
	lmnode = lm_message_node_get_child (m->node, "body");
	body = cw_strdupa(lmnode->value);
	from = (char *) lm_message_node_get_attribute (m->node, "from");



	if ((node = jabber_message_node_new(from, "Inbound Message", "%s\n", body))) {
		jabber_message_node_push(profile, node, Q_INBOUND);
	}


	if (profile->chan)
    {
        if ((frx = cw_frdup(&fr)))
    		cw_queue_frame(profile->chan, frx);
        else
    		cw_log(LOG_WARNING, "Unable to duplicate frame\n");
	}


	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void jabber_disconnect(struct jabber_profile *profile)
{
	lm_connection_close (profile->connection, NULL);
	cw_clear_flag(profile, JFLAG_RUNNING);
	cw_set_flag(profile, JFLAG_SHUTDOWN);
}

static int jabber_connect(struct jabber_profile *profile)
{
	LmMessageHandler *handler;
	gboolean result;
	int res = -1;

	profile->connection = lm_connection_new(profile->server);
	handler = lm_message_handler_new (handle_messages, profile, NULL);
	lm_connection_register_message_handler (profile->connection, handler, LM_MESSAGE_TYPE_MESSAGE, LM_HANDLER_PRIORITY_NORMAL);
	lm_message_handler_unref (handler);	

	if ((result = lm_connection_open (profile->connection, (LmResultFunction) connection_open_cb, profile, NULL, NULL))) {
		cw_set_flag((profile), JFLAG_RUNNING);
		cw_clear_flag((profile), JFLAG_ERROR);
		res = 0;
	}

	return res;
}


static int check_outbound_message_queue(struct jabber_profile *profile)
{
	LmMessage *message;
	struct jabber_message_node *node;
	int ret = 0;
	gboolean result;
	GError *error = NULL;

	while ((node=jabber_message_node_shift(profile, Q_OUTBOUND))) {
		message = lm_message_new(node->jabber_id, LM_MESSAGE_TYPE_MESSAGE);		
		lm_message_node_add_child (message->node, "body", node->body);
		lm_message_node_add_child (message->node, "subject", node->subject);
		if (!(result = lm_connection_send(profile->connection, message, &error))) {
			cw_log(LOG_ERROR, "Cannot Send Message! DOH!\n");
			jabber_message_node_unshift(profile, node, Q_OUTBOUND);
			lm_message_unref (message);
			cw_set_flag(profile, JFLAG_ERROR);
			cw_clear_flag(profile, JFLAG_AUTHED);
			ret = -1;
			break;
		}

	    lm_message_unref (message);
		free_jabber_message_node(&node);
	}


	
	return ret;
}


static int jabber_message_parse(struct jabber_message_node *node, struct jabber_message *jmsg) 
{
	char *cur, *cr, *next = NULL, *buf, *body;
	
	memset(jmsg, 0, sizeof(*jmsg));
	buf = cw_strdupa(node->body);

	cw_log(LOG_DEBUG, "Message:\n[%s]\n==========\n%s\n==========\n", node->jabber_id, buf);

	next = buf;

	cw_copy_string(jmsg->subject, node->subject, sizeof(jmsg->subject));
	cw_copy_string(jmsg->jabber_id, node->jabber_id, sizeof(jmsg->jabber_id));
	
	if ((body = strstr(buf, JABBER_RECORD_SEPERATOR))) {
		body += strlen(JABBER_RECORD_SEPERATOR);
		cw_copy_string(jmsg->body, body, sizeof(jmsg->body));
		cw_set_flag(jmsg, MFLAG_CONTENT);
	}
	
	while ((cur = next)) {
		if ((cr = strstr(cur, JABBER_LINE_SEPERATOR))) {
			*cr = '\0';
			next = cr + (sizeof(JABBER_LINE_SEPERATOR) - 1);
			if (!next) {
				break;
			}
		}
		if (!cur || !*cur) {
			break;
		}

		if (!jmsg->last) {
			char *p;
			cw_set_flag(jmsg, MFLAG_EXISTS);
			
			if ((p = strchr(cur, ' '))) {
				*p++ = '\0';
				strncpy(jmsg->command_args, p, JABBER_STRLEN);
			}
			strncpy(jmsg->command, cur, JABBER_STRLEN);
		} else {
			char *name, *val;
			name = cur;
			if ((val = strchr(name, ':'))) {
				*val = '\0';
				val++;
				while (*val == ' ') {
					*val = '\0';
					val++;
				}
				strncpy(jmsg->values[jmsg->last-1], val, JABBER_STRLEN);
			}
			strncpy(jmsg->names[jmsg->last-1], name, JABBER_STRLEN);
		}
		jmsg->last++;

		if(!cr) {
			break;
		}
	}

	jmsg->last--;

	return cw_test_flag(jmsg, MFLAG_EXISTS);

}

static char *jabber_message_header(struct jabber_message *jmsg, char *key)
{
    int x = 0;
    char *value = NULL;

    for (x = 0 ; x < jmsg->last ; x++) {
        if (!strcasecmp(jmsg->names[x], key)) {
            value = jmsg->values[x];
            break;
        }
    }

    return value;
}


static int jabber_context_open(struct jabber_profile *profile)
{
	int res = 0;

	if((profile->context = g_main_context_new())) {
		g_main_context_acquire(profile->context);
		g_main_context_ref(profile->context);
	} else {
		cw_log(LOG_ERROR, "Error Acquiring Context\n");
		res = -1;
	}
	return res;
}


static int jabber_context_close(struct jabber_profile *profile)
{
	int res = 0;

	if (profile->context) {
		g_main_context_unref(profile->context);
		g_main_context_release(profile->context);
		g_main_context_unref(profile->context);
		profile->context = NULL;
	}

	return res;
}




static void *jabber_thread(void *obj)
{
	struct jabber_profile *profile = obj;
	struct jabber_message_node *node;
	struct jabber_message jmsg;
	int res = 0;

	//jabber_context_open(profile);

	profile->context = g_main_context_default();
	g_main_context_ref(profile->context);

	
	for (;;) {
		if (jabber_connect(profile) < 0) {
			cw_log(LOG_NOTICE, "Jabber reconnect attempt\n");
                       	usleep(2000000);
                       	sched_yield();
			continue;
		}
		while (cw_test_flag(profile, JFLAG_RUNNING) && !cw_test_flag(profile, JFLAG_ERROR)){
			
			g_main_context_iteration(profile->context, FALSE);
		
			if (cw_test_flag(profile, JFLAG_AUTHED)) {
				check_outbound_message_queue(profile);
			}else {
                               	cw_log(LOG_ERROR, "Jabber - not authed\n");
                        }

			if ((node = jabber_message_node_shift(profile, Q_INBOUND))) {
				if (jabber_message_parse(node, &jmsg)) {
					cw_log(LOG_DEBUG, "Message From %s\n", node->jabber_id);
					res = parse_jabber_command_main(&jmsg);
				}
				free_jabber_message_node(&node);
			}
			usleep(1000);
			sched_yield();
		}
		jabber_disconnect(profile);
	
		if (cw_test_flag(profile, JFLAG_ERROR)) {
			continue;
		}

		break;
	}

	g_main_context_unref(profile->context);

	cw_log(LOG_DEBUG, "Closing Main Thread\n");
	//jabber_context_close(profile);
	return NULL;
}

static void launch_jabber_thread(struct jabber_profile *profile) 
{
	pthread_attr_t attr;
	int result = 0;
	pthread_t thread;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, jabber_thread, profile);
	result = pthread_attr_destroy(&attr);
}


static int waitfor_socket(int fd, int timeout)
{
    struct pollfd pfds[1];
    int res;

    memset(&pfds[0], 0, sizeof(pfds[0]));
    pfds[0].fd = fd;
    pfds[0].events = POLLIN | POLLERR;
    res = poll(pfds, 1, timeout);

    if ((pfds[0].revents & POLLERR)) {
        res = -1;
    } else if((pfds[0].revents & POLLIN)) {
        res = 1;
    }

    return res;
}


static void *media_receive_thread(void *obj)
{
	int res = 0;
	struct jabber_profile *profile = obj;
	g_main_context_ref(profile->context);

	struct cw_channel *chan = profile->chan;
	struct cw_frame write_frame = {CW_FRAME_VOICE, CW_FORMAT_SLINEAR};
	char buf[1024];
	int err = 0;
	unsigned int fromlen;
	int socket = profile->media_socket;
	char *name = cw_strdupa(chan->name);
	struct cw_frame *frx;

	cw_set_flag(profile, JFLAG_RECEIVEMEDIA);
	cw_log(LOG_DEBUG, "MEDIA UP %s\n", name);
	while (cw_test_flag(profile, JFLAG_RUNNING) && cw_test_flag(profile, JFLAG_RECEIVEMEDIA) && socket > -1) {
		fromlen = sizeof(struct sockaddr_in);
		
		if((res = waitfor_socket(socket, 100)) < 0) {
			err++;
			break;
		} else if (res == 0) {
			continue;
		}

		if (!cw_test_flag(profile, JFLAG_RUNNING)) {
			break;
		}

		if ((res = recvfrom(socket, buf, sizeof(buf), 0, (struct sockaddr *) &profile->media_recv_addr, &fromlen)) > -1)
        {
			//cw_verbose("PACKET\n");
			if (res == 6 && !strncmp(buf, "HANGUP", 6)) {
				cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
				break;
			}

			write_frame.subclass = chan->readformat || CW_FORMAT_SLINEAR;
			write_frame.data = buf;
			write_frame.datalen = res;
			write_frame.samples = res / 2;
            if ((frx = cw_frdup(&write_frame)))
    			jabber_profile_queue_frame(profile, frx);
            else
        		cw_log(LOG_WARNING, "Unable to duplicate frame\n");
		}
        else
        {
			err++;
			break;
		}
	}

	cw_clear_flag(profile, JFLAG_RECEIVEMEDIA);
	media_close(&socket);
	cw_log(LOG_DEBUG, "MEDIA DOWN %s\n", name);
	g_main_context_unref(profile->context);
	return NULL;
}


static void launch_media_receive_thread(struct jabber_profile *profile) 
{
	pthread_attr_t attr;
	int result = 0;
	pthread_t thread;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, media_receive_thread, profile);
	result = pthread_attr_destroy(&attr);
}


static int parse_jabber_command_profile(struct jabber_profile *profile, struct jabber_message *jmsg) 
{


	char *arg = NULL;
	struct jabber_message_node *node;
	struct cw_channel *chan;
	int res = 0;

	assert(profile != NULL);
	chan = profile->chan;

	if(!cw_strlen_zero(jmsg->command_args)) {
		arg = cw_strdupa(jmsg->command_args);
	}
	

	if (!strcasecmp(jmsg->command, "call")) {
		parse_jabber_command_main(jmsg);
	} else if (!strcasecmp(jmsg->command, "exec")) {
		char *app;
		char *data;
		struct cw_app *APP;

		if (arg) {
			app = arg;
		} else {
			cw_log(LOG_WARNING, "this command requires an argument\n");
			return 0;
		}

		if ((data = strchr(app, ' '))) {
			*data++ = '\0';
		} else {
			data = "";
		}
		
		if ((APP = pbx_findapp(app))) {
			cw_log(LOG_DEBUG, "Executing App %s(%s) on %s\n", app, data, chan->name);
			res = pbx_exec(chan, APP, data);
			if ((node = jabber_message_node_printf(profile->master,
												   "Event",
												   "EVENT ENDAPP\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "Application: %s\n"
												   "Data: %s\n"
												   ,
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   app,
												   data
												   ))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}

		} else {
			cw_log(LOG_WARNING, "Unknown App %s(%s) called on %s\n", app, data, chan->name);
		}
		
	} else if (!strcasecmp(jmsg->command, "hangup")) {
		cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
	} else if (!strcasecmp(jmsg->command, "answer")) {
		profile_answer(profile);
	} else if (!strcasecmp(jmsg->command, "stream") && arg) {
		cw_openstream(chan, arg, chan->language);
		pbx_builtin_setvar_helper(chan, "STREAMFILE", arg);
	} else if (!strcasecmp(jmsg->command, "stopstream")) {
		cw_stopstream(chan);
		chan->stream = NULL;
		if ((node = jabber_message_node_printf(profile->master,
											   "Event",
											   "EVENT STOPSTREAM\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   "FileName: %s\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid,
											   pbx_builtin_getvar_helper(chan, "STREAMFILE")
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}

	} else if (!strcasecmp(jmsg->command, "info")) {
		if ((node = jabber_message_node_printf(jmsg->jabber_id,
											   "Event",
											   "EVENT INFO\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
	} else if (!strcasecmp(jmsg->command, "forwardmedia")) {
		char *ip = jabber_message_header(jmsg, "ip");
		char *porta = jabber_message_header(jmsg, "port");
		int port;
		struct hostent *hp;
		struct cw_hostent ahp;

		if (!ip) {
			ip = "127.0.0.1";
		}

		if(!porta && arg) {
			porta = arg;
		}

		if (porta && (hp = cw_gethostbyname(ip, &ahp))) {
			port = atoi(porta);
			profile->media_send_addr.sin_family = hp->h_addrtype;
			memcpy((char *) &profile->media_send_addr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
			profile->media_send_addr.sin_port = htons(port);
			cw_set_flag(profile, JFLAG_FORWARDMEDIA);
			if ((node = jabber_message_node_printf(profile->master,
												   "Event",
												   "EVENT FORWARDMEDIA\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "ip: %s\n"
												   "port: %d\n",
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   ip,
												   port))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}
		}
	} else if (!strcasecmp(jmsg->command, "receivemedia")) {
		char *ip = jabber_message_header(jmsg, "ip");
		char *porta = jabber_message_header(jmsg, "port");
		char *forwardip = jabber_message_header(jmsg, "forwardip");
		char *forwardporta = jabber_message_header(jmsg, "forwardport");
		char *bridgeto = jabber_message_header(jmsg, "bridgeto");
		char *bridgetome = jabber_message_header(jmsg, "bridgetome");
		int port, forwardport = 0, tries = 0;
		
		if (!ip) {
			ip = globals.media_ip;
			if (!ip) {
				ip = "127.0.0.1";
			}
		}

		if(!porta && arg) {
			porta = arg;
		}

		if (forwardporta) {
			forwardport = atoi(forwardporta);
		}

		if (porta) {
			port = atoi(porta);
		} else {
			port = next_media_port();
		}

		media_close(&profile->media_socket);
		
		while ((profile->media_socket = create_udp_socket(ip, port, &profile->media_recv_addr, 1)) < 0) {
			if (porta) {
				break;
			}
			if((tries++) >= 2000) {
				cw_log(LOG_ERROR, "Error! 2000 port failures in a row!\n");
				break;
			}

			port = next_media_port();
		}


		if(profile->media_socket > -1) {
			cw_log(LOG_DEBUG, "open socket %d on %s:%d for media\n", profile->media_socket, ip, port);
			if ((node = jabber_message_node_printf(profile->master, 
												   "Event",
												   "EVENT RECEIVEMEDIA\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "ip: %s\n"
												   "port: %d\n",
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   ip,
												   port))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}
			launch_media_receive_thread(profile);
			
			if (bridgetome) {
				bridgeto = jmsg->jabber_id;
			}

			if (bridgeto) {
				if ((node = jabber_message_node_printf(bridgeto, 
													   "Command",
													   "RECEIVEMEDIA\n"
													   "ForwardIp: %s\n"
													   "ForwardPort: %d\n",
													   ip,
													   port))) {
					jabber_message_node_push(profile, node, Q_OUTBOUND);
				}
			}


			if (forwardip && forwardport) {
				if ((node = jabber_message_node_printf("myself", 
													   "Command",
													   "FORWARDMEDIA\n"
													   "Ip: %s\n"
													   "Port: %d\n",
													   forwardip,
													   forwardport))) {
					jabber_message_node_push(profile, node, Q_INBOUND);
				}
				if ((node = jabber_message_node_printf(jmsg->jabber_id, 
													   "Command",
													   "FORWARDMEDIA\n"
													   "Ip: %s\n"
													   "Port: %d\n",
													   ip,
													   port))) {
					jabber_message_node_push(profile, node, Q_OUTBOUND);
				}
			}
			

				
		} else {
			cw_log(LOG_ERROR, "Error opening media socket %s:%d\n", ip, port);
			if ((node = jabber_message_node_printf(profile->master, 
												   "Event",
												   "EVENT MEDIASOCKETFAIL\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "ip: %s\n"
												   "port: %d\n",
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   ip,
												   port))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}
				
		}
		
	} else if (!strcasecmp(jmsg->command, "pausereceivemedia")) {
		cw_clear_flag(profile, JFLAG_RECEIVEMEDIA);
	} else if (!strcasecmp(jmsg->command, "resumereceivemedia")) {
		if (profile->media_socket > -1) {
			cw_set_flag(profile, JFLAG_RECEIVEMEDIA);
		}
	} else if (!strcasecmp(jmsg->command, "pauseforwardmedia")) {
		cw_clear_flag(profile, JFLAG_FORWARDMEDIA);
	} else if (!strcasecmp(jmsg->command, "resumeforwardmedia")) {
		if (profile->media_send_addr.sin_port) {
			cw_set_flag(profile, JFLAG_FORWARDMEDIA);
		}
	} else if (!strcasecmp(jmsg->command, "noreceivemedia")) {
		media_close(&profile->media_socket);
		cw_clear_flag(profile, JFLAG_RECEIVEMEDIA);
		if ((node = jabber_message_node_printf(profile->master, 
											   "Event",
											   "EVENT NOREVEIVEMEDIA\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
	} else if (!strcasecmp(jmsg->command, "noforwardmedia")) {
		cw_clear_flag(profile, JFLAG_FORWARDMEDIA);
		if ((node = jabber_message_node_printf(profile->master, 
											   "Event",
											   "EVENT NOFORWARDMEDIA\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
	} else if (!strcasecmp(jmsg->command, "startmoh")) {
		if ((node = jabber_message_node_printf(profile->master, 
											   "Event",
											   "EVENT STARTMOH\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
		cw_moh_start(chan, arg);
	} else if (!strcasecmp(jmsg->command, "stopmoh")) {
		if ((node = jabber_message_node_printf(profile->master, 
											   "Event",
											   "EVENT STOPMOH\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   ,
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid
											   ))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
		cw_moh_stop(chan);
		
	} else if (!strcasecmp(jmsg->command, "setreadformat") && arg) {
		int format;
		if ((format=cw_getformatbyname(arg))) {
			cw_set_read_format(chan, format);
		}
	} else if (!strcasecmp(jmsg->command, "setwriteformat") && arg) {
		int format;
		if ((format=cw_getformatbyname(arg))) {
			cw_set_write_format(chan, format);
		}
	} else if (!strcasecmp(jmsg->command, "setmaster") && arg) {
		char *oldmaster = cw_strdupa(profile->master);
		g_free_if_exists (profile->master);
		profile->master = g_strdup(arg);
		
		if ((node = jabber_message_node_printf(oldmaster, 
											   "Event",
											   "EVENT MASTERCHANGE\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   "OldMaster: %s\n"
											   "NewMaster: %s\n",
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid,
											   oldmaster,
											   profile->master))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}



		if ((node = jabber_message_node_printf(profile->master, 
											   "Event",
											   "EVENT MASTERCHANGE\n"
											   "ChannelName: %s\n"
											   "From: %s\n"
											   "Identifier: %s\n"
											   "CallID: %d\n"
											   "OldMaster: %s\n"
											   "NewMaster: %s\n",
											   chan->name,
											   profile->resource,
											   profile->identifier,
											   profile->callid,
											   oldmaster,
											   profile->master))) {
			jabber_message_node_push(profile, node, Q_OUTBOUND);
		}
	}


	return res;
}

static void setup_cdr(struct cw_channel *chan) 
{
	if (chan->cdr) {
		return;
	}

	chan->cdr = cw_cdr_alloc();
	if (chan->cdr) {
		cw_cdr_init(chan->cdr, chan);
	}
	if (chan->cdr) {
		cw_cdr_setapp(chan->cdr, name, "");
		cw_cdr_update(chan);
		cw_cdr_start(chan->cdr);
		cw_cdr_end(chan->cdr);
	} 
}

static void profile_answer(struct jabber_profile *profile)
{
	struct cw_channel *chan = profile->chan;

	profile->chanstate = CHANSTATE_ANSWER;
	setup_cdr(chan);
	cw_answer(chan);
	profile->timeout = -1;
}

static void *jabber_pbx_session(void *obj)
{
	struct jabber_profile *profile = obj;
	struct cw_channel *chan = profile->chan;
	struct jabber_message_node *node;
	struct jabber_message jmsg;
	struct cw_frame *f, *sf;
	time_t start, now;
	int res = 0;
	int readformat;
	int writeformat;
	int state;

	jabber_context_open(profile);
	

	if (cw_set_read_format(chan, CW_FORMAT_SLINEAR)) {
		cw_log(LOG_ERROR, "Error Setting Read Format.\n");
		return NULL;
	}
	if (cw_set_write_format(chan, CW_FORMAT_SLINEAR)) {
		cw_log(LOG_ERROR, "Error Setting Write Format.\n");
		return NULL;
	}

	g_main_context_ref(profile->context);
	chan->appl = (char *)name;
	readformat = chan->readformat;
	writeformat = chan->writeformat;
	state = chan->_state;
	profile->chanstate = CHANSTATE_NEW;
	
	if (jabber_connect(profile) < 0) {
		goto punt;
    }

	time(&start);
	while (!cw_test_flag(profile, JFLAG_AUTHED)) {
		time(&now);
		if ((now - start) > 20) {
			goto punt;
		}
		usleep(10000);
	}
	check_outbound_message_queue(profile);

	if (chan->_state == CW_STATE_UP) {
		profile->timeout = -1;
	}

	while (cw_test_flag(profile, JFLAG_RUNNING) && 
		   (res = cw_waitfor(chan, profile->timeout)) > -1) {

		if (profile->timeout > -1) {
			if (!res) {
				break;
			}
			profile->timeout = res;
		}
		
		if (!(f=cw_read(chan))) {
			break;
		}

		if (chan->stream) {
			if ((sf = cw_readframe(chan->stream))) {
				cw_write(chan, sf);
				cw_fr_free(sf);
			} else {
				cw_stopstream(chan);
				chan->stream = NULL;
				if ((node = jabber_message_node_printf(profile->master,
													   "Event",
													   "EVENT STOPSTREAM\n"
													   "ChannelName: %s\n"
													   "From: %s\n"
													   "Identifier: %s\n"
													   "CallID: %d\n"
													   "FileName: %s\n"
													   ,
													   chan->name,
													   profile->resource,
													   profile->identifier,
													   profile->callid,
													   pbx_builtin_getvar_helper(chan, "STREAMFILE")
													  
													   ))) {
					jabber_message_node_push(profile, node, Q_OUTBOUND);
				}
			}
		}

		if (state != chan->_state) {
			if ((node = jabber_message_node_printf(profile->master, 
												   "Event",
												   "EVENT STATECHANGE\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "OldState: %d\n"
												   "NewState: %d\n",
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   state,
												   chan->_state
												   ))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
				state = chan->_state;
			}
		}


		switch (f->frametype) {

		case CW_FRAME_CONTROL:
			switch (f->subclass) {
			case CW_CONTROL_BUSY:
			case CW_CONTROL_CONGESTION:
				profile->chanstate = CHANSTATE_BUSY;
				cw_clear_flag(profile, JFLAG_RUNNING);
				continue;
				break;
			case CW_CONTROL_ANSWER:
				profile_answer(profile);
				continue;
				break;
			}
			break;
		case CW_FRAME_DTMF:
			if ((node = jabber_message_node_printf(profile->master, 
												   "Event",
												   "EVENT DTMF\n"
												   "ChannelName: %s\n"
												   "From: %s\n"
												   "Identifier: %s\n"
												   "CallID: %d\n"
												   "Digit: %c\n",
												   chan->name,
												   profile->resource,
												   profile->identifier,
												   profile->callid,
												   f->subclass))) {
				jabber_message_node_push(profile, node, Q_OUTBOUND);
			}
			break;
		case CW_FRAME_VOICE:
			if (readformat != chan->readformat) {
				if ((node = jabber_message_node_printf(profile->master, 
													   "Event",
													   "EVENT READFORMAT\n"
													   "ChannelName: %s\n"
													   "From: %s\n"
													   "Identifier: %s\n"
													   "CallID: %d\n"
													   "Format: %s\n",
													   chan->name,
													   profile->resource,
													   profile->identifier,
													   profile->callid,
													   cw_getformatname(chan->readformat)))) {
					jabber_message_node_push(profile, node, Q_OUTBOUND);
					readformat = chan->readformat;
				}
			}
			if (writeformat != chan->writeformat) {
				if ((node = jabber_message_node_printf(profile->master, 
													   "Event",
													   "EVENT WRITEFORMAT\n"
													   "ChannelName: %s\n"
													   "From: %s\n"
													   "Identifier: %s\n"
													   "CallID: %d\n"
													   "Format: %s\n",
													   chan->name,
													   profile->resource,
													   profile->identifier,
													   profile->callid,
													   cw_getformatname(chan->writeformat)))) {
					jabber_message_node_push(profile, node, Q_OUTBOUND);
					writeformat = chan->writeformat;
				}
			}

			if (profile->media_socket > -1) {

				if (cw_test_flag(profile, JFLAG_RECEIVEMEDIA)) {
					struct cw_frame *f;

					while ((f=jabber_profile_shift_frame(profile))) {
						if (!chan->stream) { /* for now we'll ignore voice while a file plays */
							cw_write(chan, f);
						}
						cw_fr_free(f);
					}
				}

				if (chan->_state == CW_STATE_UP && cw_test_flag(profile, JFLAG_FORWARDMEDIA)) {
					int i;

					i = sendto(profile->media_socket,
							   f->data,
							   f->datalen,
							   0,
							   (struct sockaddr *) &profile->media_send_addr, 
							   sizeof(profile->media_send_addr));
					if (i < 0) {
						cw_log(LOG_ERROR, "Error sending media\n");
						cw_clear_flag(profile, JFLAG_FORWARDMEDIA);
					}
				}
			}

		default:
			break;

		}

		cw_fr_free(f);
		res = 0;
		g_main_context_iteration(profile->context, FALSE);
		
		if ((node = jabber_message_node_shift(profile, Q_INBOUND))) {
			if (jabber_message_parse(node, &jmsg)) {
				res = parse_jabber_command_profile(profile, &jmsg);
			}

			free_jabber_message_node(&node);
		}
		
		if (cw_test_flag(profile, JFLAG_AUTHED)) {
            check_outbound_message_queue(profile);
        }

		if (cw_test_flag(profile, JFLAG_ERROR)) {
			jabber_connect(profile);
			continue;
		}

		if (res < 0 || cw_check_hangup(chan)) {
			break;
		}

	}

	if (cw_test_flag(profile, JFLAG_FORWARDMEDIA)) {
		int i;
		
		i = sendto(profile->media_socket,
				   "HANGUP",
				   6,
				   0,
				   (struct sockaddr *) &profile->media_send_addr, 
				   sizeof(profile->media_send_addr));
		if (i < 0) {
			cw_log(LOG_ERROR, "Error sending media\n");
			cw_clear_flag(profile, JFLAG_FORWARDMEDIA);
		}
	}
	


	setup_cdr(chan);

	if ((node = jabber_message_node_printf(profile->master, 
										   "Event",
										   "EVENT END CALL\n"
										   "ChannelName: %s\n"
										   "From: %s\n"
										   "Identifier: %s\n"
										   "CallID: %d\n"
										   "CallingPartyName: %s\n"
										   "CallingPartyNumber: %s\n"
										   "CalledParty: %s\n"
										   "FinalState: %s\n",
										   chan->name,
										   profile->resource,
										   profile->identifier,
										   profile->callid,
										   chan->cid.cid_name,
										   chan->cid.cid_num,
										   chan->exten,
										   profile->chanstate
										   ))) {
		jabber_message_node_push(profile, node, Q_OUTBOUND);
		check_outbound_message_queue(profile);
		usleep(10000);
	}

 punt:

	jabber_disconnect(profile);
	media_close(&profile->media_socket);
	
	if (chan && cw_test_flag(profile, JFLAG_MALLOC)) {
		cw_hangup(chan);
		chan = NULL;
	}

	res = 0;
	while (cw_test_flag(profile, JFLAG_RECEIVEMEDIA)) {
		res++;
		usleep(1000);
		sched_yield();
		if (res > 100000) {
			cw_log(LOG_ERROR, "Error Waiting for media thread\n");
			break;
		}
	}


	jabber_context_close(profile);
	jabber_profile_destroy(profile);

	return NULL;
}

#ifdef JABBER_DYNAMIC

static void launch_jabber_pbx_session(struct jabber_profile *profile) 
{
	pthread_attr_t attr;
	int result = 0;
	pthread_t thread;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, jabber_pbx_session, profile);
	result = pthread_attr_destroy(&attr);
}


static struct jabber_profile *jabber_profile_new(void)
{
	struct jabber_profile *profile = NULL;
	if ((profile=malloc(sizeof(*profile)))) {
		memset(profile, 0, sizeof(*profile));
		cw_set_flag(profile, JFLAG_MALLOC);
	}
	return profile;
}

#endif

static void jabber_profile_init(struct jabber_profile *profile, char *resource, char *identifier, struct cw_channel *chan, unsigned int flags)
{
	memset(profile, 0, sizeof(*profile));
	cw_mutex_init(&profile->ib_qlock);
	cw_mutex_init(&profile->ob_qlock);
	cw_mutex_init(&profile->fr_qlock);
	profile->master =  g_strdup (globals.master);
	profile->server = g_strdup (globals.server);
	profile->login = g_strdup (globals.login);
	profile->passwd = g_strdup (globals.passwd);
	profile->resource = g_strdup (resource);
	profile->timeout = 60000;
	profile->identifier = g_strdup(identifier);
	profile->callid = next_callid();

	cw_set_flag(profile, flags);
	profile->media_socket = -1;
	if (chan) {
		profile->chan = chan;
	}
}


static void jabber_profile_destroy(struct jabber_profile *profile)
{
	cw_mutex_destroy(&profile->ib_qlock);
	cw_mutex_destroy(&profile->ob_qlock);
	cw_mutex_destroy(&profile->fr_qlock);
	g_free_if_exists(profile->login);
	g_free_if_exists(profile->passwd);
	g_free_if_exists(profile->resource);
	g_free_if_exists(profile->server);
	g_free_if_exists(profile->master);
	g_free_if_exists(profile->bridgeto);
	g_free_if_exists(profile->identifier);

	if (cw_test_flag(profile, JFLAG_MALLOC)) {
		free(profile);
	}
}



static int create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client)
{
	int sock;

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}
	memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin_family=AF_INET;
	sockaddr->sin_addr.s_addr=INADDR_ANY;
	sockaddr->sin_port=htons(port);
	if (bind(sock, (struct sockaddr *) sockaddr, sizeof(*sockaddr)) < 0) {
		media_close(&sock);
	}

	return sock;
}

/*
  static int ass2create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client)
  {
  int rc;
  struct hostent *result, hp;
  struct sockaddr_in local_addr;
  char buf[512];
  int err = 0;
  int sock;

  memset(&hp, 0, sizeof(hp));
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0))) {
  gethostbyname_r(ip, &hp, buf, sizeof(buf), &result, &err);
  if (result) {
  sockaddr->sin_family = hp.h_addrtype;
  memcpy((char *) &sockaddr->sin_addr.s_addr, hp.h_addr_list[0], hp.h_length);
  sockaddr->sin_port = htons(port);
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(0);
  if ((rc = bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr))) < 0) {
  media_close(&sock);
  }
  }
  }

  return sock;
  }
*/

/*
  static int create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client)
  {
  int rc, sd = 0;
  struct hostent *hp;
  struct cw_hostent ahp;
  struct sockaddr_in servAddr, *addr, cliAddr;
	
  if(sockaddr) {
  addr = sockaddr;
  } else {
  addr = &servAddr;
  }
	
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0))) {
  if ((hp = cw_gethostbyname(ip, &ahp))) {
  addr->sin_family = hp->h_addrtype;
  memcpy((char *) &addr->sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
  addr->sin_port = htons(port);
  if (client) {
  cliAddr.sin_family = AF_INET;
  cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliAddr.sin_port = htons(0);
  rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
  } else {
  rc = bind(sd, (struct sockaddr *) addr, sizeof(cliAddr));
  }
  if(rc < 0) {
  cw_log(LOG_ERROR,"Error opening udp socket\n");
  media_close(&sd);
  }
  }
  }

  return sd;
  }
*/
static void *cli_command_thread(void *cli_command)
{
 	int fd;
   	fd = fileno(stderr);
   	cw_cli_command(fd, (char *)cli_command);
/*   	free(cli_command);	 */
	return NULL;
}	

static void launch_cli_thread(char *cli_command) 
{
	pthread_attr_t attr;
	int result = 0;
	char *cli_command_dup;
	pthread_t thread;
	struct jabber_message_node *node;

    if(!cw_strlen_zero(cli_command)) {
        cli_command_dup = cw_strdupa(cli_command);
    }
    else {
    	return;
    }
	if ((node = jabber_message_node_printf("Cli command", 
											    "Cli Command Call",
												"Command: %s\n",
												cli_command 
												))) {
				jabber_message_node_push(&global_profile, node, Q_OUTBOUND);
	}
	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, cli_command_thread, cli_command_dup);
	result = pthread_attr_destroy(&attr);
}

static int parse_jabber_command_main(struct jabber_message *jmsg)
{
	int res = 0;
	char *arg = NULL;
	struct jabber_message_node *node;

	if(!cw_test_flag(jmsg, MFLAG_EXISTS)) {
		return 0;
	}


    if(!cw_strlen_zero(jmsg->command_args)) {
        arg = cw_strdupa(jmsg->command_args);
    }

    if (!strcasecmp(jmsg->command, "call")) {
		char *type = jabber_message_header(jmsg, "type");
		char *data = jabber_message_header(jmsg, "data");
		char *toa = jabber_message_header(jmsg, "timeout");
		char *cid_name = jabber_message_header(jmsg, "callingpartyname");
		char *cid_num = jabber_message_header(jmsg, "callingpartynumber");
		char *formata = jabber_message_header(jmsg, "format");
		char *pname = jabber_message_header(jmsg, "resource");
		char *identifier = jabber_message_header(jmsg, "identifier");
		char *bridgeto = jabber_message_header(jmsg, "bridgeto");
		

		int timeout = 0;
		int format = 0;
		int reason = 0;

		
		timeout = toa ? atoi(toa) : 60000;
		format = formata ? cw_getformatbyname(formata) : CW_FORMAT_SLINEAR;

		if(type && data) {
			struct cw_channel *chan;
			struct jabber_profile *profile;
			char callida[80];

			if((chan = cw_request(type, format, data, &reason))) {
				cw_set_callerid(chan, cid_num, cid_name, cid_num);
				if (!cw_call(chan, data, timeout)) {

					if(!pname) {
						pname = chan->name;
					}
					if(!identifier) {
						identifier = pname;
					}

					if ((profile = jabber_profile_new())) {
						time_t now;
						jabber_profile_init(profile, pname, identifier, chan, JFLAG_SUB|JFLAG_MALLOC);
						
						time(&now);
						profile->toolate = now + timeout;
						profile->timeout = timeout;

						if (!pname) {
							pname = chan->name;
						}
						sprintf(callida, "%d", profile->callid);						

						pbx_builtin_setvar_helper(chan, "IDENTIFIER", profile->identifier);
						pbx_builtin_setvar_helper(chan, "CALLID", callida);


						
						if ((node = jabber_message_node_printf(profile->master, 
															   "Event",
															   "EVENT OUTGOING CALL\n"
															   "From: %s\n"
															   "Identifier: %s\n"
															   "CallID: %d\n"
															   "CallingPartyName: %s\n"
															   "CallingPartyNumber: %s\n"
															   "CalledParty: %s\n"
															   ,
															   pname,
															   profile->identifier,
															   profile->callid,
															   chan->cid.cid_name,
															   chan->cid.cid_num,
															   chan->exten
														
															   ))) {
							jabber_message_node_push(profile, node, Q_OUTBOUND);
						}
	
						if (bridgeto) {
							if ((node = jabber_message_node_printf("myself", 
																   "Command",
																   "RECEIVEMEDIA\n"
																   "BridgeTo: %s\n",
																   bridgeto
																   ))) {
								jabber_message_node_push(profile, node, Q_INBOUND);
							}
						}
						
						launch_jabber_pbx_session(profile);

					} else {
						cw_hangup(chan);
						chan = NULL;
					}
				} else {
					cw_hangup(chan);
					chan = NULL;
				}
			}
		}
	}
   if (!strcasecmp(jmsg->command, "cli")) {
   	    launch_cli_thread(jmsg->command_args);
   }

	return res;
}




static int res_jabber_exec(struct cw_channel *chan, int argc, char **argv)
{
	struct localuser *u;
	struct jabber_message_node *node;
	struct jabber_profile profile;
	char *name, *master;

	if (cw_set_read_format(chan, CW_FORMAT_SLINEAR)) {
		cw_log(LOG_ERROR, "Error Setting Read Format.\n");
		return -1;
	}
	if (cw_set_write_format(chan, CW_FORMAT_SLINEAR)) {
		cw_log(LOG_ERROR, "Error Setting Write Format.\n");
		return -1;
	}

	LOCAL_USER_ADD(u);

	name = chan->uniqueid;

	jabber_profile_init(&profile, name, name, chan, JFLAG_SUB);
	master = (argc > 1 && argv[0][0] ? argv[0] : profile.master);

	if ((node = jabber_message_node_printf(master, 
										   "EVENT",
										   "EVENT INCOMING CALL\n"
										   "ChannelName: %s\n"
										   "From: %s\n"
										   "Identifier: %s\n"
										   "CallID: %d\n"
										   "CallingPartyName: %s\n"
										   "CallingPartyNumber: %s\n"
										   "CalledParty: %s\n"
										   ,
										   chan->name,
										   profile.resource,
										   profile.identifier,
										   profile.callid,
										   chan->cid.cid_name,
										   chan->cid.cid_num,
										   chan->exten
										
										   ))) {
		jabber_message_node_push(&profile, node, Q_OUTBOUND);
	}
	

	jabber_pbx_session(&profile);

	LOCAL_USER_REMOVE(u);
	return -1;
}

#ifdef PATCHED_MANAGER
static struct manager_custom_hook jabber_hook = {
	.file = "res_jabber",
	.helper = jabber_manager_event
};
#endif

int unload_module(void)
{
	STANDARD_HANGUP_LOCALUSERS;

#ifdef PATCHED_MANAGER
	if (globals.event_master) {
		cw_log(LOG_NOTICE, "Un-Registering Manager Event Hook\n");
		del_manager_hook(&jabber_hook);
	}
#endif

	cw_clear_flag((&global_profile), JFLAG_RUNNING);
	while (!cw_test_flag((&global_profile), JFLAG_SHUTDOWN)) {
		usleep(1000);
		sched_yield();
	}
	jabber_profile_destroy(&global_profile);
	return cw_unregister_application(app);
}

static void init_globals(int do_free) 
{
	if (do_free) {
		cw_mutex_lock(&global_lock);
		/*******************LOCK*********************/
		g_free_if_exists(globals.master);
		g_free_if_exists(globals.server);
		g_free_if_exists(globals.login);
		g_free_if_exists(globals.passwd);
		g_free_if_exists(globals.resource);
		g_free_if_exists(globals.media_ip);
		g_free_if_exists(globals.event_master);
		/*******************LOCK*********************/
		cw_mutex_unlock(&global_lock);
	}

	memset(&globals, 0, sizeof(globals));

}

static int config_jabber(int reload) 
{
	struct cw_config *cfg;
	char *entry;
	struct cw_variable *v;
	int count = 0;

	init_globals(reload);
	
	if ((cfg = cw_config_load(configfile))) {
		for (entry = cw_category_browse(cfg, NULL); entry != NULL; entry = cw_category_browse(cfg, entry)) {
			if (!strcmp(entry, "settings")) {
				for (v = cw_variable_browse(cfg, entry); v ; v = v->next) {
					if (!strcmp(v->name, "master")) {
						globals.master = g_strdup(v->value);
					} else if (!strcmp(v->name, "server")) {
						globals.server = g_strdup(v->value);
					} else if (!strcmp(v->name, "login")) {
						globals.login = g_strdup(v->value);
					} else if (!strcmp(v->name, "passwd")) {
						globals.passwd = g_strdup(v->value);
					} else if (!strcmp(v->name, "media_ip")) {
						globals.media_ip = g_strdup(v->value);
					} else if (!strcmp(v->name, "resource")) {
						globals.resource = g_strdup(v->value);
					} else if (!strcmp(v->name, "event_master")) {
						globals.event_master = g_strdup(v->value);
					}
				}
			} 
		}
		count++;
		cw_config_destroy(cfg);
	} else {
		return 0;
	}

	if (!globals.media_ip) {
		globals.media_ip = g_strdup("127.0.0.1");
	}

	return count;

}

int reload(void)
{
	config_jabber(1);
	return 0;
}


int load_module(void)
{
	config_jabber(0);
	
	jabber_profile_init(&global_profile, globals.resource, globals.resource, NULL, JFLAG_MAIN);
	launch_jabber_thread(&global_profile);
#ifdef PATCHED_MANAGER
	if (globals.event_master) {
		cw_log(LOG_NOTICE, "Registering Manager Event Hook\n");
		add_manager_hook(&jabber_hook);
	}
#endif
	app = cw_register_application(name, res_jabber_exec, synopsis, syntax, desc);
	return 0;
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}
