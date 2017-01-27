#ifndef __FCGI_H__
#define __FCGI_H__

#include <stdint.h>
#include <event.h>
#include <pthread.h>
#include <zlib.h>

#define SOCKET_OWNER		"www"
#define DEFAULT_USER		"www"
#define DEFAULT_SOCKET		"/var/www/run/ali.frad.ir"

#define MAX_THREADS		64
#define TIMEOUT_DEFAULT		120

#define BUF_PER_CONNECT		(440 << 10)
#define BUF_PER_THREAD		(512 << 10)
#define FD_PER_CONNECT		1
#define FD_PER_THREAD		2
#define FD_RESERVED		15

#define FCGI_CONTENT_SIZE	65535
#define FCGI_PADDING_SIZE	255
#define FCGI_RECORD_SIZE	(sizeof(struct fcgi_record_header) + \
    FCGI_CONTENT_SIZE + FCGI_PADDING_SIZE + 10)

#define STATUS_STARTED		0 /* request is not complete */
#define STATUS_WORKING		1 /* working on request */
#define STATUS_FINISHED		2 /* response generated */
#define STATUS_CLOSING		3 /* wait for worker and then close */

#define COMPRESS_GZIP		1
#define COMPRESS_DEFLATE	2

#define MAGIC_PNG		(0x89504e47)

#define MIME_HTML		"text/html"
#define MIME_PNG		"image/png"
#define MIME_JPEG		"image/jpeg"

struct fcgi_record_header {
	uint8_t		 version;
	uint8_t		 type;
	uint16_t	 id;
	uint16_t	 content_len;
	uint8_t		 padding_len;
	uint8_t		 reserved;
}__packed;

struct mythread {
	int		 content_len;
	int		 head_len;
	int		 body_len;
	int		 log_len;
	int		 lang;
	struct myglobal	*g;
	const char	*document_uri;
	const char	*http_accept;
	const char	*http_cookie;
	const char	*http_host;
	const char	*http_referer;
	const char	*http_user_agent;
	const char	*path_info;
	const char	*query_string;
	const char	*remote_addr;
	const char	*remote_port;
	const char	*request_method;
	const char	*request_uri;
	const char	*script_name;
	const char	*server_addr;
	const char	*server_name;
	const char	*server_port;
	const char	*server_software;
	const char	*content;
	pthread_t	 thread;
	z_stream	 gzip;
	char		 base_path[64 << 10];
	char		 in_buf[BUF_PER_CONNECT];
	char		 head[64 << 10];
	char		 log[64 << 10];
	char		 body[BUF_PER_THREAD];
};

struct myconnect {
	struct myconnect*next;
	int		 fd;
	int		 fcgi_id;
	int		 param_len;
	int		 content_len;
	int		 compress;
	int		 lang;
	int		 in_pos;
	int		 in_len;
	int		 out_pos;
	int		 out_len;
	volatile int	 status;
	struct myglobal	*g;
	struct event	 ev_read;
	struct event	 ev_write;
	struct event	 ev_timeout;
	char		 in_buf[FCGI_RECORD_SIZE];
	char		 out_buf[BUF_PER_CONNECT];
};

struct myglobal {
	int		 num_thread;
	int		 max_connect;
	volatile int	 num_connect;
	struct myconnect *ll_free;
	struct myconnect *volatile ll_jobdone;
	struct myconnect *volatile ll_last_jobdone;
	struct myconnect *volatile ll_job;
	struct myconnect *volatile ll_last_job;
	pthread_t	 th_main;
	pthread_mutex_t	 m_jobdone;
	pthread_mutex_t	 m_job;
	pthread_cond_t	 cv_job;
	struct event	 ev_accept;
	struct event	 ev_resume;
	struct event	 ev_sigterm;
	struct event	 ev_sigint;
	struct event	 ev_sigpipe;
	struct event	 ev_sigusr1;
	struct mythread *thread;
	struct myconnect *connect;
};

#define cgi_binary(t, r)	cgi_binary2(t, r, sizeof(r))
#define cgi_base64(t, r)	cgi_base64_2(t, r, sizeof(r))
#define cgi_image(t, r)		cgi_image2(t, r, sizeof(r))

const char	*fastcgi_param(struct mythread *, const char *);
int		 fastcgi_addhead(struct mythread *, const char *, ...);
int		 fastcgi_addbody(struct mythread *, const char *, ...);
int		 fastcgi_addlog(struct mythread *, const char *, ...);
void		 fastcgi_trim_end(struct mythread *);
void		 fastcgi_end(struct mythread *, struct myconnect *);
void		 cgi_main(struct mythread *, struct myconnect *);
int		 cgi_path(const char **, const char *);
void		 cgi_html_begin(struct mythread *, const char *,
		    const char *, ...);
void		 cgi_html_head(struct mythread *);
void		 cgi_html_tail(struct mythread *);
void		 cgi_notfound(struct mythread *);
void		 cgi_binary2(struct mythread *, const void *, int);
void		 cgi_base64_2(struct mythread *, const void *, int);
void		 cgi_image2(struct mythread *, const void *, int);
void		 www(struct mythread *, const char *);
void		 www_os(struct mythread *, const char *);
void		 www_games(struct mythread *, const char *);
void		 www_games_dhewm3(struct mythread *, const char *);
void		 www_games_openmw(struct mythread *, const char *);
void		 www_games_supertuxkart(struct mythread *, const char *);
void		 www_utils(struct mythread *, const char *);

#endif
