#ifndef PADSOCKET_H
#define PADSOCKET_H

#include "pad.h"
#include <errno.h>

#ifdef WIN32
	//Each app must set FD_SETSIZE to required value before including padsocket.h
	//#define FD_SETSIZE 128 /* MAX Num sockets for select */
	//else default (of 64) used.

	#include <winsock.h>

	/* Following defines are actually commented out in winsock.h cos they conflict win errno.h on NT ? */
	#define EWOULDBLOCK             WSAEWOULDBLOCK
	#define EINPROGRESS             WSAEINPROGRESS
	#define EALREADY                WSAEALREADY
	#define ENOTSOCK                WSAENOTSOCK
	#define EDESTADDRREQ            WSAEDESTADDRREQ
	#define EMSGSIZE                WSAEMSGSIZE
	#define EPROTOTYPE              WSAEPROTOTYPE
	#define ENOPROTOOPT             WSAENOPROTOOPT
	#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
	#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
	#define EOPNOTSUPP              WSAEOPNOTSUPP
	#define EPFNOSUPPORT            WSAEPFNOSUPPORT
	#define EAFNOSUPPORT            WSAEAFNOSUPPORT
	#define EADDRINUSE              WSAEADDRINUSE
	#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
	#define ENETDOWN                WSAENETDOWN
	#define ENETUNREACH             WSAENETUNREACH
	#define ENETRESET               WSAENETRESET
	#define ECONNABORTED            WSAECONNABORTED
	#define ECONNRESET              WSAECONNRESET
	#define ENOBUFS                 WSAENOBUFS
	#define EISCONN                 WSAEISCONN
	#define ENOTCONN                WSAENOTCONN
	#define ESHUTDOWN               WSAESHUTDOWN
	#define ETOOMANYREFS            WSAETOOMANYREFS
	#undef  ETIMEDOUT /* pthreads.h from cygnus defines this as arbitrary value (make
                      sure don't mix this & pthreads code*/
	#define ETIMEDOUT               WSAETIMEDOUT
	#define ECONNREFUSED            WSAECONNREFUSED
	#define ELOOP                   WSAELOOP
	//#define ENAMETOOLONG            WSAENAMETOOLONG
	#define EHOSTDOWN               WSAEHOSTDOWN
	#define EHOSTUNREACH            WSAEHOSTUNREACH
	//#define ENOTEMPTY               WSAENOTEMPTY
	#define EPROCLIM                WSAEPROCLIM
	#define EUSERS                  WSAEUSERS
	#define EDQUOT                  WSAEDQUOT
	#define ESTALE                  WSAESTALE
	#define EREMOTE                 WSAEREMOTE

#else //POSIX
    #include <netinet/tcp.h>
    #include <netinet/ip.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    typedef int SOCKET;
#endif

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned. This taken from
 * winsock.h incase not used on POSIX
 */
#undef	INVALID_SOCKET
#define INVALID_SOCKET  (SOCKET)(~0)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int readn(register SOCKET fd, register char* ptr, register int nbytes);
int writen(register SOCKET fd, register char* ptr, register int nbytes);
unsigned long int getIPAddress(const char* peerName, char* IPaddress, const int bufSize);
unsigned long int getMyIPAddress(struct sockaddr* peer, char* IPaddress, const int bufSize);
int getSocketSysError(void);
int getSocketSysGenericError(int errornum);
int getSocketError(SOCKET s);

typedef enum
{
    tconnect_SUCCESS,
    tconnect_TIMEOUT,
    tconnect_CONNECT_FAILED,
    tconnect_ERROR,
    tconnect_UNKNOWN
} tconnect_t;

tconnect_t tconnect(SOCKET s, struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize);
tconnect_t thalf_connect(struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize);

/* following are useful when using function pointers to select between tconnect and thalf_connect.
   usage is something like:
   tconnect_func_t tconnect_func;
   type == stealth ? tconnect_func = thalf_connect_wrapper: tconnect_func = tconnect */

typedef tconnect_t (*tconnect_func_t)(int,struct sockaddr*,int,long,char*,int);
tconnect_t thalf_connect_wrapper(int s, struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize);

STATUS_t setBlocking(SOCKET s, BOOL_t state, BOOL_t* changeRequired, char* errMsg, int errMsgSize);

typedef enum
{
	IsReady_YES,
	IsReady_NO,
	IsReady_ERROR
} IsReady_t;

typedef enum
{
	socketEvent_READ=		1,
	socketEvent_WRITE=		2,
	socketEvent_EXCEPTION=	4
} socketEvent_t;

IsReady_t IsSocketReady(SOCKET s, socketEvent_t forWhat, long timeout, char* errMsg, int errMsgSize);

#ifdef WIN32
typedef enum
{
	FIONBIO_BLOCKING = 0,
	FIONBIO_NONBLOCKING
}FIONBIO_t;
#endif //WIN32

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //PADSOCKET_H
