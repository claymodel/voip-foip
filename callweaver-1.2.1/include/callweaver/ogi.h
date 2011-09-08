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
 * \brief OGI Extension interfaces - CallWeaver Gateway Interface
 */

#ifndef _CALLWEAVER_OGI_H
#define _CALLWEAVER_OGI_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct ogi_state {
	int fd;		/* FD for general output */
	int audio;	/* FD for audio output */
	int ctrl;	/* FD for input control */
} OGI;

typedef struct ogi_command {
	/* Null terminated list of the words of the command */
	char *cmda[CW_MAX_CMD_LEN];
	/* Handler for the command (channel, OGI state, # of arguments, argument list). 
	    Returns RESULT_SHOWUSAGE for improper arguments */
	int (*handler)(struct cw_channel *chan, OGI *ogi, int argc, char *argv[]);
	/* Summary of the command (< 60 characters) */
	char *summary;
	/* Detailed usage information */
	char *usage;
	struct ogi_command *next;
} ogi_command;

int ogi_register(ogi_command *cmd);
void ogi_unregister(ogi_command *cmd);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_OGI_H */
