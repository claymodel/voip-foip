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
 * \brief Standard Command Line Interface
 */

#ifndef _CALLWEAVER_CLI_H
#define _CALLWEAVER_CLI_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <stdarg.h>

extern void cw_cli(int fd, char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

#define RESULT_SUCCESS		0
#define RESULT_SHOWUSAGE	1
#define RESULT_FAILURE		2

#define CW_MAX_CMD_LEN 	16

#define CW_MAX_ARGS 64

#define CW_CLI_COMPLETE_EOF	"_EOF_"

/*! \brief A command line entry */ 
struct cw_cli_entry {
	/*! Null terminated list of the words of the command */
	char *cmda[CW_MAX_CMD_LEN];
	/*! Handler for the command (fd for output, # of arguments, argument list).  Returns RESULT_SHOWUSAGE for improper arguments */
	int (*handler)(int fd, int argc, char *argv[]);
	/*! Summary of the command (< 60 characters) */
	char *summary;
	/*! Detailed usage information */
	char *usage;
	/*! Generate a list of possible completions for a given word */
	char *(*generator)(char *line, char *word, int pos, int state);
	/*! For linking */
	struct cw_cli_entry *next;
	/*! For keeping track of usage */
	int inuse;
};

/*! \brief Interprets a command 
 * Interpret a command s, sending output to fd
 * Returns 0 on succes, -1 on failure 
 */
extern int cw_cli_command(int fd, char *s);

/*! \brief Registers a command or an array of commands 
 * \param e which cli entry to register
 * Register your own command
 * Returns 0 on success, -1 on failure
 */
extern int cw_cli_register(struct cw_cli_entry *e);

/*! 
 * \brief Register multiple commands
 * \param e pointer to first cli entry to register
 * \param len number of entries to register
 */
extern void cw_cli_register_multiple(struct cw_cli_entry *e, int len);

/*! \brief Unregisters a command or an array of commands
 *
 * \param e which cli entry to unregister
 * Unregister your own command.  You must pass a completed cw_cli_entry structure
 * Returns 0.
 */
extern int cw_cli_unregister(struct cw_cli_entry *e);

/*!
 * \brief Unregister multiple commands
 * \param e pointer to first cli entry to unregister
 * \param len number of entries to unregister
 */
extern void cw_cli_unregister_multiple(struct cw_cli_entry *e, int len);

/*! \brief Readline madness 
 * Useful for readline, that's about it
 * Returns 0 on success, -1 on failure
 */
extern char *cw_cli_generator(char *, char *, int);

extern int cw_cli_generatornummatches(char *, char *);
extern char **cw_cli_completion_matches(char *, char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_CLI_H */
