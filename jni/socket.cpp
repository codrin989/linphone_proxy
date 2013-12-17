/*
 * socket.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "includes/socket.h"

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
