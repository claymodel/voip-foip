/*
 * ICD - Intelligent Call Distributor 
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com>
 * Additions, Changes and Support by Tim R. Clark <tclark at shaw dot ca>
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o., Piotr Figurny and Michal Bielicki
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2005, Digium, Inc.
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

#ifndef ICD_BRIDGE_H
#define ICD_BRIDGE_H

int icd_bridge_wait_ack(icd_caller * that);
int icd_bridge_call(icd_caller *bridger, icd_caller *bridgee);

struct cw_channel *icd_request_and_dial(char *type, int format, void *data, int timeout, int *outstate,
    char *callerid, icd_caller * caller, icd_caller * peer, icd_caller_state req_state);

/* This is the function icd_caller calls to translate strings into an callweaver channel */
struct cw_channel *icd_bridge_get_callweaver_channel(char *chanstring, char *context, char *priority,
    char *extension);

/* This is the function that icd_caller calls to dial out, typically for onhook agents */
int icd_bridge_dial_callweaver_channel(icd_caller * caller, char *chanstring, int timeout);

/* check the hangup status of the caller's chan (frontended for future cross compatibility) */
int icd_bridge__check_hangup(icd_caller * that);

void icd_bridge__safe_hangup(icd_caller * caller);

/* lift from * when we need to sleep a live up channel for wrapup */
int icd_safe_sleep(struct cw_channel *chan, int ms);

int icd_bridge__wait_call_customer(icd_caller * that);
int icd_bridge__wait_call_agent(icd_caller * that);
void icd_bridge__unbridge_caller(icd_caller * caller, icd_unbridge_flag ubf);
void icd_bridge__parse_ubf(icd_caller * caller, icd_unbridge_flag ubf);
void icd_bridge__remasq(icd_caller * caller);

int icd_bridge__play_sound_file(struct cw_channel *chan, char *file);
#endif

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

