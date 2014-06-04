/*
 * socket.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifdef __cplusplus
extern "C" {
#endif
#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "utils.h"

#define SIP_PORT 5060 /* TODO set dynamic */
#define SIP_LINPHONE 50616
#define SIP_PROXY 50606

#define DATA_PORT 7078 /* TODO set dynamic */
#define DATA_LINPHONE 51796
#define DATA_PROXY 51786

#define CONFIGURE_PORT 52124
#define MANAGER_PORT 52123

#define MAX_PACKET_SIZE 1500

typedef struct _packet_t {
	char buffer[MAX_PACKET_SIZE];
	int size;
} packet_t;

inline void copy_packet(packet_t *packet, char *buffer, int size)
{
	memset(packet->buffer, 0, MAX_PACKET_SIZE);
	memcpy(packet->buffer, buffer, size);
	packet->size = size;
}

inline int create_socket(int type, int port)
{
	int fd = -1, rc;
	struct sockaddr_in serv_addr;

	fd = socket(AF_INET, type, 0);
	DIE(-1 == fd, "socket");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	rc = bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	DIE(-1 == rc, "bind");

	return fd;
}

inline void init_sockaddr(struct sockaddr_in *addr, char *ip, int port)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(port);
}

inline void add_poll(struct pollfd *p, int fd)
{
	p->fd =fd;
	p->events = POLLIN;
	p->revents = 0;
}

inline int recv_msg(int fd, struct sockaddr_in *addr, char *buffer)
{
	char address[INET_ADDRSTRLEN];
	socklen_t addrlen;
	int rc;

	rc = recvfrom(fd, buffer, MAX_PACKET_SIZE, 0,
			(struct sockaddr *)addr, &addrlen);
	DIE(-1 == rc, "recvfrom");
#if 1
	inet_ntop(AF_INET, &(addr->sin_addr), address, INET_ADDRSTRLEN);
	eprintf("recv <%s:%d> %d bytes\n",
			address, ntohs(addr->sin_port), rc);
#endif
	return rc;
}

inline void send_msg(int fd, struct sockaddr_in *addr, char *buffer, int count)
{
	char address[INET_ADDRSTRLEN];
	int rc;

	rc = sendto(fd, buffer, count, 0,
			(struct sockaddr *)addr, sizeof(struct sockaddr_in));
	DIE(rc != count, "sendto");
#if 1
	inet_ntop(AF_INET, &(addr->sin_addr), address, INET_ADDRSTRLEN);
	eprintf("send <%s:%d> %d bytes\n",
			address, ntohs(addr->sin_port), rc);
#endif
}

#endif /* SOCKET_H_ */

#ifdef __cplusplus
}
#endif
