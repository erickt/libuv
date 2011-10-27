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
#include "internal.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


static int uv__bind(uv_udp_t* handle,
                    int domain,
                    struct sockaddr* addr,
                    socklen_t len,
                    unsigned flags) {
  int saved_errno;
  int status;
  int yes;

  saved_errno = errno;
  status = -1;

  /* Check for bad flags. */
  if (flags & ~UV_UDP_IPV6ONLY) {
    uv__set_sys_error(handle->loop, EINVAL);
    goto out;
  }

  /* Cannot set IPv6-only mode on non-IPv6 socket. */
  if ((flags & UV_UDP_IPV6ONLY) && domain != AF_INET6) {
    uv__set_sys_error(handle->loop, EINVAL);
    goto out;
  }

  /* Check for already active socket. */
  if (handle->fd != -1) {
    uv__set_artificial_error(handle->loop, UV_EALREADY);
    goto out;
  }

  if ((handle->fd = uv__socket(domain, SOCK_DGRAM, 0)) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  if (uv__stream_open((uv_stream_t*)handle, handle->fd, UV_READABLE | UV_WRITABLE)) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  if (flags & UV_UDP_IPV6ONLY) {
#ifdef IPV6_V6ONLY
    yes = 1;
    if (setsockopt(handle->fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof yes) == -1) {
      uv__set_sys_error(handle->loop, errno);
      goto out;
    }
#else
    uv__set_sys_error((uv_handle_t*)handle, ENOTSUP);
    goto out;
#endif
  }

  if (bind(handle->fd, addr, len) == -1) {
    uv__set_sys_error(handle->loop, errno);
    goto out;
  }

  status = 0;

out:
  if (status)
    uv__close(handle->fd);

  errno = saved_errno;
  return status;
}


int uv_udp_init(uv_loop_t* loop, uv_udp_t* handle) {
  uv__stream_init(loop, (uv_stream_t*)handle, UV_UDP);
  loop->counters.udp_init++;
  return 0;
}

int uv__udp_bind(uv_udp_t* handle, struct sockaddr_in addr, unsigned flags) {
  return uv__bind(handle,
                  AF_INET,
                  (struct sockaddr*)&addr,
                  sizeof addr,
                  flags);
}


int uv__udp_bind6(uv_udp_t* handle, struct sockaddr_in6 addr, unsigned flags) {
  return uv__bind(handle,
                  AF_INET6,
                  (struct sockaddr*)&addr,
                  sizeof addr,
                  flags);
}


int uv_udp_set_membership(uv_udp_t* handle, const char* multicast_addr,
  const char* interface_addr, uv_membership membership) {

  int optname;
  struct ip_mreq mreq;
  memset(&mreq, 0, sizeof mreq);

  if (interface_addr) {
    mreq.imr_interface.s_addr = inet_addr(interface_addr);
  } else {
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  }

  mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);

  switch (membership) {
  case UV_JOIN_GROUP:
    optname = IP_ADD_MEMBERSHIP;
    break;
  case UV_LEAVE_GROUP:
    optname = IP_DROP_MEMBERSHIP;
    break;
  default:
    uv__set_sys_error(handle->loop, EFAULT);
    return -1;
  }

  if (setsockopt(handle->fd, IPPROTO_IP, optname, (void*) &mreq, sizeof mreq) == -1) {
    uv__set_sys_error(handle->loop, errno);
    return -1;
  }

  return 0;
}


int uv_udp_getsockname(uv_udp_t* handle, struct sockaddr* name,
    int* namelen) {
  socklen_t socklen;
  int saved_errno;
  int rv = 0;

  /* Don't clobber errno. */
  saved_errno = errno;

  if (handle->fd < 0) {
    uv__set_sys_error(handle->loop, EINVAL);
    rv = -1;
    goto out;
  }

  /* sizeof(socklen_t) != sizeof(int) on some systems. */
  socklen = (socklen_t)*namelen;

  if (getsockname(handle->fd, name, &socklen) == -1) {
    uv__set_sys_error(handle->loop, errno);
    rv = -1;
  } else {
    *namelen = (int)socklen;
  }

out:
  errno = saved_errno;
  return rv;
}
