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
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "callweaver/icd/voidhash.h"
#include "callweaver/icd/icd_types.h"

/*--- VH_ElfHash ---------------------------------------------------
 *  The published hash algorithm used in the UNIX ELF format
 *  for object files. Accepts a pointer to a string to be hashed
 *  and returns an unsigned long.
 *-------------------------------------------------------------*/
unsigned long VH_ElfHash(const unsigned char *name)
{
    unsigned long h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xF0000000))
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

hash_storage *vh_init_hash_storage()
{

    hash_storage *new_store;

    ICD_MALLOC(new_store, sizeof(hash_storage));

    if (!new_store) {
        return NULL;
    }
    new_store->allocated = 1;
    return new_store;
}

void vh_destroy_hash_storage(hash_storage ** nuke)
{
#ifdef USE_APR
    ICD_FREE((*nuke));
#else

    if ((*nuke) && (*nuke)->allocated) {
        if ((*nuke)->var != NULL) {
            ICD_STD_FREE((*nuke)->var);
        }

        if ((*nuke)->val != NULL)
            ICD_STD_FREE((*nuke)->val);
        if ((*nuke) != NULL) {
            ICD_STD_FREE(*nuke);
            *nuke = NULL;
        }
    }
#endif

}

void_hash_table *vh_init(char *name)
{
    void_hash_table *newhash;
    int key;

    ICD_MALLOC(newhash, sizeof(void_hash_table));

    if (!newhash)
        return NULL;

    strncpy(newhash->name, name, sizeof(newhash->name));
    for (key = 0; key <= VOID_HASH_TABLE_KEY_SIZE; key++) {
        newhash->data[key] = (hash_storage *) vh_init_hash_storage();
    }

    newhash->allocated = 1;
    return newhash;
}

int vh_destroy(void_hash_table ** hash)
{
    int ret = 1, key = 0;

    if (!(*hash)) {
        return 0;
    }

    for (key = 0; key <= VOID_HASH_TABLE_KEY_SIZE; key++) {
        if ((*hash)->data[key] && (*hash)->data[key]->allocated) {
            vh_destroy_hash_storage(&(*hash)->data[key]);
        }
    }

    if ((*hash)->allocated) {
        ICD_FREE((*hash));
        *hash = NULL;
    }

    if (hash) {
        ret = 0;
    }
    return ret;
}

int vh_write_store(void_hash_table * hash, hash_storage * new)
{
    hash_storage *store, *last;
    unsigned long key = VH_ElfHash((unsigned char *) new->var) & VOID_HASH_TABLE_KEY_SIZE;
    int ow = 0;

    store = last = NULL;

    store = hash->data[key];
    while (store != NULL) {
        if (new->var && store->var != NULL && strlen(store->var) && !strcmp(store->var, new->var))
            break;
        last = store;
        store = store->next;
    }

    if (!store) {
        store = new;
    } else {
        ow = 1;
    }

    if (!ow) {
        if (last)
            last->next = store;
        else
            hash->data[key] = store;
    }

    return key;
}

int vh_write(void_hash_table * hash, char *name, void *value)
{
    hash_storage *store, *last;
    unsigned long key = VH_ElfHash((unsigned char *) name) & VOID_HASH_TABLE_KEY_SIZE;
    int ow = 0;

    store = last = NULL;

    store = hash->data[key];
    while (store != NULL) {
        if (name && store->var != NULL && strlen(store->var) && !strcmp(store->var, name))
            break;
        last = store;
        store = store->next;
    }

    if (!store) {
        store = (hash_storage *) vh_init_hash_storage();

#ifdef USE_APR

        store->var = icd_apr__submalloc(hash->memory, strlen(name) + 1);
#else
        store->var = ICD_STD_MALLOC(strlen(name) + 1);
#endif
        if (!store->var)
            return -1;

        strcpy(store->var, name);
    } else {
        ow = 1;
    }

    store->val = value;

    if (!ow) {
        if (last)
            last->next = store;
        else
            hash->data[key] = store;
    }

    return key;
}

int vh_write_cp_string(void_hash_table * hash, char *key, char *string)
{
    int res = 0;
    char *ptr;

#ifdef USE_APR
    ptr = icd_apr__submalloc(hash->memory, strlen(string) + 1);
#else
    ptr = ICD_STD_MALLOC(strlen(string) + 1);
#endif

    if (!ptr)
        res = 0;

    if (ptr) {
        strncpy(ptr, string, strlen(string) + 1);
        res = vh_write(hash, key, ptr);
    }
    return res;
}

void *vh_read(void_hash_table * hash, char *name)
{
    unsigned long key = VH_ElfHash((unsigned char *) name) & VOID_HASH_TABLE_KEY_SIZE;
    hash_storage *store;

    store = hash->data[key];
    while (store) {

        if (name && store->var && !strcmp(name, store->var)) {
            return store->val;
        }
        store = store->next;
    }

    return NULL;
}

int vh_delete(void_hash_table * hash, char *name)
{
    unsigned long key = VH_ElfHash((unsigned char *) name) & VOID_HASH_TABLE_KEY_SIZE;
    hash_storage *store = NULL, *last = NULL;
    int ret = 0;

    store = hash->data[key];
    while (store) {
        if (name && store->var && !strcmp(store->var, name)) {

            if (last) {
                last->next = store->next;
            } else if(store->next){
            	hash->data[key] = store->next;
            }
            else  {
                hash->data[key] = (hash_storage *) vh_init_hash_storage();
            }

            ICD_STD_FREE(store);

            ret = 1;
            break;
        }
        last = store;
        store = store->next;
    }

    return ret;
}

char *vh_trim_spaces(char *var)
{
    int y = 0;

    for (; var[0] == ' '; var++) ;
    y = strlen(var) - 1;
    for (y = strlen(var) - 1; var[y] == ' '; y--) {
        var[y] = '\0';
        var[y + 1] = ' ';       /* TBD - What is this for? Can we remove it? */
    }
    return var;
}

int split_and_add(void_hash_table * hash, char *pair)
{
    char *var = pair;
    char *val = NULL;
    char *ret;
    char helper[50];
    int x = 0, y = 0;

    for (val = pair; val && y <= strlen(pair); val++, y++) {
        if (val[0] == '=') {
            val[0] = '\0';
            val++;
            x = 1;
            break;
        }
    }

    if (x < 1) {
        val = pair;
        for (x = 0;; x++) {
            sprintf(helper, "%d", x);
            ret = vh_read(hash, helper);
            if (!ret)
                break;
        }

        var = helper;
    }

    /* trim leading and trailing spaces */

    var = vh_trim_spaces(var);
    val = vh_trim_spaces(val);

    return vh_write_cp_string(hash, var, val);
}

void vh_print(void_hash_table * hash)
{
    int key;
    hash_storage *store;

    for (key = 0; key <= VOID_HASH_TABLE_KEY_SIZE; key++) {
        if (hash->data[key]) {
            store = hash->data[key];

            if (store) {
                if (!store->next) {
                    if (store->var)
                        printf("'%s'='%s'\n", store->var, (char *) store->val);
                } else {
                    store = store->next;
                    while (store) {
                        if (store->var)
                            printf("'%s'='%s'\n", store->var, (char *) store->val);
                        store = store->next;
                    }
                }
            }
        }
    }
}

vh_keylist *vh_keylist_init(void)
{
    vh_keylist *new;

    ICD_MALLOC(new, sizeof(vh_keylist));

    if (!new)
        return NULL;

    return new;

}

void vh_keylist_destroy(vh_keylist ** nuke)
{

    ICD_FREE((*nuke));
    *nuke = NULL;
}

vh_keylist *vh_keys(void_hash_table * hash)
{
    int key;
    hash_storage *store;
    vh_keylist *top, *new = NULL, *cur = NULL;

    cur = top = NULL;

    for (key = 0; key <= VOID_HASH_TABLE_KEY_SIZE; key++) {
        if (hash->data[key]) {
            for (store = hash->data[key]; store; store = store->next) {
                if (store->var && strlen(store->var)) {
                    new = (vh_keylist *) vh_keylist_init();
                    strncpy(new->name, store->var, sizeof(new->name));
                    if (!top)
                        top = new;

                    if (!cur) {
                        cur = new;
                    } else if (cur && !cur->next) {
                        cur->next = new;
                        cur = cur->next;
                    }

                }
            }
        }
    }

    return top;
}

/*
  takes a hash obj, long string (char *) deliminated by (char delim)
  and chops up data and copies it into the hash obj
  name=fred|age=12|conf=yes

*/

int vh_carve_data(void_hash_table * hash, char *data, char delim)
{
    char *p, *args;
    int count = 1;

    args = data;

    while ((p = strchr(args, delim))) {
        *p = '\0';
        p++;

        split_and_add(hash, args);
        count++;
        args = p;
        p = NULL;
    }

    if (args && hash)
        split_and_add(hash, args);

    return count;
}

int strncpy_if_exists(void_hash_table * hash, char *key, char *cpy_to, size_t size)
{
    char *key_test;

    if ((key_test = (char *) vh_read(hash, key)) != NULL) {
        strncpy(cpy_to, key_test, size);
        return 1;
    }

    return 0;
}

int atoi_if_exists(void_hash_table * hash, char *key, int dft)
{
    char *key_test;

    if (((key_test = (char *) vh_read(hash, key)) != NULL) && atoi(key_test)) {
        return atoi(key_test);
    }

    return dft;
}

/* Duplicates a string out of the hash table based on a key. If the key is
   not found and the deflt_str is not NULL, it is duplicated instead.
   Otherwise the function returns NULL. Freeing the object again is
   up to you. */
char *vh_get_strdup(void_hash_table * hash, char *key, char *deflt_str)
{
    char *value = (char *) vh_read(hash, key);

    if (value == NULL) {
        value = deflt_str;
    }
    if (value == NULL) {
        return NULL;
    }
    return strdup(value);
}

int vh_cp_string(void_hash_table * hash, char *key, char *string)
{
    char *ptr;

#ifdef USE_APR
    ptr = icd_apr__submalloc(hash->memory, strlen(string) + 1);
#else
    ptr = ICD_STD_MALLOC(strlen(string) + 1);
#endif

    if (ptr) {
        strncpy(ptr, string, strlen(string) + 1);
        return vh_write(hash, key, ptr);
    } else
        return 0;
}

int vh_carve_data_perm(void_hash_table * hash, char *data, char delim)
{
    char *ptr;

    vh_cp_string(hash, "__data", data);
    ptr = vh_read(hash, "__data");
    return vh_carve_data(hash, ptr, delim);
}

void_hash_table *vh_clone(void_hash_table * hash)
{
    void_hash_table *new = (void_hash_table *) vh_init("");
    vh_keylist *keys = vh_keys(hash), *key;

    for (key = keys; key; key = key->next)
        vh_cp_string(new, key->name, vh_read(hash, key->name));
    return new;
}

void vh_merge(void_hash_table * hash, void_hash_table * new)
{
    vh_keylist *keys = vh_keys(new), *key;

    for (key = keys; key; key = key->next)
        vh_cp_string(hash, key->name, vh_read(new, key->name));
}

void vh_merge_if_undef(void_hash_table * hash, void_hash_table * new)
{
    vh_keylist *keys = vh_keys(new), *key;
    char *ptr;

    for (key = keys; key; key = key->next) {
        ptr = (char *) vh_read(hash, key->name);
        if (!ptr)
            vh_cp_string(hash, key->name, vh_read(new, key->name));
    }
}

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

