/*
 * Intelligent Call Distributor
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
 * \brief Main entry point to the ICD system
 * 
 * This is the main entry point for the ICD program.
 * It is responsible for providing the APIs that CallWeaver requires in
 * order to load ICD as a module, as well as providing all of the set up
 * functions required to initialize ICD so that it is ready to handle calls.
 * It also acts as a registry for Queues and Agents.
 * TBD Should it act as a registry for channels, as well?
 *
 * **
 * Brief description of the function
 * @param parma_1_name explanation
 * @param parma_2_name explanation
 * @param parma_n_name explanation
 * @tip Any extra information people should know.
 * @deffunc function prototype if required
 *
 */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include "callweaver/icd/app_icd.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_globals.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/voidhash.h"
#include "callweaver/icd/icd_command.h"
#include "callweaver/icd/icd_fieldset.h"
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_conference.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_config.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_event.h"
#include "callweaver/app.h"
#include <pthread.h>
#include "callweaver/icd/icd_caller_private.h"

static char *qdesc = "Intelligent Call Distribution System";

static void *icd_customer_app;
static void *icd_queue_app;
static void *icd_agent_app;
static void *icd_logout_app;
static void *icd_add_member_app;
static void *icd_remove_member_app;
static void *icd_agent_callback_app;
static void *icd_customer_callback_app;

static char *icd_customer_app_name = "ICDCustomer";
static char *icd_queue_app_name = "ICDQueue";
static char *icd_agent_app_name = "ICDAgent";
static char *icd_logout_app_name = "ICDLogout";
static char *icd_add_member_app_name = "ICDAddMember";
static char *icd_remove_member_app_name = "ICDRemoveMember";
static char *icd_agent_callback_app_name = "ICDAgentCallback";
static char *icd_customer_callback_app_name = "ICDCustomerCallback";

static char *app_icd_customer_synopsis = "ICD Customer distribution definition";
static char *app_icd_customer_callback_synopsis = "ICD Customer Callback distribution definition";
static char *app_icd_agent_synopsis = "ICD Agent Login";
static char *app_icd_agent_callback_synopsis = "ICD Callback Agent Login";
static char *app_icd_logout_synopsis = "ICD Agent Logout";
static char *app_icd_add_member_synopsis = "ICD dynamicaly add member to a queue";
static char *app_icd_remove_member_synopsis = "ICD remove a member from a queue";

static char *app_icd_customer_syntax = "ICDCustomer(queue=[queueid][,option])";
static char *app_icd_customer_callback_syntax = "ICDCustomerCallback";
static char *app_icd_agent_syntax = "ICDAgent(agent=[agentid or dynamic][,option])";
static char *app_icd_agent_callback_syntax = "ICDAgentCallback";
static char *app_icd_logout_syntax = "ICDLogout(agent=[agentid])";
static char *app_icd_add_member_syntax = "ICDAddMember";
static char *app_icd_remove_member_syntax = "ICDRemoveMmember";

static char *app_icd_customer_desc =
"Creates a distributor for a outside caller (customer) for a queue.\n\nThe option string may contain zero or more of the following:\n\tname=[name for the customer] -- a identifier for this customer access\n\tinfo=[any string] -- free form comment on this customer distributor\n\tpriority=[number] -- a priority for sorting the customer into the queue\n\nIn the ICD system queues get created in the definition files but queue distributors per extension or customer get created in the dialplan. So you have to first define a queue and can than add different kinds of customer access to it, which you define by utilising icd_customer\n\n\t\t---\n\n   icd_customer(conference=[conference or query or autoscan][,option])\nA customer distributor can also be defined via conferences. In that case the option string can be zero or more of:\n\tspy=[1 or 0] -- mute the customer in the conference\n\tconference=[conference,autoscan or query] -- conferecne will place the customer in that conference, autoscan will round robin through the available conferences and if used in combination wity spy=1 behave like zapscan, meaning via pressing the * key you could listen in on conferences one after the other.Query will ask for the customer to enter a conference id.";
static char *app_icd_customer_callback_desc = "";
static char *app_icd_agent_desc =
"Logs an ageent into an ICD queue or icd queues. While logged in, the agent can receive calls.\n\nThe option string may contain zero or more of the following:\n\tqueue=[queueid or space separated list of queues] -- which queues the agent should be a member off\n\tpriority=[priority] -- the priority in the queue of the agent (for priority ditribution = priority)\n\tnoauth=[1 or 0] - if we should authenticate the agent\n\tdunamic=[yes or no] -- if this is a dynamically created agent\n\nDefinitions in the dialplan override definitions in the icd_agent.conf file\n";
static char *app_icd_agent_callback_desc = "";
static char *app_icd_logout_desc = "Logs an agent out of the ICD system\n";
static char *app_icd_add_member_desc = "Let's you dynamically add members to icd queues";
static char *app_icd_remove_member_desc = "Dynamically deletes a queue member from an ICD queue";


#ifdef ICD_TRAP_CORE
static int core_count = 0;
#endif

/* icd verbosity using cw_verbose in caller fsm, icd flag can be set to CW option or run independent*/
int icd_verbose=2;

/* icd debug flag enables debugging using cw_log-debug in caller fsm  */
int icd_debug = 0;

/* delimiter for lists of options in icd conf files */
char icd_delimiter = ',';

/* delimiter for args in interface to callweaver _exec functions */
char cw_delimiter = ',';

/* This is the module mask (icd.conf module_mask=) for what module events to show in the default icd cli.*/
int module_mask[ICD_MAX_MODULES];

/* This is the event mask (icd.conf event_mask=)for what events to show in the default icd cli.*/
int event_mask[ICD_MAX_EVENTS];

/* This is the registry for queues, associating queue names with objects.*/
icd_fieldset *queues;

/* This is the registry for agents, associating agent names with objects. */
icd_fieldset *agents;

/* This is the registry for customers, associating customers names with objects. */
icd_fieldset *customers;

/* This is the lock customers add, remove and seek */
cw_mutex_t customers_lock;

/***** Private APIs, Types, and Variables */

/* static icd_module module_id = APP_ICD; */

/*** For CallWeaver functions ***/

/* Required for interacting with the CallWeaver.org CLI */
static struct cw_cli_entry icd_command_cli_struct = {
    {"icd", NULL, NULL, NULL},
    icd_command_cli, "Execute ICD Command",
    "icd cmd <command>", NULL
};

/* Creates a list of channels controlled by this module ("local" channels) */
STANDARD_LOCAL_USER;

/* Creates other variables that CallWeaver.org requires for the local channel list */
LOCAL_USER_DECL;

/*** For the ICD Module Initialization ***/
static char *default_icd_config = "icd_config/icd.conf";
static char *default_queue_config = "icd_config/icd_queues.conf";
static char *default_agent_config = "icd_config/icd_agents.conf";
static char *default_conference_config = "icd_config/icd_conference.conf";

 CW_MUTEX_DEFINE_STATIC(icdlock);
static int run_conference(cw_channel * chan, icd_customer * customer, icd_conference * conf, char *spymode);

/* Maximum string length for a list of queues */
static const int queue_entry_len = 512;

/* This is the registry for channels, associating channels with callers.
   Key is string form of address of channel. */
/* Coming Soon - static icd_fieldset *channels; */

/* login and password dialog */
icd_agent *app_icd__dtmf_login(struct cw_channel *chan, char *login, char *pass, int tries);


/* Listener on all agents maintained by the agent registry */
static int clear_agent_from_registry(void *listener, icd_event * event, void *extra);

/* Listener on all queues maintained by the queue registry */
static int clear_queue_from_registry(void *listener, icd_event * event, void *extra);

/* Associates agents with a queue. */
static icd_status app_icd__store_agent_list(char *agents, char *queuename, icd_fieldset * map);

/* Associates queues with an agent. */
static icd_status app_icd__store_queue_list(char *queues, char *agentname, icd_fieldset * map);

/* internal reload routines */
static icd_status reload_icd(void);
static icd_status reload_conferences(void);
static icd_status reload_queues(icd_fieldset * outstanding_members);
static icd_status reload_agents(icd_fieldset * outstanding_members);
static icd_status autologin(void);

#ifdef ICD_TRAP_CORE
jmp_buf env;

static int handle_core(int sig)
{
    int f;

    core_count++;
    cw_verbose("\n\n***************** WOAH NELLY  I CAUGHT CORE #%d ! *****************\n\n\n", core_count);
    scanf("%d", &f);
    (void) signal(SIGSEGV, SIG_DFL);
    return 0;

    if (core_count >= 100) {    // i give up
        (void) signal(SIGSEGV, SIG_DFL);
    } else {
        longjmp(env, sig);
    }
    return 0;
}
#endif

/***** CallWeaver.org Required Module API Implementation *****/

/* CallWeaver.org calls this when it loads a module. All of our initialization is here. */
int load_module(void)
{
    
    icd_status result;

    ICD_INIT;

#ifdef ICD_TRAP_CORE
    (void) signal(SIGSEGV, (void *) handle_core);
#endif

    /* need to read icd_config/icd.conf first so we can set global defaults for debug/verbosity 
     * then read again after dynamic module load so its to possible reset eventmasks
     */
    reload_icd();
    /* Create variables from ICD modules */
    app_icd_config_registry = create_icd_config_registry("ICD Config Registry");

    queues = create_icd_fieldset("ICD Queues");
    agents = create_icd_fieldset("ICD Agents");
    customers = create_icd_fieldset("ICD Customers");  
    cw_mutex_init(&customers_lock);

    
    /* TBD convert this is the std icd_field set wrapper on void hash
       conferences = create_icd_fieldset("ICD Conferences"); 
    */
    icd_conference__init_registry();

    /* Now do all the registration with CallWeaver.org. done b4 loadable mods so they can add icd cmds */
    create_command_hash();

    cw_verbose(VERBOSE_PREFIX_2 "APP ICD: Loading external modules.\n");
    icd_module_load_dynamic_module(app_icd_config_registry);

    
    /* init caller agent, customer plugable strucs b4 loading caller, agent, customer */
    result = icd_caller_load_module();

    /* lock and load the queue and agents configurations (is locking necessary?) */
    cw_verbose(VERBOSE_PREFIX_2 "APP ICD: Loading Module[%s].\n", icd_module__to_string(APP_ICD));
    reload_app_icd(APP_ICD);

    cw_cli_register(&icd_command_cli_struct);
    icd_queue_app = cw_register_application(icd_queue_app_name,
                                              app_icd__customer_exec,
                                              app_icd_customer_synopsis,
                                              app_icd_customer_syntax,
                                              app_icd_customer_desc);
    icd_customer_app = cw_register_application(icd_customer_app_name,
                                                 app_icd__customer_exec,
                                                 app_icd_customer_synopsis,
                                                 app_icd_customer_syntax,
                                                 app_icd_customer_desc);
    icd_customer_callback_app = cw_register_application(icd_customer_callback_app_name,
                                                          app_icd__customer_callback_login,
                                                          app_icd_customer_callback_synopsis,
                                                          app_icd_customer_callback_syntax,
                                                          app_icd_customer_callback_desc);
    icd_agent_app = cw_register_application(icd_agent_app_name,
                                              app_icd__agent_exec,
                                              app_icd_agent_synopsis,
                                              app_icd_agent_syntax,
                                              app_icd_agent_desc);
    icd_agent_callback_app = cw_register_application(icd_agent_callback_app_name,
                                                       app_icd__agent_callback_login,
                                                       app_icd_agent_callback_synopsis,
                                                       app_icd_agent_callback_syntax,
                                                       app_icd_agent_callback_desc);
    icd_logout_app = cw_register_application(icd_logout_app_name,
                                               app_icd__logout_exec,
                                               app_icd_logout_synopsis,
                                               app_icd_logout_syntax,
                                               app_icd_logout_desc);
    icd_add_member_app = cw_register_application(icd_add_member_app_name,
                                                   app_icd__add_member_exec,
                                                   app_icd_add_member_synopsis,
                                                   app_icd_add_member_syntax,
                                                   app_icd_add_member_desc);
    icd_remove_member_app = cw_register_application(icd_remove_member_app_name,
                                                      app_icd__remove_member_exec,
                                                      app_icd_remove_member_synopsis,
                                                      app_icd_remove_member_syntax,
                                                      app_icd_remove_member_desc);

    /* If this is the best place to come back to then there is nothing we can do but die.
       if(setjmp(env) == SIGSEGV) {
       cw_log(LOG_ERROR,"I GOTS TA GO!\n");
       (void) signal(SIGSEGV,SIG_DFL);
       abort();
       }
    */
     return 0;
}

/* CallWeaver.org calls this to unload a module. All our cleanup is in here. */
int unload_module(void)
{
    icd_fieldset_iterator *iter;
    char *curr_key;
    icd_queue *queue;
    icd_agent *agent;
    int res = 0;

    ICD_UNINIT;
    cw_log(LOG_WARNING, "ICD unloading from CallWeaver.org, all callers will be lost!\n");
    destroy_icd_config_registry(&app_icd_config_registry);
    icd_conference__destroy_registry();

    STANDARD_HANGUP_LOCALUSERS;

    iter = icd_fieldset__get_key_iterator(agents);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        agent = icd_fieldset__get_value(agents, curr_key);
        cw_log(LOG_DEBUG, "ICD Caller[%d] %s has been cleared\n", icd_caller__get_id((icd_caller *) agent),
                icd_caller__get_name((icd_caller *) agent));
        destroy_icd_agent(&agent);
    }
    destroy_icd_fieldset_iterator(&iter);

    iter = icd_fieldset__get_key_iterator(queues);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        queue = (icd_queue *) icd_fieldset__get_value(queues, curr_key);
        destroy_icd_queue(&queue);
    }
    destroy_icd_fieldset_iterator(&iter);

    destroy_icd_fieldset(&queues);
    destroy_icd_fieldset(&agents);
    cw_mutex_destroy(&customers_lock);
    destroy_icd_fieldset(&customers);
    

    /* must unload loadable mod after callers since we may have function ptrs that 
     * are need till we destroy all agents / customer 
     */
    icd_module_unload_dynamic_modules();

    destroy_icd_config_registry(&app_icd_config_registry);
    icd_conference__destroy_registry();

    res |= cw_unregister_application(icd_queue_app);
    res |= cw_unregister_application(icd_logout_app);
    res |= cw_unregister_application(icd_add_member_app);
    res |= cw_unregister_application(icd_remove_member_app);
    res |= cw_unregister_application(icd_agent_app);
    res |= cw_unregister_application(icd_agent_callback_app);
    res |= cw_unregister_application(icd_customer_app);
    res |= cw_unregister_application(icd_customer_callback_app);

    cw_cli_unregister(&icd_command_cli_struct);
    destroy_command_hash();

    return res;
}

/* This returns a count of the channels we are currently controlling */
int usecount(void)
{
    int channel_count;

    STANDARD_USECOUNT(channel_count);
    return channel_count;
}

/* Gives back a string describing our module */
char *description(void)
{
    return qdesc;
}

/* CallWeaver.org calls this when we are reloaded. All reinitialization is here. */
int reload(void)
{
    icd_status result;

    cw_verbose(VERBOSE_PREFIX_2 "APP ICD: reload request from callweaver cli \n");
    /* the ICD way */
    reload_app_icd(APP_ICD);

    return result;

}

icd_status reload_app_icd(icd_module module)
{
    icd_fieldset *outstanding_members;
    icd_status result;

    /* TBD
       - Create new "queues" and "agents" registries while keeping old ones
       - Read in each queue and agent from configuration files
       - if queue or agent in old registry
       - copy to new registry with potential reconfiguration (possible?)
       - if not, create new queue or caller and add to registry
       - For each queue or agent in old registries
       - if not in new registry, destroy
       - destroy old registries
    */

    /* Records agents memberships in queues before agents have been loaded */
    outstanding_members = create_icd_fieldset("Outstanding memberships");
    /* this assume all the conference, queue, and caller regsitries have been initalized */
    /* lock and load the queue and agents configurations (is locking necessary?) */
    cw_mutex_lock(&icdlock);
    switch (module) {
    case APP_ICD:              /* ok cheating here but app_icd enum means reload all */
        result = reload_icd();
        result = reload_conferences();
        result = reload_queues(outstanding_members);
        result = reload_agents(outstanding_members);
        break;
    case ICD_CONFERENCE:
        result = reload_conferences();
        break;
    case ICD_QUEUE:
        result = reload_queues(outstanding_members);
        break;
    case ICD_AGENT:
        result = reload_agents(outstanding_members);
        break;
    default:
        break;                  /* No Op */
    }
    cw_mutex_unlock(&icdlock);

    /* push all agent with the autologin flag on to theri distributors */
    result = autologin();

    /* icd_read_agents_config() will have dealt with all these, so delete them. */
    destroy_icd_fieldset(&outstanding_members);

    return result;
}

icd_status reload_icd(void)
{
    char *icd_config_name;
    icd_status result = ICD_SUCCESS;

    icd_config_name = default_icd_config;
    result = app_icd__read_icd_config(icd_config_name);

    return result;

}

icd_status reload_conferences(void)
{
    char *conference_config_name;
    icd_status result;

    conference_config_name = default_conference_config;
    result = app_icd__read_conference_config(conference_config_name);

    return result;
}

icd_status reload_queues(icd_fieldset * outstanding_members)
{
    char *queue_config_name;
    icd_status result;

    queue_config_name = default_queue_config;
    result = app_icd__read_queue_config(queues, queue_config_name, outstanding_members);

    return result;
}

icd_status reload_agents(icd_fieldset * outstanding_members)
{
    char *agent_config_name;
    icd_status result;

    agent_config_name = default_agent_config;
    result = app_icd__read_agents_config(agents, agent_config_name, queues, outstanding_members);

    return result;
}

icd_status autologin(void)
{
    icd_fieldset_iterator *iter;
    char *curr_key;
    icd_agent *agent;
    char *fld_autologin;
    char *fld_chanstring;
    char *fld_noauth;

    iter = icd_fieldset__get_key_iterator(agents);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        agent = icd_fieldset__get_value(agents, curr_key);
        /* check to see if we can make this guy ready for calls */
        fld_autologin = icd_caller__get_param((icd_caller *) agent, "autologin");
        fld_chanstring = icd_caller__get_channel_string((icd_caller *) agent);
        fld_noauth = icd_caller__get_param((icd_caller *) agent, "noauth");
        /*
          cw_log(LOG_DEBUG, "ICD Caller[%d] %s has autologin[%s] queues[%s, %d],chanstring[%s] ,noauth[%s], onhook[%d]\n",
          icd_caller__get_id((icd_caller *)agent), 
          icd_caller__get_name((icd_caller *) agent),
          fld_autologin, 
          fld_queues, 
          icd_caller__get_member_count((icd_caller *)agent),
          fld_chanstring, fld_noauth, 
          icd_caller__get_onhook((icd_caller *)agent)
          );
        */
        if (fld_autologin != NULL && cw_true(fld_autologin)) {
            if (fld_noauth != NULL && cw_true(fld_noauth) && (strlen(fld_chanstring) > 0)
                && icd_caller__get_member_count((icd_caller *) agent) > 0
                && icd_caller__get_onhook((icd_caller *) agent)) {

                icd_caller__loop((icd_caller *) agent, 1);
                if (icd_debug)
                    cw_log(LOG_DEBUG, "Caller id[%d] [%s] has autologin set\n",
                            icd_caller__get_id((icd_caller *) agent), icd_caller__get_name((icd_caller *) agent));
            } else {
                cw_log(LOG_WARNING,
                        "Caller id[%d] [%s] has autologin set but missing channel, noauth, onhook, or queues \n",
                        icd_caller__get_id((icd_caller *) agent), icd_caller__get_name((icd_caller *) agent));

            }
        }
    }
    destroy_icd_fieldset_iterator(&iter);

    return ICD_SUCCESS;
}

/***** APIs for initializing ICD *****/

/* this is where the customer enters the queue. Assumes channel already available. */
/* TBD - All these _exec() functions have the same building blocks. These need to be
   refactored out. */
int app_icd__customer_exec(struct cw_channel *chan, int argc, char **argv)
{
    icd_customer *customer;
    char custname[256];
    icd_config *config;
    struct localuser *u;
    icd_queue *queue;
    char *queuename;
    char *conference_name;
    char *identifier;
    icd_conference *conf;
    char *spymode, *nohangup, *pin, *key;
    icd_caller *conf_owner = NULL;
    char input[80];
    vh_keylist *keys;
    int res = 0, agents_only = 0, looping = 0, oldrformat = 0, oldwformat = 0;
    char *cust_uniq_name;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);


    /* Make sure channel is properly set up */
    if (chan->_state != CW_STATE_UP) {
        res = cw_answer(chan);
    }
    
    oldrformat = chan->readformat;
    oldwformat = chan->writeformat;
    
    if(!(res=cw_set_read_format(chan,  CW_FORMAT_SLINEAR))) {
        res = cw_set_write_format(chan,  CW_FORMAT_SLINEAR);
    }
    if(res) {
        cw_log(LOG_WARNING,"Unable to prepare channel %s\n",chan->name);
        if(oldrformat)
            cw_set_read_format(chan, oldrformat);
        if(oldwformat)
            cw_set_write_format(chan, oldwformat);
        vh_destroy(&arghash);
        LOCAL_USER_REMOVE(u);
        return -1;
    }

    /* Get queue for customer */
    queuename = vh_read(arghash, "queue");
    if (queuename != NULL) {
        queue = (icd_queue *) icd_fieldset__get_value(queues, queuename);
        if (queue == NULL) {
            cw_log(LOG_WARNING, "CUSTOMER FAILURE! Customer assigned to undefined Queue [%s]\n", queuename);
            if(oldrformat)
                cw_set_read_format(chan, oldrformat);
            if(oldwformat)
                cw_set_write_format(chan, oldwformat);
            vh_destroy(&arghash);
            LOCAL_USER_REMOVE(u);
            return -1;
        }

    }
    identifier = vh_read(arghash, "identifier");
    conference_name = vh_read(arghash, "conference");
    nohangup = vh_read(arghash, "nohangup");
    pin = vh_read(arghash, "pin");
    spymode = vh_read(arghash, "spy");
    agents_only = cw_true(vh_read(arghash, "agents_only"));

    if (queuename == NULL && !conference_name) {
        cw_log(LOG_WARNING,
                "CUSTOMER FAILURE! Could not find queue for incoming customer.\n"
                "Please provide a 'queue' argument in the extensions.conf file\n");
        if(oldrformat)
            cw_set_read_format(chan, oldrformat);
        if(oldwformat)
            cw_set_write_format(chan, oldwformat);
        vh_destroy(&arghash);
        LOCAL_USER_REMOVE(u);
        return -1;
    }

    /* All customers are of the dynamic type */
    config = create_icd_config(app_icd_config_registry, "customer_config");
    /* Install arghash into caller who destroys it in destroy_icd_caller.
       TBD - (Note that this breaks our malloc rules. It needs fixing) */
    icd_config__set_raw(config, "params", arghash);
    /* TBD channel registry
       icd_config__set_raw(config, "chan", chan);
    */

    customer = create_icd_customer(config);
    icd_caller__set_param_string((icd_caller *) customer, "identifier", identifier);

    if (queuename != NULL)
        sprintf(custname, "customer_%d_for_queue_%s", icd_caller__get_id((icd_caller *) customer), queuename);
    else if (conference_name)
        sprintf(custname, "caller_%d_for_conf_%s", icd_caller__get_id((icd_caller *) customer), conference_name);
    icd_caller__set_name((icd_caller *) customer, custname);
    icd_caller__set_dynamic((icd_caller *) customer, 1);
    snprintf(input, sizeof(input), "Memberships of Caller %s", custname);
    icd_member_list__set_name(icd_caller__get_memberships((icd_caller *) customer), input);
    snprintf(input, sizeof(input), "Associations of Caller %s", custname);
    icd_caller_list__set_name(icd_caller__get_associations((icd_caller *) customer), input);
    
    destroy_icd_config(&config);

    if (nohangup && cw_true(nohangup))
        icd_caller__add_flag((icd_caller *) customer, ICD_NOHANGUP_FLAG);

    icd_caller__assign_channel((icd_caller *) customer, chan);

    if (conference_name) {
        if (!strcasecmp(conference_name, "query")) {
            conference_name = NULL;
            if (!cw_app_getdata(chan, "conf-getconfno", input, sizeof(input) - 1, 0))
                conference_name = input;
        }
        if (!strcasecmp(conference_name, "autoscan")) {
            conf = NULL;
            for (;;) {
                conference_name = NULL;
                res = cw_app_getdata(chan, "conf-getconfno", input, sizeof(input) - 1, 0);
                if (!res)
                    conference_name = input;
                else if (res < 0)
                    break;
                if (!conference_name || !strcmp(conference_name, "")) {
                    break;
                }

                if (!strcmp(conference_name, "*")) {
                    for (looping = 0; looping > -1; looping = cw_waitfordigit(chan, 100)) {
                        for (keys = icd_conference__list(); keys; keys = keys->next) {
                            conf = icd_conference__locate(keys->name);
                            if (conf) {
                                conf_owner = icd_conference__get_owner(conf);
                                if (icd_conference__usecount(conf) && ((conf_owner && agents_only) || !agents_only)) {
                                    res = run_conference(chan, customer, conf, spymode);
                                }
                            }
                            looping = cw_waitfordigit(chan, 100);
                            if (looping < 0)
                                break;
                        }
                        sched_yield();
                    }
                } else {
                    conf = icd_conference__locate(conference_name);
                    if (conf) {
                        run_conference(chan, customer, conf, spymode);
                    } else
                        icd_bridge__play_sound_file(chan, "conf-invalid");
                }
            }
            if (res > -1)
                icd_bridge__play_sound_file(chan, "vm-goodbye");
            destroy_icd_customer(&customer);
            if(oldrformat)
                cw_set_read_format(chan, oldrformat);
            if(oldwformat)
                cw_set_write_format(chan, oldwformat);
            LOCAL_USER_REMOVE(u);
            return -1;

        } else {
            conf = icd_conference__locate(conference_name);
            if (conf) {
                conf_owner = icd_conference__get_owner(conf);
                if (conf_owner != NULL && icd_caller__get_state(conf_owner) != ICD_CALLER_STATE_CONFERENCED) {
                    cw_log(LOG_ERROR, "Agent Conference Not In Session......%s\n", conference_name);
                    destroy_icd_customer(&customer);
                    icd_bridge__play_sound_file(chan, "conf-invalid");
                    if(oldrformat)
                        cw_set_read_format(chan, oldrformat);
                    if(oldwformat)
                        cw_set_write_format(chan, oldwformat);
                    LOCAL_USER_REMOVE(u);
                    return -1;
                }

                key = icd_conference__key(conf);
                if (key) {
                    if (!pin) {
                        if (!cw_app_getdata(chan, "conf-getpin", input, sizeof(input) - 1, 0))
                            pin = input;
                    }

                    if (strcmp(key, pin)) {
                        icd_bridge__play_sound_file(chan, "conf-invalidpin");
                        if(oldrformat)
                            cw_set_read_format(chan, oldrformat);
                        if(oldwformat)
                            cw_set_write_format(chan, oldwformat);
                        LOCAL_USER_REMOVE(u);
                        return -1;
                    }

                }
                run_conference(chan, customer, conf, spymode);
                destroy_icd_customer(&customer);
                LOCAL_USER_REMOVE(u);
                if(oldrformat)
                    cw_set_read_format(chan, oldrformat);
                if(oldwformat)
                    cw_set_write_format(chan, oldwformat);
                return -1;

            } else {
                cw_log(LOG_ERROR, "No Such Conference......%s\n", conference_name);
                icd_bridge__play_sound_file(chan, "conf-invalid");
                if(oldrformat)
                    cw_set_read_format(chan, oldrformat);
                if(oldwformat)
                    cw_set_write_format(chan, oldwformat);
                destroy_icd_customer(&customer);
                LOCAL_USER_REMOVE(u);
                return -1;
            }
        }
    } else {
        if(icd_list__size((icd_list *)icd_queue__get_customers(queue)) <= icd_list__count((icd_list *)icd_queue__get_customers(queue))){
           cw_log(LOG_ERROR, "No room in a queue [%s]\n", icd_queue__get_name(queue));
	   manager_event(EVENT_FLAG_USER, "icd_event","CustomerCall: NoRoomInAQueue\r\n"
	   "ID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nQueue: %s\r\nCustomersInQueue: %d\r\nChannelUniqueID: %s\r\nChannelName: %s\r\n",
	   icd_caller__get_id((icd_caller *)customer),
	   icd_caller__get_caller_id((icd_caller *)customer), 
	   icd_caller__get_name((icd_caller *)customer),
	   icd_queue__get_name(queue),
	   icd_list__count((icd_list *)icd_queue__get_customers(queue)),
           chan ? chan->uniqueid : "nochan", 
           chan ? chan->name : "nochan");
	   return 1;
	}
	else{
          icd_caller__add_to_queue((icd_caller *) customer, queue);
          icd_caller__add_role((icd_caller *) customer, ICD_LOOPER_ROLE);
	}
    }
    /* This becomes the thread to manage customer state and incoming stream */
    input[0] = '\0';
    cust_uniq_name = input;
    if (chan) if(chan->uniqueid) {
        strncpy(input, chan->uniqueid, sizeof(input));
        cust_uniq_name = input;
    }
    if (!cust_uniq_name){
        cust_uniq_name = custname;
    }

    cw_mutex_lock(&customers_lock);
    icd_fieldset__set_value(customers, cust_uniq_name, customer);
    cw_mutex_unlock(&customers_lock);
	icd_caller__set_caller_id((icd_caller *)customer, cust_uniq_name);
	if(queue){
		manager_event(EVENT_FLAG_USER, "icd_event",
    	"Event: Approx. wait time for customer[%s] UniqueID[%s] in queue[%s] position [%d] is [%d] minutes", 
    	custname, cust_uniq_name?cust_uniq_name:"nochan", icd_queue__get_name(queue), icd_queue__get_customer_position(queue, customer), icd_queue__get_holdannounce_holdtime(queue));
	}    
    icd_caller__loop((icd_caller *) customer, 0);
    
    if (cust_uniq_name) {
        cw_mutex_lock(&customers_lock);    
        icd_fieldset__remove_key(customers, cust_uniq_name);
        cw_mutex_unlock(&customers_lock);    
    }
    destroy_icd_customer(&customer);    /* TC always destroy the customer callers */

    /* Once we hit here, the call is finished */
    if(oldrformat)
        cw_set_read_format(chan, oldrformat);
    if(oldwformat)
        cw_set_write_format(chan, oldwformat);
    LOCAL_USER_REMOVE(u);
    return 0;
}

/* This is intended to allow customers to be called back instead of waiting in a queue
 * Its also is intended to be used by external api that just want to schedule customer for callbacks
 *
 * */
int app_icd__customer_callback_login(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;
    icd_customer *customer;
    icd_queue *queue;
    icd_config *config;
    char buf[256];
    char buf2[256];
    char custname[256];
    char *queuename = NULL;
    char *customername = NULL;
    char *extension = NULL;
    char *prioritystring = NULL;
    char *context = NULL;
    char *customerinputfile = NULL;
    int res = 0;
    int tries = 0;
    int authenticated = 0;
    int priority =0;
    int pos = 0, oldrformat = 0, oldwformat = 0;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* Get queue for customer */
    queuename = vh_read(arghash, "queue");
    if (queuename != NULL) {
        queue = (icd_queue *) icd_fieldset__get_value(queues, queuename);
        if (queue == NULL) {
            cw_log(LOG_WARNING, "CUSTOMER FAILURE! Customer assigned to undefined Queue [%s]\n", queuename);
            vh_destroy(&arghash);
            LOCAL_USER_REMOVE(u);
            return -1;
        }

    }

    customer = NULL;
    extension = NULL;
    config = create_icd_config(app_icd_config_registry, "customer_config");
    /* Install arghash into caller who destroys it in destroy_icd_caller.
       TBD - (Note that this breaks our malloc rules. It needs fixing) */
    icd_config__set_raw(config, "params", arghash);
    /* TBD channel registry
       icd_config__set_raw(config, "chan", chan);
    */

    customer = create_icd_customer(config);
    icd_caller__add_flag((icd_caller *) customer, ICD_ORPHAN_FLAG);

    if (queuename != NULL)
        sprintf(custname, "customer_%d_for_queue_%s", icd_caller__get_id((icd_caller *) customer), queuename);
    icd_caller__set_name((icd_caller *) customer, custname);
    /* All customers are of the dynamic type */
    icd_caller__set_dynamic((icd_caller *) customer, 1);
    /* All CallBacks Customer are On Hook */
    icd_caller__set_onhook((icd_caller *) customer, 1);
    destroy_icd_config(&config);

    if (context == NULL) {
        context = chan->context;
    }

    cw_log(LOG_WARNING, "SET Context for EXTEN to current context %s\n", context);
    /* Three tries at logging in and authenticating */
    tries = 0;
    authenticated = 0;
    customerinputfile = "customer-user";
    tries = 0;

    priority = 1;
    if (prioritystring != NULL && strlen(prioritystring) > 0) {
        priority = atoi(prioritystring);
        if (priority < 0) {
            priority = 0;
        }
    }

    /* Now get the extension, if necessary, and ensure that it is valid. */
    pos = 0;
    buf[0] = '\0';
    while (1) {
        if (extension != NULL && strlen(extension) > 0) {
            strncpy(buf, extension, sizeof(buf) - 1);
            res = 0;
        } else {
            res = cw_app_getdata(chan, "agent-newlocation", buf + pos, sizeof(buf) - (1 + pos), 0);
            cw_log(LOG_DEBUG, "customer callback extension dialed in is [%s]\n", buf);
        }
        if (strlen(buf) == 0 || cw_exists_extension(chan, context
                                                     && strlen(context) ? context : "default", buf, priority, NULL)) {
            /* This is the exit point of the loop, if either the extension exists or the user entered nothing */
            break;
        }
        if (extension != NULL) {
            cw_log(LOG_WARNING, "Extension '%s' is not valid for automatic login of customer '%s'\n", extension,
                    icd_caller__get_name((icd_caller *) customer));
            extension = NULL;
            pos = 0;
            buf[0] = '\0';
        } else {
            res = icd_bridge__play_sound_file(chan, "invalid");
            if (!res) {
                res = cw_waitstream(chan,  CW_DIGIT_ANY);
            }
            if (res > 0) {
                buf[0] = res;
                buf[1] = '\0';
                pos = 1;
            } else {
                buf[0] = '\0';
                pos = 0;
            }
        }
    }

    /* res is 0 if channel is still up and working properly */
    if (res == 0) {
        if (strlen(buf) > 0) {
            if (context != NULL && strlen(context) > 0) {
                /* note the use of /n aka  masq optimization, since masq will hangup a channel */
                snprintf(buf2, sizeof(buf2), "Local/%s@%s/n", buf, context);
                //snprintf(buf2, sizeof(buf2), "Local/%s@%s", buf, context);
            }
            icd_caller__set_param((icd_caller *) customer, "channel", buf2);
            icd_caller__set_channel_string((icd_caller *) customer, buf2);
            res = icd_bridge__play_sound_file(chan, "agent-loginok");
        }

        if (res == 0) {
            cw_waitstream(chan, "");
        }

        oldrformat = chan->readformat;
        oldwformat = chan->writeformat;
    
        if(!(res=cw_set_read_format(chan,  CW_FORMAT_SLINEAR))) {
            res = cw_set_write_format(chan,  CW_FORMAT_SLINEAR);
        }
        if(res) {
            cw_log(LOG_WARNING,"Unable to prepare channel %s\n",chan->name);
            if(oldrformat)
                cw_set_read_format(chan, oldrformat);
            if(oldwformat)
                cw_set_write_format(chan, oldwformat);
            vh_destroy(&arghash);
            LOCAL_USER_REMOVE(u);
            return -1;
        }

    }
    if (res == 0) {
        /* Just say goodbye and be done with it */
        res = cw_safe_sleep(chan, 500);
        if (res == 0) {
            res = icd_bridge__play_sound_file(chan, "vm-goodbye");
        }
        if (res == 0) {
            res = cw_waitstream(chan, "");
        }
        if (res == 0) {
            res = cw_safe_sleep(chan, 1000);
        }

        /* Tell caller to start thread */
        cw_log(LOG_NOTICE, "Exec CustomerCallBack: Customer %s starting independent caller thread\n",
                customername);

        icd_caller__add_to_queue((icd_caller *) customer, queue);
        icd_caller__add_role((icd_caller *) customer, ICD_LOOPER_ROLE);
        icd_caller__loop((icd_caller *) customer, 1);
        /* dynamic & OffHook destroy on exit, TC dynamic and OnHook icd_customer__standard_cleanup_caller */
        if (icd_caller__get_dynamic((icd_caller *) customer) && (!icd_caller__get_onhook((icd_caller *) customer)))
            destroy_icd_customer(&customer);

        cw_log(LOG_NOTICE, "Exec CustomerCallBack: PBX thread for customer %s ending\n", customername);

    }
    /* res */
    if(oldrformat)
        cw_set_read_format(chan, oldrformat);
    if(oldwformat)
        cw_set_write_format(chan, oldwformat);
    LOCAL_USER_REMOVE(u);

    return -1;
}

/* this is where the agent logs in */
int app_icd__agent_exec(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;
    icd_agent *agent = NULL;
    icd_queue *queue;
    icd_config *config;
    char *agentname;
    char *dynamicstring;
    char *loginstring;
    char *queuename;
    char *queuelist;
    char *queuesleft;
    char *currqueue;
    char *hookinfo;
    char *noauth;
    char *identifier;
    char *passwd;
    char agentbuf[256];
    int res = 0;
    int dynamic = 0, oldrformat = 0, oldwformat = 0;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* Make sure channel is properly set up */
    if (chan->_state != CW_STATE_UP) {
        res = cw_answer(chan);
    }
    oldrformat = chan->readformat;
    oldwformat = chan->writeformat;
    
    if(!(res=cw_set_read_format(chan,  CW_FORMAT_SLINEAR))) {
        res = cw_set_write_format(chan,  CW_FORMAT_SLINEAR);
    }
    if(res) {
        cw_log(LOG_WARNING,"Unable to prepare channel %s\n",chan->name);
        if(oldrformat)
            cw_set_read_format(chan, oldrformat);
        if(oldwformat)
            cw_set_write_format(chan, oldwformat);
        vh_destroy(&arghash);
        LOCAL_USER_REMOVE(u);
        return -1;
    }


    /* We need to find the appropriate agent:
     *   1. find match for "agent" parameter from extensions.conf in agents registry
     *   2. if "dynamic" is true, generate an agent as for customers if "agent" doesn't exist already
     *           (in this case, "queue" in extensions.conf will be a good idea unless agent adds self to queues)
     *   3. if "identify" is true and channel is up, get DTMF and search for agent (authentication comes later)
     *   4. otherwise error
     * TBD - Do we need to protect against two users trying to use the same agent structure? YES!
     */
    identifier = vh_read(arghash, "identifier");
    dynamicstring = vh_read(arghash, "dynamic");
    agentname = vh_read(arghash, "agent");
    queuename = vh_read(arghash, "queues");
    passwd = vh_read(arghash, "password");
    if (queuename == NULL) {
        queuename = vh_read(arghash, "queue");
    }

    noauth = vh_read(arghash, "noauth");
    /* 1. find match in registry */
    if (agent == NULL) {
        if (agentname != NULL) {
            if (noauth) {
                agent = (icd_agent *) icd_fieldset__get_value(agents, agentname);
                if (agent)
                    res = icd_bridge__play_sound_file(chan, "agent-loginok");
            } else
                agent = app_icd__dtmf_login(chan, agentname, passwd, 3);
        }
        if (agent != NULL) {
            cw_log(LOG_NOTICE, "Agent [%s] found in registry and marked in use.\n",
                    icd_caller__get_name((icd_caller *) agent));
            if (dynamicstring != NULL && cw_true(dynamicstring)) {
                cw_log(LOG_NOTICE, "The 'dynamic' setting will be ignored.\n");
            }
        }
    }
    /* 2. If "dynamic" is true. Should we print a warning if agentname is in registry here? */
    if (agent == NULL && dynamicstring != NULL && cw_true(dynamicstring)) {
        dynamic = 1;
        config = create_icd_config(app_icd_config_registry, "dynamic_agent_config");
        /* Install arghash into caller who destroys it in destroy_icd_caller.
           TBD - (Note that this breaks our malloc rules. It needs fixing) */
        if (agentname != NULL) {
            icd_config__set_raw(config, "name", agentname);
        }
        icd_config__set_raw(config, "params", arghash);
        /* TBD channel registry
           icd_config__set_raw(config, "chan", chan);
        */
        agent = create_icd_agent(config);
        if (agentname == NULL && agent != NULL) {
            if (queuename == NULL) {
                sprintf(agentbuf, "agent_%d_for_dynamic_queues", icd_caller__get_id((icd_caller *) agent));
            } else {
                sprintf(agentbuf, "agent_%d_for_queue_%s", icd_caller__get_id((icd_caller *) agent), queuename);
            }
            agentname = agentbuf;
            cw_log(LOG_WARNING, "\n\nagent_name1: [%s]\n\n", agentname);
            icd_caller__set_name((icd_caller *) agent, agentname);
        }
        icd_caller__set_dynamic((icd_caller *) agent, 1);
        destroy_icd_config(&config);
        if (agent != NULL) {
            icd_fieldset__set_value(agents, agentname, agent);
            icd_agent__add_listener(agent, agents, clear_agent_from_registry, agentname);
            cw_log(LOG_NOTICE, "Agent [%s] dynamically generated and added to registry.\n", agentname);
        }
    }
    /* 3. Allow login with DTMF if agent not specifically identified. */

    if (agent == NULL && agentname == NULL) {
        loginstring = vh_read(arghash, "identify");
        /* icd_agent() == icd_agent(identify=1) */
#if 0
/* TODO: fudged to compile */
        if ((loginstring != NULL && cw_true(loginstring)) || (data == NULL || !strlen(data))) {
            agent = app_icd__dtmf_login(chan, NULL, NULL, 3);
        }
#endif
        if (agent != NULL) {
            agentname = icd_caller__get_param((icd_caller *) agent, "name");
            cw_log(LOG_WARNING, "\n\nagent_name: [%s]\n\n", agentname);
            cw_log(LOG_NOTICE, "User identified self as Agent [%s].\n", agentname);
            if(icd_caller__get_state((icd_caller *) agent) != ICD_CALLER_STATE_READY)
                icd_caller__set_state((icd_caller *) agent, ICD_CALLER_STATE_CALL_END);
        }
    }
    /* 4. Otherwise, error */
    if (agent == NULL) {
        if (agentname == NULL) {
            cw_log(LOG_WARNING,
                    "AGENT FAILURE!  No 'agent' argument specified.\n" "Please correct the extensions.conf file\n");
        } else {
            cw_log(LOG_WARNING,
                    "AGENT FAILURE!  Agent '%s' could not be found.\n"
                    "Please correct the 'agent' argument in the extensions.conf file\n", agentname);
            res = icd_bridge__play_sound_file(chan, "pbx-invalid");

        }
        if (arghash)
            vh_destroy(&arghash);
        if(oldrformat)
            cw_set_read_format(chan, oldrformat);
        if(oldwformat)
            cw_set_write_format(chan, oldwformat);
        LOCAL_USER_REMOVE(u);
        return -1;
    }

    if (agentname == NULL) {
        agentname = icd_caller__get_name((icd_caller *) agent);
        cw_log(LOG_WARNING, "\n\nagent_name: [%s]\n\n", agentname);
    }
    if (icd_caller__get_state((icd_caller *) agent) == ICD_CALLER_STATE_SUSPEND) {
        icd_caller__set_state((icd_caller *) agent, ICD_CALLER_STATE_READY);
    } else {

        /* At this point, we have an agent. So add into any given queue(s). */
        if (queuename != NULL) {
            queuelist = strdup(queuename);
            if (queuelist != NULL) {
                queuesleft = queuelist;
                while (queuesleft != NULL) {
                    currqueue = strsep(&queuesleft, ",;| ");
                    cw_log(LOG_WARNING, "Exec Agent: Agent %s processsing Queue %s!\n", agentname, currqueue);
                    if (currqueue != NULL && strlen(currqueue) > 0) {
                        queue = icd_fieldset__get_value(queues, currqueue);
                        if (queue != NULL) {
                            icd_caller__add_to_queue((icd_caller *) agent, queue);
                        } else {
                            cw_log(LOG_WARNING, "Exec Agent: Agent %s requires nonexistent Queue %s!\n", agentname,
                                    currqueue);
                        }
                    }
                }
                ICD_STD_FREE(queuelist);
            }
        }
    }
    /* Things are different if the agent is on-hook or off-hook. Extensions can override agents.conf */
    hookinfo = vh_read(arghash, "onhook");
    if (hookinfo != NULL) {
        if (cw_true(hookinfo)) {
            icd_caller__set_onhook((icd_caller *) agent, 1);
        } else {
            icd_caller__set_onhook((icd_caller *) agent, 0);
        }
    }

    /* identifier - get identifier from icd_agents.conf, if don't set from extensions.conf */
    if (identifier == NULL) {
        identifier = icd_caller__get_param((icd_caller *) agent, "identifier");
    }
    icd_caller__set_param_string((icd_caller *) agent, "identifier", identifier);
    ((icd_caller *)agent)->thread_state = ICD_THREAD_STATE_UNINITIALIZED;
	manager_event(EVENT_FLAG_USER, "icd_event","AgentLogin: OK\r\n"
	"ID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nChannelUniqueID: %s\r\nChannelName: %s\r\n",
	icd_caller__get_id((icd_caller *)agent),
	icd_caller__get_caller_id((icd_caller *)agent), 
	icd_caller__get_name((icd_caller *)agent),
    chan ? chan->uniqueid : "nochan", 
    chan ? chan->name : "nochan");
    if (icd_caller__get_onhook((icd_caller *) agent)) {
        /* On hook - Tell caller to start thread */
        cw_log(LOG_NOTICE, "Exec Agent: Agent %s starting independent caller thread\n", agentname);
        cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
        cw_hangup(chan);
        icd_caller__set_channel((icd_caller *)agent, NULL);        
        icd_caller__loop((icd_caller *) agent, 1);
//        res = icd_bridge__play_sound_file(chan, "agent-loginok");
    } else {
        /* Off hook - Use the PBX thread */
        cw_log(LOG_NOTICE, "Exec Agent: Agent %s using PBX thread to manage caller state\n", agentname);
        icd_caller__assign_channel((icd_caller *) agent, chan);
        icd_caller__add_role((icd_caller *) agent, ICD_LOOPER_ROLE);

        /* This becomes the thread to manage agent state and incoming stream */
        icd_caller__loop((icd_caller *) agent, 0);
        /* Once we hit here, the call is finished */
		manager_event(EVENT_FLAG_USER, "icd_event","AgentLogin: END\r\n"
		"ID: %d\r\nCallerID: %s\r\nCallerName: %s\r\n",
		icd_caller__get_id((icd_caller *)agent),
		icd_caller__get_caller_id((icd_caller *)agent), 
		icd_caller__get_name((icd_caller *)agent));
    }
    if (icd_caller__get_dynamic((icd_caller *) agent)) {
        destroy_icd_agent(&agent);
    }
    cw_log(LOG_NOTICE, "Exec Agent: PBX thread for Agent %s ending\n", agentname);

    LOCAL_USER_REMOVE(u);
    if (arghash)
        vh_destroy(&arghash);
/* There is probably no more channel   */
/*    if(oldrformat)
        cw_set_read_format(chan, oldrformat);
    if(oldwformat)
        cw_set_write_format(chan, oldwformat);
*/	
    return 0;
}

/* this is where the agent becomes a member of a queue */
int app_icd__add_member_exec(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;
    icd_queue *queue = NULL;
    char *queuename;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* The queue name is required. */
    /* TBD - Allow parsing of comma separated queues */
    queuename = vh_read(arghash, "queue");
    if (queuename != NULL) {
        queue = icd_fieldset__get_value(queues, queuename);
    }
    if (queue == NULL) {
        if (queuename == NULL) {
            cw_log(LOG_WARNING,
                    "ADD MEMBER FAILURE!  No 'queue' argument specified.\n"
                    "Please correct the extensions.conf file\n");
        } else {
            cw_log(LOG_WARNING,
                    "ADD MEMBER FAILURE!  Queue '%s' could not be found.\n"
                    "Please correct the 'queue' argument in the extensions.conf file\n", queuename);
        }
        vh_destroy(&arghash);
        LOCAL_USER_REMOVE(u);
        return -1;
    }

    /* TBD - Identify agent just like in app_icd__agent_exec and mostly like
       app_icd__logout_exec. Clearly this needs to be factored out. */
    /* TBD - Add agent to queue. */

    LOCAL_USER_REMOVE(u);

    return -1;
}

/* this is where the agent drops membership in a queue */
int app_icd__remove_member_exec(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* TBD - Implement this. It is identical to app_icd__add_member_exec
       except that it removes the membership rather than adding it. */

    LOCAL_USER_REMOVE(u);

    return -1;
}

/* This is intended to duplicate the AgentCallbackLogin function from chan_agent */
int app_icd__agent_callback_login(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;
    icd_agent *agent;
    icd_queue *queue;
    char buf[256];
    char buf2[256];
    char *queuename;
    char *agentname;
    char *password;
    char *extension;
    char *prioritystring;
    char *context;
    char *dynamic;
    char *noauth;
    char *channel;
    char *acknowledge_callstring;
    char *agentinputfile;
    char *queuelist;
    char *queuesleft;
    char *currqueue;
    int res = 0;
    int tries = 0;
    int authenticated = 0;
    int priority = 0;
    int dialed_agent =0;
    int pos =0, oldrformat = 0, oldwformat = 0;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* Deal with incomplete state change in channel received from callweaver */
    if (chan->_state != CW_STATE_UP) {
        res = cw_answer(chan);
    }
    queuename = vh_read(arghash, "queue");
    agentname = vh_read(arghash, "agent");
    password = vh_read(arghash, "password");
    extension = vh_read(arghash, "extension");
    prioritystring = vh_read(arghash, "priority");
    acknowledge_callstring = vh_read(arghash, "acknowledgecall");
    context = vh_read(arghash, "context");
    dynamic = vh_read(arghash, "dynamic");
    noauth = vh_read(arghash, "noauth");
    channel = vh_read(arghash, "channel");

    if (agentname != NULL) {
        agent = (icd_agent *) icd_fieldset__get_value(agents, agentname);
        if (agent != NULL) {
            icd_caller__add_flag((icd_caller *) agent, ICD_ORPHAN_FLAG);
            if (icd_caller__get_state((icd_caller *) agent) == ICD_CALLER_STATE_SUSPEND) {
                cw_log(LOG_NOTICE, "Agent [%s] (found in registry) will be logged in.\n", agentname);
                icd_caller__set_state((icd_caller *) agent, ICD_CALLER_STATE_READY);
                res = icd_bridge__play_sound_file(chan, "agent-loginok");
                return 0;
            }                   /* agent was suspended should ask emd what do extension they want to use */
        }
    }

    if (!(context != NULL && strlen(context) > 0)) {
        context = chan->context;
        /* TC this should work i want the specfic include context * finds a match in for this app
           context = pbx_builtin_getvar_helper(chan, "CONTEXT");
           chan->proc_context; chan->context;
        */
    }

    cw_log(LOG_WARNING, "SET Context for EXTEN to current context %s\n", context);
    /* Three tries at logging in and authenticating */
    tries = 0;
    authenticated = 0;
    agent = NULL;
    agentinputfile = "agent-user";
    tries = 0;

    while ((agent == NULL || authenticated == 0) && tries < 3) {
        /* Get user id, prompting when not provided */
        if (agent == NULL && agentname != NULL) {
            agent = (icd_agent *) icd_fieldset__get_value(agents, agentname);
        }
        if (agent == NULL && dynamic != NULL && cw_true(dynamic)) {
            /* TBD - Create agent dynamically (and onhook) at this point */
        }
        dialed_agent = 0;
        if (agent == NULL) {
            buf[0] = '\0';
            res = cw_app_getdata(chan, agentinputfile, buf, sizeof(buf) - 1, 0);
            cw_log(LOG_DEBUG, "Agent callback user id dialed in is [%s], res=%d, len=%d\n", buf, res, strlen(buf));
            if (strlen(buf) > 0) {
                agent = (icd_agent *) icd_fieldset__get_value(agents, buf);
                if (agent != NULL) {
                    icd_caller__add_flag((icd_caller *) agent, ICD_ORPHAN_FLAG);
                    cw_log(LOG_DEBUG, "Agent callback user id dialed has been found\n");
                    dialed_agent = 1;
                } else {
                    cw_log(LOG_DEBUG, "Agent callback user id dialed is incorrect, no such user\n");
                }
            }
        }
        if (agent != NULL && password == NULL) {
            password = icd_caller__get_param((icd_caller *) agent, "password");
            if (password == NULL) {
                /* TBD - Get password from "secret" setting in iax.conf, sip.conf, etc */
            }
        }
        if (agent != NULL && noauth == NULL) {
            noauth = icd_caller__get_param((icd_caller *) agent, "noath");
        }
        /* Whether you got the agent or not, prompt for password. */
        if ((noauth == NULL || !cw_true(noauth)) && password != NULL && strlen(password) > 0) {
            buf[0] = '\0';
            res = cw_app_getdata(chan, "agent-pass", buf, sizeof(buf) - 1, 0);
            cw_log(LOG_DEBUG, "Agent callback password dialed in is [%s], res=%d\n", buf, res);
            if (strlen(buf) > 0 && strcmp(buf, password) == 0) {
                cw_log(LOG_DEBUG, "Agent callback password authenticated\n");
                authenticated = 1;
            } else {
                cw_log(LOG_DEBUG, "Agent callback password FAILED authenticated\n");
                authenticated = 0;
                /* Reset the agent if we dialed it but got the password wrong */
                if (dialed_agent > 0) {
                    agent = NULL;
                    dialed_agent = 0;
                }
            }
        } else {
            /* Skip authentication */
            authenticated = 1;
        }
        if (agent == NULL || authenticated == 0) {
            agentinputfile = "agent-incorrect";
        }
        tries++;
    }                           /* agent oassword auth */

    if (agent == NULL || authenticated == 0) {
        res = icd_bridge__play_sound_file(chan, "vm-goodbye");

    }
    if (agent == NULL) {
        cw_log(LOG_WARNING, "AGENT CALLBACK LOGIN FAILURE! Agent could not be identified\n");
        LOCAL_USER_REMOVE(u);
        return -1;
    }
    if (authenticated == 0) {
        cw_log(LOG_WARNING, "AGENT CALLBACK LOGIN FAILURE! Agent could not enter correct password\n");
        LOCAL_USER_REMOVE(u);
        return -1;
    }

    /* Pull values from the agent configuration */
    if (queuename == NULL) {
        queuename = icd_caller__get_param((icd_caller *) agent, "queues");      /*%TC we use queues= in config ?? */
    }
    if (channel == NULL) {
        channel = icd_caller__get_param((icd_caller *) agent, "channel");
    }
    if (channel != NULL && extension == NULL && strncasecmp(channel, "local/", 6)) {
        extension = &(channel[6]);
    }
    if (prioritystring == NULL) {
        prioritystring = icd_caller__get_param((icd_caller *) agent, "priority");
    }
    if (acknowledge_callstring == NULL) {
        acknowledge_callstring = icd_caller__get_param((icd_caller *) agent, "acknowledgecall");
    }
    if (context == NULL) {
        context = icd_caller__get_param((icd_caller *) agent, "context");
    }

    priority = 1;
    if (prioritystring != NULL && strlen(prioritystring) > 0) {
        priority = atoi(prioritystring);
        if (priority < 0) {
            priority = 0;
        }
    }

    /* Now get the extension, if necessary, and ensure that it is valid. */
    pos = 0;
    buf[0] = '\0';
    while (1) {
        if (extension != NULL && strlen(extension) > 0) {
            strncpy(buf, extension, sizeof(buf) - 1);
            res = 0;
        } else {
            res = cw_app_getdata(chan, "agent-newlocation", buf + pos, sizeof(buf) - (1 + pos), 0);
            cw_log(LOG_DEBUG, "Agent callback extension dialed in is [%s]\n", buf);
        }
        if (strlen(buf) == 0 || cw_exists_extension(chan, context
                                                     && strlen(context) ? context : "default", buf, priority, NULL)) {
            /* This is the exit point of the loop, if either the extension exists or the user entered nothing */
            break;
        }
        if (extension != NULL) {
            cw_log(LOG_WARNING, "Extension '%s' is not valid for automatic login of agent '%s'\n", extension,
                    icd_caller__get_name((icd_caller *) agent));
            extension = NULL;
            pos = 0;
            buf[0] = '\0';
        } else {
            res = icd_bridge__play_sound_file(chan, "invalid");
            if (!res) {
                res = cw_waitstream(chan,  CW_DIGIT_ANY);
            }
            if (res > 0) {
                buf[0] = res;
                buf[1] = '\0';
                pos = 1;
            } else {
                buf[0] = '\0';
                pos = 0;
            }
        }
    }                           /*agent extension validation */

    /* res is 0 if channel is still up and working properly */
    if (res == 0) {
        if (strlen(buf) > 0) {
            if (context != NULL && strlen(context) > 0) {
                /* note the use of /n aka  masq optimization, since a masquade will hangup a channel */
                snprintf(buf2, sizeof(buf2), "Local/%s@%s/n", buf, context);
                //snprintf(buf2, sizeof(buf2), "Local/%s@%s", buf, context);
            }
            icd_caller__set_param((icd_caller *) agent, "channel", buf2);
            icd_caller__set_channel_string((icd_caller *) agent, buf2);
            res = icd_bridge__play_sound_file(chan, "agent-loginok");
        } else {
            res = icd_bridge__play_sound_file(chan, "agent-loggedoff");
            /* dynamic & OffHook destroy on exit, dynamic & OnHook destroy on logout, static destroy on module unload */
            if (icd_caller__get_dynamic((icd_caller *) agent) && icd_caller__get_onhook((icd_caller *) agent))
                destroy_icd_agent(&agent);
            else
                icd_caller__set_state((icd_caller *) agent, ICD_CALLER_STATE_SUSPEND);

        }

        if (res == 0) {
            cw_waitstream(chan, "");
        }

        oldrformat = chan->readformat;
        oldwformat = chan->writeformat;
    
        if(!(res=cw_set_read_format(chan,  CW_FORMAT_SLINEAR))) {
            res = cw_set_write_format(chan,  CW_FORMAT_SLINEAR);
        }
        if(res) {
            cw_log(LOG_WARNING,"Unable to prepare channel %s\n",chan->name);
            if(oldrformat)
                cw_set_read_format(chan, oldrformat);
            if(oldwformat)
                cw_set_write_format(chan, oldwformat);
            vh_destroy(&arghash);
            LOCAL_USER_REMOVE(u);
            return -1;
        }


        /* BCA - What does this do? Do we need to worry about it?,
           this allows the agent to acknowledge the call b4 being bridged,
           prolly should change icd_request_and_dial to run icd_wait_call
           if the config flag required the agent to ack b4 a bridge
           gives agent a few secs to prepare for a bridge, should prolly working
           in conjunction with timeout flag so if they dont ack the call with in a timeout
           period we return from icd_call_wait with state chan failed
        */
        //        p->acknowledged = 0;
        //        /* store/clear the global variable that stores agentid based on the callerid */
        //        if (chan->callerid) {
        //            char agentvar[ CW_MAX_BUF];
        //            snprintf(agentvar, sizeof(agentvar), "%s_%s",GETAGENTBYCALLERID, chan->callerid);
        //            if (!strlen(p->loginchan))
        //                pbx_builtin_setvar_helper(NULL, agentvar, NULL);
        //            else
        //                pbx_builtin_setvar_helper(NULL, agentvar, p->agent);
        //        }
        //        if(updatecdr && chan->cdr)
        //            snprintf(chan->cdr->channel, sizeof(chan->cdr->channel), "Agent/%s", p->agent);

    }

    if (res == 0) {
        /* Just say goodbye and be done with it */
        res = cw_safe_sleep(chan, 500);
        if (res == 0) {
            res = icd_bridge__play_sound_file(chan, "vm-goodbye");
        }
        if (res == 0) {
            res = cw_waitstream(chan, "");
        }
        if (res == 0) {
            res = cw_safe_sleep(chan, 1000);
        }

        /* Now build the queues and all the rest like a regular agent would */
        /* At this point, we have an agent. So add into any given queue(s). */
        if (queuename != NULL) {
            queuelist = strdup(queuename);
            if (queuelist != NULL) {
                queuesleft = queuelist;
                while (queuesleft != NULL) {
                    currqueue = strsep(&queuesleft, ",;| ");
                    if (currqueue != NULL && strlen(currqueue) > 0) {
                        queue = icd_fieldset__get_value(queues, currqueue);
                        if (queue != NULL) {
                            icd_caller__add_to_queue((icd_caller *) agent, queue);
                        } else {
                            cw_log(LOG_WARNING, "Exec Agent: Agent %s requires nonexistent Queue %s!\n", agentname,
                                    currqueue);
                        }
                    }
                }
                ICD_STD_FREE(queuelist);
            }
        }

        icd_caller__set_onhook((icd_caller *) agent, 1);
        /* Tell caller to start thread */
        if (icd_debug)
            cw_log(LOG_DEBUG, "Exec Agent: AgentCallBack %s starting independent caller thread\n", agentname);

        icd_caller__loop((icd_caller *) agent, 1);
        /* dynamic & OffHook destroy on exit, dynamic & OnHook destroy on logout, static destroy on module unload */
        if (icd_caller__get_dynamic((icd_caller *) agent) && (!icd_caller__get_onhook((icd_caller *) agent)))
            destroy_icd_agent(&agent);

        if (icd_debug)
            cw_log(LOG_DEBUG, "Exec AgentCallBack: PBX thread for Agent %s ending\n", agentname);

    }

    /* res */
    LOCAL_USER_REMOVE(u);
    if(oldrformat)
        cw_set_read_format(chan, oldrformat);
    if(oldwformat)
        cw_set_write_format(chan, oldwformat);
    return -1;
}

/* this is where the agent logs out */
int app_icd__logout_exec(struct cw_channel *chan, int argc, char **argv)
{
    struct localuser *u;
    icd_agent *agent = NULL;
    char *agentname;
    char *loginstring;

    void_hash_table *arghash = vh_init("args");

    for (; argc; argv++, argc--)
	    split_and_add(arghash, argv[0]);

    LOCAL_USER_ADD(u);

    /* Identify agent just like app_icd__agent_exec, only this time we skip
       dynamically creating an agent. */
    agentname = vh_read(arghash, "agent");
    if (agentname != NULL) {
        agent = (icd_agent *) icd_fieldset__get_value(agents, agentname);
        if (agent != NULL) {
            cw_log(LOG_NOTICE, "Agent [%s] (found in registry) will be logged out.\n", agentname);
        }
    }

    if (agent == NULL && agentname == NULL) {
        loginstring = vh_read(arghash, "identify");
        if (loginstring != NULL && cw_true(loginstring)) {
            agent = app_icd__dtmf_login(chan, NULL, NULL, 3);
        }
        if (agent != NULL) {
            agentname = icd_caller__get_param((icd_caller *) agent, "name");
            if (icd_debug)
                cw_log(LOG_DEBUG, "Agent [%s] (self-identified) will be logged out.\n", agentname);
        }
    }

    if (agent == NULL) {
        if (agentname == NULL) {
            cw_log(LOG_WARNING,
                    "LOGOUT FAILURE!  No 'agent' argument specified.\n" "Please correct the extensions.conf file\n");
        } else {
            cw_log(LOG_WARNING,
                    "LOGOUT FAILURE!  Agent '%s' could not be found.\n"
                    "Please correct the 'agent' argument in the extensions.conf file\n", agentname);
        }
        vh_destroy(&arghash);
        LOCAL_USER_REMOVE(u);
        return -1;
    }
    /* TBD - Implement state change to ICD_CALLER_STATE_WAIT. We can't just pause the thread
     * because the caller's members would still be in the distributors. We need to go into a
     * caller state that is actually different, a paused/waiting/down state.
     */
    if(icd_caller__set_state((icd_caller *) agent, ICD_CALLER_STATE_SUSPEND)!=ICD_SUCCESS){
       cw_log(LOG_WARNING, "LOGOUT FAILURE!  Agent [%s] vetoed or ivalid state change, state [%s].\n", agentname, icd_caller__get_state_string((icd_caller *) agent));
       return -1;   	
    }	

    LOCAL_USER_REMOVE(u);

    return -1;
}

icd_status app_icd__read_conference_config(char *conference_config_name)
{
    struct cw_config *astcfg;
    struct cw_variable *varlist;
    char *entry;
    icd_conference *conf;

    astcfg = cw_config_load(conference_config_name);
    if (!astcfg) {
        cw_log(LOG_WARNING, "Conference configuration file %s not found\n", conference_config_name);
        return ICD_ENOTFOUND;
    }

    for (entry = cw_category_browse(astcfg, NULL); entry != NULL; entry = cw_category_browse(astcfg, entry)) {
        if (!strcasecmp(entry, "general")) {
            for (varlist = cw_variable_browse(astcfg, entry); varlist; varlist = varlist->next) {
                if (!strcasecmp(varlist->name, "conference_bridge_global")) {
                    icd_conference__set_global_usage(cw_true(varlist->value));
                }
            }
            continue;
        }
        varlist = cw_variable_browse(astcfg, entry);
        conf = icd_conference__new(entry);
        for (varlist = cw_variable_browse(astcfg, entry); varlist; varlist = varlist->next) {
            if (!strcasecmp(varlist->name, "pin")) {
                icd_conference__lock(conf, varlist->value);
            }
        }

        if (conf) {
            icd_conference__register(entry, conf);
            cw_verbose(VERBOSE_PREFIX_3 "Creating conference object %s:\n", entry);

        }
    }
    cw_config_destroy(astcfg);

    return ICD_SUCCESS;
}

//! Reads a queue config file, creates queues, and configures them.
/*!
 * Reads each entry out of the icd config file and set system options
 * \param modules Module registry
 * \param events Event registry
 * \param icd_config_name the name of the configuration file for icd
 * \return ICD_SUCCESS if the file was processed, ICD_ENOTFOUND if the file is not available
 */
icd_status app_icd__read_icd_config(char *icd_config_name)
{
    struct cw_config *astcfg;
    struct cw_variable *varlist;
    char *entry;
    int mod = 0;
    int event = 0;
    icd_status result = ICD_SUCCESS;

    /*  
        icd_config *config;
        icd_config *general_config;
        void_hash_table *params;
        char *fieldval;
        icd_config_iterator *iter;
        char *curr_key;
    */

    assert(icd_config_name != NULL);

    astcfg = cw_config_load(icd_config_name);
    if (!astcfg) {
        cw_log(LOG_WARNING, "ICD configuration file %s not found -- ICD option support disabled\n",
                icd_config_name);
        return ICD_ENOTFOUND;
    }
    entry = cw_category_browse(astcfg, NULL);
    /* For each category (demarcated by "[]") in the file */

    while (entry) {
        cw_verbose(VERBOSE_PREFIX_3 "Reading Secton [%s]\n", entry);
        varlist = cw_variable_browse(astcfg, entry);

        /* For each key/value pair in the [category] */
        while (varlist) {
            if (icd_debug)
                cw_log(LOG_DEBUG, "%s=%s\n", varlist->name, varlist->value);
            if (strcasecmp(entry, "general") == 0) {
                if (strcasecmp(varlist->name, "icd_delimiter") == 0) {
                    icd_delimiter = varlist->value[0];
                    if (icd_debug)
                        cw_log(LOG_DEBUG, "Set %s=%s\n", varlist->name, &icd_delimiter);
                }
                if (strcasecmp(varlist->name, "icd_debug") == 0) {
                    icd_debug = cw_true(varlist->value);
                    if (icd_debug)
                        cw_log(LOG_DEBUG, "Set %s=%d\n", varlist->name, icd_debug);
                }
                if (strcasecmp(varlist->name, "icd_verbose") == 0) {
                    icd_verbose = atoi(varlist->value);
                    if (icd_debug)
                        cw_log(LOG_DEBUG, "Set %s=%d\n", varlist->name, icd_verbose);
                } else
                    icd_verbose = option_verbose;       /*default to CW option_verbose */

            }                   /*general section */
            if (strcasecmp(entry, "events") == 0) {
                if (strcasecmp(varlist->name, "module_mask") == 0) {
                    /* iterrate over our modules & see if the module string is in the module_mask */
                    for (mod = APP_ICD; mod < ICD_MAX_MODULES; ++mod) {
                    	if (icd_module_strings[mod] == NULL)
                    	    break;
                        if (icd_instr(varlist->value, icd_module_strings[mod], icd_delimiter))
                            module_mask[mod] = 1;
                        else
                            module_mask[mod] = 0;
                        if (icd_debug)
                            cw_log(LOG_DEBUG, "MOD %s[%d]\n", icd_module_strings[mod], module_mask[mod]);
                    };          /*mod iterator */
                }               /*module_mask */
                if (strcasecmp(varlist->name, "event_mask") == 0) {
                    /* iterrate over our events & see if the event string is in the event_mask */
                    for (event = ICD_EVENT_TEST; event < ICD_MAX_EVENTS; ++event) {
                        if (icd_event_strings[event] == NULL )
                            break;
                        if(icd_instr(varlist->value, icd_event_strings[event], icd_delimiter))
                            event_mask[event] = 1;
                        else
                            event_mask[event] = 0;

                        if (icd_debug)
                            cw_log(LOG_DEBUG, "EVT %s[%d]\n", icd_event_strings[event], event_mask[event]);
                    };          /*event iterator */

                }               /*event_mask */
            }                   /*events section */
            varlist = varlist->next;
        }
        /* Move on to the next section */
        entry = cw_category_browse(astcfg, entry);

    }

    cw_config_destroy(astcfg);
    return result;
}

//! Reads a queue config file, creates queues, and configures them.
/*!
 * Reads each entry out of the queue config file and creates a queue object
 * for each one. It also stores queue memberships that need to be created in
 * the outstanding_members map.
 * \param queues Queue registry
 * \param queue_config_name the name of the configuration file for queues
 * \param outstanding_members Empty map to be populated with queue/agent memberships
 * \return ICD_SUCCESS if the file was processed, ICD_ENOTFOUND if the file is not available
 * \note this function appears to break the memory allocation rules because it
 *         allocates strings to store in the outstanding_members map (indirectly,
 *         through the call to store_agent_list()), but in fact the two functions
 *         icd_read_queue_config() and icd_read_agent_config() are paired functions,
 *         the former allocating the memory and the latter freeing it. Thus, so long
 *         as the two functions are always used together, our allocation rules are
 *         satisfied.
 * \note the other malloc issue is that this leaves a "params" hanging around. Since
 *         this is used directly by the ICD objects (a no-no) we have to live with it
 *         for now. Eventually they will create their own params (of type icd_fieldset)
 *         and copy any values they need in from the icd_config. Then "params" can be
 *         eliminated and we have our clean malloc rules again.
 * \sa app_icd__read_agents_config
 */
icd_status app_icd__read_queue_config(icd_fieldset * queues, char *queue_config_name,
                                      icd_fieldset * outstanding_members)
{
    struct cw_config *astcfg;
    struct cw_variable *varlist;
    char *entry;
    icd_queue *queue;
    icd_config *config;
    icd_config *general_config;
    void_hash_table *params;
    char *fieldval;
    icd_status result;

    icd_config_iterator *iter;
    char *curr_key;

    assert(queues != NULL);
    assert(queue_config_name != NULL);
    assert(outstanding_members != NULL);

    astcfg = cw_config_load(queue_config_name);
    if (!astcfg) {
        cw_log(LOG_WARNING, "Queue configuration file %s not found -- ICD support disabled\n", queue_config_name);
        return ICD_ENOTFOUND;
    }

    cw_verbose(VERBOSE_PREFIX_3 "Creating General Queue Configurations\n");
    general_config = create_icd_config(app_icd_config_registry, "queue.general");
    icd_config__set_raw(general_config, "name", "queue.general");
    /* For each key/value pair in the [general] */
    varlist = cw_variable_browse(astcfg, "general");
    while (varlist) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "%s=%s\n", varlist->name, varlist->value);
        icd_config__set_value(general_config, varlist->name, varlist->value);
        varlist = varlist->next;
    }

    entry = cw_category_browse(astcfg, NULL);
    /* For each category (demarcated by "[]") in the file, create a queue */

    while (entry) {

        if (strcasecmp(entry, "general") != 0) {
            varlist = cw_variable_browse(astcfg, entry);
            config = create_icd_config(app_icd_config_registry, entry);
            /* TBD Get rid of need for params here */
            params = vh_init("config");
            icd_config__set_raw(config, "name", entry);
            vh_cp_string(params, "name", entry);

            /* Use the General Config values as defaults */
            iter = icd_config__get_key_iterator(general_config);
            while (icd_config_iterator__has_more(iter)) {
                curr_key = icd_config_iterator__next(iter);
                if (strcasecmp(curr_key, "name")) {
                    icd_config__set_value(config, curr_key, icd_config__get_value(general_config, curr_key));
                    vh_cp_string(params, curr_key, icd_config__get_value(general_config, curr_key));
                }
            }
            destroy_icd_config_iterator(&iter);

            /* For each key/value pair in the [category] */
            while (varlist) {
                if (icd_debug)
                    cw_log(LOG_DEBUG, "%s=%s\n", varlist->name, varlist->value);
                icd_config__set_value(config, varlist->name, varlist->value);
                vh_cp_string(params, varlist->name, varlist->value);
                varlist = varlist->next;
            }

            /* Post-processing of key/value pairs and queue creation */
            /* Check for "disabled=true" to tell whether to ignore entry */
            fieldval = icd_config__get_value(config, "disabled");
            if (fieldval != NULL && cw_true(fieldval)) {
                destroy_icd_config(&config);
                vh_destroy(&params);
                params = NULL;
                cw_log(LOG_WARNING, "Create Queue [%s] has been manually disabled!\n", entry);
                entry = cw_category_browse(astcfg, entry);
                continue;
            }

            /* Check for "agents=" and fill outstanding_members with them */
            fieldval = icd_config__get_value(config, "agents");
            if (fieldval != NULL) {
                result = app_icd__store_agent_list(fieldval, entry, outstanding_members);
            }

            icd_config__set_raw(config, "params", params);
            if (icd_fieldset__get_value(queues, entry) == NULL) {
                /* All is ready, let's create the new queue */
                queue = create_icd_queue(entry, config);
                if (queue) {
                    icd_fieldset__set_value(queues, entry, queue);
                    icd_queue__add_listener(queue, queues, clear_queue_from_registry, entry);
                    icd_queue__start_distributing(queue);       /* start the queue's distributor thread */
                }
                cw_verbose(VERBOSE_PREFIX_2 "Create Queue [%s] %s\n", entry, queue ? "success" : "failure");
            } else {
                cw_verbose(VERBOSE_PREFIX_2 "Merge Queue [%s] %s\n", entry, queue ? "success" : "failure");
            }
            destroy_icd_config(&config);

        }
        /* not general categories */
        /* Move on to the next queue */
        entry = cw_category_browse(astcfg, entry);
    }                           /* while entry */
    destroy_icd_config(&general_config);

    cw_config_destroy(astcfg);
    return ICD_SUCCESS;
}

//! Reads an agent config file, creates agent objects, and configures them. */
/*!
 * Reads each entry out of the agent config file and creates an agent object
 * for each one. It also takes all the queue memberships in the outstanding_members
 * map as well as the queues identified by name in the agents config file, and
 * makes the agent a member of each queue. Finally, it frees any strings associated
 * with the outstanding_members map.
 * \param agents Agent registry
 * \param agent_config_name the name of the configuration file for agents
 * \param queues Queue registry
 * \param outstanding_members Map filled with queue/agent memberships
 * \return ICD_SUCCESS if the file was processed, ICD_ENOTFOUND if the file is not available
 * \sa app_icd__read_queue_config
 */
icd_status app_icd__read_agents_config(icd_fieldset * agents, char *agent_config_name, icd_fieldset * queues,
                                       icd_fieldset * outstanding_members)
{
    struct cw_config *astcfg;
    struct cw_variable *varlist;
    char *entry;
    icd_agent *agent;
    icd_config *config;
    icd_config *general_config;
    void_hash_table *params;
    char *fieldval;
    char *queuesleft;
    char *currqueue;
    icd_queue *queue;
    icd_status result;
    icd_config_iterator *iter;
    char *curr_key;
    int is_new_agent = 0;

    assert(agents != NULL);
    assert(agent_config_name != NULL);
    assert(queues != NULL);
    assert(outstanding_members != NULL);

    astcfg = cw_config_load(agent_config_name);
    if (!astcfg) {
        cw_log(LOG_WARNING, "Agent configuration file %s not found -- ICD support disabled\n", agent_config_name);
        return ICD_ENOTFOUND;
    }
    cw_verbose(VERBOSE_PREFIX_3 "Creating General Agent Configurations\n");
    general_config = create_icd_config(app_icd_config_registry, "agent.general");
    icd_config__set_raw(general_config, "name", "agent.general");
    /* For each key/value pair in the [general] */
    varlist = cw_variable_browse(astcfg, "general");
    while (varlist) {
        if (icd_debug)
            cw_log(LOG_DEBUG, "%s=%s\n", varlist->name, varlist->value);
        icd_config__set_value(general_config, varlist->name, varlist->value);
        varlist = varlist->next;
    }

    /* For each category (demarcated by "[]") in the file, create an agent */
    for (entry = cw_category_browse(astcfg, NULL); entry != NULL; entry = cw_category_browse(astcfg, entry)) {
        if (strcasecmp(entry, "general") != 0) {
            varlist = cw_variable_browse(astcfg, entry);
            config = create_icd_config(app_icd_config_registry, entry);
            /* TBD Get rid of need for params here */
            params = vh_init("config");
            icd_config__set_raw(config, "name", entry);
            vh_cp_string(params, "name", entry);

            /* Use the General Config values as defaults */
            iter = icd_config__get_key_iterator(general_config);
            while (icd_config_iterator__has_more(iter)) {
                curr_key = icd_config_iterator__next(iter);
                if (strcasecmp(curr_key, "name")) {
                    icd_config__set_value(config, curr_key, icd_config__get_value(general_config, curr_key));
                    vh_cp_string(params, curr_key, icd_config__get_value(general_config, curr_key));
                }
            }
            destroy_icd_config_iterator(&iter);

            /* For each key/value pair in the [category] */
            while (varlist) {
                if (icd_debug)
                    cw_log(LOG_DEBUG, "%s=%s\n", varlist->name, varlist->value);
                icd_config__set_value(config, varlist->name, varlist->value);
                vh_cp_string(params, varlist->name, varlist->value);
                varlist = varlist->next;
            }

            /* Post-processing of key/value pairs and queue creation */
            /* Check for "disabled=true" to tell whether to ignore entry */
            fieldval = icd_config__get_value(config, "disabled");
            if (fieldval != NULL && cw_true(fieldval)) {
                destroy_icd_config(&config);
                vh_destroy(&params);
                params = NULL;
                cw_log(LOG_WARNING, "Create Agent [%s] has been manually disabled!\n", entry);
                entry = cw_category_browse(astcfg, entry);
                continue;
            }

            /* Check for "queues=" and put in outstanding_members map */
            fieldval = icd_config__get_value(config, "queues");
            if (fieldval != NULL) {
                result = app_icd__store_queue_list(fieldval, entry, outstanding_members);
            }

            fieldval = icd_config__get_value(config, "agent_id");
            if (fieldval) {
                if (icd_fieldset__get_value(agents, fieldval) == NULL)
                    is_new_agent = 1;
            } else {
                if (icd_fieldset__get_value(agents, entry) == NULL)
                    is_new_agent = 1;
            }

            icd_config__set_raw(config, "params", params);
            if (is_new_agent == 1) {
                /* All is ready, let's create the new agent */
                agent = create_icd_agent(config);
                fieldval = icd_config__get_value(config, "bridge_tech");
                if (fieldval != NULL) {
                    if (!strcasecmp(fieldval, "conference")) {
                        icd_caller__set_bridge_technology((icd_caller *) agent, ICD_BRIDGE_CONFERENCE);
                    } else {
                        icd_caller__set_bridge_technology((icd_caller *) agent, ICD_BRIDGE_STANDARD);
                    }
                    if (icd_debug)
                        cw_log(LOG_DEBUG, "Caller id[%d] [%s] bridge_tech set to [%s]",
                                icd_caller__get_id((icd_caller *) agent), icd_caller__get_name((icd_caller *) agent),
                                fieldval);
                }
                /*found bridge_tech */
                if (agent) {
                    fieldval = icd_config__get_value(config, "agent_id");
                    if (fieldval){
                        icd_fieldset__set_value(agents, fieldval, agent);
                        icd_caller__set_caller_id((icd_caller *) agent, fieldval);
                    }    
                    else {
                        icd_fieldset__set_value(agents, entry, agent);
                        icd_caller__set_caller_id((icd_caller *) agent, entry);
                    }    
                    icd_agent__add_listener(agent, agents, clear_agent_from_registry, entry);
                }

                cw_verbose(VERBOSE_PREFIX_3 "Create Agent [%s] %s\n", entry, agent ? "success" : "failure");

                /* Now add memberships from outstanding_members. */
                fieldval = icd_fieldset__get_value(outstanding_members, entry);
                if (fieldval != NULL) {
                    /* We can carve up fieldval directly because we free it here anyway */
                    queuesleft = fieldval;
                    while (queuesleft != NULL) {
                        /* This has been normalized to use only a '|' or ',' to separate queue names */
                        currqueue = strsep(&queuesleft, "|,");
                        if (currqueue != NULL && strlen(currqueue) > 0) {
                            queue = icd_fieldset__get_value(queues, currqueue);
                            if (queue != NULL) {
                                icd_caller__add_to_queue((icd_caller *) agent, queue);
                            } else {
                                cw_log(LOG_WARNING, "Create Agent: Agent %s requires nonexistent Queue %s!\n",
                                        entry, currqueue);
                            }
                        }
                    }
                    ICD_STD_FREE(fieldval);
                }

                /*add  memberships from outstanding_members. */
            } else {
                cw_log(LOG_NOTICE, "TBD Merge Agent '%s' %s\n", entry, agent ? "success" : "failure");
            }
            destroy_icd_config(&config);
        }                       /* not a general category ie we got an agent definition */
    }                           /* while more entries */
    destroy_icd_config(&general_config);        /* TC make this a a global so that we can default agents ? */

    cw_config_destroy(astcfg);
    return ICD_SUCCESS;
}

/*===== Private Implemenations =====*/


/* Listener that responds to an agent being cleared by removing it from the registry */
static int clear_agent_from_registry(void *listener, icd_event * event, void *extra)
{
    icd_fieldset *agents = listener;
    char *agentname = extra;

    assert(agents != NULL);
    assert(agentname != NULL);

    if (icd_event__get_event_id(event) == ICD_EVENT_CLEAR) {
        icd_fieldset__remove_key(agents, agentname);
        cw_verbose(VERBOSE_PREFIX_2 "Listener on agent registry detected agent [%s] being cleared\n", agentname);
    }
    return 0;
}

static int clear_queue_from_registry(void *listener, icd_event * event, void *extra)
{
    icd_fieldset *queues = listener;
    char *queuename = extra;

    assert(queues != NULL);
    assert(queuename != NULL);

    if (icd_event__get_event_id(event) == ICD_EVENT_CLEAR) {
        icd_fieldset__remove_key(queues, queuename);
        cw_verbose(VERBOSE_PREFIX_2 "Listener on queue registry detected queue [%s] being cleared\n", queuename);
    }
    return 0;
}

//! Stores a list of agents associated with a particular queue in a map
/*!
 * This function goes through the list of agents stored in the agent parameter
 * and safely tokenizes them (the original string is preserved). It then adds
 * the queuename to the list of queues that the agent is a member for. This
 * list of queues is itself a string with the queue names separated by a
 * "," symbol.
 * \param agents a string listing the agents for this queue, spearated by ",", ";", "|", or spaces
 * \param queuename the name of the queue the agents belong to
 * \param map place where the associations are kept
 * \return success or an ICD error status code
 * \warning Currently, spaces are not allowed in agent names because they are used
 *         to separate tokens in the agents parameter. This may change, so try to
 *         always use commas to separate agent names instead.
 */
static icd_status app_icd__store_agent_list(char *agents, char *queuename, icd_fieldset * map)
{
    char *agentslist;
    char *agentsleft;
    char *curragent;
    char *currqueuelist;
    int available_len;
    int required_len;

    required_len = strlen(queuename) + 1;
    /* strsep is destructive. Protect the string in case it is useful */
    agentslist = strdup(agents);
    agentsleft = agentslist;
    while (agentsleft != NULL) {
        /* Should we allow spaces in agent names or use as separators? */
        curragent = strsep(&agentsleft, ",;| ");
        /* Check to see if we already have a queue for this agent */
        if (curragent == NULL || strlen(curragent) == 0) {
            continue;
        }
        currqueuelist = icd_fieldset__get_value(map, curragent);
        if (currqueuelist == NULL) {
            // Create currqueuelist
            currqueuelist = (char *) ICD_STD_MALLOC(queue_entry_len);
            memset(currqueuelist, 0, queue_entry_len);
        }
        available_len = queue_entry_len - strlen(currqueuelist);
        if (available_len >= required_len) {
            if (strlen(currqueuelist) != 0) {
                strcat(currqueuelist, ",");
            }
            strcat(currqueuelist, queuename);
            icd_fieldset__set_value(map, curragent, currqueuelist);
        } else {
            cw_log(LOG_WARNING, "Create Queue: Queue List for Agent %s too long, Queue %s not linked!\n",
                    curragent, queuename);
        }
    }
    ICD_STD_FREE(agentslist);
    return ICD_SUCCESS;
}

//! Stores a list of queues associated with a particular agent in a map
/*!
 * This function goes through the list of queues stored in the queue parameter
 * and safely tokenizes them (the original string is preserved). It then adds
 * the queue to the list of queues that the agent is a member for. This
 * list of queues is itself a string with the queue names separated by a
 * "," symbol.
 * \param queues a string listing the queues for this agent, spearated by ",", ";", "|", or spaces
 * \param agentname the name of the agent that belongs to the queue
 * \param map place where the associations are kept
 * \return success or an ICD error status code
 * \warning Currently, spaces are not allowed in queue names because they are used
 *         to separate tokens in the queues parameter. This may change, so try to
 *         always use commas to separate queue names instead.
 */
static icd_status app_icd__store_queue_list(char *queues, char *agentname, icd_fieldset * map)
{
    char *queueslist;
    char *queuesleft;
    char *currqueue;
    char *currqueuelist;
    int available_len;
    int altered = 0;

    /* Check to see if we already have a queue for this agent */
    currqueuelist = icd_fieldset__get_value(map, agentname);
    if (currqueuelist == NULL) {
        // Create currqueuelist
        currqueuelist = (char *) ICD_STD_MALLOC(queue_entry_len);
        memset(currqueuelist, 0, queue_entry_len);
    }
    /* strsep is destructive. Protect the string in case it is useful */
    queueslist = strdup(queues);
    queuesleft = queueslist;
    while (queuesleft != NULL) {
        /* Should we allow spaces in queue names or use as separators? */
        currqueue = strsep(&queuesleft, ",;| ");
        available_len = queue_entry_len - strlen(currqueuelist);
        if (currqueue != NULL && strlen(currqueue) > 0 && available_len > strlen(currqueue)) {
            if (strlen(currqueuelist) != 0) {
                strcat(currqueuelist, ",");
            }
            strcat(currqueuelist, currqueue);
            altered = 1;
        } else {
            cw_log(LOG_WARNING, "Create Queue: Queue List for Agent %s too long, Queue %s not linked!\n",
                    agentname, currqueue);
        }
    }
    if (altered) {
        icd_fieldset__set_value(map, agentname, currqueuelist);
    }
    ICD_STD_FREE(queueslist);
    return ICD_SUCCESS;
}

static int run_conference(cw_channel * chan, icd_customer * customer, icd_conference * conf, char *spymode)
{
    int num;

    icd_conference__associate((icd_caller *) customer, conf, 0);
    icd_caller__add_flag((icd_caller *) customer, ICD_CONF_MEMBER_FLAG);
    if (spymode && cw_true(spymode)) {
        icd_caller__add_flag((icd_caller *) customer, ICD_MONITOR_FLAG);
    } else
        icd_caller__clear_flag((icd_caller *) customer, ICD_MONITOR_FLAG);
    icd_caller__add_flag((icd_caller *) customer, ICD_NOHANGUP_FLAG);
    icd_caller__set_state((icd_caller *) customer, ICD_CALLER_STATE_CONFERENCED);
    num = atoi(conf->name);
    if (num)
        cw_say_digits(chan, num,  CW_DIGIT_ANY, NULL);
    num = icd_bridge__play_sound_file(chan, "auth-thankyou");
    if (!num)
        icd_caller__loop((icd_caller *) customer, 0);
    return num;
}

icd_agent *app_icd__dtmf_login(struct cw_channel *chan, char *login, char *pass, int tries)
{
    /* Give $tries chances to log in */
    int try = 0, res = 0;
    char agentbuf[256], passbuf[256];
    icd_agent *agent = NULL;
    char *agentname = login;
    char *agentpass = pass;

    for (try = 0; try < tries; try++) {
        agent = NULL;

        if (!agentname) {
            memset(agentbuf, 0, sizeof(agentbuf) - 1);
            res =
                cw_app_getdata(chan, try == 0 ? "agent-user" : "agent-incorrect", agentbuf, sizeof(agentbuf) - 1,
                                0);
            agentname = agentbuf;
        }
        if (!res) {
            /* We probably want to search on ID here, as well. But this will do for now */
            /* UPDATE: I added the pointer again using the agent_id as key so both ways work -Tony */

            if (agentname)
                agent = icd_fieldset__get_value(agents, agentname);
            if (agent != NULL) {
                if (!agentpass) {
                    memset(passbuf, 0, sizeof(passbuf) - 1);
                    res = cw_app_getdata(chan, "agent-pass", passbuf, sizeof(passbuf) - 1, 0);
                    agentpass = passbuf;
                }
                if (!res) {
                    if (!strcmp(agentpass, icd_caller__get_param((icd_caller *) agent, "passwd"))) {
                        res = icd_bridge__play_sound_file(chan, "agent-loginok");
                        break;
                    } else
                        agent = NULL;
                } else
                    agent = NULL;
            }
        }

    }
    return agent;
}

int icd_instr(char *bigstr, char *smallstr, char delimit)
{
    char *val = bigstr, *next;
    int cmp_str_len;

    do {
        if ((next = strchr(val, delimit))){
            cmp_str_len = (strlen(smallstr) > (next - val)) ? strlen(smallstr) : next - val;
            if (!strncmp(val, smallstr, cmp_str_len))
                return 1;
            else
                continue;
        }
        else
            return !strcmp(smallstr, val);

    } while (*(val = (next + 1)));

    return 0;
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


