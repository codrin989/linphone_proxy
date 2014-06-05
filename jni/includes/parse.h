#ifdef __cplusplus
extern "C" {
#endif
#ifndef PARSE_H_
#define PARSE_H_ 1

#include <string.h>

typedef struct _invite_data_t {
	char from[64];
	char to[64];
	char from_tag[16];
	char call_id[16];
} invite_data_t;

typedef struct _ack_data_t {
	char from[64];
	char to[64];
	char from_tag[16];
	char to_tag[16];	
	char call_id[16];
} ack_data_t;

int begins_with(char *buffer, char *word);
void get_ack_data(char *buffer, ack_data_t *ack_data);
void get_invite_data(char *buffer, invite_data_t *invite_data);

void replace_all(char *original, char *find, char *replace);

#endif /* PARSE_H_ */
#ifdef __cplusplus
}
#endif
