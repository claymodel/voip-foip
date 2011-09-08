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
 
/*! \file
 *  \brief icd_memory.h  -  ICD's memory interface
 *
 * Provides an interface for managing memory. This is either raw malloc,
 * a debugging malloc, or APR pools.
 *
 */

#ifndef ICD_MEMORY_H
#define ICD_MEMORY_H

#include "callweaver/icd/icd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Here is another pass at a set of macros we could use. What do you guys think of them?
 *
 * Here are a few things to note:
 *   - declaring a typedef for icd_memory reserves space in the structure for it, even if
 *     it is declared as 'void' (because it becomes 'void *'). If we use a macro instead
 *     we don't waste any space, at the cost of changing the size of the structure based
 *     on compilation options. I'd go with either solution here.
 *   - Do we even need *MALLOC entries? If we always want zeroed out memory (and are
 *     willing to pay the speed price of zeroing it out) perhaps we should stick with
 *     CALLOC only.
 *   - I've created a macro so that one object can inherit another's pool. So
 *     icd_caller can use icd_agent's pool, hash_storage can use
 *     void_hash_table's pool, etc.
 *   - In order to hide the pool altogether (which is probably a good idea, since we may
 *     not be using pools) I've got 4 different types of allocation routines, which for
 *     APR do this:
 *       - GLOBAL - get memory from a global pool (GLOBAL has no prefix: ICD_MALLOC, ICD_CALLOC)
 *       - OBJECT - create a pool, allocate from there, and assign pool into object
 *       - SUB - get memory from this object's pool
 *       - POOL - get memory from a particular pool.
 *     For standard behaviour, OTOH, they do this:
 *       - GLOBAL - standard malloc()/calloc() call
 *       - OBJECT - standard malloc()/calloc() call
 *       - SUB - standard malloc()/calloc() call
 *       - POOL -  standard malloc()/calloc() call
 *   - The FREE calls are likewise of 4 types, and for APR do this:
 *       - GLOBAL - Sets pointer to NULL
 *       - OBJECT - destroy the pool, set pool and pointer to null
 *       - SUB - Sets pointer to NULL, it will be freed on OBJECT free
 *       - POOL - Sets pointer to NULL, it will be freed on POOL free
 *   - A simple rule of thumb:
 *       - We should always allocate like we are using pools
 *       - we should always free like we are using malloc()/free()
 *
 * We probably also have to create our own custom copy of functions which
 * allocate memory, like strcpy() and strncpy().
 *
 * Further ideas and comments welcome.
 */

#ifdef ICD_MALLOC
#undef ICD_MALLOC
#endif
#ifdef ICD_CALLOC
#undef ICD_CALLOC
#endif
#ifdef ICD_FREE
#undef ICD_FREE
#endif
#ifdef ICD_OBJECT_CALLOC
#undef ICD_OBJECT_CALLOC
#endif
#ifdef ICD_OBJECT_FREE
#undef ICD_OBJECT_FREE
#endif
#ifdef ICD_SUBMALLOC
#undef ICD_SUBMALLOC
#endif
#ifdef ICD_SUBCALLOC
#undef ICD_SUBCALLOC
#endif
#ifdef ICD_SUBFREE
#undef ICD_SUBFREE
#endif
#ifdef ICD_POOL_MALLOC
#undef ICD_POOL_MALLOC
#endif
#ifdef ICD_POOL_CALLOC
#undef ICD_POOL_CALLOC
#endif
#ifdef ICD_POOL_FREE
#undef ICD_POOL_FREE
#endif

#ifdef USE_APR
#include <icd_apr.h>

/* malloc gives us memory from the global pool */
#define ICD_MALLOC(obj,objtype)   { (obj) = ((objtype *) icd_apr__malloc(sizeof(objtype))); }
/* calloc gives us a zeroed out block of memory from the global pool */
#define ICD_CALLOC(obj,objtype)   { (obj) = ((objtype *) icd_apr__calloc(sizeof(objtype))); }
#define ICD_FREE(obj)  { (obj) = NULL; }

/* Initializes the pool, allocates the structure, sets pool into structure, returns structure. */
#define ICD_OBJECT_CALLOC(obj,objtype)    {\
    apr_pool_t *pool = icd_apr__new_subpool();\
    (obj) = (objtype *)icd_apr__subcalloc(pool, sizeof(objtype));\
    (obj)->memory = pool; }
/* Free up the memory for an object using its memory manager. */
#define ICD_OBJECT_FREE(obj)  { icd_apr__destroy_subpool((obj)->memory); (obj) = NULL; }

/* submalloc gives us memory from the standard subpool */
#define ICD_SUBMALLOC(obj,objtype)   { (obj) = (objtype *) icd_apr__submalloc(that->memory, sizeof(objtype)); }
/* subcalloc gives us memory from the standard subpool that is zeroed out */
#define ICD_SUBCALLOC(obj,objtype)   { (obj) = (objtype *) icd_apr__subcalloc(that->memory, sizeof(objtype)); }
#define ICD_SUBMULTICALLOC(obj,count,objtype)   { (obj) = (objtype *) icd_apr__subcalloc(that->memory, sizeof(objtype) * count); }
#define ICD_SUBFREE(obj)  { (obj) = NULL; }

/* pool malloc gives us memory from the named subpool */
#define ICD_POOL_MALLOC(pool,obj,objtype)   { (obj) = (objtype *) icd_apr__submalloc(pool, sizeof(objtype)); }
/* pool calloc gives us memory from the named subpool that is zeroed out */
#define ICD_POOL_CALLOC(pool,obj,objtype)   { (obj) = (objtype *) icd_apr__subcalloc(pool, sizeof(objtype)); }
#define ICD_POOL_FREE(obj)  { (obj) = NULL; }

#define ICD_STRDUP(str)    icd_apr__strdup(str)
#define ICD_SUBSTRDUP(pool,str)   icd_apr__substrdup(pool, str);

#else

/* malloc gives us memory from the global pool */
#define ICD_MALLOC(obj,objtype)   { (obj) = (objtype *) malloc(sizeof(objtype)); }
/* calloc gives us a zeroed out block of memory from the global pool */
#define ICD_CALLOC(obj,objtype)   { (obj) = (objtype *) calloc(1, sizeof(objtype)); }
#define ICD_FREE(obj)  { free(obj); (obj) = NULL; }

/* Initializes the pool, allocates the structure, sets pool into structure, returns structure. */
#define ICD_OBJECT_CALLOC(obj,objtype)    { (obj) = (objtype *) calloc(1, sizeof(objtype)); }
/* Free up the memory for an object using its memory manager. */
#define ICD_OBJECT_FREE(obj)  { free(obj); (obj) = NULL; }

/* submalloc gives us memory from the standard subpool */
#define ICD_SUBMALLOC(obj,objtype)   { (obj) = (objtype *) malloc(sizeof(objtype)); }
/* subcalloc gives us memory from the standard subpool that is zeroed out */
#define ICD_SUBCALLOC(obj,objtype)   { (obj) = (objtype *) calloc(1, sizeof(objtype)); }
#define ICD_SUBMULTICALLOC(obj,count,objtype)   { (obj) = (objtype *) calloc(count, sizeof(objtype)); }
#define ICD_SUBFREE(obj)  { free(obj); (obj) = NULL; }

/* pool malloc gives us memory from the named subpool */
#define ICD_POOL_MALLOC(pool,obj,objtype)   { (obj) = (objtype *) malloc(sizeof(objtype)); }
/* pool calloc gives us memory from the named subpool that is zeroed out */
#define ICD_POOL_CALLOC(pool,obj,objtype)   { (obj) = (objtype *) calloc(1, sizeof(objtype)); }
#define ICD_POOL_FREE(obj)  { free(obj); (obj) = NULL; }

#define ICD_STRDUP(str)    strdup(str)
#define ICD_SUBSTRDUP(pool,str)   strdup(str);

#endif

#ifdef __cplusplus
}
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

