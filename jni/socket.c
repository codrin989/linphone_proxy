/*
 * socket.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#include "includes/utils.h"
#include "includes/socket.h"

/* creates and binds a socket */
int create_socket(int type, int port)
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

void init_sockaddr(struct sockaddr_in *addr, char *ip, int port)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(port);
}

void add_poll(struct pollfd *p, int fd)
{
	p->fd =fd;
	p->events = POLLIN;
	p->revents = 0;
}
