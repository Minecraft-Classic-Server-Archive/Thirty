// classicserver, a ClassiCube (Minecraft Classic) server
// Copyright (C) 2024 Sean Baggaley
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCKET_ECONNABORTED WSAECONNABORTED
#define SOCKET_ECONNRESET WSAECONNRESET

typedef SOCKET socket_t;

static inline int socket_error(void) { return WSAGetLastError(); }

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_ECONNABORTED ECONNABORTED
#define SOCKET_ECONNRESET ECONNRESET

#define ioctlsocket ioctl
#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

typedef int socket_t;

static inline int socket_error(void) { return errno; }

#endif
