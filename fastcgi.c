#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "fastcgi.h"
#include "lang.h"

#define FCGI_ALIGN(n)		((7 + (n)) & -8)
#define FCGI_ALIGN_DOWN(n)	((n) & -8)

#define FCGI_BEGIN_REQUEST	1
#define FCGI_ABORT_REQUEST	2
#define FCGI_END_REQUEST	3
#define FCGI_PARAMS		4
#define FCGI_STDIN		5
#define FCGI_STDOUT		6
#define FCGI_STDERR		7
#define FCGI_DATA		8
#define FCGI_GET_VALUES		9
#define FCGI_GET_VALUES_RESULT	10
#define FCGI_UNKNOWN_TYPE	11
#define FCGI_MAXTYPE		(FCGI_UNKNOWN_TYPE)

#define FCGI_REQUEST_COMPLETE	0

#define FCGI_READ_INT(d)	(*d >> 7 == 0? *d++: \
    ((*d++ & 127) << 24) + (*d++ << 16) + (*d++ << 8) + *d++)

struct fcgi_end_request_body {
	uint32_t	app_status;
	uint8_t		protocol_status;
	uint8_t		reserved[3];
}__packed;

__dead void	 usage(void);
int		 atoi_or_die(const char *, int, int);
int		 fastcgi_listen(const char *, struct passwd *, int);
void		 fastcgi_accept(int, short, void *);
void		 fastcgi_resume(int, short, void *);
void		 fastcgi_sig_handler(int, short, void *);
void		 fastcgi_close(struct myconnect *);
void		 fastcgi_read(int, short, void *);
void		 fastcgi_write(int, short, void *);
void		 fastcgi_timeout(int, short, void *);
void		*fastcgi_worker(void *);
void		 fastcgi_addjob(struct myconnect *);
void		 fastcgi_header(struct myconnect *,
		    const char *, const char *);
void		 fastcgi_end_internal(struct myconnect *, const char *,
		    const char *, const char *, ...);
void		 fastcgi_end_bigreq(struct myconnect *, const char *);
void		 fastcgi_end_bigres(struct myconnect *, const char *);
int		 fastcgi_add_fcgi_record(struct myconnect *, int,
		    const void *, int);
int		 fastcgi_add_fcgi_multirecord(struct myconnect *, int,
		    const char *, int, const char *, int);
static int	 substr_eq(const char *, int, const char *);

__dead void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-d] [-p path] [-s socket]"
	    " [-t threads] [-u user]\n", __progname);
	exit(1);
}

int
atoi_or_die(const char *s, int min, int max)
{
	const char	*msg;
	int		 res;
	res = (int)strtonum(s, min, max, &msg);
	if (msg)
		errx(1, "your number is %s: %s", msg, optarg);
	return res;
}

int
main(int argc, char **argv)
{
	struct passwd	*pw = NULL;
	int		 i, fd;
	int		 debug = 0, threads = 0, connects = 0;
	const char	*chrootpath = NULL;
	const char	*fastcgi_user = DEFAULT_USER;
	const char	*fastcgi_socket = DEFAULT_SOCKET;
	struct myglobal	*g;

	while ((i = getopt(argc, argv, "dp:s:t:u:")) >= 0) {
		switch (i) {
		case 'd':
			debug = 1;
			break;
		case 'p':
			chrootpath = optarg;
			break;
		case 's':
			fastcgi_socket = optarg;
			break;
		case 't':
			threads = atoi_or_die(optarg, 1, MAX_THREADS);
			break;
		case 'u':
			fastcgi_user = optarg;
			break;
		default:
			usage();
		}
	}
	if (!debug) {
		for (fd = getdtablesize(); --fd >= 0; )
			close(fd);
		if ((fd = open("/dev/null", O_RDWR)) < 0)
			errx(1, "could nut open null device");
		for (i = 0; i < 3; ++i)
			if (fd != i && dup2(fd, i) < 0)
				errx(1, "could not duplicate fd");
		if (fd >= 3)
			close(fd);
	}

	if (!threads) {
		if (debug)
			threads = 1;
		else {
			int	 ncpu, name[] = { CTL_HW, HW_NCPU };
			size_t	 len = sizeof(ncpu);
			if (sysctl(name, sizeof(name)/sizeof(*name),
			    &ncpu, &len, NULL, 0) < 0 ||
			    len != sizeof(ncpu))
				ncpu = 1;
			threads = ncpu;
		}
	}
	if (!connects) {
		connects = (getdtablesize()
		    - (threads * FD_PER_THREAD)
		    - FD_RESERVED) / FD_PER_CONNECT;
	}

	if (connects <= 0 ||
	    geteuid() != 0 ||
	    (!debug && daemon(1, 0) < 0) ||
	    (pw = getpwnam(SOCKET_OWNER)) == NULL)
		errx(1, "daemon/%d%d%d", connects <= 0, !!getuid(), !pw);
	fd = fastcgi_listen(fastcgi_socket, pw, 5);
	if ((pw = getpwnam(fastcgi_user)) == NULL ||
	    (!chrootpath && !(chrootpath = pw->pw_dir)) ||
	    chroot(chrootpath) < 0 ||
	    chdir("/") < 0 ||
	    setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) ||
	    pledge("stdio rpath unix proc", NULL) < 0)
		errx(1, "chroot/%d%d", !pw, !chrootpath);
	event_init();

	if ((g = calloc(1, sizeof(*g))) == NULL ||
	    (g->thread = calloc(threads, sizeof(*g->thread))) == NULL ||
	    (g->connect = calloc(connects, sizeof(*g->connect))) == NULL)
		errx(1, "calloc:%d%d%d", !g, !g->thread, !g->connect);
	g->num_thread = threads;
	g->max_connect = connects;
	g->ll_free = g->connect;
	g->num_connect = 0;
	g->ll_jobdone = NULL;
	g->ll_last_jobdone = NULL;
	g->ll_job = NULL;
	g->ll_last_job = NULL;
	g->th_main = pthread_self();
	pthread_mutex_init(&g->m_jobdone, NULL);
	pthread_mutex_init(&g->m_job, NULL);
	pthread_cond_init(&g->cv_job, NULL);
	event_set(&g->ev_accept, fd, EV_READ | EV_PERSIST, fastcgi_accept, g);
	event_set(&g->ev_resume, fd, 0, fastcgi_resume, g);
	signal_set(&g->ev_sigterm, SIGTERM, fastcgi_sig_handler, g);
	signal_set(&g->ev_sigint, SIGINT, fastcgi_sig_handler, g);
	signal_set(&g->ev_sigpipe, SIGPIPE, fastcgi_sig_handler, g);
	signal_set(&g->ev_sigusr1, SIGUSR1, fastcgi_sig_handler, g);
	for (i = 0; i < connects; ++i) {
		struct myconnect *c = &g->connect[i];
		c->next = i + 1 < connects ? c + 1 : NULL;
		c->g = g;
	}
	for (i = 0; i < threads; ++i) {
		struct mythread *t = &g->thread[i];
		t->g = g;
		if (pthread_create(&t->thread, NULL, fastcgi_worker, t))
			errx(1, "could not create thread #%d", i);
		if (deflateInit2(&t->gzip, Z_DEFAULT_COMPRESSION,
		    Z_DEFLATED, 16 + MAX_WBITS, 8, 0) != Z_OK)
			errx(1, "could not init gzip #%d", i);
	}

	event_add(&g->ev_accept, NULL);
	signal_add(&g->ev_sigterm, NULL);
	signal_add(&g->ev_sigint, NULL);
	signal_add(&g->ev_sigpipe, NULL);
	signal_add(&g->ev_sigusr1, NULL);

	event_dispatch();
	return 0;
}

int
fastcgi_listen(const char *path, struct passwd *pw, int max_queue)
{
	struct sockaddr_un	 sun;
	mode_t			 old_umask;
	int			 fd;
	const size_t		 maxpath = sizeof(sun.sun_path);
	const int		 socket_type =
	    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC;

	old_umask = umask(S_IXUSR | S_IXGRP | S_IWOTH | S_IROTH | S_IXOTH);
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	if ((fd = socket(AF_UNIX, socket_type, 0)) < 0 ||
	    strlcpy(sun.sun_path, path, maxpath) >= maxpath ||
	    (unlink(path) < 0 && errno != ENOENT) ||
	    bind(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0 ||
	    chown(path, pw->pw_uid, pw->pw_gid) < 0 ||
	    listen(fd, max_queue) < 0)
		errx(1, "listen/%d", fd < 0);
	umask(old_umask);
	return fd;
}

void
fastcgi_accept(int fd, short events, void *arg)
{
	struct myglobal		*g = arg;
	struct myconnect	*c = NULL;
	struct sockaddr_storage	 ss;
	struct timeval		 backoff = { 1, 0 };
	struct timeval		 timeout = { TIMEOUT_DEFAULT, 0 };
	socklen_t		 len = sizeof(ss);
	int			 s;

	if (g->ll_free == NULL) {
		event_del(&g->ev_accept);
		event_add(&g->ev_resume, &backoff);
		return;
	}
	if ((s = accept4(fd, (struct sockaddr *)&ss, &len,
	    SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0) {
		switch (errno) {
		case EINTR:
		case EWOULDBLOCK:
		case ECONNABORTED:
			return;
		case EMFILE:
		case ENFILE:
			event_del(&g->ev_accept);
			event_add(&g->ev_resume, &backoff);
			return;
		default:
			errx(1, "unknown error on accept: %d", errno);
		}
	}
	c = g->ll_free;
	g->ll_free = c->next;
	++g->num_connect;
	c->fd = s;
	c->fcgi_id = -1;
	c->param_len = 0;
	c->content_len = 0;
	c->compress = 0;
	c->lang = 0;
	c->in_pos = 0;
	c->in_len = 0;
	c->out_pos = 0;
	c->out_len = 0;
	c->status = STATUS_STARTED;
	event_set(&c->ev_read, s, EV_READ | EV_PERSIST, fastcgi_read, c);
	event_set(&c->ev_write, s, EV_WRITE | EV_PERSIST, fastcgi_write, c);
	event_set(&c->ev_timeout, s, 0, fastcgi_timeout, c);
	event_add(&c->ev_read, NULL);
	event_add(&c->ev_timeout, &timeout);
}

void
fastcgi_resume(int fd, short events, void *arg)
{
	struct myglobal	*g = arg;
	event_add(&g->ev_accept, NULL);
}

void
fastcgi_sig_handler(int sig, short event, void *arg)
{
	struct myglobal	*g = arg;
	switch (sig) {
	case SIGUSR1:
		pthread_mutex_lock(&g->m_jobdone);
		while (g->ll_jobdone) {
			struct myconnect *c = g->ll_jobdone;
			int old_status = c->status;
			g->ll_jobdone = c->next;
			c->status = STATUS_FINISHED;
			if (old_status == STATUS_CLOSING)
				fastcgi_close(c);
			else
				event_add(&c->ev_write, NULL);
		}
		pthread_mutex_unlock(&g->m_jobdone);
		break;
	case SIGPIPE:
		break;
	default:
		errx(1, "unknown signal: %d", sig);
	}
}

void
fastcgi_close(struct myconnect *c)
{
	struct myglobal	*g = c->g;
	int		 old_status;
	old_status = c->status;
	c->status = STATUS_CLOSING;
	if (old_status == STATUS_WORKING ||
	    old_status == STATUS_CLOSING)
		return;
	event_del(&c->ev_read);
	event_del(&c->ev_write);
	event_del(&c->ev_timeout);
	close(c->fd);
	c->next = g->ll_free;
	g->ll_free = c;
	--g->num_connect;
}

void
fastcgi_read(int fd, short event, void *arg)
{
	struct myconnect	*c = arg;
	ssize_t		 	 n;
	n = read(fd, c->in_buf + c->in_pos + c->in_len,
	    FCGI_RECORD_SIZE - c->in_pos - c->in_len);
	switch (n) {
	case -1:
		if (errno == EINTR || errno == EAGAIN)
			return;
	case 0:
		fastcgi_close(c);
		return;
	default:
		break;
	}
	c->in_len += n;
	while (c->in_len >= (int)sizeof(struct fcgi_record_header)) {
		struct fcgi_record_header *h;
		int		 data_len, total_len;
		char		*data;
		char		*buf = c->out_buf + c->out_len;
		int		 buf_len = c->out_len;

		if ((h = (struct fcgi_record_header *)
		    (c->in_buf + c->in_pos))->version != 1)
			errx(1, "unknown version: %d", h->version);
		data_len = ntohs(h->content_len);
		total_len = sizeof(*h) + data_len + h->padding_len;
		if (c->in_len < total_len)
			break;
		if (c->fcgi_id < 0)
			c->fcgi_id = ntohs(h->id);
		else if (c->fcgi_id != ntohs(h->id))
			errx(1, "fastcgi id changed");
		data = c->in_buf + c->in_pos + sizeof(*h);
		c->in_pos += total_len;
		c->in_len -= total_len;
		if (c->status != STATUS_STARTED)
			continue;
		switch (h->type) {
		case FCGI_BEGIN_REQUEST:
			c->param_len = 0;
			c->content_len = 0;
			break;
		case FCGI_PARAMS:
			if (data_len == 0) {
				c->param_len = c->out_len;
				if (!c->content_len)
					fastcgi_addjob(c);
				continue;
			}
			if (buf_len + data_len >= BUF_PER_CONNECT) {
				fastcgi_end_bigreq(c, NULL);
				c->status = STATUS_FINISHED;
				event_add(&c->ev_write, NULL);
				continue;
			}
			while (data_len > 0) {
				uint8_t *d = (uint8_t *)data;
				int name = FCGI_READ_INT(d);
				int val = FCGI_READ_INT(d);
				data_len -= (char *)d - data;
				data = (char *)d;
				if (data_len < (int64_t)name + val)
					break;
				data_len -= name + val;

				memcpy(buf, data, name);
				data += name;
				buf[name] = 0;
				buf += name = strlen(buf) + 1;

				memcpy(buf, data, val);
				data += val;
				buf[val] = 0;
				buf += val = strlen(buf) + 1;

				c->out_len += name += val;
				fastcgi_header(c, buf-name, buf-val);
			}
			break;
		case FCGI_STDIN:
			if (buf_len + data_len < BUF_PER_CONNECT) {
				memcpy(buf, data, data_len);
				if ((c->out_len += data_len) >=
				    c->param_len + c->content_len)
					fastcgi_addjob(c);
			} else {
				fastcgi_end_bigreq(c, NULL);
				c->status = STATUS_FINISHED;
				event_add(&c->ev_write, NULL);
			}
			break;
		default:
			break;
		}
	}
	if (c->in_len > 0)
		bcopy(c->in_buf + c->in_pos, c->in_buf, c->in_len);
	c->in_pos = 0;
}

void
fastcgi_write(int fd, short event, void *arg)
{
	struct myconnect	*c = arg;
	if (c->out_len > 0) {
		ssize_t n = write(fd, c->out_buf + c->out_pos, c->out_len);
		if (n < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return;
			fastcgi_close(c);
			return;
		}
		c->out_pos += n;
		c->out_len -= n;
	}
	if (c->out_len <= 0)
		fastcgi_close(c);
}

void
fastcgi_timeout(int fd, short event, void *arg)
{
	struct myconnect	*c = arg;
	fastcgi_close(c);
}

void *
fastcgi_worker(void *arg)
{
	struct mythread		*t = arg;
	struct myglobal		*g = t->g;
	for (;;) {
		struct myconnect *c;

		pthread_mutex_lock(&g->m_job);
		if (!g->ll_job)
			pthread_cond_wait(&g->cv_job, &g->m_job);
		c = g->ll_job;
		g->ll_job = c->next;
		pthread_mutex_unlock(&g->m_job);

		t->head_len = 0;
		t->body_len = 0;
		t->log_len = 0;
		c->out_len = 0;
		memcpy(t->in_buf, c->out_buf, sizeof(t->in_buf));
		t->content = t->in_buf + c->param_len;
		t->lang = c->lang;
		if (c->status != STATUS_CLOSING) {
			t->document_uri = fastcgi_param(t, "DOCUMENT_URI");
			t->http_accept = fastcgi_param(t, "HTTP_ACCEPT");
			t->http_cookie = fastcgi_param(t, "HTTP_COOKIE");
			t->http_host = fastcgi_param(t, "HTTP_HOST");
			t->http_referer = fastcgi_param(t, "HTTP_REFERER");
			t->http_user_agent = fastcgi_param(t, "HTTP_USER_AGENT");
			t->path_info = fastcgi_param(t, "PATH_INFO");
			t->query_string = fastcgi_param(t, "QUERY_STRING");
			t->remote_addr = fastcgi_param(t, "REMOTE_ADDR");
			t->remote_port = fastcgi_param(t, "REMOTE_PORT");
			t->request_method = fastcgi_param(t, "REQUEST_METHOD");
			t->request_uri = fastcgi_param(t, "REQUEST_URI");
			t->script_name = fastcgi_param(t, "SCRIPT_NAME");
			t->server_addr = fastcgi_param(t, "SERVER_ADDR");
			t->server_name = fastcgi_param(t, "SERVER_NAME");
			t->server_port = fastcgi_param(t, "SERVER_PORT");
			t->server_software = fastcgi_param(t, "SERVER_SOFTWARE");
			cgi_main(t, c);
		}
		c->next = NULL;
		pthread_mutex_lock(&g->m_jobdone);
		if (g->ll_jobdone)
			g->ll_last_jobdone->next = c;
		else
			g->ll_jobdone = c;
		g->ll_last_jobdone = c;
		pthread_mutex_unlock(&g->m_jobdone);
		pthread_kill(g->th_main, SIGUSR1);
	}
	return NULL;
}

void
fastcgi_addjob(struct myconnect *c)
{
	struct myglobal	*g = c->g;

	event_del(&c->ev_read);
	c->status = STATUS_WORKING;
	c->next = NULL;

	pthread_mutex_lock(&g->m_job);
	if (g->ll_job)
		g->ll_last_job->next = c;
	else
		g->ll_job = c;
	g->ll_last_job = c;
	pthread_cond_signal(&g->cv_job);
	pthread_mutex_unlock(&g->m_job);
}

const char *
fastcgi_param(struct mythread *t, const char *key)
{
	const char	*p = t->in_buf;
	while (p < t->content) {
		if (strcmp(p, key) == 0)
			return p + strlen(p) + 1;
		p += strlen(p) + 1;
		p += strlen(p) + 1;
	}
	return NULL;
}

void
fastcgi_header(struct myconnect *c, const char *name, const char *value)
{
#ifdef check
#undef check
#endif
#define check(s)	substr_eq(item, len, s)

	if (strcmp(name, "PATH_INFO") == 0) {
		const char *item = value;
		int lang, len = 0;
		if (*item && *item == '/')
			++item;
		while (item[len] && item[len] != '/')
			++len;
		for (lang = 0; lang < LANG_COUNT; ++lang)
			if (check(lang_code[lang]))
				break;
		if (lang < LANG_COUNT)
			c->lang = lang;
	} else if (strcmp(name, "CONTENT_LENGTH") == 0) {
		const char *e;
		int num = (int)strtonum(value, 0, BUF_PER_CONNECT, &e);
		if (!e)
			c->content_len = num;
	} else if (strcasecmp(name, "HTTP_ACCEPT_ENCODING") == 0) {
		const char *item = value;
		int methods = 0;
		for (;;) {
			int len = 0;
			while (*item && (*item <= ' ' || *item == ','))
				++item;
			if (!*item)
				break;
			while (item[len] && item[len] != ',')
				++len;
			while (len > 0 && item[len - 1] <= ' ')
				--len;
			if (check("gzip"))
				methods |= COMPRESS_GZIP;
			else if (check("deflate"))
				methods |= COMPRESS_DEFLATE;
			item += len;
		}
		c->compress = methods;
	}
}

int
fastcgi_addhead(struct mythread *t, const char *fmt, ...)
{
	va_list		 ap;
	int		 len;
	if (t->head_len < 0)
		return 0;
	len = sizeof(t->head) - t->head_len;
	if (len <= 1) {
		t->head_len = -1;
		return 0;
	}
	va_start(ap, fmt);
	len = vsnprintf(t->head + t->head_len, len, fmt, ap);
	va_end(ap);
	if (len > 0 && (t->head_len += len) >= (int)sizeof(t->head))
		t->head_len = -1;
	return len;
}

int
fastcgi_addbody(struct mythread *t, const char *fmt, ...)
{
	va_list		 ap;
	int		 len;
	if (t->body_len < 0)
		return 0;
	len = sizeof(t->body) - t->body_len;
	if (len <= 1) {
		t->body_len = -1;
		return 0;
	}
	va_start(ap, fmt);
	len = vsnprintf(t->body + t->body_len, len, fmt, ap);
	va_end(ap);
	if (len > 0 && (t->body_len += len) >= (int)sizeof(t->body))
		t->body_len = -1;
	return len;
}

int
fastcgi_addlog(struct mythread *t, const char *fmt, ...)
{
	va_list		 ap;
	int		 len;
	if (t->log_len < 0)
		return 0;
	len = sizeof(t->log) - t->log_len;
	if (len <= 1) {
		t->log_len = -1;
		return 0;
	}
	va_start(ap, fmt);
	len = vsnprintf(t->log + t->log_len, len, fmt, ap);
	va_end(ap);
	if (len > 0 && (t->log_len += len) >= (int)sizeof(t->log))
		t->log_len = -1;
	return len;
}


void
fastcgi_end(struct mythread *t, struct myconnect *c)
{
	if (c->status != STATUS_WORKING)
		return;
	if (t->log_len < 0)
		t->log[t->log_len = sizeof(t->log) - 1] = 0;
	if (t->head_len < 0 || t->body_len < 0) {
		fastcgi_end_bigres(c, t->log);
		return;
	}
	c->compress = t->body_len <= 0 ? 0 :
	    (c->compress & COMPRESS_GZIP) ? COMPRESS_GZIP :
	    /* (c->compress & COMPRESS_DEFLATE) ? COMPRESS_DEFLATE : */
	    0;
	switch (c->compress) {
	case COMPRESS_GZIP:
		t->gzip.next_in = t->body;
		t->gzip.avail_in = t->body_len;
		t->gzip.next_out = c->out_buf;
		t->gzip.avail_out = sizeof(c->out_buf);
		if (deflate(&t->gzip, Z_FINISH) == Z_STREAM_END)
			memcpy(t->body, c->out_buf, t->body_len =
			    (char *)t->gzip.next_out - c->out_buf);
		else
			c->compress = 0;
		deflateReset(&t->gzip);
		break;
	default:
		c->compress = 0;
		break;
	}
	if (t->body_len < 0) {
		fastcgi_end_bigres(c, t->log);
		return;
	}
	if (t->body_len > 0)
		fastcgi_addhead(t, "Content-Length: %d\n",
		    t->body_len);
	if (c->compress)
		fastcgi_addhead(t, "Content-Encoding: %s\n",
		    c->compress == COMPRESS_GZIP ? "gzip" :
		    c->compress == COMPRESS_DEFLATE ? "deflate" :
		    "unknown");
	fastcgi_addhead(t, "Pragma: no-cache\n"
	    "Cache-control: no-store\n"
	    "X-Frame-Options: SAMEORIGIN\n"
	    "Strict-Transport-Security: max-age=15768000\n"
	    "\n");
	if (t->head_len < 0 || t->body_len < 0) {
		fastcgi_end_bigres(c, t->log);
		return;
	}
	fastcgi_add_fcgi_multirecord(c, FCGI_STDERR, t->log, -1, 0, 0);
	fastcgi_add_fcgi_multirecord(c, FCGI_STDOUT,
	    t->head, t->head_len, t->body, t->body_len);
	fastcgi_add_fcgi_record(c, FCGI_END_REQUEST, NULL, 0);
}

void
fastcgi_end_internal(struct myconnect *c, const char *log,
    const char *adding_log, const char *fmt, ...)
{
	int		 len, L = c->lang;
	char		 header[4096], body[4096];
	va_list		 ap;

	c->out_len = 0;
	fastcgi_add_fcgi_multirecord(c, FCGI_STDERR,
	    log, -1, adding_log, -1);
	snprintf(body, sizeof(body),
	    "<!DOCTYPE html>\n"
	    "<html dir=\"%s\" lang=\"%s\">"
	    "<head>"
	    "<meta charset=\"utf-8\">"
	    "<meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">"
	    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
	    "<title>%s</title>"
	    "</head><body>"
	    "<h3>ERROR</h3>",
	    lang_dir[L],
	    lang_code[L],
	    str_error[L]);
	len = strlen(body);
	va_start(ap, fmt);
	vsnprintf(body + len, sizeof(body) - len, fmt, ap);
	va_end(ap);
	len = strlen(body);
	snprintf(body + len, sizeof(body) - len, "</body></html>");
	snprintf(header, sizeof(header),
	    "Status: %d\n"
	    "Content-Type: " MIME_HTML "\n"
	    "Content-Length: %d\n"
	    "\n",
	    500, (int)strlen(body));
	fastcgi_add_fcgi_multirecord(c, FCGI_STDOUT,
	    header, -1, body, -1);
	fastcgi_add_fcgi_record(c, FCGI_END_REQUEST, NULL, 0);
}

void
fastcgi_end_bigreq(struct myconnect *c, const char *log)
{
	int		 L = c->lang;
	fastcgi_end_internal(c, log, "bigreq\n",
	    "<h4>%s%s</h4>"
	    "<h4>%s%s</h4>",
	    str_your_request_is_too_big[L], lang_period[L],
	    str_please_compress_uploading_files_and_try_again[L],
	    lang_period[L]);
}

void
fastcgi_end_bigres(struct myconnect *c, const char *log)
{
	int		 L = c->lang;
	fastcgi_end_internal(c, log, "bigres\n",
	    "<h4>%s%s %s%s</h4>",
	    str_your_request_completed[L], lang_comma[L],
	    str_but_there_is_a_problem_in_displaying_results[L],
	    lang_period[L]);
}

int
fastcgi_add_fcgi_record(struct myconnect *c, int type,
    const void *content, int content_len)
{
	struct fcgi_record_header	*h;
	struct fcgi_end_request_body	 endreq;
	int				 rem;
	rem = FCGI_ALIGN_DOWN(sizeof(c->out_buf) - c->out_len);
	rem = rem - FCGI_ALIGN(sizeof(*h) + sizeof(endreq));
	if (type == FCGI_END_REQUEST) {
		if (rem < 0)
			return 0;
		endreq.app_status = htonl(0);
		endreq.protocol_status = FCGI_REQUEST_COMPLETE;
		endreq.reserved[0] = 0;
		endreq.reserved[1] = 0;
		endreq.reserved[2] = 0;
		content = &endreq;
		content_len = sizeof(endreq);
	} else {
		if (rem < FCGI_ALIGN((int)sizeof(*h) + 1))
			return 0;
		if (content_len >= (1 << 16))
			content_len = (1 << 16) - sizeof(*h);
		if (rem < FCGI_ALIGN((int)sizeof(*h) + content_len))
			content_len = rem - sizeof(*h);
	}
	h = (void *)(c->out_buf + c->out_len);
	h->version = 1;
	h->type = type;
	h->id = htons(c->fcgi_id);
	h->content_len = htons(content_len);
	h->padding_len = FCGI_ALIGN(sizeof(*h) + content_len) -
	    sizeof(*h);
	h->reserved = 0;
	c->out_len += sizeof(*h);
	if (content_len > 0 && content)
		memcpy(c->out_buf + c->out_len, content, content_len);
	c->out_len += content_len;
	if (h->padding_len > 0)
		memset(c->out_buf + c->out_len, 0, h->padding_len);
	c->out_len += h->padding_len;
	return content_len;
}

int
fastcgi_add_fcgi_multirecord(struct myconnect *c, int type,
    const char *content1, int len1, const char *content2, int len2)
{
	int		 t, res = 0;
	const int	 hsize = (int)sizeof(struct fcgi_record_header);
	const int	 bestlen = (1 << 16) - hsize;
	if (len1 < 0)
		len1 = content1 ? strlen(content1) : 0;
	if (len2 < 0)
		len2 = content2 ? strlen(content2) : 0;
	while (len1 >= bestlen) {
		t = fastcgi_add_fcgi_record(c, type, content1, bestlen);
		if (t <= 0)
			return res;
		res += t;
		content1 += t;
		len1 -= t;
	}
	if (len1 > 0) {
		char *p = c->out_buf + c->out_len + hsize;
		t = fastcgi_add_fcgi_record(c, type, NULL,
		    len1 + len2 < bestlen ? len1 + len2 : bestlen);
		if (t <= 0)
			return res;
		res += t;
		if (t < len1) {
			memcpy(p, content1, t);
			return res;
		}
		memcpy(p, content1, len1);
		p += len1;
		t -= len1;
		if (t > 0)
			memcpy(p, content2, t);
		content2 += t;
		len2 -= t;
		return res;
	}
	while (len2 > 0) {
		t = fastcgi_add_fcgi_record(c, type, content2, bestlen);
		if (t <= 0)
			return res;
		res += t;
		content2 += t;
		len2 -= t;
	}
	return res;
}

static int
substr_eq(const char *s1, int len1, const char *s2)
{
	int		 i;
	for (i = 0; i < len1; ++i)
		if (s1[i] != s2[i])
			return 0;
	return s2[len1] ? 0 : 1;
}

