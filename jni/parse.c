#include "includes/utils.h"
#include "includes/parse.h"

int begins_with(char *buffer, char *word)
{
	return (0 == strncmp(buffer, word, strlen(word))) ? 1 : 0;
}

void get_invite_data(char *buffer, invite_data_t *invite_data)
{
	char *p, *q;
	int len; 

	printf("here\n");
	p = strstr(buffer, "From: ") + 6;
	q = strstr(p, ";");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(invite_data->from, p, len);
	invite_data->from[len] = 0;

	printf("here\n");
	p = strstr(q, "tag=") + 4;
	q = strstr(p, "\r\n");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(invite_data->from_tag, p, len);
	invite_data->from_tag[len] = 0;

	printf("here\n");
	p = strstr(q, "To: ") + 4;
	q = strstr(p, ">") + 1;
	len = q - p;
	printf("len: %d\n", len);
	strncpy(invite_data->to, p, len);
	invite_data->to[len] = 0;

	printf("here\n");
	p = strstr(p, "Call-ID: ") + 9;
	q = strstr(p, "\r\n");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(invite_data->call_id, p, len);
	invite_data->call_id[len] = 0;
}

void get_ack_data(char *buffer, ack_data_t *ack_data)
{
	char *p, *q;
	int len; 

	printf("here\n");
	p = strstr(buffer, "From: ") + 6;
	q = strstr(p, ";");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(ack_data->from, p, len);
	ack_data->from[len] = 0;

	printf("here\n");
	p = strstr(q, "tag=") + 4;
	q = strstr(p, "\r\n");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(ack_data->from_tag, p, len);
	ack_data->from_tag[len] = 0;

	printf("here\n");
	p = strstr(q, "To: ") + 4;
	q = strstr(p, ">") + 1;
	len = q - p;
	printf("len: %d\n", len);
	strncpy(ack_data->to, p, len);
	ack_data->to[len] = 0;

	printf("here\n");
	p = strstr(q, "tag=") + 4;
	q = strstr(p, "\r\n");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(ack_data->to_tag, p, len);
	ack_data->to_tag[len] = 0;

	printf("here\n");
	p = strstr(p, "Call-ID: ") + 9;
	q = strstr(p, "\r\n");
	len = q - p;
	printf("len: %d\n", len);
	strncpy(ack_data->call_id, p, len);
	ack_data->call_id[len] = 0;
}
