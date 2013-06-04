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

#define START_PORT_NO_INCOMING 			50606
#define START_PORT_NO_FORWARDING 		50616
#define START_PORT_NO_DATA_INCOMING		51786
#define START_PORT_NO_DATA_FORWARDING	51796
#define SIP_UDP_PORT 					5060	/* TODO set dynamic */
#define SIP_DATA_PORT 					7078	/* TODO set dynamic */
#define CONFIGURE_PORT					52124
#define MANAGER_PORT					52123
#define MAX_LEN 255
#define MAX_PACKET_SIZE 1522

#ifndef __ANDROID_API__
#define CALLER_IP "192.168.1.148"	/* TODO set dynamic */
#else
#define CALLER_IP "192.168.1.135"	/* TODO set dynamic */
#endif

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
