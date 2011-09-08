/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2005, Mikael Magnusson
 *
 * Mikael Magnusson <mikma@users.sourceforge.net>
 *
 * See http://www.callweaver.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file res_srtp.c 
 *
 * \brief Secure RTP (SRTP)
 * 
 * Secure RTP (SRTP) 
 * Specified in RFC 3711.
 */

#include <srtp/srtp.h>
#include <srtp/srtp_priv.h>
#include "callweaver.h"

#include "callweaver/lock.h"
#include "callweaver/module.h"
#include "callweaver/options.h"
#include "callweaver/rtp.h"

struct cw_srtp {
	struct cw_rtp *rtp;
	srtp_t session;
	const struct cw_srtp_cb *cb;
	void *data;
	unsigned char buf[8192 + CW_FRIENDLY_OFFSET];
};

struct cw_srtp_policy {
	srtp_policy_t sp;
};

static const char desc[] = "Secure RTP (SRTP)";
static int srtpdebug = 1;
static int g_initialized = 0;

/* Exported functions */
int load_module(void);
int unload_module(void);
int usecount(void);
char *description(void);

/* SRTP functions */
static int res_srtp_create(struct cw_srtp **srtp,
			   struct cw_rtp *rtp,
			   struct cw_srtp_policy *policy);
static void res_srtp_destroy(struct cw_srtp *srtp);
static int res_srtp_add_stream(struct cw_srtp *srtp,
			       struct cw_srtp_policy *policy);

static int res_srtp_unprotect(struct cw_srtp *srtp, void *buf, int *len);
static int res_srtp_protect(struct cw_srtp *srtp, void **buf, int *len);
static int res_srtp_get_random(unsigned char *key, size_t len);
static void res_srtp_set_cb(struct cw_srtp *srtp,
			    const struct cw_srtp_cb *cb, void *data);

/* Policy functions */
static struct cw_srtp_policy *res_srtp_policy_alloc(void);
static void res_srtp_policy_destroy(struct cw_srtp_policy *policy);
static int res_srtp_policy_set_suite(struct cw_srtp_policy *policy,
				     enum cw_srtp_suite suite);
static int res_srtp_policy_set_master_key(struct cw_srtp_policy *policy,
			      const unsigned char *key, size_t key_len,
			      const unsigned char *salt, size_t salt_len);
static int res_srtp_policy_set_encr_alg(struct cw_srtp_policy *policy,
					enum cw_srtp_ealg ealg);
static int res_srtp_policy_set_auth_alg(struct cw_srtp_policy *policy,
					enum cw_srtp_aalg aalg);
static void res_srtp_policy_set_encr_keylen(struct cw_srtp_policy *policy,
					    int ekeyl);
static void res_srtp_policy_set_auth_keylen(struct cw_srtp_policy *policy,
					    int akeyl);
static void res_srtp_policy_set_srtp_auth_taglen(struct cw_srtp_policy *policy,
						 int autht);
static void res_srtp_policy_set_srtp_encr_enable(struct cw_srtp_policy *policy,
						 int enable);
static void res_srtp_policy_set_srtcp_encr_enable(struct cw_srtp_policy *policy,
						  int enable);
static void res_srtp_policy_set_srtp_auth_enable(struct cw_srtp_policy *policy,
						 int enable);
static void res_srtp_policy_set_ssrc(struct cw_srtp_policy *policy,
				     unsigned long ssrc, int inbound);

static struct cw_srtp_res srtp_res = {
	.create = res_srtp_create,
	.destroy = res_srtp_destroy,
	.add_stream = res_srtp_add_stream,
	.set_cb = res_srtp_set_cb,
	.unprotect = res_srtp_unprotect,
	.protect = res_srtp_protect,
	.get_random = res_srtp_get_random
};

static struct cw_srtp_policy_res policy_res = {
	.alloc = res_srtp_policy_alloc,
	.destroy = res_srtp_policy_destroy,
	.set_suite = res_srtp_policy_set_suite,
	.set_master_key = res_srtp_policy_set_master_key,
	.set_encr_alg = res_srtp_policy_set_encr_alg,
	.set_auth_alg = res_srtp_policy_set_auth_alg,
	.set_encr_keylen = res_srtp_policy_set_encr_keylen,
	.set_auth_keylen = res_srtp_policy_set_auth_keylen,
	.set_srtp_auth_taglen = res_srtp_policy_set_srtp_auth_taglen,
	.set_srtp_encr_enable = res_srtp_policy_set_srtp_encr_enable,
	.set_srtcp_encr_enable = res_srtp_policy_set_srtcp_encr_enable,
	.set_srtp_auth_enable = res_srtp_policy_set_srtp_auth_enable,
	.set_ssrc = res_srtp_policy_set_ssrc
};

static const char *srtp_errstr(int err)
{
	switch(err) {
	case err_status_ok:
		return "nothing to report";
	case err_status_fail:
		return "unspecified failure";
	case err_status_bad_param:
		return "unsupported parameter";
	case err_status_alloc_fail:
		return "couldn't allocate memory";
	case err_status_dealloc_fail:
		return "couldn't deallocate properly";
	case err_status_init_fail:
		return "couldn't initialize";
	case err_status_terminus:
		return "can't process as much data as requested";
	case err_status_auth_fail:
		return "authentication failure";
	case err_status_cipher_fail:
		return "cipher failure";
	case err_status_replay_fail:
		return "replay check failed (bad index)";
	case err_status_replay_old:
		return "replay check failed (index too old)";
	case err_status_algo_fail:
		return "algorithm failed test routine";
	case err_status_no_such_op:
		return "unsupported operation";
	case err_status_no_ctx:
		return "no appropriate context found";
	case err_status_cant_check:
		return "unable to perform desired validation";
	case err_status_key_expired:
		return "can't use key any more";
	default:
		return "unknown";
	}
}

static struct cw_srtp *res_srtp_new(void)
{
	struct cw_srtp *srtp = malloc(sizeof(*srtp));
	memset(srtp, 0, sizeof(*srtp));
	return srtp;
}

/*
  struct cw_srtp_policy
*/
static void srtp_event_cb(srtp_event_data_t *data)
{
cw_log(LOG_DEBUG, "Hello!\n");

	switch (data->event) {
	case event_ssrc_collision: {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "SSRC collision ssrc:%u dir:%d\n",
				ntohl(data->stream->ssrc),
				data->stream->direction);
		}
		break;
	}
	case event_key_soft_limit: {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "event_key_soft_limit\n");
		}
		break;
	}
	case event_key_hard_limit: {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "event_key_hard_limit\n");
		}
		break;
	}
	case event_packet_index_limit: {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "event_packet_index_limit\n");
		}
		break;
	}
	}
}

static void res_srtp_policy_set_ssrc(struct cw_srtp_policy *policy,
				     unsigned long ssrc, int inbound)
{
	if (ssrc) {
		policy->sp.ssrc.type = ssrc_specific;
		policy->sp.ssrc.value = ssrc;
	} else {
		policy->sp.ssrc.type =
			inbound ? ssrc_any_inbound : ssrc_any_outbound;
	}
}

static struct cw_srtp_policy *res_srtp_policy_alloc()
{
	struct cw_srtp_policy *tmp = malloc(sizeof(*tmp));

	memset(tmp, 0, sizeof(*tmp));
	return tmp;
}

static void
res_srtp_policy_destroy(struct cw_srtp_policy *policy)
{
	if (policy->sp.key) {
		free(policy->sp.key);
		policy->sp.key = NULL;
	}
	free(policy);
}

static int policy_set_suite(crypto_policy_t *p, enum cw_srtp_suite suite)
{
	switch (suite) {
	case CW_AES_CM_128_HMAC_SHA1_80:
		p->cipher_type = AES_128_ICM;
		p->cipher_key_len = 30;
		p->auth_type = HMAC_SHA1;
		p->auth_key_len = 20;
		p->auth_tag_len = 10;
		p->sec_serv = sec_serv_conf_and_auth;
		return 0;

	case CW_AES_CM_128_HMAC_SHA1_32:
		p->cipher_type = AES_128_ICM;
		p->cipher_key_len = 30;
		p->auth_type = HMAC_SHA1;
		p->auth_key_len = 20;
		p->auth_tag_len = 4;
		p->sec_serv = sec_serv_conf_and_auth;
		return 0;

	default:
		cw_log(LOG_ERROR, "Invalid crypto suite: %d\n", suite);
		return -1;
	}
}

static int
res_srtp_policy_set_suite(struct cw_srtp_policy *policy,
			  enum cw_srtp_suite suite)
{
	int res = policy_set_suite(&policy->sp.rtp, suite) |
		policy_set_suite(&policy->sp.rtcp, suite);

	return res;
}

static int
res_srtp_policy_set_master_key(struct cw_srtp_policy *policy,
			       const unsigned char *key, size_t key_len,
			       const unsigned char *salt, size_t salt_len)
{
	size_t size = key_len + salt_len;
	unsigned char *master_key = NULL;

	if (policy->sp.key) {
		free(policy->sp.key);
		policy->sp.key = NULL;
	}

	master_key = malloc(size);

	memcpy(master_key, key, key_len);
	memcpy(master_key + key_len, salt, salt_len);

	policy->sp.key = master_key;
	return 0;
}

static int
res_srtp_policy_set_encr_alg(struct cw_srtp_policy *policy,
			     enum cw_srtp_ealg ealg)
{
	int type = -1;

	switch (ealg) {
	case CW_MIKEY_SRTP_EALG_NULL:
		type = NULL_CIPHER;
		break;
	case CW_MIKEY_SRTP_EALG_AESCM:
		type = AES_128_ICM;
		break;
	default:
		return -1;
	}

	policy->sp.rtp.cipher_type = type;
	policy->sp.rtcp.cipher_type = type;
	return 0;
}

static int
res_srtp_policy_set_auth_alg(struct cw_srtp_policy *policy,
			     enum cw_srtp_aalg aalg)
{
	int type = -1;

	switch (aalg) {
	case CW_MIKEY_SRTP_AALG_NULL:
		type = NULL_AUTH;
		break;
	case CW_MIKEY_SRTP_AALG_SHA1HMAC:
		type = HMAC_SHA1;
		break;
	default:
		return -1;
	}

	policy->sp.rtp.auth_type = type;
	policy->sp.rtcp.auth_type = type;
	return 0;
}

static void
res_srtp_policy_set_encr_keylen(struct cw_srtp_policy *policy, int ekeyl)
{
	policy->sp.rtp.cipher_key_len = ekeyl;
	policy->sp.rtcp.cipher_key_len = ekeyl;
}

static void
res_srtp_policy_set_auth_keylen(struct cw_srtp_policy *policy, int akeyl)
{
	policy->sp.rtp.auth_key_len = akeyl;
	policy->sp.rtcp.auth_key_len = akeyl;
}

static void
res_srtp_policy_set_srtp_auth_taglen(struct cw_srtp_policy *policy, int autht)
{
	policy->sp.rtp.auth_tag_len = autht;
	policy->sp.rtcp.auth_tag_len = autht;
	
}

static void
res_srtp_policy_set_srtp_encr_enable(struct cw_srtp_policy *policy, int enable)
{
	int serv = enable ? sec_serv_conf : sec_serv_none;
	policy->sp.rtp.sec_serv = 
		(policy->sp.rtp.sec_serv & ~sec_serv_conf) | serv;
}

static void
res_srtp_policy_set_srtcp_encr_enable(struct cw_srtp_policy *policy, int enable)
{
	int serv = enable ? sec_serv_conf : sec_serv_none;
	policy->sp.rtcp.sec_serv = 
		(policy->sp.rtcp.sec_serv & ~sec_serv_conf) | serv;
}

static void
res_srtp_policy_set_srtp_auth_enable(struct cw_srtp_policy *policy, int enable)
{
	int serv = enable ? sec_serv_auth : sec_serv_none;
	policy->sp.rtp.sec_serv = 
		(policy->sp.rtp.sec_serv & ~sec_serv_auth) | serv;
}


static int res_srtp_get_random(unsigned char *key, size_t len)
{
	int res = crypto_get_random(key, len);

	return res != err_status_ok ? -1: 0;
}

static void res_srtp_set_cb(struct cw_srtp *srtp,
			    const struct cw_srtp_cb *cb, void *data)
{
	if (!srtp)
		return;
	
	srtp->cb = cb;
	srtp->data = data;
}


/* Vtable functions */

static int
res_srtp_unprotect(struct cw_srtp *srtp, void *buf, int *len)
{
	int res = 0;
	int i;

	for (i = 0; i < 2; i++) {
		srtp_hdr_t *header = buf;
		
		res = srtp_unprotect(srtp->session, buf, len);
		if (res != err_status_no_ctx)
			break;
		
		if (srtp->cb && srtp->cb->no_ctx) {
			if (srtp->cb->no_ctx(srtp->rtp, ntohl(header->ssrc), srtp->data) < 0) {
				break;
			}
			
		} else {
			break;
		}
	}
	
	if (res != err_status_ok) {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "SRTP unprotect: %s\n",
				 srtp_errstr(res));
		}
		return -1;
	}

	return *len;
}

static int
res_srtp_protect(struct cw_srtp *srtp, void **buf, int *len)
{
	int res = 0;
   	
	if ((*len + SRTP_MAX_TRAILER_LEN) > sizeof(srtp->buf))
		return -1;

	memcpy(srtp->buf, *buf, *len);

	res = srtp_protect(srtp->session, srtp->buf, len);

	if (res != err_status_ok) {
		if (option_debug || srtpdebug) {
			cw_log(LOG_DEBUG, "SRTP protect: %s\n",
				 srtp_errstr(res));
		}
		return -1;
	}

	*buf = srtp->buf;
	return *len;
}

static int
res_srtp_create(struct cw_srtp **srtp, struct cw_rtp *rtp,
		struct cw_srtp_policy *policy)
{
	int res;
	struct cw_srtp *temp = res_srtp_new();

	res = srtp_create(&temp->session, &policy->sp);
	if (res != err_status_ok) {
		return -1;
	}
	
	temp->rtp = rtp;
	*srtp = temp;

	return 0;
}

static void
res_srtp_destroy(struct cw_srtp *srtp)
{
	if (srtp->session) {
		srtp_dealloc(srtp->session);
	}

	free(srtp);
}

static int
res_srtp_add_stream(struct cw_srtp *srtp, struct cw_srtp_policy *policy)
{
	int res;
	
	res = srtp_add_stream(srtp->session, &policy->sp);
	if (res != err_status_ok)
		return -1;

	return 0;
}

static int res_srtp_init(void)
{
	int res;

	if (g_initialized)
		return 0;

	res = srtp_init();
	if (res != err_status_ok)
		return -1;
	
	srtp_install_event_handler(srtp_event_cb);

	return cw_rtp_register_srtp(&srtp_res, &policy_res);
}


/*
 * Exported functions
 */

int load_module(void)
{
	return  res_srtp_init();
}

int unload_module(void)
{
	return cw_rtp_unregister_srtp(&srtp_res, &policy_res);
}

int usecount(void)
{
	return 1;
}

char *description(void)
{
	return (char *)desc;
}
