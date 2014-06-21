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

#define LISTENING_SOCKETS 8	/* TODO - for now */

int
run_proxy(
		struct udp_session *linphone_proxy,
		struct tcp_session *vnc_proxy,
		int configure_socketfd,
		char *remote_ip);

#endif /* RUN_PROXY_H_ */
