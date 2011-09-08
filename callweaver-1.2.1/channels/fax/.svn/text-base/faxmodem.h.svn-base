/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Fax Channel Driver
 * 
 * Copyright (C) 2005 Anthony Minessale II
 *
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

#ifndef _FAXMODEM_H
#define _FAXMODEM_H

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <byteswap.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <tiffio.h>
#include <spandsp.h>
#include <spandsp/expose.h>

typedef enum {
	FAXMODEM_STATE_INIT,	
	FAXMODEM_STATE_ONHOOK,	
	FAXMODEM_STATE_ACQUIRED,
	FAXMODEM_STATE_RINGING,
	FAXMODEM_STATE_ANSWERED,
	FAXMODEM_STATE_CALLING,
	FAXMODEM_STATE_CONNECTED,
	FAXMODEM_STATE_HANGUP,
	FAXMODEM_STATE_LAST
} faxmodem_state_t;

struct faxmodem;

typedef int (*faxmodem_control_handler_t)(struct faxmodem *, int op, const char *);
typedef int (*faxmodem_logger_t)(int, const char *, int, const char *, const char *, ...);

#define faxmodem_test_flag(p,flag)    ({ \
                                        ((p)->flags & (flag)); \
                                        })

#define faxmodem_set_flag(p,flag)     do { \
                                        ((p)->flags |= (flag)); \
                                        } while (0)

#define faxmodem_clear_flag(p,flag)   do { \
                                        ((p)->flags &= ~(flag)); \
                                        } while (0)

#define faxmodem_copy_flags(dest,src,flagz)   do { \
                                        (dest)->flags &= ~(flagz); \
                                        (dest)->flags |= ((src)->flags & (flagz)); \
                                        } while (0)



typedef enum {
	FAXMODEM_FLAG_RUNNING = ( 1 << 0),
	FAXMODEM_FLAG_ATDT = ( 1 << 1)
} faxmodem_flags;

struct faxmodem {
	t31_state_t t31_state;
	char digits[32];
	unsigned int flags;
	int master;
	int slave;
	char devlink[128];
	int id;
	faxmodem_state_t state;
	faxmodem_control_handler_t control_handler;
	void *user_data;
	int psock;
};

char *faxmodem_state2name(int state);
void faxmodem_clear_logger(void);
void faxmodem_set_logger(faxmodem_logger_t logger, int err, int warn, int info);
int faxmodem_close(volatile struct faxmodem *fm);
int faxmodem_init(struct faxmodem *fm, faxmodem_control_handler_t control_handler, const char *device_prefix);


#endif
