/*
 * CallWeaver -- An open source telephony toolkit.
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

enum
{
    STUN_STATE_IDLE = 0,
    STUN_STATE_REQUEST_PENDING,
    STUN_STATE_RESPONSE_RECEIVED
};
 
#define STUN_IGNORE                (0)
#define STUN_ACCEPT                (1)

#define STUN_BINDREQ            0x0001
#define STUN_BINDRESP           0x0101
#define STUN_BINDERR            0x0111
#define STUN_SECREQ             0x0002
#define STUN_SECRESP            0x0102
#define STUN_SECERR             0x0112

#define STUN_MAPPED_ADDRESS     0x0001
#define STUN_RESPONSE_ADDRESS   0x0002
#define STUN_CHANGE_REQUEST     0x0003
#define STUN_SOURCE_ADDRESS     0x0004
#define STUN_CHANGED_ADDRESS    0x0005
#define STUN_USERNAME           0x0006
#define STUN_PASSWORD           0x0007
#define STUN_MESSAGE_INTEGRITY  0x0008
#define STUN_ERROR_CODE         0x0009
#define STUN_UNKNOWN_ATTRIBUTES 0x000a
#define STUN_REFLECTED_FROM     0x000b

extern char stunserver_host[MAXHOSTNAMELEN];
extern struct sockaddr_in stunserver_ip;
extern int stunserver_portno;
extern int stun_active;            /*!< Is STUN globally enabled ?*/
extern int stundebug;            /*!< Are we debugging stun? */

typedef struct
{
    unsigned int id[4];
} __attribute__((packed)) stun_trans_id;

struct stun_header
{
    unsigned short msgtype;
    unsigned short msglen;
    stun_trans_id  id;
    unsigned char ies[0];
} __attribute__((packed));


struct stun_attr
{
    unsigned short attr;
    unsigned short len;
    unsigned char value[0];
} __attribute__((packed));

struct stun_addr
{
    unsigned char unused;
    unsigned char family;
    unsigned short port;
    unsigned int addr;
} __attribute__((packed));

struct stun_request
{
    struct stun_header req_head;
    struct stun_request *next;
    time_t whendone;
    int got_response;
    struct stun_addr mapped_addr;
};

struct stun_state
{
    unsigned short msgtype;
    stun_trans_id  id;
    unsigned char *username;
    unsigned char *password;
    struct stun_addr *mapped_addr;
    struct stun_addr *response_addr;
    struct stun_addr *source_addr;
};

int stun_addr2sockaddr (struct sockaddr_in *sin, struct stun_addr *addr);

struct stun_addr *cw_stun_find_request(stun_trans_id *st);

struct stun_request *cw_udp_stun_bindrequest(int fdus,
                                               struct sockaddr_in *suggestion, 
                                               const char *username,
                                               const char *password);

int stun_remove_request(stun_trans_id *st);

int stun_handle_packet(int s, struct sockaddr_in *src, unsigned char *data, size_t len, struct stun_state *st);

int stun_do_debug(int fd, int argc, char *argv[]);

int stun_no_debug(int fd, int argc, char *argv[]);

void cw_stun_init(void);

//static void append_attr_string(struct stun_attr **attr, int attrval, const char *s, int *len, int *left)
