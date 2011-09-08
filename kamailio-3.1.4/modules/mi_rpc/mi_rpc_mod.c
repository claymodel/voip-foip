/*
 * $Id$
 *
 * mi_rpc module
 *
 * Copyright (C) 2009 Daniel-Constantin Mierla (asipto.com).
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../sr_module.h"
#include "../../dprint.h"

#include "../../lib/kmi/mi.h"
#include "../../rpc.h"

MODULE_VERSION

static int child_init(int rank);

static str mi_rpc_indent = { "\t", 1 };

static const char* rpc_mi_exec_doc[2] = {
	"Execute MI command",
	0
};



enum mi_rpc_print_mode {
	MI_PRETTY_PRINT,
	MI_FIFO_PRINT,
	MI_DATAGRAM_PRINT,
	MI_XMLRPC_PRINT
};



static void rpc_mi_pretty_exec(rpc_t* rpc, void* c);
static void rpc_mi_fifo_exec(rpc_t* rpc, void* c);
static void rpc_mi_dg_exec(rpc_t* rpc, void* c);
static void rpc_mi_xmlrpc_exec(rpc_t* rpc, void* c);

rpc_export_t mr_rpc[] = {
	{"mi",			rpc_mi_pretty_exec,	rpc_mi_exec_doc,  RET_ARRAY},
	{"mi_fifo",		rpc_mi_fifo_exec,	rpc_mi_exec_doc,  RET_ARRAY},
	{"mi_dg",		rpc_mi_dg_exec,		rpc_mi_exec_doc,  RET_ARRAY},
	{"mi_xmlrpc",	rpc_mi_xmlrpc_exec,	rpc_mi_exec_doc,  RET_ARRAY},
	{0, 0, 0, 0}
};

struct module_exports exports = {
	"mi_rpc",
	0,           /* Exported functions */
	mr_rpc,      /* RPC methods */
	0,           /* Export parameters */
	0,           /* Module initialization function */
	0,           /* Response function */
	0,           /* Destroy function */
	0,           /* OnCancel function */
	child_init   /* Child initialization function */
};


static int child_init(int rank)
{
	if(rank==PROC_RPC) {
		if(init_mi_child()!=0) {
			LM_CRIT("Failed to init the mi commands\n");
			return -1;
		}
	} else if(rank>0) {
		if(find_module_by_name("xmlrpc")!=0)
		{
			if(init_mi_child()!=0) {
				LM_CRIT("Failed to init the mi commands for xmlrpc usage\n");
				return -1;
			}
		}
	}

	return 0;
}

struct mi_root *mi_rpc_read_params(rpc_t *rpc, void *ctx)
{
	struct mi_root *root;
	struct mi_node *node;
	str name;
	str value;

	root = init_mi_tree(0,0,0);
	if (!root) {
		LM_ERR("the MI tree cannot be initialized!\n");
		goto error;
	}
	node = &root->node;

	while (rpc->scan(ctx, "*.S", &value) == 1)
	{
		name.s   = 0;
		name.len = 0;

		if(value.len>=2 && value.s[0]=='-' && value.s[1]=='-')
		{
			/* name */
			if(value.len>2)
			{
				name.s   = value.s + 2;
				name.len = value.len - 2;
			}

			/* value */
			if(rpc->scan(ctx, "*.S", &value) != 1)
			{
				LM_ERR("value expected\n");
				goto error;
			}
		}

		if(!add_mi_node_child(node, 0, name.s, name.len,
					value.s, value.len))
		{
			LM_ERR("cannot add the child node to the MI tree\n");
			goto error;
		}
	}

	return root;

error:
	if (root)
		free_mi_tree(root);
	return 0;
}



/** prints a mi node using mode.
 * @return 0 on success, -1 on error (line too long)
 */
static int mi_rpc_print_node(rpc_t* rpc, void* ctx, struct mi_node* node,
								enum mi_rpc_print_mode mode, char* prefix)
{
	static char buf[512];
	char* p;
	int n;
	int size;
	struct mi_attr *attr;
	
	p=buf;
	*p=0;
	size=sizeof(buf);
		n=snprintf(p, size, "%s%.*s:: %.*s",
				prefix,
				node->name.len, (node->name.s)?node->name.s:"",
				node->value.len, (node->value.s)?node->value.s:"");
		if (n==-1 || n >= size)
			goto error_buf;
		p+=n;
		size-=n;
		for( attr=node->attributes ; attr!=NULL ; attr=attr->next ) {
			n=snprintf(p, size, " %.*s=%.*s",
					attr->name.len, (attr->name.s)?attr->name.s:"",
					attr->value.len, (attr->value.s)?attr->value.s:"");
			if (n==-1 || n >= size)
				goto error_buf;
			p+=n;
			size-=n;
		}
		if (mode!=MI_PRETTY_PRINT){
			n=snprintf(p, size, "\n");
			if (n==-1 || n>= size ) 
				goto error_buf;
		}
		rpc->add(ctx, "s", buf);
	return 0;
error_buf:
		ERR("line too long (extra %d chars)\n", (n>=size)?n-size+1:0);
		rpc->fault(ctx, 500, "Line too long");
		return -1;
}



static int mi_rpc_rprint_all(rpc_t* rpc, void* ctx, struct mi_node *node,
		enum mi_rpc_print_mode mode, int level)
{
	char indent[32];
	int i;
	char *p;

	p = indent;
	switch(mode){
		case MI_FIFO_PRINT:
		case MI_DATAGRAM_PRINT:
		case MI_PRETTY_PRINT:
			if(level*mi_rpc_indent.len>=32)
			{
				LM_ERR("too many recursive levels for indentation\n");
				return -1;
			}
			for(i=0; i<level; i++)
			{
				memcpy(p, mi_rpc_indent.s, mi_rpc_indent.len);
				p += mi_rpc_indent.len;
			}
			break;
		case MI_XMLRPC_PRINT:
			/* no identation  in this mode */
			break;
	}
	*p = 0;
	for( ; node ; node=node->next )
	{
		if (mi_rpc_print_node(rpc, ctx, node, mode, p)<0)
			return -1;
		if (node->kids) {
			if (mi_rpc_rprint_all(rpc, ctx, node->kids, mode, level+1)<0)
				return -1;
		}
	}
	return 0;
}



/** build an rpc reply from an mi tree.
 * @param mode - how to build the reply: mi_fifo like, mi_datagram_like,
 *               mi_xmlrpc ...
 * @return -1 on error, 0 on success
 */
static int mi_rpc_print_tree(rpc_t* rpc, void* ctx, struct mi_root *tree,
								enum mi_rpc_print_mode mode)
{
	switch(mode){
		case MI_FIFO_PRINT:
		case MI_DATAGRAM_PRINT:
			/* always success, code & reason are the part of the reply */
			rpc->printf(ctx, "%d %.*s\n", tree->code,
						tree->reason.len, tree->reason.s);
			break;
		case MI_PRETTY_PRINT:
		case MI_XMLRPC_PRINT:
			/* don't print code & reason, use fault instead */
			if (tree->code<200 || tree->code> 299) {
				rpc->fault(ctx, tree->code, tree->reason.s);
				return -1;
			}
			break;
	}

	if (tree->node.kids)
	{
		if (mi_rpc_rprint_all(rpc, ctx, tree->node.kids, mode, 0)<0)
			return -1;
	}
	if (mode==MI_FIFO_PRINT){
		/* mi fifo adds an extra "\n" at the end */
		rpc->printf(ctx, "\n");
	}
	
	return 0;
}



/* structure used to pack the rpc dyn. ctx and the print mode */
struct mi_rpc_handler_param{
	rpc_delayed_ctx_t* dctx;
	enum mi_rpc_print_mode mode;
};

/* send reply and close async context */
static void mi_rpc_async_close(struct mi_root* mi_rpl,
									struct mi_handler* mi_h,
									int done)
{
	rpc_delayed_ctx_t* dctx;
	rpc_t* rpc;
	void* c;
	enum mi_rpc_print_mode mode;
	
	dctx=0;
	if (done){
		if (mi_h->param==0){
			BUG("null param\n");
			shm_free(mi_h);
			goto error;
		}
		dctx=((struct mi_rpc_handler_param*)mi_h->param)->dctx;
		if (dctx==0){
			BUG("null dctx\n");
			shm_free(mi_h->param);
			shm_free(mi_h);
			mi_h->param=0;
			goto error;
		}
		mode=((struct mi_rpc_handler_param*)mi_h->param)->mode;
		rpc=&dctx->rpc;
		c=dctx->reply_ctx;
		
		mi_rpc_print_tree(rpc, c, mi_rpl, mode);
		
		rpc->delayed_ctx_close(dctx);
		shm_free(mi_h->param);
		mi_h->param=0;
		shm_free(mi_h);
	} /* else: no provisional support => do nothing */
error:
	if (mi_rpl)
		free_mi_tree(mi_rpl);
	return;
}



static void rpc_mi_exec(rpc_t *rpc, void *ctx, enum mi_rpc_print_mode mode)
{
	str cmd;
	struct mi_cmd *mic;
	struct mi_root *mi_req;
	struct mi_root *mi_rpl;
	struct mi_handler* mi_async_h;
	struct mi_rpc_handler_param* mi_handler_param;

	if (rpc->scan(ctx, "S", &cmd) < 1)
	{
		LM_ERR("command parameter not found\n");
		rpc->fault(ctx, 500, "command parameter missing");
		return;
	}
	
	mi_async_h=0;
	mi_req = 0;
	mi_rpl=0;
	
	mic = lookup_mi_cmd(cmd.s, cmd.len);
	if(mic==0)
	{
		LM_ERR("mi command %.*s is not available\n", cmd.len, cmd.s);
		rpc->fault(ctx, 500, "command not available");
		return;
	}

	if (mic->flags&MI_ASYNC_RPL_FLAG)
	{
		if (rpc->capabilities==0 ||
				!(rpc->capabilities(ctx) & RPC_DELAYED_REPLY))
		{
			rpc->fault(ctx, 500,
							"this rpc transport does not support async mode");
			return;
		}
	}

	if(!(mic->flags&MI_NO_INPUT_FLAG))
	{
		mi_req = mi_rpc_read_params(rpc, ctx);
		if(mi_req==NULL)
		{
			LM_ERR("cannot parse parameters\n");
			rpc->fault(ctx, 500, "cannot parse parameters");
			goto error;
		}
		if (mic->flags&MI_ASYNC_RPL_FLAG)
		{
			/* build mi async handler */
			mi_handler_param=shm_malloc(sizeof(*mi_handler_param));
			if (mi_handler_param==0){
				rpc->fault(ctx, 500, "out of memory");
				return;
			}
			mi_async_h=shm_malloc(sizeof(*mi_async_h));
			if (mi_async_h==0){
				shm_free(mi_handler_param);
				mi_handler_param=0;
				rpc->fault(ctx, 500, "out of memory");
				return;
			}
			memset(mi_async_h, 0, sizeof(*mi_async_h));
			mi_async_h->handler_f=mi_rpc_async_close;
			mi_handler_param->mode=mode;
			mi_handler_param->dctx=rpc->delayed_ctx_new(ctx);
			if (mi_handler_param->dctx==0){
				rpc->fault(ctx, 500, "internal error: async ctx"
										" creation failed");
				goto error;
			}
			/* switch context, since replies are not allowed anymore on the
			   original one */
			rpc=&mi_handler_param->dctx->rpc;
			ctx=mi_handler_param->dctx->reply_ctx;
			mi_async_h->param=mi_handler_param;
		}
		mi_req->async_hdl=mi_async_h;
	}
	mi_rpl=run_mi_cmd(mic, mi_req);

	if(mi_rpl == 0)
	{
		rpc->fault(ctx, 500, "execution failed");
		goto error;
	}

	if (mi_rpl!=MI_ROOT_ASYNC_RPL)
	{
		mi_rpc_print_tree(rpc, ctx, mi_rpl, mode);
		goto end;
	}else if (mi_async_h==0){
		/* async reply, but command not listed as async */
		rpc->fault(ctx, 500, "bad mi command: unexpected async reply");
		goto error;
	}
	mi_async_h=0; /* don't delete it */
end:
error:
	if (mi_req)
		free_mi_tree(mi_req);
	if (mi_rpl && mi_rpl!=MI_ROOT_ASYNC_RPL)
		free_mi_tree(mi_rpl);
	if (mi_async_h){
		if (mi_async_h->param){
			if (((struct mi_rpc_handler_param*)mi_async_h->param)->dctx)
				rpc->delayed_ctx_close(((struct mi_rpc_handler_param*)
											mi_async_h->param)->dctx);
			shm_free(mi_async_h->param);
		}
		shm_free(mi_async_h);
	}
	return;
}



static void rpc_mi_pretty_exec(rpc_t* rpc, void* c)
{
	rpc_mi_exec(rpc, c, MI_PRETTY_PRINT);
}



static void rpc_mi_fifo_exec(rpc_t* rpc, void* c)
{
	rpc_mi_exec(rpc, c, MI_FIFO_PRINT);
}



static void rpc_mi_dg_exec(rpc_t* rpc, void* c)
{
	rpc_mi_exec(rpc, c, MI_DATAGRAM_PRINT);
}



static void rpc_mi_xmlrpc_exec(rpc_t* rpc, void* c)
{
	rpc_mi_exec(rpc, c, MI_XMLRPC_PRINT);
}
