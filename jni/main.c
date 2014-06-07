
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

#include "includes/util.h"
#include "includes/parse.h"
#include "includes/socket.h"

#define NUM_FDS 6

static packet_t out_invite, useful_invite;
static packet_t out_ack, useful_ack;
static packet_t out_op_ok, useful_op_ok;
static packet_t options;
static packet_data_t ack_data, options_data, op_ok_data, data_101;

static int proxy_to_proxy_socket;
static int proxy_to_linphone_socket;
static int proxy_to_proxy_data_socket;
static int proxy_to_linphone_data_socket;
static int manager_socket;

static struct sockaddr_in proxy_to_linphone_addr;
static struct sockaddr_in proxy_to_proxy_addr;
static struct sockaddr_in proxy_to_proxy_data_addr;
static struct sockaddr_in proxy_to_linphone_data_addr;
static struct sockaddr_in migrate_addr;
static struct sockaddr_in manager_addr;

static char *local_ip = NULL;
static char *remote_ip = NULL;
static char migrate_ip[32];
static char redirect_ip[32];
static char initial_ip[32];
static struct pollfd fds[NUM_FDS];

static inline void migrate_init();
static inline void migrate_establish();
static inline void migrate_close();
static inline void migrate_redirect();

static char bye[] =
"BYE sip:student@local_ip SIP/2.0\r\n"
"Via: SIP/2.0/UDP remote_ip:5060;rport;branch=z9hG4bK661201147\r\n"
"From: <sip:student@remote_ip>;tag=from_tag\r\n"
"To: <sip:student@local_ip>;tag=to_tag\r\n"
"Call-ID: call_id\r\n"
"CSeq: 2 BYE\r\n"
"Contact: <sip:student@remote_ip:5060>\r\n"
"Max-Forwards: 70\r\n"
"User-Agent: Linphone/3.3.2 (eXosip2/3.3.0)\r\n"
"Content-Length: 0\r\n";

int main(int argc, char *argv[])
{
	char buffer[MAX_PACKET_SIZE];
	static struct sockaddr_in addr;
	int rc, i, count;

	KICK(argc < 3, "incorrect usage\n"
	"Usage:\n"
	"./linphone_proxy <local_ip> <remote_ip>\n");
	local_ip = argv[1];
	remote_ip = argv[2];

	add_poll(&fds[0], STDIN_FILENO);

	proxy_to_linphone_socket = create_socket(SOCK_DGRAM, SIP_LINPHONE);
	init_sockaddr(&proxy_to_linphone_addr, "127.0.0.1", SIP_PORT);
	add_poll(&fds[1], proxy_to_linphone_socket);
	eprintf("created proxy_to_linphone SIP socket   SRC:localhost:%d - DST:localhost:%d\n",
			SIP_LINPHONE, SIP_PORT);

	proxy_to_proxy_socket = create_socket(SOCK_DGRAM, SIP_PROXY);
	init_sockaddr(&proxy_to_proxy_addr, remote_ip, SIP_PROXY);
	add_poll(&fds[2], proxy_to_proxy_socket);
	eprintf("created proxy_to_sip SIP socket        SRC:localhost:%d - DST:%s:%d\n",
			SIP_PROXY, remote_ip, SIP_PROXY);

	proxy_to_linphone_data_socket = create_socket(SOCK_DGRAM, DATA_LINPHONE);
	init_sockaddr(&proxy_to_linphone_data_addr, "127.0.0.1", DATA_PORT);
	add_poll(&fds[3], proxy_to_linphone_data_socket);
	eprintf("created proxy_to_linphone DATA socket  SRC:localhost:%d - DST:localhost:%d\n",
			DATA_LINPHONE, DATA_PORT);

	proxy_to_proxy_data_socket = create_socket(SOCK_DGRAM, DATA_PROXY);
	init_sockaddr(&proxy_to_proxy_data_addr, remote_ip, DATA_PROXY);
	add_poll(&fds[4], proxy_to_proxy_data_socket);
	eprintf("created proxy_to_proxy DATA socket     SRC:localhost:%d - DST:%s:%d\n",
			DATA_PROXY, remote_ip, DATA_PROXY);

	manager_socket = create_socket(SOCK_DGRAM, MANAGER_PORT);
	init_sockaddr(&manager_addr, "0.0.0.0", MANAGER_PORT);
	add_poll(&fds[5], manager_socket);
	eprintf("created manager socket                 SRC:localhost:%d - DST:0.0.0.0:0\n",
			MANAGER_PORT);

	while (1) {
		rc = poll(fds, NUM_FDS, -1);
		DIE(-1 == rc, "poll");

		for (i = 0; i < NUM_FDS; i++) {
			if (0 == fds[i].revents) {
				continue;
			}

			switch(i) {
			/* receive line from console */
			case 0:
				break;

			/* receive SIP packet from linphone */
			case 1:
				count = recv_msg(fds[i].fd, &proxy_to_linphone_addr, buffer);

				if (begins_with(buffer, "INVITE")) {
					copy_packet(&out_invite, buffer, count);
					printf("captured INVITE packet:\n%s\n", out_invite.buffer);
				} else if (begins_with(buffer, "ACK")) {
					copy_packet(&out_ack, buffer, count);
					printf("captured ACK packet:\n%s\n", out_ack.buffer);
				} else if (strstr(buffer, "200 OK") && strstr(buffer, "OPTIONS" )) {
					copy_packet(&out_op_ok, buffer, count);
					printf("captured OPTIONS OK packet:\n%s\n", out_op_ok.buffer);
				}

				send_msg(proxy_to_proxy_socket, &proxy_to_proxy_addr, buffer, count);
				break;

			/* receive SIP packet from proxy */
			case 2:
				count = recv_msg(fds[i].fd, &addr, buffer);
				send_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer, count);
				break;

			/* receive data packet from linphone */
			case 3:
				count = recv_msg(fds[i].fd, &proxy_to_linphone_data_addr, buffer);
				send_msg(proxy_to_proxy_data_socket, &proxy_to_proxy_data_addr, buffer, count);
				break;

			/* receive data packet from proxy */
			case 4:
				count = recv_msg(fds[i].fd, &addr, buffer);
				send_msg(proxy_to_linphone_data_socket, &proxy_to_linphone_data_addr, buffer, count);
				break;

			/* receive command from manager */
			case 5:
				count = recv_msg(fds[i].fd, &manager_addr, buffer);

				if (begins_with(buffer, "IP: ")) {					
					while (!isdigit(buffer[count - 1])) {
						buffer[--count] = 0;
					}
					strcpy(migrate_ip, buffer + 4);
					migrate_init();
				} else if (begins_with(buffer, "establish")) {
					migrate_establish();
				} else if (begins_with(buffer, "redirect: ")) {
					while (!isdigit(buffer[count - 1])) {
						buffer[--count] = 0;
					}
					strcpy(redirect_ip, buffer + 10);	
					migrate_redirect();
				}
				break;

			/* error */
			default:
				break;
			}
		}
	}

	return EXIT_SUCCESS;
}

static inline void migrate_init()
{
	char buffer[MAX_PACKET_SIZE];

	eprintf("migrate to IP: #%s#\n", migrate_ip);
	init_sockaddr(&migrate_addr, migrate_ip, MANAGER_PORT);

	strcpy(buffer, "establish");
	send_msg(manager_socket, &migrate_addr, buffer, strlen(buffer));
	printf("sent establish\n");

	send_msg(manager_socket, &migrate_addr, out_invite.buffer, out_invite.size);
	printf("sent INIT\n");

	send_msg(manager_socket, &migrate_addr, out_ack.buffer, out_ack.size);
	printf("sent ACK\n");

	send_msg(manager_socket, &migrate_addr, out_op_ok.buffer, out_op_ok.size);
	printf("sent OPTIONS OK\n");

	recv_msg(manager_socket, &migrate_addr, buffer);
	printf("\nreceived:\n%s\n", buffer);

	init_sockaddr(&migrate_addr, remote_ip, MANAGER_PORT);
	strcpy(buffer, "redirect: ");
	strcat(buffer, migrate_ip);
	send_msg(manager_socket, &migrate_addr, buffer, strlen(buffer));

	get_data(out_ack.buffer, &ack_data);
	memset(buffer, 0, MAX_PACKET_SIZE);
	strcpy(buffer, bye);
	replace_all(buffer, "local_ip", local_ip);
	replace_all(buffer, "remote_ip", remote_ip);
	replace_all(buffer, "from_tag", ack_data.to_tag);
	replace_all(buffer, "to_tag", ack_data.from_tag);
	replace_all(buffer, "call_id", ack_data.call_id);
	send_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer, strlen(buffer));

}

static inline void migrate_establish()
{
	char buffer[MAX_PACKET_SIZE];
	struct pollfd fd;
	int count, rc;
	char *p, *q;

	add_poll(&fd, manager_socket);

	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	count = recv_msg(manager_socket, &manager_addr, buffer);
	copy_packet(&useful_invite, buffer, count);
	eprintf("%s\n\n", useful_invite.buffer);

	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	count = recv_msg(manager_socket, &manager_addr, buffer);
	copy_packet(&useful_ack, buffer, count);
	eprintf("%s\n\n", useful_ack.buffer);

	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	count = recv_msg(manager_socket, &manager_addr, buffer);
	copy_packet(&useful_op_ok, buffer, count);
	eprintf("%s\n\n", buffer);

	get_data(useful_ack.buffer, &ack_data);
	printf("\nACK data\n");
	printf("branch: %s\n", ack_data.branch);	
	printf("From: %s\n", ack_data.from);
	printf("tag: %s\n", ack_data.from_tag);
	printf("To: %s\n", ack_data.to);
	printf("tag: %s\n", ack_data.to_tag);
	printf("Call-id: %s\n", ack_data.call_id);
	printf("\n");

	get_data(useful_op_ok.buffer, &op_ok_data);
	printf("\nOPTIONS OK data\n");
	printf("branch: %s\n", op_ok_data.branch);	
	printf("From: %s\n", op_ok_data.from);
	printf("tag: %s\n", op_ok_data.from_tag);
	printf("To: %s\n", op_ok_data.to);
	printf("tag: %s\n", op_ok_data.to_tag);
	printf("Call-id: %s\n", op_ok_data.call_id);
	printf("\n");

	p = strstr(ack_data.from, "@") + 1;
	q = strstr(p, ">");
	count = q - p;
	strncpy(initial_ip, p, count);
	initial_ip[count] = 0;
	printf("initial_ip: %s\n", initial_ip);

	replace_all(useful_invite.buffer, remote_ip, local_ip);
	replace_all(useful_invite.buffer, initial_ip, remote_ip);
	replace_all(useful_invite.buffer, ack_data.from_tag, ack_data.to_tag);
	useful_invite.size = strlen(useful_invite.buffer);
	printf("INVITE:\n%s\n", useful_invite.buffer);
	send_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr,
			useful_invite.buffer, strlen(useful_invite.buffer));

	add_poll(&fd, proxy_to_linphone_socket);

	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	recv_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer);
	printf("\nreceived:\n%s\n", buffer);

	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	recv_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer);
	printf("\nreceived:\n%s\n", buffer);
	get_data(buffer, &data_101);
	
	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	count = recv_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer);
	copy_packet(&options, buffer, count);
	printf("\nreceived:\n%s\n", buffer);

	get_data(options.buffer, &options_data);
	printf("\nOPTIONS data\n");
	printf("branch: %s\n", options_data.branch);	
	printf("From: %s\n", options_data.from);
	printf("tag: %s\n", options_data.from_tag);
	printf("To: %s\n", options_data.to);
	printf("tag: %s\n", options_data.to_tag);
	printf("Call-id: %s\n", options_data.call_id);
	printf("\n");

	replace_all(useful_op_ok.buffer, remote_ip, local_ip);
	replace_all(useful_op_ok.buffer, initial_ip, remote_ip);
	replace_all(useful_op_ok.buffer, op_ok_data.from_tag, options_data.from_tag);
	replace_all(useful_op_ok.buffer, op_ok_data.to_tag, options_data.to_tag);
	replace_all(useful_op_ok.buffer, op_ok_data.branch, options_data.branch);
	replace_all(useful_op_ok.buffer, op_ok_data.call_id, options_data.call_id);
	printf("\nforged OPTIONS OK: \n%s\n", useful_op_ok.buffer);

	send_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr,
			useful_op_ok.buffer, strlen(useful_op_ok.buffer));

	replace_all(useful_ack.buffer, remote_ip, local_ip);
	replace_all(useful_ack.buffer, initial_ip, remote_ip);
	replace_all(useful_ack.buffer, ack_data.from_tag, "magic");
	replace_all(useful_ack.buffer, ack_data.to_tag, data_101.to_tag);
	replace_all(useful_ack.buffer, "magic", ack_data.to_tag);
	printf("\nforged ACK: \n%s\n", useful_ack.buffer);	

	printf("1\n");
	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	recv_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer);
	printf("\nreceived:\n%s\n", buffer);
	printf("2\n");
	rc = poll(&fd, 1, -1);
	DIE(-1 == rc, "poll");
	recv_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr, buffer);
	printf("\nreceived:\n%s\n", buffer);
	printf("3\n");
	send_msg(proxy_to_linphone_socket, &proxy_to_linphone_addr,
			useful_ack.buffer, strlen(useful_ack.buffer));
	printf("4\n");
	strcpy(buffer, "done");
	send_msg(manager_socket, &manager_addr, buffer, 4);
	printf("5\n");
}

static inline void migrate_redirect()
{
	printf("redirecting to ip: %s\n", redirect_ip);

	remote_ip = redirect_ip;
	init_sockaddr(&proxy_to_proxy_addr, remote_ip, SIP_PROXY);
	init_sockaddr(&proxy_to_proxy_data_addr, remote_ip, DATA_PROXY);
}
