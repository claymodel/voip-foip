/*
 * $Id$
 *
 * Copyright (C) 2006 Voice System SRL
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
 * --------
 * 2006-04-14  initial version (bogdan)
 * 2007-03-06  syncronized state machine added for dialog state. New tranzition
 *             design based on events; removed num_1xx and num_2xx (bogdan)
 * 2007-04-30  added dialog matching without DID (dialog ID), but based only
 *             on RFC3261 elements - based on an original patch submitted 
 *             by Michel Bensoussan <michel@extricom.com> (bogdan)
 * 2007-07-06  additional information stored in order to save it in the db:
 *             cseq, route_set, contact and sock_info for both caller and 
 *             callee (ancuta)
 * 2007-07-10  Optimized dlg_match_mode 2 (DID_NONE), it now employs a proper
 *             hash table lookup and isn't dependant on the is_direction 
 *             function (which requires an RR param like dlg_match_mode 0 
 *             anyways.. ;) ; based on a patch from 
 *             Tavis Paquette <tavis@galaxytelecom.net> 
 *             and Peter Baer <pbaer@galaxytelecom.net>  (bogdan)
 * 2008-04-17  added new type of callback to be triggered right before the
 *              dialog is destroyed (deleted from memory) (bogdan)
 * 2008-04-17  added new dialog flag to avoid state tranzitions from DELETED to
 *             CONFIRMED_NA due delayed "200 OK" (bogdan)
 */


/*!
 * \file
 * \brief Functions related to dialog creation and searching
 * \ingroup dialog
 * Module: \ref dialog
 */

#include <stdlib.h>
#include <string.h>

#include "../../dprint.h"
#include "../../ut.h"
#include "../../lib/kcore/hash_func.h"
#include "../../lib/kmi/mi.h"
#include "dlg_timer.h"
#include "dlg_hash.h"
#include "dlg_profile.h"
#include "dlg_req_within.h"

#define MAX_LDG_LOCKS  2048
#define MIN_LDG_LOCKS  2


/*! global dialog table */
struct dlg_table *d_table = 0;


/*!
 * \brief Initialize the global dialog table
 * \param size size of the table
 * \return 0 on success, -1 on failure
 */
int init_dlg_table(unsigned int size)
{
	unsigned int n;
	unsigned int i;

	d_table = (struct dlg_table*)shm_malloc
		( sizeof(struct dlg_table) + size*sizeof(struct dlg_entry));
	if (d_table==0) {
		LM_ERR("no more shm mem (1)\n");
		goto error0;
	}

	memset( d_table, 0, sizeof(struct dlg_table) );
	d_table->size = size;
	d_table->entries = (struct dlg_entry*)(d_table+1);

	n = (size<MAX_LDG_LOCKS)?size:MAX_LDG_LOCKS;
	for(  ; n>=MIN_LDG_LOCKS ; n-- ) {
		d_table->locks = lock_set_alloc(n);
		if (d_table->locks==0)
			continue;
		if (lock_set_init(d_table->locks)==0) {
			lock_set_dealloc(d_table->locks);
			d_table->locks = 0;
			continue;
		}
		d_table->locks_no = n;
		break;
	}

	if (d_table->locks==0) {
		LM_ERR("unable to allocted at least %d locks for the hash table\n",
			MIN_LDG_LOCKS);
		goto error1;
	}

	for( i=0 ; i<size; i++ ) {
		memset( &(d_table->entries[i]), 0, sizeof(struct dlg_entry) );
		d_table->entries[i].next_id = rand();
		d_table->entries[i].lock_idx = i % d_table->locks_no;
	}

	return 0;
error1:
	shm_free( d_table );
error0:
	return -1;
}


/*!
 * \brief Destroy a dialog, run callbacks and free memory
 * \param dlg destroyed dialog
 */
inline void destroy_dlg(struct dlg_cell *dlg)
{
	int ret = 0;

	LM_DBG("destroying dialog %p\n",dlg);

	ret = remove_dialog_timer(&dlg->tl);
	if (ret < 0) {
		LM_CRIT("unable to unlink the timer on dlg %p [%u:%u] "
			"with clid '%.*s' and tags '%.*s' '%.*s'\n",
			dlg, dlg->h_entry, dlg->h_id,
			dlg->callid.len, dlg->callid.s,
			dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
			dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
	} else if (ret > 0) {
		LM_DBG("removed timer for dlg %p [%u:%u] "
			"with clid '%.*s' and tags '%.*s' '%.*s'\n",
			dlg, dlg->h_entry, dlg->h_id,
			dlg->callid.len, dlg->callid.s,
			dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
			dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
	}

	run_dlg_callbacks( DLGCB_DESTROY , dlg, 0, DLG_DIR_NONE, 0);

	if(dlg==get_current_dlg_pointer())
		reset_current_dlg_pointer();

	if (dlg->cbs.first)
		destroy_dlg_callbacks_list(dlg->cbs.first);

	if (dlg->profile_links)
		destroy_linkers(dlg->profile_links);

	if (dlg->tag[DLG_CALLER_LEG].s)
		shm_free(dlg->tag[DLG_CALLER_LEG].s);

	if (dlg->tag[DLG_CALLEE_LEG].s)
		shm_free(dlg->tag[DLG_CALLEE_LEG].s);

	if (dlg->cseq[DLG_CALLER_LEG].s)
		shm_free(dlg->cseq[DLG_CALLER_LEG].s);

	if (dlg->cseq[DLG_CALLEE_LEG].s)
		shm_free(dlg->cseq[DLG_CALLEE_LEG].s);

	if (dlg->toroute_name.s)
		shm_free(dlg->toroute_name.s);

	shm_free(dlg);
	dlg = 0;
}


/*!
 * \brief Destroy the global dialog table
 */
void destroy_dlg_table(void)
{
	struct dlg_cell *dlg, *l_dlg;
	unsigned int i;

	if (d_table==0)
		return;

	if (d_table->locks) {
		lock_set_destroy(d_table->locks);
		lock_set_dealloc(d_table->locks);
	}

	for( i=0 ; i<d_table->size; i++ ) {
		dlg = d_table->entries[i].first;
		while (dlg) {
			l_dlg = dlg;
			dlg = dlg->next;
			destroy_dlg(l_dlg);
		}

	}

	shm_free(d_table);
	d_table = 0;

	return;
}


/*!
 * \brief Create a new dialog structure for a SIP dialog
 * \param callid dialog callid
 * \param from_uri dialog from uri
 * \param to_uri dialog to uri
 * \param from_tag dialog from tag
 * \param req_uri dialog r-uri
 * \return created dialog structure on success, NULL otherwise
 */
struct dlg_cell* build_new_dlg( str *callid, str *from_uri, str *to_uri,
		str *from_tag, str *req_uri)
{
	struct dlg_cell *dlg;
	int len;
	char *p;

	len = sizeof(struct dlg_cell) + callid->len + from_uri->len +
		to_uri->len + req_uri->len;
	dlg = (struct dlg_cell*)shm_malloc( len );
	if (dlg==0) {
		LM_ERR("no more shm mem (%d)\n",len);
		return 0;
	}

	memset( dlg, 0, len);
	dlg->state = DLG_STATE_UNCONFIRMED;

	dlg->h_entry = core_hash( callid, 0, d_table->size);
	LM_DBG("new dialog on hash %u\n",dlg->h_entry);

	p = (char*)(dlg+1);

	dlg->callid.s = p;
	dlg->callid.len = callid->len;
	memcpy( p, callid->s, callid->len);
	p += callid->len;

	dlg->from_uri.s = p;
	dlg->from_uri.len = from_uri->len;
	memcpy( p, from_uri->s, from_uri->len);
	p += from_uri->len;

	dlg->to_uri.s = p;
	dlg->to_uri.len = to_uri->len;
	memcpy( p, to_uri->s, to_uri->len);
	p += to_uri->len; 

	dlg->req_uri.s = p;
	dlg->req_uri.len = req_uri->len;
	memcpy( p, req_uri->s, req_uri->len);
	p += req_uri->len;

	if ( p!=(((char*)dlg)+len) ) {
		LM_CRIT("buffer overflow\n");
		shm_free(dlg);
		return 0;
	}

	return dlg;
}


/*!
 * \brief Set the leg information for an existing dialog
 * \param dlg dialog
 * \param tag from tag or to tag
 * \param rr record-routing information
 * \param contact caller or callee contact
 * \param cseq CSEQ of caller or callee
 * \param leg must be either DLG_CALLER_LEG, or DLG_CALLEE_LEG
 * \return 0 on success, -1 on failure
 */
int dlg_set_leg_info(struct dlg_cell *dlg, str* tag, str *rr, str *contact,
					str *cseq, unsigned int leg)
{
	char *p;

	dlg->tag[leg].s = (char*)shm_malloc( tag->len + rr->len + contact->len );
	dlg->cseq[leg].s = (char*)shm_malloc( cseq->len );
	if ( dlg->tag[leg].s==NULL || dlg->cseq[leg].s==NULL) {
		LM_ERR("no more shm mem\n");
		if (dlg->tag[leg].s)
		{
			shm_free(dlg->tag[leg].s);
			dlg->tag[leg].s = NULL;
		}
		if (dlg->cseq[leg].s)
		{
			shm_free(dlg->cseq[leg].s);
			dlg->cseq[leg].s = NULL;
		}
		return -1;
	}
	p = dlg->tag[leg].s;

	/* tag */
	dlg->tag[leg].len = tag->len;
	memcpy( p, tag->s, tag->len);
	p += tag->len;
	/* contact */
	dlg->contact[leg].s = p;
	dlg->contact[leg].len = contact->len;
	memcpy( p, contact->s, contact->len);
	p += contact->len;
	/* rr */
	if (rr->len) {
		dlg->route_set[leg].s = p;
		dlg->route_set[leg].len = rr->len;
		memcpy( p, rr->s, rr->len);
	}

	/* cseq */
	dlg->cseq[leg].len = cseq->len;
	memcpy( dlg->cseq[leg].s, cseq->s, cseq->len);

	return 0;
}


/*!
 * \brief Update or set the CSEQ for an existing dialog
 * \param dlg dialog
 * \param leg must be either DLG_CALLER_LEG, or DLG_CALLEE_LEG
 * \param cseq CSEQ of caller or callee
 * \return 0 on success, -1 on failure
 */
int dlg_update_cseq(struct dlg_cell * dlg, unsigned int leg, str *cseq)
{
	if ( dlg->cseq[leg].s ) {
		if (dlg->cseq[leg].len < cseq->len) {
			shm_free(dlg->cseq[leg].s);
			dlg->cseq[leg].s = (char*)shm_malloc(cseq->len);
			if (dlg->cseq[leg].s==NULL)
				goto error;
		}
	} else {
		dlg->cseq[leg].s = (char*)shm_malloc(cseq->len);
		if (dlg->cseq[leg].s==NULL)
			goto error;
	}

	memcpy( dlg->cseq[leg].s, cseq->s, cseq->len );
	dlg->cseq[leg].len = cseq->len;

	LM_DBG("cseq is %.*s\n", dlg->cseq[leg].len, dlg->cseq[leg].s);
	return 0;
error:
	LM_ERR("not more shm mem\n");
	return -1;
}


/*!
 * \brief Lookup a dialog in the global list
 * \param h_entry number of the hash table entry
 * \param h_id id of the hash table entry
 * \return dialog on success, NULL on failure
 */
struct dlg_cell* lookup_dlg( unsigned int h_entry, unsigned int h_id, unsigned int *del)
{
	struct dlg_cell *dlg;
	struct dlg_entry *d_entry;

	if (del != NULL)
		*del = 0;

	if (h_entry>=d_table->size)
		goto not_found;

	d_entry = &(d_table->entries[h_entry]);

	dlg_lock( d_table, d_entry);

	for( dlg=d_entry->first ; dlg ; dlg=dlg->next ) {
		if (dlg->h_id == h_id) {
			if (dlg->state==DLG_STATE_DELETED) {
				if (del != NULL)
					*del = 1;
				dlg_unlock( d_table, d_entry);
				goto not_found;
			}
			dlg->ref++;
			LM_DBG("ref dlg %p with 1 -> %d\n", dlg, dlg->ref);
			dlg_unlock( d_table, d_entry);
			LM_DBG("dialog id=%u found on entry %u\n", h_id, h_entry);
			return dlg;
		}
	}

	dlg_unlock( d_table, d_entry);
not_found:
	LM_DBG("no dialog id=%u found on entry %u\n", h_id, h_entry);
	return 0;
}


/*!
 * \brief Helper function to get a dialog corresponding to a SIP message
 * \see get_dlg
 * \param callid callid
 * \param ftag from tag
 * \param ttag to tag
 * \param dir direction
 * \return dialog structure on success, NULL on failure
 */
static inline struct dlg_cell* internal_get_dlg(unsigned int h_entry,
						str *callid, str *ftag, str *ttag, unsigned int *dir,
						unsigned int *del)
{
	struct dlg_cell *dlg;
	struct dlg_entry *d_entry;

	if (del != NULL)
		*del = 0;

	d_entry = &(d_table->entries[h_entry]);

	dlg_lock( d_table, d_entry);

	for( dlg = d_entry->first ; dlg ; dlg = dlg->next ) {
		/* Check callid / fromtag / totag */
		if (match_dialog( dlg, callid, ftag, ttag, dir)==1) {
			if (dlg->state==DLG_STATE_DELETED) {
				if (del != NULL)
					*del = 1;
				dlg_unlock( d_table, d_entry);
				goto not_found;
			}
			dlg->ref++;
			LM_DBG("ref dlg %p with 1 -> %d\n", dlg, dlg->ref);
			dlg_unlock( d_table, d_entry);
			LM_DBG("dialog callid='%.*s' found\n on entry %u, dir=%d\n",
				callid->len, callid->s,h_entry,*dir);
			return dlg;
		}
	}

	dlg_unlock( d_table, d_entry);

not_found:
	LM_DBG("no dialog callid='%.*s' found\n", callid->len, callid->s);
	return 0;
}



/*!
 * \brief Get dialog that correspond to CallId, From Tag and To Tag
 *
 * Get dialog that correspond to CallId, From Tag and To Tag.
 * See RFC 3261, paragraph 4. Overview of Operation:                 
 * "The combination of the To tag, From tag, and Call-ID completely  
 * defines a peer-to-peer SIP relationship between [two UAs] and is 
 * referred to as a dialog."
 * \param callid callid
 * \param ftag from tag
 * \param ttag to tag
 * \param dir direction
 * \return dialog structure on success, NULL on failure
 */
struct dlg_cell* get_dlg( str *callid, str *ftag, str *ttag, unsigned int *dir,
		unsigned int *del)
{
	struct dlg_cell *dlg;

	if ((dlg = internal_get_dlg(core_hash(callid, 0,
			d_table->size), callid, ftag, ttag, dir, del)) == 0 &&
			(dlg = internal_get_dlg(core_hash(callid, ttag->len
			?ttag:0, d_table->size), callid, ftag, ttag, dir, del)) == 0) {
		LM_DBG("no dialog callid='%.*s' found\n", callid->len, callid->s);
		return 0;
	}
	return dlg;
}


/*!
 * \brief Link a dialog structure
 * \param dlg dialog
 * \param n extra increments for the reference counter
 */
void link_dlg(struct dlg_cell *dlg, int n)
{
	struct dlg_entry *d_entry;

	d_entry = &(d_table->entries[dlg->h_entry]);

	dlg_lock( d_table, d_entry);

	dlg->h_id = d_entry->next_id++;
	if (d_entry->first==0) {
		d_entry->first = d_entry->last = dlg;
	} else {
		d_entry->last->next = dlg;
		dlg->prev = d_entry->last;
		d_entry->last = dlg;
	}

	dlg->ref += 1 + n;

	LM_DBG("ref dlg %p with %d -> %d\n", dlg, n+1, dlg->ref);

	dlg_unlock( d_table, d_entry);
	return;
}


/*!
 * \brief Reference a dialog without locking
 * \param _dlg dialog
 * \param _cnt increment for the reference counter
 */
#define ref_dlg_unsafe(_dlg,_cnt)     \
	do { \
		(_dlg)->ref += (_cnt); \
		LM_DBG("ref dlg %p with %d -> %d\n", \
			(_dlg),(_cnt),(_dlg)->ref); \
	}while(0)


/*!
 * \brief Unreference a dialog without locking
 * \param _dlg dialog
 * \param _cnt decrement for the reference counter
 */
#define unref_dlg_unsafe(_dlg,_cnt,_d_entry)   \
	do { \
		LM_DBG("unref dlg %p with %d, crt ref count: %d\n",\
			(_dlg),(_cnt),(_dlg)->ref);\
		if ((_dlg)->ref<=0) {\
			LM_CRIT("bogus op: ref %d with cnt %d for dlg %p [%u:%u] "\
				"with clid '%.*s' and tags '%.*s' '%.*s'\n",\
				(_dlg)->ref, _cnt, _dlg,\
				(_dlg)->h_entry, (_dlg)->h_id,\
				(_dlg)->callid.len, (_dlg)->callid.s,\
				(_dlg)->tag[DLG_CALLER_LEG].len,\
				(_dlg)->tag[DLG_CALLER_LEG].s,\
				(_dlg)->tag[DLG_CALLEE_LEG].len,\
				(_dlg)->tag[DLG_CALLEE_LEG].s); \
		} else { \
			(_dlg)->ref -= (_cnt); \
			if ((_dlg)->ref<=0) { \
				unlink_unsafe_dlg( _d_entry, _dlg);\
				LM_DBG("ref <=0 for dialog %p\n",_dlg);\
				destroy_dlg(_dlg);\
			}\
		}\
	}while(0)


/*!
 * \brief Refefence a dialog with locking
 * \see ref_dlg_unsafe
 * \param dlg dialog
 * \param cnt increment for the reference counter
 */
void ref_dlg(struct dlg_cell *dlg, unsigned int cnt)
{
	struct dlg_entry *d_entry;

	d_entry = &(d_table->entries[dlg->h_entry]);

	dlg_lock( d_table, d_entry);
	ref_dlg_unsafe( dlg, cnt);
	dlg_unlock( d_table, d_entry);
}


/*!
 * \brief Unreference a dialog with locking
 * \see unref_dlg_unsafe
 * \param dlg dialog
 * \param cnt decrement for the reference counter
 */
void unref_dlg(struct dlg_cell *dlg, unsigned int cnt)
{
	struct dlg_entry *d_entry;

	d_entry = &(d_table->entries[dlg->h_entry]);

	dlg_lock( d_table, d_entry);
	unref_dlg_unsafe( dlg, cnt, d_entry);
	dlg_unlock( d_table, d_entry);
}


/*!
 * Small logging helper functions for next_state_dlg.
 * \param event logged event
 * \param dlg dialog data
 * \see next_state_dlg
 */
static inline void log_next_state_dlg(const int event, const struct dlg_cell *dlg) {
	LM_CRIT("bogus event %d in state %d for dlg %p [%u:%u] with clid '%.*s' and tags "
		"'%.*s' '%.*s'\n", event, dlg->state, dlg, dlg->h_entry, dlg->h_id,
		dlg->callid.len, dlg->callid.s,
		dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
		dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
}


/*!
 * \brief Update a dialog state according a event and the old state
 *
 * This functions implement the main state machine that update a dialog
 * state according a processed event and the current state. If necessary
 * it will delete the processed dialog. The old and new state are also
 * saved for reference.
 * \param dlg updated dialog
 * \param event current event
 * \param old_state old dialog state
 * \param new_state new dialog state
 * \param unref set to 1 when the dialog was deleted, 0 otherwise
 */
void next_state_dlg(struct dlg_cell *dlg, int event,
		int *old_state, int *new_state, int *unref)
{
	struct dlg_entry *d_entry;

	d_entry = &(d_table->entries[dlg->h_entry]);

	*unref = 0;

	dlg_lock( d_table, d_entry);

	*old_state = dlg->state;

	switch (event) {
		case DLG_EVENT_TDEL:
			switch (dlg->state) {
				case DLG_STATE_UNCONFIRMED:
				case DLG_STATE_EARLY:
					dlg->state = DLG_STATE_DELETED;
					unref_dlg_unsafe(dlg,1,d_entry);
					*unref = 1;
					break;
				case DLG_STATE_CONFIRMED_NA:
				case DLG_STATE_CONFIRMED:
					unref_dlg_unsafe(dlg,1,d_entry);
					break;
				case DLG_STATE_DELETED:
					*unref = 1;
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_RPL1xx:
			switch (dlg->state) {
				case DLG_STATE_UNCONFIRMED:
				case DLG_STATE_EARLY:
					dlg->state = DLG_STATE_EARLY;
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_RPL3xx:
			switch (dlg->state) {
				case DLG_STATE_UNCONFIRMED:
				case DLG_STATE_EARLY:
					dlg->state = DLG_STATE_DELETED;
					*unref = 1;
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_RPL2xx:
			switch (dlg->state) {
				case DLG_STATE_DELETED:
					if (dlg->dflags&DLG_FLAG_HASBYE) {
						LM_CRIT("bogus event %d in state %d (with BYE) "
							"for dlg %p [%u:%u] with clid '%.*s' and tags '%.*s' '%.*s'\n",
							event, dlg->state, dlg, dlg->h_entry, dlg->h_id,
							dlg->callid.len, dlg->callid.s,
							dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
							dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
						break;
					}
					ref_dlg_unsafe(dlg,1);
				case DLG_STATE_UNCONFIRMED:
				case DLG_STATE_EARLY:
					dlg->state = DLG_STATE_CONFIRMED_NA;
					break;
				case DLG_STATE_CONFIRMED_NA:
				case DLG_STATE_CONFIRMED:
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_REQACK:
			switch (dlg->state) {
				case DLG_STATE_CONFIRMED_NA:
					dlg->state = DLG_STATE_CONFIRMED;
					break;
				case DLG_STATE_CONFIRMED:
					break;
				case DLG_STATE_DELETED:
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_REQBYE:
			switch (dlg->state) {
				case DLG_STATE_CONFIRMED_NA:
				case DLG_STATE_CONFIRMED:
					dlg->dflags |= DLG_FLAG_HASBYE;
					dlg->state = DLG_STATE_DELETED;
					*unref = 1;
					break;
				case DLG_STATE_EARLY:
				case DLG_STATE_DELETED:
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_REQPRACK:
			switch (dlg->state) {
				case DLG_STATE_EARLY:
				case DLG_STATE_CONFIRMED_NA:
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		case DLG_EVENT_REQ:
			switch (dlg->state) {
				case DLG_STATE_EARLY:
				case DLG_STATE_CONFIRMED_NA:
				case DLG_STATE_CONFIRMED:
					break;
				default:
					log_next_state_dlg(event, dlg);
			}
			break;
		default:
			LM_CRIT("unknown event %d in state %d "
				"for dlg %p [%u:%u] with clid '%.*s' and tags '%.*s' '%.*s'\n",
				event, dlg->state, dlg, dlg->h_entry, dlg->h_id,
				dlg->callid.len, dlg->callid.s,
				dlg->tag[DLG_CALLER_LEG].len, dlg->tag[DLG_CALLER_LEG].s,
				dlg->tag[DLG_CALLEE_LEG].len, dlg->tag[DLG_CALLEE_LEG].s);
	}
	*new_state = dlg->state;

	dlg_unlock( d_table, d_entry);

	LM_DBG("dialog %p changed from state %d to "
		"state %d, due event %d\n",dlg,*old_state,*new_state,event);
}

/**
 *
 */
int dlg_set_toroute(struct dlg_cell *dlg, str *route)
{
	if(dlg==NULL || route==NULL || route->len<=0)
		return 0;
	if(dlg->toroute_name.s!=NULL) {
		shm_free(dlg->toroute_name.s);
		dlg->toroute_name.s = NULL;
		dlg->toroute_name.len = 0;
	}
	dlg->toroute_name.s = (char*)shm_malloc((route->len+1)*sizeof(char));
	if(dlg->toroute_name.s==NULL) {
		LM_ERR("no more shared memory\n");
		return -1;
	}
	memcpy(dlg->toroute_name.s, route->s, route->len);
	dlg->toroute_name.len = route->len;
	dlg->toroute_name.s[dlg->toroute_name.len] = '\0';
	dlg->toroute = route_lookup(&main_rt, dlg->toroute_name.s);
	return 0;
}

/**************************** MI functions ******************************/
/*!
 * \brief Helper method that output a dialog via the MI interface
 * \see mi_print_dlg
 * \param rpl MI node that should be filled
 * \param dlg printed dialog
 * \param with_context if 1 then the dialog context will be also printed
 * \return 0 on success, -1 on failure
 */
static inline int internal_mi_print_dlg(struct mi_node *rpl,
									struct dlg_cell *dlg, int with_context)
{
	struct mi_node* node= NULL;
	struct mi_node* node1 = NULL;
	struct mi_attr* attr= NULL;
	int len;
	char* p;

	node = add_mi_node_child(rpl, 0, "dialog",6 , 0, 0 );
	if (node==0)
		goto error;

	attr = addf_mi_attr( node, 0, "hash", 4, "%u:%u",
			dlg->h_entry, dlg->h_id );
	if (attr==0)
		goto error;

	p= int2str((unsigned long)dlg->state, &len);
	node1 = add_mi_node_child( node, MI_DUP_VALUE, "state", 5, p, len);
	if (node1==0)
		goto error;

	p= int2str((unsigned long)dlg->start_ts, &len);
	node1 = add_mi_node_child(node,MI_DUP_VALUE,"timestart",9, p, len);
	if (node1==0)
		goto error;

	p= int2str((unsigned long)dlg->tl.timeout, &len);
	node1 = add_mi_node_child(node,MI_DUP_VALUE, "timeout", 7, p, len);
	if (node1==0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "callid", 6,
			dlg->callid.s, dlg->callid.len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "from_uri", 8,
			dlg->from_uri.s, dlg->from_uri.len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "from_tag", 8,
			dlg->tag[DLG_CALLER_LEG].s, dlg->tag[DLG_CALLER_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "caller_contact", 14,
			dlg->contact[DLG_CALLER_LEG].s,
			dlg->contact[DLG_CALLER_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "caller_cseq", 11,
			dlg->cseq[DLG_CALLER_LEG].s,
			dlg->cseq[DLG_CALLER_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE,"caller_route_set",16,
			dlg->route_set[DLG_CALLER_LEG].s,
			dlg->route_set[DLG_CALLER_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, 0,"caller_bind_addr",16,
			dlg->bind_addr[DLG_CALLER_LEG]->sock_str.s, 
			dlg->bind_addr[DLG_CALLER_LEG]->sock_str.len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "to_uri", 6,
			dlg->to_uri.s, dlg->to_uri.len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "to_tag", 6,
			dlg->tag[DLG_CALLEE_LEG].s, dlg->tag[DLG_CALLEE_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "callee_contact", 14,
			dlg->contact[DLG_CALLEE_LEG].s,
			dlg->contact[DLG_CALLEE_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE, "callee_cseq", 11,
			dlg->cseq[DLG_CALLEE_LEG].s,
			dlg->cseq[DLG_CALLEE_LEG].len);
	if(node1 == 0)
		goto error;

	node1 = add_mi_node_child(node, MI_DUP_VALUE,"callee_route_set",16,
			dlg->route_set[DLG_CALLEE_LEG].s,
			dlg->route_set[DLG_CALLEE_LEG].len);
	if(node1 == 0)
		goto error;

	if (dlg->bind_addr[DLG_CALLEE_LEG]) {
		node1 = add_mi_node_child(node, 0,
			"callee_bind_addr",16,
			dlg->bind_addr[DLG_CALLEE_LEG]->sock_str.s, 
			dlg->bind_addr[DLG_CALLEE_LEG]->sock_str.len);
	} else {
		node1 = add_mi_node_child(node, 0,
			"callee_bind_addr",16,0,0);
	}
	if(node1 == 0)
		goto error;

	if (with_context) {
		node1 = add_mi_node_child(node, 0, "context", 7, 0, 0);
		if(node1 == 0)
			goto error;
		run_dlg_callbacks( DLGCB_MI_CONTEXT, dlg, NULL, 
			DLG_DIR_NONE, (void *)node1);
	}
	return 0;

error:
	LM_ERR("failed to add node\n");
	return -1;
}


/*!
 * \brief Output a dialog via the MI interface
 * \param rpl MI node that should be filled
 * \param dlg printed dialog
 * \param with_context if 1 then the dialog context will be also printed
 * \return 0 on success, -1 on failure
 */
int mi_print_dlg(struct mi_node *rpl, struct dlg_cell *dlg, int with_context)
{
	return internal_mi_print_dlg( rpl, dlg, with_context);
}

/*!
 * \brief Helper function that output all dialogs via the MI interface
 * \see mi_print_dlgs
 * \param rpl MI node that should be filled
 * \param with_context if 1 then the dialog context will be also printed
 * \return 0 on success, -1 on failure
 */
static int internal_mi_print_dlgs(struct mi_node *rpl, int with_context)
{
	struct dlg_cell *dlg;
	unsigned int i;

	LM_DBG("printing %i dialogs\n", d_table->size);

	for( i=0 ; i<d_table->size ; i++ ) {
		dlg_lock( d_table, &(d_table->entries[i]) );

		for( dlg=d_table->entries[i].first ; dlg ; dlg=dlg->next ) {
			if (internal_mi_print_dlg(rpl, dlg, with_context)!=0)
				goto error;
		}
		dlg_unlock( d_table, &(d_table->entries[i]) );
	}
	return 0;

error:
	dlg_unlock( d_table, &(d_table->entries[i]) );
	LM_ERR("failed to print dialog\n");
	return -1;
}


static inline struct mi_root* process_mi_params(struct mi_root *cmd_tree,
													struct dlg_cell **dlg_p)
{
	struct mi_node* node;
	struct dlg_entry *d_entry;
	struct dlg_cell *dlg;
	str *callid;
	str *from_tag;
	unsigned int h_entry;

	node = cmd_tree->node.kids;
	if (node == NULL) {
		/* no parameters at all */
		*dlg_p = NULL;
		return NULL;
	}

	/* we have params -> get callid and fromtag */
	callid = &node->value;
	LM_DBG("callid='%.*s'\n", callid->len, callid->s);

	node = node->next;
	if ( !node || !node->value.s || !node->value.len) {
		from_tag = NULL;
	} else {
		from_tag = &node->value;
		LM_DBG("from_tag='%.*s'\n", from_tag->len, from_tag->s);
		if ( node->next!=NULL )
			return init_mi_tree( 400, MI_SSTR(MI_MISSING_PARM));
	}

	h_entry = core_hash( callid, 0, d_table->size);

	d_entry = &(d_table->entries[h_entry]);
	dlg_lock( d_table, d_entry);

	for( dlg = d_entry->first ; dlg ; dlg = dlg->next ) {
		if (match_downstream_dialog( dlg, callid, from_tag)==1) {
			if (dlg->state==DLG_STATE_DELETED) {
				*dlg_p = NULL;
				break;
			} else {
				*dlg_p = dlg;
				dlg_unlock( d_table, d_entry);
				return 0;
			}
		}
	}
	dlg_unlock( d_table, d_entry);

	return init_mi_tree( 404, MI_SSTR("Nu such dialog"));
}


/*!
 * \brief Output all dialogs via the MI interface
 * \param cmd_tree MI command tree
 * \param param unused
 * \return mi node with the dialog information, or NULL on failure
 */
struct mi_root * mi_print_dlgs(struct mi_root *cmd_tree, void *param )
{
	struct mi_root* rpl_tree= NULL;
	struct mi_node* rpl = NULL;
	struct dlg_cell* dlg = NULL;

	rpl_tree = process_mi_params( cmd_tree, &dlg);
	if (rpl_tree)
		/* param error */
		return rpl_tree;

	rpl_tree = init_mi_tree( 200, MI_SSTR(MI_OK));
	if (rpl_tree==0)
		return 0;
	rpl = &rpl_tree->node;

	if (dlg==NULL) {
		if ( internal_mi_print_dlgs(rpl,0)!=0 )
			goto error;
	} else {
		if ( internal_mi_print_dlg(rpl,dlg,0)!=0 )
			goto error;
	}

	return rpl_tree;
error:
	free_mi_tree(rpl_tree);
	return NULL;
}


/*!
 * \brief Print a dialog context via the MI interface
 * \param cmd_tree MI command tree
 * \param param unused
 * \return mi node with the dialog information, or NULL on failure
 */
struct mi_root * mi_print_dlgs_ctx(struct mi_root *cmd_tree, void *param )
{
	struct mi_root* rpl_tree= NULL;
	struct mi_node* rpl = NULL;
	struct dlg_cell* dlg = NULL;

	rpl_tree = process_mi_params( cmd_tree, &dlg);
	if (rpl_tree)
		/* param error */
		return rpl_tree;

	rpl_tree = init_mi_tree( 200, MI_SSTR(MI_OK));
	if (rpl_tree==0)
		return 0;
	rpl = &rpl_tree->node;

	if (dlg==NULL) {
		if ( internal_mi_print_dlgs(rpl,1)!=0 )
			goto error;
	} else {
		if ( internal_mi_print_dlg(rpl,dlg,1)!=0 )
			goto error;
	}

	return rpl_tree;
error:
	free_mi_tree(rpl_tree);
	return NULL;
}

/*!
 * \brief Terminate all or selected dialogs via the MI interface
 * \param cmd_tree MI command tree
 * \param param unused
 * \return mi node with the dialog information, or NULL on failure
 */
struct mi_root * mi_terminate_dlgs(struct mi_root *cmd_tree, void *param )
{
	struct mi_root* rpl_tree= NULL;
	struct dlg_cell* dlg = NULL;
	str headers = {0, 0};

	rpl_tree = process_mi_params( cmd_tree, &dlg);
	if (rpl_tree)
		/* param error */
		return rpl_tree;
	if (dlg==NULL)
		return init_mi_tree( 400, MI_SSTR(MI_MISSING_PARM));

	rpl_tree = init_mi_tree( 200, MI_SSTR(MI_OK));
	if (rpl_tree==0)
		return 0;
	if (dlg_bye_all(dlg, &headers)!=0)
		goto error;
	return rpl_tree;
error:
	free_mi_tree(rpl_tree);
	return NULL;
}

