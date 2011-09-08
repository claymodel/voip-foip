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
  * \brief icd_command.c - cli commands for icd
  */
 
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include "callweaver/icd/app_icd.h"
#include "callweaver/icd/icd_command.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_fieldset.h"
/* For dump function only */
#include "callweaver/icd/icd_queue.h"
#include "callweaver/icd/icd_distributor.h"
#include "callweaver/icd/icd_list.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_member.h"
#include "callweaver/icd/icd_member_list.h"
#include "callweaver/icd/icd_bridge.h"
#include "callweaver/icd/icd_agent.h"
#include "callweaver/icd/icd_customer.h"
#include "callweaver/icd/icd_caller_private.h"
#include "callweaver/icd/icd_conference.h"
#include "callweaver/icd/icd_play_dtmf.h"

static int verbosity = 1;

/* This is the lock customers add, remove and seek */
    extern cw_mutex_t customers_lock;


/*
static char show_icd_help[] =
"Usage: icd command <command>\n"
"       run a particular icd command.\n";
*/

static void_hash_table *COMMAND_HASH;
static icd_status icd_command_show_queue(int fd, int argc, char **argv);
static icd_status icd_command_show_agent(int fd, int argc, char **argv);
static icd_status icd_command_show_customer(int fd, int argc, char **argv);
static icd_status icd_command_dump_queue(int fd, int argc, char **argv);
static icd_status icd_command_dump_distributor(int fd, int argc, char **argv);
static icd_status icd_command_dump_agent(int fd, int argc, char **argv);
static icd_status icd_command_dump_customer(int fd, int argc, char **argv);
static icd_status icd_command_load_queues(int fd, int argc, char **argv);
static icd_status icd_command_load_agents(int fd, int argc, char **argv);
static icd_status icd_command_load_conferences(int fd, int argc, char **argv);
static icd_status icd_command_load_app_icd(int fd, int argc, char **argv);

typedef struct icd_command_node icd_command_node;

struct icd_command_node {
    int (*func) (int, int, char **);
    char name[ICD_STRING_LEN];
    char short_help[ICD_STRING_LEN];
    char syntax_help[ICD_STRING_LEN];
    char long_help[ICD_STRING_LEN];
    icd_memory *memory;
};

void create_command_hash(void)
{
    COMMAND_HASH = (void_hash_table *) vh_init("Command Hash");
    /* some default given commands 
     * "name" , "func pointer" , "short description" , "syntax usage text" , "long specific help message."
     */
    icd_command_register("help", icd_command_help, "help with the icd command system", "[topic]",
        "Enter a specific command for detailed help\nor no arguement for a full listing of avaliable commands");
    /* blank values will allow this one to not show up in the listing. */
    icd_command_register("_bad_command", icd_command_bad, "", "", "");

    icd_command_register("verbose", icd_command_verbose, "set verbosity of icd command output", "Level [1..9]",
        "Set a value of 1 to 9");

    icd_command_register("debug", icd_command_debug, "set debug icd debug output", "icd debug [on|off]",
        "control of debug dumps reading config files and other process messages during icd call routing");

    icd_command_register("show", icd_command_show, "show agent or queue infomation",
        "<object type> [specific object]", "Available object types:\n\nqueue, agent, customer");

    icd_command_register("dump", icd_command_dump, "dump internal information", "<object type> [specific object]",
        "Available object types:\n\nqueue,distributor,caller, agent, customer");

    icd_command_register("load", icd_command_load, "reload icd queues and agents from config files ",
        "<agents|queues>", "Load new configuration data from the icd config files");
 
    icd_command_register("transfer", icd_command_transfer, "transfer customer to a new extension ",
        "icd transfer <CustomerUniqueID> <extension@context:priority>", "");
 
    icd_command_register("ack", icd_command_ack, "send ACK signal for agent ",
        "icd ack <agent id>", "");
 
    icd_command_register("login", icd_command_login, "login agent ",
        "icd login <dialstring> <agent id> <password>", "");
 
    icd_command_register("logout", icd_command_logout, "logout agent ",
        "icd logout <agent id> <password>", "");
 
    icd_command_register("hangup", icd_command_hang_up, "hangup agent ",
        "icd hangup <agent id>", "");

    icd_command_register("hangup_chan", icd_command_hangup_channel, "hangup channel ",
        "icd hangup <channel name>", "");

    icd_command_register("playback_chan", icd_command_playback_channel, "playback channel ",
         "icd playback_chan <channel name>", "");

    icd_command_register("record", icd_command_record, "Start/stop record of customer ",
        "icd record <start|stop> <customer unique name>", "");

    icd_command_register("queue", icd_command_join_queue, "join/remove agent to/from queue ",
        "icd queue <agent id> <queue name|all> <R>", "R for remove");

    icd_command_register("control_playback", icd_command_control_playback, "controled playback in ICD", 
        "icd control_playback <agent id> <key>", "");

}

static icd_command_node *create_command_node(int (*func) (int, int, char **), char *name, char *short_help,
    char *syntax_help, char *long_help)
{
    icd_command_node *new;

    ICD_MALLOC(new, sizeof(icd_command_node));
    new->func = func;
    strncpy(new->name, name, sizeof(new->name));
    strncpy(new->short_help, short_help, sizeof(new->short_help));
    strncpy(new->syntax_help, syntax_help, sizeof(new->syntax_help));
    strncpy(new->long_help, long_help, sizeof(new->long_help));

    return new;
}

static void destroy_command_node(icd_command_node ** node)
{
    if (*node == NULL)
        return;
    ICD_FREE((*node));
}

static int cli_line(int fd, char *c, int y)
{
    int x = 0;

    for (x = 0; x < y; x++)
        cw_cli(fd, "%s", c);
    cw_cli(fd, "\n");
    /* BCA - What should this return? */
    return ICD_SUCCESS;
}

int icd_command_register(char *name, int (*func) (int, int, char **), char *short_help, char *syntax_help,
    char *long_help)
{
    icd_command_node *insert = NULL;

    if (!COMMAND_HASH)
        create_command_hash();

    insert = (icd_command_node *) create_command_node(func, name, short_help, syntax_help, long_help);
    if (vh_write(COMMAND_HASH, name, insert) != -1) {
        return ICD_SUCCESS;
    }
    return ICD_EGENERAL;
}

void *icd_command_pointer(char *name)
{
    icd_command_node *fetch = NULL;

    fetch = (icd_command_node *) vh_read(COMMAND_HASH, name);
    if (fetch)
        return fetch->func;
    else
        return NULL;
}

void destroy_command_hash(void)
{
    icd_command_node *fetch;
    vh_keylist *keys;

    for (keys = vh_keys((COMMAND_HASH)); keys; keys = keys->next) {
        fetch = (icd_command_node *) vh_read(COMMAND_HASH, keys->name);
        vh_delete(COMMAND_HASH, keys->name);
        destroy_command_node(&fetch);
        fetch = NULL;
    }
    vh_destroy(&COMMAND_HASH);
}

int icd_command_cli(int fd, int argc, char **argv)
{
    int (*func) (int, int, char **);
    char **newargv;
    int newargc;
    int x = 0, y = 0;

    func = NULL;

    if (argc > 1) {
        func = icd_command_pointer(argv[1]);
        if (func == NULL)
            func = icd_command_pointer("_bad_command");
    } else
        func = icd_command_pointer("help");

    if (func != NULL) {
        newargv = malloc((argc - 1)*sizeof(char *));

        for (x = 1; x < argc; x++) {
            newargv[y] = malloc(strlen(argv[x]) + 1);
            strncpy(newargv[y], argv[x], strlen(argv[x]) + 1);
            y++;
        }

        newargc = argc - 1;
        func(fd, newargc, newargv);
        y = 0;
        for (x = 1; x < argc; x++) {
            free(newargv[y++]);
        }
        free(newargv);
    } else
        cw_cli(fd, "Mega Error %d\n", argc);

    return ICD_SUCCESS;
}

static int icd_command_short_help(int fd, icd_command_node * node)
{
    cw_cli(fd, "'%s'", node->short_help);

    return ICD_SUCCESS;
}

static int icd_command_syntax_help(int fd, icd_command_node * node)
{
    cw_cli(fd, "Usage: %s %s", node->name, node->syntax_help);

    return ICD_SUCCESS;
}

static int icd_command_long_help(int fd, icd_command_node * node)
{
    cw_cli(fd, "%s", node->long_help);

    return ICD_SUCCESS;
}

/* all our commands */
int icd_command_list(int fd, int argc, char **argv)
{
    icd_command_node *fetch;
    vh_keylist *keys;

    if (argc < 2) {
        cw_cli(fd, "\n\n");
        cw_cli(fd, "Available Commands\n");
        cli_line(fd, "=", 80);
        cw_cli(fd, "\n");

        for (keys = vh_keys((COMMAND_HASH)); keys; keys = keys->next) {

            fetch = (icd_command_node *) vh_read(COMMAND_HASH, keys->name);
            if (fetch && strcmp(fetch->short_help, "")) {
                cw_cli(fd, "%s: ", fetch->name);
                icd_command_short_help(fd, fetch);
                cw_cli(fd, "\n");
            }
        }

        cw_cli(fd, "\n");
        cli_line(fd, "=", 80);
        cw_cli(fd, "\n");

        return ICD_SUCCESS;
    }
    // else 

    fetch = (icd_command_node *) vh_read(COMMAND_HASH, argv[1]);
    if (fetch) {

        cw_cli(fd, "\n\n");
        cw_cli(fd, "Help with '%s'\n", fetch->name);
        cli_line(fd, "=", 80);
        cw_cli(fd, "\n");

        cw_cli(fd, "%s: ", fetch->name);
        icd_command_short_help(fd, fetch);
        cw_cli(fd, "\n");
        icd_command_syntax_help(fd, fetch);
        cw_cli(fd, "\n");
        cw_cli(fd, "\n");
        icd_command_long_help(fd, fetch);
        cw_cli(fd, "\n");
        cw_cli(fd, "\n");
        cli_line(fd, "=", 80);
        cw_cli(fd, "\n");

    }

    return ICD_SUCCESS;
}

int icd_command_help(int fd, int argc, char **argv)
{
    icd_command_list(fd, argc, argv);
    cw_cli(fd, "\nUsage 'icd <command> <arg1> .. <argn>\n");

    /* BCA - What should this return? */
    return ICD_SUCCESS;
}

int icd_command_bad(int fd, int argc, char **argv)
{
    int x;

    for (x = 0; x < argc; x++)
        cw_cli(fd, "%d=%s\n", x, argv[x]);

    cw_cli(fd, "\n\nInvalid Command\n");
    icd_command_help(fd, argc, argv);

    /* BCA - What should this return? */
    return ICD_SUCCESS;
}

int icd_command_verbose(int fd, int argc, char **argv)
{

    if (argc == 2) {
        if (!strcmp(argv[1], "ast")) {
            icd_verbose = option_verbose;
            return ICD_SUCCESS;
        }
        icd_verbose = atoi(argv[1]);
        if (icd_verbose > 0 && icd_verbose < 10)
            cw_cli(fd, "ICD Verbosity[%d] set \n", icd_verbose);
        else
            cw_cli(fd, "ICD Verbosity[%d] range is 1-9 not [%s] \n", icd_verbose, argv[1]);
    } else
        cw_cli(fd, "ICD Verbosity[%d] range is 1-9 \n", icd_verbose);

    return ICD_SUCCESS;
}

int icd_command_debug(int fd, int argc, char **argv)
{

    if (argc == 2) {
        if (!strcmp(argv[1], "on"))
            icd_debug = 1;
        else if (!strcmp(argv[1], "off"))
            icd_debug = 0;
        else
            cw_cli(fd, "ICD debug[%d] must be either [on] or [off] not[%s]\n", icd_debug, argv[1]);
    } else
        cw_cli(fd, "ICD debug[%d] must be either [on] or [off] \n", icd_debug);

    return ICD_SUCCESS;
}

int icd_command_show(int fd, int argc, char **argv)
{
    static char *help[2] = { "help", "show" };

    if (argc >= 2) {
        if (!strcmp(argv[1], "q") || !strcmp(argv[1], "queue") || !strcmp(argv[1], "queues")) {
            icd_command_show_queue(fd, argc, argv);
        }

        if (!strcmp(argv[1], "a") || !strcmp(argv[1], "agent") || !strcmp(argv[1], "agents")) {
            icd_command_show_agent(fd, argc, argv);
        }
	if (!strcmp(argv[1], "c") || !strcmp(argv[1], "customer") || !strcmp(argv[1], "customers")) {
            icd_command_show_customer(fd, argc, argv);
        }
    } else
        icd_command_help(fd, 2, help);

    return ICD_SUCCESS;
}

icd_status icd_command_show_queue(int fd, int argc, char **argv)
{
//QUEUE UNATTENDED CALLS        ASSIGNED/THIS QUEUE/OTHER QUEUE
#define FMT_QUEUE_HEADING "%-18s %-8s %-14s %-15s %-10s %-18s\n"

    icd_fieldset_iterator *iter;
    char *curr_key;
    icd_queue *queue;
    int state_count[20];
    int state;
    icd_list_iterator *list_iter;
    icd_member *member;
    icd_caller *caller = NULL;

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, FMT_QUEUE_HEADING, "QUEUE", "AGENTS", "LOGIN AGENTS", "PENDING AGENTS", "CUSTOMERS", "PENDING CUSTOMERS");

    iter = icd_fieldset__get_key_iterator(queues);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        if (argc == 2 || (!strcmp(curr_key, argv[2]))) {
            queue = (icd_queue *) icd_fieldset__get_value(queues, curr_key);
            icd_queue__show(queue, verbosity, fd);
            for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state_count[state++] = 0);
	    icd_member_list__lock(icd_queue__get_agents(queue));
            list_iter = icd_queue__get_agent_iterator(queue);
            while (icd_list_iterator__has_more(list_iter)) {
                member = (icd_member *) icd_list_iterator__next(list_iter);
		if(member){
	            caller = icd_member__get_caller(member);
	            state_count[icd_caller__get_state(caller)]++;
		}
	    }
	    icd_member_list__unlock(icd_queue__get_agents(queue));
            destroy_icd_list_iterator(&list_iter);
	    cw_cli(fd, "AGENTS STATES:");
	    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state++ ){
                if(state_count[state] > 0)
                    cw_cli(fd, " %s = %d,", icd_caller_state_strings[state] + 17, state_count[state]);
            }
            
	    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state_count[state++] = 0);
	    icd_member_list__lock(icd_queue__get_customers(queue));
            list_iter = icd_queue__get_customer_iterator(queue);
            while (icd_list_iterator__has_more(list_iter)) {
                member = (icd_member *) icd_list_iterator__next(list_iter);
		if(member){
	            caller = icd_member__get_caller(member);
	            state_count[icd_caller__get_state(caller)]++;
		}
	    }
	    icd_member_list__unlock(icd_queue__get_customers(queue));
            destroy_icd_list_iterator(&list_iter);
	    cw_cli(fd, "\nCUSTOMERS STATES:");
	    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state++ ){
                if(state_count[state] > 0)
                    cw_cli(fd, " %s = %d,", icd_caller_state_strings[state] + 17, state_count[state]);
            }
	    cw_cli(fd, "\n");
	    
            if (argc != 2)
                break;
        }
    }
    destroy_icd_fieldset_iterator(&iter);

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

/* Create a cli ui display of the agent */
icd_status icd_command_show_agent(int fd, int argc, char **argv)
{
#define FMT_AGENT_HEADING "%-10s %-5s %-15s %-25s %-20s %20s %-10s  %-5s\n"
#define FMT_AGENT_DATA1   "%-10s %-5d %-15s %-25s %-20s %-20s "
#define FMT_AGENT_DATA2   "%s:%d:%s:%s:%s:%s:%s:\n"

    char *curr_key;
    struct cw_channel *chan = NULL;
    icd_agent *agent = NULL;
    icd_caller *caller = NULL;
    icd_caller *associate = NULL;
    icd_fieldset_iterator *iter;
    icd_list_iterator *list_iter;
    icd_member *member;
    icd_queue *queue;
    int state_count[20];
    int state;
    char buf[256];

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, FMT_AGENT_HEADING, "GROUP", "ID", "NAME", "STATE", "CHANNEL", "TALKING", "QUEUE", "LISTEN CODE");
    
    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state_count[state++] = 0);
        
    iter = icd_fieldset__get_key_iterator(agents);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        agent = icd_fieldset__get_value(agents, curr_key);
        caller = (icd_caller *) agent;
        buf[0] = '\0';

	state_count[icd_caller__get_state(caller)]++;
	if(argc >=3)
	  if(strcmp(argv[2],icd_caller__get_caller_id(caller)))
	     continue; 
        /* lets find all the channels they are talking to */
        if (icd_caller__get_state(caller) == ICD_CALLER_STATE_BRIDGED || icd_caller__get_state(caller) == ICD_CALLER_STATE_CONFERENCED) {
            icd_list__lock((icd_list *) (icd_caller__get_associations(caller)));
            list_iter = icd_list__get_iterator((icd_list *) (icd_caller__get_associations(caller)));
            while (icd_list_iterator__has_more(list_iter)) {
                associate = (icd_caller *) icd_list_iterator__next(list_iter);

                if (icd_caller__get_state(associate) == ICD_CALLER_STATE_BRIDGED || icd_caller__get_state(caller) == ICD_CALLER_STATE_CONFERENCED) {
                    chan = icd_caller__get_channel(associate);
                  
                    if ((sizeof(buf) - strlen(buf) - strlen(chan->name) - 1) > 0)
                        strcat(buf, chan->name);
/*
                if ((sizeof(buf) - strlen(buf) -strlen(chan->callerid ? chan->callerid : "unknown") -1) > 0 )
                    strcat(buf,chan->callerid ? chan->callerid : "unknown");                
*/
                }
            }
            destroy_icd_list_iterator(&list_iter);
            icd_list__unlock((icd_list *) (icd_caller__get_associations(caller)));
        }
	cw_cli(fd, FMT_AGENT_DATA1, (char *) icd_caller__get_param(caller, "group"),
            icd_caller__get_id(caller), icd_caller__get_name(caller), icd_caller__get_state_string(caller)+17,
            icd_caller__get_channel(caller) ? icd_caller__get_channel(caller)->name : "(None)", buf);
	    
        icd_list__lock((icd_list *) (icd_caller__get_memberships(caller)));
        list_iter = icd_list__get_iterator((icd_list *) (icd_caller__get_memberships(caller)));
        while (icd_list_iterator__has_more(list_iter)) {
                member = (icd_member *) icd_list_iterator__next(list_iter);
		if(member){
	          queue = icd_member__get_queue(member);
		  if(queue){
                      cw_cli(fd, "%s, ", icd_queue__get_name(queue));
		  }
                }
        }
        destroy_icd_list_iterator(&list_iter);
        icd_list__unlock((icd_list *) (icd_caller__get_memberships(caller)));
        

        cw_cli(fd, "\n");
    }

    destroy_icd_fieldset_iterator(&iter);
    cw_cli(fd, "AGENTS IN STATE:" );
    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state++ ){
          if(state_count[state] > 0)
                cw_cli(fd, " %s = %d,", icd_caller_state_strings[state] + 17, state_count[state]);
    }

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;

}

/* Create a cli ui display of the agent */
icd_status icd_command_show_customer(int fd, int argc, char **argv)
{
#define FMT_CUSTOMER_HEADING "%-10s %-5s %-20s %-25s %-20s %20s %-10s  %-5s\n"
#define FMT_CUSTOMER_DATA1   "%-10s %-5d %-20s %-25s %-20s %-20s "
#define FMT_CUSTOMER_DATA2   "%s:%d:%s:%s:%s:%s:%s:\n"

    char *curr_key;
    struct cw_channel *chan = NULL;
    icd_customer *customer = NULL;
    icd_caller *caller = NULL;
    icd_caller *associate = NULL;
    icd_fieldset_iterator *iter;
    icd_list_iterator *list_iter;
    icd_member *member;
    icd_queue *queue;
    int state_count[20];
    int state;
    char buf[256];

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, FMT_CUSTOMER_HEADING, "GROUP", "ID", "CALLER ID", "STATE", "CHANNEL", "TALKING", "QUEUE", "LISTEN CODE");
    
    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state_count[state++] = 0);
    
    cw_mutex_lock(&customers_lock);       
    iter = icd_fieldset__get_key_iterator(customers);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        customer = icd_fieldset__get_value(customers, curr_key);
        caller = (icd_caller *) customer;
        buf[0] = '\0';

	state_count[icd_caller__get_state(caller)]++;
	if(argc >=3)
	  if(strcmp(argv[2],icd_caller__get_caller_id(caller)))
	     continue; 
        /* lets find all the channels they are talking to */
        if (icd_caller__get_state(caller) == ICD_CALLER_STATE_BRIDGED || icd_caller__get_state(caller) == ICD_CALLER_STATE_CONFERENCED) {
            icd_list__lock((icd_list *) (icd_caller__get_associations(caller)));
            list_iter = icd_list__get_iterator((icd_list *) (icd_caller__get_associations(caller)));
            while (icd_list_iterator__has_more(list_iter)) {
                associate = (icd_caller *) icd_list_iterator__next(list_iter);

                if (icd_caller__get_state(associate) == ICD_CALLER_STATE_BRIDGED || icd_caller__get_state(caller) == ICD_CALLER_STATE_CONFERENCED) {
                    chan = icd_caller__get_channel(associate);
                  
                    if ((sizeof(buf) - strlen(buf) - strlen(chan->name) - 1) > 0)
                        strcat(buf, chan->name);
/*
                if ((sizeof(buf) - strlen(buf) -strlen(chan->callerid ? chan->callerid : "unknown") -1) > 0 )
                    strcat(buf,chan->callerid ? chan->callerid : "unknown");                
*/
                }
            }
            destroy_icd_list_iterator(&list_iter);
            icd_list__unlock((icd_list *) (icd_caller__get_associations(caller)));
        }
	cw_cli(fd, FMT_CUSTOMER_DATA1, (char *) icd_caller__get_param(caller, "group"),
            icd_caller__get_id(caller), icd_caller__get_caller_id(caller), icd_caller__get_state_string(caller)+17,
            icd_caller__get_channel(caller) ? icd_caller__get_channel(caller)->name : "(None)", buf);
	    
        icd_list__lock((icd_list *) (icd_caller__get_memberships(caller)));
        list_iter = icd_list__get_iterator((icd_list *) (icd_caller__get_memberships(caller)));
        while (icd_list_iterator__has_more(list_iter)) {
                member = (icd_member *) icd_list_iterator__next(list_iter);
		if(member){
	          queue = icd_member__get_queue(member);
		  if(queue){
                      cw_cli(fd, "%s, ", icd_queue__get_name(queue));
		  }
                }
        }
        destroy_icd_list_iterator(&list_iter);
        icd_list__unlock((icd_list *) (icd_caller__get_memberships(caller)));
        

        cw_cli(fd, "\n");
    }

    cw_mutex_unlock(&customers_lock);
    destroy_icd_fieldset_iterator(&iter);
    cw_cli(fd, "CUSTOMERS IN STATE:" );
    for(state = ICD_CALLER_STATE_CREATED; state <= ICD_CALLER_STATE_CONFERENCED; state++ ){
          if(state_count[state] > 0)
                  cw_cli(fd, " %s = %d,", icd_caller_state_strings[state] + 17, state_count[state]);
    }

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;

}

int icd_command_dump(int fd, int argc, char **argv)
{
    static char *help[2] = { "help", "dump" };

    if (argc >= 2) {
        if (!strcmp(argv[1], "q") || !strcmp(argv[1], "queue") || !strcmp(argv[1], "queues")) {
            icd_command_dump_queue(fd, argc, argv);
        }
        if (!strcmp(argv[1], "d") || !strcmp(argv[1], "dist") || !strcmp(argv[1], "distributors")) {
            icd_command_dump_distributor(fd, argc, argv);
        }
        if (!strcmp(argv[1], "caller") || !strcmp(argv[1], "callers")) {
            icd_command_dump_customer(fd, argc, argv);
            icd_command_dump_agent(fd, argc, argv);
        }
        if (!strcmp(argv[1], "a") || !strcmp(argv[1], "agent") || !strcmp(argv[1], "agents")) {
            icd_command_dump_agent(fd, argc, argv);
        }
        if (!strcmp(argv[1], "c") || !strcmp(argv[1], "customer") || !strcmp(argv[1], "customers")) {
            icd_command_dump_customer(fd, argc, argv);
        }
    } else
        icd_command_help(fd, 2, help);

    /* BCA - What should this return? */
    return ICD_SUCCESS;
}

static icd_status icd_command_dump_queue(int fd, int argc, char **argv)
{
    icd_fieldset_iterator *iter;
    char *curr_key;
    icd_queue *queue;
    icd_distributor *dist;

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "Queue Dump \n");

    iter = icd_fieldset__get_key_iterator(queues);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        if (argc == 2 || (!strcmp(curr_key, argv[2]))) {
            cw_cli(fd, "\nFound %s\n", curr_key);
            queue = (icd_queue *) icd_fieldset__get_value(queues, curr_key);
            icd_queue__dump(queue, verbosity, fd);
            dist = (icd_distributor *) icd_queue__get_distributor(queue);
            /*
               if (dist)
               icd_distributor__dump(dist, verbosity, fd);
             */
            if (argc != 2)
                break;
        }
    }
    destroy_icd_fieldset_iterator(&iter);

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

static icd_status icd_command_dump_distributor(int fd, int argc, char **argv)
{
/*
    icd_distributor *dist;

     cw_cli(fd,"\n");
     cli_line(fd,"=",80);
     cw_cli(fd,"\n");

 
     cw_cli(fd,"\n");
     cli_line(fd,"=",80);
     cw_cli(fd,"\n");
*/
    return ICD_SUCCESS;
}

static icd_status icd_command_dump_customer(int fd, int argc, char **argv)
{
    icd_fieldset_iterator *fs_iter;
    char *curr_key;
    icd_queue *queue;
    icd_member_list *customers;

    icd_list_iterator *iter;
    icd_member *member;
    icd_caller *caller;

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "Customer Dump \n");

    fs_iter = icd_fieldset__get_key_iterator(queues);
    if (fs_iter == NULL) {
        return 0;
    }
    while (icd_fieldset_iterator__has_more(fs_iter)) {
        curr_key = icd_fieldset_iterator__next(fs_iter);
        cw_cli(fd, "\nCustomers in Queue %s\n", curr_key);
        queue = icd_fieldset__get_value(queues, curr_key);
        customers = (icd_member_list *) icd_queue__get_customers(queue);

        if (verbosity > 1) {
            iter = icd_list__get_iterator((icd_list *) (customers));
            if (iter != NULL) {
                while (icd_list_iterator__has_more(iter)) {
                    member = (icd_member *) icd_list_iterator__next(iter);
                    caller = (icd_caller *) icd_member__get_caller(member);
                    if (caller) {
                        //caller->dump_fn_extra
                        icd_caller__dump(caller, verbosity, fd);
                    }
                    destroy_icd_list_iterator(&iter);
                }
            }
        } else {
            icd_member_list__dump(customers, verbosity, fd);
        }

    }
    destroy_icd_fieldset_iterator(&fs_iter);

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

static icd_status icd_command_dump_agent(int fd, int argc, char **argv)
{
    icd_fieldset_iterator *iter;
    char *curr_key;
    icd_agent *agent = NULL;
    icd_caller *caller = NULL;

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "Agent Dump \n");

    iter = icd_fieldset__get_key_iterator(agents);
    while (icd_fieldset_iterator__has_more(iter)) {
        curr_key = icd_fieldset_iterator__next(iter);
        agent = icd_fieldset__get_value(agents, curr_key);
        caller = (icd_caller *) agent;
	if(argc >=3)
	  if(strcmp(argv[2],icd_caller__get_caller_id(caller)))
	     continue; 	
        icd_caller__dump(caller, verbosity, fd);
    }
    destroy_icd_fieldset_iterator(&iter);

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

/*
static icd_status icd_command_dump__agent(int fd, int argc, char **argv) {

    icd_queue *queue;
    icd_caller *caller;

     cw_cli(fd,"\n");
     cli_line(fd,"=",80);
     cw_cli(fd,"\n");
     cw_cli(fd,"Agent Dump \n",);

     iter = icd_fieldset__get_key_iterator(agents);
     while (icd_fieldset_iterator__has_more(iter)) {
       curr_key = icd_fieldset_iterator__next(iter);
       cw_cli(fd,"\nFound %s\n",curr_key);
       agent = icd_fieldset__get_value(agents, curr_key) ;
     }
     destroy_icd_fieldset_iterator(&iter);

     cw_cli(fd,"\n");
     cli_line(fd,"=",80);
     cw_cli(fd,"\n");

    return ICD_SUCCESS;
}
*/
int icd_command_load(int fd, int argc, char **argv)
{
    static char *help[2] = { "help", "load" };

    if (argc >= 2) {
        if (!strcmp(argv[1], "i") || !strcmp(argv[1], "icd")) {
            icd_command_load_app_icd(fd, argc, argv);
        }
        if (!strcmp(argv[1], "q") || !strcmp(argv[1], "queue") || !strcmp(argv[1], "queues")) {
            icd_command_load_queues(fd, argc, argv);
        }
        if (!strcmp(argv[1], "a") || !strcmp(argv[1], "agent") || !strcmp(argv[1], "agents")) {
            icd_command_load_agents(fd, argc, argv);
        }
        if (!strcmp(argv[1], "c") || !strcmp(argv[1], "conference") || !strcmp(argv[1], "conferences")) {
            icd_command_load_conferences(fd, argc, argv);
        }
    } else
        icd_command_help(fd, 2, help);

    return ICD_SUCCESS;
}

icd_status icd_command_load_app_icd(int fd, int argc, char **argv)
{
    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "APP_ICD Reload \n");

    reload_app_icd(APP_ICD);    /*implemenation in app_icd.c */

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

icd_status icd_command_load_conferences(int fd, int argc, char **argv)
{
    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "Conferences Reload \n");

    reload_app_icd(ICD_CONFERENCE);     /*implemenation in app_icd.c */

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

icd_status icd_command_load_agents(int fd, int argc, char **argv)
{
    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "Agents Reload \n");

    reload_app_icd(ICD_AGENT);  /*implemenation in app_icd.c */

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

icd_status icd_command_load_queues(int fd, int argc, char **argv)
{

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");
    cw_cli(fd, "Queue Reload \n");

    reload_app_icd(ICD_QUEUE);  /*implemenation in app_icd.c */

    cw_cli(fd, "\n");
    cli_line(fd, "=", 80);
    cw_cli(fd, "\n");

    return ICD_SUCCESS;
}

int icd_command_ack (int fd, int argc, char **argv)
{
  char * agent_id;
  icd_agent *agent = NULL;

  if(argc != 2) {
     	cw_cli(fd, "icd ack: Bad number of parameters\n");
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Ack\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
     	return -1;
  }
  agent_id = argv[1];   	   
  agent = (icd_agent *) icd_fieldset__get_value(agents, agent_id);
  if (!agent) {
        cw_cli(fd, "icd ack failed. Agent [%s] could not be found.\n", agent_id);        
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Ack\r\nResult: Fail\r\nCause: Agent not found\r\nCallerID: %s\r\n", agent_id);
		return -1;
  }		    
  if(icd_caller__get_state((icd_caller *) agent) == ICD_CALLER_STATE_READY ||
     icd_caller__get_state((icd_caller *) agent) == ICD_CALLER_STATE_DISTRIBUTING ||
     icd_caller__get_state((icd_caller *) agent) == ICD_CALLER_STATE_GET_CHANNELS_AND_BRIDGE) {
     	icd_caller__add_flag((icd_caller *)agent, ICD_ACK_EXTERN_FLAG);
   	 manager_event(EVENT_FLAG_USER, "icd_command",
       "Command: Ack\r\nResult: OK\r\nCallerID: %s\r\nState: %s\r\n", 
       agent_id, icd_caller__get_state_string((icd_caller *)agent));
     cw_cli(fd, "icd ack for agent[%s] - OK\n", agent_id);
     return 0;
  }
  cw_log(LOG_WARNING, "Function Ack failed, Agent [%s] is not in appropriate state [%s]\n", agent_id, icd_caller__get_state_string((icd_caller *) agent));
  manager_event(EVENT_FLAG_USER, "icd_command",
       "Command: Ack\r\nResult: Fail\r\nCause: Not correct state\r\nCallerID: %s\r\nState: %s\r\n", 
       agent_id, icd_caller__get_state_string((icd_caller *)agent));
  return -1;
}

int icd_command_hang_up (int fd, int argc, char **argv)
{
    icd_caller *agent = NULL;
    char *agent_id;
    
    if (argc != 2) {
        cw_cli(fd, "Function Hang up failed- bad number of parameters [%d]\n", argc);
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Hangup\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
        return -1;
    }
    agent_id = argv[1];   
    agent = (icd_caller *) icd_fieldset__get_value(agents, agent_id);
    if (!agent) {
        cw_cli(fd, "Function Hang up failed. Agent '%s' could not be found.\n", agent_id);        
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Hangup\r\nResult: Fail\r\nCause: Agent not found\r\nCallerID: %s\r\n", agent_id);
		return -1;
    }		    
    if(icd_caller__get_state(agent) != ICD_CALLER_STATE_BRIDGED &&
       icd_caller__get_state(agent) != ICD_CALLER_STATE_CONFERENCED){
       cw_cli(fd, "Function Hang up failed. Agent '%s' in state [%s].\n", agent_id,
		    icd_caller__get_state_string(agent));        
        manager_event(EVENT_FLAG_USER, "icd_command",
            "Command: Hangup\r\nResult: Fail\r\nCause: Not correct state\r\nCallerID: %s\r\nState: %s\r\n", 
            agent_id, icd_caller__get_state_string(agent));
		return -1;
    }    
    if(icd_caller__set_state(agent, ICD_CALLER_STATE_CALL_END)!=ICD_SUCCESS){    	     
       cw_cli(fd, "Function Hang up failed. Agent [%s] can not change state to CALL_END\n", agent_id);
       manager_event(EVENT_FLAG_USER, "icd_command",
            "Command: Hangup\r\nResult: Fail\r\nCause: State change to CALL_END failed\r\nCallerID: %s\r\nState: %s\r\n", 
            agent_id, icd_caller__get_state_string(agent));
    	return -1;
    }
    cw_log(LOG_NOTICE, "Function Hang up for agent [%s] executed OK.\n", agent_id);
    manager_event(EVENT_FLAG_USER, "icd_command",
        "Command: Hangup\r\nResult: OK\r\nCallerID: %s\r\n", 
         agent_id);
    return 0;
}

static void *icd_command_login_thread(void *arg) {
    icd_caller *agent = arg;
    struct cw_channel *chan;
    char *channelstring;
    char *agent_id;
    int res;
    char buf[200];
    char *passwd;
	
	channelstring = icd_caller__get_channel_string(agent);
	agent_id = icd_caller__get_caller_id(agent);
    chan = icd_caller__get_channel(agent);
    res = icd_bridge_dial_callweaver_channel(agent, channelstring, 20000);
    icd_caller__set_channel(agent, NULL);
    if (res != CW_CONTROL_ANSWER){
        cw_log(LOG_WARNING, "Login of agent [%s] failed - unable to get answer from channel [%s] .\n", agent_id, channelstring);
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: No channel answer\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nChannelString: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                channelstring);
/* More detailed check why there is no answer probably needed in the future. */	 
        cw_hangup(chan);
        icd_caller__del_param(agent, "LogInProgress");
	    return NULL;
    }	 
  	sprintf(buf, "agent=%s", agent_id);
  	passwd = icd_caller__get_param(agent, "login_password");
  	if (passwd)
  		sprintf(buf + strlen(buf), "|password=%s", passwd);  			
#if 0
/* TODO: fudged to compile */
    app_icd__agent_exec(chan, buf);
#endif
    icd_caller__del_param(agent, "LogInProgress");
    cw_log(LOG_NOTICE, "Agent login: External thread for Agent [%s] ending\n", agent_id);
/*    icd_bridge__safe_hangup(agent);*/
    chan = icd_caller__get_channel(agent);
    if(chan){
        icd_caller__stop_waiting(agent);
        cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
        cw_hangup(chan);
        icd_caller__set_channel(agent, NULL);
    }	
    return NULL;
}

int icd_command_login (int fd, int argc, char **argv)
{
    icd_caller *agent = NULL;
    char *agent_id;
    char *passwd=NULL;
    char *channelstring;
    int logFlag=1;
	pthread_t thread;
	pthread_attr_t attr;
	int result;

    if ((argc != 3) && (argc !=4)){
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
		return ICD_EGENERAL;
    }	 
    channelstring = argv[1];
    agent_id = argv[2];
    if (argc==4) passwd = argv[3];
      	 
    agent = (icd_caller *) icd_fieldset__get_value(agents, agent_id);    
    if (!agent) {
        cw_log(LOG_WARNING,
                    "AGENT LOGIN Fail!  Agent '%s' could not be found.\n"
                    "Please correct the 'agent' argument in the extensions.conf file\n", agent_id);        
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: Agent not found\r\nCallerID: %s\r\n", agent_id);
	    return ICD_EGENERAL;
    }       
    if (icd_caller__get_state(agent) != ICD_CALLER_STATE_SUSPEND &&
      icd_caller__get_state(agent) != ICD_CALLER_STATE_INITIALIZED) {
        cw_log(LOG_WARNING, "Login - Agent '%s' already logged in nothing to do\n", agent_id);        
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: Already logged\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nCallerState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
         	    return ICD_EGENERAL;
    }
	if(icd_caller__get_param(agent, "LogInProgress")){ 
	    cw_log(LOG_WARNING, "Login - Agent '%s' previous login try not finished yet.\n", agent_id);         
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: Previouv login try not finished\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nCallerState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
	    return ICD_EGENERAL;
	} 
    icd_caller__set_param(agent, "LogInProgress", &logFlag);
    icd_caller__set_channel_string(agent, channelstring);
    icd_caller__set_param_string(agent, "channel", channelstring);
    if(!icd_caller__create_channel(agent) ) {
        cw_log(LOG_WARNING,"Not avaliable channel [%s] \n", channelstring);
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Login\r\nResult: Fail\r\nCause: Channel not avaliable\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nCallerState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
        icd_caller__del_param(agent, "LogInProgress");
	    return ICD_EGENERAL;
    }
    if (passwd) {
    	icd_caller__set_param_string(agent, "login_password", passwd);
    }
    else{
    	icd_caller__del_param(agent, "login_password");
    }
	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = cw_pthread_create(&thread, &attr, icd_command_login_thread, agent);
	result = pthread_attr_destroy(&attr);	
    return 0;
}

int icd_command_logout (int fd, int argc, char **argv)
{
    icd_caller *agent = NULL;
    char *agent_id;
    char *passwd_to_check;
    char *passwd; 

    /* Identify agent just like app_icd__agent_exec, only this time we skip
       dynamically creating an agent. */
    if (argc != 3) {
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Logout\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
         return ICD_EGENERAL;
    }	 
    agent_id = argv[1];
    passwd_to_check = argv[2];
    agent = (icd_caller *) icd_fieldset__get_value(agents, agent_id);   
    if (agent == NULL) {
        cw_log(LOG_WARNING,
                    "LOGOUT FAILURE!  Agent '%s' could not be found.\n", agent_id);
          manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Logout\r\nResult: Fail\r\nCause: Agent not found\r\nCallerID: %s\r\n", agent_id);
		    
	        return ICD_EGENERAL;
    }
    passwd = icd_caller__get_param(agent, "passwd");
    if (passwd) 
          if(strcmp(passwd, passwd_to_check)){
          cw_log(LOG_WARNING,
                 "LOGOUT FAILURE! Wrong password for Agent '%s'.\n", agent_id);
          manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Logout\r\nResult: Fail\r\nCause: Wrong password\r\nID: %d\r\nCallerID: %s\r\nAgentName: %s\r\nAgentState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
          return ICD_EGENERAL;
    }     
    
    cw_log(LOG_NOTICE, "Agent [%s] (found in registry) will be logged out.\n", agent_id);
    /* TBD - Implement state change to ICD_CALLER_STATE_WAIT. We can't just pause the thread
     * because the caller's members would still be in the distributors. We need to go into a
     * caller state that is actually different, a paused/waiting/down state.
     */
    
     if (icd_caller__set_state(agent, ICD_CALLER_STATE_SUSPEND)  != ICD_SUCCESS){
        cw_log(LOG_WARNING,
                    "LOGOUT FAILURE!  Agent [%s] vetoed or ivalid state change, state [%s].\n", agent_id,icd_caller__get_state_string(agent));
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Logout\r\nResult: Fail\r\nCause: Unable to change state\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nCallerState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
	    return ICD_EGENERAL;
	} 
	else {	    
        cw_log(LOG_WARNING, "LOGOUT OK!  Agent [%s] logged out.\n", agent_id);
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Logout\r\nResult: OK\r\nID: %d\r\nCallerID: %s\r\nCallerName: %s\r\nCallerState: %s\r\n",
                icd_caller__get_id(agent), icd_caller__get_caller_id(agent), icd_caller__get_name(agent),
                icd_caller__get_state_string(agent));
        
        return ICD_SUCCESS;
	}
    return ICD_EGENERAL;
}

int icd_command_hangup_channel (int fd, int argc, char **argv)
{
   char *chan_name;
   struct cw_channel *chan;

   if (argc != 2) {
       cw_cli(fd, "Function Hang up channel failed - bad number of parameters [%d]\n", argc);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: HangupChannel\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
	   return -1;
    }
   chan_name = argv[1];
   chan = cw_get_channel_by_name_locked(chan_name);
   if (chan == NULL) {
       cw_cli(fd, "Function Hang up channel failed - channel not found [%s]\n", chan_name);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: HangupChannel\r\nResult: Fail\r\nCause: Channel not found\r\nChannel : %s\r\n", chan_name);
	   return -1;
   }
   cw_mutex_unlock (&chan->lock);
   cw_softhangup(chan, CW_SOFTHANGUP_EXPLICIT);
   cw_cli(fd, "Function Hang Up succeed - channel[%s]\n", chan_name);
   manager_event(EVENT_FLAG_USER, "icd_command",
      "Command: HangupChannel\r\nResult: OK\r\nChannel : %s\r\n", chan_name);
   return 0;
}



int icd_command_playback_channel (int fd, int argc, char **argv)
{
   icd_agent * agent; 
   char * agent_id;
   icd_conference * conf;
   int len;
   int res;
   unsigned char * data;
   char * key;


   if (argc != 3){
       cw_cli(fd, "Function Playback_chan (play_dtmf) failed - bad number of parameters [%d]\n", argc);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: PlaybackChannel\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
       return -1;
    }


   agent_id = argv[1];
   agent = (icd_agent *) icd_fieldset__get_value(agents, agent_id);

    key = argv[2];

   if (agent == NULL) {
       cw_cli(fd, "Function Playback_chan (play_dtmf) failed - agent not found [%s]\n", agent_id);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: PlaybackChannel\r\nResult: Fail\r\nCause: Channel not found\r\nChannel : %s\r\n", agent_id);
       return -1;
   }
   
   
    switch (key[0]){
        case '0' : 
            data = zero;
            len = sizeof(zero);
            break;
        case '1' :
            data = one;
            len = sizeof(one);
            break;
        case '2' :
            data = two;
            len = sizeof(two);
            break;
        case '3' :
            data = three;
            len = sizeof(three);
            break;
        case '4' :
            data = four;
            len = sizeof(four);
            break;
        case '5' :
            data = five;
            len = sizeof(five);
            break;
        case '6' :
            data = six;
            len = sizeof(six);
            break;
        case '7' :
            data = seven;
            len = sizeof(seven);
            break;
        case '8' :
            data = eight;
            len = sizeof(eight);
            break;
        case '9' :
            data = nine;
            len = sizeof(nine);
            break;
        case '*' :
            data = star;
            len = sizeof(star);
            break;
        case '#' :
            data = hash;
            len = sizeof(hash);
            break;

        default:
            return -1;
    }

        conf = ((icd_caller *)agent)->conference;

        if (!conf) {
            cw_cli(fd, "Function Playback_chan (play_dtmf) failed - agent conference not found [%s]\n", agent_id);
            manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: PlaybackChannel\r\nResult: Fail\r\nCause: Agent conference not found\r\nAgent : %s\r\n", agent_id);
            return -1;
        }


        while (len) {
    	    res = write(conf->fd, data, len);

    	    if (res < 1) {      
                cw_log(LOG_WARNING, "Failed to write audio data to conference: \n");
                return 0;
    	    }
    	    len -= res;
    	    data += res;
        }

    cw_cli(fd, "Function Playback succeed - agent[%s]\n", agent_id);
    manager_event(EVENT_FLAG_USER, "icd_command",
      "Command: PlaybackChannel (play_dtmf)\r\nResult: OK\r\nAgent : %s\r\n", agent_id);
    return 0;
}




/*
params:
argv[0] = record
argv[1] = start or stop
argv[2] = customer token 
argv[3] = if start - directory & file name. %D -day, %M - minute, %S - second. To this failename wil be added
	  token and .WAV. Example: 
argv[3] = /tmp/%D/%m/  fliename is: /tmp/29/59/callweaver123123423423454.WAV	                 
*/

int icd_command_record(int fd, int argc, char **argv)
{
  icd_caller * customer;
  char rec_directory_buf[200];
  char rec_format_buf[50]="";
  char *rec_format;
  char buf[300];
  char cust_buf[40];
  char *RecFile;
  cw_channel * chan;
  char * customer_source;
  struct tm *ptr;
  time_t tm;
  int record_start = -1;

  if (argc != 3  && argc != 4 ) {
       cw_cli(fd, "Function record bad no of parameters [%d]\n", argc);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
       return 1;
   }    
   
   if (!strcasecmp(argv[1], "start"))
        record_start = 1;
   if (!strcasecmp(argv[1],"stop"))
        record_start = 0;
   if (record_start == -1) {
       cw_cli(fd, "Function record first parameter [%s] start/stop allowed\n", argv[1]);
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nResult: Fail\r\nCause: Wrong parameters\r\n");
       return 1;
   }    
	
   if (record_start) {
        strcpy(buf,"MuxMon start ");
   }
   else {
        strcpy(buf,"MuxMon stop ");
   }	 
   customer_source = argv[2]; 
   cw_mutex_lock(&customers_lock);
   customer = (icd_caller *) icd_fieldset__get_value(customers, customer_source);
   cw_mutex_unlock(&customers_lock);

   if (customer == NULL) {
            cw_cli(fd, "Record FAILURE! Customer [%s] not found\n", customer_source);
       		manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nResult: Fail\r\nCause: Wrong customer ID\r\nCallerID: %s\r\n", 
                customer_source);
	        return 1;
   }
   chan = icd_caller__get_channel(customer);
   if (chan == NULL) {
            cw_cli(fd, "Record FAILURE! Channel for customer [%s] not found\n", customer_source);
       		manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nResult: Fail\r\nCause: No customer channel\r\nCallerID: %s\r\n", 
                customer_source);
	        return 1;
   }
   if (chan->name == NULL) {
            cw_cli(fd,  "Record FAILURE! Channel name for customer [%s] not found\n", customer_source);
       		manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nResult: Fail\r\nCause: No customer channel name\r\nCallerID: %s\r\n", 
                customer_source);
	        return 1;
   }
   strncpy(buf + strlen(buf), chan->name, sizeof(buf) - strlen(buf));
   if (!record_start){
   		cw_log(LOG_NOTICE, "Stop of recording for customer [%s] \n", customer_source);
    	manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nSubCommand: Stop\r\nResult: OK\r\nCallerID: %s\r\n", 
                customer_source);
    	manager_event(EVENT_FLAG_USER, "icd_event",
                "Event: RecordStop\r\nCallerID: %s\r\n", 
                customer_source);
        fd = fileno(stderr);
        cw_cli_command(fd, buf);
        return 0;
   }
   if(chan->spiers !=NULL){
   	cw_log(LOG_NOTICE, "Start of recording for customer [%s] failed - already recording \n", customer_source);
        manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nSubCommand: Start\r\nResult: Fail\r\nCause: Already recording\r\nCallerID: %s\r\n", 
                customer_source);
	return 1;
   }
   strcpy(buf + strlen(buf), " ");
   strncpy(rec_directory_buf, argv[3],sizeof(rec_directory_buf)-1);
   if((rec_format=strchr(rec_directory_buf,'.'))){
      *rec_format='\0';
      rec_format++;
      rec_format_buf[0]='.';
      strncpy(rec_format_buf+1, rec_format, sizeof(rec_format_buf)-2);
   }   
   tm = time(NULL);
   ptr = localtime(&tm);
   int pos; char c;
   cust_buf[0] = '\0';
   for(pos=0;(pos < strlen(customer_source)) && (pos < sizeof(cust_buf)-1);pos++){
   	  c = customer_source[pos];
      cust_buf[pos] = ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ? c : '_';
   	  cust_buf[pos + 1] = '\0';
   }
   RecFile = buf + strlen(buf);
   strftime(buf + strlen(buf), sizeof(buf) - strlen(buf), rec_directory_buf, ptr);
   strncpy(buf + strlen(buf),  cust_buf, sizeof(buf) - strlen(buf)-1);
   strncpy(buf + strlen(buf),  rec_format_buf, sizeof(buf) - strlen(buf)-1);
 
//   muxmon <start|stop> <chan_name> <args>cw_cli_command(fd, command);fd can be like fileno(stderr)
   fd = fileno(stderr);
   cw_cli_command(fd, buf);
   cw_log(LOG_NOTICE, "Start of recording for customer [%s] \n", customer_source);
   manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Record\r\nSubCommand: Start\r\nResult: OK\r\nCallerID: %s\r\nFileName: %s\r\n", 
                customer_source, RecFile);
   manager_event(EVENT_FLAG_USER, "icd_event",
                "Command: RecordStart\r\nCallerID: %s\r\nFileName: %s\r\n", 
                customer_source, RecFile);

   return 0;
}
/* 
 	params: 
 	argv[0] = queue 
 	513	argv[1] = agent id 
 	514	argv[2] = queue name to which agent will be joined 
 	515	argv[3] = nothing or R to remove from queue, if argv[2]=all remove from all queues 
 	516	*/ 
 
int icd_command_join_queue (int fd, int argc, char **argv) 
{ 
	    icd_caller *agent = NULL; 
	    char *agent_id; 
	    char *queuename; 
	    int remove=0; 
	    icd_queue *queue; 
	    icd_member *member; 
	 
	     
	    if ((argc != 3) && (argc !=4)) { 
       		cw_cli(fd, "icd queue FAILURE! bad parameters\n");
       		manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Queue\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
  	         return 1;
	    }     
	    agent_id = argv[1]; 
	    queuename = argv[2]; 
	    agent = (icd_caller *) icd_fieldset__get_value(agents, agent_id);    
	    if (agent == NULL) { 
            cw_cli(fd, "icd queue FAILURE! Agent [%s] not found\n", agent_id);
            manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Queue\r\nResult: Fail\r\nCause: Agent not found\r\nCallerID: %s\r\n", agent_id);
	        return 1;
	    } 
	    if (argc==4) 
	      if(!strcasecmp(argv[3],"R")) { 
	        remove = 1; 
	    } 
	    queue = NULL;  
	    if(!remove || strcasecmp(queuename,"all")){          
	      queue = (icd_queue *) icd_fieldset__get_value(queues, queuename); 
	      if (queue == NULL) { 
            	cw_cli(fd,"icd queue FAILURE! Queue not found[%s], Agent [%s]\n", queuename, agent_id);
            	manager_event(EVENT_FLAG_USER, "icd_command",
                	"Command: Queue\r\nResult: Fail\r\nCause: Queue not found\r\nCallerID: %s\r\nQueue: %s\r\n", agent_id, queuename);
	        	return 1;
	      } 
	    }  	 
	    cw_log(LOG_NOTICE, "Agent [%s] will %s the queue [%s]\n", agent_id, remove?"leave":"join", queuename); 
            if(!remove){ 
              icd_list__lock((icd_list *) (agent->memberships));
 	        if(icd_caller__add_to_queue(agent, queue) == ICD_SUCCESS){
		   if(icd_caller__get_state(agent) ==  ICD_CALLER_STATE_READY){ 
	           	member = icd_member_list__get_for_queue(agent->memberships, queue); 
	           	if(member){ 
	                	icd_queue__agent_distribute(queue, member); 
	           	}
		   }		   		 
                  cw_cli(fd,"icd queue OK! Agent[%s] added to queue[%s]\n", agent_id, queuename);
                   manager_event(EVENT_FLAG_USER, "icd_command",
               	    "Command: Queue\r\nSubCommand: Add\r\nResult: OK\r\nCallerID: %s\r\nQueue: %s\r\n", 
                  agent_id, queuename);
		  }
                icd_list__unlock((icd_list *) (agent->memberships));
	    }
	    else { 
              icd_list__lock((icd_list *) (agent->memberships));
	      if(queue){ 
	       if(agent->memberships) 
	           if((member = icd_member_list__get_for_queue(agent->memberships, queue))){ 
	                if(icd_caller__get_active_member(agent) == member){ 
	                     icd_caller__set_active_member (agent, NULL); 
	                }   
	           icd_caller__remove_from_queue(agent, queue); 
            	   cw_cli(fd,"icd queue OK! Agent[%s] removed from queue[%s]\n", agent_id, queuename);
            	   manager_event(EVENT_FLAG_USER, "icd_command",
                   "Command: Queue\r\nSubCommand: Remove\r\nResult: OK\r\nCallerID: %s\r\nQueue: %s\r\n", 
                  agent_id, queuename);
	         }    
 	      } 
	      else {
               icd_caller__set_active_member (agent, NULL); 
	       icd_caller__remove_from_all_queues(agent); 
      	       cw_cli(fd,"icd queue OK! Agent[%s] removed from all queues\n", agent_id);
                manager_event(EVENT_FLAG_USER, "icd_command",
              	  "Command: Queue\r\nSubCommand: RemoveAll\r\nResult: OK\r\nCallerID: %s\r\n", 
               	  agent_id);
	    }
            icd_list__unlock((icd_list *) (agent->memberships));
	}        
     return 0;
	 
} 


int icd_command_control_playback(int fd, int argc, char **argv) {

    icd_agent * agent;
    icd_caller * associated_caller;
    char * agent_id;
    icd_conference * conf;
    char * key;
    struct cw_frame write_frame;
    int count;
    int i;

    if (argc < 3){
        cw_cli(fd, "Function control_playback failed - bad number of parameters [%d]\n", argc);
        manager_event(EVENT_FLAG_USER, "icd_command",
            "Command: control_playback\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
        return -1;
    }


    agent_id = argv[1];
    agent = (icd_agent *) icd_fieldset__get_value(agents, agent_id);

    key = argv[2];
    
    if (argc > 3) {
        count = atoi(argv[3]);
    } else {
        count = 1;
    }

    if (agent == NULL) {
        cw_cli(fd, "Function control_playback failed - agent not found [%s]\n", agent_id);
        manager_event(EVENT_FLAG_USER, "icd_command",
            "Command: control_playback\r\nResult: Fail\r\nCause: Agent not found\r\nAgent : %s\r\n", agent_id);
        return -1;
    }

    conf = ((icd_caller *)agent)->conference;

    if (!conf) {
        cw_cli(fd, "Function control_playback failed - agent conference not found [%s]\n", agent_id);
        manager_event(EVENT_FLAG_USER, "icd_command",
            "Command: control_playback\r\nResult: Fail\r\nCause: Agent conference not found\r\nAgent : %s\r\n", agent_id);
        return -1;
    }

    associated_caller = (icd_caller *) icd_list__peek((icd_list *) ((icd_caller*)agent)->associations);
    
    if (!associated_caller){
        cw_cli(fd, "Associated caller not found for agent [%s]\n", agent_id);
        return -1;
    }

    if (!associated_caller->chan){
        cw_cli(fd, "Associated caller channel not found for agent [%s]\n", agent_id);
        return -1;
    }

    cw_fr_init_ex(&write_frame, CW_FRAME_DTMF, *key, NULL);
    write_frame.offset = 76;

    for (i = 0;  i < count;  i++)
    {
        if (cw_write(associated_caller->chan, &write_frame) < 0) {
            cw_log(LOG_WARNING, "Unable to write frame to channel: %s\n", "strerror(errno)");
        }
    }

    cw_cli(fd, "Function control_playback succeed - agent[%s]\n", agent_id);
    manager_event(EVENT_FLAG_USER, "icd_command",
        "Command: control_playback\r\nResult: OK\r\nAgent : %s\r\n", agent_id);
    return 0;

}


int icd_command_transfer (int fd, int argc, char **argv)
{
  icd_caller *customer;
  struct cw_channel *chan = NULL;
  char *customer_source;
  int pri=0;
  char *pria, *exten, *context;

  if (argc != 3) {
       cw_cli(fd, "Transfer FAILURE! bad parameters\n");
       manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Transfer\r\nResult: Fail\r\nCause: Wrong parameters number\r\n");
       return 1;
   }    
   customer_source = argv[1];
   cw_mutex_lock(&customers_lock);
   customer = (icd_caller *) icd_fieldset__get_value(customers, customer_source);
   cw_mutex_unlock(&customers_lock);
   if (customer == NULL) {
            cw_cli(fd,"Transfer FAILURE! Customer [%s] not found\n", customer_source);
            manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Transfer\r\nResult: Fail\r\nCause: Customer not found\r\nCallerID: %s\r\n", customer_source);
	        return 1;
   }
   	exten = cw_strdupa(argv[2]);
	if((context = strchr(exten,'@'))) {
		*context = 0;
		context++;
		if(!(context && exten)) {
			cw_cli(fd,"Transfer failure, customer[%s] : no context\n", customer_source);
            manager_event(EVENT_FLAG_USER, "icd_command",
                "Command: Transfer\r\nResult: Fail\r\nCause: Wrong extension@context\r\nCallerID: %s\r\n", customer_source);
			return 1;
		}
		if((pria = strchr(context,':'))) {
			*pria = '\0';
			pria++;
			pri = atoi(pria);
		} 
		if(!pri)
			pri = 1;
	}
	else {		
		cw_cli(fd,"Transfer failure, customer[%s] : no context\n", customer_source);
        manager_event(EVENT_FLAG_USER, "icd_command",
        	"Command: Transfer\r\nResult: Fail\r\nCause: Wrong extension@context\r\nCallerID: %s\r\n", customer_source);
		return 1;
	}
	chan = icd_caller__get_channel(customer);
    if(!chan){
		cw_cli(fd,"Transfer failure, customer[%s] : no channel\n", customer_source);
    	manager_event(EVENT_FLAG_USER, "icd_command",
    	"Command: Transfer\r\nResult: Fail\r\nCause: No channel\r\nCallerID: %s\r\nContext: %s\r\nExtension: %s\r\nPriority: %d\r\n", 
        customer_source, context, exten, pri);
    }	 	
    if(!cw_exists_extension(chan, context, exten, pri, NULL)){
		cw_cli(fd,"Transfer failure, customer[%s] : not correct context-extension\n", customer_source);
    	manager_event(EVENT_FLAG_USER, "icd_command",
    	"Command: Transfer\r\nResult: Fail\r\nCause: Destination not exist\r\nCallerID: %s\r\nContext: %s\r\nExtension: %s\r\nPriority: %d\r\n", 
        customer_source, context, exten, pri);
        return 1;
    }	 	
	if(cw_goto_if_exists(chan, context, exten, pri)){
		cw_cli(fd,"Transfer failed customer[%s] to context[%s], extension[%s], priority [%d]\n", customer_source, context, exten, pri);
    	manager_event(EVENT_FLAG_USER, "icd_command",
    	"Command: Transfer\r\nResult: Fail\r\nCause: Unknown\r\nCallerID: %s\r\nContext: %s\r\nExtension: %s\r\nPriority: %d\r\n", 
        customer_source, context, exten, pri);
	    return 1;	   
	};
 	if(icd_caller__set_state(customer, ICD_CALLER_STATE_CALL_END) != ICD_SUCCESS){
		cw_cli(fd,"Transfer failed customer[%s] to context[%s], extension[%s], priority [%d]\n", customer_source, context, exten, pri);
    	manager_event(EVENT_FLAG_USER, "icd_command",
    	"Command: Transfer\r\nResult: Fail\r\nCause: Unable to change state to CALL_END\r\nCallerID: %s\r\nState: %s\r\nContext: %s\r\nExtension: %s\r\nPriority: %d\r\n", 
        customer_source, icd_caller__get_state_string(customer), context, exten, pri);
		return 1;
 	}; 
    manager_event(EVENT_FLAG_USER, "icd_command",
    	"Command: Transfer\r\nResult: OK\r\nCallerID: %s\r\nContext: %s\r\nExtension: %s\r\nPriority: %d\r\n", 
        customer_source, context, exten, pri);
    return 0;
}

void icd_manager_send_message( char *format, ...)
{
   va_list args;
   char message[1024];
   
   va_start(args, format);
   vsnprintf(message, sizeof(message)-1, format, args);
   va_end(args);	
   manager_event(EVENT_FLAG_USER, "icd_message",
                "Message: %s\r\n", message);
   
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

