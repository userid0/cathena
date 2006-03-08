#ifndef __BASESOCKET_H__
#define __BASESOCKET_H__

#include "basetypes.h"
#include "baseobjects.h"
#include "baseinet.h"
#include "baseregex.h"


///////////////////////////////////////////////////////////////////////////////
// maximum accept time of a socket in microseconds
// it defines the maximum time after an incoming socket is accepted
// and ready to read from (it does not affect read/write, only accept)
// half a second is a good praktical value
#define SOCKETACCEPTTIME	500000


// define to use non-blocking sockets instead of blocking sockets
// nonblocking might be nicer but need periferal tests to
// assure complete read/write operations
//#define NONBLOCKINGSOCKETS


// variation of FD_SETSIZE is not that portable as described
// if possible do not reduce it
// if the number of aquired sockets exceed FD_SETSIZE,
// on unix you cannot use the sockets because they 
// don't fit into the fd_set structre
// on windows machines this does not matter since sockets 
// and FD_SETSIZE are not related as on unix
// on unix it is usually >1024 or windows standard is 64
// 64bit systems define FD_SETSIZE usually as 65536
// #define FD_SETSIZE set a number if necessary

// a worker thread is opened for each number of FD_WORKERSIZE sockets
// to get a more responsive socket behaviour with a huge number of sockets
// FD_WORKERSIZE can be defined smaller than FD_SETSIZE on unix
// so the possible FD_SETSIZE sockets are devided to 
// max (FD_SETSIZE/FD_WORKERSIZE) independend worker threads.
// on windows it is possible to just reduce the FD_SETSIZE 
// and have FD_WORKERSIZE = FD_SETSIZE for the same effect 
// at reduced memory usage because FD_SETSIZE only limits the
// size if the fd_set structure and not the number of usable sockets
#define FD_WORKERSIZE FD_SETSIZE



//////////////////////////////////////////////////////////////////////////
// BSD-compatible socket error codes for Win32
//////////////////////////////////////////////////////////////////////////
// since there is no way to access this structure in a portable way
// we need to headbang at a wall until one breaks			
//typedef	unsigned long	fd_mask;
#ifndef NBBY
#define	NBBY 8
#endif
#ifndef howmany
#define	howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif  
#ifndef NFDBITS
#define	NFDBITS	(sizeof (unsigned long) * NBBY)	// bits per mask
#endif


//////////////////////////////////////////////////////////////////////////
// socketlen type definition
//////////////////////////////////////////////////////////////////////////
#ifndef __socklen_t_defined
  #if defined(__DARWIN__) || defined(WIN32)
    typedef int socklen_t;
  #elif CYGWIN
    // stupid cygwin is not standard conform
    #ifdef socklen_t
      #undef socklen_t
    #endif
    typedef int socklen_t;
  #else// normal unix with undefined socklen_t
    typedef unsigned int socklen_t;
  #endif
  // and set the proper macro
  #define __socklen_t_defined
#endif//__socklen_t_defined
typedef char* sockval_t;


//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// unix specific interface for windows
extern inline int read(SOCKET fd, char*buf, int sz)		
{
	return recv(fd,buf,sz,0); 
}
extern inline int write(SOCKET fd, const char*buf, int sz)	
{
	return send(fd,buf,sz,0); 
}

//////////////////////////////////////////////////////////////////////////
#else
// windows specific interface for non-windows systems
typedef int		SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

// function definitions
extern inline int closesocket(SOCKET fd)		
{
	return close(fd); 
}
extern inline int ioctlsocket(SOCKET fd, long cmd, unsigned long *arg)		
{
	return ioctl(fd,cmd,arg); 
}

#endif






///////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
///////////////////////////////////////////////////////////////////////////////
// dynamic size, unix system independend fd_set replacement
class CFDSET
{
	///////////////////////////////////////////////////////////////////////////
	// class data
	unsigned long*	cArray;		// array pointer
	unsigned long	cSZ;		// alloced size

	///////////////////////////////////////////////////////////////////////////
	// resize the array; only grow, no shrink
	void checksize(size_t pos);
	void copy(const CFDSET& cfd);
public:
	///////////////////////////////////////////////////////////////////////////
	// Construct/Destruct
	CFDSET() : cArray(new unsigned long[FD_SETSIZE/NBBY/sizeof(unsigned long)]),cSZ(FD_SETSIZE/NBBY/sizeof(unsigned long))	{}
	~CFDSET()	{ if(cArray) delete [] cArray; }

	///////////////////////////////////////////////////////////////////////////
	// Copy/Assign
	CFDSET(const CFDSET& cfd) : cArray(NULL),cSZ(0)
	{
		copy(cfd);
	}
	const CFDSET& operator =(const CFDSET& cfd)
	{
		copy(cfd);
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// clear everything
	void clear()
	{
		memset (cArray,0, cSZ*sizeof(unsigned long));
	}
	///////////////////////////////////////////////////////////////////////////
	// set a bit
	void set_bit(int fd)
	{
		if(fd>0)
		{
			// always working method
//			const size_t pos = fd/(NBBY*sizeof(cArray[0]));
//			const size_t bit = fd%(NBBY*sizeof(cArray[0]));
			// bit more optimized but still generous
//			const ldiv_t d = ldiv( fd, (NBBY*sizeof(cArray[0])) );
//			const size_t pos = d.quot;
//			const size_t bit = d.rem;
			// sizeof ulong is quite fixed despite the size change on 64bit
#ifdef __64BIT__
			const size_t pos = fd>>6;	// equals /64
			const size_t bit = fd&0x3F;	// equals %64
#else // 32bit system
			const size_t pos = fd>>5;	// equals /32
			const size_t bit = fd&0x1F;	// equals %32
#endif
			checksize(pos);
			cArray[pos] |= (1<<bit);
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// Clear a bit
	void clear_bit(int fd)
	{
		if(fd>0)
		{
			// always working method
//			const size_t pos = fd/(NBBY*sizeof(cArray[0]));
//			const size_t bit = fd%(NBBY*sizeof(cArray[0]));
			// bit more optimized but still generous
//			const ldiv_t d = ldiv( fd, (NBBY*sizeof(cArray[0])) );
//			const size_t pos = d.quot;
//			const size_t bit = d.rem;
			// sizeof ulong is quite fixed despite the size change on 64bit
#ifdef __64BIT__
			const size_t pos = fd>>6;	// equals /64
			const size_t bit = fd&0x3F;	// equals %64
#else // 32bit system
			const size_t pos = fd>>5;	// equals /32
			const size_t bit = fd&0x1F;	// equals %32
#endif
			checksize(pos);
			cArray[pos] &= ~(1<<bit);
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// Clear a bit
	bool is_set(int fd) const
	{
		if(fd>0)
		{
			// always working method
//			const size_t pos = fd/(NBBY*sizeof(cArray[0]));
//			const size_t bit = fd%(NBBY*sizeof(cArray[0]));
			// bit more optimized but still generous
//			const ldiv_t d = ldiv( fd, (NBBY*sizeof(cArray[0])) );
//			const size_t pos = d.quot;
//			const size_t bit = d.rem;
			// sizeof ulong is quite fixed despite the size change on 64bit
#ifdef __64BIT__
			const size_t pos = fd>>6;	// equals /64
			const size_t bit = fd&0x3F;	// equals %64
#else // 32bit system
			const size_t pos = fd>>5;	// equals /32
			const size_t bit = fd&0x1F;	// equals %32
#endif
			return (pos<cSZ) && (0!=(cArray[pos] & (1<<bit)));
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// Call a function with each set bit
	// version 1 (using log2)
	size_t foreach1( void(*func)(size_t), size_t max) const;

	///////////////////////////////////////////////////////////////////////////
	// Call a function with each set bit
	// version 2 (using shifts)
	size_t foreach2( void(*func)(size_t), size_t max ) const;

	///////////////////////////////////////////////////////////////////////////
	// pretending to be an unix fd_set structure
	operator fd_set*() const
	{
		return (fd_set*)cArray; 
	}
	///////////////////////////////////////////////////////////////////////////
	// size
	int size() const
	{ 
		return cSZ * NFDBITS;
	}
};

#else 

///////////////////////////////////////////////////////////////////////////////
// dynamic size, windows system independend fd_set replacement
class CFDSET
{
	///////////////////////////////////////////////////////////////////////////
	// class data
	// windows
	struct winfdset
	{
		u_int fd_count;				// how many are SET?
		SOCKET  fd_array[1];		// an array of SOCKETs 
									// only one in the struct the others will be alloced outside
	};
	unsigned long	cSZ;			// alloced elements
	struct winfdset *cSet;			// the set struct

	///////////////////////////////////////////////////////////////////////////
	// resize the array; only grow, no shrink
	void checksize();
	void copy(const CFDSET& cfd);
	bool find(SOCKET sock, size_t &pos) const;

public:
	///////////////////////////////////////////////////////////////////////////
	// Construct/Destruct
	CFDSET() : cSet((struct winfdset *) new char[sizeof(struct winfdset)+128*sizeof(SOCKET)]),cSZ(128)	{ cSet->fd_count=0; }
	~CFDSET()	{ if(cSet) delete [] ((char*)cSet); }

	///////////////////////////////////////////////////////////////////////////
	// Copy/Assign
	CFDSET(const CFDSET& cfd) : cSet(NULL),cSZ(0)
	{
		copy(cfd);
	}
	const CFDSET& operator =(const CFDSET& cfd)
	{
		copy(cfd);
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// clear everything
	void clear()
	{
		cSet->fd_count = 0;
	}
	///////////////////////////////////////////////////////////////////////////
	// set a bit
	void set_bit(int fd);
	///////////////////////////////////////////////////////////////////////////
	// Clear a bit
	void clear_bit(int fd);

	///////////////////////////////////////////////////////////////////////////
	// Clear a bit
	bool is_set(int fd) const;

	///////////////////////////////////////////////////////////////////////////
	// Call a function with each set bit
	size_t foreach1( void(*func)(size_t), size_t max) const;

	///////////////////////////////////////////////////////////////////////////
	// Call a function with each set bit
	size_t foreach2( void(*func)(size_t), size_t max ) const
	{	// no different approaches on windows
		return foreach1( func, max );
	}
	///////////////////////////////////////////////////////////////////////////
	// pretending to be an fd_set structure
	operator fd_set*() const	
	{
		return (fd_set*)cSet; 
	}
	///////////////////////////////////////////////////////////////////////////
	// size
	int size() const
	{ 
		return cSZ;
	}
};
#endif











//////////////////////////////////////////////////////////////////////////
class minisocket
{
	SOCKET sock;
public:
	minisocket() : sock(INVALID_SOCKET)
	{
		this->create();
	}
	~minisocket()
	{
		this->close();
	}
	bool connect(ipaddress& ip, ushort port)
	{
		if( create() )
		{
			struct sockaddr_in addr;
			addr.sin_family			= AF_INET;
			addr.sin_addr.s_addr	= htonl( ip );
			addr.sin_port			= htons( port );

			if( 0 <= ::connect(this->sock, (struct sockaddr *)(&addr),sizeof(struct sockaddr_in)) )
				return true;
			this->close();
		}
		return false;
	}
	bool connect(const char* address)
	{
		// we are lazy here
		CRegExp regex("(?:([^/:]+)://)?([^:]+)(?::(\\d+))?");
		if( regex.match(address) )
		{
			ipaddress ip = ::hostbyname(regex[2]);
			ushort port = 80;
			if(regex[1]=="http")
				port = 80;
			else if(regex[1]=="ftp")
				port = 21;
			if( atoi(regex[4]) )
				port = atoi(regex[4]);
			return this->connect(ip, port);
		}
		return false;
	}
	bool create()
	{
		if(this->sock==INVALID_SOCKET)
			this->sock = ::socket( AF_INET, SOCK_STREAM, 0 );
		return (this->sock!=INVALID_SOCKET);
	}
	bool close()
	{
		if(this->sock!=INVALID_SOCKET)
		{
			::closesocket(this->sock);
			this->sock = INVALID_SOCKET;
		}
		return (this->sock == INVALID_SOCKET);
	}
	size_t read(unsigned char*buf, size_t maxlen)
	{
		unsigned long arg = 0;
		int rv = ::ioctlsocket(this->sock, FIONREAD, &arg);
		if( (rv == 0) && (arg > 0) )
		{	// we are reading 'arg' bytes of data from the socket
			
			if(!buf)
			{	// requested buffer length
				return arg;
			}
			else
			{	// read up to maxlen
				if( arg > maxlen ) 
					arg = maxlen;
				int len=::read(sock,(char *)buf,arg);
				return len == (int)arg;
			}
		}
		return 0;
	}
	bool write(const unsigned char*buf, size_t len)
	{
		return (int)len == ::write(this->sock, (const char*)buf, len);
	}
	bool waitfor(ulong millisec)
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(sock,&fdset);
		struct timeval timeout;
		timeout.tv_sec  = millisec/1000;
		timeout.tv_usec = millisec%1000*1000;
		return 0!=::select(sock+1,&fdset,NULL,NULL,&timeout);
	}
};






#endif//__BASESOCKET_H__

