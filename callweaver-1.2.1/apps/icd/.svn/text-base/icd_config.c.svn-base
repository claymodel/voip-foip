/*
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
 * \brief icd_config.c  -  a table of configuration parameters
 * 
 * The icd_config module holds a set of parameters that can be used to
 * initialize any of the icd structures. Each module will have its own set
 * of key values that it can pull out of an icd_config object. In addition,
 * this module holds an icd_config_registry structure, which allows the
 * dynamic translation of a key setting into any type of void * value.
 *
 * Note that this makes no attempt to be typesafe, but that should be ok.
 * It is hard to imagine multiple threads wanting to alter the same
 * set of configuration parameters. If this assumption turns out to be
 * false, the code will need to be reworked.
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
#include "callweaver/icd/icd_config.h"
#include "callweaver/icd/icd_fieldset.h"
#include "callweaver/icd/icd_common.h"

/* This is for getting at the function pointers in the registry */
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_list.h"

/*===== Private Types, Variables, and APIs =====*/

/* static icd_module module_id = ICD_CONFIG; */

typedef enum {
    ICD_CONFIG_STATE_CREATED, ICD_CONFIG_STATE_INITIALIZED,
    ICD_CONFIG_STATE_CLEARED, ICD_CONFIG_STATE_DESTROYED,
    ICD_CONFIG_STATE_L, CW_STANDARD
} icd_config_state;

typedef enum {
    ICD_CONFIG_REGNODE_EXACT, ICD_CONFIG_REGNODE_PARENT,
    ICD_CONFIG_REGNODE_XLATE, ICD_CONFIG_REGNODE_L, CW_STANDARD_FIXME
} icd_config_regnode_type;

struct icd_config {
    char name[ICD_STRING_LEN];
    icd_fieldset *entries;
    icd_config_registry *registry;
    icd_config_state state;
    icd_memory *memory;
    int allocated;
};

typedef struct icd_config_registry_node {
    char *key;
    char *key_setting;
    void *value;
    icd_memory *memory;
    icd_config_regnode_type type;
} icd_config_registry_node;

struct icd_config_registry {
    char name[ICD_STRING_LEN];
    icd_fieldset *entries;
    icd_config_state state;
    icd_memory *memory;
    int allocated;
    int validate;
};

icd_config_registry *app_icd_config_registry;

static char *novalue = "novalue";

/* Private functions */

icd_status load_default_registry_entries(icd_config_registry * that);
char *icd_config__create_child_key(char *key, char *setting);
icd_status icd_config__key_value_add(icd_config * that, char *str);
char *icd_config__trim_spaces(char *str);

/***** Init - Destroyer for icd_config *****/

/* Create a configuration set. */
icd_config *create_icd_config(icd_config_registry * registry, char *name)
{
    icd_config *config;
    icd_status result;

    /* make a new config from scratch */
    ICD_MALLOC(config, sizeof(icd_config));
    config->allocated = 1;
    if (config == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD config\n");
        return NULL;
    }
    config->state = ICD_CONFIG_STATE_CREATED;
    result = init_icd_config(config, registry, name);
    if (result != ICD_SUCCESS) {
        ICD_STD_FREE(config);
        return NULL;
    }

    return config;
}

/* Destroy a configuration set. */
icd_status destroy_icd_config(icd_config ** configp)
{
    int clear_result;

    assert(configp != NULL);
    assert((*configp) != NULL);

    if ((clear_result = icd_config__clear(*configp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the configuration set if from the heap */
    if ((*configp)->allocated) {
        (*configp)->state = ICD_CONFIG_STATE_DESTROYED;
        ICD_FREE((*configp));
        *configp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize an already created configuration set. */
icd_status init_icd_config(icd_config * that, icd_config_registry * registry, char *name)
{
    assert(that != NULL);
    strncpy(that->name, name, sizeof(that->name));
    that->entries = create_icd_fieldset(name);
    that->registry = registry;
    that->state = ICD_CONFIG_STATE_INITIALIZED;
    that->allocated = 0;
    return ICD_SUCCESS;
}

/* Clear out a configuration */
icd_status icd_config__clear(icd_config * that)
{
    int result;

    assert(that != NULL);

    /* Detect a previously cleared object, considering it cleared. */
    if (that->state == ICD_CONFIG_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    assert(that->entries != NULL);

    that->registry = NULL;
    result = destroy_icd_fieldset(&(that->entries));
    that->state = ICD_CONFIG_STATE_CLEARED;
    if (result == 0) {
        return ICD_EGENERAL;
    }
    return ICD_SUCCESS;
}

/* Gets a value from the config. Note that this value is already
   translated if it was set with icd_config__set_value() */
void *icd_config__get_value(icd_config * that, char *key)
{

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return icd_fieldset__get_value(that->entries, key);
}

/* Sets a value in the config, using the registry to decide how to do it. */
icd_status icd_config__set_value(icd_config * that, char *key, char *setting)
{
    icd_config_registry_node *regnode;
    icd_config_registry_node *child_regnode;
    char *child_key;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);
    assert(key != NULL);

    if (that->registry == NULL || that->registry->entries == NULL) {
        icd_fieldset__set_value(that->entries, key, setting);
        return ICD_SUCCESS;
    }

    regnode = (icd_config_registry_node *) icd_fieldset__get_value(that->registry->entries, key);
    if (regnode == NULL) {
        if (that->registry->validate == 0) {
            icd_fieldset__set_value(that->entries, key, setting);
            return ICD_SUCCESS;
        } else {
            cw_log(LOG_WARNING, "Could not find key %s in registry %s for configuration %s\n", key,
                correct_null_str(that->registry->name), correct_null_str(that->name));
            return ICD_ENOTFOUND;
        }
    }
    if (regnode->type == ICD_CONFIG_REGNODE_EXACT) {
        icd_fieldset__set_value(that->entries, key, setting);
    } else if (regnode->type == ICD_CONFIG_REGNODE_PARENT) {
        child_key = icd_config__create_child_key(key, setting);
        child_regnode = icd_fieldset__get_value(that->registry->entries, child_key);
        /* If no child, we have a registered key with no valid value. This is always wrong. */
        if (child_regnode == NULL) {
            cw_log(LOG_WARNING, "Could not find child key %s in registry %s for configuration %s\n", child_key,
                correct_null_str(that->registry->name), correct_null_str(that->name));
            ICD_STD_FREE(child_key);
            return ICD_ENOTFOUND;
        }
        ICD_STD_FREE(child_key);
        icd_fieldset__set_value(that->entries, key, child_regnode->value);
    } else {
        cw_log(LOG_ERROR, "Config Registry %s error, invalid registry type for key %s\n",
            correct_null_str(that->name), key);
    }
    return ICD_SUCCESS;
}

/* Sets a void pointer directly in the config, without translation. */
icd_status icd_config__set_raw(icd_config * that, char *key, void *data)
{
    icd_config_registry_node *regnode;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);
    assert(key != NULL);

    if (that->registry == NULL || that->registry->entries == NULL || that->registry->validate == 0) {
        icd_fieldset__set_value(that->entries, key, data);
        return ICD_SUCCESS;
    }
    /* We still need to check for validation of this entry, even if raw. */
    regnode = (icd_config_registry_node *) icd_fieldset__get_value(that->registry->entries, key);
    if (regnode == NULL) {
        cw_log(LOG_WARNING, "Could not find key %s in registry %s for configuration %s\n", key,
            correct_null_str(that->registry->name), correct_null_str(that->name));
        return ICD_ENOTFOUND;
    }
    icd_fieldset__set_value(that->entries, key, data);
    return ICD_SUCCESS;
}

/* Sets a value for a key only if it is not already set. */
icd_status icd_config__set_if_new(icd_config * that, char *key, char *setting)
{
    void *value;

    assert(that != NULL);
    assert(that->entries != NULL);

    value = icd_fieldset__get_value(that->entries, key);
    if (value == NULL) {
        icd_config__set_value(that, key, setting);
    }
    return ICD_SUCCESS;
}

/* Gets a string value out of the config set, duplicating it.
   If it isn't in the set, a default string is duplicated instead.
   Remember to free it! */
char *icd_config__get_strdup(icd_config * that, char *key, char *default_str)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return icd_fieldset__get_strdup(that->entries, key, default_str);
}

/* Copy the contents of a string value into another string buffer */
icd_status icd_config__strncpy(icd_config * that, char *key, char *target, int maxchars)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return icd_fieldset__strncpy(that->entries, key, target, maxchars);
}

/* Returns an integer value from the config set, or a default value if
   key isn't found in the set. */
int icd_config__get_int_value(icd_config * that, char *key, int default_int)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return icd_fieldset__get_int_value(that->entries, key, default_int);
}

/* Gets a void pointer value from the config set, or a default if the key
   isn't found in the set. */
void *icd_config__get_any_value(icd_config * that, char *key, void *default_any)
{
    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return icd_fieldset__get_any_value(that->entries, key, default_any);
}

/* Gets a subset of all the keys out of a config set, creating a
   new set in the process with the begin_key prefix stripped off. */
icd_config *icd_config__get_subset(icd_config * that, char *begin_key)
{
    icd_config *subset;
    icd_config_iterator *iter;
    char *curr_key;
    int begin_len;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);
    assert(begin_key != NULL);

    iter = icd_config__get_key_iterator(that);
    if (iter == NULL) {
        return NULL;
    }
    begin_len = strlen(begin_key);
    subset = create_icd_config(that->registry, begin_key);
    while (icd_config_iterator__has_more(iter)) {
        curr_key = icd_config_iterator__next(iter);
        if (strlen(curr_key) > begin_len && !strncmp(curr_key, begin_key, begin_len)) {
            icd_config__set_value(subset, &(curr_key[begin_len]), icd_config__get_value(that, curr_key));
        }
    }
    destroy_icd_config_iterator(&iter);
    return subset;
}

/* Returns the registry for this config, if any. */
icd_config_registry *icd_config__get_registry(icd_config * that, char *key)
{
    assert(that != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return that->registry;
}

/* Gets an iterator over the keys of the config. */
icd_config_iterator *icd_config__get_key_iterator(icd_config * that)
{
    assert(that != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return (icd_config_iterator *) icd_fieldset__get_key_iterator(that->entries);
}

/*Gets an iterator over the keys of a config registry  */
icd_config_iterator *icd_config__get_registered_keys_iterator(icd_config_registry * that)
{
    assert(that != NULL);
    assert(that->state == ICD_CONFIG_STATE_INITIALIZED);

    return (icd_config_iterator *) icd_fieldset__get_key_iterator(that->entries);
}

/* Returns 0 if at the end of the list of keys, nonzero otherwise */
int icd_config_iterator__has_more(icd_config_iterator * that)
{
    assert(that != NULL);

    return icd_fieldset_iterator__has_more((icd_fieldset_iterator *) that);
}

/* Returns the key string the iterator is currently pointing at. */
char *icd_config_iterator__next(icd_config_iterator * that)
{
    assert(that != NULL);

    return icd_fieldset_iterator__next((icd_fieldset_iterator *) that);
}

/* Destroy the key iterator */
icd_status destroy_icd_config_iterator(icd_config_iterator ** iterp)
{
    assert(iterp != NULL);
    assert((*iterp) != NULL);

    return destroy_icd_fieldset_iterator((icd_fieldset_iterator **) iterp);
}

/***** Init - Destroyer for icd_config_registry *****/

/* Create a configuration options registry */
icd_config_registry *create_icd_config_registry(char *name)
{
    icd_config_registry *registry;
    icd_status result;

    /* make a new config from scratch */
    ICD_MALLOC(registry, sizeof(icd_config_registry));
    if (registry == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new ICD Config Registry\n");
        return NULL;
    }
    registry->allocated = 1;
    registry->state = ICD_CONFIG_STATE_CREATED;
    result = init_icd_config_registry(registry, name);
    if (result != ICD_SUCCESS) {
        ICD_STD_FREE(registry);
        return NULL;
    }

    return registry;
}

/* Destroy a configuration options registry. */
icd_status destroy_icd_config_registry(icd_config_registry ** regp)
{
    int clear_result;

    assert(regp != NULL);
    assert((*regp) != NULL);

    if ((clear_result = icd_config_registry__clear(*regp)) != ICD_SUCCESS) {
        return clear_result;
    }

    /* Destroy the registry if from the heap */
    if ((*regp)->allocated) {
        (*regp)->state = ICD_CONFIG_STATE_DESTROYED;
        ICD_FREE((*regp));
        *regp = NULL;
    }
    return ICD_SUCCESS;
}

/* Initialize a previously created configuration registry */
icd_status init_icd_config_registry(icd_config_registry * that, char *name)
{
    assert(that != NULL);

    strncpy(that->name, name, sizeof(that->name));
    that->entries = create_icd_fieldset(name);
    that->state = ICD_CONFIG_STATE_INITIALIZED;
    that->allocated = 0;
    that->validate = 0;
    load_default_registry_entries(that);
    return ICD_SUCCESS;
}

/* Clear a configuration registry. */
icd_status icd_config_registry__clear(icd_config_registry * that)
{
    int result;

    assert(that != NULL);

    if (that->state == ICD_CONFIG_STATE_CLEARED) {
        return ICD_SUCCESS;
    }

    assert(that->entries != NULL);

    result = icd_fieldset__clear(that->entries);
    that->validate = 0;
    that->state = ICD_CONFIG_STATE_CLEARED;

    return result;
}

/* Register a key that just passes along its value, whatever that is set to. */
icd_status icd_config_registry__register(icd_config_registry * that, char *key)
{
    icd_config_registry_node *node;

    assert(that != NULL);
    assert(that->entries != NULL);
    assert(key != NULL);

    /* Check to make sure it isn't already in there */
    node = (icd_config_registry_node *) icd_fieldset__get_value(that->entries, key);
    if (node != NULL) {
        cw_log(LOG_WARNING, "Configuration %s Registry key '%s' is duplicated\n", correct_null_str(that->name),
            key);
        return ICD_EGENERAL;
    }
    ICD_MALLOC(node, sizeof(icd_config_registry_node));
    if (node == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new entry in ICD Config Registry %s\n",
            correct_null_str(that->name));
        return ICD_ERESOURCE;;
    }

    node->key = key;
    node->key_setting = NULL;
    node->value = NULL;
    node->type = ICD_CONFIG_REGNODE_EXACT;
    icd_fieldset__set_value(that->entries, key, node);
    return ICD_SUCCESS;
}

/* Register a key, a value for that key, and the pointer that combination should return. */
icd_status icd_config_registry__register_ptr(icd_config_registry * that, char *key, char *keysetting, void *value)
{
    icd_config_registry_node *parent_node;
    icd_config_registry_node *child_node;
    char *child_key;

    assert(that != NULL);
    assert(key != NULL);

    /* look for child node - if found, error */
    child_key = icd_config__create_child_key(key, keysetting);
    child_node = (icd_config_registry_node *) icd_fieldset__get_value(that->entries, child_key);
    if (child_node != NULL) {
        cw_log(LOG_WARNING, "Configuration %s Registry key '%s' for '%s' is duplicated.\n",
            correct_null_str(that->name), key, correct_null_str(value));
        ICD_STD_FREE(child_key);
        return ICD_EGENERAL;
    }

    /* look for parent node - if not found, create */
    parent_node = (icd_config_registry_node *) icd_fieldset__get_value(that->entries, key);
    if (parent_node == NULL) {
        ICD_MALLOC(parent_node, sizeof(icd_config_registry_node));
        if (parent_node == NULL) {
            cw_log(LOG_ERROR, "No memory available to create a parent entry in ICD Config Registry %s\n",
                correct_null_str(that->name));
            return ICD_ERESOURCE;;
        }
        parent_node->key = key;
        parent_node->key_setting = NULL;        /* %TC add a list of keysetting comma delimited */
        parent_node->value = NULL;
        parent_node->type = ICD_CONFIG_REGNODE_PARENT;

        icd_fieldset__set_value(that->entries, key, parent_node);
    }

    /* Now create child node */

    ICD_MALLOC(child_node, sizeof(icd_config_registry_node));
    if (child_node == NULL) {
        cw_log(LOG_ERROR, "No memory available to create a new entry in ICD Config Registry %s\n",
            correct_null_str(that->name));
        return ICD_ERESOURCE;;
    }
    child_node->key = key;
    child_node->key_setting = keysetting;
    child_node->value = value;
    child_node->type = ICD_CONFIG_REGNODE_PARENT;
    icd_fieldset__set_value(that->entries, child_key, child_node);
    ICD_STD_FREE(child_key);

    return ICD_SUCCESS;
}

/* Should the registry require all config keys to be registered? */
icd_status icd_config_registry__set_validate(icd_config_registry * that, int validate)
{
    assert(that != NULL);

    if (validate == 0) {
        that->validate = 0;
        return ICD_SUCCESS;
    }
    that->validate = 1;
    return ICD_SUCCESS;
}

/* Returns whether the registry requires all config keys to be registered. */
int icd_config_registry__get_validate(icd_config_registry * that)
{
    assert(that != NULL);

    return that->validate;
}

/***** Private Functions *****/

/* Loads a registry with the standard entries icd supports */

icd_status load_default_registry_entries(icd_config_registry * that)
{
    assert(that != NULL);

    /* Common settings */
    icd_config_registry__register(that, "name");

    /* icd_distributor settings */
    icd_config_registry__register(that, "agentsfile");
    icd_config_registry__register(that, "agents.name");
    icd_config_registry__register(that, "agents.size");
    icd_config_registry__register(that, "agents.init");
    icd_config_registry__register(that, "agents.insert");
    icd_config_registry__register(that, "agents.insert.extra");
    icd_config_registry__register(that, "agents.compare");
    icd_config_registry__register(that, "agents.key");
    icd_config_registry__register(that, "agents.add.notify");
    icd_config_registry__register(that, "agents.add.notify.extra");
    icd_config_registry__register(that, "agents.remove.notify");
    icd_config_registry__register(that, "agents.remove.notify.extra");
    icd_config_registry__register(that, "agents.clear.notify");
    icd_config_registry__register(that, "agents.clear.notify.extra");
    icd_config_registry__register(that, "agents.destroy.notify");
    icd_config_registry__register(that, "agents.destroy.notify.extra");
    icd_config_registry__register(that, "customers.name");
    icd_config_registry__register(that, "customers.size");
    icd_config_registry__register(that, "customers.init");
    icd_config_registry__register(that, "customers.insert");
    icd_config_registry__register(that, "customers.insert.extra");
    icd_config_registry__register(that, "customers.compare");
    icd_config_registry__register(that, "customers.key");
    icd_config_registry__register(that, "customers.add.notify");
    icd_config_registry__register(that, "customers.add.notify.extra");
    icd_config_registry__register(that, "customers.remove.notify");
    icd_config_registry__register(that, "customers.remove.notify.extra");
    icd_config_registry__register(that, "customers.clear.notify");
    icd_config_registry__register(that, "customers.clear.notify.extra");
    icd_config_registry__register(that, "customers.destroy.notify");
    icd_config_registry__register(that, "customers.destroy.notify.extra");

    /* Builtin Distributor strategies */
    icd_config_registry__register_ptr(that, "dist", "roundrobin", init_icd_distributor_round_robin);
    icd_config_registry__register_ptr(that, "dist", "lifo", init_icd_distributor_least_recent_agent);
    icd_config_registry__register_ptr(that, "dist", "fifo", init_icd_distributor_most_recent_agent);
    icd_config_registry__register_ptr(that, "dist", "priority", init_icd_distributor_agent_priority);
    icd_config_registry__register_ptr(that, "dist", "callcount", init_icd_distributor_least_calls_agent);
    icd_config_registry__register_ptr(that, "dist", "random", init_icd_distributor_random);
    icd_config_registry__register_ptr(that, "dist", "ringall", init_icd_distributor_ringall);
    /* Built in distributor actions when we pop customer/agent callers off the
     * distributor lists these action are usually shared between strategies
     */
    icd_config_registry__register_ptr(that, "dist.link", "pop", icd_distributor__link_callers_via_pop);
    icd_config_registry__register_ptr(that, "dist.link", "popandpush",
        icd_distributor__link_callers_via_pop_and_push);
    icd_config_registry__register_ptr(that, "dist.link", "ringall", icd_distributor__link_callers_via_ringall);
    /*default plugable distributor plugable function finder 
     * this allow the a dist to return the correct plugable functions
     * for a given strategy / role should install a default plugable or
     * only use it optionally with custom distr strategies
     icd_config_registry__register_ptr(that, "dist.plugable.fn", "standard",
     icd_distributor__get_plugable_fns);
     */
    icd_config_registry__register_ptr(that, "dist.plugable.fn", "standard", NULL);

    /* icd_caller settings */
    icd_config_registry__register(that, "moh");
    icd_config_registry__register(that, "announce");

    /* icd_list settings */
    icd_config_registry__register(that, "size");
    icd_config_registry__register_ptr(that, "insert", "fifo", icd_list__insert_fifo);
    icd_config_registry__register_ptr(that, "insert", "lifo", icd_list__insert_lifo);
    icd_config_registry__register_ptr(that, "insert", "random", icd_list__insert_random);
    icd_config_registry__register_ptr(that, "insert", "ordered", icd_list__insert_ordered);
    icd_config_registry__register_ptr(that, "insert.extra", "list.name", icd_list__cmp_name_order);
    icd_config_registry__register_ptr(that, "insert.extra", "list.name.rev", icd_list__cmp_name_reverse_order);
    icd_config_registry__register_ptr(that, "compare", "list.name", icd_list__cmp_name_order);
    icd_config_registry__register_ptr(that, "compare", "list.name.rev", icd_list__cmp_name_reverse_order);
    icd_config_registry__register_ptr(that, "key", "list.name", icd_list__by_name);
    icd_config_registry__register_ptr(that, "add.notify", "dummy", icd_list__dummy_notify_hook);
    icd_config_registry__register_ptr(that, "remove.notify", "dummy", icd_list__dummy_notify_hook);
    icd_config_registry__register_ptr(that, "clear.notify", "dummy", icd_list__dummy_notify_hook);
    icd_config_registry__register_ptr(that, "destroy.notify", "dummy", icd_list__dummy_notify_hook);

    /* Note that there are no standard settings yet for the extra parameter to the notify functions */

    /* TBD - All the rest of the modules */

    /*
       icd_config_registry__register(that, "");
       icd_config_registry__register(that, "");
       icd_config_registry__register(that, "");
       icd_config_registry__register(that, "");
       icd_config_registry__register(that, "");
       icd_config_registry__register(that, "");

       icd_config_registry__register_ptr(that, "", "", );
       icd_config_registry__register_ptr(that, "", "", );
       icd_config_registry__register_ptr(that, "", "", );
       icd_config_registry__register_ptr(that, "", "", );
       icd_config_registry__register_ptr(that, "", "", );
     */
    /* this should be removed */
    icd_config_registry__register(that, "type");

    return ICD_SUCCESS;
}

/* Given a line containing key/value pairs, create entries in an icd_config.
   Note that this modifies the line in-place. Be careful of your lifecycle. 
   (Perhaps the config should keep track of whether the config allocated the 
   key and/or the value?) */
icd_status icd_config__parse(icd_config * that, char *line, char delim)
{
    char *delim_ptr;
    char *pair_ptr;
    icd_status result;

    assert(that != NULL);
    assert(line != NULL);

    pair_ptr = line;
    /* For each key/value pair */
    while ((delim_ptr = strchr(pair_ptr, delim)) != NULL) {
        *delim_ptr = '\0';
        result = icd_config__key_value_add(that, pair_ptr);
        pair_ptr = delim_ptr + 1;
        delim_ptr = NULL;
    }

    /* Don't forget the pair after the last delimiter! */
    result = icd_config__key_value_add(that, pair_ptr);
    return result;
}

/* Takes a string of the form "key=value" and parses it and adds to config */
icd_status icd_config__key_value_add(icd_config * that, char *str)
{
    char *val_ptr;

    assert(that != NULL);
    assert(str != NULL);

    val_ptr = strchr(str, '=');
    if (val_ptr != NULL) {
        *val_ptr = '\0';
        val_ptr++;
        return icd_config__set_value(that, icd_config__trim_spaces(str), icd_config__trim_spaces(val_ptr));
    }
    /* Store key with "novalue" value */
    return icd_config__set_value(that, icd_config__trim_spaces(str), novalue);
}

/* In-place editing to remove spaces from either end of string.  */
char *icd_config__trim_spaces(char *str)
{
    char *begin_ptr;
    char *end_ptr;
    int new_end;

    assert(str != NULL);

    new_end = 0;
    begin_ptr = str;
    end_ptr = (str + strlen(str)) - 1;
    while ((*begin_ptr == ' ' || *begin_ptr == '\t') && begin_ptr <= end_ptr) {
        begin_ptr++;
    }
    while ((*end_ptr == ' ' || *end_ptr == '\t') && end_ptr > begin_ptr) {
        new_end = 1;
        end_ptr--;
    }
    if (new_end) {
        *end_ptr = '\0';
    }
    return begin_ptr;
}

/* malloc's a child key for finding a registry node */
char *icd_config__create_child_key(char *key, char *setting)
{
    char *buf;
    int keylen;

    assert(key != NULL);
    assert(setting != NULL);

    keylen = strlen(key) + strlen(setting) + 2;
    buf = (char *) ICD_STD_MALLOC(keylen);
    /* We need a child node for the value, create the key */
    strcpy(buf, key);
    strcat(buf, "=");
    strcat(buf, icd_config__trim_spaces(setting));
    return buf;
}

void *icd_config__get_param(icd_config * that, char *name)
{
    void_hash_table *hash = icd_config__get_value(that, "params");

    if (!hash)
        cw_verbose("WTF\n");
    return vh_read(hash, name);
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


