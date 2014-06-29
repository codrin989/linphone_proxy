/*
 * socket.c
 *
 *  Created on: Mar 12, 2013
 *      Author: codrin
 */


#include "socket.h"
#include <netinet/tcp.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define REPAIR_FILE "/tmp/proxy_tcp_repair.dmp"
#define REPAIR_FILE_2 "/tmp/proxy_tcp_repair.dmp_out"
static int tcp_max_wshare = 2U << 20;
static int tcp_max_rshare = 3U << 20;

int
write_repair_socket(const char *file, struct tcp_server_repair *tse, char *in_buf, char *out_buf)
{
	int fd, rc;

	fd = open(file, O_WRONLY | O_CREAT, 0777);
	if (fd < 0) {
		perror("Failed to create file for TCP repair: ");
		return -1;
	}

	/* write magic string */
	rc = write(fd, "TCP", 3);
	if (rc != 3) {
		perror("Failed to write whole magic string\n");
		close(fd);
		return -1;
	}
	rc = write(fd, tse, sizeof(*tse));
	if (rc != sizeof(*tse)) {
		perror("Failed to write whole TCP repair structure\n");
		close(fd);
		return -1;
	}

	rc = write(fd, in_buf, tse->inq_len);
	if (rc != tse->inq_len) {
		perror("Failed to write whole TCP receive structure\n");
		close(fd);
		return -1;
	}

	rc = write(fd, out_buf, tse->outq_len);
	if (rc != tse->outq_len) {
		perror("Failed to write whole TCP send structure\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int
read_repair_socket(const char *file, struct tcp_server_repair *tse, char **in_buf, char **out_buf)
{
	int fd, rc;
	char magic[4];
	bzero(magic, sizeof(magic));

	fd = open(file, O_RDONLY, 0644);
	if (fd < 0) {
		perror("Failed to open file for TCP repair: ");
		return -1;
	}

	/* write magic string */
	rc = read(fd, magic, 3);
	if (rc != 3) {
		perror("Failed to read whole magic string\n");
		close(fd);
		return -1;
	}
	if (strcmp(magic, "TCP") != 0) {
		printf("Invalid magic string found\n");
		close(fd);
		return -1;
	}

	rc = read(fd, tse, sizeof(*tse));
	if (rc != sizeof(*tse)) {
		perror("Failed to read whole TCP repair structure\n");
		close(fd);
		return -1;
	}

	*in_buf = malloc(tse->inq_len);
	if (!*in_buf) {
		perror("Failed to alloc data\n");
		close(fd);
		return -1;
	}

	rc = read(fd, *in_buf, tse->inq_len);
	if (rc != tse->inq_len) {
		perror("Failed to read whole TCP receive structure\n");
		close(fd);
		free(*in_buf);
		return -1;
	}

	*out_buf = malloc(tse->outq_len);
	if (!out_buf ) {
		perror("Failed to alloc data\n");
		close(fd);
		free(*in_buf);
		return -1;
	}

	rc = read(fd, *out_buf, tse->outq_len);
	if (rc != tse->outq_len) {
		perror("Failed to write whole TCP send structure\n");
		free(*in_buf);
		free(*out_buf);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int
do_getsockopt(int sk, int level, int name, void *val, int len)
{
	socklen_t aux = len;

	if (getsockopt(sk, level, name, val, &aux) < 0) {
		perror("Can't get level and name:");
		return -1;
	}

	if (aux != len) {
		printf("ERROR: Len mismatch on %d:%d : %d, want %d\n",
				level, name, aux, len);
		return -1;
	}

	return 0;
}

int
do_setsockopt(int sk, int level, int name, void *val, int len)
{
	if (setsockopt(sk, level, name, val, len) < 0) {
		perror("Can't setsockopt: ");
		return -1;
	}

	return 0;
}

static int
restore_prepare_socket(int sk)
{
	int flags;

	/* In kernel a bufsize has type int and a value is doubled. */
	u32 maxbuf = INT_MAX / 2;

	if (do_setsockopt(sk, SOL_SOCKET, SO_SNDBUFFORCE, &maxbuf, sizeof(maxbuf)))
		return -1;
	if (do_setsockopt(sk, SOL_SOCKET, SO_RCVBUFFORCE, &maxbuf, sizeof(maxbuf)))
		return -1;

	/* Prevent blocking on restore */
	flags = fcntl(sk, F_GETFL, 0);
	if (flags == -1) {
		perror("Unable to get flags for file descriptor: ");
		return -1;
	}
	if (fcntl(sk, F_SETFL, flags | O_NONBLOCK) ) {
		perror("Unable to set O_NONBLOCK for file descriptor: ");
		return -1;
	}

	return 0;
}


int
create_socket(int domain, int type) {
	int sockfd;

	sockfd = socket(domain, type, 0);
	if (sockfd < 0) {
		return_error("cannot open socket", sockfd);
	}

	return sockfd;
}

int
bind_socket(int sockfd, int domain, int portno) {
	struct sockaddr_in serv_addr;
	int rc;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = domain;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	rc = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (rc < 0) {
		close(sockfd);
		return_error("cannot bind socket", rc);
	}
	return rc;
}

void
destroy_socket(int sockfd) {
	close(sockfd);
}

int
listen_socket(int sockfd) {
	return listen(sockfd, 5);
}


int
socket_type_get(int sockfd)
{
	int optval;
	unsigned int size = sizeof(optval);

	if (getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &optval, &size)) {
		perror("Cannot get socket TYPE:");
		return -1;
	}
	return optval;
}

int
tcp_state_get(int sockfd)
{
	struct tcp_info info;
	unsigned int size = sizeof(info);

	if (getsockopt(sockfd, SOL_TCP, TCP_INFO, &info, &size)) {
		perror("Cannot get socket TYPE:");
		return -1;
	}
	return info.tcpi_state;
}

int
tcp_repair_on(int fd)
{
	int ret, aux = 1;

	ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
	if (ret < 0)
		perror("Can't turn TCP repair mode ON");

	return ret;
}

static inline void
tcp_repair_off(int fd)
{
	int aux = 0, ret;

	ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
	if (ret < 0)
		perror("Failed to turn off repair mode on socket: ");
	return;
}

static int
refresh_inet_sk(struct inet_tcp_sk_desc *sk)
{
	unsigned int size;
	struct tcp_info info;

	size = sizeof(info);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_INFO, &info, size)) {
		perror("Failed to obtain TCP_INFO");
		return -1;
	}

	switch (info.tcpi_state) {
	case TCP_ESTABLISHED:
	case TCP_CLOSE:
		break;
	default:
		printf("ERROR: Unknown state %d\n", sk->state);
		return -1;
	}

	if (ioctl(sk->rfd, SIOCOUTQ, &size) == -1) {
		perror("ERROR: Unable to get size of snd queue");
		return -1;
	}

	sk->wqlen = size;

	if (ioctl(sk->rfd, SIOCOUTQNSD, &size) == -1) {
		perror("Unable to get size of unsent data");
		return -1;
	}

	sk->uwqlen = size;

	if (ioctl(sk->rfd, SIOCINQ, &size) == -1) {
		perror("Unable to get size of recv queue");
		return -1;
	}

	sk->rqlen = size;

	return 0;
}

static int
tcp_stream_get_options(int sk, struct tcp_server_repair *tse)
{
	int ret;
	socklen_t auxl;
	struct tcp_info ti;
	int val;

	auxl = sizeof(ti);
	ret = getsockopt(sk, SOL_TCP, TCP_INFO, &ti, &auxl);
	if (ret < 0) {
		perror("Couldn't get TCP info for options\n");
		return ret;
	}

	auxl = sizeof(tse->mss_clamp);
	ret = getsockopt(sk, SOL_TCP, TCP_MAXSEG, &tse->mss_clamp, &auxl);
	if (ret < 0) {
		perror("Couldn't get MSS clamp for TCP socket\n");
		return ret;
	}

	tse->opt_mask = ti.tcpi_options;
	if (ti.tcpi_options & TCPI_OPT_WSCALE) {
		tse->snd_wscale = ti.tcpi_snd_wscale;
		tse->rcv_wscale = ti.tcpi_rcv_wscale;
		tse->has_rcv_wscale = true;
	}

	if (ti.tcpi_options & TCPI_OPT_TIMESTAMPS) {
		auxl = sizeof(val);
		ret = getsockopt(sk, SOL_TCP, TCP_TIMESTAMP, &val, &auxl);
		if (ret < 0) {
			perror("Couldn't get timestamp for TCP socket\n");
			return ret;
		}

		tse->has_timestamp = true;
		tse->timestamp = val;
	}

	printf("\toptions: mss_clamp %x wscale %x tstamp %d sack %d\n",
			(int)tse->mss_clamp,
			ti.tcpi_options & TCPI_OPT_WSCALE ? (int)tse->snd_wscale : -1,
			ti.tcpi_options & TCPI_OPT_TIMESTAMPS ? 1 : 0,
			ti.tcpi_options & TCPI_OPT_SACK ? 1 : 0);

	return 0;

}

static int
tcp_repair_established(int fd, struct inet_tcp_sk_desc *sk)
{
	int ret;

	printf("\tTurning repair on for a socket\n");
	/*
	 * Keep the socket open in criu till the very end. In
	 * case we close this fd after one task fd dumping and
	 * fail we'll have to turn repair mode off
	 */
#if 0
	sk->rfd = dup(fd);
	if (sk->rfd < 0) {
		perror("Can't save socket fd for repair");
		return -1;
	}
#endif
	sk->rfd = fd;

	ret = tcp_repair_on(sk->rfd);
	if (ret < 0)
		return ret;


	ret = refresh_inet_sk(sk);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * TCP queues sequences and their relations to the code below
 *
 *       output queue
 * net <----------------------------- sk
 *        ^       ^       ^    seq >>
 *        snd_una snd_nxt write_seq
 *
 *                     input  queue
 * net -----------------------------> sk
 *     << seq   ^       ^
 *              rcv_nxt copied_seq
 *
 *
 * inq_len  = rcv_nxt - copied_seq = SIOCINQ
 * outq_len = write_seq - snd_una  = SIOCOUTQ
 * inq_seq  = rcv_nxt
 * outq_seq = write_seq
 *
 * On restore kernel moves the option we configure with setsockopt,
 * thus we should advance them on the _len value in restore_tcp_seqs.
 *
 */

static int
tcp_stream_get_queue(int sk, int queue_id,
		u32 *seq, u32 len, char **bufp)
{
	int ret, aux;
	socklen_t auxl;
	char *buf;

	//pr_debug("\tSet repair queue %d\n", queue_id);
	aux = queue_id;
	auxl = sizeof(aux);
	ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &aux, auxl);
	if (ret < 0) {
		perror("Failed to set TCP repair queue: ");
		return ret;
	}

	//pr_debug("\tGet queue seq\n");
	auxl = sizeof(*seq);
	ret = getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, seq, &auxl);
	if (ret < 0) {
		perror("Failed to get queue seq: ");
		return ret;
	}

	printf("\t`- seq %u len %u\n", *seq, len);

	if (len) {
		/*
		 * Try to grab one byte more from the queue to
		 * make sure there are len bytes for real
		 */
		buf = malloc(len + 1);
		if (!buf) {
			printf("No more memmory\n");
			return -1;
		}

		printf("\tReading queue (%d bytes)\n", len);
		ret = recv(sk, buf, len + 1, MSG_PEEK | MSG_DONTWAIT);
		if (ret != len) {
			perror("error when reading queue\n");
			free(buf);
			return -1;
		}
	} else
		buf = NULL;

	*bufp = buf;

	return 0;
}


static int
dump_tcp_conn_state(struct inet_tcp_sk_desc *sk)
{
	int ret, aux;
	struct tcp_server_repair tse;
	char *in_buf, *out_buf;
	unsigned int size;

	bzero(&tse,sizeof(tse));

	/*
	 * Read queue
	 */

	printf("Reading inq for socket\n");
	tse.inq_len = sk->rqlen;
	ret = tcp_stream_get_queue(sk->rfd, TCP_RECV_QUEUE,
			&tse.inq_seq, tse.inq_len, &in_buf);
	if (ret < 0) {
		printf("ERROR: Failed to get TCP receive queue\n");
		return -1;
	}

	/*
	 * Write queue
	 */

	printf("Reading outq for socket\n");
	tse.outq_len = sk->wqlen;
	tse.unsq_len = sk->uwqlen;
	tse.has_unsq_len = true;
	ret = tcp_stream_get_queue(sk->rfd, TCP_SEND_QUEUE,
			&tse.outq_seq, tse.outq_len, &out_buf);
	if (ret < 0) {
		printf("ERROR: Failed to get TCP send queue\n");
		return ret;
	}

	/*
	 * Initial options
	 */

	printf("Reading options for socket\n");
	ret = tcp_stream_get_options(sk->rfd, &tse);
	if (ret < 0) {
		printf("Failed to get options for TCP socket\n");
	}

	/*
	 * TCP socket options
	 */

	size = sizeof(aux);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_NODELAY, &aux, size)) {
		printf("Failed to get TCP NODELAY value\n");
		return -1;
	}
	if (aux) {
		tse.has_nodelay = true;
		tse.nodelay = true;
	}

	size = sizeof(aux);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_CORK, &aux, size)) {
		printf("Failed to get TCP CORK value\n");
		return -1;
	}

	if (aux) {
		tse.has_cork = true;
		tse.cork = true;
	}

	/*
	 * Push the stuff to image
	 */

	if (write_repair_socket(REPAIR_FILE, &tse, in_buf, out_buf)) {
		printf("Writing to TCP repair state to %s failed\n", REPAIR_FILE);
		return -1;
	}

	return 0;
}

int
dump_one_tcp(int fd, struct inet_tcp_sk_desc *sk)
{
	if (sk->state != TCP_ESTABLISHED)
		return 0;

	printf("Dumping TCP connection\n");

	if (tcp_repair_established(fd, sk))
		return -1;

	if (dump_tcp_conn_state(sk))
		return -1;

	/*
	 * Socket is left in repair mode, so that at the end it's just
	 * closed and the connection is silently terminated
	 */
	return 0;
}

static int
set_tcp_queue_seq(int sk, int queue, u32 seq)
{
	printf("\tSetting %d queue seq to %u\n", queue, seq);

	if (setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue, sizeof(queue)) < 0) {
		perror("Can't set repair queue");
		return -1;
	}

	if (setsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, sizeof(seq)) < 0) {
		perror("Can't set queue seq");
		return -1;
	}

	return 0;
}

static int
restore_tcp_seqs(int sk, struct tcp_server_repair *tse)
{
	if (set_tcp_queue_seq(sk, TCP_RECV_QUEUE,
				tse->inq_seq - tse->inq_len))
		return -1;
	if (set_tcp_queue_seq(sk, TCP_SEND_QUEUE,
				tse->outq_seq - tse->outq_len))
		return -1;

	return 0;
}

static int
__send_tcp_queue(int sk, int queue, u32 len, char *data_q)
{
	int ret, err = -1;
	int off, max;
	char *buf;

	buf = malloc(len);
	if (!buf)
		return -1;

//	if (read_img_buf(imgfd, buf, len) < 0)
//		goto err;

	max = (queue == TCP_SEND_QUEUE) ? tcp_max_wshare : tcp_max_rshare;
	off = 0;
	while (len) {
		int chunk = (len > max ? max : len);

		ret = send(sk, data_q + off, chunk, 0);
		if (ret != chunk) {
			perror("Can't restore queue data");
			goto err;
		}
		off += chunk;
		len -= chunk;
	}

	err = 0;
err:
	free(buf);

	return err;
}

static int
send_tcp_queue(int sk, int queue, u32 len, char *data_q)
{
	printf("\tRestoring TCP %d queue data %u bytes\n", queue, len);

	if (setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue, sizeof(queue)) < 0) {
		perror("Can't set repair queue");
		return -1;
	}

	return __send_tcp_queue(sk, queue, len, data_q);
}

static int
restore_tcp_queues(int sk, struct tcp_server_repair *tse, char *in_buf, char *out_buf)
{
	u32 len;

	if (restore_prepare_socket(sk))
		return -1;

	len = tse->inq_len;
	if (len && send_tcp_queue(sk, TCP_RECV_QUEUE, len, in_buf))
		return -1;

	/*
	 * All data in a write buffer can be divided on two parts sent
	 * but not yet acknowledged data and unsent data.
	 * The TCP stack must know which data have been sent, because
	 * acknowledgment can be received for them. These data must be
	 * restored in repair mode.
	 */
	len = tse->outq_len - tse->unsq_len;
	if (len && send_tcp_queue(sk, TCP_SEND_QUEUE, len, out_buf))
		return -1;

	/*
	 * The second part of data have never been sent to outside, so
	 * they can be restored without any tricks.
	 */
	len = tse->unsq_len;
	tcp_repair_off(sk);
	if (len && __send_tcp_queue(sk, TCP_SEND_QUEUE, len, out_buf + (tse->outq_len - tse->unsq_len)))
		return -1;
	if (tcp_repair_on(sk))
		return -1;

	return 0;
}

static int
restore_tcp_opts(int sk, struct tcp_server_repair *tse)
{
	struct tcp_repair_opt opts[4];
	int onr = 0;

	printf("\tRestoring TCP options\n");

	if (tse->opt_mask & TCPI_OPT_SACK) {
		opts[onr].opt_code = TCPOPT_SACK_PERM;
		opts[onr].opt_val = 0;
		onr++;
	}

	if (tse->opt_mask & TCPI_OPT_WSCALE) {
		opts[onr].opt_code = TCPOPT_WINDOW;
		opts[onr].opt_val = tse->snd_wscale + (tse->rcv_wscale << 16);
		onr++;
	}

	if (tse->opt_mask & TCPI_OPT_TIMESTAMPS) {
		opts[onr].opt_code = TCPOPT_TIMESTAMP;
		opts[onr].opt_val = 0;
		onr++;
	}

	printf("Will set mss clamp to %u\n", tse->mss_clamp);
	opts[onr].opt_code = TCPOPT_MAXSEG;
	opts[onr].opt_val = tse->mss_clamp;
	onr++;

	if (setsockopt(sk, SOL_TCP, TCP_REPAIR_OPTIONS,
				opts, onr * sizeof(struct tcp_repair_opt)) < 0) {
		perror("Can't repair options: ");
		return -1;
	}

	if (tse->has_timestamp) {
		if (setsockopt(sk, SOL_TCP, TCP_TIMESTAMP,
				&tse->timestamp, sizeof(tse->timestamp)) < 0) {
			perror("Can't set timestamp: ");
			return -1;
		}
	}

	return 0;
}

static int
restore_tcp_conn_state(int sk, struct inet_tcp_sk_desc *ii,
		struct sockaddr_in *rem_sock, unsigned int rem_len)
{
	int aux;
	char *in_buf, *out_buf;
	struct tcp_server_repair tse;

	if (read_repair_socket(REPAIR_FILE_2, &tse, &in_buf, &out_buf))
		printf("Failed to read TCP repair file\n");

	if (restore_tcp_seqs(sk, &tse))
		goto err_c;

	if (bind_socket(sk, ii->family, ii->src_port))
		goto err_c;

	/* Put TCP connection directly into ESTABLISHED state */
	if (connect(sk, (struct sockaddr *)rem_sock, rem_len) == -1) {
		perror("Can't connect inet socket back");
		goto err_c;
	}

	if (restore_tcp_opts(sk, &tse))
		goto err_c;

	if (restore_tcp_queues(sk, &tse, in_buf, out_buf))
		goto err_c;

	if (tse.has_nodelay && tse.nodelay) {
		aux = 1;
		if (do_setsockopt(sk, SOL_TCP, TCP_NODELAY, &aux, sizeof(aux)))
			goto err_c;
	}

	if (tse.has_cork && tse.cork) {
		aux = 1;
		if (do_setsockopt(sk, SOL_TCP, TCP_CORK, &aux, sizeof(aux)))
			goto err_c;
	}

	bzero(&tse, sizeof(tse));

	return 0;

err_c:
	bzero(&tse, sizeof(tse));

	return -1;
}

int
restore_one_tcp(int fd, struct inet_tcp_sk_desc *ii, struct sockaddr_in *rem_sock, unsigned int rem_len)
{
	int aux, ret;

	printf("Restoring TCP connection\n");

	if (tcp_repair_on(fd))
		return -1;

	if (restore_tcp_conn_state(fd, ii, rem_sock, rem_len))
		return -1;

	tcp_repair_off(fd);

	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &aux, sizeof(aux));
	if (ret < 0) {
		perror("Failed to set reuseaddr\n");
		return ret;
	}

	return 0;
}
