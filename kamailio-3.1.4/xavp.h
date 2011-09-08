/*
 * $Id$
 *
 * Copyright (C) 2009 Daniel-Constantin Mierla (asipto.com) 
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * History:
 * --------
 *  2009-05-20  created by daniel
 */


#ifndef _SR_XAVP_H_
#define _SR_XAVP_H_

#ifdef WITH_XAVP

#include <time.h>
#include "str.h"

struct _sr_xavp;

typedef enum {
	SR_XTYPE_NULL=0,
	SR_XTYPE_INT,
	SR_XTYPE_STR,
	SR_XTYPE_TIME,
	SR_XTYPE_LONG,
	SR_XTYPE_LLONG,
	SR_XTYPE_XAVP,
	SR_XTYPE_DATA
} sr_xtype_t;

typedef void (*sr_xavp_sfree_f)(void *d);
typedef void (*sr_data_free_f)(void *d, sr_xavp_sfree_f sfree);

typedef struct _sr_data {
	void *p;
	sr_data_free_f pfree;
} sr_data_t;

typedef struct _sr_xval {
	sr_xtype_t type;
	union {
		int i;
		str s;                 /* replicated */
		time_t t;
		long l;
		long long ll;
		struct _sr_xavp *xavp; /* must be given in shm */
		sr_data_t *data;       /* must be given in shm */
	} v;
} sr_xval_t;

typedef struct _sr_xavp {
	unsigned int id;
	str name;
	sr_xval_t val;
	struct _sr_xavp *next;
} sr_xavp_t;

int xavp_init_head(void);
void avpx_free(sr_xavp_t *xa);

sr_xavp_t *xavp_add_value(str *name, sr_xval_t *val, sr_xavp_t **list);
sr_xavp_t *xavp_set_value(str *name, int idx, sr_xval_t *val, sr_xavp_t **list);
sr_xavp_t *xavp_get(str *name, sr_xavp_t *start);
sr_xavp_t *xavp_get_by_index(str *name, int idx, sr_xavp_t **start);
sr_xavp_t *xavp_get_next(sr_xavp_t *start);
int xavp_rm_by_name(str *name, int all, sr_xavp_t **head);
int xavp_rm_by_index(str *name, int idx, sr_xavp_t **head);
int xavp_rm(sr_xavp_t *xa, sr_xavp_t **head);
int xavp_count(str *name, sr_xavp_t **start);
void xavp_destroy_list_unsafe(sr_xavp_t **head);
void xavp_destroy_list(sr_xavp_t **head);
void xavp_reset_list(void);
sr_xavp_t **xavp_set_list(sr_xavp_t **head);
sr_xavp_t **xavp_get_crt_list(void);

void xavp_print_list(sr_xavp_t **head);
#endif

#endif
