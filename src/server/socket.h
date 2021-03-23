#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0501
#endif // WINVER
#include <ws2tcpip.h>
#include <winsock2.h>
#else
//translate windows functions to linux functions
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR	(-1)
#define closesocket close
#define SOCKADDR_IN sockaddr_in
#define ZeroMemory(d,l) memset((d), 0, (l))
#define ioctlsocket ioctl
#ifndef SA_INTERRUPT
#define SA_INTERRUPT 0 //ignore this setting
#endif
#define SD_BOTH SHUT_RDWR
#ifndef __hpux
#include <sys/select.h>
#endif /* __hpux */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
typedef sockaddr *LPSOCKADDR;
#endif // _WIN32

#endif // SOCKET_H_INCLUDED
