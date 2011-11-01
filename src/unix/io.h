#include "uv.h"

typedef void (*uv_io_cb)(EV_P_ ev_io* watcher, int revents);

void uv__io_watcher_start(
    uv_handle_t* handle,
    uv_io_t* io,
    ev_io* w,
    uv_io_cb cb);

void uv__io_watcher_stop(uv_handle_t* handle, uv_io_t* io, ev_io* w);

#if 0
void uv__io_init_read(
    uv_io_t* io,
    int fd);

void uv__io_start_read(uv_loop_t* loop, uv_io_t* io);
void uv__io_start_write(uv_loop_t* loop, uv_io_t* io);
#endif
