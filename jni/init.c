/*
 * init.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */
#include "init.h"

int proxy_to_proxy_port, proxy_to_linphone_port, proxy_to_proxy_data_port, proxy_to_linphone_data_port, configure_port;

int
add_iptables_rule(const char operation, const char * type, const char * intf, int negate_interface, int dest_port, int red_to_port) {
	char buff[MAX_LEN];
	int rc = -1;

	if (strcmp (type, "PREROUTING") == 0) {
		if (negate_interface != 0)
			sprintf(buff, "iptables -t nat -%c %s ! -i %s -p udp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
		else
			sprintf(buff, "iptables -t nat -%c %s -i %s -p udp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
	}
	else if (strcmp (type, "OUTPUT") == 0) {
		sprintf(buff, "iptables -t nat -%c %s -p udp -d %s --sport %d --dport %d -j DNAT --to 127.0.0.1:%d", operation, type, CALLER_IP, dest_port, dest_port, red_to_port);

	}

	rc = system(buff);
	if (rc == -1)
		return_error(buff, rc);

	if (strcmp (type, "OUTPUT") == 0) {
		sprintf(buff, "iptables -t nat -%c POSTROUTING -p udp -d %s --sport %d --dport %d -j SNAT --to-source 192.168.1.148:%d", operation, CALLER_IP, red_to_port, dest_port, dest_port );
	}
	rc = system(buff);
	if (rc == -1)
		return_error(buff, rc);

	return rc;
}

int
init(int *proxy_to_proxy_socket, int *proxy_to_linphone_socket, int *proxy_to_proxy_data_socket, int *proxy_to_linphone_data_socket, int *configure_socket) {
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

#if 0
	/* add preroute rule for incoming packets - remember to delete at the end */
	i = add_iptables_rule('A', "PREROUTING", "lo", 1, SIP_UDP_PORT, proxy_to_proxy_port);
	if (i == -1) {
		proxy_to_proxy_port = -1; /* no need to delete the iptable rule */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif

#if 0
	/* add preroute rule for packets that initate a call - local linphone */
	i = add_iptables_rule('A', "OUTPUT","lo", 1, SIP_UDP_PORT, proxy_to_linphone_port);
	if (i == -1) {
		proxy_to_linphone_port = -1; /* no need to delete the iptable rule; dele the one before */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif

#if 0
	/* add preroute rule for incoming data packets - remember to delete at the end */
	i = add_iptables_rule('A', "PREROUTING", "lo", 1, SIP_DATA_PORT, proxy_to_proxy_data_port);
	if (i == -1) {
		proxy_to_proxy_data_port = -1; /* no need to delete the iptable rule */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif

#if 0
	/* add preroute rule for packets that send data packets - local linphone */
	i = add_iptables_rule('A', "OUTPUT","lo", 1, SIP_DATA_PORT, proxy_to_linphone_data_port);
	if (i == -1) {
		proxy_to_linphone_data_port = -1; /* no need to delete the iptable rule; delete the one before */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif

	return 0;
}

void
release(int proxy_to_proxy_socket, int proxy_to_linphone_socket, int proxy_to_proxy_data_socket, int proxy_to_linphone_data_socket, int configure_socket) {
	int rc;

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

#if 0
	if (proxy_to_proxy_port != -1) {
		rc = add_iptables_rule('D', "PREROUTING", "lo", 1, SIP_UDP_PORT, proxy_to_proxy_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_proxy_port = -1;
	}
#endif

#if 0
	if (proxy_to_linphone_port != -1) {
		rc = add_iptables_rule('D', "OUTPUT","lo", 1, SIP_UDP_PORT, proxy_to_linphone_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_linphone_port = -1;
	}
#endif

#if 0
	if (proxy_to_proxy_data_port != -1) {
		rc = add_iptables_rule('D', "PREROUTING", "lo", 1, SIP_DATA_PORT, proxy_to_proxy_data_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_proxy_data_port = -1;
	}
#endif

#if 0
	if (proxy_to_linphone_data_port != -1) {
		rc = add_iptables_rule('D', "OUTPUT","lo", 1, SIP_DATA_PORT, proxy_to_linphone_data_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_linphone_data_port = -1;
	}
#endif

}
