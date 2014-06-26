/*
 ============================================================================
 Name        : linphone_proxy.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "main.h"

/* int proxy_to_proxy_socket, proxy_to_linphone_socket, proxy_to_proxy_data_socket, proxy_to_linphone_data_socket,*/
int configure_socket;

enum behavior_type behavior;

int
main(int argc, char *argv[]) {
	int rc;
	struct udp_session linphone_proxy;
	struct tcp_session vnc_proxy;

	KICK(argc < 3, "incorrect usage\n"
	"Usage:\n"
	"./session_proxy <remote_ip> <client/server>\n");
	if (strcmp(argv[2], "server") == 0)
		behavior = SERVER;
	else if (strcmp(argv[2], "client") == 0)
		behavior = CLIENT;
	else KICK(argc < 3, "incorrect usage\n"
			"Usage:\n"
			"./session_proxy <remote_ip> <client/server>\n");

	puts("Session proxy started...");

	rc = init(
			&linphone_proxy,
			&vnc_proxy,
			&configure_socket,
			argv[1]
			);
	if (rc < 0)
		exit_error("failed to initialize proxy\n", EXIT_FAILURE);
	/*
	do {
		fgets (command, MAX_LEN, stdin);
		command[strlen(command) - 1] = '\0';
		if (strcmp (command, "exit") == 0)
			break;
	}while(1);
*/
	rc = run_proxy(
			&linphone_proxy,
			&vnc_proxy,
			configure_socket,
			argv[1]
			);
	if (rc == 0)
		printf("Proxy finished ok...\n");
	release(
			&linphone_proxy,
			&vnc_proxy,
			configure_socket,
			argv[1]
			);

	return EXIT_SUCCESS;
}
