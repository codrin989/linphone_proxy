/*
 * run_proxy.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef RUN_PROXY_H_
#define RUN_PROXY_H_

#include "socket.h"

#include <poll.h>

#define LISTENING_SOCKETS 6	/* TODO - for now */

int
run_proxy(
		int proxy_to_proxy_socket,
		int proxy_to_linphone_socket,
		int proxy_to_proxy_data_socket,
		int proxy_to_linphone_data_socket,
		int configure_socketfd,
		char *remote_ip);

#endif /* RUN_PROXY_H_ */
