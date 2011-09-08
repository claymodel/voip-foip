/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2007, Eris Associates Ltd
 *
 * Mike Jagdis <mjagdis@eris-associates.co.uk>
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

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#ifdef malloc
#undef malloc
#undef realloc

#include <stdlib.h>


void *rpl_malloc(size_t size)
{
	if (size == 0)
		size = 1;
	return malloc(size);
}


void *rpl_realloc(void *ptr, size_t size)
{
	if (size == 0)
		size = 1;
	return realloc(ptr, size);
}


#endif /* malloc */
