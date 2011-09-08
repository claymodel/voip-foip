/*
 * ICD - Intelligent Call Distributor 
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com>
 * Additions, Changes and Support by Tim R. Clark <tclark at shaw dot ca>
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o., Piotr Figurny and Michal Bielicki
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2005, Digium, Inc.
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
 
#ifndef ICD_COMMON_H

#define ICD_COMMON_H

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

#include "callweaver.h"

#include "callweaver/file.h"
#include "callweaver/say.h"
#include "callweaver/logger.h"
#include "callweaver/utils.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/musiconhold.h"
#include "callweaver/cli.h"
#include "callweaver/config.h"
#include "callweaver/manager.h"
#include "callweaver/features.h"

#include "callweaver/icd/icd_fieldset.h"
#include "callweaver/icd/icd_listeners.h"
#include "callweaver/icd/icd_event.h"
#include "callweaver/icd/voidhash.h"
#include "callweaver/icd/icd_config.h"
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_globals.h"
#include "callweaver/icd/icd_plugable_fn.h"

/* 
   Support for pre/post 1.0 rendition of cw_set_(read/write)_format.
   Add the CFLAG -DCW_POST_10 in make.conf to get the 3 arg version *default*
   or comment it to get the 2 arg version.
   This is obsolete as of 06/01/2004 do we nuke this macros 
*/

#ifdef CW_POST_10
#define icd_set_read_format(chan,fmt) cw_set_read_format(chan,fmt,0);
#define icd_set_write_format(chan,fmt) cw_set_write_format(chan,fmt,0);
#else
#define icd_set_read_format(chan,fmt) cw_set_read_format(chan,fmt);
#define icd_set_write_format(chan,fmt) cw_set_write_format(chan,fmt);
#endif
#endif


/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

