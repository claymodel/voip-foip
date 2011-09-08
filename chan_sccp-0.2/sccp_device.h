sccp_device_t * sccp_find_device(const char * name);
sccp_channel_t * sccp_get_active_channel(sccp_device_t * d);
sccp_channel_t * sccp_device_active_channel(sccp_device_t * d);

int  sccp_dev_send(sccp_device_t * d, sccp_moo_t * r);
int  sccp_session_send(sccp_session_t * s, sccp_moo_t * r);
void sccp_dev_sendmsg(sccp_device_t * d, sccp_message_t t);
void sccp_session_sendmsg(sccp_session_t * s, sccp_message_t t);

int sccp_dev_delete_channel(sccp_channel_t * c);
void sccp_dev_cleardisplay(sccp_device_t * d);
void sccp_dev_set_registered(sccp_device_t * d, int opt);
void sccp_dev_set_callstatus(sccp_channel_t * c, int status);
void sccp_dev_set_keyset(sccp_device_t * d, sccp_channel_t * c, int opt);
void sccp_dev_set_sptone(sccp_device_t * d, char * tstr);
void sccp_dev_set_ringer(sccp_device_t * d, int opt);
void sccp_dev_set_speaker(sccp_device_t * d, StationSpeakerMode opt);
void sccp_dev_set_microphone(sccp_device_t * d, StationMicrophoneMode opt);
void sccp_dev_set_mwi(sccp_device_t * d, int line, int hasMail);
void sccp_dev_set_cplane(sccp_device_t * d, int status, int line);
void sccp_dev_set_sptone_byid(sccp_device_t * d, int tone);
void sccp_dev_statusprompt_set(sccp_device_t * d, sccp_channel_t * c, char * msg, int timeout);
sccp_channel_t * sccp_dev_allocate_channel(sccp_device_t * d, sccp_line_t * l, int outgoing, char * dial);

sccp_speed_t * sccp_dev_speed_find_byindex(sccp_device_t * d, int ind);
int sccp_dev_attach_line(sccp_device_t * d, sccp_line_t * l);
void sccp_dev_remove_channel(sccp_channel_t * c);
sccp_device_t * sccp_dev_find_byid(char * name);
sccp_line_t * sccp_dev_get_activeline(sccp_device_t * d);
void sccp_dev_check_mwi(sccp_device_t * d);
void sccp_device_select_line(sccp_device_t * d, sccp_line_t * l);

#define REQ(r, t) { \
  int s = sizeof(r->msg.t); \
  r = malloc(sizeof(sccp_moo_t)); \
  memset(r, 0,  s+12); \
  r->length = s + 4; \
  r->messageId = t; }

#define REQCMD(r, t) { \
  r = malloc(sizeof(sccp_moo_t)); \
  memset(r, 0,  12); \
  memset(r, 0, 12); \
  r->length = 4; \
  r->messageId = t; \
  }
