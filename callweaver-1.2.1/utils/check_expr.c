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

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "callweaver/callweaver_expr.h"

static unsigned int global_lineno = 1;
static unsigned int global_expr_count = 0;
static unsigned int global_expr_max_size = 0;
static unsigned int global_expr_tot_size = 0;
static unsigned int global_warn_count = 0;
static unsigned int global_OK_count = 0;

struct varz
{
	char varname[100]; /* a really ultra-simple, space-wasting linked list of var=val data */
	char varval[1000]; /* Input strings will be truncated if the buffers are not big enough. */
	struct varz *next;
};

struct varz *global_varlist;

/* Our own version of cw_log, since the expr parser uses it. */

void cw_log(int level, const char *file, unsigned int line, const char *function, const char *fmt, ...) __attribute__ ((format (printf,5,6)));

void cw_log(int level, const char *file, unsigned int line, const char *function, const char *fmt, ...)
{
	va_list vars;
	va_start(vars,fmt);
	
	if (printf("LOG: lev:%d file:%s  line:%u func: %s  ",
		       level, file, line, function) < 0)
	{
		exit(errno);
	}

	if (vprintf(fmt, vars) < 0)
	{
		exit(errno);
	}

	if (fflush(stdout))
	{
		exit(errno);
	}

	va_end(vars);
}

char *find_var(const char *varname) /* the list should be pretty short, if there's any list at all */
{
	struct varz *t;
	for (t= global_varlist; t; t = t->next) {
		if (!strcmp(t->varname, varname)) {
			return t->varval;
		}
	}
	return 0;
}

void set_var(const char *varname, const char *varval)
{
	struct varz* t = (struct varz*) calloc(1, sizeof(struct varz));
	
	if (!t)
	{
		exit(ENOMEM);
	}
	
	strncat(t->varname, varname, sizeof(t->varname) - 1);
	strncat(t->varval, varval, sizeof(t->varval) - 1);
	t->next = global_varlist;
	global_varlist = t;
}

unsigned int check_expr(char* buffer, char* error_report)
{
	char* cp;
	unsigned int warn_found = 0;

	error_report[0] = 0;
	
	for (cp = buffer; *cp; ++cp)
	{
		switch (*cp)
		{
			case '"':
				/* skip to the other end */
				while (*(++cp) && *cp != '"') ;

				if (*cp == 0)
				{
					if (fprintf(stderr,
							"Trouble? Unterminated double quote found at line %u\n",
							global_lineno) < 0)
					{
						exit(errno);
					}
				}
				break;
				
			case '>':
			case '<':
			case '!':
				if (   (*(cp + 1) == '=')
					&& ( ( (cp > buffer) && (*(cp - 1) != ' ') ) || (*(cp + 2) != ' ') ) )
				{
					char msg[200];
					snprintf(msg,
						sizeof(msg),
						"WARNING: line %u: '%c%c' operator not separated by spaces. This may lead to confusion. You may wish to use double quotes to quote the grouping it is in. Please check!\n",
						global_lineno, *cp, *(cp + 1));
					strcat(error_report, msg);
					++global_warn_count;
					++warn_found;
				}
				break;
				
			case '|':
			case '&':
			case '=':
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '?':
			case ':':
				if ( ( (cp > buffer) && (*(cp - 1) != ' ') ) || (*(cp + 1) != ' ') )
				{
					char msg[200];
					snprintf(msg,
						sizeof(msg),
						"WARNING: line %u: '%c' operator not separated by spaces. This may lead to confusion. You may wish to use double quotes to quote the grouping it is in. Please check!\n",
						global_lineno, *cp, *(cp + 1));
					strcat(error_report, msg);
					++global_warn_count;
					++warn_found;
				}
				break;
		}
	}

	return warn_found;
}

int check_eval(char *buffer, char *error_report)
{
	char *cp, *ep;
	char s[4096];
	char evalbuf[80000];
	int result;

	error_report[0] = 0;
	ep = evalbuf;

	for (cp = buffer; *cp; ++cp) {
		if (*cp == '$' && *(cp+1) == '{') {
			int brack_lev = 1;
			char *xp= cp+2;
			
			while (*xp) {
				if (*xp == '{')
					brack_lev++;
				else if (*xp == '}')
					brack_lev--;
				
				if (brack_lev == 0)
					break;
				xp++;
			}
			if (*xp == '}') {
				char varname[200];
				char *val;
				
				strncpy(varname,cp+2, xp-cp-2);
				varname[xp-cp-2] = 0;
				cp = xp;
				val = find_var(varname);
				if (val) {
					char *z = val;
					while (*z)
						*ep++ = *z++;
				}
				else {
					*ep++ = '5';  /* why not */
					*ep++ = '5';
					*ep++ = '5';
				}
			}
			else {
				if (printf("Unterminated variable reference at line %u\n", global_lineno) < 0)
				{
					exit(errno);
				}
				*ep++ = *cp;
			}
		}
		else if (*cp == '\\') {
			/* braindead simple elim of backslash */
			cp++;
			*ep++ = *cp;
		}
		else
			*ep++ = *cp;
	}
	*ep++ = 0;

	/* now, run the test */
	result = cw_expr(evalbuf, s, sizeof(s));
	if (result) {
		sprintf(error_report, "line %u, evaluation of $[ %s ] result: %s\n", global_lineno, evalbuf, s);
		return 1;
	} else {
		sprintf(error_report, "line %u, evaluation of $[ %s ] result: ****SYNTAX ERROR****\n", global_lineno, evalbuf);
		return 1;
	}
}


void parse_file(const char *fname)
{
	FILE *f = fopen(fname,"r");
	FILE *l;
	int c1;
	char last_char= 0;
	char buffer[30000]; /* I sure hope no expr gets this big! */
	
	if (!f) {
		if (fprintf(stderr,
				"Couldn't open %s for reading... need an extensions.conf file to parse!\n",
				fname) < 0)
		{
			exit(errno);
		}
		exit(20);
	}
	if (!l) {

	if (!(l = fopen("expr2_log", "w"))) {
		if (fprintf(stderr, "Couldn't open 'expr2_log' file for writing... please fix and re-run!\n") < 0)
		{
			exit(errno);
		}

		exit(21);
	}
	
	global_lineno = 1;
	
	while ((c1 = fgetc(f)) != EOF) {
		if (c1 == '\n')
			global_lineno++;
		else if (c1 == '[') {
			if (last_char == '$') {
				/* bingo, an expr */
				int bracklev = 1;
				unsigned int bufcount = 0;
				int retval;
				char error_report[30000];
				
				while ((c1 = fgetc(f)) != EOF) {
					if (c1 == '[')
						bracklev++;
					else if (c1 == ']')
						bracklev--;
					if (c1 == '\n') {
						if (fprintf(l, "ERROR-- A newline in an expression? Weird! ...at line %u\n", global_lineno) < 0)
						{
							exit(errno);
						}

						if (fclose(f))
						{
							exit(errno);
						}

						if (fclose(l))
						{
							exit(errno);
						}

						if (printf("--- ERROR --- A newline in the middle of an expression at line %u!\n", global_lineno) < 0)
						{
							exit(errno);
						}
					}
					
					if (bracklev == 0)
						break;
					buffer[bufcount++] = c1;
				}
				if (c1 == EOF) {
					if (fprintf(l, "ERROR-- End of File Reached in the middle of an Expr at line %u\n", global_lineno) < 0)
					{
						exit(errno);
					}

					if (fclose(f))
					{
						exit(errno);
					}

					if (fclose(l))
					{
						exit(errno);
					}

					if (printf("--- ERROR --- EOF reached in middle of an expression at line %u!\n", global_lineno) < 0)
					{
						exit(errno);
					}
					exit(22);
				}
				
				buffer[bufcount] = 0;
				/* update stats */
				global_expr_tot_size += bufcount;
				global_expr_count++;
				if (bufcount > global_expr_max_size)
					global_expr_max_size = bufcount;
				
				retval = check_expr(buffer, error_report); /* check_expr should bump the warning counter */
				if (retval != 0) {
					/* print error report */
					if (printf("Warning(s) at line %u, expression: $[%s]; see expr2_log file for details\n", 
						   global_lineno, buffer) < 0)
					{
						exit(errno);
					}

					if (fprintf(l, "%s", error_report) < 0)
					{
						exit(errno);
					}
				}
				else {
					if (printf("OK -- $[%s] at line %u\n", buffer, global_lineno) < 0)
					{
						exit(errno);
					}
					global_OK_count++;
				}
				error_report[0] = 0;
				retval = check_eval(buffer, error_report);

				if (fprintf(l, "%s", error_report) < 0)
				{
					exit(errno);
				}
			}
		}
		last_char = c1;
	}
	if (printf("Summary:\n  Expressions detected: %u\n  Expressions OK:  %u\n  Total # Warnings:   %u\n  Longest Expr:   %u chars\n  Ave expr len:  %u chars\n",
		   global_expr_count,
		   global_OK_count,
		   global_warn_count,
		   global_expr_max_size,
		   global_expr_count ? global_expr_tot_size/global_expr_count : 0) < 0)
	{
		exit(errno);
	}

	if (fclose(f))
	{
		exit(errno);
	}

	if (fclose(l))
	{
		exit(errno);
	}

}


int main(int argc, char **argv)
{
	int argc1;
	char *eq;
	
	if (argc < 2) {
		if (printf("Hey-- give me a path to an extensions.conf file!\n") < 0)
		{
			return errno;
		}

		return 19;
	}
	global_varlist = 0;
	for (argc1=2;argc1 < argc; argc1++) {
		if ((eq = strchr(argv[argc1],'='))) {
			*eq = 0;
			set_var(argv[argc1],eq+1);
		}
	}

	/* parse command args for x=y and set varz */
	
	parse_file(argv[1]);
	return 0;
}
