#ifndef __SCCP_UTILS_H
#define __SCCP_UTILS_H

sccp_device_t * sccp_device_find_byid(const char * name);
sccp_device_t * sccp_device_find_byname(const char * name);
sccp_line_t * sccp_line_find_byname(const char * name);
sccp_line_t * sccp_line_find_byid(sccp_device_t * d, uint8_t instance);
sccp_channel_t * sccp_channel_find_byid(uint32_t id);
sccp_channel_t * sccp_channel_find_byid_on_line(sccp_line_t * l, uint32_t id);
sccp_channel_t * sccp_channel_find_bystate_on_line(sccp_line_t * l, uint8_t state);
sccp_channel_t * sccp_channel_find_bycallstate_on_line(sccp_line_t * l, uint8_t state);
sccp_channel_t * sccp_channel_find_bystate_on_device(sccp_device_t * d, uint8_t state);
sccp_channel_t * sccp_channel_find_byid_on_device(sccp_device_t * d, uint32_t id);

void sccp_cw_setstate(sccp_channel_t * c, int state);

void sccp_dev_dbput(sccp_device_t * d);
void sccp_dev_dbget(sccp_device_t * d);
void sccp_dev_dbclean(void);

const char * sccp_extensionstate2str(uint8_t type);
const char * sccpmsg2str(uint32_t e);
const char * skinny_lbl2str(uint8_t label);
const char * skinny_tone2str(uint8_t tone);
const char * skinny_alarm2str(uint8_t alarm);
const char * skinny_devicetype2str(uint32_t type);
const char * skinny_stimulus2str(uint8_t type);
const char * skinny_buttontype2str(uint8_t type);
const char * skinny_lampmode2str(uint8_t type);
const char * skinny_ringermode2str(uint8_t type);
const char * skinny_softkeyset2str(uint8_t type);
const char * skinny_devicestate2str(uint8_t type);
const char * skinny_registrationstate2str(uint8_t type);
const char * skinny_calltype2str(uint8_t type);
const char * skinny_codec2str(uint8_t type);
const char * sccp_dndmode2str(uint8_t type);
uint8_t sccp_codec_ast2skinny(int fmt);
int sccp_codec_skinny2ast(uint8_t fmt);

unsigned int sccp_app_separate_args(char *buf, char delim, char **array, int arraylen);

#endif
