sccp_line_t * sccp_line_find_byname(char * line);
sccp_line_t * sccp_line_find_byid(sccp_device_t * d, int instance);

void sccp_line_dial(sccp_line_t * l, char * num);
void sccp_line_kill(sccp_line_t * l);



