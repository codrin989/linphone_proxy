#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#ifndef PARSE_H_
#define PARSE_H_ 1

typedef struct _packet_data_t {
	char branch[64];
	char from[64];
	char to[64];
	char from_tag[16];
	char to_tag[16];	
	char call_id[16];
} packet_data_t;

inline int begins_with(char *buffer, char *word);
void get_data(char *buffer, packet_data_t *packet_data);
void replace_all(char *original, char *find, char *replace);

#endif /* PARSE_H_ */
#ifdef __cplusplus
}
#endif
