/*
 * socket.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define SIP_PORT 5060 /* TODO set dynamic */
#define SIP_LINPHONE 50616
#define SIP_PROXY 50606

#define DATA_PORT 7078 /* TODO set dynamic */
#define DATA_LINPHONE 51796
#define DATA_PROXY 51786

#define CONFIGURE_PORT 52124
#define MANAGER_PORT 52123

int create_socket(int domain, int type);
void init_sockaddr(struct sockaddr_in *addr, char *ip, int port);
void add_poll(struct pollfd *p, int fd);

#ifdef __cplusplus
}
#endif

#endif /* SOCKET_H_ */
