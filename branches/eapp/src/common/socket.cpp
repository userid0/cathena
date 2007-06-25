// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include "baseparam.h"
#include "baseipfilter.h"
#include "basebits.h"
#include "socket.h"
#include "mmo.h"	// [Valaris] thanks to fov
#include "utils.h"
#include "showmsg.h"

//#define SOCKET_DEBUG_PRINT
#define SOCKET_DEBUG_LOG



///////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
// defines for direct access of fd_set data on unix
//
// it seams on linux you need to compile with __BSD_VISIBLE, _GNU_SOURCE or __USE_XOPEN
// defined to get the internal structures visible.
// anyway, since I can only test on solaris, 
// i cannot tell what other machines would need
// so I explicitely set the necessary structures here.
// and you would not need to redefine anything
//
// anyway, if compiler warning/error occure here, try to comment out 
// the quested thing and please give a report
//
// it should be checked if the fd_mask is still an ulong on 64bit machines 
// if lucky the compiler will throw a redefinition warning if different
// further, 64 bit unix defines FD_SETSIZE with 65535 so maybe reconsider 
// the current structures

//typedef	unsigned long	fd_mask;
#ifndef NBBY
#define	NBBY 8
#endif
#ifndef howmany
#define	howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif  
#ifndef NFDBITS
#define	NFDBITS	(sizeof(unsigned long) * NBBY)	// bits per mask
#endif
#ifndef __FDS_BITS
# define __FDS_BITS(set) ((set)->fds_bits)
#endif

#endif
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
time_t								last_tick   = time(0);
basics::CParam<ulong>				stall_time("stall_time", 60);
basics::CParam<basics::ipfilter>	ddos("ipfilter");


///////////////////////////////////////////////////////////////////////////////

//int rfifo_size = 65536;
//int wfifo_size = 65536;

// from freya, playing around with that a bit

size_t RFIFO_SIZE = (2*1024);	// a player that send more than 2k is probably a hacker without be parsed
								// biggest known packet: S 0153 <len>.w <emblem data>.?B -> 24x24 256 color .bmp (0153 + len.w + 1618/1654/1756 bytes)
size_t WFIFO_SIZE = (32*1024);

///////////////////////////////////////////////////////////////////////////////
#ifndef TCP_FRAME_LEN
#define TCP_FRAME_LEN 1053
#endif



///////////////////////////////////////////////////////////////////////////////
//
// Socket <-> Session Handling
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// array of session data
struct socket_data *session[FD_SETSIZE];
// number of used array elements

fd_set readfds;		// in WIN32 this contains a sorted list of the sockets

size_t fd_max=0;	// greatest used session fd in the field
					// it is also used for the select call on Berkeley sockets
					// but it could be replaced with FD_SETSIZE there
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// extra data for windows because windows gives only system wide sockets
// so you can fast exceed the FD_SETSIZE

// list of session positions corrosponding to fd_set list
// so fist search a socket in readfds with binary search to get a position
// and then look up the corrosponding fd for session[fd]
size_t session_pos[FD_SETSIZE];	

// list of sockets corrosponding to session list
// socket_pos[fd] is the SOCKET of the session[fd]
SOCKET	socket_pos[FD_SETSIZE];	


///////////////////////////////////////////////////////////////////////////////
// binary search implementation
// might be not that efficient in this implementation
// it first checks the boundaries so calls outside 
// the list range are handles much faster 
// at the expence of some extra code
// runtime behaviour much faster if often called for outside data
///////////////////////////////////////////////////////////////////////////////
bool SessionBinarySearch(const SOCKET elem, size_t &retpos)
{	// do a binary search with smallest first
	// make some initial stuff
	bool ret = false;
	size_t a=0, b=readfds.fd_count-1, c;
	size_t pos = 0;

	// just to be sure we have to do something
	if( readfds.fd_count==0 )
	{	ret = false;
	}
	else if( elem < readfds.fd_array[a] )
	{	// less than lower
		pos = a;
		ret = false; 
	}
	else if( elem > readfds.fd_array[b] )
	{	// larger than upper
		pos = b+1;
		ret = false; 
	}
	else if( elem == readfds.fd_array[a] )
	{	// found at first position
		pos=a;
		ret = true;
	}
	else if( elem == readfds.fd_array[b] )
	{	// found at last position
		pos = b;
		ret = true;
	}
	else
	{	// binary search
		// search between first and last
		do
		{
			c=(a+b)/2;
			if( elem == readfds.fd_array[c] )
			{	// found it
				b=c;
				ret = true;
				break;
			}
			else if( elem < readfds.fd_array[c] )
				b=c;
			else
				a=c;
		}while( (a+1) < b );
		pos = b;
		// return the next larger element to the given 
		// or the found element so we can insert a new element there
	}
	retpos = pos;
	return ret;
}
bool SessionFindSocket(const SOCKET elem, size_t &retpos)
{
	size_t pos;
	if( SessionBinarySearch(elem, pos) )
	{
		retpos = session_pos[pos];
		return true;
	}
	return false;
}

int SessionInsertSocket(const SOCKET elem)
{
	size_t pos,fd;
	if(fd_max<FD_SETSIZE) // max number of allowed sockets
	if( !SessionBinarySearch(elem, pos) )
	{
		if((size_t)fd_max!=pos)
		{
			memmove( session_pos     +pos+1, session_pos     +pos, (readfds.fd_count-pos)*sizeof(size_t));
			memmove( readfds.fd_array+pos+1, readfds.fd_array+pos, (readfds.fd_count-pos)*sizeof(SOCKET));
		}

		// find an empty position in session[]
		for(fd=0; fd<FD_SETSIZE; ++fd)
			if( NULL==session[fd] )
				break;
		// we might not need to test validity 
		// since there must have been an empty place
		session_pos[pos]      = fd;
		readfds.fd_array[pos] = elem;
		readfds.fd_count++;

		if((size_t)fd_max<=fd)	fd_max = fd+1;
		socket_pos[fd] = elem;	// corrosponding to session[fd]

		return (int)fd;
	}
	// otherwise the socket is already in the list
	return -1;
}

bool SessionRemoveSocket(const SOCKET elem)
{
	size_t pos;
	if( SessionBinarySearch(elem, pos) )
	{
		// be sure to clear session[]
		// we do not care for that here
		shutdown(elem,SD_BOTH);
		closesocket(elem);
		socket_pos[session_pos[pos]] = (SOCKET)-1;

		memmove( session_pos     +pos, session_pos     +pos+1, (readfds.fd_count-pos-1)*sizeof(size_t));
		memmove( readfds.fd_array+pos, readfds.fd_array+pos+1, (readfds.fd_count-pos-1)*sizeof(SOCKET));
		readfds.fd_count--;
		return true;
	}
	// otherwise the socket is not in the list
	return false;
}

bool SessionRemoveIndex(const size_t fd)
{
	if( fd < FD_SETSIZE )
		return SessionRemoveSocket( socket_pos[fd] );
	return false;
}
SOCKET SessionGetSocket(const size_t fd)
{
	if( fd < FD_SETSIZE )
		return socket_pos[fd];
	return (SOCKET)-1;
}
///////////////////////////////////////////////////////////////////////////////
#else//! WIN32
///////////////////////////////////////////////////////////////////////////////
bool SessionFindSocket(const SOCKET elem, size_t &retpos)
{	// socket and position are identical
	if(elem < FD_SETSIZE)
	{
		retpos = (size_t)elem;
		return true;
	}
	return false;
}
int SessionInsertSocket(const SOCKET elem)
{
	if(elem > 0 && elem < FD_SETSIZE)
	{
		FD_SET(elem,&readfds);
		// need for select
		// we reduce this on building the writefds when necessary
		if((SOCKET)fd_max<=elem) fd_max = elem+1; 
	}
	return elem;
}
bool SessionRemoveSocket(const SOCKET elem)
{	// socket and position are identical
	if(elem > 0 && elem < FD_SETSIZE)
	{
		FD_CLR(elem,&readfds);
		closesocket(elem);
	}
	return true;
}
bool SessionRemoveIndex(const size_t pos)
{	// socket and position are identical
	if(pos > 0 && pos < FD_SETSIZE)
	{
		FD_CLR(pos,&readfds);
		closesocket(pos);
	}
	return true;
}
SOCKET SessionGetSocket(const size_t pos)
{	// socket and position are identical
	if(pos < FD_SETSIZE)
		return (SOCKET)pos;
	return (SOCKET)-1;
}
///////////////////////////////////////////////////////////////////////////////
#endif//! WIN32
///////////////////////////////////////////////////////////////////////////////
































///////////////////////////////////////////////////////////////////////////////
//
// Parse Functions
//
///////////////////////////////////////////////////////////////////////////////
int null_parse(int fd);
int (*default_func_parse)(int) = null_parse;
int null_console_parse(const char *buf);
int (*default_console_parse)(const char*) = null_console_parse;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_defaultparse(int (*defaultparse)(int))
{
	default_func_parse = defaultparse;
}


int null_parse(int fd)
{
	ShowMessage("null_parse : %d\n",fd);
	RFIFOSKIP(fd,RFIFOREST(fd));
	return 0;
}




///////////////////////////////////////////////////////////////////////////////
//
// Socket Control
//
///////////////////////////////////////////////////////////////////////////////
void socket_setopts(SOCKET sock)
{
	unsigned long ctlyes = 1;
	// FIONBIO Use with a nonzero argp parameter to enable the nonblocking mode of the socket. 
	// The argp parameter is zero if nonblocking is to be disabled. 
	ioctlsocket(sock, FIONBIO, &ctlyes);
	
#ifndef WIN32
	int optyes = 1;
#endif
	// I don't think we need this
	// TCP_NODELAY BOOL Disables the Nagle algorithm for send coalescing. 
	//if(setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&optyes,sizeof(optyes))<0)
	//	printf("setsockopt: TCP_NODELAY failed! errno=%d: %s\n", basics::sockerrno(), basics::sockerrmsg(basics::sockerrno()));

#ifndef WIN32
    // set SO_REAUSEADDR to true, unix only. on windows this option causes
    // the previous owner of the socket to give up, which is not desirable
    // in most cases, neither compatible with unix.
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&optyes,sizeof(optyes));
#ifdef SO_REUSEPORT
	setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,(char *)&optyes,sizeof(optyes));
#endif
#endif
// not applicable since buffer size is varying
//	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &wfifo_size , sizeof(rfifo_size ));
//	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &rfifo_size , sizeof(rfifo_size ));

/*	{	// set SO_LINGER option (from Freya)
		// (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/closesocket_2.asp)
		// quite useless since no real advantage:
		// SO_DONTLINGER: Graceful disconnection, No waiting
		// SO_LINGER with 0: hard disconnection, No waiting
		// SO_LINGER with time: Graceful disconnection, waiting for time (seconds)
		struct linger opt;
		opt.l_onoff = 1;
		opt.l_linger = 0;	// force disconnection, any unsent data is lost.
		if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt)))
			ShowWarning("setsocketopts: Unable to set SO_LINGER mode for connection %d!\n",sock);
	}
*/
}





///////////////////////////////////////////////////////////////////////////////
//
// FIFO Reading/Sending Functions
//
///////////////////////////////////////////////////////////////////////////////
#ifdef SOCKET_DEBUG_PRINT
void dumpx(unsigned char *buf, int len)
{	int i;
	if(len>0)
	{
		for(i=0; i<len; ++i)
			printf("%02X ", buf[i]);
	}
	printf("\n");
	fflush(stdout);
}
#endif


int recv_to_fifo(int fd)
{
	int len;
	unsigned long arg = 0;
	int rv;

	//ShowMessage("recv_to_fifo : %d\n",fd);
	if( !session_isActive(fd) )
		return -1;

	// check for incoming data and the amount
	rv = ioctlsocket(SessionGetSocket(fd), FIONREAD, &arg);
	if( (rv == 0) && (arg > 0) )
	{	// we are reading 'arg' bytes of data from the socket

		// if there is more on the socket, limit the read size
		// fifo should be sized that the message with max expected len fit in
		unsigned long sz = RFIFOSPACE(fd);
		if( arg > sz ) arg = sz;

		len=read(SessionGetSocket(fd),(char*)(session[fd]->rdata+session[fd]->rdata_size),arg);

		if(len > (int)RFIFOSPACE(fd))
		{
			ShowError("Read Socket out of bound\n"
				CL_SPACE"error not recoverable, quitting.\n");
			exit(1);
		}
#ifdef SOCKET_DEBUG_PRINT
		printf("<-");
		dumpx(session[fd]->rdata, len);
#endif  
		if(len>0){
			session[fd]->rdata_size+=len;
			if(session[fd]->rdata_tick) session[fd]->rdata_tick = last_tick;
		} else if(len<=0){
			session[fd]->flag.connected = false;
			session[fd]->wdata_size=0;
		}
	} else {	
		// the socket has been terminated
		session[fd]->flag.connected = false;
	}
	return 0;
}
int send_from_fifo(int fd)
{
	int len;

	//ShowMessage("send_from_fifo : %d\n",fd);
	if( !session_isValid(fd) )
		return -1;

	if (session[fd]->wdata_size == 0)
		return 0;

	// clear buffer if not connected
	if( !session[fd]->flag.connected )
	{
		session[fd]->wdata_size = 0;
		return 0;
	}

	len=write(SessionGetSocket(fd),(char*)(session[fd]->wdata),session[fd]->wdata_size);
#ifdef SOCKET_DEBUG_PRINT
	printf("->");
	dumpx(session[fd]->wdata, len);
#endif
	if(len>0)
	{
		if((size_t)len<session[fd]->wdata_size)
		{
			memmove(session[fd]->wdata,session[fd]->wdata+len,session[fd]->wdata_size-len);
			session[fd]->wdata_size -= len;
		}
		else
		{
			session[fd]->wdata_size=0;
		}
	}
	else if (errno != EAGAIN)
	{
//		ShowMessage("set eof :%d\n",fd);
		session[fd]->flag.connected = false;
		session[fd]->wdata_size=0;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Fifo Control
//
///////////////////////////////////////////////////////////////////////////////
void flush_fifos() 
{
	// write fifos and be sure the data in on the run
	// more complex method which might be faster in the end
	size_t fd;
	int len, c;
	fd_set wfd;
	
	while(1)
	{
		memset(&wfd,0,sizeof(fd_set));
		c=0;
		for(fd=0; fd<fd_max; ++fd)
		{
			if( session_isActive(fd) && session[fd]->func_send == send_from_fifo && session[fd]->wdata_size > 0)
			{
				// try to write the data nonblocking
				len=write(SessionGetSocket(fd),(char*)(session[fd]->wdata),session[fd]->wdata_size);
#ifdef SOCKET_DEBUG_PRINT
				printf("->");
				dumpx(session[fd]->wdata, len);
#endif
				if(len>0)
				{
					if((size_t)len < session[fd]->wdata_size)
					{
						memmove(session[fd]->wdata,session[fd]->wdata+len,session[fd]->wdata_size-len);
						session[fd]->wdata_size-=len;

						// we have a problem, not all data has been send
						// so we mark that socket
						FD_SET(SessionGetSocket(fd), &wfd);
						++c;
					}
					else
					{	// everything is ok and on the run
						session[fd]->wdata_size=0;
					}
				}
				else if (errno != EAGAIN) 
				{	// the socket is gone, just disconnect it, will be cleared later
					session[fd]->flag.connected = false;
					session[fd]->wdata_size=0;
				}
				else // EAGAIN
				{	// try it again later; might lead to infinity loop when connections is generally blocked
					FD_SET(SessionGetSocket(fd), &wfd);
					++c;
				}
			}
		}
		// finish if all has been send and no need to wait longer
		if(c==0) break; 
		// otherwise wait until marked write sockets can expect more data 
		select(fd_max,NULL,&wfd,NULL,NULL);
	}//end while
}

int realloc_readfifo(int fd, size_t addition)
{
	size_t newsize;

	if( !session_isValid(fd) ) // might not happen
		return 0;

	if( session[fd]->rdata_size + addition  > session[fd]->rdata_max )
	{	// grow rule
		newsize = RFIFO_SIZE;
		while( session[fd]->rdata_size + addition > newsize ) newsize += newsize;
	}
//	else if( session[fd]->rdata_max>RFIFO_SIZE && (session[fd]->rdata_size+addition)*4 < session[fd]->rdata_max )
//	{	// shrink rule
//		newsize = session[fd]->rdata_max/2;
//	}
	else // no change
		return 0;

//	RECREATE(session[fd]->rdata, unsigned char, newsize);
//	session[fd]->rdata_max  = newsize;
	unsigned char *temp = new unsigned char[newsize];
	if(session[fd]->rdata)
	{
		if(session[fd]->rdata_size)
			memcpy(temp, session[fd]->rdata, session[fd]->rdata_size);
		delete[] session[fd]->rdata;
	}
	session[fd]->rdata = temp;
	session[fd]->rdata_max  = newsize;


	return 0;
}

int realloc_writefifo(int fd, size_t addition)
{
	size_t newsize;

	if( !session_isValid(fd) ) // might not happen
		return 0;

	if( session[fd]->wdata_size + addition  > session[fd]->wdata_max )
	{	// grow rule
		newsize = WFIFO_SIZE;
		while( session[fd]->wdata_size + addition > newsize ) newsize += newsize;
	}
//	else if( session[fd]->wdata_max>WFIFO_SIZE && (session[fd]->wdata_size+addition)*4 < session[fd]->wdata_max )
//	{	// shrink rule
//		newsize = session[fd]->wdata_max/2;
//	}
	else // no change
		return 0;

	//printf("realloc %d->%d (+%d)\n", session[fd]->wdata_max, newsize,addition);fflush(stdout);

//	RECREATE(session[fd]->wdata, unsigned char, newsize);
//	session[fd]->wdata_max  = newsize;
	unsigned char *temp = new unsigned char[newsize];
	if(session[fd]->wdata)
	{
		if(session[fd]->wdata_size)
			memcpy(temp, session[fd]->wdata, session[fd]->wdata_size);
		delete[] session[fd]->wdata;
	}
	session[fd]->wdata = temp;
	session[fd]->wdata_max  = newsize;
	return 0;
}

int realloc_fifo(int fd, size_t rfifo_size, size_t wfifo_size)
{
	struct socket_data *s =session[fd];
	
	if( !session_isActive(fd) )
		return 0;
	
	//printf("realloc all %d,%d\n", rfifo_size, wfifo_size);fflush(stdout);
	if( s->rdata_max != rfifo_size && s->rdata_size < rfifo_size)
	{
		unsigned char *temp = new unsigned char[rfifo_size];
		if(session[fd]->rdata)
		{
			if(session[fd]->rdata_size)
				memcpy(temp, session[fd]->rdata, session[fd]->rdata_size);
			delete[] session[fd]->rdata;
		}
		session[fd]->rdata = temp;
		session[fd]->rdata_max  = rfifo_size;

	}
	if( s->wdata_max != wfifo_size && s->wdata_size < wfifo_size){
		unsigned char *temp = new unsigned char[wfifo_size];
		if(session[fd]->wdata)
		{
			if(session[fd]->wdata_size)
				memcpy(temp, session[fd]->wdata, session[fd]->wdata_size);
			delete[] session[fd]->wdata;
		}
		session[fd]->wdata = temp;
		session[fd]->wdata_max  = wfifo_size;

	}
	return 0;
}

bool session_checkbuffer(int fd, size_t sz)
{
	if( session_isActive(fd) )
	{
		struct socket_data *s =session[fd];
		
		sz += s->wdata_size;
		if( sz > s->wdata_max)
		{
			unsigned char *temp = new unsigned char[sz];
			if(session[fd]->wdata)
			{
				if(session[fd]->wdata_size)
					memcpy(temp, session[fd]->wdata, session[fd]->wdata_size);
				delete[] session[fd]->wdata;
			}
			session[fd]->wdata = temp;
			session[fd]->wdata_max  = sz;
		}
	}
	return true;
}


int WFIFOPACKET(int fd, const NSocket::IPacket& p)
{
	if( !session_isValid(fd) )
		return 0;
	memcpy(WFIFOP(fd,0), p.data(), p.length());
	return WFIFOSET(fd, p.length());
}

int WFIFOSET(int fd,size_t len)
{
	size_t newreserve;

	if( session_isValid(fd) && session[fd]->wdata )
	{
		// we have written len bytes to the buffer already
		session[fd]->wdata_size += len;

		if( session[fd]->wdata_size > session[fd]->wdata_max )
		{	// we had a buffer overflow already
			unsigned long ip = session[fd]->client_ip;
			ShowError("socket: Buffer Overflow. Connection %d (%d.%d.%d.%d). has written %d bytes (%d allowed).\n"
				CL_SPACE"error not recoverable, quitting.\n",
				fd, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip)&0xFF, session[fd]->wdata_size+len, session[fd]->wdata_max);
			exit(1);
		}

		// always keep a WFIFO_SIZE reserve in the buffer
		newreserve = session[fd]->wdata_size + WFIFO_SIZE; 

		send_from_fifo(fd);

		// realloc after sending
		realloc_writefifo(fd, newreserve); 
	}
	return 0;
}

int RFIFOSKIP(int fd, size_t len)
{
	if( session_isValid(fd) )
	{
		struct socket_data *s=session[fd];
		if( session_isActive(fd) )
		{
			if( s->rdata_pos + len > s->rdata_size ) 
			{	// this should not happen
				ShowError("Read FIFO is skipping more data then it has (%i<%i).\n",RFIFOREST(fd),len);
				s->rdata_pos = s->rdata_size;
			}
			else
			{
				s->rdata_pos += len;
			}
		}
		else
		{	//not active just clear the buffer
			s->rdata_size = 0;
			s->rdata_pos  = 0;
		}
	}
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
//
// Creating Sockets and Sessions
//
///////////////////////////////////////////////////////////////////////////////
int connect_client(int listen_fd)
{
	SOCKET sock;
	int fd;
	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);

	// quite dangerous, app is dead in this case
	// should exit or restart the listener
	if( !session_isActive(listen_fd) )
		return -1;

	sock = accept(SessionGetSocket(listen_fd),(struct sockaddr*)&client_address,&len);
	if(sock==INVALID_SOCKET) 
	{	// same here, app might have passed away
		ShowError("accept: %s\n",
			basics::sockerrmsg(basics::sockerrno()));
		return -1;
	}
	else if( !const_cast<basics::ipfilter&>(*ddos).access_from(ntohl(client_address.sin_addr.s_addr)) )
	{	// const_cast because a parameter by default only returns const references
		// to prevent accidental assignments to the parameter value
		closesocket(sock);
		return -1;
	}
	else
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif
	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	//ShowMessage("connect_client: %d <- %d\n",listen_fd, fd);

	if(fd<0 || session[fd]) 
	{	
		closesocket(sock);
		ShowWarning("socket insert %i %p", fd, session[fd]);
		return -1;
	}
	socket_setopts(sock);

	session[fd] = new struct socket_data();
	////////////////
	// init members
	// move to constructor
	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->rdata_tick   = last_tick;	
	
	session[fd]->rdata = new unsigned char[RFIFO_SIZE];
	session[fd]->rdata_max  = RFIFO_SIZE;
	session[fd]->rdata_size = 0;
	session[fd]->rdata_pos  = 0;

	session[fd]->wdata = new unsigned char[WFIFO_SIZE];
	session[fd]->wdata_max  = WFIFO_SIZE;
	session[fd]->wdata_size = 0;

	session[fd]->client_ip    = ntohl(client_address.sin_addr.s_addr);

	session[fd]->func_recv    = recv_to_fifo;
	session[fd]->func_send    = send_from_fifo;
	session[fd]->func_parse   = default_func_parse;
	session[fd]->func_term    = NULL;
	session[fd]->func_console = NULL;
	
	session[fd]->user_session = NULL;

	ShowStatus("Incoming connection from %d.%d.%d.%d:%i\n",
		(session[fd]->client_ip>>24)&0xFF,(session[fd]->client_ip>>16)&0xFF,(session[fd]->client_ip>>8)&0xFF,(session[fd]->client_ip)&0xFF,
		ntohs(client_address.sin_port),fd );
	return fd;
}

int make_listen(unsigned long ip, unsigned short port)
{
	struct sockaddr_in server_address;
	SOCKET sock;
	int fd;
	int result;

	sock = socket( AF_INET, SOCK_STREAM, 0 );
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif
	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = htonl( ip );
	server_address.sin_port        = htons(port);

	result = bind(sock, (struct sockaddr*)&server_address, sizeof(server_address));
	if( result == SOCKET_ERROR ) {
		ShowError("bind: %s\n"CL_SPACE"error not recoverable, quitting.\n",
			basics::sockerrmsg(basics::sockerrno()));
		closesocket(sock);
		exit(1);
	}
	result = listen( sock, 5 );
	if( result == SOCKET_ERROR ) {
		ShowError("listen: %s\n"CL_SPACE"error not recoverable, quitting.\n",
			basics::sockerrmsg(basics::sockerrno()));
		closesocket(sock);
		ShowError(CL_SPACE"error not recoverable, quitting.\n");
		exit(1);
	}

	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0 || session[fd]) 
	{	
		closesocket(sock);
		ShowWarning("socket insert %i %p", fd, session[fd]);
		return -1;
	}
	socket_setopts(sock);

	session[fd] = new struct socket_data();
	////////////////
	// init members
	// move to constructor
	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->rdata_tick   = 0;	
	
	session[fd]->rdata      = NULL;
	session[fd]->rdata_max  = 0;
	session[fd]->rdata_size = 0;
	session[fd]->rdata_pos  = 0;

	session[fd]->wdata      = NULL;
	session[fd]->wdata_max  = 0;
	session[fd]->wdata_size = 0;

	session[fd]->client_ip    = ip;

	session[fd]->func_recv    = connect_client;
	session[fd]->func_send    = NULL;
	session[fd]->func_parse   = NULL;
	session[fd]->func_term    = NULL;
	session[fd]->func_console = NULL;
	
	session[fd]->user_session = NULL;

	if(ip==INADDR_ANY)
	{	size_t i;
		ShowStatus("Open listen ports on localhost");
		for(i=0; i<basics::ipaddress::GetSystemIPCount(); ++i)
			ShowMessage(", %d.%d.%d.%d:%i",basics::ipaddress::GetSystemIP(i)[3],basics::ipaddress::GetSystemIP(i)[2],basics::ipaddress::GetSystemIP(i)[1],basics::ipaddress::GetSystemIP(i)[0],port);
		ShowMessage("\n");
	}
	else
		ShowStatus("Open listen port on %d.%d.%d.%d:%i\n",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF,port);
	return fd;
}
int make_connection(unsigned long ip, unsigned short port)
{
	struct sockaddr_in server_address;
	int fd;
	SOCKET sock;
	int result;

	sock = socket( AF_INET, SOCK_STREAM, 0 );
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif

	server_address.sin_family		= AF_INET;
	server_address.sin_addr.s_addr	= htonl( ip );
	server_address.sin_port			= htons(port);
	
	ShowStatus("Connecting to %d.%d.%d.%d:%i\n",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF,port);

	result = connect(sock, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));
	if( result < 0 )
	{
		closesocket(sock);
		ShowWarning("Connect failed\n");
		return -1;
	}
	else
		ShowStatus("Connect ok\n");

	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0 || session[fd]) 
	{	
		closesocket(sock);
		ShowWarning("socket insert %i %p", fd, session[fd]);
		return -1;
	}

	// set nonblocking after connecting so we can use the blocking timeout
	// connecting nonblocking sockets would need to wait until connection is established
	socket_setopts(sock);

	session[fd] = new struct socket_data();
	////////////////
	// init members
	// move to constructor
	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->rdata_tick   = last_tick;	
	
	session[fd]->rdata = new unsigned char[RFIFO_SIZE];
	session[fd]->rdata_max  = RFIFO_SIZE;
	session[fd]->rdata_size = 0;
	session[fd]->rdata_pos  = 0;

	session[fd]->wdata = new unsigned char[WFIFO_SIZE];
	session[fd]->wdata_max  = WFIFO_SIZE;
	session[fd]->wdata_size = 0;

	session[fd]->client_ip    = ip;

	session[fd]->func_recv    = recv_to_fifo;
	session[fd]->func_send    = send_from_fifo;
	session[fd]->func_parse   = default_func_parse;
	session[fd]->func_term    = NULL;
	session[fd]->func_console = NULL;
	
	session[fd]->user_session = NULL;

	return fd;
}

///////////////////////////////////////////////////////////////////////////////
//
// Console Reciever [Wizputer]
//
///////////////////////////////////////////////////////////////////////////////

int console_recieve(int fd) {
	int n;
	char buf[128];
	memset(buf,0,sizeof(buf));

	if( !session_isActive(fd) )
		return -1;

	n = read(fd, buf , 128);
	if( (n < 0) || !session[fd]->func_console )
		ShowMessage("Console input read error\n");
	else if(session[fd]->func_console)
		session[fd]->func_console(buf);
	return 0;
}

void set_defaultconsoleparse(int (*defaultparse)(const char*))
{
	default_console_parse = defaultparse;
}

int null_console_parse(const char *buf)
{
	ShowMessage("null_console_parse : %s\n",buf);
	return 0;
}

// Console Input [Wizputer]
int start_console(void) {
	
	SOCKET sock=0; // default zero socket, is not used regulary
	int fd;
    
	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0) 
	{	ShowWarning("Socket Insert failed");
		return -1;
	}

	session[fd] = new struct socket_data();
	////////////////
	// init members
	// move to constructor
	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->rdata_tick   = last_tick;	
	
	session[fd]->rdata = new unsigned char[RFIFO_SIZE];
	session[fd]->rdata_max    = RFIFO_SIZE;
	session[fd]->rdata_size   = 0;
	session[fd]->rdata_pos    = 0;

	session[fd]->wdata = new unsigned char[WFIFO_SIZE];
	session[fd]->wdata_max    = WFIFO_SIZE;
	session[fd]->wdata_size   = 0;

	session[fd]->client_ip    = (uint32)INADDR_ANY;

	session[fd]->func_recv    = console_recieve;
	session[fd]->func_send    = NULL;
	session[fd]->func_parse   = NULL;
	session[fd]->func_term    = NULL;
	session[fd]->func_console = default_console_parse;
	
	session[fd]->user_session = NULL;

	return 0;
}   

///////////////////////////////////////////////////////////////////////////////
//
// Incoming Data Processing
//
///////////////////////////////////////////////////////////////////////////////
void process_read(size_t fd)
{
	if( session_isValid(fd) )
	{
		if( session[fd]->func_recv )
			session[fd]->func_recv((int)fd);

		if(session[fd]->rdata_size==0 && session[fd]->flag.connected)
			return;

		if( session[fd]->func_parse )
			session[fd]->func_parse((int)fd);

		// session could be deleted in func_parse so better check again
		if(session[fd] && session[fd]->rdata) 
		{	//RFIFOFLUSH(fd);
			if( session[fd]->rdata_size>session[fd]->rdata_pos )
			{
				memmove(session[fd]->rdata, session[fd]->rdata+session[fd]->rdata_pos, session[fd]->rdata_size-session[fd]->rdata_pos);
				session[fd]->rdata_size = RFIFOREST(fd);
				session[fd]->rdata_pos  = 0;
			}
			else
			{
				session[fd]->rdata_size = 0;
				session[fd]->rdata_pos  = 0;
			}
		}

	}
}
///////////////////////////////////////////////////////////////////////////////
void process_write(size_t fd)
{
	if( session_isValid(fd) )
	{
		if( session[fd]->func_send )
			session[fd]->func_send((int)fd);
	}
}

///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
// the windows fd_set structures are simple
// typedef struct fd_set {
//        u_int fd_count;               // how many are SET?
//        SOCKET  fd_array[FD_SETSIZE];   // an array of SOCKETs
// } fd_set;
// 
// the select the sets, the correct fd_count and the array
// and access the signaled sockets one by one
size_t process_fdset(fd_set* fds, void(*func)(size_t) )
{
	size_t i;
	size_t fd;

	if(func)
	for(i=0;i<fds->fd_count;++i)
	{
		if( SessionFindSocket( fds->fd_array[i], fd ) )
		{
			func(fd);
		}
	}
	return fds->fd_count;
}
// just a dummy to keep win/unix structure consistent
inline size_t process_fdset2(fd_set* fds, void(*func)(size_t) )
{	return process_fdset(fds, func);
}
///////////////////////////////////////////////////////////////////////////////
#else//!WIN32
///////////////////////////////////////////////////////////////////////////////
//
// unix uses a bit array where the socket number equals the 
// position in the array, so finding sockets inside that array 
// is not that easy exept the socket is knows before
// so this method here goes through the bit array 
// and build the socket number from the position 
// where a set bit was found. 
// since we can skip 32 sockets all together when none is set
// we can travel quite fast through the array
size_t process_fdset(fd_set* fds, void(*func)(size_t) )
{
	size_t fd,c=0;
	SOCKET sock;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)' for the whole array
	unsigned long	max = howmany(fd_max, NFDBITS);

	if(func)
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = __FDS_BITS(fds)[nfd];
		while( bits )
		{	// method 1
			// calc the highest bit with log2 and clear it from the field
			// this method is especially fast 
			// when only a few bits are set in the field
			// which usually happens on read events
			val = basics::log2( bits );
			bits ^= (1<<val);	
			// build the socket number
			sock = nfd*NFDBITS + val;

			///////////////////////////////////////////////////
			// call the user function
			fd = sock; // should use SessionFindSocket for consitency but this is ok
			func(fd);
			c++;
		}
		// go to next field position
		nfd++;
	}
	return c;
}

size_t process_fdset2(fd_set* fds, void(*func)(size_t) )
{
	size_t fd, c=0;
	SOCKET sock;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)'
	unsigned long	max = howmany(fd_max, NFDBITS);

	if(func)
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = __FDS_BITS(fds)[nfd];
		val = 0;
		while( bits )
		{	// method 2
			// calc the next set bit with shift/add
			// therefore copy the value from fds_bits 
			// array to an unsigned type (fd_bits is an field of long)
			// otherwise it would not shift the MSB
			// the shift add method is faster if many bits are set in the field
			// which is usually valid for write operations on large fields
			while( !(bits & 1) )
			{
				bits >>= 1;
				val ++;
			}
			//calculate the socket number
			sock = nfd*NFDBITS + val;
			// shift one more for the next loop entrance
			bits >>= 1;
			val ++;

			///////////////////////////////////////////////////
			// call the user function
			fd = sock; // should use SessionFindSocket for consitency but this is ok
			func(fd);
			c++;
		}
		// go to next field position
		nfd++;
	}
	return c; // number of processed sockets
}
///////////////////////////////////////////////////////////////////////////////
#endif//!WIN32
///////////////////////////////////////////////////////////////////////////////

#ifdef SOCKET_DEBUG_LOG
static char temp_buffer1[1024],temp_buffer2[1024];
void debug_collect(int fd)
{	static char buf[32];
	snprintf(buf,32,"(%i,%i%i%i)",fd,session[fd]->flag.connected,session[fd]->flag.marked,session[fd]->flag.remove);
	strcat(temp_buffer1,buf);
}
void debug_output()
{
	if( 0!=strcmp(temp_buffer1,temp_buffer2) )
	{
		printf("[%lu]%s\n",(unsigned long)last_tick, temp_buffer1);
		fflush(stdout);
		memcpy(temp_buffer2, temp_buffer1,sizeof(temp_buffer1));
	}
	*temp_buffer1=0;
}
#endif

int do_sendrecv(int next)
{
	fd_set rfd,wfd;
	struct timeval timeout;
	int ret;
	size_t fd;
	size_t cnt=0;

	// update global tick_timer
	last_tick = time(NULL);

	FD_ZERO(&wfd);
	FD_ZERO(&rfd);
#ifdef SOCKET_DEBUG_PRINT
	printf("\r[%ld]", (unsigned long)last_tick);
#endif
	for(fd=0; fd<fd_max; ++fd)
	{
		if( session[fd] )
		{
#if defined(SOCKET_DEBUG_PRINT)
			printf("(%i,%i%i%i)",fd,session[fd]->flag.connected,session[fd]->flag.marked,session[fd]->flag.remove);
			fflush(stdout);
#elif defined(SOCKET_DEBUG_LOG)
			debug_collect(fd);
#endif
			if( session[fd]->rdata_tick && (last_tick - session[fd]->rdata_tick > (long)stall_time()) )
			{	// emulate a disconnection
				session[fd]->flag.connected = false;
				// and call the read function
				process_read(fd);
				// it should come out with a set remove or marked flag		
			}

			if( session[fd]->flag.remove )
			{	// delete sessions scheduled for remove here
				// we have to go through the field anyway
				// and this is the safest place for deletions
				session_Delete((int)fd);
				continue;
			}
			else if( session[fd]->flag.connected )
			{
				// build a read-fd_set with connected sockets only
				FD_SET( SessionGetSocket(fd),&rfd);

				// build a write-fd_set with all sockets that have something to write
				if( session[fd]->wdata_size )
					FD_SET( SessionGetSocket(fd),&wfd);
			}
			// double usage of fd_max as max session number and to call select is valid
			// because on unix the fd field position equals the socket identifier
			// and on windows the first parameter of select is ignored
			cnt = fd; 
		}
	}
#if defined SOCKET_DEBUG_PRINT
	printf("\n");
#elif defined SOCKET_DEBUG_LOG
	debug_output();
#endif

	fd_max = cnt+1;

	div_t d = div(next,1000);
	timeout.tv_sec  = d.quot;
	timeout.tv_usec = d.rem*1000;

	// wait until timeout or some events on the line
	ret = select(fd_max,&rfd,&wfd,NULL,&timeout);

	// check if something happend before timeout
	if(ret>0)
	{
		// process writes
		process_fdset(&wfd,process_write);

		// process readings and parse them immediately
		process_fdset(&rfd,process_read);
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// delayed session removal timer entry
///////////////////////////////////////////////////////////////////////////////
int session_WaitClose(int tid, unsigned long tick, int id, basics::numptr data) 
{
	if( session_isValid(id) && session[id]->flag.marked )
	{	// set session to offline
		// it will be removed by do_sendrecv
		session[id]->flag.connected	= false;
		session[id]->flag.marked	= false;
		session[id]->flag.remove	= true;
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////
// delayed session removal control
///////////////////////////////////////////////////////////////////////////////
bool session_SetWaitClose(int fd, unsigned long timeoffset)
{
	if( session_isValid(fd) && !session[fd]->flag.marked && !session[fd]->flag.remove )
	{	// set the session to marked state
		session[fd]->flag.marked	= true;

		if(session[fd]->user_session == NULL)
			// limited timer, just to send information.
			add_timer(gettick() + 1000, session_WaitClose, fd, 0);
		else
			add_timer(gettick() + timeoffset, session_WaitClose, fd, 0);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// Session Control
//
///////////////////////////////////////////////////////////////////////////////
bool session_Delete(int fd)
{
	if( session_isValid(fd) )
	{	// socket is marked for delayed deletion
		// but is not called from the delay handler
		// so we skip deletion here and wait for the handler
		if( session[fd]->flag.marked && session[fd]->flag.remove )
		{
			session[fd]->flag.connected = false;
			session[fd]->flag.remove = false;
			return false;
		}

		// call the termination function to save/cleanup/etc
		if( session[fd]->func_term )
			session[fd]->func_term(fd);

		// and clean up
		if(session[fd]->rdata){			delete[] session[fd]->rdata; session[fd]->rdata=NULL; }
		if(session[fd]->wdata){			delete[] session[fd]->wdata; session[fd]->wdata=NULL;  }
		if(session[fd]->user_session){	delete session[fd]->user_session; session[fd]->user_session=NULL;  }
		delete session[fd];
		session[fd]=NULL;

		SessionRemoveIndex( fd );
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// Initialisation and Cleanup
//
///////////////////////////////////////////////////////////////////////////////
void socket_init(void)
{
	basics::loadParam("conf/socket_athena.conf");

	add_timer_func_list(session_WaitClose, "session_WaitClose");
}

void socket_final(void)
{
	size_t fd;
	for(fd=0; fd<fd_max; ++fd){
		// don't use session_Delete here
		// just force deletion of the sessions
		if( session_isValid(fd) )
		{
			if(session[fd]->rdata)			delete[] session[fd]->rdata;
			if(session[fd]->wdata)			delete[] session[fd]->wdata;
			if(session[fd]->user_session)	delete session[fd]->user_session;

			delete session[fd];
			session[fd]=NULL;
			SessionRemoveIndex( fd );
		}
	}
}



//#define WHATISMYIP // for using whatismyip.com
// default using checkip.dyndns.org
bool detect_WAN(basics::ipaddress& wanip)
{	// detect WAN
	basics::minisocket ms;

#ifdef WHATISMYIP
	static const char* query = "GET / HTTP/1.1\r\nHost: whatismyip.com\r\n\r\n";
	if( !ms.connect("http://whatismyip.com") )
#else
	static const char* query = "GET / HTTP/1.1\r\nHost: checkip.dyndns.org\r\n\r\n";
	if( !ms.connect("http://checkip.dyndns.org") )
#endif
		return false;
	
	ms.write((const unsigned char*)query, strlen(query));
	if( ms.waitfor(1000) )
	{
		unsigned char buffer[1024];
		//size_t len=
			ms.read(buffer, sizeof(buffer));
		buffer[sizeof(buffer)-1]=0;

#ifdef WHATISMYIP
		static const basics::CRegExp regex("WhatIsMyIP.com -\\s+([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
#else
		static const basics::CRegExp regex("Current IP Address:\\s+([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
#endif
		regex.match((const char*)buffer);
		wanip = (const char*)regex[1];
		if(wanip != basics::ipaddress::GetSystemIP() )
		{
			ShowStatus("WANIP dectected as: %s\n", (const char*)basics::tostring(wanip));
			return true;
		}
	}
	return false;
}

//check if wan has been dropped
bool dropped_WAN(const basics::ipaddress& wanip, const ushort wanport)
{	
	if( wanip != basics::ipany )
	{	// try to open a socket to wanip
		struct sockaddr_in server_address;

		SOCKET sock = socket( AF_INET, SOCK_STREAM, 0 );

		server_address.sin_family		= AF_INET;
		server_address.sin_addr.s_addr	= htonl( wanip );
		server_address.sin_port			= htons(wanport);

		const int result = connect(sock, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));

		// only need the connection result
		closesocket(sock);

		// returns true, when connection has failed
		return (result < 0);
	}
	// return true when wanip was not set up
	return true;
}

//
bool initialize_WAN(basics::ipset& set, const ushort defaultport)
{
	basics::ipaddress wanip;
	bool ret = detect_WAN(wanip);
	if( ret &&
		set.WANIP() != wanip && 
		set.LANIP() != wanip &&
		!set.SetLANIP(wanip) )
	{
		ShowStatus("Setting WAN IP %s (overwriting config).\n", (const char*)basics::tostring(wanip));
		set.SetWANIP(wanip);
		if( set.LANMask() == basics::ipany )
		{
			set.LANMask() = basics::ipaddress(0xFFFFFF00ul);
			ShowStatus("Setting LAN Mask to %s (overwriting config).\n", (const char*)basics::tostring(set.LANMask()));
		}
		if( set.WANPort() == 0 )
		{
			ShowStatus("Setting default WAN Port to %d.\n", defaultport);
			set.WANPort() = defaultport;
		}
	}
	return ret;
}


