#ifndef	_BASE_H_
#define _BASE_H_

//////////////////////////////////////////////////////////////////////////
// basic include for all basics
// introduces types and global functions
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// some global switches
//////////////////////////////////////////////////////////////////////////

#define COUNT_GLOBALS		// enables countage of created objects
#define CHECK_BOUNDS		// enables boundary check for arrays and lists
#define CHECK_LOCKS			// enables check of locking/unlocking sync objects
//#define SINGLETHREAD		// builds without multithread guards
//#define CHECK_EXCEPTIONS	// use exceptions for "exception" handling
#define _USE_32BIT_TIME_T	// use 32 bit time variables on windows



//////////////////////////////////////////////////////////////////////////
// setting some defines on platforms
//////////////////////////////////////////////////////////////////////////
#if (defined(__WIN32__) || defined(__WIN32) || defined(_WIN32) || defined(_MSC_VER) || defined(__BORLANDC__)) && !defined(WIN32)
#define WIN32
#endif

// __APPLE__ is the only predefined macro on MacOS X
#if defined(__APPLE__)
#define __DARWIN__
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif


//////////////////////////////////////////////////////////////////////////
// setting some defines for compile modes
//////////////////////////////////////////////////////////////////////////
// check global objects count and array boundaries in debug mode
#if defined(DEBUG) && !defined(COUNT_GLOBALS)
#define COUNT_GLOBALS
#endif
#if defined(DEBUG) && !defined(CHECK_BOUNDS)
#define CHECK_BOUNDS
#endif
#if defined(DEBUG) && !defined(CHECK_LOCKS)
#define CHECK_LOCKS
#endif


//////////////////////////////////////////////////////////////////////////
// System specific defined
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
//////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_	// prevent inclusion of winsock.h
#define _USE_32BIT_TIME_T
//////////////////////////////
#endif
//////////////////////////////



//////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////
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
#include <limits.h>
#include <signal.h>

#include <assert.h>

//////////////////////////////
#ifdef WIN32
//////////////////////////////
#pragma warning(disable : 4996)	// disable deprecated warnings

#include <windows.h>
#include <winsock2.h>
#include <conio.h>

//////////////////////////////
#else
//////////////////////////////

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#ifndef FIONREAD
#include <sys/filio.h>	// FIONREAD on Solaris, might conflict on other systems
#endif
#ifndef SIOCGIFCONF
#include <sys/sockio.h> // SIOCGIFCONF on Solaris, maybe others?
#endif

//////////////////////////////
#endif
//////////////////////////////





//////////////////////////////////////////////////////////////////////////
// no c support anymore
//////////////////////////////////////////////////////////////////////////
#ifndef __cplusplus
#  error "this is C++ source"
#endif





//////////////////////////////////////////////////////////////////////////
// useful typedefs
//////////////////////////////////////////////////////////////////////////

typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned char   uchar;
typedef signed char     schar;
typedef char*           pchar;
typedef const char*     cchar;
typedef void*           ptr;
typedef int*            pint;


/////////////////////////////
#ifdef __cplusplus
//////////////////////////////

// starting over in another project and transfering 
// after coding and testing is finished


#ifdef min // windef has macros for that, kill'em
#undef min
#endif
template <class T> inline T &min(const T &i1, const T &i2)
{	if(i1 < i2) return (T&)i1; else return (T&)i2;
}
#ifdef max // windef has macros for that, kill'em
#undef max
#endif
template <class T> inline T &max(const T &i1, const T &i2)	
{	if(i1 > i2) return (T&)i1; else return (T&)i2;
}

#ifdef swap // just to be sure
#undef swap
#endif
template <class T> inline void swap(T &i1, T &i2)
{	T dummy = i1; i1=i2; i2=dummy;
}

//////////////////////////////
#else // not cplusplus
//////////////////////////////

// boolean types for C
typedef int bool;
#define false	(1==0)
#define true	(1==1)

#ifndef swap
// hmm only ints?
//#define swap(a,b) { int temp=a; a=b; b=temp;} 
// if macros then something that is type independent
#define swap(a,b) ((a == b) || ((a ^= b), (b ^= a), (a ^= b)))
#endif swap

//////////////////////////////
#endif // not cplusplus
//////////////////////////////



//////////////////////////////
// socketlen type definition
//////////////////////////////
#ifndef __socklen_t_defined

#ifdef WIN32
typedef int socklen_t;
#else
typedef unsigned int socklen_t;
#endif

#define __socklen_t_defined
#endif



#ifndef NULL
#define NULL (void *)0
#endif

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define Assert(EX) //assert(EX)



//////////////////////////////
#ifdef WIN32
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
// no inlining them because of varargs; those might not conlict in defines anyway
#define vsnprintf			_vsnprintf
#define snprintf			_snprintf


//#define strcasecmp		in utils.h
//#define strncasecmp		in utils.h


extern inline void sleep(unsigned long time)	
{
	Sleep(time);
}
extern inline int read(SOCKET fd, char*buf, int sz)		
{
	return recv(fd,buf,sz,0); 
}
extern inline int write(SOCKET fd, char*buf, int sz)	
{
	return send(fd,buf,sz,0); 
}


// missing functions and helpers
extern inline void gettimeofday(struct timeval *timenow, void *dummy)
{
	time_t t;
	t = clock();
	timenow->tv_usec = (long)t;
	timenow->tv_sec = (long)t / CLK_TCK;
	return;
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

extern inline int closesocket(SOCKET fd)		
{
	return close(fd); 
}
extern inline int ioctlsocket(SOCKET fd, long cmd, unsigned long *arg)		
{
	return ioctl(fd,cmd,arg); 
}



// missing functions and helpers

extern inline unsigned long GetTickCount() {
	 struct timeval tv;
	 gettimeofday( &tv, NULL );
	 return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
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


//////////////////////////////////////////////////////////////////////////
// byte word dword access
//////////////////////////////////////////////////////////////////////////

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

extern inline unsigned long MakeDWord(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3)
{
	return 	  ((unsigned long)byte0)
			| ((unsigned long)byte1<<0x08)
			| ((unsigned long)byte2<<0x10)
			| ((unsigned long)byte3<<0x18);
}

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
extern inline unsigned short SwapTwoBytes(unsigned short w)
{
    return	  ((w & 0x00FF) << 0x08)
			| ((w & 0xFF00) >> 0x08);
}


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
extern inline unsigned long SwapFourBytes(unsigned long w)
{
    return	  ((w & 0x000000FF) << 0x18)
			| ((w & 0x0000FF00) << 0x08)
			| ((w & 0x00FF0000) >> 0x08)
			| ((w & 0xFF000000) >> 0x18);
}



// Find the log base 2 of an N-bit integer in O(lg(N)) operations
// in this case for 32bit input it would be 11 operations
extern inline unsigned long log2(unsigned long v)
{
//	static const unsigned long b[] = 
//		{0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
//	static const unsigned long S[] = 
//		{1, 2, 4, 8, 16};
	// result of log2(v) will go here
	register unsigned long c = 0; 
//	int i;
//	for (i = 4; i >= 0; i--) 
//	{
//	  if (v & b[i])
//	  {
//		v >>= S[i];
//		c |= S[i];
//	  } 
//	}
	// unroll for speed...
//	if (v & b[4]) { v >>= S[4]; c |= S[4]; } 
//	if (v & b[3]) { v >>= S[3]; c |= S[3]; }
//	if (v & b[2]) { v >>= S[2]; c |= S[2]; }
//	if (v & b[1]) { v >>= S[1]; c |= S[1]; }
//	if (v & b[0]) { v >>= S[0]; c |= S[0]; }
	// put values in for more speed...
	if (v & 0xFFFF0000) { v >>= 0x10; c |= 0x10; } 
	if (v & 0x0000FF00) { v >>= 0x08; c |= 0x08; }
	if (v & 0x000000F0) { v >>= 0x04; c |= 0x04; }
	if (v & 0x0000000C) { v >>= 0x02; c |= 0x02; }
	if (v & 0x00000002) { v >>= 0x01; c |= 0x01; }
	return c;
}








#endif//_BASE_H_

