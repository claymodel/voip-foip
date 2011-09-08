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

extern struct cw_generator membergen;


enum member_types {
    MEMBERTYPE_MASTER,
    MEMBERTYPE_SPEAKER,
    MEMBERTYPE_LISTENER,
    MEMBERTYPE_TALKER,
    MEMBERTYPE_CONSULTANT
};

struct cw_conf_member 
{
	cw_mutex_t lock ; 		// member data mutex
	
	struct cw_channel* chan ; 	// member's channel  
	char* channel_name ; 		// member's channel name
	struct cw_callerid cid;		// member's channel cid
	struct cw_conference *conf;	// reference to the conference the member is in

	// pointer to next member in single-linked list	
	struct cw_conf_member *next ;

	//User number
	//int user_no;
	// flag indicating we should remove this member
	short remove_flag ;
	short force_remove_flag ;
	// flagindicating this memberis ready to be used. (usefull?)
	short active_flag;
	// flag indicating that the user is speaking
	short is_speaking;
	// VAD is enable for this member ? 
	int enable_vad;
	// VAD is enable for this member ? 
	int enable_vad_allowed;
	// Don't play enter/leave messages
	int quiet_mode;
	// on hold with MOH enabled
	int beep_only_mode;
	// Dont play calleridnum. Only beep to announce if a member join/left
	int is_on_hold;
	// Master said to put me on hold
	int force_on_hold;
	int skip_moh_when_alone;

	// flag indicating the actual volume
	//int   listen_volume;	  // Not used - The phone should have it !!
	int   talk_volume;
	int   talk_volume_adjust; // we have to adjust the volume on the frames. Channel driver does not support it.
	// flag indicating if we want to mute the member
	int   talk_mute;

	// Don't play any sound for this member
	int   dont_play_any_sound;

	// DTMF MAGICS AND HANDLING
	// Detect DTMF Tones
	short manage_dtmf;
	short dtmf_admin_mode;
	short dtmf_long_insert;
	char dtmf_buffer[64];

	// start time
	struct timeval time_entered ;

	// frame informations
	long framelen;		// frame length in milliseconds
	int  datalen;		// frame length in milliseconds
	int  samples;		// number of samples in framelen
	long samplefreq;	// calculated sample frequency

	int lostframecount;

	// Circular buffer pointer
	struct member_cbuffer *cbuf;
	// Output frame buffer
	short framedata[2048];

	// values passed to create_member () via *data
	enum member_types type ;	// L = ListenOnly, M = Moderator, S = Standard (Listen/Talk)
	char* id ;			// member id
	char* flags ;			// raw member-type flags
	char* pin ;			// raw member-type flags

	// audio format this member is using
	int write_format ;
	int read_format ;

	// input/output smoother
	struct cw_smoother *inSmoother;
	int smooth_size_in;
	int smooth_size_out;

	// number of frames to ignore speex_preprocess()
	int skip_voice_detection;
	int silence_nr;

	// To play sounds
	struct cw_conf_soundq *soundq;

	struct cw_trans_pvt* to_slinear ;
	struct cw_trans_pvt* from_slinear ;

	// THOSE ARE PASSED TO THE CONFERENCE
	short auto_destroy;
} ;


void send_state_change_notifications( struct cw_conf_member* member );
int member_exec( struct cw_channel* chan, int argc, char **argv );
struct cw_conf_member* create_member( struct cw_channel *chan, int argc, char **argv ) ;
struct cw_conf_member* delete_member( struct cw_conf_member* member ) ;
int cw_conf_member_genactivate( struct cw_conf_member *member ) ;
char *membertypetostring  (int member_type);

