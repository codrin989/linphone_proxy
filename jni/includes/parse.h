#ifndef PARSE_H_
#define PARSE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

enum {
	INVITE = 1,
	INVITE_OK,
	ACK
};

int string_eq(char *buffer, char *word);

int parse(char *buffer, int len);
void get_first_tag(char *buffer, char *tag);
void get_second_tag(char *buffer, char *tag);
void get_call_id(char *buffer, char *call_id);

int get_spoofed_invite(char *buffer, char *peer_ip, char *remote_ip,
		char *call_id, char *local_tag, char *remote_tag);
int get_spoofed_ack(char *buffer, char *peer_ip, char *remote_ip,
		char *call_id, char *local_tag, char *remote_tag);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_H_ */
