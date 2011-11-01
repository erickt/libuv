#include "io.h"

#include <assert.h>

void uv__io_watcher_start(
    uv_handle_t* handle,
    uv_io_t* io,
    ev_io* w,
    uv_io_cb cb) {
  int flags;

  assert(w == &io->read_watcher || w == &io->write_watcher);
  flags = (w == &io->read_watcher ? EV_READ : EV_WRITE);

  w->data = handle;
  ev_set_cb(w, cb);
  ev_io_set(w, handle->fd, flags);
  ev_io_start(handle->loop->ev, w);
}


void uv__io_watcher_stop(uv_handle_t* handle, uv_io_t* io, ev_io* w) {
  int flags;

  assert(w == &io->read_watcher || w == &io->write_watcher);
  flags = (w == &io->read_watcher ? EV_READ : EV_WRITE);

  ev_io_stop(handle->loop->ev, w);
  ev_io_set(w, -1, flags);
  ev_set_cb(w, NULL);
  w->data = (void*)0xDEADBABE;
}

#if 0
void uv__io_init_read(
    uv_io_t* io,
    void (*cb)(EV_P_ ev_io* watcher, int revents),
    int fd) {
  ev_io_init(&io->read_watcher, cb, fd, EV_READ);
}

void uv__io_start_read(uv_loop_t* loop, uv_io_t* io) {
  ev_io_start(loop->ev, &io->read_watcher);
}

void uv__io_start_write(uv_loop_t* loop, uv_io_t* io) {
  ev_io_start(loop->ev, &io->write_watcher);
}

void uv__io_init(
    uv_io_t* io,
    void (*cb)(ev_loop *loop, ev_TYPE *watcher, int revents),
    void *data) {
  ngx_queue_init(&io->write_queue);
  ngx_queue_init(&io->write_completed_queue);
  io->write_queue_size = 0;

  ev_init(&io->read_watcher, cb);
  io->read_watcher.data = data;

  ev_init(&io->write_watcher, cb);
  io->write_watcher.data = data;

  assert(ngx_queue_empty(&io->write_queue));
  assert(ngx_queue_empty(&io->write_completed_queue));
  assert(io->write_queue_size == 0);
}


void uv__io_open(uv_io_t* io, int fd) {
  assert(fd >= 0);

  /* Associate the fd with each ev_io watcher. */
  ev_io_set(io->read_watcher, fd, EV_READ);
  ev_io_set(io->write_watcher, fd, EV_WRITE);
}


void uv__io_destroy(uv_io_t* io) {
  while (!ngx_queue_empty(&io->write_queue)) {
    q = ngx_queue_head(&io->write_queue);
    ngx_queue_remove(q);

    req = ngx_queue_data(q, uv_write_t, queue);
    if (req->addr) {
      free(req->addr);
    }

    if (req->bufs != req->bufsml) {
      free(req->bufs);
    }

    if (req->cb) {
      uv__set_artificial_error(req->handle->loop, UV_EINTR);
      req->cb(req, -1);
    }
  }

  while (!ngx_queue_empty(&io->write_completed_queue)) {
    q = ngx_queue_head(&io->write_completed_queue);
    ngx_queue_remove(q);

    req = ngx_queue_data(q, uv_write_t, queue);
    if (req->cb) {
      uv__set_artificial_error(io->loop, req->error);
      req->cb(req, req->error ? -1 : 0);
    }
  }
}


void uv__server_io(EV_P_ ev_io* watcher, int revents) {
}


static uv_write_t* uv__write_queue_head(uv_io_t* io) {
  ngx_queue_t* q;
  uv_write_t* req;

  if (ngx_queue_empty(&io->write_queue)) {
    return NULL;
  }

  q = ngx_queue_head(&io->write_queue);
  if (!q) {
    return NULL;
  }

  req = ngx_queue_data(q, struct uv_write_s, queue);
  assert(req);

  return req;
}


static void uv__drain(uv_io_t* io, int flags) {
  uv_shutdown_t* req;

  assert(!uv__write_queue_head(io));
  assert(io->write_queue_size == 0);

  ev_io_stop(io->loop->ev, &io->write_watcher);

  /* Shutdown? */
  if ((flags & UV_SHUTTING) &&
      !(flags & UV_CLOSING) &&
      !(flags & UV_SHUT)) {
    assert(io->shutdown_req);

    req = io->shutdown_req;

    if (shutdown(io->fd, SHUT_WR)) {
      /* Error. Report it. User should call uv_close(). */
      uv__set_sys_error(io->loop, errno);
      if (req->cb) {
        req->cb(req, -1);
      }
    } else {
      uv__set_sys_error(io->loop, 0);
      ((uv_handle_t*) io)->flags |= UV_SHUT;
      if (req->cb) {
        req->cb(req, 0);
      }
    }
  }
}


static size_t uv__write_req_size(uv_write_t* req) {
  size_t size;

  size = uv__buf_count(req->bufs + req->write_index,
                       req->bufcnt - req->write_index);
  assert(req->handle->write_queue_size >= size);

  return size;
}


static void uv__write_req_finish(uv_write_t* req) {
  uv_io_t* io = req->handle;

  /* Pop the req off tcp->write_queue. */
  ngx_queue_remove(&req->queue);
  if (req->bufs != req->bufsml) {
    free(req->bufs);
  }
  req->bufs = NULL;

  /* Add it to the write_completed_queue where it will have its
   * callback called in the near future.
   */
  ngx_queue_insert_tail(&io->write_completed_queue, &req->queue);
  ev_feed_event(io->loop->ev, &io->write_watcher, EV_WRITE);
}
#endif
