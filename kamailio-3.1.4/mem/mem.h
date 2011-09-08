/* $Id$
 *
 * memory related stuff (malloc & friends)
 * 
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
 */
/*
 * History:
 * --------
 *  2003-03-10  __FUNCTION__ is a gcc-ism, defined it to "" for sun cc
 *               (andrei)
 *  2003-03-07  split init_malloc into init_pkg_mallocs & init_shm_mallocs 
 *               (andrei)
 *  2007-02-23   added pkg_info() and pkg_available() (andrei)
 */



#ifndef mem_h
#define mem_h
#include "../config.h"
#include "../dprint.h"

/* fix debug defines, DBG_F_MALLOC <=> DBG_QM_MALLOC */
#ifdef F_MALLOC
	#ifdef DBG_F_MALLOC
		#ifndef DBG_QM_MALLOC
			#define DBG_QM_MALLOC
		#endif
	#elif defined(DBG_QM_MALLOC)
		#define DBG_F_MALLOC
	#endif
#endif

#ifdef PKG_MALLOC
#	ifdef F_MALLOC
#		include "f_malloc.h"
		extern struct fm_block* mem_block;
#	elif defined DL_MALLOC
#		include "dl_malloc.h"
#   else
#		include "q_malloc.h"
		extern struct qm_block* mem_block;
#	endif

	extern char mem_pool[PKG_MEM_POOL_SIZE];


#	ifdef DBG_QM_MALLOC

#	include "src_loc.h" /* src location macros: _SRC_* */
#		ifdef F_MALLOC
#			define pkg_malloc(s) fm_malloc(mem_block, (s), _SRC_LOC_, \
				_SRC_FUNCTION_, _SRC_LINE_)
#			define pkg_free(p)   fm_free(mem_block, (p), _SRC_LOC_,  \
				_SRC_FUNCTION_, _SRC_LINE_)
#			define pkg_realloc(p, s) fm_realloc(mem_block, (p), (s), \
					_SRC_LOC_, _SRC_FUNCTION_, _SRC_LINE_)
#		else
#			define pkg_malloc(s) qm_malloc(mem_block, (s),_SRC_LOC_, \
				_SRC_FUNCTION_, _SRC_LINE_)
#			define pkg_realloc(p, s) qm_realloc(mem_block, (p), (s), \
				_SRC_LOC_, _SRC_FUNCTION_, _SRC_LINE_)
#			define pkg_free(p)   qm_free(mem_block, (p), _SRC_LOC_,  \
				_SRC_FUNCTION_, _SRC_LINE_)
#		endif
#	else
#		ifdef F_MALLOC
#			define pkg_malloc(s) fm_malloc(mem_block, (s))
#			define pkg_realloc(p, s) fm_realloc(mem_block, (p), (s))
#			define pkg_free(p)   fm_free(mem_block, (p))
#		elif defined DL_MALLOC
#			define pkg_malloc(s) dlmalloc((s))
#			define pkg_realloc(p, s) dlrealloc((p), (s))
#			define pkg_free(p)   dlfree((p))
#		else
#			define pkg_malloc(s) qm_malloc(mem_block, (s))
#			define pkg_realloc(p, s) qm_realloc(mem_block, (p), (s))
#			define pkg_free(p)   qm_free(mem_block, (p))
#		endif
#	endif
#	ifdef F_MALLOC
#		define pkg_status()    fm_status(mem_block)
#		define pkg_info(mi)    fm_info(mem_block, mi)
#		define pkg_available() fm_available(mem_block)
#		define pkg_sums()      fm_sums(mem_block)
#	elif defined DL_MALLOC
#		define pkg_status()  0
#		define pkg_info(mi)  0
#		define pkg_available()  0
#		define pkg_sums()  0
#	else
#		define pkg_status()    qm_status(mem_block)
#		define pkg_info(mi)    qm_info(mem_block, mi)
#		define pkg_available() qm_available(mem_block)
#		define pkg_sums()      qm_sums(mem_block)
#	endif
#elif defined(SHM_MEM) && defined(USE_SHM_MEM)
#	include "shm_mem.h"
#	define pkg_malloc(s) shm_malloc((s))
#	define pkg_free(p)   shm_free((p))
#	define pkg_status()  shm_status()
#	define pkg_sums()    shm_sums()
#else
#	include <stdlib.h>
#	include "memdbg.h"
#	define pkg_malloc(s) \
	(  { void *____v123; ____v123=malloc((s)); \
	   MDBG("malloc %p size %lu end %p\n", ____v123, (unsigned long)(s), (char*)____v123+(s));\
	   ____v123; } )
#	define pkg_realloc(p, s) \
	(  { void *____v123; ____v123=realloc(p, s); \
	   MDBG("realloc %p size %lu end %p\n", ____v123, (unsigned long)(s), (char*)____v123+(s));\
	    ____v123; } )
#	define pkg_free(p)  do{ MDBG("free %p\n", (p)); free((p)); }while(0);
#	define pkg_status()
#	define pkg_sums()
#endif

int init_pkg_mallocs();
int init_shm_mallocs(int force_alloc);

/*! generic logging helper for allocation errors in private memory pool/ system */
#ifdef SYSTEM_MALLOC
#define PKG_MEM_ERROR LM_ERR("could not allocate private memory from system")
#else
#define PKG_MEM_ERROR LM_ERR("could not allocate private memory from available pool")
#endif
/*! generic logging helper for allocation errors in shared memory pool */
#define SHM_MEM_ERROR LM_ERR("could not allocate shared memory from available pool")

#endif
