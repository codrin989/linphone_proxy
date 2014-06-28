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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef uint64_t	u64;
typedef int64_t		s64;
typedef uint32_t	u32;
typedef int32_t		s32;
typedef uint16_t	u16;
typedef int16_t		s16;
typedef uint8_t		u8;
typedef int8_t		s8;

#define REPAIR_FILE "/tmp/proxy_tcp_repair.dmp"

int
write_repair_socket(const char *file, struct tcp_server_repair *tse, char *in_buf, char *out_buf)
{
	int fd, rc;

	fd = open(file, O_WRONLY | O_CREAT, 0644);
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
read_repair_socket(const char *file, struct tcp_server_repair *tse, char *in_buf, char *out_buf)
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

	in_buf = malloc(tse->inq_len);
	if (!in_buf) {
		perror("Failed to alloc data\n");
		close(fd);
		return -1;
	}

	rc = read(fd, in_buf, tse->inq_len);
	if (rc != tse->inq_len) {
		perror("Failed to read whole TCP receive structure\n");
		close(fd);
		free(in_buf);
		return -1;
	}

	out_buf = malloc(tse->outq_len);
	if (!out_buf ) {
		perror("Failed to alloc data\n");
		close(fd);
		free(in_buf);
		return -1;
	}

	rc = read(fd, out_buf, tse->outq_len);
	if (rc != tse->outq_len) {
		perror("Failed to write whole TCP send structure\n");
		free(in_buf);
		free(out_buf);
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

static int
refresh_inet_sk(struct inet_tcp_sk_desc *sk)
{
	unsigned int size;
	struct tcp_info info;

	size = sizeof(info);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_INFO, &info, &size)) {
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
tcp_repair_establised(int fd, struct inet_tcp_sk_desc *sk)
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
	int ret, img_fd, aux;
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
		printf("Faile to get options for TCP socket\n");
	}

	/*
	 * TCP socket options
	 */

	size = sizeof(aux);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_NODELAY, &aux, &size)) {
		printf("Failed to get TCP NODELAY value\n");
		return -1;
	}
	if (aux) {
		tse.has_nodelay = true;
		tse.nodelay = true;
	}

	size = sizeof(aux);
	if (do_getsockopt(sk->rfd, SOL_TCP, TCP_CORK, &aux, &size)) {
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

	write_repair_socket(REPAIR_FILE, &tse, in_buf, out_buf);
#if 0
	img_fd = open_image(CR_FD_TCP_STREAM, O_DUMP, sk->sd.ino);
	if (img_fd < 0)
		goto err_img;

	ret = pb_write_one(img_fd, &tse, PB_TCP_STREAM);
	if (ret < 0)
		goto err_iw;

	if (in_buf) {
		ret = write_img_buf(img_fd, in_buf, tse.inq_len);
		if (ret < 0)
			goto err_iw;
	}

	if (out_buf) {
		ret = write_img_buf(img_fd, out_buf, tse.outq_len);
		if (ret < 0)
			goto err_iw;
	}

	pr_info("Done\n");
err_iw:
	close(img_fd);
err_img:
err_opt:
	xfree(out_buf);
err_out:
	xfree(in_buf);
err_in:
#endif
	return ret;
}

int dump_one_tcp(int fd, struct inet_tcp_sk_desc *sk)
{
	if (sk->state != TCP_ESTABLISHED)
		return 0;

	printf("Dumping TCP connection\n");

	if (tcp_repair_establised(fd, sk))
		return -1;

	if (dump_tcp_conn_state(sk))
		return -1;

	/*
	 * Socket is left in repair mode, so that at the end it's just
	 * closed and the connection is silently terminated
	 */
	return 0;
}

int
tcp_repair_socket_get(int sockfd, struct tcp_server_repair *repair)
{

	return 0;
}

int
tcp_repair_socket_set(int sockfd, const struct tcp_server_repair *repair)
{

	/* Not implemented */
	/*
	if (app_sock_descr.type != SOCK_STREAM) {
									printf("Error: Trying to dump non-TCP socket\n");
								}
*/
	return 0;
}
