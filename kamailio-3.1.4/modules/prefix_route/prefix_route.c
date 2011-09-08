/*
 * Prefix Route Module
 *
 * Copyright (C) 2007 Alfred E. Heggestad
 * Copyright (C) 2008 Telio Telecom AS
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
 */


#include <stdio.h>
#include <stdlib.h>
#include "../../lib/srdb2/db.h"
#include "../../rpc.h"
#include "../../sr_module.h"
#include "../../mem/mem.h"
#include "../../data_lump_rpl.h"
#include "../../parser/parse_uri.h"
#include "../../parser/msg_parser.h"
#include "../../locking.h"
#include "../../action.h"
#include "../../route.h"
#include "tree.h"
#include "pr.h"


MODULE_VERSION


/* Modules parameters */
static char *db_url   = DEFAULT_DB_URL;
static char *db_table = "prefix_route";


static int add_route(struct tree_item *root, const char *prefix,
		     const char *route)
{
	int ix, err;

	/* We cache the route index here so we can avoid route_lookup()
	 * in the prefix_route() routing function.
	 */
	ix = route_lookup(&main_rt, (char *)route);
	if (ix < 0) {
		LOG(L_CRIT, "prefix_route: db_load(): "
		    "route name '%s' is not defined\n", route);
		return -1;
	}

	if (ix >= main_rt.entries) {
		LOG(L_CRIT, "prefix_route: route %d > n_entries (%d)\n",
		    ix, main_rt.entries);
		return -1;
	}

	err = tree_item_add(root, prefix, route, ix);
	if (0 != err) {
		LOG(L_CRIT, "prefix_route: db_load(): "
		    "tree_item_add() failed (%d)\n", err);
		return err;
	}

	return 0;
}


/**
 * Load prefix rules from database and build tree in memory
 */
int pr_db_load(void)
{
	db_ctx_t *ctx;
	db_cmd_t *cmd = NULL;
	db_res_t* res = NULL;
	db_rec_t* rec;
	struct tree_item *root;
	int err = -1;
	int count = 0;
	db_fld_t match[] = {
		{ .name = "prefix",  .type = DB_CSTR },
		{ .name = "route",   .type = DB_CSTR },
		{ .name = "comment", .type = DB_CSTR },
		{ .name = NULL,      .type = DB_NONE }
	};

	ctx = db_ctx("prefix_route");
	if (!ctx) {
		LOG(L_ERR, "prefix_route: db_load(): db_ctx() failed\n");
		return -1;
	}
	if (db_add_db(ctx, db_url) < 0) {
		LOG(L_ERR, "prefix_route: db_load(): could not add db\n");
		goto out;
	}
	if (db_connect(ctx) < 0) {
		LOG(L_ERR, "prefix_route: db_load(): could not connect\n");
		goto out;
	}

	cmd = db_cmd(DB_GET, ctx, db_table, match, NULL, NULL);
	if (!cmd) {
		LOG(L_ERR, "prefix_route: db_load(): db_cmd() failed\n");
		goto out;
	}

	if (db_exec(&res, cmd) < 0) {
		LOG(L_ERR, "prefix_route: db_load(): db_exec() failed\n");
		goto out;
	}

	root = tree_item_alloc();
	if (NULL == root) {
		LOG(L_ERR, "prefix_route: db_load() tree alloc failed\n");
		err = -1;
		goto out;
	}

	/* Assume Success */
	err = 0;

	/* Read from DB and build tree */
	for (rec = db_first(res); rec != NULL && !err; rec = db_next(res)) {
		const char *prefix, *route, *comment;

		++count;

		if (rec->fld[0].flags & DB_NULL) {
			LOG(L_CRIT, "prefix_route: ERROR: bad 'prefix' "
			    "record in table %s, skipping...\n", db_table);
			continue;
		}
		if (rec->fld[1].flags & DB_NULL) {
			LOG(L_CRIT, "prefix_route: ERROR: bad 'route' "
			    "record in table %s, skipping...\n", db_table);
			continue;
		}

		prefix  = rec->fld[0].v.cstr;
		route   = rec->fld[1].v.cstr;
		comment = rec->fld[2].v.cstr;

		LOG(L_INFO, "  %d: prefix=%-10s  route=%-15s  comment=%s\n",
		    count, prefix, route, comment);

		err = add_route(root, prefix, route);
	}

	LOG(L_NOTICE, "prefix_route: Total prefix routes loaded: %d\n", count);

	/* Error */
	if (0 != err) {
		LOG(L_ERR, "prefix_route: db_load(): error, flushing tree\n");
		tree_item_free(root);
		goto out;
	}

	/* Swap trees */
	err = tree_swap(root);
	if (0 != err)
		goto out;

 out:
	/* Free database results */
	if (res)
		db_res_free(res);
	if (cmd)
		db_cmd_free(cmd);

	/* Close database connection */
	if (ctx)
		db_ctx_free(ctx);

	return err;
}


/**
 * Initialize module
 */
static int mod_init(void)
{
	/* Initialise tree */
	if (0 != tree_init()) {
		LOG(L_CRIT, "prefix_route: tree_init() failed\n\n");
		return -1;
	}

	/* Populate database */
	if (0 != pr_db_load()) {
		LOG(L_CRIT, "prefix_route: db_load() failed\n\n");
		return -1;
	}

	return 0;
}


/**
 * Destroy module and free all resources
 */
static void mod_destroy(void)
{
	tree_close();
}


/**
 * Extract username from the Request URI
 * First try to look at the original Request URI and if there
 * is no username use the new Request URI
 */
static int get_username(struct sip_msg* msg, str *user)
{
	if (!msg || !user)
		return -1;

	if (parse_sip_msg_uri(msg) < 0){
		LOG(L_ERR, "get_username(): bad uri\n");
		return -1; /* error, bad uri */
	}

	if (msg->parsed_uri.user.s == 0){
		/* no user in uri */
		LOG(L_ERR, "get_username(): no user in uri\n");
		return -2;
	}

	*user = msg->parsed_uri.user;

	return 0;
}


/**
 * SER Command "prefix_route"
 */
static int prefix_route(struct sip_msg *msg, char *p1, char *p2)
{
	struct run_act_ctx ra_ctx;
	str user;
	int err;
	int route;

	/* Unused */
	(void)p1;
	(void)p2;

	/* Get request URI */
	err = get_username(msg, &user);
	if (0 != err) {
		LOG(L_ERR, "prefix_route: could not get username in"
		    " Request URI (%d)\n", err);
		return err;
	}

	route = tree_route_get(&user);
	if (route <= 0)
		return -1;

	/* If match send to route[x] */
	init_run_actions_ctx(&ra_ctx);

	err = run_actions(&ra_ctx, main_rt.rlist[route], msg);
	if (err < 0) {
		LOG(L_ERR, "prefix_route: run_actions failed (%d)\n", err);
		return -1;
	}

	/* Success */
	return 0;
}


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"prefix_route", prefix_route, 0, 0, REQUEST_ROUTE},
	{0,              0,            0, 0, 0            }
};

/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"db_url",       STR_PARAM, &db_url  },
	{"db_table",     STR_PARAM, &db_table},
	{0,              0,         0        }
};

/*
 * Module description
 */
struct module_exports exports = {
	"prefix_route",  /* Module name             */
	cmds,            /* Exported functions      */
	pr_rpc,          /* RPC methods             */
	params,          /* Exported parameters     */
	mod_init,        /* Initialization function */
	0,               /* Response function       */
	mod_destroy,     /* Destroy function        */
	0,               /* OnCancel function       */
	0                /* Child init function     */
};
