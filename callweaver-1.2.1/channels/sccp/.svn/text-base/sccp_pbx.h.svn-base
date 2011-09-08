#ifndef __SCCP_PBX_H
#define __SCCP_PBX_H

uint8_t sccp_pbx_channel_allocate(sccp_channel_t * c);
void * sccp_pbx_startchannel (void *data);
void start_rtp(sccp_channel_t * sub);

const struct cw_channel_tech sccp_tech;

void sccp_pbx_senddigit(sccp_channel_t * c, char digit);
void sccp_pbx_senddigits(sccp_channel_t * c, char digits[CW_MAX_EXTENSION]);

#endif
