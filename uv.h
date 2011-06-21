/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UV_H
#define UV_H
#ifdef __cplusplus
extern "C" {
#endif

#define UV_VERSION_MAJOR 0
#define UV_VERSION_MINOR 1

#define CARES_STATICLIB 1

#include <stdint.h> /* int64_t */
#include <sys/types.h> /* size_t */

#include "c-ares/ares.h"

#ifndef _SSIZE_T_
typedef intptr_t ssize_t;
#endif

#ifndef UV_MULTIPLICITY
# define UV_MULTIPLICITY 1
#endif

#if UV_MULTIPLICITY
  /* Support multiple event loops */
# define UV_P         uv_loop_t* __uv_loop
# define UV_P_        UV_P,
# define UV_A         __uv_loop
# define UV_A_        UV_A,
# define UV_DEFAULT   (&uv_default_loop)
# define UV_DEFAULT_  UV_DEFAULT,
# define UV_LOOP      __uv_loop
#else
  /* Default loop only */
# define UV_P         /* empty */
# define UV_P_        /* empty */
# define UV_A         /* empty */
# define UV_A_        /* empty */
# define UV_DEFAULT   /* empty */
# define UV_DEFAULT_  /* empty */
# define UV_LOOP      (&uv_default_loop)
#endif

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_err_s uv_err_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_prepare_s uv_prepare_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_idle_s uv_idle_t;
typedef struct uv_req_s uv_req_t;
typedef struct uv_async_s uv_async_t;

extern uv_loop_t uv_default_loop;


#if defined(__unix__) || defined(__POSIX__) || defined(__APPLE__)
# include "uv-unix.h"
#else
# include "uv-win.h"
#endif


/* The status parameter is 0 if the request completed successfully,
 * and should be -1 if the request was cancelled or failed.
 * For uv_close_cb, -1 means that the handle was closed due to an error.
 * Error details can be obtained by calling uv_last_error().
 *
 * In the case of uv_read_cb the uv_buf_t returned should be freed by the
 * user.
 */
typedef uv_buf_t (*uv_alloc_cb)(UV_P_ uv_tcp_t* tcp, size_t suggested_size);
typedef void (*uv_read_cb)(UV_P_ uv_tcp_t* tcp, ssize_t nread, uv_buf_t buf);
typedef void (*uv_write_cb)(UV_P_ uv_req_t* req, int status);
typedef void (*uv_connect_cb)(UV_P_ uv_req_t* req, int status);
typedef void (*uv_shutdown_cb)(UV_P_ uv_req_t* req, int status);
typedef void (*uv_connection_cb)(UV_P_ uv_tcp_t* server, int status);
typedef void (*uv_close_cb)(UV_P_ uv_handle_t* handle);
typedef void (*uv_timer_cb)(UV_P_ uv_timer_t* handle, int status);
/* TODO: do these really need a status argument? */
typedef void (*uv_async_cb)(UV_P_ uv_async_t* handle, int status);
typedef void (*uv_prepare_cb)(UV_P_ uv_prepare_t* handle, int status);
typedef void (*uv_check_cb)(UV_P_ uv_check_t* handle, int status);
typedef void (*uv_idle_cb)(UV_P_ uv_idle_t* handle, int status);


/* Expand this list if necessary. */
typedef enum {
  UV_UNKNOWN = -1,
  UV_OK = 0,
  UV_EOF,
  UV_EACCESS,
  UV_EAGAIN,
  UV_EADDRINUSE,
  UV_EADDRNOTAVAIL,
  UV_EAFNOSUPPORT,
  UV_EALREADY,
  UV_EBADF,
  UV_EBUSY,
  UV_ECONNABORTED,
  UV_ECONNREFUSED,
  UV_ECONNRESET,
  UV_EDESTADDRREQ,
  UV_EFAULT,
  UV_EHOSTUNREACH,
  UV_EINTR,
  UV_EINVAL,
  UV_EISCONN,
  UV_EMFILE,
  UV_ENETDOWN,
  UV_ENETUNREACH,
  UV_ENFILE,
  UV_ENOBUFS,
  UV_ENOMEM,
  UV_ENONET,
  UV_ENOPROTOOPT,
  UV_ENOTCONN,
  UV_ENOTSOCK,
  UV_ENOTSUP,
  UV_EPROTO,
  UV_EPROTONOSUPPORT,
  UV_EPROTOTYPE,
  UV_ETIMEDOUT
} uv_err_code;

typedef enum {
  UV_UNKNOWN_HANDLE = 0,
  UV_TCP,
  UV_NAMED_PIPE,
  UV_TTY,
  UV_FILE,
  UV_TIMER,
  UV_PREPARE,
  UV_CHECK,
  UV_IDLE,
  UV_ASYNC
} uv_handle_type;

typedef enum {
  UV_UNKNOWN_REQ = 0,
  UV_CONNECT,
  UV_ACCEPT,
  UV_READ,
  UV_WRITE,
  UV_SHUTDOWN,
  UV_WAKEUP
} uv_req_type;


struct uv_err_s {
  /* read-only */
  uv_err_code code;
  /* private */
  int sys_errno_;
};


struct uv_loop_s {
  UV_LOOP_PRIVATE_FIELDS
};


struct uv_req_s {
  /* read-only */
  uv_req_type type;
  /* public */
  uv_handle_t* handle;
  void* cb;
  void* data;
  /* private */
  UV_REQ_PRIVATE_FIELDS
};

/*
 * Initialize a request for use with uv_write, uv_shutdown, or uv_connect.
 */
void uv_req_init(UV_P_ uv_req_t* req, uv_handle_t* handle, void* cb);


#define UV_HANDLE_FIELDS \
  /* read-only */ \
  uv_handle_type type; \
  /* public */ \
  uv_close_cb close_cb; \
  void* data; \
  /* private */ \
  UV_HANDLE_PRIVATE_FIELDS \

/* The abstract base class of all handles.  */
struct uv_handle_s {
  UV_HANDLE_FIELDS
};

/*
 * Returns 1 if the prepare/check/idle handle has been started, 0 otherwise.
 * For other handle types this always returns 1.
 */
int uv_is_active(UV_P_ uv_handle_t* handle);

/*
 * Request handle to be closed. close_cb will be called asynchronously after
 * this call. This MUST be called on each handle before memory is released.
 */
int uv_close(UV_P_ uv_handle_t* handle, uv_close_cb close_cb);


/*
 * A subclass of uv_handle_t representing a TCP stream or TCP server. In the
 * future this will probably be split into two classes - one a stream and
 * the other a server.
 */
struct uv_tcp_s {
  UV_HANDLE_FIELDS
  size_t write_queue_size; /* number of bytes queued for writing */
  UV_TCP_PRIVATE_FIELDS
};

int uv_tcp_init(UV_P_ uv_tcp_t* handle);

int uv_bind(UV_P_ uv_tcp_t* handle, struct sockaddr_in);

int uv_connect(UV_P_ uv_req_t* req, struct sockaddr_in);

int uv_shutdown(UV_P_ uv_req_t* req);

int uv_listen(UV_P_ uv_tcp_t* handle, int backlog, uv_connection_cb cb);

/* This call is used in conjunction with uv_listen() to accept incoming TCP
 * connections. Call uv_accept after receiving a uv_connection_cb to accept
 * the connection. Before calling uv_accept use uv_tcp_init() must be
 * called on the client. Non-zero return value indicates an error.
 *
 * When the uv_connection_cb is called it is guaranteed that uv_accept will
 * complete successfully the first time. If you attempt to use it more than
 * once, it may fail. It is suggested to only call uv_accept once per
 * uv_connection_cb call.
 */
int uv_accept(UV_P_ uv_tcp_t* server, uv_tcp_t* client);

/* Read data from an incoming stream. The callback will be made several
 * several times until there is no more data to read or uv_read_stop is
 * called. When we've reached EOF nread will be set to -1 and the error is
 * set to UV_EOF. When nread == -1 the buf parameter might not point to a
 * valid buffer; in that case buf.len and buf.base are both set to 0.
 * Note that nread might also be 0, which does *not* indicate an error or
 * eof; it happens when libuv requested a buffer through the alloc callback
 * but then decided that it didn't need that buffer.
 */
int uv_read_start(UV_P_ uv_tcp_t*, uv_alloc_cb alloc_cb, uv_read_cb read_cb);

int uv_read_stop(UV_P_ uv_tcp_t*);

/* Write data to stream. Buffers are written in order. Example:
 *
 *   uv_buf_t a[] = {
 *     { .base = "1", .len = 1 },
 *     { .base = "2", .len = 1 }
 *   };
 *
 *   uv_buf_t b[] = {
 *     { .base = "3", .len = 1 },
 *     { .base = "4", .len = 1 }
 *   };
 *
 *   // writes "1234"
 *   uv_write(req, a, 2);
 *   uv_write(req, b, 2);
 *
 */
int uv_write(UV_P_ uv_req_t* req, uv_buf_t bufs[], int bufcnt);


/*
 * Subclass of uv_handle_t. libev wrapper. Every active prepare handle gets
 * its callback called exactly once per loop iteration, just before the
 * system blocks to wait for completed i/o.
 */
struct uv_prepare_s {
  UV_HANDLE_FIELDS
  UV_PREPARE_PRIVATE_FIELDS
};

int uv_prepare_init(UV_P_ uv_prepare_t* prepare);

int uv_prepare_start(UV_P_ uv_prepare_t* prepare, uv_prepare_cb cb);

int uv_prepare_stop(UV_P_ uv_prepare_t* prepare);


/*
 * Subclass of uv_handle_t. libev wrapper. Every active check handle gets
 * its callback called exactly once per loop iteration, just after the
 * system returns from blocking.
 */
struct uv_check_s {
  UV_HANDLE_FIELDS
  UV_CHECK_PRIVATE_FIELDS
};

int uv_check_init(UV_P_ uv_check_t* check);

int uv_check_start(UV_P_ uv_check_t* check, uv_check_cb cb);

int uv_check_stop(UV_P_ uv_check_t* check);


/*
 * Subclass of uv_handle_t. libev wrapper. Every active idle handle gets its
 * callback called repeatedly until it is stopped. This happens after all
 * other types of callbacks are processed.  When there are multiple "idle"
 * handles active, their callbacks are called in turn.
 */
struct uv_idle_s {
  UV_HANDLE_FIELDS
  UV_IDLE_PRIVATE_FIELDS
};

int uv_idle_init(UV_P_ uv_idle_t* idle);

int uv_idle_start(UV_P_ uv_idle_t* idle, uv_idle_cb cb);

int uv_idle_stop(UV_P_ uv_idle_t* idle);


/*
 * Subclass of uv_handle_t. libev wrapper. uv_async_send wakes up the event
 * loop and calls the async handle's callback There is no guarantee that
 * every uv_async_send call leads to exactly one invocation of the callback;
 * The only guarantee is that the callback function is  called at least once
 * after the call to async_send. Unlike all other libuv functions,
 * uv_async_send can be called from another thread.
 */
struct uv_async_s {
  UV_HANDLE_FIELDS
  UV_ASYNC_PRIVATE_FIELDS
};

int uv_async_init(UV_P_ uv_async_t* async, uv_async_cb async_cb);

int uv_async_send(uv_async_t* async);


/*
 * Subclass of uv_handle_t. Wraps libev's ev_timer watcher. Used to get
 * woken up at a specified time in the future.
 */
struct uv_timer_s {
  UV_HANDLE_FIELDS
  UV_TIMER_PRIVATE_FIELDS
};

int uv_timer_init(UV_P_ uv_timer_t* timer);

int uv_timer_start(UV_P_ uv_timer_t* timer, uv_timer_cb cb, int64_t timeout, int64_t repeat);

int uv_timer_stop(UV_P_ uv_timer_t* timer);

/*
 * Stop the timer, and if it is repeating restart it using the repeat value
 * as the timeout. If the timer has never been started before it returns -1 and
 * sets the error to UV_EINVAL.
 */
int uv_timer_again(UV_P_ uv_timer_t* timer);

/*
 * Set the repeat value. Note that if the repeat value is set from a timer
 * callback it does not immediately take effect. If the timer was nonrepeating
 * before, it will have been stopped. If it was repeating, then the old repeat
 * value will have been used to schedule the next timeout.
 */
void uv_timer_set_repeat(UV_P_ uv_timer_t* timer, int64_t repeat);

int64_t uv_timer_get_repeat(UV_P_ uv_timer_t* timer);


/*
 * Most functions return boolean: 0 for success and -1 for failure.
 * On error the user should then call uv_last_error() to determine
 * the error code.
 */
uv_err_t uv_last_error(UV_P);
char* uv_strerror(UV_P_ uv_err_t err);
const char* uv_err_name(UV_P_ uv_err_t err);

/* Initialize libuv. This also initializes the default event loop. */
void uv_init();

/* Event loops other than the default must be initialized before */
/* to being used. */
#if UV_MULTIPLICITY
  int uv_loop_init(UV_P);
#endif

int uv_run(UV_P);

/*
 * Manually modify the event loop's reference count. Useful if the user wants
 * to have a handle or timeout that doesn't keep the loop alive.
 */
void uv_ref(UV_P);
void uv_unref(UV_P);

void uv_update_time(UV_P);
int64_t uv_now(UV_P);


/* Utility */
struct sockaddr_in uv_ip4_addr(const char* ip, int port);

/* Gets the executable path */
int uv_get_exepath(UV_P_ char* buffer, size_t* size);

/* the presence of this union forces similar struct layout */
union uv_any_handle {
  uv_tcp_t tcp;
  uv_prepare_t prepare;
  uv_check_t check;
  uv_idle_t idle;
  uv_async_t async;
  uv_timer_t timer;
};

/* Diagnostic counters */
typedef struct {
  uint64_t req_init;
  uint64_t handle_init;
  uint64_t tcp_init;
  uint64_t prepare_init;
  uint64_t check_init;
  uint64_t idle_init;
  uint64_t async_init;
  uint64_t timer_init;
} uv_counters_t;

uv_counters_t* uv_counters(UV_P);

#ifndef	SEC
#define	SEC		1
#endif

#ifndef	MILLISEC
#define	MILLISEC	1000
#endif

#ifndef	MICROSEC
#define	MICROSEC	1000000
#endif

#ifndef	NANOSEC
#define	NANOSEC		1000000000
#endif

/*
 * Returns the current high-resolution real time. This is expressed in
 * nanoseconds. It is relative to an arbitrary time in the past. It is not
 * related to the time of day and therefore not subject to clock drift. The
 * primary use is for measuring performance between intervals.
 *
 * Note not every platform can support nanosecond resolution; however, this
 * value will always be in nanoseconds.
 */
extern uint64_t uv_get_hrtime(void);

#ifdef __cplusplus
}
#endif
#endif /* UV_H */
