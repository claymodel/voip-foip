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
 *  \brief icd_fieldset - a set of extension fields
 * 
 * The icd_fieldset module holds a set of fields that can be used by
 * ICD modules to hold any type of data that is required.
 * 
 * It provides a mechanism for setting and retrieving the contents of a
 * field based on a character string name. It also provides some functions
 * which allow you to retrieve either a field setting or a default value
 * if none is provided.
 * 
 * These functions are not threadsafe. It is unclear whether they need to
 * be. Overwriting with new values isn't a problem since any caller can
 * just make do with what they receive. Removing and adding fields, if it
 * happens frequently, might cause problems, though.
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "callweaver/logger.h"
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_fieldset.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/voidhash.h"

/*===== Private Types, Variables, and APIs =====*/

/* static icd_module module_id = ICD_FIELDSET; */

typedef enum {
    ICD_FIELDSET_STATE_CREATED, ICD_FIELDSET_STATE_INITIALIZED,
    ICD_FIELDSET_STATE_CLEARED, ICD_FIELDSET_STATE_DESTROYED,
    ICD_FIELDSET_STATE_L, CW_STANDARD
} icd_fieldset_state;

typedef enum {
    ICD_FIELDSET_REGNODE_EXACT, ICD_FIELDSET_REGNODE_PARENT,
    ICD_FIELDSET_REGNODE_XLATE, ICD_FIELDSET_REGNODE_L, CW_STANDARD_FIXME
} icd_fieldset_regnode_type;

struct icd_fieldset {
    char name[ICD_STRING_LEN];
    void_hash_table *entries;
    icd_fieldset_state state;
    icd_memory *memory;
    int allocated;
};

struct icd_fieldset_iterator {
    struct vh_keylist *first;
    struct vh_keylist *curr;
    struct vh_keylist *next;
    icd_memory *memory;
};

void_hash_table *icd_fieldset__get_hash(icd_fieldset * that);

/***** Init - Destroyer for icd_fieldset *****/

/* Create a field set. */
icd_fieldset *create_icd_fieldset(char *name)
{
    icd_fieldset *fieldset;
    icd_status result;

    /* make a new fieldset from scratch */
    ICD_MALLOC(fieldset, sizeof(icd_fieldset));
    if (fieldset == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD fieldset\n");
        return NULL;
    }
    fieldset->allocated = 1;
    fieldset->state = ICD_FIELDSET_STATE_CREATED;
    result = init_icd_fieldset(fieldset, name);
    if (result != ICD_SUCCESS) {
        ICD_FREE(fieldset);
        return NULL;
    }

    return fieldset;
}

/* Destroy a field set. */
icd_status destroy_icd_fieldset(icd_fieldset ** fieldsetp)
{
    int clear_result;

    assert(fieldsetp != NULL);
    assert((*fieldsetp) != NULL);

    if ((clear_result = icd_fieldset__clear(*fieldsetp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the field set if from the heap */
    if ((*fieldsetp)->allocated) {
        (*fieldsetp)->state = ICD_FIELDSET_STATE_DESTROYED;
        ICD_FREE((*fieldsetp));
        *fieldsetp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize an already created field set. */
icd_status init_icd_fieldset(icd_fieldset * that, char *name)
{
    assert(that != NULL);

    if (that->allocated != 1)
        ICD_MEMSET_ZERO(that, sizeof(icd_fieldset));

    strncpy(that->name, name, sizeof(that->name));
    that->entries = vh_init(name);
    that->state = ICD_FIELDSET_STATE_INITIALIZED;
    that->allocated = 0;
    return ICD_SUCCESS;
}

/* Clear out a field set */
icd_status icd_fieldset__clear(icd_fieldset * that)
{
    int result;

    assert(that != NULL);

    /* Detect a previously cleared object, considering it cleared. */
    if (that->state == ICD_FIELDSET_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    assert(that->entries != NULL);

    result = vh_destroy(&(that->entries));
    that->state = ICD_FIELDSET_STATE_CLEARED;
    if (result == 0) {
        return ICD_EGENERAL;
    }
    return ICD_SUCCESS;
}

/***** Actions *****/

/* Gets a value from the fieldset, null if field not present */
void *icd_fieldset__get_value(icd_fieldset * that, char *key)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    return vh_read(that->entries, key);
}

/* Sets a value in the fieldset, using the registry to decide how to do it. */
icd_status icd_fieldset__set_value(icd_fieldset * that, char *key, void *setting)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    vh_write(that->entries, key, setting);
    return ICD_SUCCESS;
}

/* Sets a value for a key only if it is not already set. */
icd_status icd_fieldset__set_if_new(icd_fieldset * that, char *key, void *setting)
{
    void *value;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    value = vh_read(that->entries, key);
    if (value == NULL) {
        icd_fieldset__set_value(that, key, setting);
    }
    return ICD_SUCCESS;
}

/* Gets a string value out of the field set, duplicating it.
   If it isn't in the set, a default string is duplicated instead.
   Remember to free it! */
char *icd_fieldset__get_strdup(icd_fieldset * that, char *key, char *default_str)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    return vh_get_strdup(that->entries, key, default_str);
}

/* Copy the contents of a string value into another string buffer */
icd_status icd_fieldset__strncpy(icd_fieldset * that, char *key, char *target, int maxchars)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    if (strncpy_if_exists(that->entries, key, target, maxchars) == 0) {
        return ICD_EGENERAL;
    }
    return ICD_SUCCESS;
}

/* Returns an integer value from the field set, or a default value if
   key isn't found in the set. */
int icd_fieldset__get_int_value(icd_fieldset * that, char *key, int default_int)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    return atoi_if_exists(that->entries, key, default_int);
}

/* Gets a void pointer value from the field set, or a default if the key
   isn't found in the set. */
void *icd_fieldset__get_any_value(icd_fieldset * that, char *key, void *default_any)
{
    void *any;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    any = vh_read(that->entries, key);
    if (any == NULL) {
        return default_any;
    }
    return any;
}

/* Gets a subset of all the keys out of a field set, creating a
   new set in the process. */
icd_fieldset *icd_fieldset__get_subset(icd_fieldset * that, char *begin_key)
{
    icd_fieldset *subset;
    icd_fieldset_iterator *iter;
    char *curr_key;
    int begin_len;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(begin_key != NULL);

    iter = icd_fieldset__get_key_iterator(that);
    if (iter == NULL) {
        return NULL;
    }
    begin_len = strlen(begin_key);
    subset = create_icd_fieldset(begin_key);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        if (strlen(curr_key) > begin_len && strncmp(curr_key, begin_key, begin_len)) {
            icd_fieldset__set_value(subset, &(curr_key[begin_len]), icd_fieldset__get_value(that, curr_key));
        }
    }
    destroy_icd_fieldset_iterator(&iter);
    return subset;
}

/* Remove a field from the fieldset, based on the field name */
icd_status icd_fieldset__remove_key(icd_fieldset * that, char *key)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);
    assert(key != NULL);

    if (vh_delete(that->entries, key)) {
        return ICD_SUCCESS;
    }
    return ICD_ENOTFOUND;
}

/* Remove a field from a fieldset based on its value (first only) */
icd_status icd_fieldset__remove_value(icd_fieldset * that, void *value)
{
    icd_fieldset_iterator *iter;
    char *curr_key;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);

    iter = icd_fieldset__get_key_iterator(that);
    if (iter == NULL) {
        return ICD_ERESOURCE;
    }
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        if (icd_fieldset__get_value(that, curr_key) == value) {
            icd_fieldset__remove_key(that, curr_key);
            destroy_icd_fieldset_iterator(&iter);
            return ICD_SUCCESS;
        }
    }
    destroy_icd_fieldset_iterator(&iter);
    return ICD_ENOTFOUND;
}

/* Remove all fields from a fieldset */
icd_status icd_fieldset__remove_all(icd_fieldset * that)
{
    icd_fieldset_iterator *iter;
    char *curr_key;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);

    iter = icd_fieldset__get_key_iterator(that);
    if (iter == NULL) {
        return ICD_ERESOURCE;
    }
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        icd_fieldset__remove_key(that, curr_key);
    }
    destroy_icd_fieldset_iterator(&iter);
    return ICD_SUCCESS;
}

/* Given a line containing key/value pairs, create fieldset entries */
icd_status icd_fieldset__parse(icd_fieldset * that, char *line, char delim)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(line != NULL);

    vh_carve_data(that->entries, line, delim);
    return ICD_SUCCESS;
}

/***** Iterator Functions *****/

/* Gets an iterator over the keys of the fieldset. */
icd_fieldset_iterator *icd_fieldset__get_key_iterator(icd_fieldset * that)
{
    icd_fieldset_iterator *iter;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_FIELDSET_STATE_INITIALIZED);

    ICD_MALLOC(iter, sizeof(icd_fieldset_iterator));
    if (iter == NULL) {
        cw_log(LOG_ERROR, "No memory available to create an iterator on ICD Fieldset\n");
        return NULL;
    }

    iter->first = vh_keys(that->entries);
    iter->next = iter->first;
    return iter;
}

/* Returns 0 if at the end of the list of keys, nonzero otherwise */
int icd_fieldset_iterator__has_more(icd_fieldset_iterator * that)
{
    assert(that != NULL);

    return (that->next != NULL);
}

/* Returns the key string the iterator is currently pointing at. */
char *icd_fieldset_iterator__next(icd_fieldset_iterator * that)
{
    assert(that != NULL);

    if (that->next == NULL) {
        if (that->curr == NULL) {
            return NULL;
        } else {
            that->next = that->curr->next;
        }
    }
    that->curr = that->next;
    if (that->curr != NULL) {
        that->next = that->curr->next;
    }
    if (that->curr == NULL) {
        return NULL;
    }
    return that->curr->name;
}

/* Destroy the key iterator */
icd_status destroy_icd_fieldset_iterator(icd_fieldset_iterator ** iterp)
{
    assert(iterp != NULL);
    assert((*iterp) != NULL);

    vh_keylist_destroy(&((*iterp)->first));
    ICD_FREE((*iterp));
    *iterp = NULL;
    return ICD_SUCCESS;
}

/***** Helper Functions *****/

/* Ensures a string that would be null is empty instead. */
char *correct_null_str(char *str)
{
    if (str == NULL) {
        return "";
    }
    return str;
}

/***** Private Functions *****/

/* This is provided for icd_config giving backward compatibility, 
 * and should be removed once the _param() functions are dropped 
 * from icd_config.
 */
void_hash_table *icd_fieldset__get_hash(icd_fieldset * that)
{
    assert(that != NULL);

    return that->entries;
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

