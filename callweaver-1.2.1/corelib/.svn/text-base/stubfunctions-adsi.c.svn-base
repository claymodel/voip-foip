/*
 * CallWeaver -- An open source telephony toolkit.
 *
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

//#include <callweaver/adsi.h>
#include <stdio.h>
#include "callweaver/channel.h"



static int stub_adsi_begin_download(struct cw_channel *chan, char *service, unsigned char *fdn, unsigned char *sec, int version)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_end_download(struct cw_channel *chan)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_channel_restore(struct cw_channel *chan)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_print(struct cw_channel *chan, char **lines, int *align, int voice)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_load_session(struct cw_channel *chan, unsigned char *app, int ver, int data)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_unload_session(struct cw_channel *chan)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_transmit_messages(struct cw_channel *chan, unsigned char **msg, int *msglen, int *msgtype)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_transmit_message(struct cw_channel *chan, unsigned char *msg, int msglen, int msgtype)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_transmit_message_full(struct cw_channel *chan, unsigned char *msg, int msglen, int msgtype, int dowait)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_read_encoded_dtmf(struct cw_channel *chan, unsigned char *buf, int maxlen)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_connect_session(unsigned char *buf, unsigned char *fdn, int ver)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_query_cpeid(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_query_cpeinfo(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_get_cpeid(struct cw_channel *chan, unsigned char *cpeid, int voice)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_get_cpeinfo(struct cw_channel *chan, int *width, int *height, int *buttons, int voice)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_download_connect(unsigned char *buf, char *service, unsigned char *fdn, unsigned char *sec, int ver)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_disconnect_session(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_download_disconnect(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_data_mode(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_clear_soft_keys(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_clear_screen(unsigned char *buf)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_voice_mode(unsigned char *buf, int when)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_available(struct cw_channel *chan)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_display(unsigned char *buf, int page, int line, int just, int wrap, char *col1, char *col2)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_set_line(unsigned char *buf, int page, int line)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_load_soft_key(unsigned char *buf, int key, char *llabel, char *slabel, char *ret, int data)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_set_keys(unsigned char *buf, unsigned char *keys)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_input_control(unsigned char *buf, int page, int line, int display, int format, int just)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}

static int stub_adsi_input_format(unsigned char *buf, int num, int dir, int wrap, char *format1, char *format2)
{
	cw_log(LOG_NOTICE, "res_adsi not loaded!\n");
	return -1;
}



int (*adsi_begin_download)(struct cw_channel *chan, char *service, unsigned char *fdn, unsigned char *sec, int version) =
	stub_adsi_begin_download;

int (*adsi_end_download)(struct cw_channel *chan) =
	stub_adsi_end_download;

int (*adsi_channel_restore)(struct cw_channel *chan) =
	stub_adsi_channel_restore;

int (*adsi_print)(struct cw_channel *chan, char **lines, int *align, int voice) =
	stub_adsi_print;

int (*adsi_load_session)(struct cw_channel *chan, unsigned char *app, int ver, int data) =
	stub_adsi_load_session;

int (*adsi_unload_session)(struct cw_channel *chan) =
	stub_adsi_unload_session;

int (*adsi_transmit_messages)(struct cw_channel *chan, unsigned char **msg, int *msglen, int *msgtype) =
	stub_adsi_transmit_messages;

int (*adsi_transmit_message)(struct cw_channel *chan, unsigned char *msg, int msglen, int msgtype) =
	stub_adsi_transmit_message;

int (*adsi_transmit_message_full)(struct cw_channel *chan, unsigned char *msg, int msglen, int msgtype, int dowait) =
	stub_adsi_transmit_message_full;

int (*adsi_read_encoded_dtmf)(struct cw_channel *chan, unsigned char *buf, int maxlen) =
	stub_adsi_read_encoded_dtmf;

int (*adsi_connect_session)(unsigned char *buf, unsigned char *fdn, int ver) =
	stub_adsi_connect_session;

int (*adsi_query_cpeid)(unsigned char *buf) =
	stub_adsi_query_cpeid;

int (*adsi_query_cpeinfo)(unsigned char *buf) =
	stub_adsi_query_cpeinfo;

int (*adsi_get_cpeid)(struct cw_channel *chan, unsigned char *cpeid, int voice) =
	stub_adsi_get_cpeid;

int (*adsi_get_cpeinfo)(struct cw_channel *chan, int *width, int *height, int *buttons, int voice) =
	stub_adsi_get_cpeinfo;

int (*adsi_download_connect)(unsigned char *buf, char *service, unsigned char *fdn, unsigned char *sec, int ver) =
	stub_adsi_download_connect;

int (*adsi_disconnect_session)(unsigned char *buf) =
	stub_adsi_disconnect_session;

int (*adsi_download_disconnect)(unsigned char *buf) =
	stub_adsi_download_disconnect;

int (*adsi_data_mode)(unsigned char *buf) =
	stub_adsi_data_mode;

int (*adsi_clear_soft_keys)(unsigned char *buf) =
	stub_adsi_clear_soft_keys;

int (*adsi_clear_screen)(unsigned char *buf) =
	stub_adsi_clear_screen;

int (*adsi_voice_mode)(unsigned char *buf, int when) =
	stub_adsi_voice_mode;

int (*adsi_available)(struct cw_channel *chan) =
	stub_adsi_available;

int (*adsi_display)(unsigned char *buf, int page, int line, int just, int wrap, char *col1, char *col2) =
	stub_adsi_display;

int (*adsi_set_line)(unsigned char *buf, int page, int line) =
	stub_adsi_set_line;

int (*adsi_load_soft_key)(unsigned char *buf, int key, char *llabel, char *slabel, char *ret, int data) =
	stub_adsi_load_soft_key;

int (*adsi_set_keys)(unsigned char *buf, unsigned char *keys) =
	stub_adsi_set_keys;

int (*adsi_input_control)(unsigned char *buf, int page, int line, int display, int format, int just) =
	stub_adsi_input_control;

int (*adsi_input_format)(unsigned char *buf, int num, int dir, int wrap, char *format1, char *format2) =
	stub_adsi_input_format;
