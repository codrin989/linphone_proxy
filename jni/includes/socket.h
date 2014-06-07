/*
 * socket.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifndef SOCKET_H_
#define SOCKET_H_ 1

#define MAX_PACKET_SIZE 1500

#define SIP_PORT 5060 /* TODO set dynamic */
#define SIP_LINPHONE 50616
#define SIP_PROXY 50606

#define DATA_PORT 7078 /* TODO set dynamic */
#define DATA_LINPHONE 51796
#define DATA_PROXY 51786

#define CONFIGURE_PORT 52124
#define MANAGER_PORT 52123

typedef struct _packet_t {
	char buffer[MAX_PACKET_SIZE];
	int size;
} packet_t;

inline void copy_packet(packet_t *packet, char *buffer, int size);
inline int create_socket(int type, int port);
inline void init_sockaddr(struct sockaddr_in *addr, char *ip, int port);
inline void add_poll(struct pollfd *p, int fd);
inline int recv_msg(int fd, struct sockaddr_in *addr, char *buffer);
inline void send_msg(int fd, struct sockaddr_in *addr, char *buffer, int count);
#endif /* SOCKET_H_ */
#ifdef __cplusplus
}
#endif
