/*
 * init.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */
#include "includes/init.h"

int proxy_to_proxy_port, proxy_to_linphone_port, proxy_to_proxy_data_port, proxy_to_linphone_data_port, configure_port;

int
init(int *proxy_to_proxy_socket,
		int *proxy_to_linphone_socket,
		int *proxy_to_proxy_data_socket,
		int *proxy_to_linphone_data_socket,
		int *configure_socket,
		char *remote_ip)
{
	int port, i, rc;

	/* create socket for proxy-to-proxy communication */
	*proxy_to_proxy_socket = -1;
	port = START_PORT_NO_INCOMING;
	if ((*proxy_to_proxy_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		exit_error("cannot create proxy-to-proxy socket", EXIT_FAILURE);
	rc = -1;
	rc = bind_socket(
			*proxy_to_proxy_socket,
			AF_INET,
			port);
	if (rc < 0)
		exit_error("cannot bind proxy-to-proxy socket", EXIT_FAILURE);
	proxy_to_proxy_port = port;


	/* creating socket for proxy-to-linphone communication */
	*proxy_to_linphone_socket = -1;
	port = START_PORT_NO_FORWARDING;
	if ((*proxy_to_linphone_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		exit_error("cannot create proxy-to-linphone socket", EXIT_FAILURE);

	rc = -1;
	for (i=0; i<10 && rc < 0; i++) {
		rc = bind_socket(
			*proxy_to_linphone_socket,
			AF_INET,
			port + i);
	}
	if (rc < 0)
		exit_error("cannot bind proxy-to-linphone socket", EXIT_FAILURE);

	proxy_to_linphone_port = port + i - 1;
	/* create socket for proxy-to-proxy data communication */
	*proxy_to_proxy_data_socket = -1;
	port = START_PORT_NO_DATA_INCOMING;
	if ((*proxy_to_proxy_data_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		exit_error("cannot create proxy-to-proxy data socket", EXIT_FAILURE);

	rc = -1;
	rc = bind_socket(
			*proxy_to_proxy_data_socket,
			AF_INET,
			port);
	if (rc < 0)
		exit_error("cannot bind proxy-to-proxy data socket", EXIT_FAILURE);
	proxy_to_proxy_data_port = port;

	/* creating socket for proxy-to-linphone data communication */
	*proxy_to_linphone_data_socket = -1;
	port = START_PORT_NO_DATA_FORWARDING;

	if ((*proxy_to_linphone_data_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		exit_error("cannot create proxy-to-linphone data socket", EXIT_FAILURE);

	rc = -1;
	for (i=0; i<10 && rc < 0; i++) {
		rc = bind_socket(
				*proxy_to_linphone_data_socket,
				AF_INET,
				port + i);
	}
	if (rc < 0)
		exit_error("cannot bind proxy-to-linphone data socket", EXIT_FAILURE);
	proxy_to_linphone_data_port = port + i - 1;

	*configure_socket = create_socket(AF_INET, SOCK_DGRAM);
	configure_port = CONFIGURE_PORT;
	if (*configure_socket < 0)
		exit_error("cannot create configure socket", EXIT_FAILURE);
	rc = bind_socket(
			*configure_socket,
			AF_INET,
			configure_port);
	if (rc < 0)
		exit_error("cannot bind configure socket", EXIT_FAILURE);

	printf ("socket created for proxy-to-proxy setup traffic on port %d\n", proxy_to_proxy_port);
	printf ("socket created for proxy-to-linphone setup traffic on port %d\n", proxy_to_linphone_port);
	printf ("socket created for proxy-to-proxy data traffic on port %d\n", proxy_to_proxy_data_port);
	printf ("socket created for proxy-to-linphone data traffic on port %d\n", proxy_to_linphone_data_port);
	printf ("socket created for configurations on port %d\n", configure_port);

	return 0;
}

void
release(int proxy_to_proxy_socket, int proxy_to_linphone_socket, int proxy_to_proxy_data_socket, int proxy_to_linphone_data_socket, int configure_socket, char *remote_ip) {

	if (proxy_to_proxy_socket < 0) {
		destroy_socket(proxy_to_proxy_socket);
		proxy_to_proxy_socket = -1;
	}

	if (proxy_to_linphone_socket < 0) {
		destroy_socket(proxy_to_linphone_socket);
		proxy_to_linphone_socket = -1;
	}

	if (proxy_to_proxy_data_socket < 0) {
		destroy_socket(proxy_to_proxy_data_socket);
		proxy_to_proxy_data_socket = -1;
	}

	if (proxy_to_linphone_data_socket < 0) {
		destroy_socket(proxy_to_linphone_data_socket);
		proxy_to_linphone_data_socket = -1;
	}

	if (configure_socket < 0) {
			destroy_socket(configure_socket);
			configure_socket = -1;
		}
}
