#ifndef	_BASE_H_
#define _BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>

#include <assert.h>



#if defined(__WIN32__) || defined(__WIN32) || defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif



//////////////////////////////
#ifdef _WIN32
//////////////////////////////
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <conio.h>
//////////////////////////////
#else
//////////////////////////////

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <unistd.h>



#ifndef SIOCGIFCONF
#include <sys/sockio.h> // SIOCGIFCONF on Solaris, maybe others?
#endif



//////////////////////////////
#endif
//////////////////////////////



#ifndef NULL
#define NULL (void *)0
#endif

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define Assert(EX) //assert(EX)



//////////////////////////////
#ifdef _WIN32/////////////////
//////////////////////////////

// keywords
#ifndef __cplusplus
#define inline __inline
#endif

// defines
// txtやlogなどの書き出すファイルの改行コード
#define RETCODE	"\r\n"	// (CR/LF：Windows系)
#define RET RETCODE

#define PATHSEP '\\'



// abnormal function definitions
#define vsnprintf			_vsnprintf
#define snprintf			_snprintf


//#define strcasecmp		in utils.h
//#define strncasecmp		in utils.h


#define	sleep				Sleep

#define close(fd)			closesocket(fd)
#define read(fd,buf,sz)		recv(fd,buf,sz,0)
#define write(fd,buf,sz)	send(fd,buf,sz,0)


// missing functions and helpers
extern inline void gettimeofday(struct timeval *timenow, void *dummy)
{
	time_t t;
	t = clock();
	timenow->tv_usec = t;
	timenow->tv_sec = t / CLK_TCK;
	return;
}

extern inline void socket_nonblocking(int fd)
{
	unsigned long val = 1;
	ioctlsocket(fd, FIONBIO, &val);
}


//////////////////////////////
#else/////////////////////////
//////////////////////////////


// keywords

// defines

// txtやlogなどの書き出すファイルの改行コード
#define RETCODE "\n"	// (LF：Unix系）
#define RET RETCODE

#define PATHSEP '/'

// typedefs

typedef int		SOCKET;

// abnormal function definitions



// missing functions and helpers

extern inline unsigned long GetTickCount() {
	 struct timeval tv;
	 gettimeofday( &tv, NULL );
	 return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

extern inline void socket_nonblocking(int fd)
{
	int result;
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
}

//////////////////////////////
#endif////////////////////////
//////////////////////////////



//////////////////////////////
#ifdef __GNUC__ // GCC has variable length arrays

#define CREATE_BUFFER(name, type, size)		type name[size]
#define DELETE_BUFFER(name)					

#else			// others don't, so we emulate them

#define CREATE_BUFFER(name, type, size)		type *name=(type*)aCalloc(size,sizeof(type))
#define DELETE_BUFFER(name)					aFree(name);name=NULL

#endif
//////////////////////////////



//////////////////////////////////////////////////////////////////////////
// portable 64-bit integers
//////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(__BORLANDC__)
 typedef __int64             int64;
 typedef unsigned __int64    uint64;
#define LLCONST(a) (a##i64)
#else
 typedef long long           int64;
 typedef unsigned long long  uint64;
#define LLCONST(a) (a##ll)
#endif

#ifndef INT64_MIN
#define INT64_MIN  (LLCONST(-9223372036854775807)-1)
#endif
#ifndef INT64_MAX
#define INT64_MAX  (LLCONST(9223372036854775807))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX (LLCONST(18446744073709551615u))
#endif



/*
** byte word dword access
*/
extern inline unsigned char GetByte(unsigned long val, size_t num)
{
	switch(num)
	{
	case 0:
		return (unsigned char)((val & 0x000000FF)      );
	case 1:
		return (unsigned char)((val & 0x0000FF00)>>0x08);
	case 2:
		return (unsigned char)((val & 0x00FF0000)>>0x10);
	case 3:
		return (unsigned char)((val & 0xFF000000)>>0x18);
	default:
		return 0;	//better throw something here
	}
}
extern inline unsigned short GetWord(unsigned long val, size_t num)
{
	switch(num)
	{
	case 0:
		return (unsigned char)((val & 0x0000FFFF)      );
	case 1:
		return (unsigned char)((val & 0xFFFF0000)>>0x10);
	default:
		return 0;	//better throw something here
	}	
}
extern inline unsigned short MakeWord(unsigned char byte0, unsigned char byte1)
{
	return byte0 | (byte1<<0x08);
}
extern inline unsigned long MakeDWord(unsigned short word0, unsigned short word1)
{
	return 	  ((unsigned long)word0)
			| ((unsigned long)word1<<0x10);
}

#ifdef __cplusplus
extern inline unsigned long MakeDWord(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3)
{
	return 	  ((unsigned long)byte0)
			| ((unsigned long)byte1<<0x08)
			| ((unsigned long)byte2<<0x10)
			| ((unsigned long)byte3<<0x18);
}
#endif

// Check the byte-order of the CPU.
#define LSB_FIRST        0
#define MSB_FIRST        1
extern inline int CheckByteOrder(void)
{
    static short  w = 0x0001;
    static char  *b = (char *) &w;
    return(b[0] ? LSB_FIRST : MSB_FIRST);
}

// Swap the bytes within a 16-bit WORD.
extern inline void SwapTwoBytes(char *p)
{	if(p)
	{	char tmp =p[0];
		p[0] = p[1];
		p[1] = tmp;
	}
}
#ifdef __cplusplus
extern inline unsigned short SwapTwoBytes(unsigned short w)
{
    return	  ((w & 0x00FF) << 0x08)
			| ((w & 0xFF00) >> 0x08);
}
#endif


// Swap the bytes within a 32-bit DWORD.
extern inline void SwapFourBytes(char *p)
{	if(p)
	{	char tmp;
		tmp  = p[0];
		p[0] = p[3];
		p[3] = tmp;
		tmp  = p[1];
		p[1] = p[2];
		p[2] = tmp;
	}
}
#ifdef __cplusplus
extern inline unsigned long SwapFourBytes(unsigned long w)
{
    return	  ((w & 0x000000FF) << 0x18)
			| ((w & 0x0000FF00) << 0x08)
			| ((w & 0x00FF0000) >> 0x08)
			| ((w & 0xFF000000) >> 0x18);
}
#endif






#ifdef __cplusplus

///////////////////////////////////////////////
///////////////////////////////////////////////
// starting over in another project and transfering 
// after coding and testing is finished

template<class T> class Buffer
{
	T		*cArray;
public:

	Buffer(size_t sz=0):cArray(NULL)
	{	
		if(sz>0)
			cArray = new T[sz];
	}
	~Buffer()
	{	
		if(cArray)
			delete[] cArray;
	}

	operator T*()	{return cArray;}
};

///////////////////////////////////////////////
///////////////////////////////////////////////

template<class T> class autoptr
{


};


template <class T> inline void swap(T &i1, T &i2)
{	T dummy = i1; i1=i2; i2=dummy;
}



#else // not cplusplus


typedef int bool;
#define false	(1==0)
#define true	(1==1)

#ifndef swap
#define swap(a,b) ((a == b) || ((a ^= b), (b ^= a), (a ^= b)))
//#define swap(a,b) { int temp=a; a=b; b=temp;}
#endif swap

#endif // not cplusplus



#endif//_BASE_H_

