/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2005, Anthony Minessale II
 *
 * Anthony Minessale <anthmct@yahoo.com>
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
 * \brief A machine to gather up arbitrary frames and convert them
 * to raw slinear on demand.
 */

#ifndef _CALLWEAVER_SLINFACTORY_H
#define _CALLWEAVER_SLINFACTORY_H
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "callweaver/lock.h"



#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct cw_slinfactory {
	struct {
		struct cw_frame *head;
		struct cw_frame *tail;
		unsigned count;
	} queue;
	struct cw_trans_pvt *trans;
	short hold[1280];
	short *offset;
	size_t holdlen;
	int size;
	int format;
	cw_mutex_t lock;

};

void cw_slinfactory_init(struct cw_slinfactory *sf);
void cw_slinfactory_destroy(struct cw_slinfactory *sf);
int cw_slinfactory_feed(struct cw_slinfactory *sf, struct cw_frame *f);
int cw_slinfactory_read(struct cw_slinfactory *sf, short *buf, size_t bytes);
		 


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_SLINFACTORY_H */
