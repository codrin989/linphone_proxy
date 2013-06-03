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
		int *proxy_to_proxy_socket,
		int *proxy_to_linphone_socket,
		int *proxy_to_proxy_data_socket,
		int *proxy_to_linphone_data_socket,
		int *configure_socket
		);

void
release(int proxy_to_proxy_socket,
		int proxy_to_linphone_socket,
		int proxy_to_proxy_data_socket,
		int proxy_to_linphone_data_socket,
		int configure_socket
		);

#endif /* INIT_H_ */
