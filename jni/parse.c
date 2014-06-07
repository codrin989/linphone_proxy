#include "includes/util.h"
#include "includes/parse.h"
#include "includes/socket.h"

inline int begins_with(char *buffer, char *word)
{
	return (0 == strncmp(buffer, word, strlen(word))) ? 1 : 0;
}

void get_data(char *buffer, packet_data_t *packet_data)
{
	char *p, *q;
	int len; 

	printf("get branch\n");
	p = strstr(buffer, "branch=") + 7;
	q = strstr(p, "\r\n");
	len = q - p;
	strncpy(packet_data->branch, p, len);
	packet_data->branch[len] = 0;	
	p = strstr(packet_data->branch, ";received=");
	if (p) {
		*p = 0;
	}

	p = strstr(q, "From: ") + 6;
	q = strstr(p, ";");
	len = q - p;
	strncpy(packet_data->from, p, len);
	packet_data->from[len] = 0;

	p = strstr(q, "tag=") + 4;
	q = strstr(p, "\r\n");
	len = q - p;
	strncpy(packet_data->from_tag, p, len);
	packet_data->from_tag[len] = 0;

	p = strstr(q, "To: ") + 4;
	q = strstr(p, ">") + 1;
	len = q - p;
	strncpy(packet_data->to, p, len);
	packet_data->to[len] = 0;

	if ((p = strstr(q, "tag="))) {
		p += 4;
		q = strstr(p, "\r\n");
		len = q - p;
		strncpy(packet_data->to_tag, p, len);
		packet_data->to_tag[len] = 0;
	}

	p = strstr(q, "Call-ID: ") + 9;
	q = strstr(p, "\r\n");
	len = q - p;
	strncpy(packet_data->call_id, p, len);
	packet_data->call_id[len] = 0;
}

void replace_all(char *original, char *find, char *replace)
{
	char buffer[MAX_PACKET_SIZE];
	char *p, *q;
	int len;

	memset(buffer, 0, MAX_PACKET_SIZE);

	p = q = original;
	while ((q = strstr(q, find))) {
		len = q - p;
		strncat(buffer, p, len);
		strcat(buffer, replace);
		q = p = q + strlen(find);
	}
	strcat(buffer, p);
	strcpy(original, buffer);
}
