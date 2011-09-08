#ifndef __SCCP_HELPER_H
#define __SCCP_HELPER_H

const char * sccpmsg2str(int e);
int sccp_line_hasmessages(sccp_line_t * l);
sccp_intercom_t * sccp_intercom_find_byname(char * name);
char * sccp_helper_getversionfor(sccp_session_t * s);

#endif /* __SCCP_HELPER_H */

