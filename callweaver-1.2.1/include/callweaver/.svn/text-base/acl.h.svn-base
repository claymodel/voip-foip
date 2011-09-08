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

/*! \file
 * \brief Access Control of various sorts
 */

#ifndef _CALLWEAVER_ACL_H
#define _CALLWEAVER_ACL_H


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <netinet/in.h>
#include "callweaver/io.h"

#define CW_SENSE_DENY                  0
#define CW_SENSE_ALLOW                 1

/* Host based access control */

struct cw_ha;

extern void cw_free_ha(struct cw_ha *ha);
extern struct cw_ha *cw_append_ha(char *sense, char *stuff, struct cw_ha *path);
extern int cw_apply_ha(struct cw_ha *ha, struct sockaddr_in *sin);
extern int cw_get_ip(struct sockaddr_in *sin, const char *value);
extern int cw_get_ip_or_srv(struct sockaddr_in *sin, const char *value, const char *service);
extern int cw_ouraddrfor(struct in_addr *them, struct in_addr *us);
extern int cw_lookup_iface(char *iface, struct in_addr *address);
extern struct cw_ha *cw_duplicate_ha_list(struct cw_ha *original);
extern int cw_find_ourip(struct in_addr *ourip, struct sockaddr_in bindaddr);
extern int cw_str2tos(const char *value, int *tos);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CALLWEAVER_ACL_H */
