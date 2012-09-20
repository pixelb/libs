#include "PadSocket.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pad.h"
#include "bitops.h"

#ifndef WIN32
	#include <fcntl.h>
	#include <sys/time.h>
#endif

/* A checksum function from UC Berkely... */
unsigned short in_cksum (unsigned short *addr,int len);

/*
 * This is a "safe" version of read. The standard read
 * may return before all the requested bytes are read because
 * the internal buffer (4096 bytes for a pipe for e.g. in some UNIX')
 * has filled.
 *
 * This may also occur if the "message" you're reading has been
 * packetised by the originating or intermediary TCP/IP stacks, so
 * that depending on timing the recv() could return with just the
 * first part of the message (as when the first full TCP packet is
 * received it is read into the socket buffer). Or conversely the
 * buffer could contain more data than required also due to packetisation.
 *
 * Should modify this to accept the timeout params and then call
 * IsRead(fd, READ, ..) before each recv below. The same should
 * be done for writen
 */
int readn(register SOCKET fd, register char* ptr, register int nbytes)
{
	int nleft, nread;

	nleft = nbytes;
	while(nleft > 0)
	{
		/* Should specify timeout param for readn also.
		   Then here I would call IsSocketReady(fd, socketEvent_READ, timeout, errMsg, sizeof(errMsg))
		   Note C++ is great for this where default value for timeout param would be -1 */
		nread = recv(fd, ptr, nleft,0); /* This is read() on UNIX */
		if (nread < 0)
		{
			if (getSocketSysError()==EINTR) continue; /* If signal interupted us, do recv again */
			return nread; /* the error */
		}
		else if (nread == 0)
			break; /* EOF, short count will be returned below */

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes - nleft); /* return >=0 */
}

/*
 * This is a "safe" version of write. The standard write
 * may return before all the requested bytes are written because
 * the internal buffer (4096 bytes for a pipe for e.g. in some UNIX')
 * has filled.
 */
int writen(register SOCKET fd, register char* ptr, register int nbytes)
{
	int nleft, nwritten;

	nleft = nbytes;
	while(nleft > 0)
	{
		/* To be robust must do an isWriteReady() every time here */
		nwritten = send(fd, ptr, nleft,0);/* This is write() on UNIX */
		if (nwritten < 0)
		{
			if (getSocketSysError()==EINTR) continue; /* If signal interupted us, do send again */
			return nwritten; /* the error */
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nbytes - nleft); /* return >=0 */
}

/*
Note for get*by* functions use h_errno
to get error not getSocketError(). These
functions return NULL on error and valid
values are:

	[HOST_NOT_FOUND]
		    The name you have used is not an official hostname or alias; this
		    is not a soft error, another type of name server request may be
		    successful.

	[NO_ADDRESS]
		    The requested name is valid but does not have an Internet address
		    at the name server.

	[NO_RECOVERY]
		    This is a nonrecoverable error.

	[TRY_AGAIN]
		    This is a soft error that indicates that the local server did not
		    receive a response from an authoritative server. A retry at some
		    later time may be successful.

	[NETDB_INTERNAL] //doze doesn't have this
		    NB need to check errno for error

	non socket specific return -1 => check errno

Also have a:
char* getSocketErrorString()
{
	WIN32
		FormatMessage()
	POSIX
		StrError() or herror()
}
*/

int getSocketSysError(void)
{
	int errornum;
#ifdef WIN32
	errornum=WSAGetLastError();

	/* Following are generic errors so use values from errno.h */
	switch (errornum)
	{
		case WSAEINTR:
		case WSAEBADF:
		case WSAEACCES:
		case WSAEFAULT:
		case WSAEINVAL:
		case WSAEMFILE:
			errornum-=WSABASEERR;
	};
#else
	errornum=errno;
#endif

	return errornum;
}

int getSocketError(SOCKET s)
{
	int error;
	int sizeof_error = sizeof(error);
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &sizeof_error)<0)
		return errno;
	else
		return error;
}

/*
 * Following mapping from SFL, test for
 * EAGAIN to see if need to make calls again
 */
int getSocketSysGenericError(int errornum)
{
	switch(errornum)
	{
		case EWOULDBLOCK:
		case EINPROGRESS:
		case ENETDOWN:
		/* case EINTR:? */
			errornum = EAGAIN;
			break;
		case ECONNABORTED:
		case EINVAL:
			errornum = EPIPE;
			break;
	};
	return errornum;
}

/*
 * Returns the first ip address for the peerName.
 * If peerName is an ip address, then no lookup is
 * done, but the address is returned.
 *
 * If IPaddress is non NULL then a string representation of the address
 * will be copied in.
 *
 * The IP address in network byte order is returned as a long int.
 * 0 indicates error.
 *
 * This function is threadsafe but has only been tested on glibc.
 * It probably will not work on other systems.
 */
unsigned long int getIPAddress(const char* peerName, char* IPaddress, const int bufSize)
{
    unsigned long int retAddress=0;

    if (IPaddress)
        IPaddress[0] = '\0'; /* This indicates error */

    if (!IPaddress || bufSize>=sizeof("111.222.333.444")) /* sanity check (TODO: this should be assertion) */
    {
	struct hostent hostbuf, *phe;
	size_t hstbuflen;
	char *tmphstbuf;
	int res;
	int herr;

	hstbuflen = 1024;
	tmphstbuf = malloc(hstbuflen);
    	if (!tmphstbuf)
	    return 0;

	while ((res = gethostbyname_r(peerName, &hostbuf, tmphstbuf, hstbuflen, &phe, &herr)) == ERANGE)
	{
	    /* Enlarge the buffer */
	    hstbuflen *= 2;
	    tmphstbuf = realloc(tmphstbuf, hstbuflen);
	}

	if (res==0 && phe!=NULL)
	{
	    if (IPaddress) /* Note inet_ntoa is threadsafe in glibc */
        	strncpy(IPaddress, inet_ntoa(*(struct in_addr*)phe->h_addr), bufSize);
	    retAddress = ((struct in_addr*)phe->h_addr)->s_addr;
	}

	free(tmphstbuf);
    	return retAddress;
    }
    return 0;
}

/*
 * If peer IP is NULL, the "main" IP address
 * of the system is returned, otherwise the peer
 * address is used to calculate the appropriate
 * source address to use.
 *
 * If IPaddress is non NULL then a string representation of the address
 * will be copied in.
 *
 * The IP address in network byte order is returned as a long int.
 * 0 indicates error.
 */
unsigned long int getMyIPAddress(struct sockaddr* peer, char* IPaddress, const int bufSize)
{
    if (IPaddress)
        IPaddress[0] = '\0'; /* This indicates error */

    if (!IPaddress || bufSize>=sizeof("111.222.333.444")) /* sanity check (TODO: this should be assertion) */
    {
        if (peer)
        {
            struct sockaddr_in address;
            int sizeof_address;
            int s;

            /* get source address to use for specified destination address */
            s = socket(AF_INET, SOCK_DGRAM, 0);
            if (s < 0)
                return 0;
            if (connect(s, peer, sizeof(struct sockaddr_in)) < 0)
            {
                close(s);
                return 0;
            }
            sizeof_address = sizeof(address);
            if (getsockname(s, (struct sockaddr *) &address, (socklen_t *) &sizeof_address) < 0)
            {
                close(s);
                return 0;
            }
            close(s);

            if (IPaddress) /* Note inet_ntoa is threadsafe in glibc */
                strncpy(IPaddress, inet_ntoa((struct in_addr)address.sin_addr), bufSize);
            return ((struct in_addr)address.sin_addr).s_addr;
        }
        else
        {
            char hostname[255]; /* Should be big enough? */
    	    gethostname(hostname, sizeof(hostname));
            return getIPAddress(hostname, IPaddress, bufSize);
        }
    }
    return 0;
}

/*
 * isconnected - check connect status.
 * This function is not a general function and
 * is just valid for calling from within tconnect
 * (or equivalent).
 * Note if an error occurred in the connect
 * this will be put in errno (WSAGetLastError on doze)
 * just as if checking the error from connect after
 * calling it directly. Must double check that
 * WSAGetLastError is infact set on doze
 */
static int isconnected(SOCKET s, fd_set* rd, fd_set* wr, fd_set* ex)
{

#ifdef WIN32

	if (FD_ISSET( s, ex ))
	{
		WSASetLastError(getSocketError(s));
		return 0;
	}
	if (FD_ISSET( s, wr ))
		return 1;
	return 0; /* No error, just hasn't connected yet */
#else

	int err;
	int len;

	errno = 0;			/* assume no error */
	if ( !FD_ISSET( s, rd ) && !FD_ISSET( s, wr ) )
		return 0; /* No error, just hasn't connected yet */
	if ( getsockopt( s, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
		return 0; /* on some Unixs getsockopt sets errno instead of err */
	errno = err; /* in case we're not connected (err !=0) */
	return err == 0;

#endif
}

/*
 * Notes on Unix & doze treatment of socket descriptors.
 *
 * Point 1 is on doze a socket descriptor is not a generic
 * file descriptor. So you can't use fcntl, open, close, read, ...
 * On Unix fcntl sets things generic to all files(sockets), ioctl() is
 * intended for device specific operations and so would have nothing to
 * do with files(sockets). Things specific to sockets like NAGLE's algorthim
 * for e.g. are manipulated using special functions called setsockopt() &
 * getsockopt(). For e.g. to turn off NAGLE's algorithm (packet coalescing) do:
 *   int flag = 1;//Non Zero
 *   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
 *
 * On the other hand if you want to set the socket(file) to non blocking
 * (which has nothing to do with NAGLE's algorithm by they way, even though
 * the constants have similar naming) then you use the generic fcntl as follows:
 *   fcntl(fd, F_SETFL, O_NDELAY); //System V
 *   fcntl(fd, F_SETFL, FNDELAY);  //BSD
 *
 * Again since windoze seperates file handles and socket descriptors, you
 * must use a different method to control blocking on a socket (ioctlsocket()).
 *
 * A socket handle can optionally be a file handle In Windows Sockets 2. It is
 * possible to use socket handles with ReadFile, WriteFile, ReadFileEx, WriteFileEx,
 * DuplicateHandle, and other Win32 functions. Not all transport service providers will
 * support this option. For an application to run over the widest possible number of service
 * providers, it should not assume that socket handles are file handles.
 *
 * if (changeRequired == NULL) it won't be set.
 */

/*
 * Caveat on windows this always returns true for changeRequired param
 * even if socket is already in the request state.
 */
STATUS_t setBlocking(SOCKET s, BOOL_t state, BOOL_t* changeRequired, char* errMsg, int errMsgSize)
{
#ifdef WIN32
	long arg;

    if (changeRequired)
    	*changeRequired=TRUE_BOOL; /* Don't know how to find blocking status of a socket on doze, so assuming this. */

	if (state == FALSE_BOOL)
		arg=FIONBIO_NONBLOCKING;
	else
		arg=FIONBIO_BLOCKING;

	if (ioctlsocket (s, FIONBIO, &arg) == SOCKET_ERROR)
	{
		snprintf(errMsg, errMsgSize, "ioctlsocket (FIONBIO) failed. errno = [%d]", getSocketSysError());
		return FAIL_STATUS;
	}
#else
	int flags;
	if( ( flags = fcntl( s, F_GETFL, 0 ) ) < 0 )
	{
		snprintf(errMsg, errMsgSize, "fcntl (F_GETFL) failed. errno = [%d]", errno);
		return FAIL_STATUS;
	}

    if (changeRequired)
    	*changeRequired=FALSE_BOOL;

	if (state == FALSE_BOOL)
	{
		if (!BitTst(flags, O_NONBLOCK))
		{
            if (changeRequired)
    			*changeRequired=TRUE_BOOL;
			BitSet(flags,O_NONBLOCK);
		}
	}
	else
	{
		if (BitTst(flags, O_NONBLOCK))
		{
            if (changeRequired)
    			*changeRequired=TRUE_BOOL;
			BitClr(flags,O_NONBLOCK);
		}
	}

	if ( fcntl( s, F_SETFL, flags) < 0 )
	{
		snprintf(errMsg, errMsgSize, "fcntl (F_SETFL) failed. errno = [%d]", errno);
		return FAIL_STATUS;
	}
#endif
	return SUCCESS_STATUS;
}

/*
 * This is a wrapper function for connect() which gives the extra
 * functionality of specifying a timeout for the connection attempt.
 * connect() on its own has a timeout of course, but this is determined
 * by various settings in the TCP/IP stack underneath, but is generally
 * around 75 seconds. Usually this is too long and therefore this function
 * can be used to specify a shorter timeout. Note you CAN'T specify a longer
 * timeout than this system timeout. Note Solaris 2.6 for e.g. allows setting
 * the timeout per socket using setsockopt(), but this is not portable.
 *
 * May have to take different actions depending, on which
 * of the following 4 things occur:
 *		success
 *		timeout
 *		connect failed (within timeout) 	(specific error details in errno (same as returned by connect) (or WSAGetLastError on doze))
 *		other error 						(specific error details in errMsg)
 *
 * Note timeout is returned for system timeout also (if we specify timeout that's
 * longer than the system connect timeout). Note in this case you could loop
 * through the connect() calls again, subtracting the appropriate time.
 *
 * Caveat on windoze is that this always changes the socket to blocking
 * if it was nonblocking before this call (probabaly was anyway).
 *
 * Note for timeout values to pick, when on a LAN a timeout value of about 8 to
 * 20 milliseconds is sufficient, whereas general internet connections can take
 * from 40 to 800 milliseconds or even more. The timeout param here is in milliseconds
 * Note being able to specify timeouts down to milliseconds would help a lot for
 * port scanners etc.
 * A note on the timer resolution, on Linux select() and usleep() have a 10ms granularity,
 * as they depend on the scheduler, and hence the system time tick which is 10ms on intel and
 * 1ms on alpha. Note also usleep (but not select I think) often overshoots by up to 10ms since it has
 * to sleep at least the asked amount, not less. If you usleep() near the end of a tick, the delay
 * will be closer to what you asked for. So 20ms is the lowest determinable delay for usleep!
 * Note man nanosleep, says it's posix and can do a busy wait with microsecond resolution for up
 * to 2 ms which may be useful.
 * From the Linux I/O port programming mini-HOWTO:
 *
 *  For delays <= 2 ms, if (and only if) your process is set to soft real
 *  time scheduling (using sched_setscheduler()), nanosleep() uses a busy
 *  loop; otherwise it sleeps, just like usleep().
 *
 *  The busy loop uses udelay() (an internal kernel function used by many
 *  kernel drivers), and the length of the loop is calculated using the
 *  BogoMips value (the speed of this kind of busy loop is one of the
 *  things that BogoMips measures accurately). See
 *  /usr/include/asm/delay.h) for details on how it works.
 *
 * The next section of the HOWTO describes something very useful:
 *
 *  Another way of delaying small numbers of microseconds is port
 *  I/O. Inputting or outputting any byte from/to port 0x80 (see above for
 *  how to do it) should wait for almost exactly 1 microsecond independent
 *  of your processor type and speed [note it obviously has to be x86 arch
 *  for port I/O to work]. You can do this multiple times to wait a few
 *  microseconds. The port output should have no harmful side effects on any
 *  standard machine (and some kernel drivers use it). This is how {in|out}[bw]_p()
 *  normally do the delay (see asm/io.h).
 *
 *  Actually, a port I/O instruction on most ports in the 0-0x3ff range
 *  takes almost exactly 1 microsecond, so if you're, for example, using
 *  the parallel port directly, just do additional inb()s from that port
 *  to delay.
 *
 * Note occasionally, the process could get pre-empted while pausing so we get a
 * delay that's a little longer than we want?
 *
 * Very good thread on lkml related to this: http://www.uwsg.iu.edu/hypermail/linux/kernel/9907.1/0552.html
 * Mentions a patch (utime) to kernel that increases granularity. Note this is part of KURT.
 * Getting into can of worms here though in regard to RTAI, RTLinux, Low latency patches, comparisons with Tru64....
 *
 * Another very NB use of this function is when we're being called from
 * IIS or something, we want to report back ASAP that can't connect if
 * server we're talking to dies or something. Else the context in IIS will
 * fill with clients waiting for default timeout (can be up to 1 minute!).
 *
 * The current implementation of connect() poses some severe programming problems related to the connection
 * timeout. connect() has a default retry and timeout setting and there's no way to change this.
 * Windows NT lets you set a retry count in the registry, but this affects all programs. Besides, for
 * Windows 95 or Windows for Workgroups 3.11 there's no such option. The total timeout for a
 * connect() call (if the server doesn't respond) is about 1.6 seconds.
 *
 * Note there are some caveats to this approach. Internally, connect() does not finish until after
 * its own timeout has expired, so this implies that if you use a timeout shorter than connect()s
 * internal timeout, you will loose error info and just get a timeout. Note I also noticed when I made
 * a connection from 95->Tru64 that if a connection is refused, this isn't indicated for about 1.5 seconds!
 * It could be the Tru64 stack doing this so port scanning is harder? If it's connect() on doze retrying
 * then this is brain dead. Must look into this on different platforms, but setting the timeout >= 1.6 seconds
 * should be high enough if you require this error indication back. Note another possible prob with connect still
 * retrying in background is that if you try other operations (like connect again for e.g.) on the socket,
 * it would probably come back with an error, saying ALREADY_DOING_THAT or whatever?
 * Update here is Linux 2.4 -> Linux 2.4 => if can't connect then this is indicated immediately.
 *
 * Also if you need a longer timeout value than the internal one, you must call connect() again after its
 * timeout has expired. Note if you connect using dialup or ISDN connections, these can take several seconds to set
 * up. However, using the non-blocking version of connect() makes it impossible to determine if connect() is still
 * waiting or retrying. On Windows 95 it appears to be possible to measure this value by repeatedly calling select()
 * with short timeout intervals, since GetLastError() returns a different value when connect() has timed out.
 * However, this trick failed under Windows NT and depends on undocumented behavior. The proper solution would
 * be to use WinSock 2 if possible, which has event and synchronization mechanisms.
 *
 * Note for input parameter timeout:
 *    =0 => invalid
 *    <0 => no functionality over connect(), but supported so can always just call tconnect
 *    >0 => timeout in milliseconds
 */

tconnect_t tconnect(SOCKET s, struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize)
{
	fd_set rdevents, wrevents, exevents;
	struct timeval tv;
	int rc;
    int curr_error;
	BOOL_t changeWasRequired;

	errMsg[0]='\0';

	if (timeout==0) /* This function is redundant/invalid if a timeout of 0 is used. */
	{
		snprintf(errMsg, errMsgSize, "timeout parameter can not be 0");
		return tconnect_ERROR;
	}
	FD_ZERO(&rdevents);
	FD_SET(s, &rdevents);
	wrevents = exevents = rdevents;
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;

	if (setBlocking(s, FALSE_BOOL, &changeWasRequired, errMsg, errMsgSize)==FAIL_STATUS)
		return tconnect_ERROR;

    for(;;) {
        rc = connect(s, peer, addrlen);
	    if (rc) {
            curr_error = getSocketSysError();
            if (curr_error == EINTR) {
                continue;
            } else {
                if (getSocketSysGenericError(curr_error) == EAGAIN) {
                    break; /* else OK to go on as connect could complete later on */
                } else {
                    return tconnect_CONNECT_FAILED;
                }
            }
        } else { /* already connected? */
		    if (changeWasRequired==TRUE_BOOL)
			    if (setBlocking(s, TRUE_BOOL, &changeWasRequired, errMsg, errMsgSize)==FAIL_STATUS)
				    return tconnect_ERROR;
		    return tconnect_SUCCESS;
        }
    }

	for(;;)
	{
		rc = select( s + 1, &rdevents, &wrevents, &exevents, (timeout < 0) ? 0: &tv);
		if ( rc < 0 )
		{
			if (getSocketSysError()==EINTR) continue; /* If signal interupted us, do syscall again (note this restarts timeout) */
			snprintf(errMsg, errMsgSize, "select error. errno = [%d]", getSocketSysError());
			return tconnect_ERROR;
		}
		else if ( rc == 0 )
			return tconnect_TIMEOUT;
		/* NB this (isconnected) must be last function called that sets errno before
		   returning tconnect_CONNECT_FAILED. Should really fix this dependency */
		else if ( isconnected( s, &rdevents, &wrevents, &exevents ) )
		{
			if (changeWasRequired==TRUE_BOOL)
				if (setBlocking(s, TRUE_BOOL, &changeWasRequired, errMsg, errMsgSize)==FAIL_STATUS)
					return tconnect_ERROR;
			return tconnect_SUCCESS;
		}
		else
		{
			if (getSocketSysError() == ETIMEDOUT) /* system timeout came before specified timeout */
				return tconnect_TIMEOUT;
			else
				return tconnect_CONNECT_FAILED;
		}
	}
}

/*
 *    return:
 *    success
 *    timeout
 *    connect failed (within timeout) 	(specific error details in errno (same as returned by connect) (or WSAGetLastError on doze))
 *    other error 						(specific error details in errMsg, errno as set by syscall)
 *
 *    timeout:
 *    =0 => invalid
 *    <0 => no functionality over connect(), but supported so can always just call tconnect
 *    >0 => timeout in milliseconds
 */

tconnect_t thalf_connect(struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize)
{
    struct tcphdr send_tcp;

    struct
    {
        struct iphdr ip;
        struct tcphdr tcp;
        unsigned char junk_data[65535];
    } recv_tcp;

    struct /*This struct is used only to allow us to properly calculate the
             checksum in the tcp header struct. */
    {
        unsigned long int src_address;
        unsigned long int dest_address;
        unsigned char placeholder;
        unsigned char protocol;
        unsigned short tcp_length;
        struct tcphdr tcp;
    }  pseudo_header;

    int tcp_sock;
    struct sockaddr_in sockaddr;
    int sockaddr_len;
    IsReady_t socket_state;
    tconnect_t ret;
    static unsigned short int blah = 0;
    static unsigned short int curr_unique_val;
    unsigned long int dest_inet_address;
    unsigned short int dest_inet_port;

    errMsg[0]='\0';

    if (timeout==0) /* This function is redundant/invalid if a timeout of 0 is used. */
    {
        snprintf(errMsg, errMsgSize, "timeout parameter can not be 0");
        return tconnect_ERROR;
    }

    /* Generate diff values for source port and sequence numbers just in case */
    do {
        blah++;
        curr_unique_val = getpid() + blah;/*TODO: possibly use gettid() ?*/
    } while (!curr_unique_val); /*don't use 0 as this is special and will be modified by kernel*/

    /* extract required bits from passed address */
    dest_inet_address = ((struct in_addr)((struct sockaddr_in*)peer)->sin_addr).s_addr;
    dest_inet_port = ((struct sockaddr_in*)peer)->sin_port;

    /*First, fill in the TCP header fields */
    send_tcp.source = curr_unique_val;
    send_tcp.dest = dest_inet_port;
    send_tcp.seq = curr_unique_val;
    send_tcp.ack_seq = 0;
    send_tcp.res1 = 0;
    send_tcp.doff = 5;
    send_tcp.res2 = 0;
    send_tcp.fin = 0;
    send_tcp.syn = 1;  /* NB bit */
    send_tcp.rst = 0;
    send_tcp.psh = 0;
    send_tcp.ack = 0;
    send_tcp.urg = 0;
    send_tcp.window = htons(512);
    send_tcp.check = 0;  /*real checksum calculated farther down...*/
    send_tcp.urg_ptr = 0;

    /* fill pseudo header */
    pseudo_header.src_address = getMyIPAddress(peer, (char*)NULL, 0);
    if (!pseudo_header.src_address)
    {
        snprintf(errMsg, errMsgSize, "getMyIPAddress error. errno = [%d]", getSocketSysError());
        return tconnect_ERROR;
    }
    pseudo_header.dest_address = dest_inet_address;
    pseudo_header.placeholder = 0;
    pseudo_header.protocol = IPPROTO_TCP;
    pseudo_header.tcp_length = htons(sizeof (send_tcp));

    /* Calculate the checksum for the (pseudo) TCP header */
    memcpy (&pseudo_header.tcp, &send_tcp, sizeof (send_tcp));
    send_tcp.check = in_cksum ((unsigned short *)&pseudo_header, sizeof (pseudo_header));

    /* Fill in the sockaddr_in structure */
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = dest_inet_port;
    sockaddr.sin_addr.s_addr = dest_inet_address;
    sockaddr_len = sizeof(sockaddr);

    /* Create the socket */
    tcp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (tcp_sock < 0)
    {
        snprintf(errMsg, errMsgSize, "socket error. errno = [%d]", getSocketSysError());
        return tconnect_ERROR;
    }

    if (setBlocking(tcp_sock, FALSE_BOOL, (BOOL_t*) NULL, errMsg, errMsgSize)==FAIL_STATUS)
    {
        ret = tconnect_ERROR;
        goto thalf_connect_error_handler;
    }

    /* Send out the packet */
    if (sendto (tcp_sock, &send_tcp, 20, 0, (struct sockaddr *)&sockaddr, sockaddr_len) != 20) {
        snprintf(errMsg, errMsgSize, "sendto error. errno = [%d]", getSocketSysError());
        ret = tconnect_ERROR;
        goto thalf_connect_error_handler;
    }

    /* Receive the reply */
    do
    {
        ret = tconnect_UNKNOWN;
        socket_state = IsSocketReady(tcp_sock, socketEvent_READ, timeout, errMsg, errMsgSize);
        switch (socket_state)
        {
            case IsReady_YES:
                memset (&recv_tcp, '\0', sizeof(struct iphdr) + sizeof(struct tcphdr));
                if (recvfrom (tcp_sock, &recv_tcp, sizeof (recv_tcp), 0, /* could use read instead */
		            (struct sockaddr *)&sockaddr, &sockaddr_len) < 0)
                {
                    snprintf(errMsg, errMsgSize, "recvfrom error. errno = [%d]", getSocketSysError());
                    ret = tconnect_ERROR;
                    goto thalf_connect_error_handler;
                }
                ret = tconnect_SUCCESS;
                break;
            case IsReady_NO:
                ret = tconnect_TIMEOUT;
                goto thalf_connect_error_handler;
                break;
            case IsReady_ERROR:
                ret = tconnect_ERROR;
                goto thalf_connect_error_handler;
                break;
        }
    } while (recv_tcp.tcp.dest != curr_unique_val);

thalf_connect_error_handler:

    close(tcp_sock);
    if (ret == tconnect_SUCCESS)
    {
        if (recv_tcp.tcp.syn != 1)  /*The port is inactive.. (should I check for !rst?)*/
        {
            errno = ECONNREFUSED;
            ret = tconnect_CONNECT_FAILED;
        }
    }

    return ret;
}

/* the following is useful if you select between tconnect and thalf_connect
   using a function pointer. I.E. this function gives thalf_connect the
   same interface as tconnect, which is: tconnect_func_t */
tconnect_t thalf_connect_wrapper(int s, struct sockaddr* peer, int addrlen, long timeout, char* errMsg, int errMsgSize)
{
    return thalf_connect(peer,addrlen,timeout,errMsg,errMsgSize);
}

/*
 * For the moment have read/write/exception events
 * mutually exclusive, i.e. not OR'd together?
 * If need to change this then will have to return
 * which event(s) happened somehow.
 *
 * timeout = 0 => return immediately (i.e. poll)
 * timeout < 0 => block indefinitely until event happens.
 * timeout > 0 => wait this number of milliseconds for event to happen
 *
 * Note if timeout of 0 is specified (i.e. polling) then
 * this will be a nonblocking call.
 *
 * The forWhat parameter can accept:
 *  socketEvent_READ
 *  socketEvent_WRITE
 *  socketEvent_EXCEPTION
 *
 * The following can be returned
 *	IsReady_YES    (requested event has occurred)
 *	IsReady_NO     (due to timeout or in case of polling just indication)
 *	IsReady_ERROR  (message in errMsg parameter, errno is as set by select)
 */

IsReady_t IsSocketReady(SOCKET s, socketEvent_t forWhat, long timeout, char* errMsg, int errMsgSize)
{
	fd_set fds, *fdrset, *fdwset, *fdeset;
	struct timeval tv;
	int rc;

	fdrset= fdwset = fdeset = NULL;
	FD_ZERO (&fds);
	FD_SET (s, &fds);
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;

	switch (forWhat)
	{
		case socketEvent_READ:		fdrset = &fds; break;
		case socketEvent_WRITE:		fdwset = &fds; break;
		case socketEvent_EXCEPTION:	fdeset = &fds; break;
		default: /* Compiler should only allow the above, but if someone casts for some reason... */
		{
			snprintf(errMsg, errMsgSize, "Received unrecognised value [%d] for parameter forWhat", forWhat);
			return IsReady_ERROR;
		}
	}

	for (;;)
	{
		rc = select (s+1, fdrset, fdwset, fdeset, (timeout < 0) ? 0: &tv);
		if (rc < 0)
		{
			if (getSocketSysError()==EINTR) continue; /* If signal interupted us, do syscall again (note this restarts timeout?) */
			snprintf(errMsg, errMsgSize, "IsSocketReady::select error. errno = [%d]", getSocketSysError());
			return IsReady_ERROR;
		}
		else if (rc == 0) /* none set (after poll or timeout) */
			return IsReady_NO;
		else
			return IsReady_YES;
	}
}

#if 0
/* returns 1 if both ip & netmask OK. 0 otherwise
   ip can be 0 if you just want to check mask validity
   else it's checked for consistency with the given IP address.

   e.g. usage:

   struct in_addr ip_addr,nm_addr;
   char* ip_str="123.24.1.2";
   char* nm_str="123.24.255.255";
   inet_aton(ip_str, &ip_addr);
   inet_aton(nm_str, &nm_addr);
   check_netmask(nm_addr.s_addr, 0);
   check_netmask(nm_addr.s_addr, ip_addr.s_addr);
*/
int check_ip_netmask(unsigned long int nm, unsigned long int ip)
{
    nm = ntohl(~nm);
	if (nm & (nm+1))
		return 0;
    if (!(orig_nm & addr))
        return 0;
	return 1;
}
#endif

/*
 * Copyright (c) 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

/*
 * in_cksum --
 *      Checksum routine for Internet Protocol family headers (C Version)
 */
unsigned short in_cksum(unsigned short *addr,int len)
{
        register int sum = 0;
        u_short answer = 0;
        register u_short *w = addr;
        register int nleft = len;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}
