#ifndef UDPFROMTO_H
#define UDPFROMTO_H

#include <sys/socket.h>

int cw_udpfromto_init(int s);
int cw_recvfromto(int s, void *buf, size_t len, int flags,
	struct sockaddr *from, socklen_t *fromlen,
	struct sockaddr *to, socklen_t *tolen);
int cw_sendfromto(int s, void *buf, size_t len, int flags,
	struct sockaddr *from, socklen_t fromlen,
	struct sockaddr *to, socklen_t tolen);

#endif
