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
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include "faxmodem.h"

static int NEXT_ID = 0;
static int REF_COUNT = 0;

static struct {
	faxmodem_logger_t func;
	int err;
	int warn;
	int info;
} LOGGER = {};

#define do_log(id, fmt, ...) if(LOGGER.func) LOGGER.func(id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__);


struct faxmodem_state {
	int state;
	char *name;
};

static struct faxmodem_state FAXMODEM_STATE[] = {
	{FAXMODEM_STATE_INIT, "INIT"},
	{FAXMODEM_STATE_ONHOOK,	"ONHOOK"},
	{FAXMODEM_STATE_ACQUIRED, "ACQUIRED"},
	{FAXMODEM_STATE_RINGING, "RINGING"},
	{FAXMODEM_STATE_ANSWERED, "ANSWERED"},
	{FAXMODEM_STATE_CALLING, "CALLING"},
	{FAXMODEM_STATE_CONNECTED, "CONNECTED"},
	{FAXMODEM_STATE_HANGUP, "HANGUP"},
	{FAXMODEM_STATE_LAST, "UNKNOWN"}
};

static int t31_at_tx_handler(at_state_t *s, void *user_data, const uint8_t *buf, size_t len)
{
	struct faxmodem *fm = user_data;
	ssize_t wrote = write(fm->master, buf, len);
	if (wrote != len) {
		do_log(LOGGER.err, "Unable to pass the full buffer onto the device file. %d bytes of %d written.", wrote, len);
	}
	return wrote;
}

static int modem_control_handler(t31_state_t *s, void *user_data, int op, 
				 const char *num)
{
	struct faxmodem *fm = user_data;
	int ret = 0;

	if (fm->control_handler) {
		ret = fm->control_handler(fm, op, num);
	} else {
		do_log(LOGGER.err, "DOH! NO CONTROL HANDLER INSTALLED\n");
	}

	return ret;
}

char *faxmodem_state2name(int state) 
{
	if (state > FAXMODEM_STATE_LAST || state < 0) {
		state = FAXMODEM_STATE_LAST;
	}

	return FAXMODEM_STATE[state].name;

}

void faxmodem_clear_logger(void) 
{
	memset(&LOGGER, 0, sizeof(LOGGER));
}

void faxmodem_set_logger(faxmodem_logger_t logger, int err, int warn, int info) 
{
	LOGGER.func = logger;
	LOGGER.err = err;
	LOGGER.warn = warn;
	LOGGER.info = info;
}

int faxmodem_close(volatile struct faxmodem *fm) 
{
	int r = 0;

	faxmodem_clear_flag(fm, FAXMODEM_FLAG_RUNNING);

	if (fm->master > -1) {
		close(fm->master);
		fm->master = -1;
		r++;
	}

	if (fm->slave > -1) {
		close(fm->slave);
		fm->slave = -1;
		r++;
	}

	REF_COUNT--;
	return r;
}

int faxmodem_init(struct faxmodem *fm, faxmodem_control_handler_t control_handler, const char *device_prefix)
{
	char buf[256];

	memset(fm, 0, sizeof(*fm));

	fm->master = -1;
	fm->slave = -1;

	if (openpty(&fm->master, &fm->slave, NULL, NULL, NULL)) {
		do_log(LOGGER.err, "Fatal error: failed to initialize pty\n");
		return -1;
	}

	ptsname_r(fm->master, buf, sizeof(buf));

	do_log(LOGGER.info, "Opened pty, slave device: %s\n", buf);

	snprintf(fm->devlink, sizeof(fm->devlink), "%s%d", device_prefix, NEXT_ID++);

	if (!unlink(fm->devlink)) {
		do_log(LOGGER.warn, "Removed old %s\n", fm->devlink);
	}

	if (symlink(buf, fm->devlink)) {
		do_log(LOGGER.err, "Fatal error: failed to create %s symbolic link\n", fm->devlink);
		faxmodem_close(fm);
		return -1;
	}

	do_log(LOGGER.info, "Created %s symbolic link\n", fm->devlink);

	if (fcntl(fm->master, F_SETFL, fcntl(fm->master, F_GETFL, 0) | O_NONBLOCK)) {
		do_log(LOGGER.err, "Cannot set up non-blocking read on %s\n", ttyname(fm->master));
		faxmodem_close(fm);
		return -1;
	}
	
	if (t31_init(&fm->t31_state, t31_at_tx_handler, fm, modem_control_handler, fm, 0, 0) < 0) {
		do_log(LOGGER.err, "Cannot initialize the T.31 modem\n");
		faxmodem_close(fm);
        return -1;

	}

	fm->control_handler = control_handler;
	faxmodem_set_flag(fm, FAXMODEM_FLAG_RUNNING);
	fm->state = FAXMODEM_STATE_INIT;
	
	do_log(LOGGER.info, "Fax Modem [%s] Ready\n", fm->devlink);
	REF_COUNT++;
	return 0;
}
