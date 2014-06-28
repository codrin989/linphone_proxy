/*
 * utils.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


#define true	1
#define false	0

#define START_PORT_NO_INCOMING 			50606
#define START_PORT_NO_FORWARDING 		50616
#define START_PORT_NO_DATA_INCOMING		51786
#define START_PORT_NO_DATA_FORWARDING	51796
#define SIP_UDP_PORT 					5060	/* TODO set dynamic */
#define SIP_DATA_PORT 					7076	/* TODO set dynamic */
#define CONFIGURE_PORT					52124
#define MANAGER_PORT					52123
#define VNC_TCP_PORT					5925
#define START_PORT_NO_TCP_INCOMMING		59999
#define START_PORT_NO_TCP_FORWARDING	59256

#define MAX_LEN 255
#define MAX_PACKET_SIZE 1472

#ifndef __ANDROID_API__
#define CALLER_IP "192.168.1.148"	/* TODO set dynamic */
#else
#define CALLER_IP "192.168.1.135"	/* TODO set dynamic */
#endif

struct udp_session {
	int proxy_to_proxy_socket;
	int proxy_to_proxy_port;
	int proxy_to_linphone_socket;
	int proxy_to_linphone_port;
	int proxy_to_proxy_data_socket;
	int proxy_to_proxy_data_port;
	int proxy_to_linphone_data_socket;
	int proxy_to_linphone_data_port;
};

struct tcp_session {
	int proxy_to_proxy_socket;
	int proxy_to_proxy_port;
	int proxy_to_app_socket;
	int proxy_to_app_port;
};

enum behavior_type {
	CLIENT = 0,
	SERVER
};

inline int
exit_error (const char * msg, int exit_code);

inline int
return_error (const char * msg, int return_code);

#define KICK(assertion, call_description) \
	do { \
		if (assertion) { \
			fprintf(stderr, "(%s, %d): ", \
					__FILE__, __LINE__); \
			fprintf(stderr, "%s\n", call_description); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#endif /* UTILS_H_ */
