/*
 * app_nconference
 *
 * NConference
 * A channel independent conference application for CallWeaver
 *
 * Copyright (C) 2002, 2003 Navynet SRL
 * http://www.navynet.it
 *
 * Massimo "CtRiX" Cetra - ctrix (at) navynet.it
 *
 * This program may be modified and distributed under the 
 * terms of the GNU Public License V2.
 *
 */

extern cw_mutex_t conflist_lock;

extern struct cw_conference *conflist;
extern int conference_count;


enum conf_actions {
    CONF_ACTION_HANGUP,
    CONF_ACTION_ENABLE_SOUNDS,
    CONF_ACTION_MUTE_ALL,
    CONF_ACTION_QUEUE_SOUND,
    CONF_ACTION_QUEUE_NUMBER,
    CONF_ACTION_PLAYMOH
};

struct cw_conf_command_queue 
{
    // The command issued
    int  command;

    // An optional integer  value passed 
    int  param_number;

    // An optional string value passed
    char param_text[80];

    // The member who ordered this command
    struct cw_conf_member *issuedby;

    // The next command in the list
    struct cw_conf_command_queue *next;
};

struct cw_conference 
{
	// conference name
	char name[128]; 
	// The pin to gain conference admin priileges	
	char pin[20];
	// Auto destroy when empty
	short auto_destroy;
	// Is it locked ? (No more members)
	short is_locked;

	//Command queue to be executed on all members
	struct cw_conf_command_queue *command_queue;
	// single-linked list of members in conference
	struct cw_conf_member* memberlist ;
	int membercount ;
	
	// conference thread id
	pthread_t conference_thread ;
	// conference data mutex
	cw_mutex_t lock ;
	
	// pointer to next conference in single-linked list
	struct cw_conference* next ;

} ;

void init_conference( void ) ;

int add_command_to_queue ( 
	struct cw_conference *conf, struct cw_conf_member *member ,
	int command, int param_number, char *param_text 
	) ;


struct cw_conference* start_conference( struct cw_conf_member* member ) ;
void remove_conf( struct cw_conference *conf );
struct cw_conf_member *find_member( struct cw_conference *conf, const char* name ) ;
struct cw_conference *find_conf( const char* name );

int conference_parse_admin_command(struct cw_conf_member *member);

