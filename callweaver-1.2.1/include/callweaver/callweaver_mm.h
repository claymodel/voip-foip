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

/*
 * CallWeaver memory usage debugging
 */

#ifndef _CALLWEAVER_CALLWEAVER_MM_H
#define _CALLWEAVER_CALLWEAVER_MM_H

/* Include these now to prevent them from being needed later */
#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/* Undefine any macros */
#undef malloc
#undef calloc
#undef realloc
#undef strdup
#undef strndup
#undef vasprintf

void *__cw_calloc(size_t nmemb, size_t size, const char *file, int lineno, const char *func);
void *__cw_malloc(size_t size, const char *file, int lineno, const char *func);
void __cw_free(void *ptr, const char *file, int lineno, const char *func);
void *__cw_realloc(void *ptr, size_t size, const char *file, int lineno, const char *func);
char *__cw_strdup(const char *s, const char *file, int lineno, const char *func);
char *__cw_strndup(const char *s, size_t n, const char *file, int lineno, const char *func);
int __cw_vasprintf(char **strp, const char *format, va_list ap, const char *file, int lineno, const char *func);

void __cw_mm_init(void);


/* Provide our own definitions */
#define calloc(a,b) \
	__cw_calloc(a,b,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define malloc(a) \
	__cw_malloc(a,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define free(a) \
	__cw_free(a,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define realloc(a,b) \
	__cw_realloc(a,b,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define strdup(a) \
	__cw_strdup(a,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define strndup(a,b) \
	__cw_strndup(a,b,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#define vasprintf(a,b,c) \
	__cw_vasprintf(a,b,c,__FILE__, __LINE__, __PRETTY_FUNCTION__)

#else
#error "NEVER INCLUDE callweaver_mm.h DIRECTLY!!"
#endif
