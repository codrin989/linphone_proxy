/*
 * init.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef INIT_H_
#define INIT_H_

#include "utils.h"
#include "socket.h"


int
init(
		struct udp_session *linphone_proxy,
		struct tcp_session *vnc_proxy,
		int *configure_socket,
		char *remote_ip
		);

void
release(
		struct udp_session *linphone_proxy,
		struct tcp_session *vnc_proxy,
		int configure_socket,
		char *remote_ip
		);

#endif /* INIT_H_ */
