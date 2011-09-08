/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
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
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif  

#include <stdio.h>
#include <callweaver/crypto.h>


/* Hrm, I wonder if the compiler is smart enough to only create two functions
   for all these...  I could force it to only make two, but those would be some
   really nasty looking casts. */

static struct cw_key *stub_cw_key_get(const char *kname, int ktype)
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return NULL;
}

static int stub_cw_check_signature(struct cw_key *key, const char *msg, const char *sig)
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return -1;
}

static int stub_cw_check_signature_bin(struct cw_key *key, const char *msg, int msglen, const unsigned char *sig)
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return -1;
}

static int stub_cw_sign(struct cw_key *key, char *msg, char *sig) 
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return -1;
}

static int stub_cw_sign_bin(struct cw_key *key, const char *msg, int msglen, unsigned char *sig)
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return -1;
}

static int stub_cw_encdec_bin(unsigned char *dst, const unsigned char *src, int srclen, struct cw_key *key)
{
	cw_log(LOG_NOTICE, "Crypto support not loaded!\n");
	return -1;
}

struct cw_key *(*cw_key_get)(const char *key, int type) = 
	stub_cw_key_get;

int (*cw_check_signature)(struct cw_key *key, const char *msg, const char *sig) =
	stub_cw_check_signature;
	
int (*cw_check_signature_bin)(struct cw_key *key, const char *msg, int msglen, const unsigned char *sig) =
	stub_cw_check_signature_bin;
	
int (*cw_sign)(struct cw_key *key, char *msg, char *sig) = 
	stub_cw_sign;

int (*cw_sign_bin)(struct cw_key *key, const char *msg, int msglen, unsigned char *sig) =
	stub_cw_sign_bin;
	
int (*cw_encrypt_bin)(unsigned char *dst, const unsigned char *src, int srclen, struct cw_key *key) =
	stub_cw_encdec_bin;

int (*cw_decrypt_bin)(unsigned char *dst, const unsigned char *src, int srclen, struct cw_key *key) =
	stub_cw_encdec_bin;
