#ifndef __CHAN_SCCP_H
#define __CHAN_SCCP_H

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "callweaver/acl.h"
#include "callweaver/module.h"
#include "callweaver/rtp.h"
#include "callweaver/options.h"
#include "callweaver/logger.h"
#include "callweaver/config.h"

#define SCCP_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */
#define SCCP_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */

#if defined( __alpha__ ) || defined( __alpha ) || defined( i386 )       ||   \
    defined( __i386__ )  || defined( _M_I86 )  || defined( _M_IX86 )    ||   \
    defined( __OS2__ )   || defined( sun386 )  || defined( __TURBOC__ ) ||   \
    defined( vax )       || defined( vms )     || defined( VMS )        ||   \
    defined( __VMS )

#define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN

#endif

#if defined( AMIGA )    || defined( applec )  || defined( __AS400__ )  ||   \
    defined( _CRAY )    || defined( __hppa )  || defined( __hp9000 )   ||   \
    defined( ibm370 )   || defined( mc68000 ) || defined( m68k )       ||   \
    defined( __MRC__ )  || defined( __MVS__ ) || defined( __MWERKS__ ) ||   \
    defined( sparc )    || defined( __sparc)  || defined( SYMANTEC_C ) ||   \
    defined( __TANDEM ) || defined( THINK_C ) || defined( __VMCMS__ )

#define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN

#endif

/*  if the platform is still not known, try to find its byte order  */
/*  from commonly used definitions in the headers included earlier  */

#if !defined(SCCP_PLATFORM_BYTE_ORDER)

#if defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN)
#  if    defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif !defined(LITTLE_ENDIAN) &&  defined(BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  elif defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  endif


#elif defined(_LITTLE_ENDIAN) || defined(_BIG_ENDIAN)
#  if    defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif !defined(_LITTLE_ENDIAN) &&  defined(_BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  elif defined(_BYTE_ORDER) && (_BYTE_ORDER == _LITTLE_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif defined(_BYTE_ORDER) && (_BYTE_ORDER == _BIG_ENDIAN)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  endif

#elif defined(__LITTLE_ENDIAN__) || defined(__BIG_ENDIAN__)
#  if    defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif !defined(__LITTLE_ENDIAN__) &&  defined(__BIG_ENDIAN__)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __LITTLE_ENDIAN__)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN
#  elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __BIG_ENDIAN__)
#    define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN
#  endif
#endif

#endif

/* #define SCCP_PLATFORM_BYTE_ORDER SCCP_LITTLE_ENDIAN */
/* #define SCCP_PLATFORM_BYTE_ORDER SCCP_BIG_ENDIAN */

#if !defined(SCCP_PLATFORM_BYTE_ORDER)
#error Please edit chan_sccp.h (line 94 or 95) to set the platform byte order
#endif

#if SCCP_PLATFORM_BYTE_ORDER == SCCP_LITTLE_ENDIAN
#define letohl(x) (x)
#define letohs(x) (x)
#define htolel(x) (x)
#define htoles(x) (x)
#else
static inline unsigned short bswap_16(unsigned short x) {
  return (x>>8) | (x<<8);
}

static inline unsigned int bswap_32(unsigned int x) {
  return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}
/*
static inline unsigned long long bswap_64(unsigned long long x) {
  return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}
*/
#define letohl(x) bswap_32(x)
#define letohs(x) bswap_16(x)
#define htolel(x) bswap_32(x)
#define htoles(x) bswap_16(x)
#endif

#define SCCP_VERSION "20051217"

/* I don't like the -1 returned value */
#define sccp_true(x) (cw_true(x) ? 1 : 0)
#define sccp_log(x) if (sccp_globals->debug >= x) cw_verbose
#define GLOB(x) sccp_globals->x

#define DEV_ID_LOG(x) x ? x->id : "SCCP"

#define CS_CW_CHANNEL_PVT(x) x->tech_pvt

#define CS_CW_BRIDGED_CHANNEL(x) cw_bridged_channel(x)

/*
#ifndef CS_CW_HAS_CW_GROUP_T
typedef unsigned int cw_group_t;
#endif
*/

typedef struct sccp_channel		sccp_channel_t;
typedef struct sccp_session		sccp_session_t;
typedef struct sccp_line		sccp_line_t;
typedef struct sccp_speed		sccp_speed_t;
typedef struct sccp_device		sccp_device_t;
typedef struct sccp_hint		sccp_hint_t;
typedef struct sccp_hostname	sccp_hostname_t;

typedef void sk_func (sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);

#include "sccp_protocol.h"

struct sccp_hostname {
	char name[MAXHOSTNAMELEN];
	sccp_hostname_t *next;
};

struct sccp_hint {
	int hintid;
	sccp_device_t  *device;
	uint8_t	instance;
	char context[CW_MAX_CONTEXT];
	char exten[CW_MAX_EXTENSION];
	sccp_hint_t *next;
};

/* A line is a the equiv of a 'phone line' going to the phone. */
struct sccp_line {

	/* lockmeupandtiemedown */
	cw_mutex_t lock;

	/* This line's ID, used for logging into (for mobility) */
	char id[4];

	/* PIN number for mobility/roaming. */
	char pin[8];

	/* The lines position/instanceId on the current device*/
	uint8_t instance;

	/* the name of the line, so use in callweaver (i.e SCCP/<name>) */
	char name[80];

	/* A description for the line, displayed on in header (on7960/40)
	* or on main  screen on 7910 */
	char description[StationMaxNameSize];

	/* A name for the line, displayed next to the button (7960/40). */
	char label[StationMaxNameSize];

	/* mainbox numbers (seperated by commas) to check for messages */
	char mailbox[CW_MAX_EXTENSION];

	/* Voicemail number to dial */
	char vmnum[CW_MAX_EXTENSION];

	/* The context we use for outgoing calls. */
	char context[CW_MAX_CONTEXT];
	char language[MAX_LANGUAGE];
	char accountcode[CW_MAX_ACCOUNT_CODE];
	char musicclass[MAX_MUSICCLASS];
	int						amaflags;
	cw_group_t				callgroup;
#ifdef CS_SCCP_PICKUP
	cw_group_t				pickupgroup;
#endif
	/* CallerId to use on outgoing calls*/
	char cid_name[CW_MAX_EXTENSION];
	char cid_num[CW_MAX_EXTENSION];

	/* max incoming calls limit */
	uint8_t incominglimit;

	/* rtp stream tos */
	uint32_t	rtptos;

	/* The currently active channel. */
	/*  sccp_channel_t * activeChannel; */

	/* Linked list of current channels for this line */
	sccp_channel_t * channels;

	/* Number of currently active channels */
	uint8_t channelCount;

	/* Prev and Next line in the global list */
	sccp_line_t * next, * prev;

	/* Prev and Next line on the current device. */
	sccp_line_t * next_on_device, * prev_on_device;

	/* The device this line is currently registered to. */
	sccp_device_t * device;

	/* list of hint pointers. Internal lines to notify the state */
	sccp_hint_t * hints;

	/* call forward SCCP_CFWD_ALL or SCCP_CFWD_BUSY */
	uint8_t cfwd_type;
	char * cfwd_num;

	/* transfer to voicemail softkey. Basically a call forward */
	char * trnsfvm;

	/* secondary dialtone */
	char secondary_dialtone_digits[10];
	uint8_t	secondary_dialtone_tone;

	unsigned int			mwilight:1;

	/* echocancel phone support */
	unsigned int			echocancel:1;
	unsigned int			silencesuppression:1;
	unsigned int			transfer:1;
	unsigned int			spareBit4:1;
	unsigned int			spareBit5:1;
	unsigned int			spareBit6:1;
};

/* This defines a speed dial button */
struct sccp_speed {
	/* The instance of the speeddial in the sccp.conf */
	uint8_t config_instance;
	/* The instance on the current device */
	uint8_t instance;
	/* SKINNY_BUTTONTYPE_SPEEDDIAL or SKINNY_BUTTONTYPE_LINE (hint) */
	uint8_t type;

	/* The name of the speed dial button */
	char name[StationMaxNameSize];

	/* The number to dial when it's hit */
	char ext[CW_MAX_EXTENSION];
	char hint[CW_MAX_EXTENSION];

	/* Pointer to next speed dial */
	sccp_speed_t * next;
};


struct sccp_device {
	cw_mutex_t lock;

	/* SEP<macAddress> of the device. */
	char id[StationMaxDeviceNameSize];

	/* internal description. Skinny protocol does not use it */
	char description[40];

	/* lines to auto login this device to */
	char autologin[SCCP_MAX_AUTOLOGIN];

	/* model of this phone used for setting up features/softkeys/buttons etc. */
	char config_type[10];

	/* model of this phone sent by the station, devicetype*/
	uint32_t skinny_type;

	/* timezone offset */
	int tz_offset;

	/* version to send to the phone */
	char imageversion[StationMaxVersionSize];

	/* If the device has been rully registered yet */
	uint8_t registrationState;

	/* callweaver codec device preference */
	struct cw_codec_pref codecs;

	/* SCCP_DEVICE_ONHOOK or SCCP_DEVICE_OFFHOOK */
	uint8_t state;

	/* ringer mode. Need it for the ringback */
	uint8_t ringermode;

	/* dnd mode: see SCCP_DNDMODE_* */
	uint8_t dndmode;

	/* last dialed number */
	char lastNumber[CW_MAX_EXTENSION];

	/* callweaver codec capability */
	int capability;

	/* channel state where to open the rtp media stream */
	uint8_t earlyrtp;

	/* Number of currently active channels */
	uint8_t channelCount;
	
	/* skinny protocol version */
	uint8_t protocolversion;

/*	btnlist					*btntemplate; XXX remove */

	/* permit or deny connections to the main socket */
	struct cw_ha			*ha;
	/* permit registration to the hostname ip address */
	sccp_hostname_t			*permithost;

	unsigned int			mwilamp:3;
	unsigned int			mwioncall:1;
	unsigned int			softkeysupport:1;
	unsigned int			mwilight:1;
	unsigned int			dnd:1;
	unsigned int			transfer:1;
	unsigned int			park:1;
	unsigned int			cfwdall:1;
	unsigned int			cfwdbusy:1;
	unsigned int			dtmfmode:1; /* 0 inband - 1 outofband */
	unsigned int			nat:1;
	unsigned int			trustphoneip:1;
	unsigned int			needcheckringback:1;
	unsigned int			private:1; /* permit private function on this device */

	sccp_channel_t * active_channel;
	/* the channel under transfer */
	sccp_channel_t * transfer_channel;
	sccp_speed_t * speed_dials;
	sccp_line_t * lines;
	sccp_line_t * currentLine;
	sccp_session_t * session;
	sccp_device_t * next;
	/* list of hint pointers. Internal lines to notify the state */
	sccp_hint_t * hints;
};

struct sccp_session {
  cw_mutex_t			lock;
  void					*buffer;
  size_t				buffer_size;
  struct sockaddr_in	sin;
  /* ourip is for rtp use */
  struct in_addr		ourip;
  time_t				lastKeepAlive;
  int					fd;
  int					rtpPort;
  sccp_device_t 		*device;
  sccp_session_t		*prev, *next;
  unsigned int			needcheckringback:1;
};

struct sccp_channel {
	cw_mutex_t			lock;
	/* codec requested by callweaver */
	int					format;
	char				calledPartyName[StationMaxNameSize];
	char				calledPartyNumber[StationMaxDirnumSize];
	char				callingPartyName[StationMaxNameSize];
	char				callingPartyNumber[StationMaxDirnumSize];
	uint32_t			callid;
	/* internal channel state SCCP_CHANNELSTATE_* */
	uint8_t				state;
	/* skinny state */
	uint8_t				callstate;
	/* SKINNY_CALLTYPE_* */
	uint8_t				calltype;
	/* timeout on dialing state */
	time_t				digittimeout;
	/* SCCPRingerMode application */
	uint8_t				ringermode;
	/* last dialed number */
	char dialedNumber[CW_MAX_EXTENSION];

	sccp_device_t 	 *	device;
	struct cw_channel *	owner;
	sccp_line_t		 *	line;
	struct cw_rtp	 *	rtp;
	struct sockaddr_in	rtp_addr;
	sccp_channel_t	 *	prev, * next,
					 *	prev_on_line, * next_on_line;
	uint8_t				autoanswer_type;
	uint8_t				autoanswer_cause;

	/* don't allow sccp phones to monitor (hint) this call */
	unsigned int		private:1;
	unsigned int		hangupok:1;
};

struct sccp_global_vars {
	cw_mutex_t				lock;

	sccp_session_t			*sessions;
	cw_mutex_t				sessions_lock;
	sccp_device_t			*devices;
	cw_mutex_t				devices_lock;
	sccp_line_t	  			*lines;
	cw_mutex_t				lines_lock;
	sccp_channel_t 			*channels;
	cw_mutex_t				channels_lock;
	cw_mutex_t				socket_lock;
	int 					descriptor;
	/* Keep track of when we're in use. */
	int						usecnt;
	cw_mutex_t				usecnt_lock;

	char					servername[StationMaxDisplayNotifySize];
	struct sockaddr_in		bindaddr;
	int						ourport;
	/* permit or deny connections to the main socket */
	struct cw_ha			*ha;
	/* localnet for NAT */
	struct cw_ha			*localaddr;
	struct sockaddr_in		externip;
	char 					externhost[MAXHOSTNAMELEN];
	time_t 					externexpire;
	int 					externrefresh;

	char					context[CW_MAX_CONTEXT];
	char					language[MAX_LANGUAGE];
	char					accountcode[CW_MAX_ACCOUNT_CODE];
	char					musicclass[MAX_MUSICCLASS];
	int						amaflags;
	cw_group_t				callgroup;
#ifdef CS_SCCP_PICKUP
	cw_group_t				pickupgroup;
#endif
	int						global_capability;
	struct					cw_codec_pref global_codecs;
	int						keepalive;
	int						debug;
	char 					date_format[7];
/* Wait up to 16 seconds for first digit */
	uint8_t					firstdigittimeout;
/* How long to wait for following digits */
	uint8_t					digittimeout;
/* what char will force the dial */
	char					digittimeoutchar;
	uint32_t				tos;
	uint32_t				rtptos;
	/* channel state where to open the rtp media stream */
	uint8_t					earlyrtp;
		/* dnd mode: see SCCP_DNDMODE_* */
	uint8_t dndmode;
		/* skinny protocol version */
	uint8_t protocolversion;

	/* autoanswer stuff */
	uint8_t					autoanswer_ring_time;
	uint8_t					autoanswer_tone;
	uint8_t					remotehangup_tone;
	uint8_t					transfer_tone;
	uint8_t					callwaiting_tone;
	/* echocancel phone support */
	unsigned int			mwilamp:3;
	unsigned int			mwioncall:1;
	unsigned int			echocancel:1;
	unsigned int			silencesuppression:1;
	unsigned int			trustphoneip:1;
	unsigned int			private:1; /* permit private function */
	unsigned int			blindtransferindication:1; /* SCCP_BLINDTRANSFER_* */
};

struct sccp_global_vars *sccp_globals;

uint8_t sccp_handle_message(sccp_moo_t * r, sccp_session_t * s);

struct cw_channel *sccp_request(const char *type, int format, void *data, int *cause);

int sccp_devicestate(void *data);
sccp_hint_t * sccp_hint_make(sccp_device_t *d, uint8_t instance);
void sccp_hint_notify_devicestate(sccp_device_t * d, uint8_t state);
void sccp_hint_notify_linestate(sccp_line_t * l, uint8_t state, sccp_device_t * onedevice);
void sccp_hint_notify_channelstate(sccp_device_t *d, uint8_t instance, sccp_channel_t * c);
int sccp_hint_state(char *context, char* exten, int state, void *data);
void sccp_hint_notify(sccp_channel_t *c, sccp_device_t * onedevice);

#endif /* __CHAN_SCCP_H */

