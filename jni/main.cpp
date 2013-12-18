/*
 ============================================================================
 Name        : linphone_proxy.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "includes/main.h"
#include <pthread.h>

using namespace std;

int proxy_to_proxy_socket, proxy_to_linphone_socket, proxy_to_proxy_data_socket, proxy_to_linphone_data_socket, configure_socket;

void *console_routine(void *arg)
{
	char line[256];

	while (1) {
		scanf("%s", line);
		printf("received: %s\n", line);
	}

	return arg;
}

int
main(int argc, char *argv[]) {
	int rc;
	pthread_t console_thread;

	KICK(argc < 2, "incorrect usage\n"
	"Usage:\n"
	"./linphone_proxy <remote_ip>\n");

	puts("Linphone proxy started...");

	rc = init(
			&proxy_to_proxy_socket,
			&proxy_to_linphone_socket,
			&proxy_to_proxy_data_socket,
			&proxy_to_linphone_data_socket,
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

	rc = pthread_create(&console_thread, NULL, console_routine, NULL);

	rc = run_proxy(
			proxy_to_proxy_socket,
			proxy_to_linphone_socket,
			proxy_to_proxy_data_socket,
			proxy_to_linphone_data_socket,
			configure_socket,
			argv[1]
			);
	if (rc == 0)
		printf("Proxy finished ok...\n");
	release(
			proxy_to_proxy_socket,
			proxy_to_linphone_socket,
			proxy_to_proxy_data_socket,
			proxy_to_linphone_data_socket,
			configure_socket,
			argv[1]
			);

	return EXIT_SUCCESS;
}
