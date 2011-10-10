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

#include "uv.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_HANDLE(handle) \
  ASSERT((uv_udp_t*)(handle) == &server || (uv_udp_t*)(handle) == &client)

static uv_udp_t server;
static uv_udp_t client;

static int cl_writeto_cb_called;
static int cl_readfrom_cb_called;

static int sv_writeto_cb_called;
static int sv_readfrom_cb_called;

static int close_cb_called;


static uv_buf_t alloc_cb(uv_handle_t* handle, size_t suggested_size) {
  static char slab[65536];

  CHECK_HANDLE(handle);
  ASSERT(suggested_size <= sizeof slab);

  return uv_buf_init(slab, sizeof slab);
}


static void close_cb(uv_handle_t* handle) {
  CHECK_HANDLE(handle);
  close_cb_called++;
}


static void cl_readfrom_cb(uv_stream_t* handle,
                           ssize_t nread,
                           uv_buf_t buf,
                           struct sockaddr* addr,
                           size_t addrlen,
                           unsigned flags) {
  CHECK_HANDLE(handle);
  ASSERT(flags == 0);

  if (nread < 0) {
    ASSERT(0 && "unexpected error");
  }

  if (nread == 0) {
    /* Returning unused buffer */
    /* Don't count towards cl_readfrom_cb_called */
    ASSERT(addr == NULL);
    return;
  }

  ASSERT(addr != NULL);
  ASSERT(nread == 4);
  ASSERT(!memcmp("PONG", buf.base, nread));

  cl_readfrom_cb_called++;

  uv_close((uv_handle_t*) handle, close_cb);
}


static void cl_writeto_cb(uv_write_t* req, int status) {
  int r;

  ASSERT(req != NULL);
  ASSERT(status == 0);
  CHECK_HANDLE(req->handle);

  r = uv_readfrom_start(req->handle, alloc_cb, cl_readfrom_cb);
  ASSERT(r == 0);

  cl_writeto_cb_called++;
}


static void sv_writeto_cb(uv_write_t* req, int status) {
  ASSERT(req != NULL);
  ASSERT(status == 0);
  CHECK_HANDLE(req->handle);

  uv_close((uv_handle_t*) req->handle, close_cb);
  free(req);

  sv_writeto_cb_called++;
}


static void sv_readfrom_cb(uv_stream_t* handle,
                           ssize_t nread,
                           uv_buf_t buf,
                           struct sockaddr* addr,
                           size_t addrlen,
                           unsigned flags) {
  uv_write_t* req;
  int r;

  if (nread < 0) {
    ASSERT(0 && "unexpected error");
  }

  if (nread == 0) {
    /* Returning unused buffer */
    /* Don't count towards sv_readfrom_cb_called */
    ASSERT(addr == NULL);
    return;
  }

  CHECK_HANDLE(handle);
  ASSERT(flags == 0);

  ASSERT(addr != NULL);
  ASSERT(nread == 4);
  ASSERT(!memcmp("PING", buf.base, nread));

  /* FIXME? `uv_udp_readfrom_stop` does what it says: readfrom_cb is not called
    * anymore. That's problematic because the read buffer won't be returned
    * either... Not sure I like that but it's consistent with `uv_read_stop`.
    */
  r = uv_read_stop((uv_stream_t*)handle);
  ASSERT(r == 0);

  req = malloc(sizeof *req);
  ASSERT(req != NULL);

  buf = uv_buf_init("PONG", 4);

  r = uv_writeto(req,
                 (uv_stream_t*)handle,
                 &buf,
                 1,
                 addr,
                 addrlen,
                 sv_writeto_cb);
  ASSERT(r == 0);

  sv_readfrom_cb_called++;
}


TEST_IMPL(udp_writeto_and_readfrom) {
  struct sockaddr_in addr;
  uv_write_t req;
  uv_buf_t buf;
  int r;

  addr = uv_ip4_addr("0.0.0.0", TEST_PORT);

  r = uv_udp_init(uv_default_loop(), &server);
  ASSERT(r == 0);

  r = uv_udp_bind(&server, addr, 0);
  ASSERT(r == 0);

  r = uv_readfrom_start((uv_stream_t*)&server, alloc_cb, sv_readfrom_cb);
  ASSERT(r == 0);

  addr = uv_ip4_addr("127.0.0.1", TEST_PORT);

  r = uv_udp_init(uv_default_loop(), &client);
  ASSERT(r == 0);

  /* client writetos "PING", expects "PONG" */
  buf = uv_buf_init("PING", 4);

  r = uv_writeto(&req,
                 (uv_stream_t*)&client,
                 &buf,
                 1,
                 (struct sockaddr*)&addr,
                 sizeof(addr),
                 cl_writeto_cb);
  ASSERT(r == 0);

  ASSERT(close_cb_called == 0);
  ASSERT(cl_writeto_cb_called == 0);
  ASSERT(cl_readfrom_cb_called == 0);
  ASSERT(sv_writeto_cb_called == 0);
  ASSERT(sv_readfrom_cb_called == 0);

  uv_run(uv_default_loop());

  ASSERT(cl_writeto_cb_called == 1);
  ASSERT(cl_readfrom_cb_called == 1);
  ASSERT(sv_writeto_cb_called == 1);
  ASSERT(sv_readfrom_cb_called == 1);
  ASSERT(close_cb_called == 2);

  return 0;
}
