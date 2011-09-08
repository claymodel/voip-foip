/*
 * CallWeaver Bluetooth Channel
 *
 * callweaver'ized by Rico Gloeckner <mc+callweaver@ukeer.de>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 */

#ifdef HAVE_CONFIG_H
 #include "confdefs.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <endian.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sco.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "callweaver/lock.h"
#include "callweaver/utils.h"
#include "callweaver/channel.h"
#include "callweaver/config.h"
#include "callweaver/logger.h"
#include "callweaver/module.h"
#include "callweaver/pbx.h"
#include "callweaver/sched.h"
#include "callweaver/options.h"
#include "callweaver/cli.h"

#ifndef HANDSFREE_AUDIO_GW_SVCLASS_ID
# define HANDSFREE_AUDIO_GW_SVCLASS_ID 0x111f
#endif
#define BLUETOOTH_FORMAT    CW_FORMAT_SLINEAR
#define BLT_CHAN_NAME       "BLT"
#define BLT_CONFIG_FILE     "chan_bluetooth.conf"
#define BLT_DESCRIPTION     "Bluetooth Channel driver for CallWeaver.org"
#define BLT_RDBUFF_MAX      1024
#define BLT_DEFAULT_HCI_DEV 0

/*! Bluetooth Device Roles */

typedef enum {
  BLT_ROLE_NONE = 0, // Unknown Device
  BLT_ROLE_HS = 1,   // Device is a Headset
  BLT_ROLE_AG = 2    // Device is an Audio Gateway
} blt_role_t;

/*! Bluetooth: State when we're in HS mode */

typedef enum {
  BLT_STATE_WANT_R   = 0,
  BLT_STATE_WANT_N   = 1,
  BLT_STATE_WANT_CMD = 2,
  BLT_STATE_WANT_N2  = 3,
} blt_state_t;

/*! Bluetooth: State of BT Device */
typedef enum {
  BLT_STATUS_DOWN,
  BLT_STATUS_CONNECTING,
  BLT_STATUS_NEGOTIATING,
  BLT_STATUS_READY,
  BLT_STATUS_RINGING,
  BLT_STATUS_IN_CALL,
} blt_status_t;



/* Default config settings */

#define BLT_DEFAULT_CHANNEL_AG   5
#define BLT_DEFAULT_CHANNEL_HS   6
#define BLT_DEFAULT_ROLE         BLT_ROLE_HS
#define BLT_OBUF_LEN             (48 * 25)

#define BUFLEN (4800)

/* ---------------------------------- */

typedef struct blt_dev blt_dev_t;

struct blt_ring {
	unsigned char buf[BUFLEN];
};


// FIXME needs to be cleaned up
/*! Bluetooth: this struct holds all information about a particular BT Device */
struct blt_dev {

  blt_status_t status;              /* Device Status */

  struct cw_channel * owner;       /* Channel we belong to, possibly NULL */
  blt_dev_t * dev;                  /* The bluetooth device channel is for */
  struct cw_frame fr;              /* Recieved frame */

  /* SCO Handler */
  int sco_pipe[2];                   /* SCO alert pipe */
  int sco;                           /* SCO fd */
  int sco_handle;                    /* SCO Handle */
  int sco_mtu;                       /* SCO MTU */
  int sco_running;                   /* 1 when sCO thread should be running */
  pthread_t sco_thread;              /* SCO thread */
  cw_mutex_t sco_lock;              /* SCO lock */
  int sco_pos_in;                    /* Reader in position (drain)*/
  int sco_pos_inrcv;                 /* Reader in position (fill) */
  int wakeread;	                     /* blt_read() needs to be woken */
  int sco_pos_out;                   /* Reader out position */
  int sco_sending;                   /* Sending SCO packets */
  char buf[1200];                    /* Incoming data buffer */
  int bufpos;
  char sco_buf_out[BUFLEN];          /* 24 chunks of 48 */
  char sco_buf_in[BUFLEN];           /* 24 chunks of 48 */

  char dnid[1024];                    /* Outgoi gncall dialed number */
  unsigned char * obuf[BLT_OBUF_LEN]; /* Outgoing data buffer */
  int obuf_len;                       /* Output Buffer Position */
  int obuf_wpos;                      /* Buffer Reader */

  // device
  int autoconnect;                  /* 1 for autoconnect */
  int outgoing_id;                  /* Outgoing connection scheduler id */
  char * name;                      /* Devices friendly name */
  blt_role_t role;                  /* Device role (HS or AG) */
  bdaddr_t bdaddr;                  /* remote address */
  int channel;                      /* remote channel */
  int rd;                           /* RFCOMM fd */
  int tmp_rd;                       /* RFCOMM fd */
  int call_cnt;                     /* Number of attempted calls */
  cw_mutex_t lock;                 /* RFCOMM socket lock */
  char rd_buff[BLT_RDBUFF_MAX];     /* RFCOMM input buffer */
  int rd_buff_pos;                  /* RFCOMM input buffer position */
  int ready;                        /* 1 When ready */
  char *context;

  /* AG mode */
  char last_ok_cmd[BLT_RDBUFF_MAX];        /* Runtime[AG]: Last AT command that was OK */
  int cind;                                /* Runtime[AG]: Recieved +CIND */  
  int call_pos, service_pos, callsetup_pos;  /* Runtime[AG]: Positions in CIND/CMER */
  int call, service, callsetup;              /* Runtime[AG]: Values */
  char cid_num[CW_MAX_EXTENSION];
  char cid_name[CW_MAX_EXTENSION];

  /* HS mode */
  blt_state_t state;                       /* Runtime: Device state (AG mode only) */
  int ring_timer;                          /* Runtime:Ring Timer */
  char last_err_cmd[BLT_RDBUFF_MAX];       /* Runtime: Last AT command that was OK */
  void (*cb)(blt_dev_t * dev, char * str); /* Runtime: Callback when in HS mode */

  int brsf;                                /* Runtime: Bluetooth Retrieve Supported Features */
  int bvra;                                /* Runtime: Bluetooth Voice Recognised Activation */
  int gain_speaker;                        /* Runtime: Gain Of Speaker */
  int clip;                                /* Runtime: Supports CLID */
  int colp;                                /* Runtime: Connected Line ID */
  int ccwa;                                /* Runtime: Call waiting ind enabled */
  int elip;                                /* Runtime: (Ericsson) Supports CLID */
  int eolp;                                /* Runtime: (Ericsson) Connected Line ID */
  int ringing;                             /* Runtime: Device is ringing */

  blt_dev_t * next;                        /* Next in linked list */

};

/*! Bluetooth: AT command callbacks */
typedef struct blt_atcb {

  /* The command */
  char * str;

  /* DTE callbacks: */
  int (*set)(blt_dev_t * dev, const char * arg, int len);
  int (*read)(blt_dev_t * dev);
  int (*execute)(blt_dev_t * dev, const char * data);
  int (*test)(blt_dev_t * dev);

  /* DCE callbacks: */
  int (*unsolicited)(blt_dev_t * dev, const char * value);

} blt_atcb_t;

/* ---------------------------------- */

static void rd_close(blt_dev_t * dev, int reconnect, int err);
static int send_atcmd(blt_dev_t * device, const char * fmt, ...);
static int sco_connect(blt_dev_t * dev);
static int sco_start(blt_dev_t * dev, int fd);

/* ---------------------------------- */

/* RFCOMM channel we listen on*/
static int rfcomm_channel_ag = BLT_DEFAULT_CHANNEL_AG;
static int rfcomm_channel_hs = BLT_DEFAULT_CHANNEL_HS;

/* Address of local bluetooth interface */
static int hcidev_id;
static bdaddr_t local_bdaddr;

/* All the current sockets */
CW_MUTEX_DEFINE_STATIC(iface_lock);
static blt_dev_t * iface_head;
static int ifcount = 0;

static int sdp_record_hs = -1;
static int sdp_record_ag = -1;

/* RFCOMM listen socket */
static int rfcomm_sock_ag = -1;
static int rfcomm_sock_hs = -1;
static int sco_socket = -1;

static int monitor_pid = -1;

/* The socket monitoring thread */
static pthread_t monitor_thread = CW_PTHREADT_NULL;
CW_MUTEX_DEFINE_STATIC(monitor_lock);

/* Cound how many times this module is currently in use */
static int usecnt = 0;
CW_MUTEX_DEFINE_STATIC(usecnt_lock);

static struct sched_context * sched = NULL;

/* ---------------------------------- */

static struct cw_channel *blt_request(const char *type, int format, void *data, int *cause);
static int blt_hangup(struct cw_channel *c);
static int blt_answer(struct cw_channel *c);
static struct cw_frame *blt_read(struct cw_channel *chan);
static int blt_call(struct cw_channel *c, char *dest, int timeout);
static int blt_write(struct cw_channel *chan, struct cw_frame *f);
static int blt_indicate(struct cw_channel *chan, int cond);

static int blt_show_information(int, int, char**);
static int blt_show_peers(int, int, char **);
static int blt_ag_sendcmd(int, int, char **);

static int atcmd_cclk_read(blt_dev_t *);
static int atcmd_cind_read(blt_dev_t *);
static int atcmd_cind_test(blt_dev_t *);
static int atcmd_chld_test(blt_dev_t *);
static int atcmd_clan_read(blt_dev_t *);

static int atcmd_brsf_set(blt_dev_t *, const char *, int);
static int atcmd_bvra_set(blt_dev_t *, const char *, int);
static int atcmd_clip_set(blt_dev_t *, const char *, int);
static int atcmd_colp_set(blt_dev_t *, const char *, int);
static int atcmd_ccwa_set(blt_dev_t *, const char *, int);
static int atcmd_cmer_set(blt_dev_t *, const char *, int);
static int atcmd_cpbr_set(blt_dev_t *, const char *, int);
static int atcmd_cpbs_set(blt_dev_t *, const char *, int);
static int atcmd_cscs_set(blt_dev_t *, const char *, int);
static int atcmd_eips_set(blt_dev_t *, const char *, int);
static int atcmd_vgs_set(blt_dev_t *, const char *, int);

static int atcmd_bldn_execute(blt_dev_t *, const char *);
static int atcmd_chup_execute(blt_dev_t *, const char *);
static int atcmd_dial_execute(blt_dev_t *, const char *);
static int atcmd_answer_execute(blt_dev_t *, const char *);

static int ag_unsol_ciev(blt_dev_t *, const char *);
static int ag_unsol_cind(blt_dev_t *, const char *);
static int ag_unsol_clip(blt_dev_t *, const char *);

static int blt_parse_config(void);
static char *complete_device_2_ag(char *, char *, int, int);

/*! Bluetooth: channel tech callback information */
static const struct cw_channel_tech blt_tech = {
	.type = BLT_CHAN_NAME,
	.description = BLT_DESCRIPTION,
	.capabilities = BLUETOOTH_FORMAT,
	.requester = blt_request,
	.hangup = blt_hangup,
	.answer = blt_answer,
	.read = blt_read,
	.call = blt_call,
	.write = blt_write,
	.indicate = blt_indicate,
};

/*! Bluetooth: List of all AT cmd:s plus their at callbacks */
static blt_atcb_t
atcmd_list[] = 
{
  { "A",     NULL,           NULL,            atcmd_answer_execute, NULL,             NULL },
  { "D",     NULL,           NULL,            atcmd_dial_execute,   NULL,             NULL },
  { "+BRSF", atcmd_brsf_set, NULL,            NULL,                 NULL,             NULL },
  { "+BVRA", atcmd_bvra_set, NULL,            NULL,                 NULL,             NULL },
  { "+CCLK", NULL,           atcmd_cclk_read, NULL,                 NULL,             NULL },
  { "+CCWA", atcmd_ccwa_set, NULL,            NULL,                 NULL,             NULL },
  { "+CHLD", NULL,           NULL,            NULL,                 atcmd_chld_test,  NULL },
  { "+CHUP", NULL,           NULL,            atcmd_chup_execute,   NULL,             NULL },
  { "+CIEV", NULL,           NULL,            NULL,                 NULL,             ag_unsol_ciev },
  { "+CIND", NULL,           atcmd_cind_read, NULL,                 atcmd_cind_test,  ag_unsol_cind },
  { "+CLAN", NULL,           atcmd_clan_read, NULL,                 NULL,             NULL },
  { "+CLIP", atcmd_clip_set, NULL,            NULL,                 NULL,             ag_unsol_clip },
  { "+COLP", atcmd_colp_set, NULL,            NULL,                 NULL,             NULL },
  { "+CMER", atcmd_cmer_set, NULL,            NULL,                 NULL,             NULL },
  { "+CPBR", atcmd_cpbr_set, NULL,            NULL,                 NULL,             NULL },
  { "+CPBS", atcmd_cpbs_set, NULL,            NULL,                 NULL,             NULL },
  { "+CSCS", atcmd_cscs_set, NULL,            NULL,                 NULL,             NULL },
  { "*EIPS", atcmd_eips_set, NULL,            NULL,                 NULL,             NULL },
  { "+VGS",  atcmd_vgs_set,  NULL,            NULL,                 NULL,             NULL },
  { "+BLDN", NULL,           NULL,            atcmd_bldn_execute,   NULL,             NULL },
};

#define ATCMD_LIST_LEN (sizeof(atcmd_list) / sizeof(blt_atcb_t))


/*
 * CW.org CLI command definitions for Channel:Bluetooth
 */

static char show_peers_usage[] =
"Usage: bluetooth show peers\n"
"       List all bluetooth peers and their status\n";

static struct cw_cli_entry
cli_show_peers =
    { { "bluetooth", "show", "peers", NULL }, blt_show_peers, "List Bluetooth Peers", show_peers_usage };


static char ag_sendcmd[] =
"Usage: bluetooth ag <device> sendcmd <cmd>\n"
"       Sends a AT cmd over the RFCOMM link, and print result (AG only)\n";

static struct cw_cli_entry
cli_ag_sendcmd =
    { { "bluetooth", "sendcmd", NULL }, blt_ag_sendcmd, "Send AG an AT command", ag_sendcmd, complete_device_2_ag };

static char show_information[] =
"Usage: bluetooth show information\n"
"       Lists information about the bluetooth subsystem\n";

static struct cw_cli_entry
cli_show_information =
    { { "bluetooth", "show", "information", NULL }, blt_show_information, "List Bluetooth Info", show_information };

