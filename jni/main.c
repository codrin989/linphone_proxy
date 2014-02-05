/*
 ============================================================================
 Name        : linphone_proxy.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <pthread.h>
#include <unistd.h>

#include "includes/socket.h"
#include "includes/utils.h"
#include "includes/parse.h"

#define NUM_FDS 6

enum {
	NORMAL,
	REDIRECTING,
	ON_HOLD
};

int main(int argc, char *argv[])
{
	char buffer[MAX_PACKET_SIZE];
	char spoofed_invite[MAX_PACKET_SIZE];
	char spoofed_ack[MAX_PACKET_SIZE];
	int spoofed_invite_len;
	int spoofed_ack_len;

	/* sockets used for communication */
	int proxy_to_proxy_socket;
	int proxy_to_linphone_socket;
	int proxy_to_proxy_data_socket;
	int proxy_to_linphone_data_socket;
	int manager_socket;
	struct pollfd fds[NUM_FDS];

	struct sockaddr_in proxy_to_linphone_addr;
	struct sockaddr_in proxy_to_proxy_addr;
	struct sockaddr_in proxy_to_proxy_data_addr;
	struct sockaddr_in proxy_to_linphone_data_addr;
	struct sockaddr_in redirect_data_addr;
	struct sockaddr_in manager_addr;
	struct sockaddr_in redirect_manager_addr;

	char *remote_ip = NULL;
	char *peer_ip = NULL;
	char call_id[10];
	char local_tag[11];
	char remote_tag[11];
	int state = NORMAL;

	int rc, i, count;
	socklen_t addrlen;
	struct sockaddr_in addr;

	KICK(argc < 2, "incorrect usage\n"
	"Usage:\n"
	"./linphone_proxy <remote_ip>\n");
	remote_ip = argv[1];

	add_poll(&fds[0], STDIN_FILENO);

	proxy_to_linphone_socket = create_socket(SOCK_DGRAM, SIP_LINPHONE);
	init_sockaddr(&proxy_to_linphone_addr, "127.0.0.1", SIP_PORT);
	add_poll(&fds[1], proxy_to_linphone_socket);

	proxy_to_proxy_socket = create_socket(SOCK_DGRAM, SIP_PROXY);
	init_sockaddr(&proxy_to_proxy_addr, remote_ip, SIP_PROXY);
	add_poll(&fds[2], proxy_to_proxy_socket);

	proxy_to_linphone_data_socket = create_socket(SOCK_DGRAM, DATA_LINPHONE);
	init_sockaddr(&proxy_to_linphone_data_addr, "127.0.0.1", DATA_PORT);
	add_poll(&fds[3], proxy_to_linphone_data_socket);

	proxy_to_proxy_data_socket = create_socket(SOCK_DGRAM, DATA_PROXY);
	init_sockaddr(&proxy_to_proxy_data_addr, remote_ip, DATA_PROXY);
	add_poll(&fds[4], proxy_to_proxy_data_socket);

	manager_socket = create_socket(SOCK_DGRAM, MANAGER_PORT);
	init_sockaddr(&manager_addr, "0.0.0.0", MANAGER_PORT);
	add_poll(&fds[5], manager_socket);

	addrlen = sizeof(struct sockaddr_in);
	memset(buffer, 0, MAX_PACKET_SIZE);
	while (1) {
		rc = poll(fds, NUM_FDS, -1);
		DIE(-1 == rc, "poll");

		for (i = 0; i < NUM_FDS; i++) {
			if (0 == fds[i].revents) {
				continue;
			}

			switch(i) {
			/* receive line from console */
			case 0:
				break;

			/* receive SIP packet from linphone */
			case 1:
				rc = recvfrom(
						fds[i].fd, buffer, MAX_PACKET_SIZE, 0,
						(struct sockaddr *)&proxy_to_linphone_addr,
						&addrlen);
				if (-1 == rc) {
					ERR("SIP: no message received from linphone");
					break;
				}
				count = rc;
				printf("SIP: from [LINPHONE: %d] - %d bytes\n",
						htons(proxy_to_linphone_addr.sin_port), count);

				rc = parse(buffer, count);
				if (INVITE_OK == rc) {
					get_call_id(buffer, call_id);
					get_second_tag(buffer, local_tag);
					get_first_tag(buffer, remote_tag);
				}

				rc = sendto(
						proxy_to_proxy_socket,
						buffer, count, 0,
						(struct sockaddr *)&proxy_to_proxy_addr,
						addrlen);
				if (rc != count) {
					ERR("SIP: cannot forward packet to proxy");
					break;
				}
				printf("SIP: sent [REMOTE] - %d bytes\n", count);
				break;

			/* receive SIP packet from proxy */
			case 2:
				rc = recvfrom(
						fds[i].fd, buffer, MAX_PACKET_SIZE, 0,
						(struct sockaddr *)&proxy_to_proxy_addr,
						&addrlen);
				if (-1 == rc) {
					ERR("SIP: no message received from proxy");
					break;
				}
				count = rc;
				printf("SIP: from [REMOTE: %d] - %d bytes\n",
						htons(proxy_to_proxy_addr.sin_port), count);

				rc = parse(buffer, count);
				if (INVITE_OK == rc) {
					get_call_id(buffer, call_id);
					get_first_tag(buffer, local_tag);
					get_second_tag(buffer, remote_tag);
				}

				rc = sendto(
						proxy_to_linphone_socket,
						buffer, count, 0,
						(struct sockaddr *)&proxy_to_linphone_addr,
						addrlen);
				if (rc != count) {
					ERR("SIP: cannot forward message to linphone");
					break;
				}
				printf("CALL: sent [LINPHONE] - %d bytes\n", count);
				break;

			/* receive data packet from linphone */
			case 3:
				rc = recvfrom(
						fds[i].fd, buffer, MAX_PACKET_SIZE, 0,
						(struct sockaddr *)&proxy_to_linphone_data_addr,
						&addrlen);
				if (-1 == rc) {
					ERR("DATA: no data received from linphone");
					break;
				}
				count = rc;
				printf("DATA: from [LINPHONE: %d] - %d bytes\n",
						htons(proxy_to_linphone_data_addr.sin_port), count);

				if (ON_HOLD == state) {
					break;
				}
				addr = (NORMAL == state) ?
						proxy_to_proxy_data_addr : redirect_data_addr;
				rc = sendto(
						proxy_to_proxy_data_socket,
						buffer, count, 0,
						(struct sockaddr *)&addr,
						addrlen);
				if (rc != count) {
					ERR("DATA: cannot forward data to proxy");
					break;
				}
				printf("DATA: sent [REMOTE] - %d bytes\n", count);
				break;

			/* receive data packet from proxy */
			case 4:
				addr = (NORMAL == state) ?
						proxy_to_proxy_data_addr : redirect_data_addr;
				rc = recvfrom(
						fds[i].fd, buffer, MAX_PACKET_SIZE, 0,
						(struct sockaddr *)&addr,
						&addrlen);
				if (-1 == rc) {
					ERR("DATA: no data received from proxy");
					break;
				}
				count = rc;
				printf("DATA: from [REMOTE: %d] - %d bytes\n",
						htons(proxy_to_proxy_data_addr.sin_port), count);

				rc = sendto(
						proxy_to_linphone_data_socket,
						buffer, count, 0,
						(struct sockaddr *)&proxy_to_linphone_addr,
						addrlen);
				if (rc != count) {
					ERR("DATA: cannot forward data to LINPHONE");
					break;
				}
				printf("DATA: sent [LINPHONE] - %d bytes\n", count);
				break;

			/* receive command from manager */
			case 5:
				rc = recvfrom(
						fds[i].fd, buffer, MAX_PACKET_SIZE, 0,
						(struct sockaddr *)&manager_addr,
						&addrlen);
				if (-1 == rc) {
					ERR("MNGR: no command received");
					break;
				}
				count = rc;

				if (NORMAL == state && string_eq(buffer, "IP: ")) {
					printf("MNGR: received: %s\n", buffer);
					strcpy(remote_ip, buffer + 4);
					init_sockaddr(&redirect_manager_addr,
							peer_ip, MANAGER_PORT);

					count = get_spoofed_invite(buffer, peer_ip, remote_ip,
							call_id, local_tag, remote_tag);
					rc = sendto(
							manager_socket,
							buffer, count, 0,
							(struct sockaddr *)&redirect_manager_addr,
							addrlen);
					if (-1 == rc) {
						ERR("MNGR: spoofed INVITE not sent");
						break;
					}
					printf("MNGR: sent spoofed INVITE - %d bytes\n", rc);

					count = get_spoofed_invite(buffer, peer_ip, remote_ip,
							call_id, local_tag, remote_tag);
					rc = sendto(
							manager_socket,
							buffer, count, 0,
							(struct sockaddr *)&redirect_manager_addr,
							addrlen);
					if (-1 == rc) {
						ERR("MNGR: spoofed ACK not sent");
						break;
					}
					printf("MNGR: sent spoofed ACK - %d bytes\n", rc);

					rc = recvfrom(
							manager_socket,
							buffer, MAX_PACKET_SIZE, 0,
							(struct sockaddr *)&redirect_manager_addr,
							&addrlen);
					if (-1 == rc) {
						ERR("MNGR: confirmation not received");
					}
					if (!string_eq(buffer, "confirm")) {
						printf("MNGR: confirmation not received\n");
						break;
					}

					init_sockaddr(&redirect_manager_addr,
							remote_ip, MANAGER_PORT);
					strcpy(buffer, "REDIRECT: ");
					strcat(buffer, peer_ip);
					rc = sendto(
							manager_socket,
							buffer, strlen(buffer), 0,
							(struct sockaddr *)&redirect_manager_addr,
							addrlen);
					if (-1 == rc) {
						ERR("MNGR: REDIRECT not sent");
						break;
					}
					printf("MNGR: sent REDIRECT\n");

					rc = recvfrom(
							manager_socket,
							buffer, MAX_PACKET_SIZE, 0,
							(struct sockaddr *)&redirect_manager_addr,
							&addrlen);
					if (-1 == rc) {
						ERR("MNGR: confirmation not received");
					}
					if (!string_eq(buffer, "confirm")) {
						printf("MNGR: confirmation not received\n");
						break;
					}

					state = ON_HOLD;
					printf("chaged state to ON_HOLD ...\n");
				}

				if (NORMAL == state && string_eq(buffer, "INVITE")) {
					printf("MNGR: received spoofed INVITE - %d bytes\n", count);
					memcpy(spoofed_invite, buffer, count);
					spoofed_invite_len = count;

					rc = recvfrom(
							manager_socket,
							buffer, MAX_PACKET_SIZE, 0,
							(struct sockaddr *)&manager_addr,
							&addrlen);
					if (-1 == rc) {
						ERR("MNGR: no command received");
						break;
					}
					printf("MNGR: received spoofed ACK - %d bytes\n", rc);
					memcpy(spoofed_ack, buffer, rc);
					spoofed_ack_len = rc;

					rc = sendto(
							proxy_to_linphone_socket,
							spoofed_invite, spoofed_invite_len, 0,
							(struct sockaddr *)&proxy_to_linphone_addr,
							addrlen);
					if (rc != count) {
						ERR("MNGR: spoofed INVITE not sent to LINPHONE");
						break;
					}
					printf("MNGR: sent spoofed INVITE to LINPHONE - %d bytes\n",
							spoofed_invite_len);

					do {
						rc = recvfrom(
								proxy_to_linphone_socket,
								buffer, MAX_PACKET_SIZE, 0,
								(struct sockaddr *)&proxy_to_linphone_addr,
								&addrlen);
						if (-1 == rc) {
							ERR("MNGR: no message received from linphone");
							break;
						}
						count = rc;
						printf("MNGR: from [LINPHONE: %d] - %d bytes\n",
								htons(proxy_to_linphone_addr.sin_port), count);
					} while (!string_eq(buffer, "SIP/2.0 200 OK"));

					rc = sendto(
							proxy_to_linphone_socket,
							spoofed_ack, spoofed_ack_len, 0,
							(struct sockaddr *)&proxy_to_linphone_addr,
							addrlen);
					if (rc != count) {
						ERR("MNGR: spoofed ACK not sent to LINPHONE");
						break;
					}
					printf("MNGR: sent spoofed ACK to LINPHONE - %d bytes\n",
							spoofed_invite_len);

					strcpy(buffer, "confirm");
					rc = sendto(
							manager_socket,
							buffer, strlen(buffer), 0,
							(struct sockaddr *)&manager_addr,
							addrlen);
					if (-1 == rc) {
						ERR("MNGR: confirmation not sent");
					}
					printf("MNGR: sent confirmation\n");
				}

				if (NORMAL == state && string_eq(buffer, "REDIRECT: ")) {
					printf("MNGR: received REDIRECT\n");
					strcpy(remote_ip, buffer + 4);

					strcpy(buffer, "confirm");
					rc = sendto(
							manager_socket,
							buffer, strlen(buffer), 0,
							(struct sockaddr *)&manager_addr,
							addrlen);
					if (-1 == rc) {
						ERR("MNGR: confirmation not sent");
					}
					printf("MNGR: sent confirmation\n");

					init_sockaddr(&redirect_data_addr, remote_ip, DATA_PROXY);
					state = REDIRECTING;
					printf("chaged state to REDIRECTING ...\n");
				}

				break;

			/* error */
			default:
				break;
			}

			memset(buffer, 0, MAX_PACKET_SIZE);
		}
	}

	return EXIT_SUCCESS;
}
