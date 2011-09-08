/*
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of sip-router, a free SIP server.
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
 *
 *
 * History:
 * --------
 *  2003-04-08  init_mallocs split into init_{pkg,shm}_malloc (andrei)
 * 
 */


#include <stdio.h>
#include "../config.h"
#include "../dprint.h"
#include "../globals.h"
#include "mem.h"

#ifdef PKG_MALLOC
#include "q_malloc.h"
#endif

#ifdef SHM_MEM
#include "shm_mem.h"
#endif

#ifdef PKG_MALLOC
	#ifndef DL_MALLOC
	char mem_pool[PKG_MEM_POOL_SIZE];
	#endif

	#ifdef F_MALLOC
		struct fm_block* mem_block;
	#elif defined DL_MALLOC
		/* don't need this */
	#else
		struct qm_block* mem_block;
	#endif
#endif


int init_pkg_mallocs()
{
#ifdef PKG_MALLOC
	/*init mem*/
	#ifdef F_MALLOC
		mem_block=fm_malloc_init(mem_pool, PKG_MEM_POOL_SIZE);
	#elif DL_MALLOC
		/* don't need this */
	#else
		mem_block=qm_malloc_init(mem_pool, PKG_MEM_POOL_SIZE);
	#endif
	#ifndef DL_MALLOC
	if (mem_block==0){
		LOG(L_CRIT, "could not initialize memory pool\n");
		fprintf(stderr, "Too much pkg memory demanded: %d\n",
			PKG_MEM_POOL_SIZE );
		return -1;
	}
	#endif
#endif
	return 0;
}



int init_shm_mallocs(int force_alloc)
{
#ifdef SHM_MEM
	if (shm_mem_init(force_alloc)<0) {
		LOG(L_CRIT, "could not initialize shared memory pool, exiting...\n");
		 fprintf(stderr, "Too much shared memory demanded: %ld\n",
			shm_mem_size );
		return -1;
	}
#endif
	return 0;
}


