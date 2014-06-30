/*
 * socket.h
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef TCPOPT_SACK_PERM
#define TCPOPT_SACK_PERM TCPOPT_SACK_PERMITTED
#endif

struct inet_tcp_sk_desc {
	unsigned int		family;
	unsigned int		type;
	unsigned int		src_port;
	unsigned int		dst_port;
	unsigned int		state;
	unsigned int		rqlen;
	unsigned int		wqlen; /* sent + unsent data */
	unsigned int		uwqlen; /* unsent data */
	unsigned int		src_addr[4];
	unsigned int		dst_addr[4];

	int rfd;
};

struct tcp_server_repair
{
	uint32_t inq_len; /* receive queue length */
	uint32_t inq_seq; /* receive queue sequence number */
	uint32_t outq_len; /* send queue length */
	uint32_t outq_seq; /* send quueu sequence number */
	uint32_t opt_mask; /* mask used for TCP options */
	uint32_t snd_wscale; /* send window size scale */
	uint32_t mss_clamp; /* MSS clamp value */
	int has_rcv_wscale; /* if true, the next value is considered */
	uint32_t rcv_wscale; /* scale factor for receive window */
	int has_timestamp; /* if the, the next value is considered */
	uint32_t timestamp; /* timestamp */
	int has_cork; /* if true, the next value is considered */
	int cork; /* cork value for the TCP connection */
	int has_nodelay; /* if true, the next value is considered */
	int nodelay; /* no delay flag is used */
	int has_unsq_len; /* if true, the next value is considered */
	uint32_t unsq_len; /* unsent data in the send queue */
};

struct sk_opts_entry
{
  uint32_t so_sndbuf;
  uint32_t so_rcvbuf;
  uint64_t so_snd_tmo_sec;
  uint64_t so_snd_tmo_usec;
  uint64_t so_rcv_tmo_sec;
  uint64_t so_rcv_tmo_usec;
  int has_reuseaddr;
  int reuseaddr;
  int has_so_priority;
  uint32_t so_priority;
  int has_so_rcvlowat;
  uint32_t so_rcvlowat;
  int has_so_mark;
  uint32_t so_mark;
  int has_so_passcred;
  int so_passcred;
  int has_so_passsec;
  int so_passsec;
  int has_so_dontroute;
  int so_dontroute;
  int has_so_no_check;
  int so_no_check;
  char *so_bound_dev;
  size_t n_so_filter;
  uint64_t *so_filter;
};


int
create_socket(int domain, int type);

void
destroy_socket(int sockfd);

int
bind_socket(int sockfd, int domain, int portno);

int
listen_socket(int sockfd);

int
socket_type_get(int sockfd);

int
tcp_state_get(int sockfd);

int
dump_one_tcp(int fd, struct inet_tcp_sk_desc *sk);

int
restore_one_tcp(int fd, struct inet_tcp_sk_desc *ii, struct sockaddr_in *rem_sock, unsigned int rem_len);

#endif /* SOCKET_H_ */
