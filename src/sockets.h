#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCKET_ECONNABORTED WSAECONNABORTED

typedef SOCKET socket_t;

static inline int socket_error(void) { return WSAGetLastError(); }

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>

#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_ECONNABORTED ECONNABORTED

#define ioctlsocket ioctl
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

typedef int socket_t;

static inline int socket_error(void) { return errno; }

#endif
