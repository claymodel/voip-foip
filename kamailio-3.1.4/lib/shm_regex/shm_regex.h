/*
 * $Id$
 *
 * Copyright (C) 2009 iptelorg GmbH
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History
 * -------
 *  2009-04-03	Initial version (Miklos)
 *  2010-04-25  Use own struct with locking to work-around libc locking failure
 *              on multi-core hw (skeller)
 */

#ifndef _SHM_REGEX_H
#define _SHM_REGEX_H

#include <sys/types.h>
#include <regex.h>
#include "locking.h"

typedef struct shm_regex {
	regex_t regexp;
	gen_lock_t lock;
} shm_regex_t;

int shm_regcomp(shm_regex_t *preg, const char *regex, int cflags);
void shm_regfree(shm_regex_t *preg);
int shm_regexec(shm_regex_t *preg, const char *string, size_t nmatch,
                   regmatch_t pmatch[], int eflags);
size_t shm_regerror(int errcode, const shm_regex_t *preg, char *errbuf,
                      size_t errbuf_size);

#endif /* _SHM_REGEX_H */
