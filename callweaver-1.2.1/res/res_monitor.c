/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
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

/*! \file
 *
 * \brief PBX channel monitoring
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>		/* dirname() */

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/res/res_monitor.c $", "$Revision: 4723 $")

#include "callweaver/lock.h"
#include "callweaver/channel.h"
#include "callweaver/logger.h"
#include "callweaver/file.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/manager.h"
#include "callweaver/cli.h"
#include "callweaver/monitor.h"
#include "callweaver/app.h"
#include "callweaver/utils.h"
#include "callweaver/config.h"

CW_MUTEX_DEFINE_STATIC(monitorlock);

static unsigned long seq = 0;

static void *monitor_app;
static const char *monitor_name = "Monitor";
static const char *monitor_synopsis = "Monitor a channel";
static const char *monitor_syntax = "Monitor([file_format[:urlbase]][, [fname_base][, [options]]])";
static const char *monitor_descrip =
"Used to start monitoring a channel. The channel's input and output\n"
"voice packets are logged to files until the channel hangs up or\n"
"monitoring is stopped by the StopMonitor application.\n"
"  file_format		optional, if not set, defaults to \"wav\"\n"
"  fname_base		if set, changes the filename used to the one specified.\n"
"  options:\n"
"    m   - when the recording ends mix the two leg files into one and\n"
"          delete the two leg files.  If the variable MONITOR_EXEC is set, the\n"
"          application referenced in it will be executed instead of\n"
"          soxmix and the raw leg files will NOT be deleted automatically.\n"
"          soxmix or MONITOR_EXEC is handed 3 arguments, the two leg files\n"
"          and a target mixed file name which is the same as the leg file names\n"
"          only without the in/out designator.\n"
"          If MONITOR_EXEC_ARGS is set, the contents will be passed on as\n"
"          additional arguements to MONITOR_EXEC\n"
"          Both MONITOR_EXEC and the Mix flag can be set from the\n"
"          administrator interface\n"
"\n"
"    b   - Don't begin recording unless a call is bridged to another channel\n"
"\nReturns -1 if monitor files can't be opened or if the channel is already\n"
"monitored, otherwise 0.\n"
;

static void *stopmonitor_app;
static const char *stopmonitor_name = "StopMonitor";
static const char *stopmonitor_synopsis = "Stop monitoring a channel";
static const char *stopmonitor_syntax = "StopMonitor";
static const char *stopmonitor_descrip =
	"Stops monitoring a channel. Has no effect if the channel is not monitored\n";

static void *changemonitor_app;
static const char *changemonitor_name = "ChangeMonitor";
static const char *changemonitor_synopsis = "Change monitoring filename of a channel";
static const char *changemonitor_syntax = "ChangeMonitor(filename_base)";
static const char *changemonitor_descrip =
	"Changes monitoring filename of a channel. Has no effect if the channel is not monitored\n"
	"The argument is the new filename base to use for monitoring this channel.\n";


/* Predeclare statics to keep GCC 4.x happy */
static int __cw_monitor_start( struct cw_channel *, const char *, const char *, int);
static int __cw_monitor_stop(struct cw_channel *, int);
static int __cw_monitor_change_fname(struct cw_channel *, const char *, int);
static void __cw_monitor_setjoinfiles(struct cw_channel *, int);

/* Start monitoring a channel */
static int __cw_monitor_start(	struct cw_channel *chan, const char *format_spec,
		const char *fname_base, int need_lock)
{
	int res = 0;
	char tmp[256];

	if (need_lock) {
		if (cw_mutex_lock(&chan->lock)) {
			cw_log(LOG_WARNING, "Unable to lock channel\n");
			return -1;
		}
	}

	if (!(chan->monitor)) {
		struct cw_channel_monitor *monitor;
		char *channel_name, *p;

		/* Create monitoring directory if needed */
		if (mkdir(cw_config_CW_MONITOR_DIR, 0770) < 0) {
			if (errno != EEXIST) {
				cw_log(LOG_WARNING, "Unable to create audio monitor directory: %s\n",
					strerror(errno));
			}
		}

		monitor = malloc(sizeof(struct cw_channel_monitor));
		if (!monitor) {
			if (need_lock) 
				cw_mutex_unlock(&chan->lock);
			return -1;
		}
		memset(monitor, 0, sizeof(struct cw_channel_monitor));

		/* Determine file names */
		if (fname_base && !cw_strlen_zero(fname_base)) {
			int directory = strchr(fname_base, '/') ? 1 : 0;
			/* try creating the directory just in case it doesn't exist */
			if (directory) {
				char *name = strdup(fname_base);
				snprintf(tmp, sizeof(tmp), "mkdir -p \"%s\"",dirname(name));
				free(name);
				cw_safe_system(tmp);
			}
			snprintf(monitor->read_filename, FILENAME_MAX, "%s/%s-in",
						directory ? "" : cw_config_CW_MONITOR_DIR, fname_base);
			snprintf(monitor->write_filename, FILENAME_MAX, "%s/%s-out",
						directory ? "" : cw_config_CW_MONITOR_DIR, fname_base);
			cw_copy_string(monitor->filename_base, fname_base, sizeof(monitor->filename_base));
		} else {
			cw_mutex_lock(&monitorlock);
			snprintf(monitor->read_filename, FILENAME_MAX, "%s/audio-in-%ld",
						cw_config_CW_MONITOR_DIR, seq);
			snprintf(monitor->write_filename, FILENAME_MAX, "%s/audio-out-%ld",
						cw_config_CW_MONITOR_DIR, seq);
			seq++;
			cw_mutex_unlock(&monitorlock);

			if((channel_name = cw_strdupa(chan->name))) {
				while((p = strchr(channel_name, '/'))) {
					*p = '-';
				}
				snprintf(monitor->filename_base, FILENAME_MAX, "%s/%ld-%s",
						 cw_config_CW_MONITOR_DIR, time(NULL),channel_name);
				monitor->filename_changed = 1;
			} else {
				cw_log(LOG_ERROR,"Failed to allocate Memory\n");
				return -1;
			}
		}

		monitor->stop = __cw_monitor_stop;

		/* Determine file format */
		if (format_spec && !cw_strlen_zero(format_spec)) {
			monitor->format = strdup(format_spec);
		} else {
			monitor->format = strdup("wav");
		}
		
		/* open files */
		if (cw_fileexists(monitor->read_filename, NULL, NULL) > 0) {
			cw_filedelete(monitor->read_filename, NULL);
		}
		if (!(monitor->read_stream = cw_writefile(monitor->read_filename,
						monitor->format, NULL,
						O_CREAT|O_TRUNC|O_WRONLY, 0, 0644))) {
			cw_log(LOG_WARNING, "Could not create file %s\n",
						monitor->read_filename);
			free(monitor);
			cw_mutex_unlock(&chan->lock);
			return -1;
		}
		if (cw_fileexists(monitor->write_filename, NULL, NULL) > 0) {
			cw_filedelete(monitor->write_filename, NULL);
		}
		if (!(monitor->write_stream = cw_writefile(monitor->write_filename,
						monitor->format, NULL,
						O_CREAT|O_TRUNC|O_WRONLY, 0, 0644))) {
			cw_log(LOG_WARNING, "Could not create file %s\n",
						monitor->write_filename);
			cw_closestream(monitor->read_stream);
			free(monitor);
			cw_mutex_unlock(&chan->lock);
			return -1;
		}
		chan->monitor = monitor;
		/* so we know this call has been monitored in case we need to bill for it or something */
		pbx_builtin_setvar_helper(chan, "__MONITORED","true");
	} else {
		cw_log(LOG_DEBUG,"Cannot start monitoring %s, already monitored\n",
					chan->name);
		res = -1;
	}

	if (need_lock) {
		cw_mutex_unlock(&chan->lock);
	}
	return res;
}

/* Stop monitoring a channel */
static int __cw_monitor_stop(struct cw_channel *chan, int need_lock)
{
	char *execute;
	char *execute_args;
    int explicit_file_type;
	int delfiles = 0;

	if (need_lock) {
		if (cw_mutex_lock(&chan->lock)) {
			cw_log(LOG_WARNING, "Unable to lock channel\n");
			return -1;
		}
	}

	if (chan->monitor) {
		char filename[ FILENAME_MAX ];

		if (chan->monitor->read_stream) {
			cw_closestream(chan->monitor->read_stream);
		}
		if (chan->monitor->write_stream) {
			cw_closestream(chan->monitor->write_stream);
		}

		if (chan->monitor->filename_changed && !cw_strlen_zero(chan->monitor->filename_base)) {
			if (cw_fileexists(chan->monitor->read_filename,NULL,NULL) > 0) {
				snprintf(filename, FILENAME_MAX, "%s-in", chan->monitor->filename_base);
				if (cw_fileexists(filename, NULL, NULL) > 0) {
					cw_filedelete(filename, NULL);
				}
				cw_filerename(chan->monitor->read_filename, filename, chan->monitor->format);
			} else {
				cw_log(LOG_WARNING, "File %s not found\n", chan->monitor->read_filename);
			}

			if (cw_fileexists(chan->monitor->write_filename,NULL,NULL) > 0) {
				snprintf(filename, FILENAME_MAX, "%s-out", chan->monitor->filename_base);
				if (cw_fileexists(filename, NULL, NULL) > 0) {
					cw_filedelete(filename, NULL);
				}
				cw_filerename(chan->monitor->write_filename, filename, chan->monitor->format);
			} else {
				cw_log(LOG_WARNING, "File %s not found\n", chan->monitor->write_filename);
			}
		}

		if (chan->monitor->joinfiles && !cw_strlen_zero(chan->monitor->filename_base)) {
			char tmp[1024];
			char tmp2[1024];
			char *format = (strcasecmp(chan->monitor->format, "wav49") == 0)  ?  "WAV"  :  chan->monitor->format;
			char *name = chan->monitor->filename_base;
			int directory = strchr(name, '/') ? 1 : 0;
			char *dir = directory ? "" : cw_config_CW_MONITOR_DIR;

			execute_args = pbx_builtin_getvar_helper(chan, "MONITOR_EXEC_ARGS");
			if (execute_args == NULL  ||  execute_args[0] == '\0')
				execute_args = "";
			/* Set the execute application */
			execute = pbx_builtin_getvar_helper(chan, "MONITOR_EXEC");
            explicit_file_type = FALSE;
			if (execute == NULL  ||  execute[0] == '\0')
            { 
				execute = "nice -n 19 soxmix";
				delfiles = 1;
                /* Explicitly specify the file type, so file names with multiple '.'
                   characters don't confuse things. */
                explicit_file_type = TRUE;
			}
			snprintf(tmp,
                     sizeof(tmp),
                     "%s \"%s/%s-in.%s\" \"%s/%s-out.%s\" %s%s \"%s/%s.%s\" %s &",
                     execute,
                     dir,
                     name,
                     format,
                     dir,
                     name,
                     format,
                     (explicit_file_type)  ?  "-t "  :  "",
                     (explicit_file_type)  ?  format  :  "",
                     dir,
                     name,
                     format,
                     execute_args);
			if (delfiles)
            {
                /* Remove legs when done mixing */
				snprintf(tmp2, sizeof(tmp2), "( %s& rm -f \"%s/%s-\"* ) &", tmp, dir ,name);
				cw_copy_string(tmp, tmp2, sizeof(tmp));
			}
			cw_log(LOG_DEBUG,"monitor executing %s\n",tmp);
			if (cw_safe_system(tmp) == -1)
				cw_log(LOG_WARNING, "Execute of %s failed.\n",tmp);
		}
		
		free(chan->monitor->format);
		free(chan->monitor);
		chan->monitor = NULL;
	}

	if (need_lock)
		cw_mutex_unlock(&chan->lock);
	return 0;
}

/* Change monitoring filename of a channel */
static int __cw_monitor_change_fname(struct cw_channel *chan, const char *fname_base, int need_lock)
{
	char tmp[256];
	if ((!fname_base) || (cw_strlen_zero(fname_base))) {
		cw_log(LOG_WARNING, "Cannot change monitor filename of channel %s to null\n", chan->name);
		return -1;
	}
	
	if (need_lock) {
		if (cw_mutex_lock(&chan->lock)) {
			cw_log(LOG_WARNING, "Unable to lock channel\n");
			return -1;
		}
	}

	if (chan->monitor) {
		int directory = strchr(fname_base, '/') ? 1 : 0;
		/* try creating the directory just in case it doesn't exist */
		if (directory) {
			char *name = strdup(fname_base);
			snprintf(tmp, sizeof(tmp), "mkdir -p %s",dirname(name));
			free(name);
			cw_safe_system(tmp);
		}

		snprintf(chan->monitor->filename_base, FILENAME_MAX, "%s/%s", directory ? "" : cw_config_CW_MONITOR_DIR, fname_base);
	} else {
		cw_log(LOG_WARNING, "Cannot change monitor filename of channel %s to %s, monitoring not started\n", chan->name, fname_base);
	}

	if (need_lock)
		cw_mutex_unlock(&chan->lock);

	return 0;
}

static int start_monitor_exec(struct cw_channel *chan, int argc, char **argv)
{
	char tmp[256];
	int joinfiles = 0;
	int waitforbridge = 0;
	int res = 0;

	if (argc > 2) {
		for (; argv[2][0]; argv[2]++) {
			switch (argv[2][0]) {
				case 'b': waitforbridge = 1; break;
				case 'm': joinfiles = 1; break;
			}
		}
		argc--;
	}

	if (waitforbridge) {
		pbx_builtin_setvar_helper(chan, "AUTO_MONITOR_FORMAT", (argc > 0 ? argv[0] : ""));
		pbx_builtin_setvar_helper(chan, "AUTO_MONITOR_FNAME_BASE", (argc > 1 ? argv[1] : ""));
		/* We must not pass the "b" option */
		pbx_builtin_setvar_helper(chan, "AUTO_MONITOR_OPTS", (joinfiles ? "m" : ""));
		return 0;
	}

	res = __cw_monitor_start(chan, argv[0], (argc > 1 ? argv[1] : ""), 1);
	if (res < 0)
		res = __cw_monitor_change_fname(chan, (argc > 1 ? argv[1] : ""), 1);
	__cw_monitor_setjoinfiles(chan, joinfiles);

	return res;
}

static int stop_monitor_exec(struct cw_channel *chan, int argc, char **argv)
{
	return __cw_monitor_stop(chan, 1);
}

static int change_monitor_exec(struct cw_channel *chan, int argc, char **argv)
{
	return __cw_monitor_change_fname(chan, argv[0], 1);
}

static char start_monitor_action_help[] =
"Description: The 'Monitor' action may be used to record the audio on a\n"
"  specified channel.  The following parameters may be used to control\n"
"  this:\n"
"  Channel     - Required.  Used to specify the channel to record.\n"
"  File        - Optional.  Is the name of the file created in the\n"
"                monitor spool directory.  Defaults to the same name\n"
"                as the channel (with slashes replaced with dashes).\n"
"  Format      - Optional.  Is the audio recording format.  Defaults\n"
"                to \"wav\".\n"
"  Mix         - Optional.  Boolean parameter as to whether to mix\n"
"                the input and output channels together after the\n"
"                recording is finished.\n";

static int start_monitor_action(struct mansession *s, struct message *m)
{
	struct cw_channel *c = NULL;
	char *name = astman_get_header(m, "Channel");
	char *fname = astman_get_header(m, "File");
	char *format = astman_get_header(m, "Format");
	char *mix = astman_get_header(m, "Mix");
	char *d;
	
	if ((!name) || (cw_strlen_zero(name))) {
		astman_send_error(s, m, "No channel specified");
		return 0;
	}
	c = cw_get_channel_by_name_locked(name);
	if (!c) {
		astman_send_error(s, m, "No such channel");
		return 0;
	}

	if ((!fname) || (cw_strlen_zero(fname))) {
		/* No filename base specified, default to channel name as per CLI */
		fname = malloc (FILENAME_MAX);
		if (!fname) {
			astman_send_error(s, m, "Could not start monitoring channel");
			cw_mutex_unlock(&c->lock);
			return 0;
		}
		memset(fname, 0, FILENAME_MAX);
		cw_copy_string(fname, c->name, FILENAME_MAX);
		/* Channels have the format technology/channel_name - have to replace that /  */
		if ((d=strchr(fname, '/'))) *d='-';
	}
	
	if (__cw_monitor_start(c, format, fname, 1)) {
		if (__cw_monitor_change_fname(c, fname, 1)) {
			astman_send_error(s, m, "Could not start monitoring channel");
			cw_mutex_unlock(&c->lock);
			return 0;
		}
	}

	if (cw_true(mix)) {
		__cw_monitor_setjoinfiles(c, 1);
	}

	cw_mutex_unlock(&c->lock);
	astman_send_ack(s, m, "Started monitoring channel");
	return 0;
}

static char stop_monitor_action_help[] =
"Description: The 'StopMonitor' action may be used to end a previously\n"
"  started 'Monitor' action.  The only parameter is 'Channel', the name\n"
"  of the channel monitored.\n";

static int stop_monitor_action(struct mansession *s, struct message *m)
{
	struct cw_channel *c = NULL;
	char *name = astman_get_header(m, "Channel");
	int res;
	if ((!name) || (cw_strlen_zero(name))) {
		astman_send_error(s, m, "No channel specified");
		return 0;
	}
	c = cw_get_channel_by_name_locked(name);
	if (!c) {
		astman_send_error(s, m, "No such channel");
		return 0;
	}
	res = __cw_monitor_stop(c, 1);
	cw_mutex_unlock(&c->lock);
	if (res) {
		astman_send_error(s, m, "Could not stop monitoring channel");
		return 0;
	}
	astman_send_ack(s, m, "Stopped monitoring channel");
	return 0;
}

static char change_monitor_action_help[] =
"Description: The 'ChangeMonitor' action may be used to change the file\n"
"  started by a previous 'Monitor' action.  The following parameters may\n"
"  be used to control this:\n"
"  Channel     - Required.  Used to specify the channel to record.\n"
"  File        - Required.  Is the new name of the file created in the\n"
"                monitor spool directory.\n";

static int change_monitor_action(struct mansession *s, struct message *m)
{
	struct cw_channel *c = NULL;
	char *name = astman_get_header(m, "Channel");
	char *fname = astman_get_header(m, "File");
	if ((!name) || (cw_strlen_zero(name))) {
		astman_send_error(s, m, "No channel specified");
		return 0;
	}
	if ((!fname)||(cw_strlen_zero(fname))) {
		astman_send_error(s, m, "No filename specified");
		return 0;
	}
	c = cw_get_channel_by_name_locked(name);
	if (!c) {
		astman_send_error(s, m, "No such channel");
		return 0;
	}
	if (__cw_monitor_change_fname(c, fname, 1)) {
		astman_send_error(s, m, "Could not change monitored filename of channel");
		cw_mutex_unlock(&c->lock);
		return 0;
	}
	cw_mutex_unlock(&c->lock);
	astman_send_ack(s, m, "Stopped monitoring channel");
	return 0;
}

static void __cw_monitor_setjoinfiles(struct cw_channel *chan, int turnon)
{
	if (chan->monitor)
		chan->monitor->joinfiles = turnon;
}

int load_module(void)
{
	monitor_app = cw_register_application(monitor_name, start_monitor_exec, monitor_synopsis, monitor_syntax, monitor_descrip);
	stopmonitor_app = cw_register_application(stopmonitor_name, stop_monitor_exec, stopmonitor_synopsis, stopmonitor_syntax, stopmonitor_descrip);
	changemonitor_app = cw_register_application(changemonitor_name, change_monitor_exec, changemonitor_synopsis, changemonitor_syntax, changemonitor_descrip);

	cw_manager_register2("Monitor", EVENT_FLAG_CALL, start_monitor_action, monitor_synopsis, start_monitor_action_help);
	cw_manager_register2("StopMonitor", EVENT_FLAG_CALL, stop_monitor_action, stopmonitor_synopsis, stop_monitor_action_help);
	cw_manager_register2("ChangeMonitor", EVENT_FLAG_CALL, change_monitor_action, changemonitor_synopsis, change_monitor_action_help);

	cw_monitor_start = __cw_monitor_start;
	cw_monitor_stop = __cw_monitor_stop;
	cw_monitor_change_fname = __cw_monitor_change_fname;
	cw_monitor_setjoinfiles = __cw_monitor_setjoinfiles;

	return 0;
}

int unload_module(void)
{
	cw_unregister_application(monitor_app);
	cw_unregister_application(stopmonitor_app);
	cw_unregister_application(changemonitor_app);

	cw_manager_unregister("Monitor");
	cw_manager_unregister("StopMonitor");
	cw_manager_unregister("ChangeMonitor");
	return 0;
}

char *description(void)
{
	return "Call Monitoring Resource";
}

int usecount(void)
{
	/* Never allow monitor to be unloaded because it will
	   unresolve needed symbols in the channel */
#if 0
	int res;
	STANDARD_USECOUNT(res);
	return res;
#else
	return 1;
#endif
}
