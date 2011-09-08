/* Copyright (C) 2008 Telecats BV
 *
 * This file is part of openser, a free SIP server.
 *
 * openser is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * openser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * History:
 * ---------
 *  2008-07-14 first version, function definitions copied from textops.c (Ardjan Zwartjes)
 */

#ifndef TEXTOPS_H_
#define TEXTOPS_H_
#include "../../mod_fix.h"

int search_f(struct sip_msg*, char*, char*);
int search_append_f(struct sip_msg*, char*, char*);
int remove_hf_f(struct sip_msg* msg, char* str_hf, char* foo);
int add_hf_helper(struct sip_msg* msg, str *str1, str *str2, gparam_p hfval, int mode, gparam_p hfanc);

int fixup_regexp_none(void** param, int param_no);
int fixup_free_regexp_none(void** param, int param_no);
#endif /*TEXTOPS_H_*/
