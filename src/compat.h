// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_COMPAT_H
#define SOTERIA_COMPAT_H

// ============================================================================
// This matters because the Win32 SDK also declares a global
// `typedef unsigned char byte`. Without a rename, any translation unit that
// sees both ends up with an ambiguity when `using namespace std;` is in
// effect (which several headers use).
// ============================================================================
#include <cstddef>
#include <type_traits>

#if defined(HAVE_CONFIG_H)
#include <config/soteria-config.h>
#endif

// ============================================================================
// Platform-specific setup
// ============================================================================
#ifdef WIN32

// Prevent windows.h from defining min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Rename Win32 `byte` -> `win_byte` for the duration of the Windows headers.
// push_macro / pop_macro ensures the rename is scoped and cannot leak.
#pragma push_macro("byte")
#define byte win_byte

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdint.h>

#pragma pop_macro("byte")

#else // not WIN32

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>

#endif // WIN32

// ============================================================================
// Socket abstractions
// ============================================================================
#ifndef WIN32
typedef unsigned int SOCKET;
#include <errno.h>
#define WSAGetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEALREADY         EALREADY
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define WSAENOTSOCK         EBADF
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#endif

// ============================================================================
// Common defines
// ============================================================================
#ifndef PRIO_MAX
#define PRIO_MAX 20
#endif
#ifndef THREAD_PRIORITY_LOWEST
#define THREAD_PRIORITY_LOWEST          PRIO_MAX
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
#define THREAD_PRIORITY_BELOW_NORMAL    2
#endif
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL          0
#endif
#ifndef THREAD_PRIORITY_ABOVE_NORMAL
#define THREAD_PRIORITY_ABOVE_NORMAL    (-2)
#endif

#ifdef WIN32
#ifndef S_IRUSR
#define S_IRUSR             0400
#define S_IWUSR             0200
#endif
#else
#define MAX_PATH            1024
#endif

#ifdef _MSC_VER
#if !defined(ssize_t)
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
#endif
#endif

#if HAVE_DECL_STRNLEN == 0
size_t strnlen( const char *start, size_t max_len);
#endif

#ifndef WIN32
typedef void* sockopt_arg_type;
#else
typedef char* sockopt_arg_type;
#endif

bool static inline IsSelectableSocket(const SOCKET& s) {
#ifdef WIN32
    return true;
#else
    return (s < FD_SETSIZE);
#endif
}

#endif // SOTERIA_COMPAT_H
