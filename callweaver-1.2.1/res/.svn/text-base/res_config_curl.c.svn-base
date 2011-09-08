/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * res_config_curl.c <curl+curl plugin for portable configuration engine >
 *
 * Copyright (C) 2004-2006, Anthony Minessale II
 *
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h> 

#include <curl/curl.h>

#include "callweaver.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/config.h"
#include "callweaver/module.h"
#include "callweaver/lock.h"
#include "callweaver/options.h"
#include "callweaver/utils.h"

static char *tdesc = "CURL URL Based Configuration";
static const char *global_tmp_prefix = "/var/tmp/res_config_curl/";
static int global_cache_time = 0;
static int global_no_cache_neg = 1;
static char *global_config_file = "curl.conf";

static void *app_1;
static const char *name_1 = "URLFetch";
static const char *synopsis_1 = "Fetch Data from a URL";
static const char *syntax_1 = "URLFetch(url)";
static const char *desc_1 = "load a url that returns cw_config and set according chanvars\n"
;


STANDARD_LOCAL_USER;
LOCAL_USER_DECL;

struct config_data {
	struct cw_variable *vars;
	struct cw_config *cfg;
	const char *table;
	const char *database;
	const char *keyfield;
	const char *lookup;
	const char *file;
	const char *action;
	char url[1024];
	char cachefile[1152];
	int fd;
	int recur;
	int skip_prep;
	va_list aq;
};

static size_t realtime_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = size * nmemb;
	struct config_data *config_data = data;
	size_t len;
	char *line = NULL, *newline = NULL, *nextline = NULL;
	
	if ((line = cw_strdupa((char *) ptr))) {
		while (line) {

			if ((nextline = strchr(line, '\n'))) {
				*nextline = '\0';
				nextline++;
			}
			if (!strcmp(line, ";OK;")) {
				if (option_verbose > 2)
					cw_verbose(VERBOSE_PREFIX_3 "Open file %s\n", config_data->cachefile);
				if (!config_data->fd) 
					config_data->fd = open(config_data->cachefile, O_WRONLY | O_TRUNC | O_CREAT, 0600);
			} else {
				if (config_data->fd) {
					len = strlen(line);
					newline = alloca(len + 1);
					strncpy(newline, line, len);
					newline[len] = '\n';
					if (write(config_data->fd, newline, len+1) != len+1)
						break;
				}
			}
			line = nextline;
		}
	} else 
		cw_log(LOG_ERROR, "Memory Allocation Failed.\n");
	
	if (config_data->fd)
		close(config_data->fd);

	return realsize;
}

static void curl_process(struct config_data *config_data) {
	CURL *curl_handle = NULL;
	const char *param = NULL, *val = NULL;
	int x = 0, offset=0;
	struct cw_config *cfg; 
	struct cw_variable *v, *vp;
	struct stat sbuf;
	int elapsed = 0;
	int perform = 1;

	strncpy(config_data->url, config_data->database, sizeof(config_data->url));

	if(! config_data->skip_prep) {
		if (strchr(config_data->url, '?'))
			x++;
		if (config_data->file) {
			if (config_data->url[strlen(config_data->url)-1] == '/')
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%s", config_data->file);
			else
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%c_file=%s", x++ == 0 ? '?' : '&', config_data->file);
		} else {
			if (config_data->table) {
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%c_table=%s", x++ == 0 ? '?' : '&', config_data->table);
			}
			if (config_data->action) {
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%c_action=%s", x++ == 0 ? '?' : '&', config_data->action);
			}
			if (config_data->keyfield && config_data->lookup) {
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%c_keyfield=%s&_lookup=%s", 
						 x++ == 0 ? '?' : '&', config_data->keyfield, config_data->lookup);
				x++;
			}
			while ((param = va_arg(config_data->aq, const char *)) && (val = va_arg(config_data->aq, const char *))) {
				snprintf(config_data->url + strlen(config_data->url), sizeof(config_data->url) - strlen(config_data->url), "%c%s=%s", x++ == 0 ? '?' : '&', param, val);
			}

		}
	}


	snprintf(config_data->cachefile, sizeof(config_data->cachefile), "%s", global_tmp_prefix);
	offset = strlen(global_tmp_prefix);
	for (x = 0; x < strlen(config_data->url); x++) {
		if (config_data->url[x] == '/' || 
		   config_data->url[x] == ':' || 
		   config_data->url[x] == '$' ||
		   config_data->url[x] == '&' ||
		   config_data->url[x] == '(' ||
		   config_data->url[x] == ')' ||
		   config_data->url[x] == '[' ||
		   config_data->url[x] == ']' ||
		   config_data->url[x] == '*' ||
		   config_data->url[x] == '?' ||
		   config_data->url[x] == '=' ||
		   config_data->url[x] == '.' ||
		   config_data->url[x] == '#' )
			config_data->cachefile[offset+x] = '_';
		else
			config_data->cachefile[offset+x] = config_data->url[x];
	}
	
	if (global_cache_time && !stat(config_data->cachefile, &sbuf)) {
		elapsed = (time(NULL) - sbuf.st_mtime);

		if (elapsed < global_cache_time) {
			if (option_verbose > 2)
				cw_verbose(VERBOSE_PREFIX_3 "CURL config engine using cached file for %d more seconds\n", global_cache_time - elapsed);
			perform  = 0;
		}
	}
	
	if (perform) {
		if (option_verbose > 2)
			cw_verbose(VERBOSE_PREFIX_3 "CURL config engine fetching [%s]\n", config_data->url);

		curl_global_init(CURL_GLOBAL_ALL);
		curl_handle = curl_easy_init();
		if (!strncasecmp(config_data->url, "https", 5)) {
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		}
		curl_easy_setopt(curl_handle, CURLOPT_URL, config_data->url);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, realtime_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)config_data);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "callweaver/1.2");
		curl_easy_perform(curl_handle);
		curl_easy_cleanup(curl_handle);
	}
	if (!cw_strlen_zero(config_data->cachefile) && !strncmp(config_data->action, "realtime", 8) &&
		(cfg = cw_config_load(config_data->cachefile))) {
		vp = config_data->vars;
		for(v = cw_variable_browse(cfg, "realtime") ; v ; v = v->next) {
			vp->next = cw_variable_new(v->name, v->value);
			vp = vp->next;
		}
		cw_config_destroy(cfg);
		cfg = NULL;
	} else if (!cw_strlen_zero(config_data->cachefile) && 
			   (cfg = cw_config_internal_load(config_data->cachefile, config_data->cfg)))
		config_data->cfg = cfg;
	
	if (global_no_cache_neg && !cw_strlen_zero(config_data->cachefile) && !config_data->vars && !config_data->cfg)
		unlink(config_data->cachefile);

}


static struct cw_variable *realtime_curl(const char *database, const char *table, va_list ap)
{

	struct config_data config_data;
	
	memset(&config_data, 0, sizeof(config_data));
	va_copy(config_data.aq, ap);
	config_data.database = database;
	config_data.table = table;
	config_data.action = "realtime_lookup";
	curl_process(&config_data);
	va_end(config_data.aq);
	return config_data.vars;
}

static int realtime_exec(struct cw_channel *chan, int argc, char **argv) 
{
	struct config_data config_data;
	struct cw_variable *v;

	memset(&config_data, 0, sizeof(config_data));
	config_data.database = argv[0];
	config_data.action = "realtime_lookup";
	config_data.skip_prep = 1;
	curl_process(&config_data);

	if (config_data.vars) {
		for (v = config_data.vars; v; v = v->next) {
			if(strncmp(v->name, "private_", 8))
				pbx_builtin_setvar_helper(chan, v->name, v->value);
		}
		cw_variables_destroy(config_data.vars);
	}
	return 0;
}



static struct cw_config *realtime_multi_curl(const char *database, const char *table, va_list ap)
{
	struct config_data config_data;
	
	memset(&config_data, 0, sizeof(config_data));
	va_copy(config_data.aq, ap);
	config_data.cfg = cw_config_new();
	config_data.database = database;
	config_data.table = table;
	config_data.action = "realtime_multi_lookup";
	curl_process(&config_data);
	va_end(config_data.aq);
	return config_data.cfg;
}

static int update_curl(const char *database, const char *table, const char *keyfield, const char *lookup, va_list ap)
{
	struct config_data config_data;
	struct cw_variable *v;
	int res = -1;
	memset(&config_data, 0, sizeof(config_data));
	va_copy(config_data.aq, ap);
	config_data.database = database;
	config_data.keyfield = keyfield;
	config_data.table = table;
	config_data.lookup = lookup;
	config_data.action = "realtime_update";
	curl_process(&config_data);
	va_end(config_data.aq);
	if (config_data.vars) {
		for (v = config_data.vars; v; v = v->next) {
			if (!strcmp(v->name, "result")) {
				res = cw_true(v->value) ? 0 : -1;
			}
		}
		cw_variables_destroy(config_data.vars);
	}
	return res;
}

static struct cw_config *config_curl (const char *database, const char *table, const char *file, struct cw_config *cfg)
{
	struct config_data config_data;
	/* can't configure myself */
	if (!strcmp(file, global_config_file))
		return NULL;
	
    memset(&config_data, 0, sizeof(config_data));
	config_data.action = "config";
    config_data.cfg = cfg;
	config_data.database = database;
	config_data.file = file;
	
	if (!config_data.cfg)
		config_data.cfg = cw_config_new();
    curl_process(&config_data);
	
    return config_data.cfg;
   
}

static void load_config(void) {
	struct cw_config *cfg;
	struct cw_variable *v;

	
	if ( (cfg = cw_config_load(global_config_file)) ) {
		for (v = cw_variable_browse(cfg, "config"); v ; v = v->next)
			if (!strcasecmp(v->name, "cache_time"))
				global_cache_time = atoi(v->value);
			else if (!strcasecmp(v->name, "no_cache_neg"))
				global_no_cache_neg = atoi(v->value);
		cw_config_destroy(cfg);
		cfg = NULL;
	}

}


static struct cw_config_engine curl_engine = {
	.name = "curl",
	.load_func = config_curl,
	.realtime_func =  realtime_curl,
	.realtime_multi_func = realtime_multi_curl,
	.update_func = update_curl
};


int reload(void) {
	load_config();
	return 0;
}

int unload_module (void)
{
	int res = 0;
	cw_config_engine_deregister(&curl_engine);
	if (option_verbose)
		cw_verbose(VERBOSE_PREFIX_1 "res_config_curl unloaded.\n");
	res |= cw_unregister_application(app_1);
	STANDARD_HANGUP_LOCALUSERS;
	return res;
}


int load_module (void)
{
	char cmd[128];
	snprintf(cmd, 128, "/bin/rm -fr %s ; /bin/mkdir -p %s", global_tmp_prefix, global_tmp_prefix);
	system(cmd);
	load_config();
	cw_config_engine_register(&curl_engine);
	app_1 = cw_register_application(name_1, realtime_exec, synopsis_1, syntax_1, desc_1);
	if (option_verbose)
		cw_verbose(VERBOSE_PREFIX_1 "res_config_curl loaded.\n");
	return 0;
}

char *description (void)
{
	return tdesc;
}

int usecount (void)
{
	/* never unload a config module */
	return 1;
}

