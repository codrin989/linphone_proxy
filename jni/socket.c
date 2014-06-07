#include "includes/socket.h"

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

	memset(buffer, 0, MAX_PACKET_SIZE);
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
