/*
 * run_proxy.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "run_proxy.h"

extern int proxy_to_proxy_port, proxy_to_proxy_data_port;

int
run_proxy(
		int proxy_to_proxy_socket,
		int proxy_to_linphone_socket,
		int proxy_to_proxy_data_socket,
		int proxy_to_linphone_data_socket,
		int configure_socket)
{
	printf ("proxy_to_proxy_port %d and proxy_to_proxy_data_port %d\n", proxy_to_proxy_port, proxy_to_proxy_data_port);
	int rc, num_fds = LISTENING_SOCKETS, i;
	unsigned int proxy_to_proxy_len, proxy_to_linphone_len, proxy_to_proxy_data_len, proxy_to_linphone, manager_len;
	char *buff = malloc(MAX_PACKET_SIZE);

	struct pollfd *fds = malloc (num_fds * sizeof(struct pollfd));
	struct sockaddr_in proxy_to_proxy_sock, proxy_to_linphone_sock, proxy_to_proxy_data_sock, proxy_to_linphone_data_sock, manager_sock;

	/* init caller socket */
	proxy_to_proxy_len = sizeof(proxy_to_proxy_sock);
	memset(&proxy_to_proxy_sock, 0, sizeof(proxy_to_proxy_sock));
	proxy_to_proxy_sock.sin_family = AF_INET;
	proxy_to_proxy_sock.sin_addr.s_addr = inet_addr(CALLER_IP);
	proxy_to_proxy_sock.sin_port = htons(proxy_to_proxy_port);

	/* init receiver socket */
	proxy_to_linphone_len = sizeof(proxy_to_linphone_sock);
	memset(&proxy_to_linphone_sock, 0, sizeof(proxy_to_linphone_sock));
	proxy_to_linphone_sock.sin_family = AF_INET;
	proxy_to_linphone_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	proxy_to_linphone_sock.sin_port = htons(SIP_UDP_PORT);

	/* init caller data socket */
	proxy_to_proxy_data_len = sizeof(proxy_to_proxy_data_sock);
	memset(&proxy_to_proxy_data_sock, 0, sizeof(proxy_to_proxy_data_sock));
	proxy_to_proxy_data_sock.sin_family = AF_INET;
	proxy_to_proxy_data_sock.sin_addr.s_addr = inet_addr(CALLER_IP);
	proxy_to_proxy_data_sock.sin_port = htons(proxy_to_proxy_data_port);

	/* init receiver data socket */
	proxy_to_linphone = sizeof(proxy_to_linphone_data_sock);
	memset(&proxy_to_linphone_data_sock, 0, sizeof(proxy_to_linphone_data_sock));
	proxy_to_linphone_data_sock.sin_family = AF_INET;
	proxy_to_linphone_data_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	proxy_to_linphone_data_sock.sin_port = htons(SIP_DATA_PORT);

	/* init manager socket */
	manager_len = sizeof(manager_sock);
	memset(&manager_sock, 0, manager_len);
	manager_sock.sin_family = AF_INET;
	manager_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	manager_sock.sin_port = htons(MANAGER_PORT);

/* This order of listener must be maintained
 * 0 - stdin
 * 1 - incoming call socket (from outside)
 * 2 - forward socket to localhost linphone application
 * 3 - incoming data socket (from outside)
 * 4 - forward data socket to localhost linphone application
 * 5 - locahost proxy manager
 * please add at the end of the list*/

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	fds[1].fd = proxy_to_proxy_socket;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	fds[2].fd = proxy_to_linphone_socket;
	fds[2].events = POLLIN;
	fds[2].revents = 0;

	fds[3].fd = proxy_to_proxy_data_socket;
	fds[3].events = POLLIN;
	fds[3].revents = 0;

	fds[4].fd = proxy_to_linphone_data_socket;
	fds[4].events = POLLIN;
	fds[4].revents = 0;

	fds[5].fd = configure_socket;
	fds[5].events = POLLIN;
	fds[5].revents = 0;

	while (1) {
		if (poll(fds, num_fds, -1) == -1)
			exit_error("poll error", EXIT_FAILURE);

		//printf("poll out\n");

		for (i=0; i<num_fds; i++) {
			if (fds[i].revents != 0) {
				/* input command */
				if(i == 0) {
					//printf("command received\n");
					fgets(buff, MAX_LEN, stdin);
					buff[strlen(buff) - 1] = '\0';
					if (strcmp(buff, "exit") == 0) {
						goto out;
					}
				}else if(i == 1){ /* call packet from another proxy */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_proxy_sock, &proxy_to_proxy_len)) < 0) {
						printf("CALL: no message received on proxy-to-proxy port\n");
					}
					else {
						printf("CALL: received packet from proxy with source port %d on proxy port with %d bytes\n", ntohs(proxy_to_proxy_sock.sin_port), rc);
						//getchar();
						if (sendto(proxy_to_linphone_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_linphone_sock, sizeof(proxy_to_linphone_sock)) != rc) {
							perror("CALL: Cannot forward message to linphone application\n");
						}
						else {
							printf("CALL: message sent to localhost linphone, size %d bytes\n", rc);
							//getchar();
							memset(buff, 0, MAX_PACKET_SIZE);
						}
					}
				}else if(i == 2) {	/* received something from localhost linphone socket */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_linphone_sock, &proxy_to_linphone_len)) < 0) {
						perror("CALL: no message received on forwarding port\n");
					}
					else {
						printf("CALL: linphone answered with source port %d, on proxy-linphone, packet with %d bytes\n", htons(proxy_to_linphone_sock.sin_port), rc);
						//getchar();
						if (sendto(proxy_to_proxy_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_proxy_sock, sizeof(proxy_to_proxy_sock)) != rc) {
							perror("CALL: Cannot send linphone message back\n");
						}
						else {
							printf("CALL: Sent packet to external proxy with size %d bytes\n", rc);
							memset(buff, 0, MAX_PACKET_SIZE);
						}
					}
				}else if(i == 3) {/* data packet form another caller */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_proxy_data_sock, &proxy_to_proxy_data_len)) < 0) {
						printf ("DATA: no message received on listening data port\n");
					}
					else {
						printf ("DATA: received data packet with source port %d on listening port with %d bytes\n", ntohs(proxy_to_proxy_data_sock.sin_port), rc);
						//getchar();
						fds[i].revents = 0;
						if (sendto(proxy_to_linphone_data_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_linphone_data_sock, sizeof(proxy_to_linphone_data_sock)) != rc) {
							perror("DATA: Cannot forward data packet to linphone application\n");
						}
						else {
							printf("DATA: data message sent to localhost linphone, size %d bytes\n", rc);
							//getchar();
							memset (buff, 0, MAX_PACKET_SIZE);
						}
					}
				}
				else if(i == 4) {	/* data packet to another caller */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_linphone_data_sock, &proxy_to_linphone)) < 0) {
						perror("DATA: no message received on forwarding port\n");
					}
					else {
						printf("DATA: linphone answered with source port %d, on forwarding port, data packet with %d bytes\n", htons(proxy_to_linphone_sock.sin_port), rc);
						//getchar();
						if (sendto(proxy_to_proxy_data_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_proxy_data_sock, sizeof(proxy_to_proxy_data_sock)) != rc) {
							perror("DATA: Cannot forward linphone data packet\n");
						}
						else {
							printf("DATA: Sent data packet to caller with size %d bytes\nDone\n", rc);
							memset (buff, 0, MAX_PACKET_SIZE);
						}
					}
				}
				else if(i == 5) { /* packet from manager */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &manager_sock, &manager_len)) < 0) {
						perror("CONFIGURE: no message received\n");
					}
					else {
						printf("DATA: linphone answered with source port %d, on forwarding port, data packet with %d bytes\n", htons(proxy_to_linphone_sock.sin_port), rc);
						//getchar();
						if (sendto(configure_socket, "ok", 2, 0, (struct sockaddr *) &manager_sock, sizeof(manager_sock)) != rc) {
							perror("CONFIGURE: Cannot answer packet\n");
						}
						else {
							printf("CONFIGURE: Sent data packet to caller with size %d bytes\nDone\n", rc);
							memset (buff, 0, MAX_PACKET_SIZE);
						}
					}
				}
				fds[i].revents = 0;
			}

		}
	}

	out:

	printf("Exiting...\n");
	free(buff);

	return 0;

}
