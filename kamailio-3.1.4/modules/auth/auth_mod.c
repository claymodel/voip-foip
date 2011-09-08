/*
 * $Id$
 *
 * Digest Authentication Module
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * --------
 * 2003-02-26 checks and group moved to separate modules (janakj)
 * 2003-03-10 New module interface (janakj)
 * 2003-03-16 flags export parameter added (janakj)
 * 2003-03-19 all mallocs/frees replaced w/ pkg_malloc/pkg_free (andrei)
 * 2003-04-28 rpid contributed by Juha Heinanen added (janakj)
 * 2007-10-19 auth extra checks: longer nonces that include selected message
 *            parts to protect against various reply attacks without keeping
 *            state (andrei)
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../parser/digest/digest.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../error.h"
#include "../../ut.h"
#include "../../modules/sl/sl.h"
#include "auth_mod.h"
#include "challenge.h"
#include "api.h"
#include "nid.h"
#include "nc.h"
#include "ot_nonce.h"
#include "rfc2617.h"

MODULE_VERSION

#define RAND_SECRET_LEN 32


/*
 * Module destroy function prototype
 */
static void destroy(void);

/*
 * Module initialization function prototype
 */
static int mod_init(void);

/*
 * Remove used credentials from a SIP message header
 */
int consume_credentials(struct sip_msg* msg, char* s1, char* s2);

static int pv_proxy_authenticate(struct sip_msg* msg, char* realm,
		char *passwd, char *flags);
static int pv_www_authenticate(struct sip_msg* msg, char* realm,
		char *passwd, char *flags);
static int fixup_pv_auth(void **param, int param_no);

static int proxy_challenge(struct sip_msg *msg, char* realm, char *flags);
static int www_challenge(struct sip_msg *msg, char* realm, char *flags);
static int fixup_auth_challenge(void **param, int param_no);


/*
 * Module parameter variables
 */
char* sec_param    = 0;     /* If the parameter was not used, the secret phrase will be auto-generated */
int   nonce_expire = 300;   /* Nonce lifetime */
/*int   auth_extra_checks = 0;  -- in nonce.c */
int   protect_contacts = 0; /* Do not include contacts in nonce by default */
int force_stateless_reply = 0; /* Always send reply statelessly */

str secret1;
str secret2;
char* sec_rand1 = 0;
char* sec_rand2 = 0;

str challenge_attr = STR_STATIC_INIT("$digest_challenge");
avp_ident_t challenge_avpid;

str proxy_challenge_header = STR_STATIC_INIT("Proxy-Authenticate");
str www_challenge_header = STR_STATIC_INIT("WWW-Authenticate");

struct qp auth_qop = {
    STR_STATIC_INIT("auth"),
    QOP_AUTH
};

static struct qp auth_qauth = {
    STR_STATIC_INIT("auth"),
    QOP_AUTH
};

static struct qp auth_qauthint = {
    STR_STATIC_INIT("auth-int"),
    QOP_AUTHINT
};

/*! SL API structure */
sl_api_t slb;

/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
    {"consume_credentials",    consume_credentials,                  0,
			0, REQUEST_ROUTE},
    {"www_challenge",          (cmd_function)www_challenge,          2,
			fixup_auth_challenge, REQUEST_ROUTE},
    {"proxy_challenge",        (cmd_function)proxy_challenge,        2,
			fixup_auth_challenge, REQUEST_ROUTE},
    {"pv_www_authorize",       (cmd_function)pv_www_authenticate,    3,
			fixup_pv_auth, REQUEST_ROUTE},
    {"pv_www_authenticate",    (cmd_function)pv_www_authenticate,    3,
			fixup_pv_auth, REQUEST_ROUTE},
    {"pv_proxy_authorize",     (cmd_function)pv_proxy_authenticate,  3,
			fixup_pv_auth, REQUEST_ROUTE},
    {"pv_proxy_authenticate",  (cmd_function)pv_proxy_authenticate,  3,
			fixup_pv_auth, REQUEST_ROUTE},
    {"bind_auth_s",           (cmd_function)bind_auth_s, 0, 0, 0        },
    {0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
    {"secret",                 PARAM_STRING, &sec_param             },
    {"nonce_expire",           PARAM_INT,    &nonce_expire          },
	{"nonce_auth_max_drift",   PARAM_INT,    &nonce_auth_max_drift  },
    {"protect_contacts",       PARAM_INT,    &protect_contacts      },
    {"challenge_attr",         PARAM_STR,    &challenge_attr        },
    {"proxy_challenge_header", PARAM_STR,    &proxy_challenge_header},
    {"www_challenge_header",   PARAM_STR,    &www_challenge_header  },
    {"qop",                    PARAM_STR,    &auth_qop.qop_str      },
	{"auth_checks_register",   PARAM_INT,    &auth_checks_reg       },
	{"auth_checks_no_dlg",     PARAM_INT,    &auth_checks_ood       },
	{"auth_checks_in_dlg",     PARAM_INT,    &auth_checks_ind       },
	{"nonce_count"  ,          PARAM_INT,    &nc_enabled            },
	{"nc_array_size",          PARAM_INT,    &nc_array_size         },
	{"nc_array_order",         PARAM_INT,    &nc_array_k            },
	{"one_time_nonce"  ,       PARAM_INT,    &otn_enabled           },
	{"otn_in_flight_no",       PARAM_INT,    &otn_in_flight_no      },
	{"otn_in_flight_order",    PARAM_INT,    &otn_in_flight_k       },
	{"nid_pool_no",            PARAM_INT,    &nid_pool_no            },
    {"force_stateless_reply",  PARAM_INT,    &force_stateless_reply },
    {0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
    "auth",
    cmds,
    0,          /* RPC methods */
    params,
    mod_init,   /* module initialization function */
    0,          /* response function */
    destroy,    /* destroy function */
    0,          /* oncancel function */
    0           /* child initialization function */
};


/*
 * Secret parameter was not used so we generate
 * a random value here
 */
static inline int generate_random_secret(void)
{
    int i;
    
    sec_rand1 = (char*)pkg_malloc(RAND_SECRET_LEN);
    sec_rand2 = (char*)pkg_malloc(RAND_SECRET_LEN);
    if (!sec_rand1 || !sec_rand2) {
	LOG(L_ERR, "auth:generate_random_secret: No memory left\n");
	if (sec_rand1){
		pkg_free(sec_rand1);
		sec_rand1=0;
	}
	return -1;
    }
    
    /* srandom(time(0));  -- seeded by core */
    
    for(i = 0; i < RAND_SECRET_LEN; i++) {
	sec_rand1[i] = 32 + (int)(95.0 * rand() / (RAND_MAX + 1.0));
    }
    
    secret1.s = sec_rand1;
    secret1.len = RAND_SECRET_LEN;
	
    for(i = 0; i < RAND_SECRET_LEN; i++) {
	sec_rand2[i] = 32 + (int)(95.0 * rand() / (RAND_MAX + 1.0));
    }
    
    secret2.s = sec_rand2;
    secret2.len = RAND_SECRET_LEN;
    
	 /* DBG("Generated secret: '%.*s'\n", secret.len, secret.s); */
    
    return 0;
}


static int mod_init(void)
{
    str attr;
    
    DBG("auth module - initializing\n");
    
	/* bind the SL API */
	if (sl_load_api(&slb)!=0) {
		LM_ERR("cannot bind to SL API\n");
		return -1;
	}

	/* If the parameter was not used */
    if (sec_param == 0) {
		/* Generate secret using random generator */
		if (generate_random_secret() < 0) {
			LOG(L_ERR, "auth:mod_init: Error while generating random secret\n");
			return -3;
		}
    } else {
		/* Otherwise use the parameter's value */
		secret1.s = sec_param;
		secret1.len = strlen(secret1.s);
		
		if (auth_checks_reg || auth_checks_ind || auth_checks_ood) {
			/* divide the secret in half: one half for secret1 and one half for
			 *  secret2 */
			secret2.len = secret1.len/2;
			secret1.len -= secret2.len;
			secret2.s = secret1.s + secret1.len;
			if (secret2.len < 16) {
				WARN("auth: consider a longer secret when extra auth checks are"
					 " enabled (the config secret is divided in 2!)\n");
			}
		}
    }
    
    if ((!challenge_attr.s || challenge_attr.len == 0) ||
		challenge_attr.s[0] != '$') {
		ERR("auth: Invalid value of challenge_attr module parameter\n");
		return -1;
    }
    
    attr.s = challenge_attr.s + 1;
    attr.len = challenge_attr.len - 1;
    
    if (parse_avp_ident(&attr, &challenge_avpid) < 0) {
		ERR("auth: Error while parsing value of challenge_attr module"
				" parameter\n");
		return -1;
    }
	
    parse_qop(&auth_qop);
	switch(auth_qop.qop_parsed){
		case QOP_OTHER:
			ERR("auth: Unsupported qop parameter value\n");
			return -1;
		case QOP_AUTH:
		case QOP_AUTHINT:
			if (nc_enabled){
#ifndef USE_NC
				WARN("auth: nounce count support enabled from config, but"
					" disabled at compile time (recompile with -DUSE_NC)\n");
				nc_enabled=0;
#else
				if (nid_crt==0)
					init_nonce_id();
				if (init_nonce_count()!=0)
					return -1;
#endif
			}
#ifdef USE_NC
			else{
				INFO("auth: qop set, but nonce-count (nc_enabled) support"
						" disabled\n");
			}
#endif
			break;
		default:
			if (nc_enabled){
				WARN("auth: nonce-count support enabled, but qop not set\n");
				nc_enabled=0;
			}
			break;
	}
	if (otn_enabled){
#ifdef USE_OT_NONCE
		if (nid_crt==0) init_nonce_id();
		if (init_ot_nonce()!=0) 
			return -1;
#else
		WARN("auth: one-time-nonce support enabled from config, but "
				"disabled at compile time (recompile with -DUSE_OT_NONCE)\n");
		otn_enabled=0;
#endif /* USE_OT_NONCE */
	}

    return 0;
}


static void destroy(void)
{
    if (sec_rand1) pkg_free(sec_rand1);
    if (sec_rand2) pkg_free(sec_rand2);
#ifdef USE_NC
	destroy_nonce_count();
#endif
#ifdef USE_OT_NONCE
	destroy_ot_nonce();
#endif
#if defined USE_NC || defined USE_OT_NONCE
	destroy_nonce_id();
#endif
}


/*
 * Remove used credentials from a SIP message header
 */
int consume_credentials(struct sip_msg* msg, char* s1, char* s2)
{
    struct hdr_field* h;
    int len;
    
    get_authorized_cred(msg->authorization, &h);
    if (!h) {
		get_authorized_cred(msg->proxy_auth, &h);
		if (!h) { 
			if (msg->REQ_METHOD != METHOD_ACK 
				&& msg->REQ_METHOD != METHOD_CANCEL) {
				LOG(L_ERR, "auth:consume_credentials: No authorized "
					"credentials found (error in scripts)\n");
			}
			return -1;
		}
    }
    
    len = h->len;
    
    if (del_lump(msg, h->name.s - msg->buf, len, 0) == 0) {
		LOG(L_ERR, "auth:consume_credentials: Can't remove credentials\n");
		return -1;
    }
    
    return 1;
}

/**
 * @brief do WWW-Digest authentication with password taken from cfg var
 */
static int pv_authenticate(struct sip_msg *msg, char *p1, char *p2,
		char *p3, int hftype)
{
    int flags = 0;
    str realm  = {0, 0};
    str passwd = {0, 0};
	struct hdr_field* h;
	auth_body_t* cred;
	int ret;
    str hf = {0, 0};
    avp_value_t val;
	static char ha1[256];
	struct qp *qop = NULL;

	cred = 0;
	ret = AUTH_ERROR;

	if (get_str_fparam(&realm, msg, (fparam_t*)p1) < 0) {
		LM_ERR("failed to get realm value\n");
		goto error;
	}

	if(realm.len==0) {
		LM_ERR("invalid realm value - empty content\n");
		goto error;
	}

	if (get_str_fparam(&passwd, msg, (fparam_t*)p2) < 0) {
		LM_ERR("failed to get passwd value\n");
		goto error;
	}

	if(passwd.len==0) {
		LM_ERR("invalid password value - empty content\n");
		goto error;
	}

	if (get_int_fparam(&flags, msg, (fparam_t*)p3) < 0) {
		LM_ERR("invalid flags value\n");
		goto error;
	}

	switch(pre_auth(msg, &realm, hftype, &h, NULL)) {
		case ERROR:
		case BAD_CREDENTIALS:
			LM_DBG("error or bad credentials\n");
			ret = AUTH_ERROR;
			goto end;
		case CREATE_CHALLENGE:
			LM_ERR("CREATE_CHALLENGE is not a valid state\n");
			ret = AUTH_ERROR;
			goto end;
		case DO_RESYNCHRONIZATION:
			LM_ERR("DO_RESYNCHRONIZATION is not a valid state\n");
			ret = AUTH_ERROR;
			goto end;
		case NOT_AUTHENTICATED:
			LM_DBG("not authenticated\n");
			ret = AUTH_ERROR;
			goto end;
		case DO_AUTHENTICATION:
			break;
		case AUTHENTICATED:
			ret = AUTH_OK;
			goto end;
	}

	cred = (auth_body_t*)h->parsed;

	/* compute HA1 if needed */
	if ((flags&1)==0) {
		/* Plaintext password is stored in PV, calculate HA1 */
		calc_HA1(HA_MD5, &cred->digest.username.whole, &realm,
				&passwd, 0, 0, ha1);
		LM_DBG("HA1 string calculated: %s\n", ha1);
	} else {
		memcpy(ha1, passwd.s, passwd.len);
		ha1[passwd.len] = '\0';
	}

	/* Recalculate response, it must be same to authorize successfully */
	ret = auth_check_response(&(cred->digest),
				&msg->first_line.u.request.method, ha1);
	if(ret==AUTHENTICATED) {
		ret = AUTH_OK;
		switch(post_auth(msg, h)) {
			case AUTHENTICATED:
				break;
			default:
				ret = AUTH_ERROR;
				break;
		}
	} else {
		if(ret==NOT_AUTHENTICATED)
			ret = AUTH_INVALID_PASSWORD;
		else
			ret = AUTH_ERROR;
	}

end:
	if (ret < 0) {
		/* check if required to add challenge header as avp */
		if(!(flags&14))
			return ret;
		if(flags&8) {
			qop = &auth_qauthint;
		} else if(flags&4) {
			qop = &auth_qauth;
		}
		if (get_challenge_hf(msg, (cred ? cred->stale : 0),
				&realm, NULL, NULL, qop, hftype, &hf) < 0) {
			ERR("Error while creating challenge\n");
			ret = AUTH_ERROR;
		} else {
			val.s = hf;
			if(add_avp(challenge_avpid.flags | AVP_VAL_STR,
							challenge_avpid.name, val) < 0) {
				LM_ERR("Error while creating attribute with challenge\n");
				ret = AUTH_ERROR;
			}
			pkg_free(hf.s);
		}
	}

error:
	return ret;

}

/**
 *
 */
static int pv_proxy_authenticate(struct sip_msg *msg, char* realm,
		char *passwd, char *flags)
{
	return pv_authenticate(msg, realm, passwd, flags, HDR_PROXYAUTH_T);
}

/**
 *
 */
static int pv_www_authenticate(struct sip_msg *msg, char* realm,
		char *passwd, char *flags)
{
	return pv_authenticate(msg, realm, passwd, flags, HDR_AUTHORIZATION_T);
}

/**
 * @brief fixup function for pv_{www,proxy}_authenticate
 */
static int fixup_pv_auth(void **param, int param_no)
{
	if(strlen((char*)*param)<=0) {
		LM_ERR("empty parameter %d not allowed\n", param_no);
		return -1;
	}

	switch(param_no) {
		case 1:
		case 2:
			return fixup_var_pve_str_12(param, 1);
		case 3:
			return fixup_var_int_12(param, 1);
	}
	return 0;
}


/**
 *
 */
static int auth_send_reply(struct sip_msg *msg, int code, char *reason,
					char *hdr, int hdr_len)
{
        str reason_str;

	/* Add new headers if there are any */
	if ((hdr!=NULL) && (hdr_len>0)) {
		if (add_lump_rpl(msg, hdr, hdr_len, LUMP_RPL_HDR)==0) {
			LM_ERR("failed to append hdr to reply\n");
			return -1;
		}
	}

	reason_str.s = reason;
	reason_str.len = strlen(reason);

	return force_stateless_reply ?
	    slb.sreply(msg, code, &reason_str) :
	    slb.freply(msg, code, &reason_str);
}

/**
 *
 */
static int auth_challenge(struct sip_msg *msg, char *p1, char *p2, int hftype)
{
    int flags = 0;
    str realm  = {0, 0};
	int ret;
    str hf = {0, 0};
	struct qp *qop = NULL;

	ret = -1;

	if (get_str_fparam(&realm, msg, (fparam_t*)p1) < 0) {
		LM_ERR("failed to get realm value\n");
		goto error;
	}

	if(realm.len==0) {
		LM_ERR("invalid realm value - empty content\n");
		goto error;
	}

	if (get_int_fparam(&flags, msg, (fparam_t*)p2) < 0) {
		LM_ERR("invalid flags value\n");
		goto error;
	}
	
	if(flags&2) {
		qop = &auth_qauthint;
	} else if(flags&1) {
		qop = &auth_qauth;
	}
	if (get_challenge_hf(msg, 0, &realm, NULL, NULL, qop, hftype, &hf) < 0) {
		ERR("Error while creating challenge\n");
		ret = -2;
		goto error;
	}
	
	ret = 1;
	switch(hftype) {
		case HDR_AUTHORIZATION_T:
			if(auth_send_reply(msg, 401, "Unauthorized",
						hf.s, hf.len) <0 )
				ret = -3;
		break;
		case HDR_PROXYAUTH_T:
			if(auth_send_reply(msg, 407, "Proxy Authentication Required",
						hf.s, hf.len) <0 )
				ret = -3;
		break;
	}
	if(hf.s) pkg_free(hf.s);
	return ret;

error:
	if(hf.s) pkg_free(hf.s);
	if(!(flags&4)) {
		if(auth_send_reply(msg, 500, "Internal Server Error", 0, 0) <0 )
			ret = -4;
	}
	return ret;
}

/**
 *
 */
static int proxy_challenge(struct sip_msg *msg, char* realm, char *flags)
{
	return auth_challenge(msg, realm, flags, HDR_PROXYAUTH_T);
}

/**
 *
 */
static int www_challenge(struct sip_msg *msg, char* realm, char *flags)
{
	return auth_challenge(msg, realm, flags, HDR_AUTHORIZATION_T);
}

/**
 * @brief fixup function for {www,proxy}_challenge
 */
static int fixup_auth_challenge(void **param, int param_no)
{
	if(strlen((char*)*param)<=0) {
		LM_ERR("empty parameter %d not allowed\n", param_no);
		return -1;
	}

	switch(param_no) {
		case 1:
			return fixup_var_str_12(param, 1);
		case 2:
			return fixup_var_int_12(param, 1);
	}
	return 0;
}
