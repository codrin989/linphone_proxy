/*
 * socket.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "socket.h"
#include <netinet/tcp.h>

int
create_socket(int domain, int type) {
	int sockfd;

	sockfd = socket(domain, type, 0);
	if (sockfd < 0) {
		return_error("cannot open socket", sockfd);
	}

	return sockfd;
}

int
bind_socket(int sockfd, int domain, int portno) {
	struct sockaddr_in serv_addr;
	int rc;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = domain;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	rc = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (rc < 0) {
		close(sockfd);
		return_error("cannot bind socket", rc);
	}
	return rc;
}

void
destroy_socket(int sockfd) {
	close(sockfd);
}

int
listen_socket(int sockfd) {
	return listen(sockfd, 5);
}


int
socket_type_get(int sockfd)
{
	int optval;

	if (getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &optval, sizeof(optval))) {
		perror("Cannot get socket TYPE:");
		return -1;
	}
	return optval;
}

int
tcp_state_get(int sockfd)
{
	struct tcp_info info;
	int size = sizeof(info);

	if (getsockopt(sockfd, SOL_TCP, TCP_INFO, &info, sizeof(size))) {
		perror("Cannot get socket TYPE:");
		return -1;
	}
	return info.tcpi_state;
}


int
tcp_repair_socket_get(int sockfd, struct tcp_server_repair *repair)
{

	return 0;
}

int
tcp_repair_socket_get(int sockfd, struct tcp_server_repair *repair)
{

	/* Not implemented */
	/*
	if (app_sock_descr.type != SOCK_STREAM) {
									printf("Error: Trying to dump non-TCP socket\n");
								}
*/
	return 0;
}
