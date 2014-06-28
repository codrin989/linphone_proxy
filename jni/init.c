/*
 * init.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */
#include "init.h"

int  configure_port;
extern enum behavior_type behavior;

static int
add_udp_iptables_rule(
		const char operation,
		const char * type,
		const char * intf,
		int negate_interface,
		char * target_ip,
		int dest_port,
		int red_to_port)
{
	char buff[MAX_LEN];
	int rc = -1;

	if (strcmp (type, "PREROUTING") == 0) {
		if (negate_interface != 0)
			sprintf(buff, "iptables -t nat -%c %s ! -i %s -p udp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
		else
			sprintf(buff, "iptables -t nat -%c %s -i %s -p udp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
	}
	else if (strcmp (type, "OUTPUT") == 0) {
		sprintf(buff, "iptables -t nat -%c %s -p udp -d %s --sport %d --dport %d -j DNAT --to 127.0.0.1:%d", operation, type, target_ip, dest_port, dest_port, red_to_port);

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


static int
add_tcp_iptables_rule(
		enum behavior_type btype,
		const char operation,
		const char * type,
		const char * intf,
		int negate_interface,
		char * target_ip,
		int dest_port,
		int red_to_port)
{
	char buff[MAX_LEN];
	int rc = -1;

	if (strcmp (type, "PREROUTING") == 0) {
		if (negate_interface != 0)
			sprintf(buff, "iptables -t nat -%c %s ! -i %s -p tcp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
		else
			sprintf(buff, "iptables -t nat -%c %s -i %s -p tcp --dport %d -j REDIRECT --to-port %d", operation, type, intf, dest_port, red_to_port);
	}
	else if (strcmp (type, "OUTPUT") == 0) {
		if (btype == CLIENT)
			sprintf(buff, "iptables -t nat -%c %s -p tcp -d %s --dport %d -j DNAT --to 127.0.0.1:%d", operation, type, target_ip, dest_port, red_to_port);
		else if (btype == SERVER)
			sprintf(buff, "iptables -t nat -%c %s -p tcp -d %s --sport %d -j DNAT --to 127.0.0.1:%d", operation, type, target_ip, dest_port, red_to_port);

	}

	rc = system(buff);
	if (rc == -1)
		return_error(buff, rc);

	if (strcmp (type, "OUTPUT") == 0) {
		if (btype == CLIENT) {
			sprintf(buff, "iptables -t nat -%c POSTROUTING -p tcp -d %s --sport %d -j SNAT --to-source %s:%d", operation, "127.0.0.1", red_to_port, target_ip, dest_port);
			rc = system(buff);
			if (rc == -1)
				return_error(buff, rc);
		}
	}


	return rc;
}

static int
init_udp(
		struct udp_session *linphone_proxy,
		char *remote_ip)
{
	int port, i, rc;

	/* create socket for proxy-to-proxy communication */
	linphone_proxy->proxy_to_proxy_socket = -1;
	port = START_PORT_NO_INCOMING;
	if ((linphone_proxy->proxy_to_proxy_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		return_error("cannot create proxy-to-proxy socket", -1);
	rc = -1;
	rc = bind_socket(
			linphone_proxy->proxy_to_proxy_socket,
			AF_INET,
			port);
	if (rc < 0)
		return_error("cannot bind proxy-to-proxy socket", -1);
	linphone_proxy->proxy_to_proxy_port = port;


	/* creating socket for proxy-to-linphone communication */
	linphone_proxy->proxy_to_linphone_socket = -1;
	port = START_PORT_NO_FORWARDING;
	if ((linphone_proxy->proxy_to_linphone_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		return_error("cannot create proxy-to-linphone socket", -1);

	rc = -1;
	for (i=0; i<10 && rc < 0; i++) {
		rc = bind_socket(
				linphone_proxy->proxy_to_linphone_socket,
			AF_INET,
			port + i);
	}
	if (rc < 0)
		return_error("cannot bind proxy-to-linphone socket", -1);

	linphone_proxy->proxy_to_linphone_port = port + i - 1;
	/* create socket for proxy-to-proxy data communication */
	linphone_proxy->proxy_to_proxy_data_socket = -1;
	port = START_PORT_NO_DATA_INCOMING;
	if ((linphone_proxy->proxy_to_proxy_data_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		return_error("cannot create proxy-to-proxy data socket", -1);

	rc = -1;
	rc = bind_socket(
			linphone_proxy->proxy_to_proxy_data_socket,
			AF_INET,
			port);
	if (rc < 0)
		return_error("cannot bind proxy-to-proxy data socket", -1);
	linphone_proxy->proxy_to_proxy_data_port = port;

	/* creating socket for proxy-to-linphone data communication */
	linphone_proxy->proxy_to_linphone_data_socket = -1;
	port = START_PORT_NO_DATA_FORWARDING;

	if ((linphone_proxy->proxy_to_linphone_data_socket = create_socket(AF_INET, SOCK_DGRAM)) < 0)
		return_error("cannot create proxy-to-linphone data socket", -1);

	rc = -1;
	for (i=0; i<10 && rc < 0; i++) {
		rc = bind_socket(
				linphone_proxy->proxy_to_linphone_data_socket,
				AF_INET,
				port + i);
	}
	if (rc < 0)
		return_error("cannot bind proxy-to-linphone data socket", -1);
	linphone_proxy->proxy_to_linphone_data_port = port + i - 1;

#if 0
	/* add preroute rule for incoming packets - remember to delete at the end */
	i = add_udp_iptables_rule('A', "PREROUTING", "lo", 1, SIP_UDP_PORT, proxy_to_proxy_port);
	if (i == -1) {
		proxy_to_proxy_port = -1; /* no need to delete the iptable rule */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif


	/* redirect control packets from linphone through proxy */
	i = add_udp_iptables_rule('A', "OUTPUT","lo", 1, remote_ip, SIP_UDP_PORT, linphone_proxy->proxy_to_linphone_port);
	if (i == -1) {
		linphone_proxy->proxy_to_linphone_port = -1; /* no need to delete the iptable rule; dele the one before */
		return_error("could not add the iptables rule", -1);
	}


#if 0
	/* add preroute rule for incoming data packets - remember to delete at the end */
	i = add_udp_iptables_rule('A', "PREROUTING", "lo", 1, SIP_DATA_PORT, proxy_to_proxy_data_port);
	if (i == -1) {
		linphone_proxy->proxy_to_proxy_data_port = -1; /* no need to delete the iptable rule */
		release(*proxy_to_proxy_socket, *proxy_to_linphone_socket, *proxy_to_proxy_data_socket, *proxy_to_linphone_data_socket, *configure_socket);
		exit_error("could not add the iptables rule", EXIT_FAILURE);
	}
#endif


	/* redirect data packets from linphone through proxy */
	i = add_udp_iptables_rule('A', "OUTPUT","lo", 1, remote_ip, SIP_DATA_PORT, linphone_proxy->proxy_to_linphone_data_port);
	if (i == -1) {
		linphone_proxy->proxy_to_linphone_data_port = -1; /* no need to delete the iptable rule; delete the one before */
		return_error("could not add the iptables rule", -1);
	}

	return 0;
}

static int
init_tcp(
		struct tcp_session *vnc_proxy,
		char *remote_ip)
{

	int port, i, rc;

	/* create socket for proxy-to-proxy communication */
	vnc_proxy->proxy_to_proxy_socket = -1;
	port = START_PORT_NO_TCP_INCOMMING;
	if ((vnc_proxy->proxy_to_proxy_socket = create_socket(AF_INET, SOCK_STREAM)) < 0)
		return_error("cannot create TCP proxy-to-proxy socket", -1);
	rc = -1;
	rc = bind_socket(
			vnc_proxy->proxy_to_proxy_socket,
			AF_INET,
			port);
	if (rc < 0)
		exit_error("cannot bind TCP proxy-to-proxy socket", EXIT_FAILURE);
	vnc_proxy->proxy_to_proxy_port = port;


	/* creating socket for proxy-to-vnc communication */
	vnc_proxy->proxy_to_app_socket = -1;
	port = START_PORT_NO_TCP_FORWARDING;
	if ((vnc_proxy->proxy_to_app_socket = create_socket(AF_INET, SOCK_STREAM)) < 0) {
		return_error("cannot create TCP proxy-to-vnc socket\n", 1);
	}

	rc = -1;
	for (i=0; i<10 && rc < 0; i++) {
		rc = bind_socket(
			vnc_proxy->proxy_to_app_socket,
			AF_INET,
			port + i);
	}
	if (rc < 0)
		return_error("cannot bind TCP proxy-to-vnc socket", -1);

	vnc_proxy->proxy_to_app_port = port + i - 1;

	/* we only add filter for client */
	if (behavior == CLIENT) {
		/* redirect packets from VNC client through proxy */
		i = add_tcp_iptables_rule(CLIENT, 'A', "OUTPUT","lo", 1, remote_ip, VNC_TCP_PORT, vnc_proxy->proxy_to_app_port);
		if (i == -1) {
			vnc_proxy->proxy_to_app_port = -1; /* no need to delete the iptable rule; delete the one before */
			return_error("could not add the TCP iptables rule", i);
		}
	}
	else if (behavior == SERVER) {
		/* SNAT the Source IP and the Source Port, to make VNC server
		 * think that a real VNC client connected to him */

		i = add_tcp_iptables_rule(SERVER, 'A', "OUTPUT","lo", 1, remote_ip, VNC_TCP_PORT, vnc_proxy->proxy_to_app_port);
		if (i == -1) {
			vnc_proxy->proxy_to_app_port = -1; /* no need to delete the iptable rule; delete the one before */
			return_error("could not add the TCP iptables rule", i);
		}
	}

	return 0;
}

int
init(
		struct udp_session *linphone_proxy,
		struct tcp_session *vnc_proxy,
		int *configure_socket,
		char *remote_ip)
{
	int rc;

	*configure_socket = create_socket(AF_INET, SOCK_DGRAM);
	configure_port = CONFIGURE_PORT;
	if (*configure_socket < 0)
		exit_error("cannot create configure socket", EXIT_FAILURE);
	rc = bind_socket(
			*configure_socket,
			AF_INET,
			configure_port);
	if (rc < 0) {
		release(linphone_proxy, vnc_proxy, *configure_socket, remote_ip);
		exit_error("cannot bind configure socket", EXIT_FAILURE);
	}
	printf ("UDP socket created for configurations on port %d\n", configure_port);

	rc = init_udp(linphone_proxy, remote_ip);
	if (rc < 0) {
		release(linphone_proxy, vnc_proxy, *configure_socket, remote_ip);
		exit_error("Could not create proxy for \n", EXIT_FAILURE);
	}
	printf ("UDP socket created for proxy-to-proxy setup traffic on port %d\n", linphone_proxy->proxy_to_proxy_port);
	printf ("UDP socket created for proxy-to-linphone setup traffic on port %d\n", linphone_proxy->proxy_to_linphone_port);
	printf ("UDP socket created for proxy-to-proxy data traffic on port %d\n", linphone_proxy->proxy_to_proxy_data_port);
	printf ("UDP socket created for proxy-to-linphone data traffic on port %d\n", linphone_proxy->proxy_to_linphone_data_port);

	rc = init_tcp(vnc_proxy, remote_ip);
	if (rc < 0) {
		release(linphone_proxy, vnc_proxy, *configure_socket, remote_ip);
		exit_error("Could not create proxy for vnc\n", EXIT_FAILURE);
	}
	printf ("TCP socket created for proxy-to-proxy setup traffic on port %d\n", vnc_proxy->proxy_to_proxy_port);
	printf ("TCP socket created for proxy-to-vnc setup traffic on port %d\n", vnc_proxy->proxy_to_app_port);

	return 0;
}

static void
release_udp(struct udp_session *linphone_proxy, char *remote_ip)
{
	int rc;

	if (linphone_proxy->proxy_to_proxy_socket < 0) {
		destroy_socket(linphone_proxy->proxy_to_proxy_socket);
		linphone_proxy->proxy_to_proxy_socket = -1;
	}

	if (linphone_proxy->proxy_to_linphone_socket < 0) {
		destroy_socket(linphone_proxy->proxy_to_linphone_socket);
		linphone_proxy->proxy_to_linphone_socket = -1;
	}

	if (linphone_proxy->proxy_to_proxy_data_socket < 0) {
		destroy_socket(linphone_proxy->proxy_to_proxy_data_socket);
		linphone_proxy->proxy_to_proxy_data_socket = -1;
	}

	if (linphone_proxy->proxy_to_linphone_data_socket < 0) {
		destroy_socket(linphone_proxy->proxy_to_linphone_data_socket);
		linphone_proxy->proxy_to_linphone_data_socket = -1;
	}

#if 0
	if (proxy_to_proxy_port != -1) {
		rc = add_udp_iptables_rule('D', "PREROUTING", "lo", 1, SIP_UDP_PORT, proxy_to_proxy_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_proxy_port = -1;
	}
#endif


	if (linphone_proxy->proxy_to_linphone_port != -1) {
		rc = add_udp_iptables_rule('D', "OUTPUT","lo", 1, remote_ip, SIP_UDP_PORT, linphone_proxy->proxy_to_linphone_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		linphone_proxy->proxy_to_linphone_port = -1;
	}

#if 0
	if (proxy_to_proxy_data_port != -1) {
		rc = add_udp_iptables_rule('D', "PREROUTING", "lo", 1, SIP_DATA_PORT, proxy_to_proxy_data_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		proxy_to_proxy_data_port = -1;
	}
#endif

	if (linphone_proxy->proxy_to_linphone_data_port != -1) {
		rc = add_udp_iptables_rule('D', "OUTPUT","lo", 1, remote_ip, SIP_DATA_PORT, linphone_proxy->proxy_to_linphone_data_port);
		if (rc == -1)
			printf ("could not delete rule from iptables\n");
		linphone_proxy->proxy_to_linphone_data_port = -1;
	}

	printf("Removed filters for UDP\n");
}

static void
release_tcp(struct tcp_session *vnc_proxy, char *remote_ip)
{
	int rc;

	if (vnc_proxy->proxy_to_proxy_socket < 0) {
		destroy_socket(vnc_proxy->proxy_to_proxy_socket);
		vnc_proxy->proxy_to_proxy_socket = -1;
	}

	if (vnc_proxy->proxy_to_app_socket < 0) {
		destroy_socket(vnc_proxy->proxy_to_app_socket);
		vnc_proxy->proxy_to_app_socket = -1;
	}

#if 0
	if (proxy_to_proxy_port != -1) {
		rc = add_udp_iptables_rule('D', "PREROUTING", "lo", 1, SIP_UDP_PORT, proxy_to_proxy_port);
		if (rc == -1)
			printf ("could not delete TCP rule from iptables\n");
		proxy_to_proxy_port = -1;
	}
#endif

#if 0
	if (vnc_proxy->proxy_to_app_port != -1) {
		rc = add_tcp_iptables_rule('D', "OUTPUT","lo", 1, remote_ip, VNC_TCP_PORT, vnc_proxy->proxy_to_app_port);
		if (rc == -1)
			printf ("could not delete rule TCP from iptables\n");
		vnc_proxy->proxy_to_app_port = -1;
	}
#endif
	if (vnc_proxy->proxy_to_app_port != -1) {
		if (behavior == CLIENT) {
			/* redirect packets from VNC client through proxy */
			rc = add_tcp_iptables_rule(CLIENT, 'D', "OUTPUT","lo", 1, remote_ip, VNC_TCP_PORT, vnc_proxy->proxy_to_app_port);
			if (rc == -1)
				return_error("could not remove the TCP iptables rule", rc);
			vnc_proxy->proxy_to_app_port = -1;
		}
		else if (behavior == SERVER) {
			/* SNAT the Source IP and the Source Port, to make VNC server
			 * think that a real VNC client connected to him */

			rc = add_tcp_iptables_rule(SERVER, 'D', "OUTPUT","lo", 1, remote_ip, VNC_TCP_PORT, vnc_proxy->proxy_to_app_port);
			if (rc == -1)
				return_error("could not remove the TCP iptables rule", rc);
			vnc_proxy->proxy_to_app_port = -1;
		}
	}

	printf("Removed filters for TCP\n");
}

void
release(struct udp_session *linphone_proxy, struct tcp_session *vnc_proxy, int configure_socket, char *remote_ip)
{
	if (configure_socket < 0) {
		destroy_socket(configure_socket);
		configure_socket = -1;
	}

	release_udp(linphone_proxy, remote_ip);
	release_tcp(vnc_proxy, remote_ip);

}
