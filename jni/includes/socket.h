/*
 * socket.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int
create_socket(int domain, int type);

void
destroy_socket(int sockfd);

int
bind_socket(int sockfd, int domain, int portno, unsigned int source_address);

int
listen_socket(int sockfd);

#endif /* SOCKET_H_ */
