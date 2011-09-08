/* $Id$
 *
 * Copyright (C) 2003-2008 Sippy Software, Inc., http://www.sippysoft.com
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * ---------
 * 2003-10-09	nat_uac_test introduced (jiri)
 *
 * 2003-11-06   nat_uac_test permitted from onreply_route (jiri)
 *
 * 2003-12-01   unforce_rtp_proxy introduced (sobomax)
 *
 * 2004-01-07	RTP proxy support updated to support new version of the
 *		RTP proxy (20040107).
 *
 *		force_rtp_proxy() now inserts a special flag
 *		into the SDP body to indicate that this session already
 *		proxied and ignores sessions with such flag.
 *
 *		Added run-time check for version of command protocol
 *		supported by the RTP proxy.
 *
 * 2004-01-16   Integrated slightly modified patch from Tristan Colgate,
 *		force_rtp_proxy function with IP as a parameter (janakj)
 *
 * 2004-01-28	nat_uac_test extended to allow testing SDP body (sobomax)
 *
 *		nat_uac_test extended to allow testing top Via (sobomax)
 *
 * 2004-02-21	force_rtp_proxy now accepts option argument, which
 *		consists of string of chars, each of them turns "on"
 *		some feature, currently supported ones are:
 *
 *		 `a' - flags that UA from which message is received
 *		       doesn't support symmetric RTP;
 *		 `l' - force "lookup", that is, only rewrite SDP when
 *		       corresponding session is already exists in the
 *		       RTP proxy. Only makes sense for SIP requests,
 *		       replies are always processed in "lookup" mode;
 *		 `i' - flags that message is received from UA in the
 *		       LAN. Only makes sense when RTP proxy is running
 *		       in the bridge mode.
 *
 *		force_rtp_proxy can now be invoked without any arguments,
 *		as previously, with one argument - in this case argument
 *		is treated as option string and with two arguments, in
 *		which case 1st argument is option string and the 2nd
 *		one is IP address which have to be inserted into
 *		SDP (IP address on which RTP proxy listens).
 *
 * 2004-03-12	Added support for IPv6 addresses in SDPs. Particularly,
 *		force_rtp_proxy now can work with IPv6-aware RTP proxy,
 *		replacing IPv4 address in SDP with IPv6 one and vice versa.
 *		This allows creating full-fledged IPv4<->IPv6 gateway.
 *		See 4to6.cfg file for example.
 *
 *		Two new options added into force_rtp_proxy:
 *
 *		 `f' - instructs nathelper to ignore marks inserted
 *		       by another nathelper in transit to indicate
 *		       that the session is already goes through another
 *		       proxy. Allows creating chain of proxies.
 *		 `r' - flags that IP address in SDP should be trusted.
 *		       Without this flag, nathelper ignores address in the
 *		       SDP and uses source address of the SIP message
 *		       as media address which is passed to the RTP proxy.
 *
 *		Protocol between nathelper and RTP proxy in bridge
 *		mode has been slightly changed. Now RTP proxy expects SER
 *		to provide 2 flags when creating or updating session
 *		to indicate direction of this session. Each of those
 *		flags can be either `e' or `i'. For example `ei' means
 *		that we received INVITE from UA on the "external" network
 *		network and will send it to the UA on "internal" one.
 *		Also possible `ie' (internal->external), `ii'
 *		(internal->internal) and `ee' (external->external). See
 *		example file alg.cfg for details.
 *
 * 2004-03-15	If the rtp proxy test failed (wrong version or not started)
 *		retry test from time to time, when some *rtpproxy* function
 *		is invoked. Minimum interval between retries can be
 *		configured via rtpproxy_disable_tout module parameter (default
 *		is 60 seconds). Setting it to -1 will disable periodic
 *		rechecks completely, setting it to 0 will force checks
 *		for each *rtpproxy* function call. (andrei)
 *
 * 2004-03-22	Fix assignment of rtpproxy_retr and rtpproxy_tout module
 *		parameters.
 *
 * 2004-03-22	Fix get_body position (should be called before get_callid)
 * 				(andrei)
 *
 * 2004-03-24	Fix newport for null ip address case (e.g onhold re-INVITE)
 * 				(andrei)
 *
 * 2004-09-30	added received port != via port test (andrei)
 *
 * 2004-10-10   force_socket option introduced (jiri)
 *
 * 2005-02-24	Added support for using more than one rtp proxy, in which
 *		case traffic will be distributed evenly among them. In addition,
 *		each such proxy can be assigned a weight, which will specify
 *		which share of the traffic should be placed to this particular
 *		proxy.
 *
 *		Introduce failover mechanism, so that if SER detects that one
 *		of many proxies is no longer available it temporarily decreases
 *		its weight to 0, so that no traffic will be assigned to it.
 *		Such "disabled" proxies are periodically checked to see if they
 *		are back to normal in which case respective weight is restored
 *		resulting in traffic being sent to that proxy again.
 *
 *		Those features can be enabled by specifying more than one "URI"
 *		in the rtpproxy_sock parameter, optionally followed by the weight,
 *		which if absent is assumed to be 1, for example:
 *
 *		rtpproxy_sock="unix:/foo/bar=4 udp:1.2.3.4:3456=3 udp:5.6.7.8:5432=1"
 *
 * 2005-02-25	Force for pinging the socket returned by USRLOC (bogdan)
 *
 * 2005-03-22	support for multiple media streams added (netch)
 *
 * 2005-07-11  SIP ping support added (bogdan)
 *
 * 2005-07-14  SDP origin (o=) IP may be also changed (bogdan)
 *
 * 2006-03-08  fix_nated_sdp() may take one more param to force a specific IP;
 *             force_rtp_proxy() accepts a new flag 's' to swap creation/
 *              confirmation between requests/replies; 
 *             add_rcv_param() may take as parameter a flag telling if the
 *              parameter should go to the contact URI or contact header;
 *             (bogdan)
 * 2006-03-28 Support for changing session-level SDP connection (c=) IP when
 *            media-description also includes connection information (bayan)
 * 2007-04-13 Support multiple sets of rtpproxies and set selection added
 *            (ancuta)
 * 2007-04-26 Added some MI commands:
 *             nh_enable_ping used to enable or disable natping
 *             nh_enable_rtpp used to enable or disable a specific rtp proxy
 *             nh_show_rtpp   used to display information for all rtp proxies 
 *             (ancuta)
 * 2007-05-09 New function start_recording() allowing to start recording RTP 
 *             session in the RTP proxy (Carsten Bock - ported from SER)
 * 2007-09-11 Separate timer process and support for multiple timer processes
 *             (bogdan)
 * 2008-12-12 Support for RTCP attribute in the SDP
 *              (Min Wang/BASIS AudioNet - ported from SER)
 * 2010-08-05 Core SDP parser integrated into nathelper
 *             (osas)
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#ifndef __USE_BSD
#define  __USE_BSD
#endif
#include <netinet/ip.h>
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../flags.h"
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../error.h"
#include "../../forward.h"
#include "../../ip_addr.h"
#include "../../mem/mem.h"
#include "../../parser/parse_from.h"
#include "../../parser/parse_to.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parser_f.h"
#include "../../parser/sdp/sdp.h"
#include "../../resolve.h"
#include "../../timer.h"
#include "../../trim.h"
#include "../../ut.h"
#include "../../pt.h"
#include "../../timer_proc.h"
#include "../../lib/kmi/attr.h"
#include "../../lib/kmi/mi.h"
#include "../../lib/kcore/km_ut.h"
#include "../../lib/kcore/parser_helpers.h"
#include "../../pvar.h"
#include "../../lvalue.h"
#include "../../msg_translator.h"
#include "../../usr_avp.h"
#include "../../socket_info.h"
#include "../../mod_fix.h"
#include "../../dset.h"
#include "../registrar/sip_msg.h"
#include "../usrloc/usrloc.h"
#include "nathelper.h"
#include "nhelpr_funcs.h"
#include "sip_pinger.h"
 
MODULE_VERSION

#if !defined(AF_LOCAL)
#define	AF_LOCAL AF_UNIX
#endif
#if !defined(PF_LOCAL)
#define	PF_LOCAL PF_UNIX
#endif

/* NAT UAC test constants */
#define	NAT_UAC_TEST_C_1918	0x01
#define	NAT_UAC_TEST_RCVD	0x02
#define	NAT_UAC_TEST_V_1918	0x04
#define	NAT_UAC_TEST_S_1918	0x08
#define	NAT_UAC_TEST_RPORT	0x10
#define	NAT_UAC_TEST_O_1918	0x20


#define DEFAULT_RTPP_SET_ID		0

#define MI_SET_NATPING_STATE		"nh_enable_ping"
#define MI_DEFAULT_NATPING_STATE	1

#define MI_ENABLE_RTP_PROXY			"nh_enable_rtpp"
#define MI_MIN_RECHECK_TICKS		0
#define MI_MAX_RECHECK_TICKS		(unsigned int)-1

#define MI_SHOW_RTP_PROXIES			"nh_show_rtpp"

#define MI_RTP_PROXY_NOT_FOUND		"RTP proxy not found"
#define MI_RTP_PROXY_NOT_FOUND_LEN	(sizeof(MI_RTP_PROXY_NOT_FOUND)-1)
#define MI_PING_DISABLED			"NATping disabled from script"
#define MI_PING_DISABLED_LEN		(sizeof(MI_PING_DISABLED)-1)
#define MI_SET						"set"
#define MI_SET_LEN					(sizeof(MI_SET)-1)
#define MI_INDEX					"index"
#define MI_INDEX_LEN				(sizeof(MI_INDEX)-1)
#define MI_DISABLED					"disabled"
#define MI_DISABLED_LEN				(sizeof(MI_DISABLED)-1)
#define MI_WEIGHT					"weight"
#define MI_WEIGHT_LEN				(sizeof(MI_WEIGHT)-1)
#define MI_RECHECK_TICKS			"recheck_ticks"
#define MI_RECHECK_T_LEN			(sizeof(MI_RECHECK_TICKS)-1)



/* Supported version of the RTP proxy command protocol */
#define	SUP_CPROTOVER	20040107
/* Required additional version of the RTP proxy command protocol */
#define	REQ_CPROTOVER	"20050322"
/* Additional version necessary for re-packetization support */
#define	REP_CPROTOVER	"20071116"
#define	PTL_CPROTOVER	"20081102"

#define	CPORT		"22222"
static int nat_uac_test_f(struct sip_msg* msg, char* str1, char* str2);
static int fix_nated_contact_f(struct sip_msg *, char *, char *);
static int add_contact_alias_f(struct sip_msg *, char *, char *);
static int handle_ruri_alias_f(struct sip_msg *, char *, char *);
static int pv_get_rr_count_f(struct sip_msg *, pv_param_t *, pv_value_t *);
static int pv_get_rr_top_count_f(struct sip_msg *, pv_param_t *, pv_value_t *);
static int fix_nated_sdp_f(struct sip_msg *, char *, char *);
static int extract_mediaip(str *, str *, int *, char *);
static int alter_mediaip(struct sip_msg *, str *, str *, int, str *, int, int);
static int fix_nated_register_f(struct sip_msg *, char *, char *);
static int fixup_fix_nated_register(void** param, int param_no);
static int fixup_fix_sdp(void** param, int param_no);
static int add_rcv_param_f(struct sip_msg *, char *, char *);

static void nh_timer(unsigned int, void *);
static int mod_init(void);
static int child_init(int);
static void mod_destroy(void);

/*mi commands*/
static struct mi_root* mi_enable_natping(struct mi_root* cmd_tree,
		void* param );


static usrloc_api_t ul;

static int cblen = 0;
static int natping_interval = 0;
struct socket_info* force_socket = 0;


static struct {
	const char *cnetaddr;
	uint32_t netaddr;
	uint32_t mask;
} nets_1918[] = {
	{"10.0.0.0",    0, 0xffffffffu << 24},
	{"172.16.0.0",  0, 0xffffffffu << 20},
	{"192.168.0.0", 0, 0xffffffffu << 16},
	{NULL, 0, 0}
};

/*
 * If this parameter is set then the natpinger will ping only contacts
 * that have the NAT flag set in user location database
 */
static int ping_nated_only = 0;
static const char sbuf[4] = {0, 0, 0, 0};
static char *force_socket_str = 0;
static pid_t mypid;
static int sipping_flag = -1;
static int natping_processes = 1;

static str nortpproxy_str = str_init("a=nortpproxy:yes");

static char* rcv_avp_param = NULL;
static unsigned short rcv_avp_type = 0;
static int_str rcv_avp_name;

static char *natping_socket = 0;
static int raw_sock = -1;
static unsigned int raw_ip = 0;
static unsigned short raw_port = 0;


/*0-> disabled, 1 ->enabled*/
unsigned int *natping_state=0;

static cmd_export_t cmds[] = {
	{"fix_nated_contact",  (cmd_function)fix_nated_contact_f,    0,
		0, 0,
		REQUEST_ROUTE|ONREPLY_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"add_contact_alias",  (cmd_function)add_contact_alias_f,    0,
		0, 0,
		REQUEST_ROUTE|ONREPLY_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"handle_ruri_alias",  (cmd_function)handle_ruri_alias_f,    0,
		0, 0,
		REQUEST_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"fix_nated_sdp",      (cmd_function)fix_nated_sdp_f,        1,
		fixup_fix_sdp,  0,
		REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"fix_nated_sdp",      (cmd_function)fix_nated_sdp_f,        2,
		fixup_fix_sdp, 0,
		REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"nat_uac_test",       (cmd_function)nat_uac_test_f,         1,
		fixup_uint_null, 0,
		REQUEST_ROUTE|ONREPLY_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE|LOCAL_ROUTE},
	{"fix_nated_register", (cmd_function)fix_nated_register_f,   0,
		fixup_fix_nated_register, 0,
		REQUEST_ROUTE },
	{"add_rcv_param",      (cmd_function)add_rcv_param_f,        0,
		0, 0,
		REQUEST_ROUTE },
	{"add_rcv_param",      (cmd_function)add_rcv_param_f,        1,
		fixup_uint_null, 0,
		REQUEST_ROUTE },
	{0, 0, 0, 0, 0, 0}
};

static pv_export_t mod_pvs[] = {
    {{"rr_count", (sizeof("rr_count")-1)}, /* number of records routes */
     PVT_CONTEXT, pv_get_rr_count_f, 0, 0, 0, 0, 0},
    {{"rr_top_count", (sizeof("rr_top_count")-1)}, /* number of topmost rrs */
     PVT_CONTEXT, pv_get_rr_top_count_f, 0, 0, 0, 0, 0},
    {{0, 0}, 0, 0, 0, 0, 0, 0, 0}
};

static param_export_t params[] = {
	{"natping_interval",      INT_PARAM, &natping_interval      },
	{"ping_nated_only",       INT_PARAM, &ping_nated_only       },
	{"nortpproxy_str",        STR_PARAM, &nortpproxy_str.s      },
	{"received_avp",          STR_PARAM, &rcv_avp_param         },
	{"force_socket",          STR_PARAM, &force_socket_str      },
	{"sipping_from",          STR_PARAM, &sipping_from.s        },
	{"sipping_method",        STR_PARAM, &sipping_method.s      },
	{"sipping_bflag",         INT_PARAM, &sipping_flag          },
	{"natping_processes",     INT_PARAM, &natping_processes     },
	{"natping_socket",        STR_PARAM, &natping_socket        },
	{0, 0, 0}
};

static mi_export_t mi_cmds[] = {
	{MI_SET_NATPING_STATE,    mi_enable_natping,    0,                0, 0},
	{ 0, 0, 0, 0, 0}
};


struct module_exports exports = {
	"nathelper",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,
	0,           /* exported statistics */
	mi_cmds,     /* exported MI functions */
	mod_pvs,     /* exported pseudo-variables */
	0,           /* extra processes */
	mod_init,
	0,           /* reply processing */
	mod_destroy, /* destroy function */
	child_init
};


static int
fixup_fix_sdp(void** param, int param_no)
{
	pv_elem_t *model;
	str s;

	if (param_no==1) {
		/* flags */
		return fixup_uint_null( param, param_no);
	}
	/* new IP */
	model=NULL;
	s.s = (char*)(*param); s.len = strlen(s.s);
	if(pv_parse_format(&s,&model)<0) {
		LM_ERR("wrong format[%s]!\n", (char*)(*param));
		return E_UNSPEC;
	}
	if (model==NULL) {
		LM_ERR("empty parameter!\n");
		return E_UNSPEC;
	}
	*param = (void*)model;
	return 0;
}

static int fixup_fix_nated_register(void** param, int param_no)
{
	if (rcv_avp_name.n == 0) {
		LM_ERR("you must set 'received_avp' parameter. Must be same value as"
				" parameter 'received_avp' of registrar module\n");
		return -1;
	}
	return 0;
}


static struct mi_root* mi_enable_natping(struct mi_root* cmd_tree, 
											void* param )
{
	unsigned int value;
	struct mi_node* node;

	if (natping_state==NULL)
		return init_mi_tree( 400, MI_PING_DISABLED, MI_PING_DISABLED_LEN);

	node = cmd_tree->node.kids;
	if(node == NULL)
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	value = 0;
	if( strno2int( &node->value, &value) <0)
		goto error;

	(*natping_state) = value?1:0;
	
	return init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
error:
	return init_mi_tree( 400, MI_BAD_PARM_S, MI_BAD_PARM_LEN);
}



static int init_raw_socket(void)
{
	int on = 1;

	raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (raw_sock ==-1) {
		LM_ERR("cannot create raw socket\n");
		return -1;
	}

	if (setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
		LM_ERR("cannot set socket options\n");
		return -1;
	}

	return raw_sock;
}


static int get_natping_socket(char *socket, 
										unsigned int *ip, unsigned short *port)
{
	struct hostent* he;
	str host;
	int lport;
	int lproto;

	if (parse_phostport( socket, &host.s, &host.len, &lport, &lproto)!=0){
		LM_CRIT("invalid natping_socket parameter <%s>\n",natping_socket);
		return -1;
	}

	if (lproto!=PROTO_UDP && lproto!=PROTO_NONE) {
		LM_CRIT("natping_socket can be only UDP <%s>\n",natping_socket);
		return 0;
	}
	lproto = PROTO_UDP;
	*port = lport?(unsigned short)lport:SIP_PORT;

	he = sip_resolvehost( &host, port, (char*)(void*)&lproto);
	if (he==0) {
		LM_ERR("could not resolve hostname:\"%.*s\"\n", host.len, host.s);
		return -1;
	}
	if (he->h_addrtype != AF_INET) {
		LM_ERR("only ipv4 addresses allowed in natping_socket\n");
		return -1;
	}

	memcpy( ip, he->h_addr_list[0], he->h_length);

	return 0;
}


static int
mod_init(void)
{
	int i;
	bind_usrloc_t bind_usrloc;
	struct in_addr addr;
	str socket_str;
	pv_spec_t avp_spec;
	str s;

	if(register_mi_mod(exports.name, mi_cmds)!=0)
	{
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	if (rcv_avp_param && *rcv_avp_param) {
		s.s = rcv_avp_param; s.len = strlen(s.s);
		if (pv_parse_spec(&s, &avp_spec)==0
				|| avp_spec.type!=PVT_AVP) {
			LM_ERR("malformed or non AVP %s AVP definition\n", rcv_avp_param);
			return -1;
		}

		if(pv_get_avp_name(0, &avp_spec.pvp, &rcv_avp_name, &rcv_avp_type)!=0)
		{
			LM_ERR("[%s]- invalid AVP definition\n", rcv_avp_param);
			return -1;
		}
	} else {
		rcv_avp_name.n = 0;
		rcv_avp_type = 0;
	}

	if (force_socket_str) {
		socket_str.s=force_socket_str;
		socket_str.len=strlen(socket_str.s);
		force_socket=grep_sock_info(&socket_str,0,0);
	}

	/* create raw socket? */
	if (natping_socket && natping_socket[0]) {
		if (get_natping_socket( natping_socket, &raw_ip, &raw_port)!=0)
			return -1;
		if (init_raw_socket() < 0)
			return -1;
	}

	if (nortpproxy_str.s==NULL || nortpproxy_str.s[0]==0) {
		nortpproxy_str.len = 0;
		nortpproxy_str.s = NULL;
	} else {
		nortpproxy_str.len = strlen(nortpproxy_str.s);
		while (nortpproxy_str.len > 0 && (nortpproxy_str.s[nortpproxy_str.len - 1] == '\r' ||
			nortpproxy_str.s[nortpproxy_str.len - 1] == '\n'))
				nortpproxy_str.len--;
		if (nortpproxy_str.len == 0)
			nortpproxy_str.s = NULL;
	}

	if (natping_interval > 0) {
		bind_usrloc = (bind_usrloc_t)find_export("ul_bind_usrloc", 1, 0);
		if (!bind_usrloc) {
			LM_ERR("can't find usrloc module\n");
			return -1;
		}

		if (bind_usrloc(&ul) < 0) {
			return -1;
		}

		natping_state =(unsigned int *) shm_malloc(sizeof(unsigned int));
		if (!natping_state) {
			LM_ERR("no shmem left\n");
			return -1;
		}
		*natping_state = MI_DEFAULT_NATPING_STATE;

		if (ping_nated_only && ul.nat_flag==0) {
			LM_ERR("bad config - ping_nated_only enabled, but no nat bflag"
				" set in usrloc module\n");
			return -1;
		}
		if (natping_processes>=8) {
			LM_ERR("too many natping processes (%d) max=8\n",
				natping_processes);
			return -1;
		}

		sipping_flag = (sipping_flag==-1)?0:(1<<sipping_flag);

		/* set reply function if SIP natping is enabled */
		if (sipping_flag) {
			if (sipping_from.s==0 || sipping_from.s[0]==0) {
				LM_ERR("SIP ping enabled, but SIP ping FROM is empty!\n");
				return -1;
			}
			if (sipping_method.s==0 || sipping_method.s[0]==0) {
				LM_ERR("SIP ping enabled, but SIP ping method is empty!\n");
				return -1;
			}
			sipping_method.len = strlen(sipping_method.s);
			sipping_from.len = strlen(sipping_from.s);
			exports.response_f = sipping_rpl_filter;
			init_sip_ping();
		}

		register_dummy_timers(natping_processes);
	}

	/* Prepare 1918 networks list */
	for (i = 0; nets_1918[i].cnetaddr != NULL; i++) {
		if (inet_aton(nets_1918[i].cnetaddr, &addr) != 1)
			abort();
		nets_1918[i].netaddr = ntohl(addr.s_addr) & nets_1918[i].mask;
	}

	return 0;
}


static int
child_init(int rank)
{
	int i;

	if (rank==PROC_MAIN && natping_interval > 0) {
		for( i=0 ; i<natping_processes ; i++ ) {
			if(fork_dummy_timer(PROC_TIMER, "TIMER NH", 1 /*socks flag*/,
								nh_timer, (void*)(unsigned long)i,
								1 /*sec*/)<0) {
				LM_ERR("failed to register timer routine as process\n");
				return -1;
				/* error */
			}
		}
	}

	if (rank<=0 && rank!=PROC_TIMER)
		return 0;

	/* Iterate known RTP proxies - create sockets */
	mypid = getpid();

	return 0;
}


static void mod_destroy(void)
{
	/*free the shared memory*/
	if (natping_state)
		shm_free(natping_state);
}



static int
isnulladdr(str *sx, int pf)
{
	char *cp;

	if (pf == AF_INET6) {
		for(cp = sx->s; cp < sx->s + sx->len; cp++)
			if (*cp != '0' && *cp != ':')
				return 0;
		return 1;
	}
	return (sx->len == 7 && memcmp("0.0.0.0", sx->s, 7) == 0);
}

/*
 * Replaces ip:port pair in the Contact: field with the source address
 * of the packet.
 */
static int
fix_nated_contact_f(struct sip_msg* msg, char* str1, char* str2)
{
	int offset, len, len1;
	char *cp, *buf, temp[2];
	contact_t *c;
	struct lump *anchor;
	struct sip_uri uri;
	str hostport;

	if (get_contact_uri(msg, &uri, &c) == -1)
		return -1;
	if ((c->uri.s < msg->buf) || (c->uri.s > (msg->buf + msg->len))) {
		LM_ERR("you can't call fix_nated_contact twice, "
		    "check your config!\n");
		return -1;
	}

	offset = c->uri.s - msg->buf;
	anchor = del_lump(msg, offset, c->uri.len, HDR_CONTACT_T);
	if (anchor == 0)
		return -1;

	hostport = uri.host;
	if (uri.port.len > 0)
		hostport.len = uri.port.s + uri.port.len - uri.host.s;

	cp = ip_addr2a(&msg->rcv.src_ip);
	len = c->uri.len + strlen(cp) + 6 /* :port */ - hostport.len + 1;
	buf = pkg_malloc(len);
	if (buf == NULL) {
		LM_ERR("out of pkg memory\n");
		return -1;
	}
	temp[0] = hostport.s[0];
	temp[1] = c->uri.s[c->uri.len];
	c->uri.s[c->uri.len] = hostport.s[0] = '\0';
	len1 = snprintf(buf, len, "%s%s:%d%s", c->uri.s, cp, msg->rcv.src_port,
	    hostport.s + hostport.len);
	if (len1 < len)
		len = len1;
	hostport.s[0] = temp[0];
	c->uri.s[c->uri.len] = temp[1];
	if (insert_new_lump_after(anchor, buf, len, HDR_CONTACT_T) == 0) {
		pkg_free(buf);
		return -1;
	}
	c->uri.s = buf;
	c->uri.len = len;

	return 1;
}


#define SALIAS        ";alias="
#define SALIAS_LEN (sizeof(SALIAS) - 1)

/*
 * Adds ;alias=ip:port param to contact uri containing received ip:port
 * if contact uri ip:port does not match received ip:port.
 */
static int
add_contact_alias_f(struct sip_msg* msg, char* str1, char* str2)
{
    int len, param_len, ip_len;
    contact_t *c;
    struct lump *anchor;
    struct sip_uri uri;
    struct ip_addr *ip;
    char *lt, *gt, *param, *at, *port, *start;

    /* Do nothing if Contact header does not exist */
    if (!msg->contact) {
	if (parse_headers(msg, HDR_CONTACT_F, 0) == -1)  {
	    LM_ERR("while parsing headers\n");
	    return -1;
	}
	if (!msg->contact) {
	    LM_DBG("no contact header\n");
	    return 2;
	}
    }
    if (get_contact_uri(msg, &uri, &c) == -1) {
	LM_ERR("failed to get contact uri\n");
	return -1;
    }

    /* Compare source ip and port against contact uri */
    if ((ip = str2ip(&(uri.host))) == NULL) {
	LM_DBG("contact uri host is not an ip address\n");
    } else {
	if (ip_addr_cmp(ip, &(msg->rcv.src_ip)) &&
	    ((msg->rcv.src_port == uri.port_no) ||
	     ((uri.port.len == 0) && (msg->rcv.src_port == 5060)))) {
	    LM_DBG("no need to add alias param\n");
	    return 2;
	}
    }
	
    /* Check if function has been called already */
    if ((c->uri.s < msg->buf) || (c->uri.s > (msg->buf + msg->len))) {
	LM_ERR("you can't call alias_contact twice, check your config!\n");
	return -1;
    }

    /* Check if Contact URI needs to be enclosed in <>s */
    lt = gt = param = NULL;
    at = memchr(msg->contact->body.s, '<', msg->contact->body.len);
    if (at == NULL) {
	lt = (char*)pkg_malloc(1);
	if (!lt) {
	    LM_ERR("no pkg memory left for lt sign\n");
	    goto err;
	}
	*lt = '<';
	anchor = anchor_lump(msg, msg->contact->body.s - msg->buf, 0, 0);
	if (anchor == NULL) {
	    LM_ERR("anchor_lump for beginning of contact body failed\n");
	    goto err;
	}
	if (insert_new_lump_before(anchor, lt, 1, 0) == 0) {
	    LM_ERR("insert_new_lump_before for \"<\" failed\n");
	    goto err;
	}
	gt = (char*)pkg_malloc(1);
	if (!gt) {
	    LM_ERR("no pkg memory left for gt sign\n");
	    goto err;
	}
	*gt = '>';
	anchor = anchor_lump(msg, msg->contact->body.s +
			     msg->contact->body.len - msg->buf, 0, 0);
	if (anchor == NULL) {
	    LM_ERR("anchor_lump for end of contact body failed\n");
	    goto err;
	}
	if (insert_new_lump_before(anchor, gt, 1, 0) == 0) {
	    LM_ERR("insert_new_lump_before for \">\" failed\n");
	    goto err;
	}
    }

    /* Create  ;alias param */
    param_len = SALIAS_LEN + IP6_MAX_STR_SIZE + 1 /* ~ */ + 5 /* port */ +
	1 /* ~ */ + 1 /* proto */;
    param = (char*)pkg_malloc(param_len);
    if (!param) {
	LM_ERR("no pkg memory left for alias param\n");
	goto err;
    }
    at = param;
    append_str(at, SALIAS, SALIAS_LEN);
    ip_len = ip_addr2sbuf(&(msg->rcv.src_ip), at, param_len - SALIAS_LEN);
    if (ip_len <= 0) {
	LM_ERR("failed to copy source ip\n");
	goto err;
    }
    at = at + ip_len;
    append_chr(at, '~');
    port = int2str(msg->rcv.src_port, &len);
    append_str(at, port, len);
    append_chr(at, '~');
    if ((msg->rcv.proto < PROTO_UDP) || (msg->rcv.proto > PROTO_SCTP)) {
	LM_ERR("invalid transport protocol\n");
	goto err;
    }
    append_chr(at, msg->rcv.proto + '0');
    param_len = at - param;
    LM_DBG("adding param <%.*s>\n", param_len, param);
    if (uri.port.len > 0) {
	start = uri.port.s + uri.port.len;
    } else {
	start = uri.host.s + uri.host.len;
    }

    /* Add  ;alias param */
    anchor = anchor_lump(msg, start - msg->buf, 0, 0);
    if (anchor == NULL) {
	LM_ERR("anchor_lump for ;alias param failed\n");
	goto err;
    }
    if (insert_new_lump_after(anchor, param, param_len, 0) == 0) {
	LM_ERR("insert_new_lump_after for ;alias param failed\n");
	goto err;
    }
    return 1;

 err:
    if (lt) pkg_free(lt);
    if (gt) pkg_free(gt);
    if (param) pkg_free(param);
    return -1;
}


#define ALIAS        "alias="
#define ALIAS_LEN (sizeof(ALIAS) - 1)

/*
 * Checks if r-uri has alias param and if so, removes it and sets $du
 * based on its value.
 */
static int
handle_ruri_alias_f(struct sip_msg* msg, char* str1, char* str2)
{
    str uri, proto;
    char buf[MAX_URI_SIZE], *val, *sep, *at, *next, *cur_uri, *rest,
	*port, *trans;
    unsigned int len, rest_len, val_len, alias_len, proto_type, cur_uri_len,
	ip_port_len;

    if (parse_sip_msg_uri(msg) < 0) {
	LM_ERR("while parsing Request-URI\n");
	return -1;
    }
    rest = msg->parsed_uri.sip_params.s;
    rest_len = msg->parsed_uri.sip_params.len;
    if (rest_len == 0) {
	LM_DBG("no params\n");
	return 2;
    }
    while (rest_len >= ALIAS_LEN) {
	if (strncmp(rest, ALIAS, ALIAS_LEN) == 0) break;
	sep = memchr(rest, 59 /* ; */, rest_len);
	if (sep == NULL) {
	    LM_DBG("no alias param\n");
	    return 2;
	} else {
	    rest_len = rest_len - (sep - rest + 1);
	    rest = sep + 1;
	}
    }

    if (rest_len < ALIAS_LEN) {
	LM_DBG("no alias param\n");
	return 2;
    }

    /* set dst uri based on alias param value */
    val = rest + ALIAS_LEN;
    val_len = rest_len - ALIAS_LEN;
    port = memchr(val, 126 /* ~ */, val_len);
    if (port == NULL) {
	LM_ERR("no '~' in alias param value\n");
	return -1;
    }
    *(port++) = ':';
    trans = memchr(port, 126 /* ~ */, val_len - (port - val));
    if (trans == NULL) {
	LM_ERR("no second '~' in alias param value\n");
	return -1;
    }
    at = &(buf[0]);
    append_str(at, "sip:", 4);
    ip_port_len = trans - val;
    alias_len = SALIAS_LEN + ip_port_len + 2 /* ~n */;
    memcpy(at, val, ip_port_len);
    at = at + ip_port_len;
    trans = trans + 1;
    if ((ip_port_len + 2 > val_len) || (*trans == ';') || (*trans == '?')) {
	LM_ERR("no proto in alias param\n");
	return -1;
    }
    proto_type = *trans - 48 /* char 0 */;
    if (proto_type != PROTO_UDP) {
	proto_type_to_str(proto_type, &proto);
	if (proto.len == 0) {
	    LM_ERR("unknown proto in alias param\n");
	    return -1;
	}
	append_str(at, ";transport=", 11);
	memcpy(at, proto.s, proto.len);
	at = at + proto.len;
    }
    next = trans + 1;
    if ((ip_port_len + 2 < val_len) && (*next != ';') && (*next != '?')) {
	LM_ERR("invalid alias param value\n");
	return -1;
    }
    uri.s = &(buf[0]);
    uri.len = at - &(buf[0]);
    LM_DBG("setting dst_uri to <%.*s>\n", uri.len, uri.s);
    if (set_dst_uri(msg, &uri) == -1) {
	LM_ERR("failed to set dst uri\n");
	return -1;
    }
    
    /* remove alias param */
    if (msg->new_uri.s) {
	cur_uri = msg->new_uri.s;
	cur_uri_len = msg->new_uri.len;
    } else {
	cur_uri = msg->first_line.u.request.uri.s;
	cur_uri_len = msg->first_line.u.request.uri.len;
    }
    at = &(buf[0]);
    len = rest - 1 /* ; */ - cur_uri;
    memcpy(at, cur_uri, len);
    at = at + len;
    len = cur_uri_len - alias_len - len;
    memcpy(at, rest + alias_len - 1, len);
    uri.s = &(buf[0]);
    uri.len = cur_uri_len - alias_len;
    LM_DBG("rewriting r-uri to <%.*s>\n", uri.len, uri.s);
    return rewrite_uri(msg, &uri);
}


/*
 * Counts and return the number of record routes in rr headers of the message.
 */
static int
pv_get_rr_count_f(struct sip_msg *msg, pv_param_t *param,
		  pv_value_t *res)
{
    unsigned int count;
    struct hdr_field *header;
    rr_t *body;

    if (msg == NULL) return -1;

    if (parse_headers(msg, HDR_EOH_F, 0) == -1) {
	LM_ERR("while parsing message\n");
	return -1;
    }

    count = 0;
    header = msg->record_route;

    while (header) {
	if (header->type == HDR_RECORDROUTE_T) {
	    if (parse_rr(header) == -1) {
		LM_ERR("while parsing rr header\n");
		return -1;
	    }
	    body = (rr_t *)header->parsed;
	    while (body) {
		count++;
		body = body->next;
	    }
	}
	header = header->next;
    }

    return pv_get_uintval(msg, param, res, (unsigned int)count);
}

/*
 * Return count of topmost record routes in rr headers of the message.
 */
static int
pv_get_rr_top_count_f(struct sip_msg *msg, pv_param_t *param,
		      pv_value_t *res)
{
    str uri;
    struct sip_uri puri;

    if (msg == NULL) return -1;

    if (!msg->record_route &&
	(parse_headers(msg, HDR_RECORDROUTE_F, 0) == -1)) {
	LM_ERR("while parsing Record-Route header\n");
	return -1;
    }

    if (!msg->record_route) {
	return pv_get_uintval(msg, param, res, 0);
    }
    
    if (parse_rr(msg->record_route) == -1) {
	LM_ERR("while parsing rr header\n");
	return -1;
    }

    uri = ((rr_t *)msg->record_route->parsed)->nameaddr.uri;
    if (parse_uri(uri.s, uri.len, &puri) < 0) {
	LM_ERR("while parsing rr uri\n");
	return -1;
    }
	
    if (puri.r2.len > 0) {
	return pv_get_uintval(msg, param, res, 2);
    } else {
	return pv_get_uintval(msg, param, res, 1);
    }
}

/*
 * Test if IP address in netaddr belongs to RFC1918 networks
 * netaddr in network byte order
 */
static inline int
is1918addr_n(uint32_t netaddr)
{
	int i;
	uint32_t hl;

	hl = ntohl(netaddr);
	for (i = 0; nets_1918[i].cnetaddr != NULL; i++) {
		if ((hl & nets_1918[i].mask) == nets_1918[i].netaddr) {
			return 1;
		}
	}
	return 0;
}

/*
 * Test if IP address pointed to by saddr belongs to RFC1918 networks
 */
static inline int
is1918addr(str *saddr)
{
	struct in_addr addr;
	int rval;
	char backup;

	rval = -1;
	backup = saddr->s[saddr->len];
	saddr->s[saddr->len] = '\0';
	if (inet_aton(saddr->s, &addr) != 1)
		goto theend;
	rval = is1918addr_n(addr.s_addr);

theend:
	saddr->s[saddr->len] = backup;
	return rval;
}

/*
 * Test if IP address pointed to by ip belongs to RFC1918 networks
 */
static inline int
is1918addr_ip(struct ip_addr *ip)
{
	if (ip->af != AF_INET)
		return 0;
	return is1918addr_n(ip->u.addr32[0]);
}

/*
 * test for occurrence of RFC1918 IP address in Contact HF
 */
static int
contact_1918(struct sip_msg* msg)
{
	struct sip_uri uri;
	contact_t* c;

	if (get_contact_uri(msg, &uri, &c) == -1)
		return -1;

	return (is1918addr(&(uri.host)) == 1) ? 1 : 0;
}

/*
 * test for occurrence of RFC1918 IP address in SDP
 */
static int
sdp_1918(struct sip_msg* msg)
{
	str body, ip;
	int pf;

	if (extract_body(msg, &body) == -1) {
		LM_ERR("cannot extract body from msg!\n");
		return 0;
	}
	if (extract_mediaip(&body, &ip, &pf,"c=") == -1) {
		LM_ERR("can't extract media IP from the SDP\n");
		return 0;
	}
	if (pf != AF_INET || isnulladdr(&ip, pf))
		return 0;

	return (is1918addr(&ip) == 1) ? 1 : 0;
}

/*
 * test for occurrence of RFC1918 IP address in top Via
 */
static int
via_1918(struct sip_msg* msg)
{

	return (is1918addr(&(msg->via1->host)) == 1) ? 1 : 0;
}

static int
nat_uac_test_f(struct sip_msg* msg, char* str1, char* str2)
{
	int tests;

	tests = (int)(long)str1;

	/* return true if any of the NAT-UAC tests holds */

	/* test if the source port is different from the port in Via */
	if ((tests & NAT_UAC_TEST_RPORT) &&
		 (msg->rcv.src_port!=(msg->via1->port?msg->via1->port:SIP_PORT)) ){
		return 1;
	}
	/*
	 * test if source address of signaling is different from
	 * address advertised in Via
	 */
	if ((tests & NAT_UAC_TEST_RCVD) && received_test(msg))
		return 1;
	/*
	 * test for occurrences of RFC1918 addresses in Contact
	 * header field
	 */
	if ((tests & NAT_UAC_TEST_C_1918) && (contact_1918(msg)>0))
		return 1;
	/*
	 * test for occurrences of RFC1918 addresses in SDP body
	 */
	if ((tests & NAT_UAC_TEST_S_1918) && sdp_1918(msg))
		return 1;
	/*
	 * test for occurrences of RFC1918 addresses top Via
	 */
	if ((tests & NAT_UAC_TEST_V_1918) && via_1918(msg))
		return 1;

	/*
	 * test for occurrences of RFC1918 addresses in source address
	 */
	if ((tests & NAT_UAC_TEST_O_1918) && is1918addr_ip(&msg->rcv.src_ip))
		return 1;

	/* no test succeeded */
	return -1;

}

#define	ADD_ADIRECTION	0x01
#define	FIX_MEDIP	0x02
#define	ADD_ANORTPPROXY	0x04
#define	FIX_ORGIP	0x08

#define	ADIRECTION	"a=direction:active"
#define	ADIRECTION_LEN	(sizeof(ADIRECTION) - 1)

#define	AOLDMEDIP	"a=oldmediaip:"
#define	AOLDMEDIP_LEN	(sizeof(AOLDMEDIP) - 1)

#define	AOLDMEDIP6	"a=oldmediaip6:"
#define	AOLDMEDIP6_LEN	(sizeof(AOLDMEDIP6) - 1)

#define	AOLDMEDPRT	"a=oldmediaport:"
#define	AOLDMEDPRT_LEN	(sizeof(AOLDMEDPRT) - 1)


static inline int 
replace_sdp_ip(struct sip_msg* msg, str *org_body, char *line, str *ip)
{
	str body1, oldip, newip;
	str body = *org_body;
	unsigned hasreplaced = 0;
	int pf, pf1 = 0;
	str body2;
	char *bodylimit = body.s + body.len;

	/* Iterate all lines and replace ips in them. */
	if (!ip) {
		newip.s = ip_addr2a(&msg->rcv.src_ip);
		newip.len = strlen(newip.s);
	} else {
		newip = *ip;
	}
	body1 = body;
	for(;;) {
		if (extract_mediaip(&body1, &oldip, &pf,line) == -1)
			break;
		if (pf != AF_INET) {
			LM_ERR("not an IPv4 address in '%s' SDP\n",line);
				return -1;
			}
		if (!pf1)
			pf1 = pf;
		else if (pf != pf1) {
			LM_ERR("mismatching address families in '%s' SDP\n",line);
			return -1;
		}
		body2.s = oldip.s + oldip.len;
		body2.len = bodylimit - body2.s;
		if (alter_mediaip(msg, &body1, &oldip, pf, &newip, pf,1) == -1) {
			LM_ERR("can't alter '%s' IP\n",line);
			return -1;
		}
		hasreplaced = 1;
		body1 = body2;
	}
	if (!hasreplaced) {
		LM_ERR("can't extract '%s' IP from the SDP\n",line);
		return -1;
	}

	return 0;
}

static int
fix_nated_sdp_f(struct sip_msg* msg, char* str1, char* str2)
{
	str body;
	str ip;
	int level;
	char *buf;
	struct lump* anchor;

	level = (int)(long)str1;
	if (str2 && pv_printf_s( msg, (pv_elem_p)str2, &ip)!=0)
		return -1;

	if (extract_body(msg, &body) == -1) {
		LM_ERR("cannot extract body from msg!\n");
		return -1;
	}

	if (level & (ADD_ADIRECTION | ADD_ANORTPPROXY)) {
		msg->msg_flags |= FL_FORCE_ACTIVE;
		anchor = anchor_lump(msg, body.s + body.len - msg->buf, 0, 0);
		if (anchor == NULL) {
			LM_ERR("anchor_lump failed\n");
			return -1;
		}
		if (level & ADD_ADIRECTION) {
			buf = pkg_malloc((ADIRECTION_LEN + CRLF_LEN) * sizeof(char));
			if (buf == NULL) {
				LM_ERR("out of pkg memory\n");
				return -1;
			}
			memcpy(buf, CRLF, CRLF_LEN);
			memcpy(buf + CRLF_LEN, ADIRECTION, ADIRECTION_LEN);
			if (insert_new_lump_after(anchor, buf, ADIRECTION_LEN + CRLF_LEN, 0) == NULL) {
				LM_ERR("insert_new_lump_after failed\n");
				pkg_free(buf);
				return -1;
			}
		}

		if ((level & ADD_ANORTPPROXY) && nortpproxy_str.len) {
			buf = pkg_malloc((nortpproxy_str.len + CRLF_LEN) * sizeof(char));
			if (buf == NULL) {
				LM_ERR("out of pkg memory\n");
				return -1;
			}
			memcpy(buf, CRLF, CRLF_LEN);
			memcpy(buf + CRLF_LEN, nortpproxy_str.s, nortpproxy_str.len);
			if (insert_new_lump_after(anchor, buf, nortpproxy_str.len + CRLF_LEN, 0) == NULL) {
				LM_ERR("insert_new_lump_after failed\n");
				pkg_free(buf);
				return -1;
			}
		}

	}

	if (level & FIX_MEDIP) {
		/* Iterate all c= and replace ips in them. */
		if (replace_sdp_ip(msg, &body, "c=", str2?&ip:0)==-1)
			return -1;
	}

	if (level & FIX_ORGIP) {
		/* Iterate all o= and replace ips in them. */
		if (replace_sdp_ip(msg, &body, "o=", str2?&ip:0)==-1)
			return -1;
	}

	return 1;
}

static int
extract_mediaip(str *body, str *mediaip, int *pf, char *line)
{
	char *cp, *cp1;
	int len, nextisip;

	cp1 = NULL;
	for (cp = body->s; (len = body->s + body->len - cp) > 0;) {
		cp1 = ser_memmem(cp, line, len, 2);
		if (cp1 == NULL || cp1[-1] == '\n' || cp1[-1] == '\r')
			break;
		cp = cp1 + 2;
	}
	if (cp1 == NULL)
		return -1;

	mediaip->s = cp1 + 2;
	mediaip->len = eat_line(mediaip->s, body->s + body->len - mediaip->s) - mediaip->s;
	trim_len(mediaip->len, mediaip->s, *mediaip);

	nextisip = 0;
	for (cp = mediaip->s; cp < mediaip->s + mediaip->len;) {
		len = eat_token_end(cp, mediaip->s + mediaip->len) - cp;
		if (nextisip == 1) {
			mediaip->s = cp;
			mediaip->len = len;
			nextisip++;
			break;
		}
		if (len == 3 && memcmp(cp, "IP", 2) == 0) {
			switch (cp[2]) {
			case '4':
				nextisip = 1;
				*pf = AF_INET;
				break;

			case '6':
				nextisip = 1;
				*pf = AF_INET6;
				break;

			default:
				break;
			}
		}
		cp = eat_space_end(cp + len, mediaip->s + mediaip->len);
	}
	if (nextisip != 2 || mediaip->len == 0) {
		LM_ERR("no `IP[4|6]' in `%s' field\n",line);
		return -1;
	}
	return 1;
}

static int
alter_mediaip(struct sip_msg *msg, str *body, str *oldip, int oldpf,
  str *newip, int newpf, int preserve)
{
	char *buf;
	int offset;
	struct lump* anchor;
	str omip, nip, oip;

	/* check that updating mediaip is really necessary */
	if (oldpf == newpf && isnulladdr(oldip, oldpf))
		return 0;
	if (newip->len == oldip->len &&
	    memcmp(newip->s, oldip->s, newip->len) == 0)
		return 0;

	if (preserve != 0) {
		anchor = anchor_lump(msg, body->s + body->len - msg->buf, 0, 0);
		if (anchor == NULL) {
			LM_ERR("anchor_lump failed\n");
			return -1;
		}
		if (oldpf == AF_INET6) {
			omip.s = AOLDMEDIP6;
			omip.len = AOLDMEDIP6_LEN;
		} else {
			omip.s = AOLDMEDIP;
			omip.len = AOLDMEDIP_LEN;
		}
		buf = pkg_malloc(omip.len + oldip->len + CRLF_LEN);
		if (buf == NULL) {
			LM_ERR("out of pkg memory\n");
			return -1;
		}
		memcpy(buf, CRLF, CRLF_LEN);
		memcpy(buf + CRLF_LEN, omip.s, omip.len);
		memcpy(buf + CRLF_LEN + omip.len, oldip->s, oldip->len);
		if (insert_new_lump_after(anchor, buf,
		    omip.len + oldip->len + CRLF_LEN, 0) == NULL) {
			LM_ERR("insert_new_lump_after failed\n");
			pkg_free(buf);
			return -1;
		}
	}

	if (oldpf == newpf) {
		nip.len = newip->len;
		nip.s = pkg_malloc(nip.len);
		if (nip.s == NULL) {
			LM_ERR("out of pkg memory\n");
			return -1;
		}
		memcpy(nip.s, newip->s, newip->len);
	} else {
		nip.len = newip->len + 2;
		nip.s = pkg_malloc(nip.len);
		if (nip.s == NULL) {
			LM_ERR("out of pkg memory\n");
			return -1;
		}
		memcpy(nip.s + 2, newip->s, newip->len);
		nip.s[0] = (newpf == AF_INET6) ? '6' : '4';
		nip.s[1] = ' ';
	}

	oip = *oldip;
	if (oldpf != newpf) {
		do {
			oip.s--;
			oip.len++;
		} while (*oip.s != '6' && *oip.s != '4');
	}
	offset = oip.s - msg->buf;
	anchor = del_lump(msg, offset, oip.len, 0);
	if (anchor == NULL) {
		LM_ERR("del_lump failed\n");
		pkg_free(nip.s);
		return -1;
	}

	if (insert_new_lump_after(anchor, nip.s, nip.len, 0) == 0) {
		LM_ERR("insert_new_lump_after failed\n");
		pkg_free(nip.s);
		return -1;
	}
	return 0;
}


static u_short raw_checksum(unsigned char *buffer, int len)
{
	u_long sum = 0;

	while (len > 1) {
		sum += *buffer << 8;
		buffer++;
		sum += *buffer;
		buffer++;
		len -= 2;
	}
	if (len) {
		sum += *buffer << 8;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum);

	return (u_short) ~sum;
}


static int send_raw(const char *buf, int buf_len, union sockaddr_union *to,
							const unsigned int s_ip, const unsigned int s_port)
{
	struct ip *ip;
	struct udphdr *udp;
	unsigned char packet[50];
	int len = sizeof(struct ip) + sizeof(struct udphdr) + buf_len;

	if (len > sizeof(packet)) {
		LM_ERR("payload too big\n");
		return -1;
	}

	ip = (struct ip*) packet;
	udp = (struct udphdr *) (packet + sizeof(struct ip));
	memcpy(packet + sizeof(struct ip) + sizeof(struct udphdr), buf, buf_len);

	ip->ip_v = 4;
	ip->ip_hl = sizeof(struct ip) / 4; // no options
	ip->ip_tos = 0;
	ip->ip_len = htons(len);
	ip->ip_id = 23;
	ip->ip_off = 0;
	ip->ip_ttl = 69;
	ip->ip_p = 17;
	ip->ip_src.s_addr = s_ip;
	ip->ip_dst.s_addr = to->sin.sin_addr.s_addr;

	ip->ip_sum = raw_checksum((unsigned char *) ip, sizeof(struct ip));

	udp->uh_sport = htons(s_port);
	udp->uh_dport = to->sin.sin_port;
	udp->uh_ulen = htons((unsigned short) sizeof(struct udphdr) + buf_len);
	udp->uh_sum = 0;

	return sendto(raw_sock, packet, len, 0, (struct sockaddr *) to, sizeof(struct sockaddr_in));
}


static void
nh_timer(unsigned int ticks, void *timer_idx)
{
	static unsigned int iteration = 0;
	int rval;
	void *buf, *cp;
	str c;
	str opt;
	str path;
	struct sip_uri curi;
	struct hostent* he;
	struct socket_info* send_sock;
	unsigned int flags;
	char proto;
	struct dest_info dst;

	if((*natping_state) == 0)
		goto done;

	buf = NULL;
	if (cblen > 0) {
		buf = pkg_malloc(cblen);
		if (buf == NULL) {
			LM_ERR("out of pkg memory\n");
			goto done;
		}
	}
	rval = ul.get_all_ucontacts(buf, cblen, (ping_nated_only?ul.nat_flag:0),
		((unsigned int)(unsigned long)timer_idx)*natping_interval+iteration,
		natping_processes*natping_interval);
	if (rval<0) {
		LM_ERR("failed to fetch contacts\n");
		goto done;
	}
	if (rval > 0) {
		if (buf != NULL)
			pkg_free(buf);
		cblen = rval * 2;
		buf = pkg_malloc(cblen);
		if (buf == NULL) {
			LM_ERR("out of pkg memory\n");
			goto done;
		}
		rval = ul.get_all_ucontacts(buf,cblen,(ping_nated_only?ul.nat_flag:0),
		   ((unsigned int)(unsigned long)timer_idx)*natping_interval+iteration,
		   natping_processes*natping_interval);
		if (rval != 0) {
			pkg_free(buf);
			goto done;
		}
	}

	if (buf == NULL)
		goto done;

	cp = buf;
	while (1) {
		memcpy(&(c.len), cp, sizeof(c.len));
		if (c.len == 0)
			break;
		c.s = (char*)cp + sizeof(c.len);
		cp =  (char*)cp + sizeof(c.len) + c.len;
		memcpy( &send_sock, cp, sizeof(send_sock));
		cp = (char*)cp + sizeof(send_sock);
		memcpy( &flags, cp, sizeof(flags));
		cp = (char*)cp + sizeof(flags);
		memcpy( &(path.len), cp, sizeof(path.len));
		path.s = path.len ? ((char*)cp + sizeof(path.len)) : NULL ;
		cp =  (char*)cp + sizeof(path.len) + path.len;

		/* determin the destination */
		if ( path.len && (flags&sipping_flag)!=0 ) {
			/* send to first URI in path */
			if (get_path_dst_uri( &path, &opt) < 0) {
				LM_ERR("failed to get dst_uri for Path\n");
				continue;
			}
			/* send to the contact/received */
			if (parse_uri(opt.s, opt.len, &curi) < 0) {
				LM_ERR("can't parse contact dst_uri\n");
				continue;
			}
		} else {
			/* send to the contact/received */
			if (parse_uri(c.s, c.len, &curi) < 0) {
				LM_ERR("can't parse contact uri\n");
				continue;
			}
		}
		if (curi.proto != PROTO_UDP && curi.proto != PROTO_NONE)
			continue;
		if (curi.port_no == 0)
			curi.port_no = SIP_PORT;
		proto = curi.proto;
		/* we sholud get rid of this resolve (to ofen and to slow); for the
		 * moment we are lucky since the curi is an IP -bogdan */
		he = sip_resolvehost(&curi.host, &curi.port_no, &proto);
		if (he == NULL){
			LM_ERR("can't resolve_host\n");
			continue;
		}
		init_dest_info(&dst);
		hostent2su(&dst.to, he, 0, curi.port_no);
		if (send_sock==0) {
			send_sock=force_socket ? force_socket : 
					get_send_socket(0, &dst.to, PROTO_UDP);
		}
		if (send_sock == NULL) {
			LM_ERR("can't get sending socket\n");
			continue;
		}
		dst.proto=PROTO_UDP;
		dst.send_sock=send_sock;

		if ( (flags&sipping_flag)!=0 &&
		(opt.s=build_sipping( &c, send_sock, &path, &opt.len))!=0 ) {
			if (udp_send(&dst, opt.s, opt.len)<0){
				LM_ERR("sip udp_send failed\n");
			}
		} else if (raw_ip) {
			if (send_raw((char*)sbuf, sizeof(sbuf), &dst.to, raw_ip, 
						 raw_port)<0) {
				LM_ERR("send_raw failed\n");
			}
		} else {
			if (udp_send(&dst, (char *)sbuf, sizeof(sbuf))<0 ) {
				LM_ERR("udp_send failed\n");
			}
		}
	}
	pkg_free(buf);
done:
	iteration++;
	if (iteration==natping_interval)
		iteration = 0;
}


/*
 * Create received SIP uri that will be either
 * passed to registrar in an AVP or apended
 * to Contact header field as a parameter
 */
static int
create_rcv_uri(str* uri, struct sip_msg* m)
{
	static char buf[MAX_URI_SIZE];
	char* p;
	str ip, port;
	int len;
	str proto;

	if (!uri || !m) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}

	ip.s = ip_addr2a(&m->rcv.src_ip);
	ip.len = strlen(ip.s);

	port.s = int2str(m->rcv.src_port, &port.len);

	switch(m->rcv.proto) {
	case PROTO_NONE:
	case PROTO_UDP:
		proto.s = 0; /* Do not add transport parameter, UDP is default */
		proto.len = 0;
		break;

	case PROTO_TCP:
		proto.s = "TCP";
		proto.len = 3;
		break;

	case PROTO_TLS:
		proto.s = "TLS";
		proto.len = 3;
		break;

	case PROTO_SCTP:
		proto.s = "SCTP";
		proto.len = 4;
		break;

	default:
		LM_ERR("unknown transport protocol\n");
		return -1;
	}

	len = 4 + ip.len + 2*(m->rcv.src_ip.af==AF_INET6)+ 1 + port.len;
	if (proto.s) {
		len += TRANSPORT_PARAM_LEN;
		len += proto.len;
	}

	if (len > MAX_URI_SIZE) {
		LM_ERR("buffer too small\n");
		return -1;
	}

	p = buf;
	memcpy(p, "sip:", 4);
	p += 4;
	
	if (m->rcv.src_ip.af==AF_INET6)
		*p++ = '[';
	memcpy(p, ip.s, ip.len);
	p += ip.len;
	if (m->rcv.src_ip.af==AF_INET6)
		*p++ = ']';

	*p++ = ':';
	
	memcpy(p, port.s, port.len);
	p += port.len;

	if (proto.s) {
		memcpy(p, TRANSPORT_PARAM, TRANSPORT_PARAM_LEN);
		p += TRANSPORT_PARAM_LEN;

		memcpy(p, proto.s, proto.len);
		p += proto.len;
	}

	uri->s = buf;
	uri->len = len;

	return 0;
}


/*
 * Add received parameter to Contacts for further
 * forwarding of the REGISTER requuest
 */
static int
add_rcv_param_f(struct sip_msg* msg, char* str1, char* str2)
{
	contact_t* c;
	struct lump* anchor;
	char* param;
	str uri;
	int hdr_param;

	hdr_param = str1?0:1;

	if (create_rcv_uri(&uri, msg) < 0) {
		return -1;
	}

	if (contact_iterator(&c, msg, 0) < 0) {
		return -1;
	}

	while(c) {
		param = (char*)pkg_malloc(RECEIVED_LEN + 2 + uri.len);
		if (!param) {
			LM_ERR("no pkg memory left\n");
			return -1;
		}
		memcpy(param, RECEIVED, RECEIVED_LEN);
		param[RECEIVED_LEN] = '\"';
		memcpy(param + RECEIVED_LEN + 1, uri.s, uri.len);
		param[RECEIVED_LEN + 1 + uri.len] = '\"';

		if (hdr_param) {
			/* add the param as header param */
			anchor = anchor_lump(msg, c->name.s + c->len - msg->buf, 0, 0);
		} else {
			/* add the param as uri param */
			anchor = anchor_lump(msg, c->uri.s + c->uri.len - msg->buf, 0, 0);
		}
		if (anchor == NULL) {
			LM_ERR("anchor_lump failed\n");
			return -1;
		}		

		if (insert_new_lump_after(anchor, param, RECEIVED_LEN + 1 + uri.len + 1, 0) == 0) {
			LM_ERR("insert_new_lump_after failed\n");
			pkg_free(param);
			return -1;
		}

		if (contact_iterator(&c, msg, c) < 0) {
			return -1;
		}
	}

	return 1;
}



/*
 * Create an AVP to be used by registrar with the source IP and port
 * of the REGISTER
 */
static int
fix_nated_register_f(struct sip_msg* msg, char* str1, char* str2)
{
	str uri;
	int_str val;

	if(rcv_avp_name.n==0)
		return 1;

	if (create_rcv_uri(&uri, msg) < 0) {
		return -1;
	}

	val.s = uri;

	if (add_avp(AVP_VAL_STR|rcv_avp_type, rcv_avp_name, val) < 0) {
		LM_ERR("failed to create AVP\n");
		return -1;
	}

	return 1;
}
