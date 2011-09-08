/*
 * $Id$
 *
 * TLS module - virtual configuration domain support
 *
 * Copyright (C) 2001-2003 FhG FOKUS
 * Copyright (C) 2005,2006 iptelorg GmbH
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/** SIP-router TLS support :: Virtual domain configuration support.
 * @file
 * @ingroup tls
 * Module: @ref tls
 */


#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
# include <openssl/ui.h>
#endif
#include "../../ut.h"
#include "../../mem/shm_mem.h"
#include "../../pt.h"
#include "../../cfg/cfg.h"
#include "tls_server.h"
#include "tls_util.h"
#include "tls_mod.h"
#include "tls_init.h"
#include "tls_domain.h"
#include "tls_cfg.h"


/*
 * create a new domain 
 */
tls_domain_t* tls_new_domain(int type, struct ip_addr *ip, unsigned short port)
{
	tls_domain_t* d;

	d = shm_malloc(sizeof(tls_domain_t));
	if (d == NULL) {
		ERR("Memory allocation failure\n");
		return 0;
	}
	memset(d, '\0', sizeof(tls_domain_t));

	d->type = type;
	if (ip) memcpy(&d->ip, ip, sizeof(struct ip_addr));
	d->port = port;
	d->verify_cert = -1;
	d->verify_depth = -1;
	d->require_cert = -1;
	return d;
/*
 error:
	shm_free(d);
	return 0; */
}


/*
 * Free all memory used by configuration domain
 */
void tls_free_domain(tls_domain_t* d)
{
	int i;
	int procs_no;
	
	if (!d) return;
	if (d->ctx) {
		procs_no=get_max_procs();
		for(i = 0; i < procs_no; i++) {
			if (d->ctx[i]) SSL_CTX_free(d->ctx[i]);
		}
		shm_free(d->ctx);
	}

	if (d->cipher_list.s) shm_free(d->cipher_list.s);
	if (d->ca_file.s) shm_free(d->ca_file.s);
	if (d->crl_file.s) shm_free(d->crl_file.s);
	if (d->pkey_file.s) shm_free(d->pkey_file.s);
	if (d->cert_file.s) shm_free(d->cert_file.s);
	shm_free(d);
}


/*
 * clean up 
 */
void tls_free_cfg(tls_domains_cfg_t* cfg)
{
	tls_domain_t* p;
	while(cfg->srv_list) {
		p = cfg->srv_list;
		cfg->srv_list = cfg->srv_list->next;
		tls_free_domain(p);
	}
	while(cfg->cli_list) {
		p = cfg->cli_list;
		cfg->cli_list = cfg->cli_list->next;
		tls_free_domain(p);
	}
	if (cfg->srv_default) tls_free_domain(cfg->srv_default);
	if (cfg->cli_default) tls_free_domain(cfg->cli_default);
}



void tls_destroy_cfg(void)
{
	tls_domains_cfg_t* ptr;

	if (tls_domains_cfg_lock) {
		lock_destroy(tls_domains_cfg_lock);
		lock_dealloc(tls_domains_cfg_lock);
		tls_domains_cfg_lock = 0;
	}

	if (tls_domains_cfg) {
		while(*tls_domains_cfg) {
			ptr = *tls_domains_cfg;
			*tls_domains_cfg = (*tls_domains_cfg)->next;
			tls_free_cfg(ptr);
		}
		
		shm_free(tls_domains_cfg);
		tls_domains_cfg = 0;
	}
}



/*
 * Print TLS domain identifier
 */
char* tls_domain_str(tls_domain_t* d)
{
	static char buf[1024];
	char* p;

	buf[0] = '\0';
	p = buf;
	p = strcat(p, d->type & TLS_DOMAIN_SRV ? "TLSs<" : "TLSc<");
	if (d->type & TLS_DOMAIN_DEF) {
		p = strcat(p, "default>");
	} else {
		p = strcat(p, ip_addr2a(&d->ip));
		p = strcat(p, ":");
		p = strcat(p, int2str(d->port, 0));
		p = strcat(p, ">");
	}
	return buf;
}


/*
 * Initialize parameters that have not been configured from
 * parent domain (usually one of default domains
 */
static int fill_missing(tls_domain_t* d, tls_domain_t* parent)
{
	if (d->method == TLS_METHOD_UNSPEC) d->method = parent->method;
	LOG(L_INFO, "%s: tls_method=%d\n", tls_domain_str(d), d->method);
	
	if (d->method < 1 || d->method >= TLS_METHOD_MAX) {
		ERR("%s: Invalid TLS method value\n", tls_domain_str(d));
		return -1;
	}
	
	if (!d->cert_file.s) {
		if (shm_asciiz_dup(&d->cert_file.s, parent->cert_file.s) < 0)
			return -1;
		d->cert_file.len = parent->cert_file.len;
	}
	LOG(L_INFO, "%s: certificate='%s'\n", tls_domain_str(d), d->cert_file.s);
	
	if (!d->ca_file.s){
		if (shm_asciiz_dup(&d->ca_file.s, parent->ca_file.s) < 0)
			return -1;
		d->ca_file.len = parent->ca_file.len;
	}
	LOG(L_INFO, "%s: ca_list='%s'\n", tls_domain_str(d), d->ca_file.s);

	if (!d->crl_file.s) {
		if (shm_asciiz_dup(&d->crl_file.s, parent->crl_file.s) < 0)
			return -1;
		d->crl_file.len = parent->crl_file.len;
	}
	LOG(L_INFO, "%s: crl='%s'\n", tls_domain_str(d), d->crl_file.s);
	
	if (d->require_cert == -1) d->require_cert = parent->require_cert;
	LOG(L_INFO, "%s: require_certificate=%d\n", tls_domain_str(d),
			d->require_cert);
	
	if (!d->cipher_list.s) {
		if ( shm_asciiz_dup(&d->cipher_list.s, parent->cipher_list.s) < 0)
			return -1;
		d->cipher_list.len = parent->cipher_list.len;
	}
	LOG(L_INFO, "%s: cipher_list='%s'\n", tls_domain_str(d), d->cipher_list.s);
	
	if (!d->pkey_file.s) {
		if (shm_asciiz_dup(&d->pkey_file.s, parent->pkey_file.s) < 0)
			return -1;
		d->pkey_file.len = parent->pkey_file.len;
	}
	LOG(L_INFO, "%s: private_key='%s'\n", tls_domain_str(d), d->pkey_file.s);
	
	if (d->verify_cert == -1) d->verify_cert = parent->verify_cert;
	LOG(L_INFO, "%s: verify_certificate=%d\n", tls_domain_str(d),
			d->verify_cert);
	
	if (d->verify_depth == -1) d->verify_depth = parent->verify_depth;
	LOG(L_INFO, "%s: verify_depth=%d\n", tls_domain_str(d), d->verify_depth);

	return 0;
}


/* called for ctx, with 2 args.
 * should return 0 on succes, <0 on critical error.
 */
typedef int (*per_ctx_cbk_f)(SSL_CTX* ctx, long larg, void* parg);


/** execute callback on all the CTX'es on a domain.
 * @param d - domain
 * @param f - callback function
 * @param l - parameter passed to the callback
 * @param p - parameter passed to the callback
 * @return 0 on success, <0 on error.
 */
static int tls_domain_foreach_CTX(tls_domain_t* d, per_ctx_cbk_f ctx_cbk,
									long l1, void* p2)
{
	int i,ret;
	int procs_no;
	
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		if ((ret=ctx_cbk(d->ctx[i], l1, p2))<0)
			return ret;
	}
	return 0;
}


/** execute callback on all the CTX'es on in a domain list.
 * @param d - domain
 * @param f - callback function
 * @param l - parameter passed to the callback
 * @param p - parameter passed to the callback
 * @return 0 on success, <0 on error.
 */
static int tls_foreach_CTX_in_domain_lst(tls_domain_t* d,
										per_ctx_cbk_f ctx_cbk,
										long l1, void* p2)
{
	int ret;
	for (; d; d=d->next)
		if ((ret=tls_domain_foreach_CTX(d, ctx_cbk, l1, p2))<0)
			return ret;
	return 0;
}


/** execute callback on all the CTX'es in all the srv domains in a tls cfg.
 * @param cfg - tls cfg.
 * @param f - callback function
 * @param l - parameter passed to the callback
 * @param p - parameter passed to the callback
 * @return 0 on success, <0 on error.
 */
static int tls_foreach_CTX_in_srv_domains(tls_domains_cfg_t* cfg,
											per_ctx_cbk_f ctx_cbk,
											long l1, void* p2)
{
	int ret;
	if ((ret = tls_domain_foreach_CTX(cfg->srv_default, ctx_cbk, l1, p2)) < 0)
		return ret;
	if ((ret = tls_foreach_CTX_in_domain_lst(cfg->srv_list, ctx_cbk, l1, p2))
			< 0)
		return ret;
	return 0;
}


/** execute callback on all the CTX'es in all the client domains in a tls cfg.
 * @param cfg - tls cfg.
 * @param f - callback function
 * @param l - parameter passed to the callback
 * @param p - parameter passed to the callback
 * @return 0 on success, <0 on error.
 */
static int tls_foreach_CTX_in_cli_domains(tls_domains_cfg_t* cfg,
											per_ctx_cbk_f ctx_cbk,
											long l1, void* p2)
{
	int ret;
	if ((ret = tls_domain_foreach_CTX(cfg->cli_default, ctx_cbk, l1, p2)) < 0)
		return ret;
	if ((ret = tls_foreach_CTX_in_domain_lst(cfg->cli_list, ctx_cbk, l1, p2))
			< 0)
		return ret;
	return 0;
}


/** execute callback on all the CTX'es in all the domains in a tls cfg.
 * @param cfg - tls cfg
 * @param f - callback function
 * @param l - parameter passed to the callback
 * @param p - parameter passed to the callback
 * @return 0 on success, <0 on error.
 */
static int tls_foreach_CTX_in_cfg(tls_domains_cfg_t* cfg,
										per_ctx_cbk_f ctx_cbk,
										long l1, void* p2)
{
	int ret;

	if ((ret = tls_foreach_CTX_in_srv_domains(cfg, ctx_cbk, l1, p2)) < 0)
		return ret;
	if ((ret = tls_foreach_CTX_in_cli_domains(cfg, ctx_cbk, l1, p2)) < 0)
		return ret;
	return 0;
}




/** fix pathnames.
 * To be used when loading the domain key, cert, ca list a.s.o.
 * It will replace path with a fixed shm allocated version. Assumes path->s
 * was shm allocated.
 * @param path - path to be fixed. If it starts with '.' or '/' is left alone
 *               (forced "relative" or "absolute" path). Otherwise the path
 *               is considered to be relative to the main config file directory
 *               (e.g. for /etc/ser/ser.cfg => /etc/ser/<path>).
 * @return  0 on success, -1 on error.
 */
int fix_shm_pathname(str* path)
{
	str new_path;
	char* abs_path;
	
	if (path->s && path->len && *path->s != '.' && *path->s != '/') {
		abs_path = get_abs_pathname(0, path);
		if (abs_path == 0) return -1;
		new_path.len = strlen(abs_path);
		new_path.s = shm_malloc(new_path.len + 1);
		memcpy(new_path.s, abs_path, new_path.len);
		new_path.s[new_path.len] = 0;
		shm_free(path->s);
		*path = new_path;
	}
	return 0;
}



/* 
 * Load certificate from file 
 */
static int load_cert(tls_domain_t* d)
{
	int i;
	int procs_no;

	if (!d->cert_file.s || !d->cert_file.len) {
		DBG("%s: No certificate configured\n", tls_domain_str(d));
		return 0;
	}
	if (fix_shm_pathname(&d->cert_file) < 0)
		return -1;
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		if (!SSL_CTX_use_certificate_chain_file(d->ctx[i], d->cert_file.s)) {
			ERR("%s: Unable to load certificate file '%s'\n",
			    tls_domain_str(d), d->cert_file.s);
			TLS_ERR("load_cert:");
			return -1;
		}
		
	}
	return 0;
}


/* 
 * Load CA list from file 
 */
static int load_ca_list(tls_domain_t* d)
{
	int i;
	int procs_no;

	if (!d->ca_file.s || !d->ca_file.len) {
		DBG("%s: No CA list configured\n", tls_domain_str(d));
		return 0;
	}
	if (fix_shm_pathname(&d->ca_file) < 0)
		return -1;
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		if (SSL_CTX_load_verify_locations(d->ctx[i], d->ca_file.s, 0) != 1) {
			ERR("%s: Unable to load CA list '%s'\n", tls_domain_str(d),
					d->ca_file.s);
			TLS_ERR("load_ca_list:");
			return -1;
		}
		SSL_CTX_set_client_CA_list(d->ctx[i],
				SSL_load_client_CA_file(d->ca_file.s));
		if (SSL_CTX_get_client_CA_list(d->ctx[i]) == 0) {
			ERR("%s: Error while setting client CA list\n", tls_domain_str(d));
			TLS_ERR("load_ca_list:");
			return -1;
		}
	}
	return 0;
}


/*
 * Load CRL from file
 */
static int load_crl(tls_domain_t* d)
{
	int i;
	int procs_no;
	X509_STORE* store;

	if (!d->crl_file.s) {
		DBG("%s: No CRL configured\n", tls_domain_str(d));
		return 0;
	}
	if (fix_shm_pathname(&d->crl_file) < 0)
		return -1;
	LOG(L_INFO, "%s: Certificate revocation lists will be checked (%.*s)\n",
				tls_domain_str(d), d->crl_file.len, d->crl_file.s);
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		if (SSL_CTX_load_verify_locations(d->ctx[i], d->crl_file.s, 0) != 1) {
			ERR("%s: Unable to load certificate revocation list '%s'\n",
					tls_domain_str(d), d->crl_file.s);
			TLS_ERR("load_crl:");
			return -1;
		}
		store = SSL_CTX_get_cert_store(d->ctx[i]);
		X509_STORE_set_flags(store,
						X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
	}
	return 0;
}


#define C_DEF_NO_KRB5 "DEFAULT:!KRB5"
#define C_DEF_NO_KRB5_LEN (sizeof(C_DEF_NO_KRB5)-1)
#define C_NO_KRB5_SUFFIX ":!KRB5"
#define C_NO_KRB5_SUFFIX_LEN (sizeof(C_NO_KRB5_SUFFIX)-1)

/* 
 * Configure cipher list 
 */
static int set_cipher_list(tls_domain_t* d)
{
	int i;
	int procs_no;
	char* cipher_list;

	cipher_list=d->cipher_list.s;
#ifdef TLS_KSSL_WORKARROUND
	if (openssl_kssl_malloc_bug) { /* is openssl bug #1467 present ? */
		if (d->cipher_list.s==0) {
			/* use "DEFAULT:!KRB5" */
			cipher_list="DEFAULT:!KRB5";
		} else {
			/* append ":!KRB5" */
			cipher_list=shm_malloc(d->cipher_list.len+C_NO_KRB5_SUFFIX_LEN+1);
			if (cipher_list) {
				memcpy(cipher_list, d->cipher_list.s, d->cipher_list.len);
				memcpy(cipher_list+d->cipher_list.len, C_NO_KRB5_SUFFIX,
						C_NO_KRB5_SUFFIX_LEN);
				cipher_list[d->cipher_list.len+C_NO_KRB5_SUFFIX_LEN]=0;
				shm_free(d->cipher_list.s);
				d->cipher_list.s=cipher_list;
				d->cipher_list.len+=C_NO_KRB5_SUFFIX_LEN;
			}
		}
	}
#endif /* TLS_KSSL_WORKARROUND */
	if (!cipher_list) return 0;
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		if (SSL_CTX_set_cipher_list(d->ctx[i], cipher_list) == 0 ) {
			ERR("%s: Failure to set SSL context cipher list \"%s\"\n",
					tls_domain_str(d), cipher_list);
			return -1;
		}
	}
	return 0;
}


/* 
 * Enable/disable certificate verification 
 */
static int set_verification(tls_domain_t* d)
{
	int verify_mode, i;
	int procs_no;

	if (d->require_cert) {
		verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		LOG(L_INFO, "%s: %s MUST present valid certificate\n", 
			tls_domain_str(d), d->type & TLS_DOMAIN_SRV ? "Client" : "Server");
	} else {
		if (d->verify_cert) {
			verify_mode = SSL_VERIFY_PEER;
			if (d->type & TLS_DOMAIN_SRV) {
				LOG(L_INFO, "%s: IF client provides certificate then it"
						" MUST be valid\n", tls_domain_str(d));
			} else {
				LOG(L_INFO, "%s: Server MUST present valid certificate\n",
				     tls_domain_str(d));
			}
		} else {
			verify_mode = SSL_VERIFY_NONE;
			if (d->type & TLS_DOMAIN_SRV) {
				LOG(L_INFO, "%s: No client certificate required and no checks"
						" performed\n", tls_domain_str(d));
			} else {
				LOG(L_INFO, "%s: Server MAY present invalid certificate\n",
				     tls_domain_str(d));
			}
		}
	}
	
	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		SSL_CTX_set_verify(d->ctx[i], verify_mode, 0);
		SSL_CTX_set_verify_depth(d->ctx[i], d->verify_depth);
		
	}
	return 0;
}


/* 
 * Configure generic SSL parameters 
 */
static int set_ssl_options(tls_domain_t* d)
{
	int i;
	int procs_no;
	long options;
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
	long ssl_version;
	STACK_OF(SSL_COMP)* comp_methods;
#endif
	
	procs_no=get_max_procs();
	options=SSL_OP_ALL; /* all the bug workarrounds by default */
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
	options|=SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
				SSL_OP_CIPHER_SERVER_PREFERENCE;
#if		OPENSSL_VERSION_NUMBER >= 0x00908000L
	ssl_version=SSLeay();
	if ((ssl_version >= 0x0090800L) && (ssl_version < 0x0090803fL)){
		/* if 0.9.8 <= openssl version < 0.9.8c and compression support is
		 * enabled disable SSL_OP_TLS_BLOCK_PADDING_BUG (set by SSL_OP_ALL),
		 * see openssl #1204 http://rt.openssl.org/Ticket/Display.html?id=1204
		 */
		
		comp_methods=SSL_COMP_get_compression_methods();
		if (comp_methods && (sk_SSL_COMP_num(comp_methods) > 0)){
			options &= ~SSL_OP_TLS_BLOCK_PADDING_BUG;
			LOG(L_WARN, "tls: set_ssl_options: openssl "
					"SSL_OP_TLS_BLOCK_PADDING bug workaround enabled "
					"(openssl version %lx)\n", ssl_version);
		}else{
			LOG(L_INFO, "tls: set_ssl_options: detected openssl version (%lx) "
					" has the SSL_OP_TLS_BLOCK_PADDING bug, but compression "
					" is disabled so no workaround is needed\n", ssl_version);
		}
	}
#	endif
#endif
	for(i = 0; i < procs_no; i++) {
		SSL_CTX_set_options(d->ctx[i], options);
	}
	return 0;
}


/* 
 * Configure session cache parameters 
 */
static int set_session_cache(tls_domain_t* d)
{
	int i;
	int procs_no;
	str tls_session_id;
	
	procs_no=get_max_procs();
	tls_session_id=cfg_get(tls, tls_cfg, session_id);
	for(i = 0; i < procs_no; i++) {
		/* janakj: I am not sure if session cache makes sense in ser, session
		 * cache is stored in SSL_CTX and we have one SSL_CTX per process,
		 * thus sessions among processes will not be reused
		 */
		SSL_CTX_set_session_cache_mode(d->ctx[i],
				cfg_get(tls, tls_cfg, session_cache) ? SSL_SESS_CACHE_SERVER :
				SSL_SESS_CACHE_OFF);
		/* not really needed is SSL_SESS_CACHE_OFF */
		SSL_CTX_set_session_id_context(d->ctx[i],
					(unsigned char*)tls_session_id.s, tls_session_id.len);
	}
	return 0;
}



/** tls SSL_CTX_set_mode and SSL_CTX_clear_mode wrapper.
 *  @param mode - SSL_MODE_*.
 *  @param clear - if set to !=0 will do a clear, else (==0) a set.
 *  @return - 0 (always succeeds).
 */
int tls_ssl_ctx_mode(SSL_CTX* ctx, long mode, void* clear)
{
	if (clear)
#if OPENSSL_VERSION_NUMBER >= 0x01000000L || \
	defined SSL_CTX_clear_mode
		SSL_CTX_clear_mode(ctx, mode);
#else
	return -1;
#endif
	else
		SSL_CTX_set_mode(ctx, mode);
	return 0;
}



/** tls set ctx->free_list_max_len.
 *  @param val - value (<0 ignored).
 *  @return - 0 (always succeeds).
 */
int tls_ssl_ctx_set_freelist(SSL_CTX* ctx, long val, void* unused)
{
	if (val >= 0)
#if OPENSSL_VERSION_NUMBER >= 0x01000000L
#ifndef OPENSSL_NO_BUF_FREELISTS
		ctx->freelist_max_len = val;
#endif
#endif
#if defined (OPENSSL_NO_BUF_FREELISTS) || OPENSSL_VERSION_NUMBER < 0x01000000L
		return -1;
#endif
	return 0;
}

/** tls SSL_CTX_set_max_send_fragment wrapper.
 *  @param val - value (<0 ignored). Should be between 512 and 16k.
 *  @return  0 on success, < 0 on failure (invalid value)
 */
int tls_ssl_ctx_set_max_send_fragment(SSL_CTX* ctx, long val, void* unused)
{
	if (val >= 0)
#if OPENSSL_VERSION_NUMBER >= 0x00909000L
		return SSL_CTX_set_max_send_fragment(ctx, val) -1;
#else
		return -1;
#endif
	return 0;
}



/** tls SSL_CTX_set_read_ahead wrapper.
 *  @param val - value (<0 ignored, 0 or >0).
 *  @return  0 (always success).
 */
int tls_ssl_ctx_set_read_ahead(SSL_CTX* ctx, long val, void* unused)
{
	if (val >= 0)
		SSL_CTX_set_read_ahead(ctx, val);
	return 0;
}

/*
 * Initialize all domain attributes from default domains
 * if necessary
 */
static int fix_domain(tls_domain_t* d, tls_domain_t* def)
{
	int i;
	int procs_no;

	if (fill_missing(d, def) < 0) return -1;

	procs_no=get_max_procs();
	d->ctx = (SSL_CTX**)shm_malloc(sizeof(SSL_CTX*) * procs_no);
	if (!d->ctx) {
		ERR("%s: Cannot allocate shared memory\n", tls_domain_str(d));
		return -1;
	}
	memset(d->ctx, 0, sizeof(SSL_CTX*) * procs_no);
	for(i = 0; i < procs_no; i++) {
		d->ctx[i] = SSL_CTX_new((SSL_METHOD*)ssl_methods[d->method - 1]);
		if (d->ctx[i] == NULL) {
			ERR("%s: Cannot create SSL context\n", tls_domain_str(d));
			return -1;
		}
	}
	
	if (load_cert(d) < 0) return -1;
	if (load_ca_list(d) < 0) return -1;
	if (load_crl(d) < 0) return -1;
	if (set_cipher_list(d) < 0) return -1;
	if (set_verification(d) < 0) return -1;
	if (set_ssl_options(d) < 0) return -1;
	if (set_session_cache(d) < 0) return -1;
	return 0;
}


static int passwd_cb(char *buf, int size, int rwflag, void *filename)
{
#if OPENSSL_VERSION_NUMBER >= 0x00907000L	
	UI             *ui;
	const char     *prompt;
	
	ui = UI_new();
	if (ui == NULL)
		goto err;

	prompt = UI_construct_prompt(ui, "passphrase", filename);
	UI_add_input_string(ui, prompt, 0, buf, 0, size - 1);
	UI_process(ui);
	UI_free(ui);
	return strlen(buf);
 
 err:
	ERR("passwd_cb: Error in passwd_cb\n");
	if (ui) {
		UI_free(ui);
	}
	return 0;
	
#else
	if (des_read_pw_string(buf, size-1, "Enter Private Key password:", 0)) {
		ERR("Error in passwd_cb\n");
		return 0;
	}
	return strlen(buf);
#endif
}


#define NUM_RETRIES 3
/*
 * load a private key from a file 
 */
static int load_private_key(tls_domain_t* d)
{
	int idx, ret_pwd, i;
	int procs_no;
	
	if (!d->pkey_file.s || !d->pkey_file.len) {
		DBG("%s: No private key specified\n", tls_domain_str(d));
		return 0;
	}
	if (fix_shm_pathname(&d->pkey_file) < 0)
		return -1;

	procs_no=get_max_procs();
	for(i = 0; i < procs_no; i++) {
		SSL_CTX_set_default_passwd_cb(d->ctx[i], passwd_cb);
		SSL_CTX_set_default_passwd_cb_userdata(d->ctx[i], d->pkey_file.s);
		
		for(idx = 0, ret_pwd = 0; idx < NUM_RETRIES; idx++) {
			ret_pwd = SSL_CTX_use_PrivateKey_file(d->ctx[i], d->pkey_file.s,
					SSL_FILETYPE_PEM);
			if (ret_pwd) {
				break;
			} else {
				ERR("%s: Unable to load private key '%s'\n",
				    tls_domain_str(d), d->pkey_file.s);
				TLS_ERR("load_private_key:");
				continue;
			}
		}
		
		if (!ret_pwd) {
			ERR("%s: Unable to load private key file '%s'\n",
			    tls_domain_str(d), d->pkey_file.s);
			TLS_ERR("load_private_key:");
			return -1;
		}
		
		if (!SSL_CTX_check_private_key(d->ctx[i])) {
			ERR("%s: Key '%s' does not match the public key of the"
					" certificate\n", tls_domain_str(d), d->pkey_file.s);
			TLS_ERR("load_private_key:");
			return -1;
		}
	}		

	DBG("%s: Key '%s' successfuly loaded\n",
	    tls_domain_str(d), d->pkey_file.s);
	return 0;
}


/*
 * Initialize attributes of all domains from default domains
 * if necessary
 */
int tls_fix_domains_cfg(tls_domains_cfg_t* cfg, tls_domain_t* srv_defaults,
				tls_domain_t* cli_defaults)
{
	tls_domain_t* d;
	int ssl_mode_release_buffers;
	int ssl_freelist_max_len;
	int ssl_max_send_fragment;
	int ssl_read_ahead;

	if (!cfg->cli_default) {
		cfg->cli_default = tls_new_domain(TLS_DOMAIN_DEF | TLS_DOMAIN_CLI,
											0, 0);
	}

	if (!cfg->srv_default) {
		cfg->srv_default = tls_new_domain(TLS_DOMAIN_DEF | TLS_DOMAIN_SRV,
											0, 0);
	}

	if (fix_domain(cfg->srv_default, srv_defaults) < 0) return -1;
	if (fix_domain(cfg->cli_default, cli_defaults) < 0) return -1;

	d = cfg->srv_list;
	while (d) {
		if (fix_domain(d, srv_defaults) < 0) return -1;
		d = d->next;
	}

	d = cfg->cli_list;
	while (d) {
		if (fix_domain(d, cli_defaults) < 0) return -1;
		d = d->next;
	}

	     /* Ask for passwords as the last step */
	d = cfg->srv_list;
	while(d) {
		if (load_private_key(d) < 0) return -1;
		d = d->next;
	}

	d = cfg->cli_list;
	while(d) {
		if (load_private_key(d) < 0) return -1;
		d = d->next;
	}

	if (load_private_key(cfg->srv_default) < 0) return -1;
	if (load_private_key(cfg->cli_default) < 0) return -1;

	/* set various global per CTX options
	 * (done here to show possible missing features messages only once)
	 */
	ssl_mode_release_buffers = cfg_get(tls, tls_cfg, ssl_release_buffers);
	ssl_freelist_max_len = cfg_get(tls, tls_cfg, ssl_freelist_max);
	ssl_max_send_fragment = cfg_get(tls, tls_cfg, ssl_max_send_fragment);
	ssl_read_ahead = cfg_get(tls, tls_cfg, ssl_read_ahead);
#if OPENSSL_VERSION_NUMBER >= 0x01000000L
	/* set SSL_MODE_RELEASE_BUFFERS if ssl_mode_release_buffers !=0,
	   reset if == 0 and ignore if < 0 */
	/* only in >= 1.0.0 */
	if (ssl_mode_release_buffers >= 0 &&
		tls_foreach_CTX_in_cfg(cfg, tls_ssl_ctx_mode, SSL_MODE_RELEASE_BUFFERS,
								(void*)(long)(ssl_mode_release_buffers==0))
		< 0) {
		ERR("invalid ssl_release_buffers value (%d)\n",
				ssl_mode_release_buffers);
		return -1;
	}
#else
	if (ssl_mode_release_buffers > 0)
		ERR("cannot change openssl mode_release_buffers, the openssl version"
				" is too old (need at least 1.0.0)\n");
#endif
	/* only in >= 1.0.0 */
#if OPENSSL_VERSION_NUMBER >= 0x01000000L
#ifndef OPENSSL_NO_BUF_FREELISTS
	if (tls_foreach_CTX_in_cfg(cfg, tls_ssl_ctx_set_freelist,
								ssl_freelist_max_len, 0) < 0) {
		ERR("invalid ssl_freelist_max_len value (%d)\n",
				ssl_freelist_max_len);
		return -1;
	}
#endif
#endif
#if defined (OPENSSL_NO_BUF_FREELISTS) || OPENSSL_VERSION_NUMBER < 0x01000000L
	if (ssl_freelist_max_len >= 0)
		ERR("cannot change openssl freelist_max_len, openssl too old"
				"(needed at least 1.0.0) or compiled without freelist support"
				" (OPENSSL_NO_BUF_FREELIST)\n");
#endif
#if OPENSSL_VERSION_NUMBER >= 0x00909000L
	/* only in >= 0.9.9 */
	if (tls_foreach_CTX_in_cfg(cfg, tls_ssl_ctx_set_max_send_fragment,
								ssl_max_send_fragment, 0) < 0) {
		ERR("invalid ssl_max_send_fragment value (%d)\n",
				ssl_max_send_fragment);
		return -1;
	}
#else
	if (ssl_max_send_fragment > 0)
		ERR("cannot change openssl max_send_fragment, the openssl version"
				" is too old (need at least 0.9.9)\n");
#endif
	if (tls_foreach_CTX_in_cfg(cfg, tls_ssl_ctx_set_read_ahead,
								ssl_read_ahead, 0) < 0) {
		ERR("invalid ssl_read_ahead value (%d)\n", ssl_read_ahead);
		return -1;
	}
	/* set options for SSL_write:
		SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER - needed when queueing
		  clear text for a future write (WANTS_READ). In this case the
		  buffer address will change for the repeated SSL_write() and
		  without this option it will trigger the openssl sanity checks.
		SSL_MODE_ENABLE_PARTIAL_WRITE - needed to deal with potentially
		  huge multi-record writes that don't fit in the default buffer
		  (the default buffer must have space for at least 1 record) */
	if (tls_foreach_CTX_in_cfg(cfg, tls_ssl_ctx_mode,
								SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
								SSL_MODE_ENABLE_PARTIAL_WRITE,
								0) < 0) {
		ERR("could not set SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER and"
				" SSL_MODE_ENABLE_PARTIAL_WRITE\n");
		return -1;
	}

	return 0;
}


/*
 * Create new configuration structure
 */
tls_domains_cfg_t* tls_new_cfg(void)
{
	tls_domains_cfg_t* r;

	r = (tls_domains_cfg_t*)shm_malloc(sizeof(tls_domains_cfg_t));
	if (!r) {
		ERR("No memory left\n");
		return 0;
	}
	memset(r, 0, sizeof(tls_domains_cfg_t));
	return r;
}


/*
 * Lookup TLS configuration based on type, ip, and port
 */
tls_domain_t* tls_lookup_cfg(tls_domains_cfg_t* cfg, int type,
								struct ip_addr* ip, unsigned short port)
{
	tls_domain_t *p;

	if (type & TLS_DOMAIN_DEF) {
		if (type & TLS_DOMAIN_SRV) return cfg->srv_default;
		else return cfg->cli_default;
	} else {
		if (type & TLS_DOMAIN_SRV) p = cfg->srv_list;
		else p = cfg->cli_list;
	}

	while (p) {
		if ((p->port == port) && ip_addr_cmp(&p->ip, ip))
			return p;
		p = p->next;
	}

	     /* No matching domain found, return default */
	if (type & TLS_DOMAIN_SRV) return cfg->srv_default;
	else return cfg->cli_default;
}


/*
 * Check whether configuration domain exists
 */
static int domain_exists(tls_domains_cfg_t* cfg, tls_domain_t* d)
{
	tls_domain_t *p;

	if (d->type & TLS_DOMAIN_DEF) {
		if (d->type & TLS_DOMAIN_SRV) return cfg->srv_default != NULL;
		else return cfg->cli_default != NULL;
	} else {
		if (d->type & TLS_DOMAIN_SRV) p = cfg->srv_list;
		else p = cfg->cli_list;
	}

	while (p) {
		if ((p->port == d->port) && ip_addr_cmp(&p->ip, &d->ip))
			return 1;
		p = p->next;
	}

	return 0;
}


/*
 * Add a domain to the configuration set
 */
int tls_add_domain(tls_domains_cfg_t* cfg, tls_domain_t* d)
{
	if (!cfg) {
		ERR("TLS configuration structure missing\n");
		return -1;
	}

	     /* Make sure the domain does not exist */
	if (domain_exists(cfg, d)) return 1;

	if (d->type & TLS_DOMAIN_DEF) {
		if (d->type & TLS_DOMAIN_CLI) {
			cfg->cli_default = d;
		} else {
			cfg->srv_default = d;
		}
	} else {
		if (d->type & TLS_DOMAIN_SRV) {
			d->next = cfg->srv_list;
			cfg->srv_list = d;
		} else {
			d->next = cfg->cli_list;
			cfg->cli_list = d;
		}
	}
	return 0;
}
