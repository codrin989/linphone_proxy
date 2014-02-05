#include "includes/utils.h"
#include "includes/parse.h"

static unsigned char invite[MAX_PACKET_SIZE];
static unsigned char ack[MAX_PACKET_SIZE];
static int invite_len;
static int ack_len;

int string_eq(char *buffer, char *word)
{
	return (0 == strncmp(buffer, word, strlen(word))) ? 1 : 0;
}
int parse(char *buffer, int len)
{
	if (string_eq(buffer, "INVITE")) {
		memcpy(invite, buffer, len);
		invite_len = len;
		return INVITE;
	}

	if (string_eq(buffer, "SIP/2.0 200 OK")) {
		return INVITE_OK;
	}

	if (string_eq(buffer, "ACK")) {
		memcpy(ack, buffer, len);
		ack_len = len;
		return ACK;
	}

	return 0;
}

void get_first_tag(char *buffer, char *tag)
{
	char *p = strstr(buffer, "tag=") + 4;
	strncpy(tag, p, 10);
	tag[10] = 0;
}

void get_second_tag(char *buffer, char *tag)
{
	char *p = strstr(buffer, "tag=") + 4;
	p = strstr(p, "tag=") + 4;
	strncpy(tag, p, 10);
	tag[10] = 0;
}

void get_call_id(char *buffer, char *call_id)
{
	char *p = strstr(buffer, "Call-ID:") + 8;
	strncpy(call_id, p, 9);
	call_id[9] = 0;
}

int get_spoofed_invite(char *buffer, char *peer_ip, char *remote_ip,
		char *call_id, char *local_tag, char *remote_tag)
{
	return 0;
}

int get_spoofed_ack(char *buffer, char *peer_ip, char *remote_ip,
		char *call_id, char *local_tag, char *remote_tag)
{
	return 0;
}
