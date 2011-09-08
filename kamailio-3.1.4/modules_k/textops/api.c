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
 *  2008-07-14 first version (Ardjan Zwartjes)
 */


#include "api.h"
#include "textops.h"
#include "../../mod_fix.h"
#include "../../mem/mem.h"


/*
 * User friendly wrapper around add_hf_helper, to be called from 
 * other modules.
 */
int append_hf_api(struct sip_msg *msg, str* str_hf){
	return add_hf_helper(msg, str_hf, NULL, NULL, 0, NULL);
}

/*
 * User friendly wrapper around remove_hf_f, to be called from 
 * other modules.
 */
int remove_hf_api(struct sip_msg *msg, str* str_hf){
	return remove_hf_f(msg, (char*)str_hf,NULL);
}


/*
 * User friendly wrapper to call search_append from other
 * modules
 */

int search_append_api(struct sip_msg *msg, str *regex, str *data_str){
	int retval;
	char *data;
	void **param;
	
	data=pkg_malloc(data_str->len+1);
	memcpy(data,data_str->s,data_str->len);
	memset(data+data_str->len,0,1);
	
	param=pkg_malloc(sizeof(void*));
	*param=pkg_malloc(regex->len+1);
	memcpy(*param,regex->s,regex->len);
	memset(*param+regex->len,0,1);
	
	fixup_regexp_none(param,1);
	
	retval=search_append_f(msg, *param, data);
	
	fixup_free_regexp_none(param,1);

	pkg_free(param);
	pkg_free(data);
	
	return retval;
	
}

/*
 * User friendly wrapper to call search from other modules.
 */
int search_api(struct sip_msg *msg, str *regex){
	int retval;

	void **param=pkg_malloc(sizeof(void*));
	
	*param=pkg_malloc(regex->len+1);
	memcpy(*param,regex->s,regex->len);
	memset(*param+regex->len,0,1);
	
	fixup_regexp_none(param,1);
	
	retval=search_f(msg, *param, NULL);
	
	fixup_free_regexp_none(param,1);
	pkg_free(param);
	
	return retval;
	
}
/*
 * Function to load the textops api.
 */
int load_textops(struct textops_binds *tob){
	if(tob==NULL){
		LM_WARN("textops_binds: Cannot load textops API into a NULL pointer\n");
		return -1;
	}
	tob->append_hf=append_hf_api;
	tob->remove_hf=remove_hf_api;
	tob->search_append=search_append_api;
	tob->search=search_api;
	return 0;
}
