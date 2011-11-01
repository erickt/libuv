#include "io.h"

#include <assert.h>


void uv__io_init(
    uv_io_t* io,
    uv_io_write_cb write_completed_cb,
    uv_io_write_cb write_destroy_cb) {
  io->write_completed_cb = write_completed_cb;
  io->write_destroy_cb = write_destroy_cb;
}

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


void uv__io_destroy(uv_handle_t* handle, uv_io_t* io) {
  ngx_queue_t* q;

  /* Error out all the unsent errors. */
  while (!ngx_queue_empty(&io->write_queue)) {
    q = ngx_queue_head(&io->write_queue);
    ngx_queue_remove(q);
    io->write_destroy_cb(handle, q);
  }

  /* Flush all the received requests. */
  while (!ngx_queue_empty(&io->write_completed_queue)) {
    q = ngx_queue_head(&io->write_completed_queue);
    ngx_queue_remove(q);
    io->write_completed_cb(handle, q);
  }
}
