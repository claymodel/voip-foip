/*
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of SIP-router, a free SIP server.
 *
 * SIP-router is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * SIP-router is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History
 * -------
 * 2003-03-06  200/INV to-tag list deallocation added;
 *             setting "kill_reason" moved in here -- it is moved
 *             from transaction state to a static var(jiri)
 * 2003-03-16  removed _TOTAG (jiri)
 * 2003-03-30  set_kr for requests only (jiri)
 * 2003-04-04  bug_fix: REQ_IN callback not called for local 
 *             UAC transactions (jiri)
 * 2003-09-12  timer_link->tg will be set only if EXTRA_DEBUG (andrei)
 * 2003-12-04  global callbacks replaceed with callbacks per transaction;
 *             completion callback merged into them as LOCAL_COMPETED (bogdan)
 * 2004-02-11  FIFO/CANCEL + alignments (hash=f(callid,cseq)) (uli+jiri)
 * 2004-02-13  t->is_invite and t->local replaced with flags;
 *             timer_link.payload removed (bogdan)
 * 2004-08-23  avp support added - move and remove avp list to/from
 *             transactions (bogdan)
 * 2006-08-11  dns failover support (andrei)
 * 2007-05-16  callbacks called on destroy (andrei)
 * 2007-06-06  don't allocate extra space for md5 if not used: syn_branch==1 
 *              (andrei)
 * 2007-06-06  switched tm bucket list to a simpler and faster clist (andrei)
 */

/*!
 * \file 
 * \brief TM :: 
 * \ingroup tm
 */

#include <stdlib.h>


#include "../../mem/shm_mem.h"
#include "../../hash_func.h"
#include "../../dprint.h"
#include "../../md5utils.h"
#include "../../ut.h"
#include "../../globals.h"
#include "../../error.h"
#include "../../char_msg_val.h"
#include "defs.h"
#include "t_reply.h"
#include "t_cancel.h"
#include "t_stats.h"
#include "h_table.h"
#include "../../fix_lumps.h" /* free_via_clen_lump */
#include "timer.h"
#include "uac.h" /* free_local_ack */


static enum kill_reason kr;

/* pointer to the big table where all the transaction data
   lives */
struct s_table*  _tm_table;

struct s_table* tm_get_table(void) {
	return _tm_table;
}

void reset_kr() {
	kr=0;
}

void set_kr( enum kill_reason _kr )
{
	kr|=_kr;
}


enum kill_reason get_kr() {
	return kr;
}


void lock_hash(int i) 
{

	int mypid;

	mypid = my_pid();
	if (likely(atomic_get(&_tm_table->entries[i].locker_pid) != mypid)) {
		lock(&_tm_table->entries[i].mutex);
		atomic_set(&_tm_table->entries[i].locker_pid, mypid);
	} else {
		/* locked within the same process that called us*/
		_tm_table->entries[i].rec_lock_level++;
	}
}


void unlock_hash(int i) 
{
	if (likely(_tm_table->entries[i].rec_lock_level == 0)) {
		atomic_set(&_tm_table->entries[i].locker_pid, 0);
		unlock(&_tm_table->entries[i].mutex);
	} else  {
		/* recursive locked => decrease rec. lock count */
		_tm_table->entries[i].rec_lock_level--;
	}
}



#ifdef TM_HASH_STATS
unsigned int transaction_count( void )
{
	unsigned int i;
	unsigned int count;

	count=0;	
	for (i=0; i<TABLE_ENTRIES; i++) 
		count+=_tm_table->entries[i].cur_entries;
	return count;
}
#endif



void free_cell( struct cell* dead_cell )
{
	char *b;
	int i;
	struct sip_msg *rpl;
	struct totag_elem *tt, *foo;
	struct tm_callback *cbs, *cbs_tmp;

	release_cell_lock( dead_cell );
	if (unlikely(has_tran_tmcbs(dead_cell, TMCB_DESTROY)))
		run_trans_callbacks(TMCB_DESTROY, dead_cell, 0, 0, 0);

	shm_lock();
	/* UA Server */
	if ( dead_cell->uas.request )
		sip_msg_free_unsafe( dead_cell->uas.request );
	if ( dead_cell->uas.response.buffer )
		shm_free_unsafe( dead_cell->uas.response.buffer );
#ifdef CANCEL_REASON_SUPPORT
	if (unlikely(dead_cell->uas.cancel_reas))
		shm_free_unsafe(dead_cell->uas.cancel_reas);
#endif /* CANCEL_REASON_SUPPORT */

	/* callbacks */
	for( cbs=(struct tm_callback*)dead_cell->tmcb_hl.first ; cbs ; ) {
		cbs_tmp = cbs;
		cbs = cbs->next;
		if (cbs_tmp->release) {
			/* It is safer to release the shm memory lock
			 * otherwise the release function must to be aware of
			 * the lock state (Miklos)
			 */
			shm_unlock();
			cbs_tmp->release(cbs_tmp->param);
			shm_lock();
		}
		shm_free_unsafe( cbs_tmp );
	}

	/* UA Clients */
	for ( i =0 ; i<dead_cell->nr_of_outgoings;  i++ )
	{
		/* retransmission buffer */
		if ( (b=dead_cell->uac[i].request.buffer) )
			shm_free_unsafe( b );
		b=dead_cell->uac[i].local_cancel.buffer;
		if (b!=0 && b!=BUSY_BUFFER)
			shm_free_unsafe( b );
		rpl=dead_cell->uac[i].reply;
		if (rpl && rpl!=FAKED_REPLY && rpl->msg_flags&FL_SHM_CLONE) {
			sip_msg_free_unsafe( rpl );
		}
#ifdef USE_DNS_FAILOVER
		if (dead_cell->uac[i].dns_h.a){
			DBG("branch %d -> dns_h.srv (%.*s) ref=%d,"
							" dns_h.a (%.*s) ref=%d\n", i,
					dead_cell->uac[i].dns_h.srv?
								dead_cell->uac[i].dns_h.srv->name_len:0,
					dead_cell->uac[i].dns_h.srv?
								dead_cell->uac[i].dns_h.srv->name:"",
					dead_cell->uac[i].dns_h.srv?
								dead_cell->uac[i].dns_h.srv->refcnt.val:0,
					dead_cell->uac[i].dns_h.a->name_len,
					dead_cell->uac[i].dns_h.a->name,
					dead_cell->uac[i].dns_h.a->refcnt.val);
		}
		dns_srv_handle_put_shm_unsafe(&dead_cell->uac[i].dns_h);
#endif
		if (unlikely(dead_cell->uac[i].path.s)) {
			shm_free_unsafe(dead_cell->uac[i].path.s);
		}
	}

#ifdef WITH_AS_SUPPORT
	if (dead_cell->uac[0].local_ack)
		free_local_ack_unsafe(dead_cell->uac[0].local_ack);
#endif

	/* collected to tags */
	tt=dead_cell->fwded_totags;
	while(tt) {
		foo=tt->next;
		shm_free_unsafe(tt->tag.s);
		shm_free_unsafe(tt);
		tt=foo;
	}

	/* free the avp list */
	if (dead_cell->user_avps_from)
		destroy_avp_list_unsafe( &dead_cell->user_avps_from );
	if (dead_cell->user_avps_to)
		destroy_avp_list_unsafe( &dead_cell->user_avps_to );
	if (dead_cell->uri_avps_from)
		destroy_avp_list_unsafe( &dead_cell->uri_avps_from );
	if (dead_cell->uri_avps_to)
		destroy_avp_list_unsafe( &dead_cell->uri_avps_to );
#ifdef WITH_XAVP
	if (dead_cell->xavps_list)
		xavp_destroy_list_unsafe( &dead_cell->xavps_list );
#endif

	/* the cell's body */
	shm_free_unsafe( dead_cell );

	shm_unlock();
	t_stats_freed();
}



static inline void init_synonym_id( struct cell *t )
{
	struct sip_msg *p_msg;
	int size;
	char *c;
	unsigned int myrand;

	if (!syn_branch) {
		p_msg=t->uas.request;
		if (p_msg) {
			/* char value of a proxied transaction is
			   calculated out of header-fields forming
			   transaction key
			*/
			char_msg_val( p_msg, t->md5 );
		} else {
			/* char value for a UAC transaction is created
			   randomly -- UAC is an originating stateful element 
			   which cannot be refreshed, so the value can be
			   anything
			*/
			/* HACK : not long enough */
			myrand=rand();
			c=t->md5;
			size=MD5_LEN;
			memset(c, '0', size );
			int2reverse_hex( &c, &size, myrand );
		}
	}
}

static void inline init_branches(struct cell *t)
{
	unsigned int i;
	struct ua_client *uac;

	for(i=0;i<MAX_BRANCHES;i++)
	{
		uac=&t->uac[i];
		uac->request.my_T = t;
		uac->request.branch = i;
		init_rb_timers(&uac->request);
		uac->local_cancel=uac->request;
#ifdef USE_DNS_FAILOVER
		dns_srv_handle_init(&uac->dns_h);
#endif
	}
}


struct cell*  build_cell( struct sip_msg* p_msg )
{
	struct cell* new_cell;
	int          sip_msg_len;
	avp_list_t* old;
	struct tm_callback *cbs, *cbs_tmp;
#ifdef WITH_XAVP
	sr_xavp_t** xold;
#endif

	/* allocs a new cell */
	/* if syn_branch==0 add space for md5 (MD5_LEN -sizeof(struct cell.md5)) */
	new_cell = (struct cell*)shm_malloc( sizeof( struct cell )+
			((MD5_LEN-sizeof(((struct cell*)0)->md5))&((syn_branch!=0)-1)) );
	if  ( !new_cell ) {
		ser_error=E_OUT_OF_MEM;
		return NULL;
	}

	/* filling with 0 */
	memset( new_cell, 0, sizeof( struct cell ) );

	/* UAS */
	new_cell->uas.response.my_T=new_cell;
	init_rb_timers(&new_cell->uas.response);
	/* timers */
	init_cell_timers(new_cell);

	old = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_URI, 
			&new_cell->uri_avps_from );
	new_cell->uri_avps_from = *old;
	*old = 0;

	old = set_avp_list(AVP_TRACK_TO | AVP_CLASS_URI, 
			&new_cell->uri_avps_to );
	new_cell->uri_avps_to = *old;
	*old = 0;

	old = set_avp_list(AVP_TRACK_FROM | AVP_CLASS_USER, 
			&new_cell->user_avps_from );
	new_cell->user_avps_from = *old;
	*old = 0;

	old = set_avp_list(AVP_TRACK_TO | AVP_CLASS_USER, 
			&new_cell->user_avps_to );
	new_cell->user_avps_to = *old;
	*old = 0;

#ifdef WITH_XAVP
	xold = xavp_set_list(&new_cell->xavps_list );
	new_cell->xavps_list = *xold;
	*xold = 0;
#endif

	     /* We can just store pointer to domain avps in the transaction context,
	      * because they are read-only
	      */
	new_cell->domain_avps_from = get_avp_list(AVP_TRACK_FROM | 
								AVP_CLASS_DOMAIN);
	new_cell->domain_avps_to = get_avp_list(AVP_TRACK_TO | AVP_CLASS_DOMAIN);

	/* enter callback, which may potentially want to parse some stuff,
	 * before the request is shmem-ized */
	if (p_msg) {
		set_early_tmcb_list(p_msg, new_cell);
		if(has_reqin_tmcbs())
			run_reqin_callbacks( new_cell, p_msg, p_msg->REQ_METHOD);
	}

	if (p_msg) {
#ifndef POSTPONE_MSG_CLONING
		/* it makes no sense to clean the lumps when they are not cloned (Miklos) */

		/* clean possible previous added vias/clen header or else they would 
		 * get propagated in the failure routes */
		free_via_clen_lump(&p_msg->add_rm);
#endif
		new_cell->uas.request = sip_msg_cloner(p_msg,&sip_msg_len);
		if (!new_cell->uas.request)
			goto error;
		new_cell->uas.end_request=((char*)new_cell->uas.request)+sip_msg_len;
	}

	/* UAC */
	init_branches(new_cell);

	new_cell->relayed_reply_branch   = -1;
	/* new_cell->T_canceled = T_UNDEFINED; */

	init_synonym_id(new_cell);
	init_cell_lock(  new_cell );
	t_stats_created();
	return new_cell;

error:
	/* Other modules may have already registered some
	 * transaction callbacks and may also allocated
	 * additional memory for their parameters,
	 * hence TMCB_DESTROY needs to be called. (Miklos)
	 */
	if (unlikely(has_tran_tmcbs(new_cell, TMCB_DESTROY)))
		run_trans_callbacks(TMCB_DESTROY, new_cell, 0, 0, 0);

	/* free the callback list */
	for( cbs=(struct tm_callback*)new_cell->tmcb_hl.first ; cbs ; ) {
		cbs_tmp = cbs;
		cbs = cbs->next;
		if (cbs_tmp->release) {
			cbs_tmp->release(cbs_tmp->param);
		}
		shm_free( cbs_tmp );
	}
	
	destroy_avp_list(&new_cell->user_avps_from);
	destroy_avp_list(&new_cell->user_avps_to);
	destroy_avp_list(&new_cell->uri_avps_from);
	destroy_avp_list(&new_cell->uri_avps_to);
#ifdef WITH_XAVP
	xavp_destroy_list(&new_cell->xavps_list);
#endif
	shm_free(new_cell);
	/* unlink transaction AVP list and link back the global AVP list (bogdan)*/
	reset_avps();
#ifdef WITH_XAVP
	xavp_reset_list();
#endif
	return NULL;
}



/* Release all the data contained by the hash table. All the aux. structures
 *  as sems, lists, etc, are also released */
void free_hash_table(  )
{
	struct cell* p_cell;
	struct cell* tmp_cell;
	int    i;

	if (_tm_table)
	{
		/* remove the data contained by each entry */
		for( i = 0 ; i<TABLE_ENTRIES; i++)
		{
			release_entry_lock( (_tm_table->entries)+i );
			/* delete all synonyms at hash-collision-slot i */
			clist_foreach_safe(&_tm_table->entries[i], p_cell, tmp_cell,
									next_c){
				free_cell(p_cell);
			}
		}
		shm_free(_tm_table);
	}
}




/*
 */
struct s_table* init_hash_table()
{
	int              i;

	/*allocs the table*/
	_tm_table= (struct s_table*)shm_malloc( sizeof( struct s_table ) );
	if ( !_tm_table) {
		LOG(L_ERR, "ERROR: init_hash_table: no shmem for TM table\n");
		goto error0;
	}

	memset( _tm_table, 0, sizeof (struct s_table ) );

	/* try first allocating all the structures needed for syncing */
	if (lock_initialize()==-1)
		goto error1;

	/* inits the entriess */
	for(  i=0 ; i<TABLE_ENTRIES; i++ )
	{
		init_entry_lock( _tm_table, (_tm_table->entries)+i );
		_tm_table->entries[i].next_label = rand();
		/* init cell list */
		clist_init(&_tm_table->entries[i], next_c, prev_c);
	}

	return  _tm_table;

error1:
	free_hash_table( );
error0:
	return 0;
}



