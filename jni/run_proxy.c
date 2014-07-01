/*
 * run_proxy.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "run_proxy.h"

typedef enum {
	NONE = 0,		/* proxy is initialized, but no packet has been received */
	DATA_COPY,		/* proxy is getting mirrored data packets from another proxy */
	INITIATING,		/* proxy received only SIP packets, that initiate the VoIP connection */
	INTERCEPTING,	/* proxy received SIP packets and receives data packets, but no mirroring is done */
	MIRRORING		/* proxy received SIP packets and receives data packets and mirroring is performed
	 	 	 	 	 	 for external data packets */
}state;

typedef enum {
	NORMAL = 0,		/* proxy acts normally forwarding data between a socket and another */
	STOPPED,		/* proxy no longer reads/forwards packets from sockets */
}tcp_state;

state proxy_state;
static tcp_state tcp_proxy_state;

extern enum behavior_type behavior;

int
run_proxy(
		struct udp_session *linphone_proxy,
		struct tcp_session *vnc_proxy,
		int configure_socket,
		char *remote_ip)
{
	proxy_state = NONE;
	int has_drop = 0;
	printf ("proxy_to_proxy_port %d and proxy_to_proxy_data_port %d\n", linphone_proxy->proxy_to_proxy_port, linphone_proxy->proxy_to_proxy_data_port);
	int rc, num_fds = LISTENING_SOCKETS, i, j;
	unsigned int udp_proxy_to_proxy_len, udp_proxy_to_linphone_len, udp_proxy_to_proxy_data_len,
		udp_proxy_to_linphone_data_len, manager_len, mirror_len, tcp_proxy_to_proxy_len, tcp_proxy_to_app_len, configure_len;
	char *buff = malloc(MAX_PACKET_SIZE), *mirror_ip;

	/* keep an extra one when acting as a client on the TCP side */
	struct pollfd *fds = malloc (num_fds* sizeof(struct pollfd));
	struct sockaddr_in udp_proxy_to_proxy_sock, udp_proxy_to_linphone_sock, udp_proxy_to_proxy_data_sock, udp_proxy_to_linphone_data_sock,
		tcp_proxy_to_proxy_sock, tcp_proxy_to_app_sock, manager_sock, mirror_sock, configure_sock;
	int tcp_proxy_to_proxy_client_socket = -1, tcp_proxy_to_app_client_socket = -1;
	int connected = 0;

	/* init UDP caller socket */
	udp_proxy_to_proxy_len = sizeof(udp_proxy_to_proxy_sock);
	memset(&udp_proxy_to_proxy_sock, 0, sizeof(udp_proxy_to_proxy_sock));
	udp_proxy_to_proxy_sock.sin_family = AF_INET;
	udp_proxy_to_proxy_sock.sin_addr.s_addr = inet_addr(remote_ip);
	udp_proxy_to_proxy_sock.sin_port = htons(linphone_proxy->proxy_to_proxy_port);

	/* init UDP receiver socket */
	udp_proxy_to_linphone_len = sizeof(udp_proxy_to_linphone_sock);
	memset(&udp_proxy_to_linphone_sock, 0, sizeof(udp_proxy_to_linphone_sock));
	udp_proxy_to_linphone_sock.sin_family = AF_INET;
	udp_proxy_to_linphone_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	udp_proxy_to_linphone_sock.sin_port = htons(SIP_UDP_PORT);

	/* init UDP caller data socket */
	udp_proxy_to_proxy_data_len = sizeof(udp_proxy_to_proxy_data_sock);
	memset(&udp_proxy_to_proxy_data_sock, 0, sizeof(udp_proxy_to_proxy_data_sock));
	udp_proxy_to_proxy_data_sock.sin_family = AF_INET;
	udp_proxy_to_proxy_data_sock.sin_addr.s_addr = inet_addr(remote_ip);
	udp_proxy_to_proxy_data_sock.sin_port = htons(linphone_proxy->proxy_to_proxy_data_port);

	/* init UDP receiver data socket */
	udp_proxy_to_linphone_data_len = sizeof(udp_proxy_to_linphone_data_sock);
	memset(&udp_proxy_to_linphone_data_sock, 0, sizeof(udp_proxy_to_linphone_data_sock));
	udp_proxy_to_linphone_data_sock.sin_family = AF_INET;
	udp_proxy_to_linphone_data_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	udp_proxy_to_linphone_data_sock.sin_port = htons(SIP_DATA_PORT);

	/* init TCP caller socket */
	tcp_proxy_to_proxy_len = sizeof(tcp_proxy_to_proxy_sock);
	memset(&tcp_proxy_to_proxy_sock, 0, tcp_proxy_to_proxy_len);
	tcp_proxy_to_proxy_sock.sin_family = AF_INET;
	tcp_proxy_to_proxy_sock.sin_addr.s_addr = inet_addr(remote_ip);
	tcp_proxy_to_proxy_sock.sin_port = htons(vnc_proxy->proxy_to_proxy_port);

	/* init TCP receiver socket */
	tcp_proxy_to_app_len = sizeof(tcp_proxy_to_app_sock);
	memset(&tcp_proxy_to_app_sock, 0, tcp_proxy_to_app_len);
	tcp_proxy_to_app_sock.sin_family = AF_INET;
	tcp_proxy_to_app_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	tcp_proxy_to_app_sock.sin_port = htons(VNC_TCP_PORT);

	if (behavior == SERVER) {
		/* Act like a SERVER and listen to CLIENTS */

		/* listen on the proxy-to-proxy socket for incomming connections to server */
		rc = listen(vnc_proxy->proxy_to_proxy_socket, 5);
		if (rc < 0)
			return_error("Failed to listen on the proxy-to-proxy TCP socket\n", rc);

	} else if (behavior == CLIENT) {

		/* Act as server for the localhost VNC client */
		/* listen on the proxy-to-app socket for incomming connections from VNC client */
		rc = listen(vnc_proxy->proxy_to_app_socket, 5);
		if (rc < 0)
			return_error("Failed to listen on the proxy-to-app TCP socket\n", rc);

	} else {
		return_error("Undefined behavior %d\n", behavior);
	}

	/* init manager socket */
	manager_len = sizeof(manager_sock);
	memset(&manager_sock, 0, manager_len);
	manager_sock.sin_family = AF_INET;
	manager_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
	manager_sock.sin_port = htons(MANAGER_PORT);

	/* init configure socket - used to communicate with other proxies */
	configure_len = sizeof(configure_sock);
	memset(&configure_sock, 0, configure_len);
	configure_sock.sin_family = AF_INET;
	configure_sock.sin_addr.s_addr = inet_addr(remote_ip);
	configure_sock.sin_port = htons(CONFIGURE_PORT);

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

	fds[1].fd = linphone_proxy->proxy_to_proxy_socket;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	fds[2].fd = linphone_proxy->proxy_to_linphone_socket;
	fds[2].events = POLLIN;
	fds[2].revents = 0;

	fds[3].fd = linphone_proxy->proxy_to_proxy_data_socket;
	fds[3].events = POLLIN;
	fds[3].revents = 0;

	fds[4].fd = linphone_proxy->proxy_to_linphone_data_socket;
	fds[4].events = POLLIN;
	fds[4].revents = 0;

	fds[5].fd = configure_socket;
	fds[5].events = POLLIN;
	fds[5].revents = 0;

	if (behavior == SERVER)
		fds[6].fd = vnc_proxy->proxy_to_proxy_socket;
	else if (behavior == CLIENT)
		fds[6].fd = -1;
	fds[6].events = POLLIN;
	fds[6].revents = 0;

	if (behavior == CLIENT)
		fds[7].fd = vnc_proxy->proxy_to_app_socket;
	else if (behavior == SERVER)
		fds[7].fd = -1;
	fds[7].events = POLLIN;
	fds[7].revents = 0;

	/* Will be completed later */
	fds[8].fd = -1;
	fds[8].events = POLLIN;
	fds[8].revents = 0;

	tcp_proxy_state = NORMAL;
	while (1) {
		if (poll(fds, num_fds, -1) == -1)
			exit_error("poll error", EXIT_FAILURE);

		//printf("poll out\n");

		for (i = 0; i < num_fds; i++) {
			if (fds[i].revents == 0)
				continue;
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

				if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &udp_proxy_to_proxy_sock, &udp_proxy_to_proxy_len)) < 0) {
					printf("CALL: no message received on proxy-to-proxy port\n");
				}
				else {
					printf("CALL: from [REMOTE: %d] - %d bytes\n", ntohs(udp_proxy_to_proxy_sock.sin_port), rc);
					if (sendto(linphone_proxy->proxy_to_linphone_socket, buff, rc, 0, (struct sockaddr *) &udp_proxy_to_linphone_sock, sizeof(udp_proxy_to_linphone_sock)) != rc) {
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

				if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &udp_proxy_to_linphone_sock, &udp_proxy_to_linphone_len)) < 0) {
					perror("CALL: no message received on forwarding port\n");
				}
				else {
					printf("CALL: from [LINPHONE: %d] - %d bytes\n", htons(udp_proxy_to_linphone_sock.sin_port), rc);
					//getchar();
					if (sendto(linphone_proxy->proxy_to_proxy_socket, buff, rc, 0, (struct sockaddr *) &udp_proxy_to_proxy_sock, sizeof(udp_proxy_to_proxy_sock)) != rc) {
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

				if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &udp_proxy_to_proxy_data_sock, &udp_proxy_to_proxy_data_len)) < 0) {
					printf ("DATA: no message received on listening data port\n");
				}
				else {
					fds[i].revents = 0;
					if (proxy_state != DATA_COPY) {/* TODO: for now */
						printf ("DATA: from [REMOTE: %d] - %d bytes\n", ntohs(udp_proxy_to_proxy_data_sock.sin_port), rc);
						if (sendto(linphone_proxy->proxy_to_linphone_data_socket, buff, rc, 0, (struct sockaddr *) &udp_proxy_to_linphone_data_sock, sizeof(udp_proxy_to_linphone_data_sock)) != rc) {
							perror("DATA: Cannot forward data packet to linphone application\n");
						}
						else {
							printf("DATA: sent [LINPHONE] - %d bytes\n", rc);
						}

						if (proxy_state == MIRRORING) {

							if (sendto(linphone_proxy->proxy_to_proxy_data_socket, buff, rc, 0, (struct sockaddr *) &mirror_sock, sizeof(mirror_sock)) != rc) {
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
						printf ("MIRROED_DATA: from [REMOTE: %d] - %d bytes\n", ntohs(udp_proxy_to_proxy_data_sock.sin_port), rc);
						memset (buff, 0, MAX_PACKET_SIZE);
					}
				}
			}
#if 0
			else if(i == 4) {	/* data packet to another caller */
				if (proxy_state == NONE)
					printf ("ERR: INTERCEPTING is not possible without INITIATION\n");
				else {
					if (proxy_state < INTERCEPTING)
						proxy_state = INTERCEPTING;
				}

				if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &udp_proxy_to_linphone_data_sock, &udp_proxy_to_linphone_data_len)) < 0) {
					perror("DATA: no message received on forwarding port\n");
				}
				else {
					printf ("TCP DATA: from [REMOTE: %d] - %d bytes\n", ntohs(tcp_proxy_to_app_sock.sin_port), rc);
					if (sendto(linphone_proxy->proxy_to_proxy_data_socket, buff, rc, 0, (struct sockaddr *) &udp_proxy_to_proxy_data_sock, sizeof(udp_proxy_to_proxy_data_sock)) != rc) {
						perror("DATA: Cannot forward linphone data packet\n");
					}
					else {
						printf("DATA: sent [REMOTE] - %d bytes\nDone\n", rc);
						memset (buff, 0, MAX_PACKET_SIZE);
					}
				}
			}
#endif
			else if(i == 5) { /* packet from manager */
				if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &manager_sock, &manager_len)) < 0) {
					perror("CONFIGURE: no message received\n");
				}
				else {
					printf("MGM: from [PROXY MANAGER: %d] - %d bytes\n\tMessage: %s\n", htons(manager_sock.sin_port), rc, buff);
					//if (behavior == SERVER)
						//getchar();
					if (strcmp (buff, "Test") == 0)
					{
						strcpy (buff, "Test ok");
						goto __answer;
#if 0
					} else if (strstr (buff, "IP: ") == buff){
						printf ("Mirroring command received: %s\n", buff);

						if (proxy_state == MIRRORING) {
							strcpy (buff, "ERR: already mirroring");
						} else if (proxy_state != INTERCEPTING) {
							strcpy (buff, "ERR: No packets intercepting at the moment");
						} else {
							proxy_state = MIRRORING;
							mirror_ip = strdup(buff + strlen("IP: "));/* TODO: carefull here */
							printf ("DBG: mirrored ip is %s\n", mirror_ip);
							/* init mirror socket */

							mirror_len = sizeof(mirror_sock);
							memset(&mirror_sock, 0, mirror_len);
							mirror_sock.sin_family = AF_INET;
							mirror_sock.sin_addr.s_addr = inet_addr(mirror_ip);
							mirror_sock.sin_port = htons(linphone_proxy->proxy_to_proxy_data_port);

							sprintf (buff, "mirroring started");
						}
#endif
					}
					if (strcmp(buff, "Stop") == 0) {
						if (tcp_proxy_state != NORMAL) {
							printf("I can't STOP if i'm not NORMAL :)\n");
							strcpy(buff, "Nack");
							goto __answer;
						}

						//fds[7].fd = -1;
						if (behavior == SERVER) {
							/* stop reading from VNC server */
							fds[7].revents = 0;
							fds[7].fd = -1;

							/* tell the other proxy to stop */
							strcpy(buff, "Stop");
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, sizeof(configure_sock))) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot send command to [REMOTE PROXY]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY: %s] - %d bytes\n", buff, rc);
							}
							goto __no_answer;
						} else if (behavior == CLIENT) {
							/* stop reading data from application */
							tcp_proxy_state = STOPPED;

							/* send data to VNC client if any */
							if (fds[6].revents != 0) {
								/* got something from REMOTE proxy */
								if ((rc = recvfrom(fds[6].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, &tcp_proxy_to_proxy_len)) <= 0) {
									perror("TCP REMOTE proxy final: no message received\n");
									if (rc == 0) {
										/* remove the socket */
										close(fds[6].fd);
										fds[6].fd = -1;
										printf("Closing socket to REMOTE PROXY...\n");
										//num_fds--;
									}

									/*remove the socket to REMOTE proxy server */
									//close(vnc_proxy->proxy_to_proxy_socket);
									//vnc_proxy->proxy_to_proxy_socket = -1;
								} else {
									/* received something from remote proxy via TCP; send to VNC client */
									printf ("TCP DATA: from [REMOTE PROXY: %d] - %d bytes\n", ntohs(tcp_proxy_to_proxy_sock.sin_port), rc);
									if (sendto(tcp_proxy_to_app_client_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, sizeof(tcp_proxy_to_app_sock)) != rc) {
										perror("TCP DATA: Cannot forward to [VNC client]\n");
									}
									else {
										printf("TCP DATA: sent [VNC client: %s] - %d bytes\n", buff, rc);
									}
									memset (buff, 0, MAX_PACKET_SIZE);
									/* If I was told to stop, no more data will be read */
								}
							}
							fds[6].fd = -1;
							fds[6].revents = 0;
							connected = 0;
							tcp_proxy_state = STOPPED;
							/* stop connecting with the remote proxy */
							close(vnc_proxy->proxy_to_proxy_socket);

							/* do not listen for VNC client anymore */
							fds[8].fd = -1;

							strcpy(buff, "Ack");
							/* let the server know it */
							//manager_sock.sin_addr.s_addr = inet_addr(remote_ip);
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, configure_len)) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot answer to [REMOTE PROXY MANAGER]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY MANAGER: %s] - %d bytes\n", buff, rc);
							}
							memset (buff, 0, MAX_PACKET_SIZE);
							goto __no_answer;
						} else {
							printf("Unknown behavior found\n");
							strcpy(buff, "unknown command");
							goto __answer;
						}
					}

					if (strcmp(buff, "Ack") == 0) {
						/* only server may receive ACKs */
						if (behavior != SERVER) {
							strcpy(buff, "unknown command");
							goto __answer;
						}

						/* Ack may only be received after a Stop command of after a redirect command */

						/* time for server to stop */
						if (tcp_proxy_state == STOPPED) {
							/* command was after IP redirect */
							/* nothing for server to do; send proxy manager ACK */

							/* remake the manager socket */
							manager_sock.sin_family = AF_INET;
							manager_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
							manager_sock.sin_port = htons(MANAGER_PORT);
							goto __answer;
						} else {
							/* command was after STOP */
							tcp_proxy_state = STOPPED;

							/* send something received from REMOTE PROXY if pending */
							if (fds[8].revents != 0) {
								if ((rc = recvfrom(fds[8].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, &tcp_proxy_to_proxy_len)) <= 0) {
									perror("TCP proxy Server - no message received: ");
									if (rc == 0) {
										/* remove the socket */
										close(fds[8].fd);
										tcp_proxy_to_proxy_client_socket = -1;
										fds[8].fd = -1;
										//num_fds--;

										/*remove the socket to the VNC server */
										//close(vnc_proxy->proxy_to_app_socket);
										//vnc_proxy->proxy_to_app_socket = -1;
										//fds[7].fd = -1;
									}

								} else {
									/* received something from remote via TCP; send to TCP appl */
									printf ("TCP DATA: from [REMOTE: %d] - %d bytes\n", ntohs(tcp_proxy_to_proxy_sock.sin_port), rc);

									if (sendto(vnc_proxy->proxy_to_app_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, sizeof(tcp_proxy_to_app_sock)) != rc) {
										perror("TCP proxy Server: Cannot forward vnc data packet\n");
									} else
										printf("TCP DATA: sent [VNC server] - %d bytes\n", rc);

									memset(buff, 0, MAX_PACKET_SIZE);
								}
							}

							fds[8].revents = 0;

							/* If I was told to stop, no more data will be read from VNC server */
							fds[7].fd = -1;

							/* add iptables rules to dump RSP */
							sprintf(buff, "iptables -A OUTPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", VNC_TCP_PORT, START_PORT_NO_TCP_FORWARDING);
							rc = system(buff);
							if (rc == -1)
								perror("Failed to add filer nr 1\n");

							sprintf(buff, "iptables -A INPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", START_PORT_NO_TCP_FORWARDING, VNC_TCP_PORT);
							rc = system(buff);
							if (rc == -1)
								perror("Failed to add filer nr 2\n");
							memset(buff, 0, MAX_PACKET_SIZE);

							has_drop = 1;

							/* send the TCP repair structure */
							printf("Starting to dump TCP repair\n");
							struct inet_tcp_sk_desc app_sock_descr;
							app_sock_descr.rfd = vnc_proxy->proxy_to_app_socket;
							app_sock_descr.family = AF_INET;
							app_sock_descr.type = socket_type_get(vnc_proxy->proxy_to_app_socket);
							app_sock_descr.state = tcp_state_get(vnc_proxy->proxy_to_app_socket);
							if (dump_one_tcp(vnc_proxy->proxy_to_app_socket, &app_sock_descr)) {
								printf("ERROR: failed to dump TCP repair state\n");
							} else {
								close(vnc_proxy->proxy_to_app_socket);
							}

							printf("TCP repaired\n");

							/* let the proxy manager know it */
							strcpy(buff, "Ack");

							/* remake the manager socket */
							manager_sock.sin_family = AF_INET;
							manager_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
							manager_sock.sin_port = htons(MANAGER_PORT);

							goto __answer;
						}

					} else if (strcmp(buff, "TCP repair") == 0) {
						if (behavior == CLIENT) {
							/* unknown command */
							strcpy(buff, "unknown command");
							goto __answer;
						} else if (behavior == SERVER) {
							/* need to repair the TCP socket connected to the server */
							struct inet_tcp_sk_desc app_sock_descr;
							close(vnc_proxy->proxy_to_app_socket);

							if (!has_drop) {
								/* add iptables rules to dump RSP */
								sprintf(buff, "iptables -A OUTPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", VNC_TCP_PORT, START_PORT_NO_TCP_FORWARDING);
								rc = system(buff);
								if (rc == -1)
									perror("Failed to add filer nr 1\n");

								sprintf(buff, "iptables -A INPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", START_PORT_NO_TCP_FORWARDING, VNC_TCP_PORT);
								rc = system(buff);
								if (rc == -1)
									perror("Failed to add filer nr 2\n");
								memset(buff, 0, MAX_PACKET_SIZE);
								has_drop = 1;
							}

							memset(&tcp_proxy_to_app_sock, 0, tcp_proxy_to_app_len);
							tcp_proxy_to_app_sock.sin_family = AF_INET;
							tcp_proxy_to_app_sock.sin_addr.s_addr = inet_addr("127.0.0.1");
							tcp_proxy_to_app_sock.sin_port = htons(VNC_TCP_PORT);

							if ((vnc_proxy->proxy_to_app_socket = create_socket(AF_INET, SOCK_STREAM)) < 0)
								perror("cannot create TCP repair proxy-to-app socket: ");

							app_sock_descr.rfd = vnc_proxy->proxy_to_app_socket;
							app_sock_descr.family = AF_INET;
							app_sock_descr.type = socket_type_get(vnc_proxy->proxy_to_app_socket);
							app_sock_descr.state = tcp_state_get(vnc_proxy->proxy_to_app_socket);
							app_sock_descr.src_port = vnc_proxy->proxy_to_app_port;
							app_sock_descr.src_addr[0] = (INADDR_ANY & 0xFF000000) >> 24;
							app_sock_descr.src_addr[1] = (INADDR_ANY & 0x00FF0000) >> 16;
							app_sock_descr.src_addr[2] = (INADDR_ANY & 0x0000FF00) >> 8;
							app_sock_descr.src_addr[3] = (INADDR_ANY & 0x000000FF);

							if (restore_one_tcp(vnc_proxy->proxy_to_app_socket, &app_sock_descr, &tcp_proxy_to_app_sock, sizeof(tcp_proxy_to_app_sock))) {
								printf("ERROR: failed to dump TCP repair state\n");
							}

							//close(vnc_proxy->proxy_to_app_socket);
							connected = 1;

							fds[7].fd = vnc_proxy->proxy_to_app_socket;
							fds[7].events = POLLIN;
							fds[7].revents = 0;

							printf("Restored TCP connection\n");
							/* send an ack */
							strcpy(buff, "Ack");
							goto __answer;
						} else {
							printf("Unknown behavior found\n");
							strcpy(buff, "unknown command");
							goto __answer;
						}
					} else if (strstr(buff, "IP: ") == buff) {
						/* command to redirect */
						if (behavior == SERVER) {
							/* send to CLIENT; nothing to do for the SERVER */
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, sizeof(configure_sock))) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot send command to [REMOTE PROXY]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY: %s] - %d bytes\n", buff, rc);
							}
							goto __no_answer;
						}

						if (behavior == CLIENT) {
							/* time to redirect */

							if (tcp_proxy_state != STOPPED) {
								printf("I am not stopped, and you want me to redirect. Are you sure??\n");
							}

							/* close the initial socket */
							//close(vnc_proxy->proxy_to_proxy_socket);
							//fds[7].revents = 0;
							//fds[7].fd = -1;
							/* create a new socket to the new ip */
							if ((vnc_proxy->proxy_to_proxy_socket = create_socket(AF_INET, SOCK_STREAM)) < 0)
								perror("cannot create new TCP proxy-to-proxy socket: ");

							j = 1;
							if (setsockopt(vnc_proxy->proxy_to_proxy_socket, SOL_SOCKET, SO_REUSEADDR, &j, sizeof(j)) == -1)
								perror("Failed to reause addr\n");

							rc = bind_socket(
									vnc_proxy->proxy_to_proxy_socket,
									AF_INET,
									START_PORT_NO_TCP_INCOMMING);
							if (rc < 0)
								perror("cannot bind new TCP proxy-to-proxy socket: ");

							vnc_proxy->proxy_to_proxy_port = START_PORT_NO_TCP_INCOMMING;

							/* init new TCP caller socket */
							printf("Creating new socket for proxy to proxy communication, with IP: %s\n", buff + strlen("IP: "));
							tcp_proxy_to_proxy_len = sizeof(tcp_proxy_to_proxy_sock);
							memset(&tcp_proxy_to_proxy_sock, 0, tcp_proxy_to_proxy_len);
							tcp_proxy_to_proxy_sock.sin_family = AF_INET;
							tcp_proxy_to_proxy_sock.sin_addr.s_addr = inet_addr(buff + strlen("IP: "));
							tcp_proxy_to_proxy_sock.sin_port = htons(vnc_proxy->proxy_to_proxy_port);

							/* connect to the new ip */
							for (j = 1; j <= 5; j++) {
								rc = connect(vnc_proxy->proxy_to_proxy_socket,  (struct sockaddr *) &tcp_proxy_to_proxy_sock, tcp_proxy_to_proxy_len);
								if (!rc) {
									printf("Connected to new Remote TCP proxy %s...\n", buff + strlen("IP: "));
									connected = 1;
									fds[6].fd = vnc_proxy->proxy_to_proxy_socket;
									fds[6].events = POLLIN;
									fds[6].revents = 0;
									break;
								}
								printf("Could not connect to new %s Remote TCP proxy Retrying %d ...\n", buff + strlen("IP: "), j);
								sleep(3);
							}

							/* restart listening from the VNC client */
							fds[8].fd = tcp_proxy_to_app_client_socket;
							fds[8].events = POLLIN;
							fds[8].revents = 0;

							tcp_proxy_state = NORMAL;
							strcpy(buff, "Ack");
							/* let the server know it */
							//manager_sock.sin_addr.s_addr = inet_addr(remote_ip);
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, configure_len)) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot answer to [REMOTE PROXY MANAGER]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY MANAGER: %s] - %d bytes\n", buff, rc);
							}

							goto __no_answer;
						} else {
							printf("Unknown behavior found\n");
							strcpy(buff, "unknown command");
							/* let the server know it */
							//manager_sock.sin_addr.s_addr = inet_addr(remote_ip);
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, configure_len)) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot answer to [REMOTE PROXY MANAGER]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY MANAGER: %s] - %d bytes\n", buff, rc);
							}
							memset (buff, 0, MAX_PACKET_SIZE);
							goto __no_answer;
						}

					} else {
						strcpy (buff, "unknown command");
					}
__answer:
					if (sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &manager_sock, sizeof(manager_sock)) != (ssize_t)strlen(buff)) {
						perror("CONFIGURE: Cannot answer to [PROXY MANAGER]\n");
					}
					else {
						printf("MGM: sent [PROXY MANAGER: %s] - %d bytes\n", buff, rc);
					}
__no_answer:
					memset (buff, 0, MAX_PACKET_SIZE);
				}
				/* listening TCP proxy-to-proxy socket */
			} else if (i == 6) {
				if (behavior == SERVER) {
					if (tcp_proxy_to_proxy_client_socket != -1) {
						perror("Another application tries to connect to TCP server\n");
					} else {
						tcp_proxy_to_proxy_client_socket = accept(vnc_proxy->proxy_to_proxy_socket, (struct sockaddr *) &tcp_proxy_to_proxy_sock, &tcp_proxy_to_proxy_len);
						if (tcp_proxy_to_proxy_client_socket == -1) {
							perror("TCP: Accept returned an error\n");
						}
						/* add socket to fds */
						//num_fds++;
						fds[8].fd = tcp_proxy_to_proxy_client_socket;
						fds[8].events = POLLIN;
						fds[8].revents = 0;
						printf("A remote proxy has connected to us via TCP\n");

						if (has_drop == 1) {
							/* remove iptables rules for RSP */
							sprintf(buff, "iptables -D OUTPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", VNC_TCP_PORT, START_PORT_NO_TCP_FORWARDING);
							rc = system(buff);
							if (rc == -1)
								perror("Failed to remove filer nr 1\n");

							sprintf(buff, "iptables -D INPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", START_PORT_NO_TCP_FORWARDING, VNC_TCP_PORT);
							rc = system(buff);
							if (rc == -1)
								perror("Failed to remove filer nr 2\n");
							has_drop = 0;
							memset(buff, 0, MAX_PACKET_SIZE);
						}
						if (!connected) {
							/* time to connect to the VNC server */
							printf("Trying to connect to the VNC server\n");
							for (j = 1; j <= 5; j++) {
								rc = connect(vnc_proxy->proxy_to_app_socket, (struct sockaddr *) &tcp_proxy_to_app_sock, tcp_proxy_to_app_len);
								if (!rc) {
									printf("Connected to VNC server...\n");
									fds[7].fd = vnc_proxy->proxy_to_app_socket;
									connected = 1;
									break;
								}
								printf("Could not connect to VNC server Retrying %d ...\n", j);
								sleep(3);
							}
						}
					}
				} else if (behavior == CLIENT) {
					/* received something from TCP remote SERVER proxy; send to app as from server */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, &tcp_proxy_to_proxy_len)) <= 0) {
						perror("TCP proxy: no message received from REMOTE proxy\n");
						if (rc == 0) {
							close(fds[i].fd);
							fds[i].fd = -1;
							connected = 0;
						}
					} else {
						printf ("TCP DATA: from [REMOTE PROXY: %d] - %d bytes\n", ntohs(tcp_proxy_to_proxy_sock.sin_port), rc);
						if (sendto(tcp_proxy_to_app_client_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, sizeof(tcp_proxy_to_app_sock)) != rc) {
							perror("TCP DATA: Cannot forward to [VNC client]\n");
						}
						else {
							printf("TCP DATA: sent [VNC client: %s] - %d bytes\n", buff, rc);
						}
						memset (buff, 0, MAX_PACKET_SIZE);
					}
				}
			} else if (i == 7) {
				/* received something from VNC server/client */

				if (behavior == SERVER) {
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, &tcp_proxy_to_app_len)) <= 0) {
						perror("TCP proxy: no message received from VNC\n");
						if (rc == 0) {
							close(fds[i].fd);
							fds[i].fd = -1;
							connected = 0;
						}
					} else {
						printf ("TCP DATA: from [VNC server: %d] - %d bytes\n", ntohs(tcp_proxy_to_app_sock.sin_port), rc);
						if (tcp_proxy_to_proxy_client_socket != -1) {
							if (sendto(tcp_proxy_to_proxy_client_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, sizeof(tcp_proxy_to_proxy_len)) != rc) {
								perror("TCP DATA: Cannot forward to [REMOTE proxy]\n");
							}
							else {
								printf("TCP DATA: sent [REMOTE: %s] - %d bytes\n", buff, rc);
							}
						}
						memset (buff, 0, MAX_PACKET_SIZE);
					}

				} else if (behavior == CLIENT) {
					/* VNC client tries to connect to our TCP proxy */
					if (tcp_proxy_to_app_client_socket != -1) {
						perror("Another localhost VNC client tries to connect to TCP proxy server\n");
					} else {
						tcp_proxy_to_app_client_socket = accept(vnc_proxy->proxy_to_app_socket, (struct sockaddr *) &tcp_proxy_to_app_sock, &tcp_proxy_to_app_len);
						if (tcp_proxy_to_app_client_socket == -1) {
							perror("TCP: Accept returned an error\n");
						}
						/* add socket to fds */
						//num_fds++;
						fds[8].fd = tcp_proxy_to_app_client_socket;
						fds[8].events = POLLIN;
						fds[8].revents = 0;
						printf("VNC client connected\n");
						if (!connected) {
							/* act as a CLIENT and CONNECT to TCP remove SERVER */

							for (j = 1; j <= 5; j++) {
								rc = connect(vnc_proxy->proxy_to_proxy_socket,  (struct sockaddr *) &tcp_proxy_to_proxy_sock, tcp_proxy_to_proxy_len);
								if (!rc) {
									printf("Connected to Remote TCP proxy...\n");
									connected = 1;
									fds[6].fd = vnc_proxy->proxy_to_proxy_socket;
									break;
								}
								printf("Could not connect to Remote TCP proxy Retrying %d ...\n", j);
								sleep(3);
							}
						}
					}
				}
			} else if (i == 8) {
				if (behavior == SERVER) {
					/* get something from remote TCP client proxy */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, &tcp_proxy_to_proxy_len)) <= 0) {
						perror("TCP proxy Server - no message received: ");
						if (rc == 0) {
							/* remove the socket */
							close(fds[i].fd);
							tcp_proxy_to_proxy_client_socket = -1;
							fds[i].fd = -1;
							//num_fds--;

							/*remove the socket to the VNC server */
							//close(vnc_proxy->proxy_to_app_socket);
							//vnc_proxy->proxy_to_app_socket = -1;
							//fds[7].fd = -1;
							connected = 0;
						}

					} else {
						/* received something from remote via TCP; send to TCP appl */
						printf ("TCP DATA: from [REMOTE: %d] - %d bytes\n", ntohs(tcp_proxy_to_proxy_sock.sin_port), rc);

						if (sendto(vnc_proxy->proxy_to_app_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, sizeof(tcp_proxy_to_app_sock)) != rc) {
							perror("TCP proxy Server: Cannot forward vnc data packet\n");
						} else
							printf("TCP DATA: sent [VNC server] - %d bytes\n", rc);

						memset(buff, 0, MAX_PACKET_SIZE);
					}
				} else if (behavior == CLIENT) {
					/* got something from localhost VNC client */
					if ((rc = recvfrom(fds[i].fd, buff, MAX_PACKET_SIZE, 0, (struct sockaddr *) &tcp_proxy_to_app_sock, &tcp_proxy_to_app_len)) <= 0) {
						perror("TCP VNC client: no message received\n");

						/* remove the socket */
						close(fds[i].fd);
						tcp_proxy_to_app_client_socket = -1;
						fds[i].fd = -1;
						//num_fds--;

						/*remove the socket to REMOTE proxy server */
						//close(vnc_proxy->proxy_to_proxy_socket);
						//vnc_proxy->proxy_to_proxy_socket = -1;
						//fds[6].fd = -1;
						//connected = 0;
					} else {
						/* received something from remote via TCP; send to TCP appl */
						printf ("TCP DATA: from [VNC client: %d] - %d bytes\n", ntohs(tcp_proxy_to_app_sock.sin_port), rc);

						if (sendto(vnc_proxy->proxy_to_proxy_socket, buff, rc, 0, (struct sockaddr *) &tcp_proxy_to_proxy_sock, sizeof(tcp_proxy_to_proxy_sock)) != rc) {
							perror("TCP proxy Server: Cannot forward vnc data packet to REMOTE PROXY\n");
						} else
							printf("TCP DATA: sent [REMOTE PROXY] - %d bytes\n", rc);

						memset(buff, 0, MAX_PACKET_SIZE);
#if 0
						/* If I was told to stop, no more data will be read */
						if (tcp_proxy_state == STOPPED) {
							fds[8].fd = -1;
							strcpy(buff, "Ack");
							/* let the server know it */
							//manager_sock.sin_addr.s_addr = inet_addr(remote_ip);
							if ((rc = sendto(configure_socket, buff, strlen(buff), 0, (struct sockaddr *) &configure_sock, configure_len)) != (ssize_t)strlen(buff)) {
								perror("CONFIGURE: Cannot answer to [REMOTE PROXY MANAGER]\n");
							}
							else {
								printf("MGM: sent [REMOTE PROXY MANAGER: %s] - %d bytes\n", buff, rc);
							}
							memset (buff, 0, MAX_PACKET_SIZE);
						}
#endif
					}
				}
			}
			printf("\n");
			fds[i].revents = 0;
			//getchar();
		}
	}

	out:
	if (has_drop == 1) {
		/* remove iptables rules for RSP */
		sprintf(buff, "iptables -D OUTPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", VNC_TCP_PORT, START_PORT_NO_TCP_FORWARDING);
		rc = system(buff);
		if (rc == -1)
			perror("Failed to remove filer nr 1\n");

		sprintf(buff, "iptables -D INPUT -p tcp --dport %d --sport %d -s 127.0.0.1 -d 127.0.0.1 -j DROP", START_PORT_NO_TCP_FORWARDING, VNC_TCP_PORT);
		rc = system(buff);
		if (rc == -1)
			perror("Failed to remove filer nr 2\n");
		has_drop = 0;
		memset(buff, 0, MAX_PACKET_SIZE);
	}
	if (tcp_proxy_to_app_client_socket != -1)
		close(tcp_proxy_to_app_client_socket);
	if (tcp_proxy_to_proxy_client_socket != -1)
		close(tcp_proxy_to_proxy_client_socket);
	printf("Exiting...\n");
	free(buff);
	free(fds);

	return 0;

}
