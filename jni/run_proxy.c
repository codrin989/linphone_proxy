/*
 * run_proxy.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "run_proxy.h"

typedef enum {
	NONE = 0,
	DATA_COPY,
	INITIATING,
	INTERCEPTING,
	MIRRORING
}state;

state proxy_state;


extern int proxy_to_proxy_port, proxy_to_proxy_data_port;

int
run_proxy(
		int proxy_to_proxy_socket,
		int proxy_to_linphone_socket,
		int proxy_to_proxy_data_socket,
		int proxy_to_linphone_data_socket,
		int configure_socket,
		char *remote_ip)
{
	proxy_state = NONE;
	printf ("proxy_to_proxy_port %d and proxy_to_proxy_data_port %d\n", proxy_to_proxy_port, proxy_to_proxy_data_port);
	int rc, num_fds = LISTENING_SOCKETS, i;
	unsigned int proxy_to_proxy_len, proxy_to_linphone_len, proxy_to_proxy_data_len,
		proxy_to_linphone_data_len, manager_len, mirror_len;
	char *buff = malloc(MAX_PACKET_SIZE), *mirror_ip;

	struct pollfd *fds = malloc (num_fds * sizeof(struct pollfd));
	struct sockaddr_in proxy_to_proxy_sock, proxy_to_linphone_sock,
	proxy_to_proxy_data_sock, proxy_to_linphone_data_sock,
	manager_sock, mirror_sock;

	/* init caller socket */
	proxy_to_proxy_len = sizeof(proxy_to_proxy_sock);
	memset(&proxy_to_proxy_sock, 0, sizeof(proxy_to_proxy_sock));
	proxy_to_proxy_sock.sin_family = AF_INET;
	proxy_to_proxy_sock.sin_addr.s_addr = inet_addr(remote_ip);
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
	proxy_to_proxy_data_sock.sin_addr.s_addr = inet_addr(remote_ip);
	proxy_to_proxy_data_sock.sin_port = htons(proxy_to_proxy_data_port);

	/* init receiver data socket */
	proxy_to_linphone_data_len = sizeof(proxy_to_linphone_data_sock);
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
					if (proxy_state != MIRRORING)
						proxy_state = INITIATING;

					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_proxy_sock, &proxy_to_proxy_len)) < 0) {
						printf("CALL: no message received on proxy-to-proxy port\n");
					}
					else {
						printf("CALL: from [REMOTE: %d] - %d bytes\n", ntohs(proxy_to_proxy_sock.sin_port), rc);
						if (sendto(proxy_to_linphone_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_linphone_sock, sizeof(proxy_to_linphone_sock)) != rc) {
							perror("CALL: Cannot forward message to linphone application\n");
						}
						else {
							printf("CALL: sent [LINPHONE] - %d bytes\n", rc);
							memset(buff, 0, MAX_PACKET_SIZE);
						}
					}
				}else if(i == 2) {	/* received something from localhost linphone socket */
					if (proxy_state != MIRRORING)
						proxy_state = INITIATING;

					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_linphone_sock, &proxy_to_linphone_len)) < 0) {
						perror("CALL: no message received on forwarding port\n");
					}
					else {
						printf("CALL: from [LINPHONE: %d] - %d bytes\n", htons(proxy_to_linphone_sock.sin_port), rc);
						//getchar();
						if (sendto(proxy_to_proxy_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_proxy_sock, sizeof(proxy_to_proxy_sock)) != rc) {
							perror("CALL: Cannot send linphone message back\n");
						}
						else {
							printf("CALL: sent [REMOTE] - %d bytes\n", rc);
							memset(buff, 0, MAX_PACKET_SIZE);
						}
					}
				}else if(i == 3) {/* data packet form another caller */
					if (proxy_state == NONE || proxy_state == DATA_COPY)
						proxy_state = DATA_COPY;
					else {
						if (proxy_state < INTERCEPTING)
							proxy_state = INTERCEPTING;
					}

					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_proxy_data_sock, &proxy_to_proxy_data_len)) < 0) {
						printf ("DATA: no message received on listening data port\n");
					}
					else {
						fds[i].revents = 0;
						if (proxy_state != DATA_COPY) {/* TODO: for now */
							printf ("DATA: from [REMOTE: %d] - %d bytes\n", ntohs(proxy_to_proxy_data_sock.sin_port), rc);
							if (sendto(proxy_to_linphone_data_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_linphone_data_sock, sizeof(proxy_to_linphone_data_sock)) != rc) {
								perror("DATA: Cannot forward data packet to linphone application\n");
							}
							else {
								printf("DATA: sent [LINPHONE] - %d bytes\n", rc);
							}

							if (proxy_state == MIRRORING) {

								if (sendto(proxy_to_proxy_data_socket, buff, rc, 0, (struct sockaddr *) &mirror_sock, sizeof(mirror_len)) != rc) {
									printf("MIRROR_DATA: Cannot forward data packet to mirror (%s)\n", mirror_ip);
									perror("MIRRORING");
								}
								else {
									printf("MIRROR_DATA: sent [MIRROR] - %d bytes\n", rc);
								}
							}

							memset (buff, 0, MAX_PACKET_SIZE);
						} else {
							/* TODO: forward later to local linphone */
							printf ("MIRROED_DATA: from [REMOTE: %d] - %d bytes\n", ntohs(proxy_to_proxy_data_sock.sin_port), rc);
							memset (buff, 0, MAX_PACKET_SIZE);
						}
					}
				}
				else if(i == 4) {	/* data packet to another caller */
					if (proxy_state == NONE)
						printf ("ERR: INTERCEPTING is not possible without INITIATION\n");
					else {
						if (proxy_state < INTERCEPTING)
							proxy_state = INTERCEPTING;
					}

					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &proxy_to_linphone_data_sock, &proxy_to_linphone_data_len)) < 0) {
						perror("DATA: no message received on forwarding port\n");
					}
					else {
						printf("DATA: from [LINPHONE: %d] - %d bytes\n", htons(proxy_to_linphone_data_sock.sin_port), rc);
						if (sendto(proxy_to_proxy_data_socket, buff, rc, 0, (struct sockaddr *) &proxy_to_proxy_data_sock, sizeof(proxy_to_proxy_data_sock)) != rc) {
							perror("DATA: Cannot forward linphone data packet\n");
						}
						else {
							printf("DATA: sent [REMOTE] - %d bytes\nDone\n", rc);
							memset (buff, 0, MAX_PACKET_SIZE);
						}
					}
				}
				else if(i == 5) { /* packet from manager */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &manager_sock, &manager_len)) < 0) {
						perror("CONFIGURE: no message received\n");
					}
					else {
						printf("MGM: from [PROXY MANAGER: %d] - %d bytes\n\tMessage: %s\n", htons(manager_sock.sin_port), rc, buff);

						if (strcmp (buff, "Test") == 0)
						{
							strcpy (buff, "Test ok");
						} else if (strstr (buff, "Mirror") == buff){
							printf ("Mirroring command received: %s\n", buff);

							if (proxy_state == MIRRORING) {
								strcpy (buff, "ERR: allready mirroring");
							} else if (proxy_state != INTERCEPTING) {
								strcpy (buff, "ERR: No packets intercepting at the moment");
							} else {
								proxy_state = MIRRORING;
								mirror_ip = strdup(buff + strlen("mirror") + 2);/* TODO: carefull here */
								printf ("DBG: mirrored ip is %s\n", mirror_ip);
								/* init mirror socket */
								mirror_len = sizeof(mirror_sock);
								memset(&mirror_sock, 0, mirror_len);
								mirror_sock.sin_family = AF_INET;
								mirror_sock.sin_addr.s_addr = inet_addr(mirror_ip);
								mirror_sock.sin_port = htons(proxy_to_proxy_data_port);

								sprintf (buff, "mirroring started to %s", mirror_ip);
							}
						} else if (strcmp (buff, "Stop") == 0 && proxy_state == MIRRORING) {
							proxy_state = INTERCEPTING;
							strcpy (buff, "mirroring stopped");
							free (mirror_ip);
						} else {
							strcpy (buff, "unknown command");
						}

						if (sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &manager_sock, sizeof(manager_sock)) != (ssize_t)strlen(buff)) {
							perror("CONFIGURE: Cannot answer to [PROXY MANAGER]\n");
						}
						else {
							printf("MGM: sent [PROXY MANAGER: %s] - %d bytes\n", buff, rc);
						}
						memset (buff, 0, MAX_PACKET_SIZE);
					}
				}
				printf("\n");
				fds[i].revents = 0;
				//getchar();
			}

		}
	}

	out:

	printf("Exiting...\n");
	free(buff);

	return 0;

}
