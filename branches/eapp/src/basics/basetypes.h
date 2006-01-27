#ifndef __BASETYPES_H__
#define __BASETYPES_H__

//////////////////////////////////////////////////////////////////////////
// basic include for all basics
// introduces types and global functions
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// some global switches
//////////////////////////////////////////////////////////////////////////
#define COUNT_GLOBALS		// enables countage of created objects
#define CHECK_BOUNDS		// enables boundary check for arrays and lists
#define CHECK_EXCEPTIONS	// use exceptions for "exception" handling
#define CHECK_LOCKS			// enables check of locking/unlocking sync objects
#define SINGLETHREAD		// builds without multithread guards
//#define MEMORY_EXCEPTIONS	// use buildin exceptions for out-of-memory handling


//////////////////////////////////////////////////////////////////////////
// no c support anymore
//////////////////////////////////////////////////////////////////////////
#ifndef __cplusplus
#error "this is C++ source"
#endif


//////////////////////////////////////////////////////////////////////////
// setting some defines on platforms
//////////////////////////////////////////////////////////////////////////
#if (defined(__WIN32__) || defined(__WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER) || defined(__BORLANDC__)) && !defined(WIN32)
#define WIN32
#endif

// __APPLE__ is the only predefined macro on MacOS X
#if defined(__APPLE__)
#define __DARWIN__
#endif

// 64bit OS
#if defined(_M_IA64) || defined(_M_X64) || defined(_WIN64) || defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
#define __64BIT__
#endif

#if defined(_ILP64)
#error "this specific 64bit architecture is not supported"
#endif

// debug mode
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
// system dependend include switches, 
// that need to be done before start including stuff
//////////////////////////////////////////////////////////////////////////

//////////////////////////////
#ifdef WIN32
//////////////////////////////
#define WIN32_LEAN_AND_MEAN		// stuff
#define _WINSOCKAPI_			// prevent inclusion of winsock.h
//#define _USE_32BIT_TIME_T		// use 32 bit time variables 
//#define __USE_W32_SOCKETS
//////////////////////////////
#endif
//////////////////////////////

//////////////////////////////
// force glibc stuff into namespaces (>=2.3 as far as I've seen)
// still testing, 
// might generally avoid fighting with glibc since forlorn
// if completely hopeless, retreat to an own namespace
// this would also solve the exception problem on windows
//////////////////////////////
//#define _GLIBCPP_USE_NAMESPACES
//////////////////////////////


//////////////////////////////
#undef FD_SETSIZE
#define FD_SETSIZE 4096
//////////////////////////////





//////////////////////////////////////////////////////////////////////////
// standard headers
//////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
// additional includes for wchar support
#include <wchar.h>
#include <wctype.h>


//////////////////////////////
#if defined(linux)
#include <stdint.h>     // for uintptr_t
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// time
//////////////////////////////////////////////////////////////////////////
#include <time.h>
//////////////////////////////
// altzone fix for Sun/GCC 3.2.
#if defined(__sun__) && defined(__GNUC__) && ( __STDC__ != 0 || defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) )
extern int cftime(char *, char *, const time_t *);
extern int ascftime(char *, const char *, const struct tm *);
extern long altzone;
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// threads & syncronisation
//////////////////////////////////////////////////////////////////////////

//////////////////////////////
#ifdef WIN32
//////////////////////////////
#define _WINSOCKAPI_			// prevent inclusion of winsock.h
								// since winsock2.h is necessary
#define _WIN32_WINNT 0x0400		// need for TryEnterCriticalSection
								// uncomment for use on win98 and lower
								// but remember that mutex::trylock()
								// will always fail then
#include <windows.h>
#include <process.h>
#include <conio.h>
//////////////////////////////
#else
//////////////////////////////
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#ifdef __sun__
#include <poll.h>
#endif
#ifndef __bsdi__
#include <semaphore.h>
#endif
//////////////////////////////
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// inet/socket/fileio
//////////////////////////////////////////////////////////////////////////

//////////////////////////////
#ifdef WIN32
//////////////////////////////
#include <winsock2.h>
// do not include <io.h> globally
//////////////////////////////
#else
//////////////////////////////
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>		// needs but does not include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#ifndef FIONREAD
#include <sys/filio.h>	// FIONREAD on Solaris, might conflict on other systems
#endif
#ifndef SIOCGIFCONF
#include <sys/sockio.h> // SIOCGIFCONF on Solaris, maybe others?
#endif
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif
//////////////////////////////
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// c++ header 
//////////////////////////////////////////////////////////////////////////
#include <typeinfo>		//!! this is poisoning the global namespace


//////////////////////////////////////////////////////////////////////////
// after including the c++ header, class name "exception" has been
// placed unfortunately in global namespace and in libc's 
// at least for the VisualC on windows platforms
// to get it working with the our own exception class we do some workaround
// by just swapping the name with a hard define, 
// drawback is, that std::exception is in this case also not usable
// another possibility would be to retreat to our own namespace
//////////////////////////////////////////////////////////////////////////
#if _MSC_VER
#define exception CException
#endif
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// no comment
//////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning (disable: 4099)	// "type name first seen using 'struct' now seen using 'class'"
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127)	// constant assignment
//#pragma warning(disable : 4200)	//'...'" warning, NULL field in struct
#pragma warning(disable : 4244) // converting type on return will shorten
#pragma warning(disable : 4250) // dominant derive, is only informational
//#pragma warning(disable : 4251)	// disable "class '...' needs to have dll-interface to be used by clients of class '...'", since the compiler may sometimes give this warning incorrectly.
//#pragma warning(disable : 4290)	// ... C++-specification of exception ignored
#pragma warning(disable : 4310)	// converting constant will shorten
#pragma warning(disable : 4355)	// "'this' : used in base member initializer list"
#pragma warning(disable : 4511)	// no copy constructor
#pragma warning(disable : 4512)	// no assign operator
#pragma warning(disable : 4514)	// unreferenced inline function
#pragma warning(disable : 4702) // disable "unreachable code" warning for throw (known compiler bug)
#pragma warning(disable : 4706) // assignment within conditional
#pragma warning(disable : 4710)	// is no inline function
#pragma warning(disable : 4996)	// disable deprecated warnings
#endif



//////////////////////////////////////////////////////////////////////////
// useful defines
//////////////////////////////////////////////////////////////////////////
// Windows compilers before VC7 don't have __FUNCTION__.
#if defined(_MSC_VER) && _MSC_VER < 1300
# define __FUNCTION__ "<unknown>"
# define __func__ __FUNCTION__
#elif defined(__sgi) && !defined(__GNUC__) && defined(__c99)
# define __FUNCTION__ __func__
#elif !defined(__NETBSD__)
# if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2
#   define __func__ __FUNCTION__
#  else
#   define __FUNCTION__ "<unknown>"
#   define __PRETTY_FUNCTION__ __FUNCTION__
#   define __func__ __FUNCTION__
#  endif
# endif
#endif

// disable attributed stuff on non-GNU
#ifndef __GNUC__
#  define  __attribute__(x)
#endif


//////////////////////////////////////////////////////////////////////////
// useful typedefs
//////////////////////////////////////////////////////////////////////////
typedef   signed int	sint;	// don't use (only for ie. scanf)
typedef unsigned int	uint;	// don't use
typedef   signed long	slong;	// don't use (only for ie. file-io)
typedef unsigned long	ulong;	// don't use
typedef   signed short	sshort;
typedef unsigned short	ushort;
typedef unsigned char	uchar;
typedef   signed char	schar;

typedef char*           pchar;
typedef const char*     cchar;
typedef unsigned char*	puchar;
typedef void*			ptr;
typedef int*			pint;


//////////////////////////////////////////////////////////////////////////
// pointer type
//////////////////////////////////////////////////////////////////////////
//#ifdef WIN32
//typedef uint         intptr;
//#else
//typedef uintptr_t    intptr;
//#endif


//////////////////////////////////////////////////////////////////////////
// typedefs to compensate type size change from 32bit to 64bit
// MS implements LLP64 model, normal unix does LP64,
// only Silicon Graphics/Cray goes ILP64 so don't care (and don't support)
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Integers with guaranteed _exact_ size.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////
#ifdef WIN32
//////////////////////////////
typedef          __int8		int8;
typedef          __int16	int16;
typedef          __int32	int32;

typedef signed __int8		sint8;
typedef signed __int16		sint16;
typedef signed __int32		sint32;

typedef unsigned __int8		uint8;
typedef unsigned __int16	uint16;
typedef unsigned __int32	uint32;
//////////////////////////////
#else // GNU
//////////////////////////////
typedef char				int8;
typedef short				int16;
typedef int					int32;

typedef signed char			sint8;
typedef signed short		sint16;
typedef signed int			sint32;

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
//////////////////////////////
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Integers with guaranteed _minimum_ size.
// These could be larger than you expect,
// they are designed for speed.
//////////////////////////////////////////////////////////////////////////
typedef          long int      ppint;
typedef unsigned long int      ppuint;

typedef          long int      ppint8;
typedef          long int      ppint16;
typedef          long int      ppint32;

typedef unsigned long int      ppuint8;
typedef unsigned long int      ppuint16;
typedef unsigned long int      ppuint32;


//////////////////////////////////////////////////////////////////////////
// integer with exact processor width (and best speed)
//						size_t already defined in stdio.h
//////////////////////////////
#ifdef WIN32 // does not have a signed size_t
//////////////////////////////
#if defined(_WIN64)	// naive 64bit windows platform
typedef __int64			ssize_t;
#else
typedef int				ssize_t;
#endif
//////////////////////////////
#endif
//////////////////////////////


//////////////////////////////////////////////////////////////////////////
// portable 64-bit integers
//////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64				int64;
typedef signed __int64		sint64;
typedef unsigned __int64	uint64;
#define LLCONST(a)			(a##i64)
#else
typedef long long			int64;
typedef signed long long	sint64;
typedef unsigned long long	uint64;
#define LLCONST(a)			(a##ll)
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
// some redefine of function redefines for some Compilers
//////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define strcasecmp			stricmp
#define strncasecmp			strnicmp
#define snprintf			_snprintf
#define vsnprintf			_vsnprintf
#endif




//////////////////////////////////////////////////////////////////////////
// glibc has become a major bitch
//////////////////////////////////////////////////////////////////////////
#if defined(__GLIBC__) && ( (__GLIBC__ >2) || (__GLIBC__ ==2 && __GLIBC_MINOR__>=3) )
// unfold hidden libc proto's
#endif



//////////////////////////////////////////////////////////////////////////
// min max and swap template
//////////////////////////////////////////////////////////////////////////
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
template <class T> inline void swap(T &i1, T &i2)
{	T dummy = i1; i1=i2; i2=dummy;
}

//////////////////////////////
#ifndef __cplusplus	// not cplusplus cannot happen, just to store it here
//////////////////////////////

// keyword replacement in windows
#ifdef _WIN32
#define inline __inline
#endif

// boolean types for C
typedef int bool;
#define false	(1==0)
#define true	(1==1)

#ifdef swap // just to be sure
#undef swap
#endif
// hmm only ints?
//#define swap(a,b) { int temp=a; a=b; b=temp;} 
// if using macros then something that is type independent
#define swap(a,b) ((a == b) || ((a ^= b), (b ^= a), (a ^= b)))

//////////////////////////////
#endif // not cplusplus
//////////////////////////////

//////////////////////////////////////////////////////////////////////////
// should not happen
#ifndef NULL
#define NULL (void *)0
#endif

//////////////////////////////////////////////////////////////////////////
// number of bits in a byte
#ifndef NBBY
#define	NBBY 8
#endif




///////////////////////////////////////////////////////////////////////////////
// conversion overloads to change signed types to the appropriate unsigned
///////////////////////////////////////////////////////////////////////////////
inline size_t to_unsigned(char t)
{
	return (unsigned char)(t);
}
inline size_t to_unsigned(unsigned char t)
{
	return (unsigned char)(t);
}
// UCT2
inline size_t to_unsigned(short t)
{
	return (unsigned short)(t);
}
inline size_t to_unsigned(unsigned short t)
{
	return (unsigned short)(t);
}
// UCT4
inline size_t to_unsigned(sint32 t)
{
	return (uint32)(t);
}
inline size_t to_unsigned(uint32 t)
{
	return (uint32)(t);
}
// others, just to be complete
inline size_t to_unsigned(long t)
{
	return (unsigned long)(t);
}
inline size_t to_unsigned(unsigned long t)
{
	return (unsigned long)(t);
}
inline uint64 to_unsigned(int64 t)
{
	return (uint64)(t);
}
inline uint64 to_unsigned(uint64 t)
{
	return (uint64)(t);
}
#ifdef WIN32
// int on non-win32 in handled by int32
inline size_t to_unsigned(int t)
{
	return (unsigned int)(t);
}
inline size_t to_unsigned(unsigned int t)
{
	return (unsigned int)(t);
}
#endif

///////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
// wrappers for Character Classification Routines
// this also gets rid of the macro definitions
//!! TODO: choose a better name for the namespace 
namespace stringcheck
{
#ifdef isalpha	// get the function form, not the macro
#undef isalpha
#endif
template <class T> extern inline bool isalpha(T c) { return 0!=::isalpha( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isalpha(char c)		{ return 0!=::isalpha ( to_unsigned(c) ); }
extern inline bool isalpha(wchar_t c)	{ return 0!=::iswalpha( to_unsigned(c) ); }

#ifdef isupper	// get the function form, not the macro
#undef isupper
#endif
template <class T> extern inline bool isupper(T c) { return ::isupper( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isupper(char c)		{ return 0!=::isupper ( to_unsigned(c) ); }
extern inline bool isupper(wchar_t c)	{ return 0!=::iswupper( to_unsigned(c) ); }

#ifdef islower	// get the function form, not the macro
#undef islower
#endif
template <class T> extern inline bool islower(T c) { return 0!=::islower( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool islower(char c)		{ return 0!=::islower ( to_unsigned(c) ); }
extern inline bool islower(wchar_t c)	{ return 0!=::iswlower( to_unsigned(c) ); }

#ifdef isdigit	// get the function form, not the macro
#undef isdigit
#endif
template <class T> extern inline bool isdigit(T c) { return 0!=::isdigit( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isdigit(char c)		{ return 0!=::isdigit ( to_unsigned(c) ); }
extern inline bool isdigit(wchar_t c)	{ return 0!=::iswdigit( to_unsigned(c) ); }

#ifdef isxdigit	// get the function form, not the macro
#undef isxdigit
#endif
template <class T> extern inline bool isxdigit(T c) { return 0!=::isxdigit( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isxdigit(char c)		{ return 0!=::isxdigit ( to_unsigned(c) ); }
extern inline bool isxdigit(wchar_t c)	{ return 0!=::iswxdigit( to_unsigned(c) ); }

#ifdef isspace	// get the function form, not the macro
#undef isspace
#endif
template <class T> extern inline bool isspace(T c) { return 0!=::isspace( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isspace(char c)		{ return 0!=::isspace ( to_unsigned(c) ); }
extern inline bool isspace(wchar_t c)	{ return 0!=::iswspace( to_unsigned(c) ); }

#ifdef ispunct	// get the function form, not the macro
#undef ispunct
#endif
template <class T> extern inline bool ispunct(T c) { return 0!=::ispunct( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool ispunct(char c)		{ return 0!=::ispunct ( to_unsigned(c) ); }
extern inline bool ispunct(wchar_t c)	{ return 0!=::iswpunct( to_unsigned(c) ); }

#ifdef isalnum	// get the function form, not the macro
#undef isalnum
#endif
template <class T> extern inline bool isalnum(T c) { return 0!=::isalnum( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isalnum(char c)		{ return 0!=::isalnum ( to_unsigned(c) ); }
extern inline bool isalnum(wchar_t c)	{ return 0!=::iswalnum( to_unsigned(c) ); }

#ifdef isprint	// get the function form, not the macro
#undef isprint
#endif
template <class T> extern inline bool isprint(T c) { return 0!=::isprint( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isprint(char c)		{ return 0!=::isprint ( to_unsigned(c) ); }
extern inline bool isprint(wchar_t c)	{ return 0!=::iswprint( to_unsigned(c) ); }

#ifdef isgraph	// get the function form, not the macro
#undef isgraph
#endif
template <class T> extern inline bool isgraph(T c) { return 0!=::isgraph( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool isgraph(char c)		{ return 0!=::isgraph ( to_unsigned(c) ); }
extern inline bool isgraph(wchar_t c)	{ return 0!=::iswgraph( to_unsigned(c) ); }

#ifdef iscntrl	// get the function form, not the macro
#undef iscntrl
#endif
template <class T> extern inline bool iscntrl(T c) { return 0!=::iscntrl( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline bool iscntrl(char c)		{ return 0!=::iscntrl ( to_unsigned(c) ); }
extern inline bool iscntrl(wchar_t c)	{ return 0!=::iswcntrl( to_unsigned(c) ); }

#ifdef toupper	// get the function form, not the macro
#undef toupper
#endif
template <class T> extern inline T toupper(T c) { return ::toupper( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline char toupper(char c)		{ return ::toupper ( to_unsigned(c) ); }
extern inline wchar_t toupper(wchar_t c){ return ::towupper( to_unsigned(c) ); }

#ifdef tolower	// get the function form, not the macro
#undef tolower
#endif
template <class T> extern inline T tolower(T c) { return ::tolower( to_unsigned(c) ); }
// implementation for char (and wchar)
extern inline char tolower(char c)		{ return ::tolower ( to_unsigned(c) ); }
extern inline wchar_t tolower(wchar_t c){ return ::towlower( to_unsigned(c) ); }

};
///////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// atomic access functions
// FASTCALL define is necessary for _asm atomic functions on windows
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

int FASTCALL atomicincrement(int* target);
int FASTCALL atomicdecrement(int* target);
int FASTCALL atomicexchange(int* target, int value);
void* FASTCALL _atomicexchange(void** target, void* value);

inline unsigned int FASTCALL atomicincrement(unsigned int* target)	{return (unsigned int)atomicincrement((int*)target);}
inline unsigned int FASTCALL atomicdecrement(unsigned int* target)	{return (unsigned int)atomicdecrement((int*)target);}

template <class T> inline T* atomicexchange(T** target, T* value)
{	return (T*)_atomicexchange((void**)target, (void*)value); 
}



//////////////////////////////////////////////////////////////////////////
// byte/word/dword access, 32bit limited
//////////////////////////////////////////////////////////////////////////

extern inline uchar GetByte(uint32 val, size_t num)
{
	switch(num)
	{
	case 0:
		return (uchar)(val      );
	case 1:
		return (uchar)(val>>0x08);
	case 2:
		return (uchar)(val>>0x10);
	case 3:
		return (uchar)(val>>0x18);
	default:
		return 0;	//better throw something here
	}
}
extern inline ushort GetWord(uint32 val, size_t num)
{
	switch(num)
	{
	case 0:
		return (ushort)(val      );
	case 1:
		return (ushort)(val>>0x10);
	default:
		return 0;	//better throw something here
	}	
}
extern inline ushort MakeWord(uchar byte0, uchar byte1)
{
	return	  (((ushort)byte0)      )
			| (((ushort)byte1)<<0x08);
}
extern inline uint32 MakeDWord(ushort word0, ushort word1)
{
	return 	  (((uint32)word0)      )
			| (((uint32)word1)<<0x10);
}

extern inline uint32 MakeDWord(uchar byte0, uchar byte1, uchar byte2, uchar byte3)
{
	return 	  (((uint32)byte0)      )
			| (((uint32)byte1)<<0x08)
			| (((uint32)byte2)<<0x10)
			| (((uint32)byte3)<<0x18);
}


// Swap two bytes in a byte stream
extern inline void SwapTwoBytes(char *p)
{	if(p)
	{	char tmp =p[0];
		p[0] = p[1];
		p[1] = tmp;
	}
}
// Swap the bytes within a 16-bit WORD.
extern inline ushort SwapTwoBytes(ushort w)
{
    return	  ((w & 0x00FF) << 0x08)
			| ((w & 0xFF00) >> 0x08);
}


// Swap the 4 bytes in a byte stream
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
// Swap the 4 bytes within a 32-bit DWORD.
extern inline uint32 SwapFourBytes(uint32 w)
{
    return	  ((w & 0x000000FF) << 0x18)
			| ((w & 0x0000FF00) << 0x08)
			| ((w & 0x00FF0000) >> 0x08)
			| ((w & 0xFF000000) >> 0x18);
}


//////////////////////////////////////////////////////////////////////////
// Check the byte-order of the CPU.
//////////////////////////////////////////////////////////////////////////
#define LSB_FIRST        0
#define MSB_FIRST        1
extern inline int CheckByteOrder(void)
{
    static short  w = 0x0001;
    static char  *b = (char *) &w;
    return(b[0] ? LSB_FIRST : MSB_FIRST);
}


//////////////////////////////////////////////////////////////////////////
// some bit twiddling
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Find the log base 2 of an N-bit integer in O(lg(N)) operations
// in this case for 32bit input it would be 11 operations
//////////////////////////////////////////////////////////////////////////
#ifdef log2 //glibc defines this as macro
#undef log2
#endif
inline unsigned long log2(unsigned long v)
{
//	static const unsigned long b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
//	static const unsigned long S[] = {1, 2, 4, 8, 16};
	// result of log2(v) will go here
	register uint32 c = 0; 
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
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	if (v & LLCONST(0xFFFFFFFF00000000)) { v >>= 0x18; c |= 0x18; } 
#endif
	if (v & 0xFFFF0000) { v >>= 0x10; c |= 0x10; } 
	if (v & 0x0000FF00) { v >>= 0x08; c |= 0x08; }
	if (v & 0x000000F0) { v >>= 0x04; c |= 0x04; }
	if (v & 0x0000000C) { v >>= 0x02; c |= 0x02; }
	if (v & 0x00000002) { v >>= 0x01; c |= 0x01; }

	return c;
}



//////////////////////////////////////////////////////////////////////////
// OR (IF YOU KNOW v IS A POWER OF 2):
//////////////////////////////////////////////////////////////////////////
inline unsigned long log2_(unsigned long v)
{
//	const unsigned long b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
//	register ulong c = ((v & b[0]) != 0);
//	int i;
//	for (i = 4; i >= 1; i--) 
//	{
//	  c |= ((v & b[i]) != 0) << i;
//	}
	// unroll for speed...
//	c |= ((v & b[4]) != 0) << 4;
//	c |= ((v & b[3]) != 0) << 3;
//	c |= ((v & b[2]) != 0) << 2;
//	c |= ((v & b[1]) != 0) << 1;
//	c |= ((v & b[0]) != 0) << 0;
	// unroll for speed...
	// put values in for more speed...
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	register ulong c = ((v & LLCONST(0xAAAAAAAAAAAAAAAA)) != 0);
	c |= ((v & LLCONST(0xFFFFFFFF00000000)) != 0) << 5;
	c |= ((v & LLCONST(0xFFFF0000FFFF0000)) != 0) << 4;
	c |= ((v & LLCONST(0xFF00FF00FF00FF00)) != 0) << 3;
	c |= ((v & LLCONST(0xF0F0F0F0F0F0F0F0)) != 0) << 2;
	c |= ((v & LLCONST(0xCCCCCCCCCCCCCCCC)) != 0) << 1;
	c |= ((v & LLCONST(0xAAAAAAAAAAAAAAAA)) != 0) << 0;
#else
	register ulong c = ((v & 0xAAAAAAAA) != 0);
	c |= ((v & 0xFFFF0000) != 0) << 4;
	c |= ((v & 0xFF00FF00) != 0) << 3;
	c |= ((v & 0xF0F0F0F0) != 0) << 2;
	c |= ((v & 0xCCCCCCCC) != 0) << 1;
	c |= ((v & 0xAAAAAAAA) != 0) << 0;
#endif
	return c;
}
//////////////////////////////////////////////////////////////////////////
// Find the log base 2 of an integer with a lookup table
// The lookup table method takes only about 7 operations 
// to find the log of a 32-bit value. 
// If extended for 64-bit quantities, it would take roughly 9 operations.
//////////////////////////////////////////////////////////////////////////
extern inline unsigned long log2t(unsigned long v)
{
	static const unsigned char LogTable256[] = 
	{
	  0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
	};
	register unsigned long c; // c will be lg(v)
	register unsigned long t;
	register unsigned long tt;
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	register unsigned long ttt = (v >> 32);
	if(ttt)
	{
		tt = (ttt >> 16);
		if(tt)
		{
			t = v >> 24;
			c = (t) ? 32+24 + LogTable256[t] : 32+16 + LogTable256[tt & 0xFF];
		}
		else 
		{
			t = v & 0xFF00;
			c = (t) ? 32+8 + LogTable256[t >> 8] : 32+LogTable256[v & 0xFF];
		}
	}
	else 
#endif
	{
		tt = (v >> 16);
		if(tt)
		{
			t = v >> 24;
			c = (t) ? 24 + LogTable256[t] : 16 + LogTable256[tt & 0xFF];
		}
		else 
		{
			t = v & 0xFF00;
			c = (t) ? 8 + LogTable256[t >> 8] : LogTable256[v & 0xFF];
		}
	}

	return c;
}

//////////////////////////////////////////////////////////////////////////
// Counting bits set, in parallel
//////////////////////////////////////////////////////////////////////////
inline unsigned long bit_count(unsigned long v)
{
//	static const ulong S[] = {1, 2, 4, 8, 16}; // Magic Binary Numbers
//	static const ulong B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
	// store the total here
	uint32 c = v; 
//	c = ((c >> S[0]) & B[0]) + (c & B[0]);
//	c = ((c >> S[1]) & B[1]) + (c & B[1]);
//	c = ((c >> S[2]) & B[2]) + (c & B[2]);
//	c = ((c >> S[3]) & B[3]) + (c & B[3]);
//	c = ((c >> S[4]) & B[4]) + (c & B[4]);
	// put values in
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	c = ((c >> 0x01) & LLCONST(0x5555555555555555)) + (c & LLCONST(0x5555555555555555));
	c = ((c >> 0x02) & LLCONST(0x3333333333333333)) + (c & LLCONST(0x3333333333333333));
	c = ((c >> 0x04) & LLCONST(0x0F0F0F0F0F0F0F0F)) + (c & LLCONST(0x0F0F0F0F0F0F0F0F));
	c = ((c >> 0x08) & LLCONST(0x00FF00FF00FF00FF)) + (c & LLCONST(0x00FF00FF00FF00FF));
	c = ((c >> 0x10) & LLCONST(0x0000FFFF0000FFFF)) + (c & LLCONST(0x0000FFFF0000FFFF));
	c = ((c >> 0x10) & LLCONST(0x00000000FFFFFFFF)) + (c & LLCONST(0x00000000FFFFFFFF));
#else
	c = ((c >> 0x01) & 0x55555555) + (c & 0x55555555);
	c = ((c >> 0x02) & 0x33333333) + (c & 0x33333333);
	c = ((c >> 0x04) & 0x0F0F0F0F) + (c & 0x0F0F0F0F);
	c = ((c >> 0x08) & 0x00FF00FF) + (c & 0x00FF00FF);
	c = ((c >> 0x10) & 0x0000FFFF) + (c & 0x0000FFFF);
#endif
	return c;
}
//////////////////////////////////////////////////////////////////////////
// Reverse the bits in a byte with 7 operations
//////////////////////////////////////////////////////////////////////////
inline uchar bit_reverse(uchar b)
{
	return (uchar)(((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16);
}
//////////////////////////////////////////////////////////////////////////
// Reverse an N-bit quantity in parallel in 5 * lg(N) operations:
// This method is best suited to situations where N is large.
// Any reasonable optimizing C compiler should treat the dereferences 
// of B, ~B, and S as constants, requiring no evaluation other than perhaps 
// a load operation for some of the B and ~B references.
// See Dr. Dobb's Journal 1983, Edwin Freed's article on Binary Magic Numbers 
// for more information. 
// Anyway I would not count on that, so I put values explicitely.
//////////////////////////////////////////////////////////////////////////
inline unsigned long bit_reverse(unsigned long v)
{
//	static const ulong S[] = {1, 2, 4, 8, 16}; // Magic Binary Numbers
//	static const ulong B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
//	v = ((v >> S[0]) & B[0]) | ((v << S[0]) & ~B[0]); // swap odd and even bits
//	v = ((v >> S[1]) & B[1]) | ((v << S[1]) & ~B[1]); // swap consecutive pairs
//	v = ((v >> S[2]) & B[2]) | ((v << S[2]) & ~B[2]); // swap nibbles ...
//	v = ((v >> S[3]) & B[3]) | ((v << S[3]) & ~B[3]);
//	v = ((v >> S[4]) & B[4]) | ((v << S[4]) & ~B[4]);
	// better set it by hand
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	v = ((v >> 0x01) & LLCONST(0x5555555555555555)) | ((v << 0x01) & ~LLCONST(0x5555555555555555));
	v = ((v >> 0x02) & LLCONST(0x3333333333333333)) | ((v << 0x02) & ~LLCONST(0x3333333333333333));
	v = ((v >> 0x04) & LLCONST(0x0F0F0F0F0F0F0F0F)) | ((v << 0x04) & ~LLCONST(0x0F0F0F0F0F0F0F0F));
	v = ((v >> 0x08) & LLCONST(0x00FF00FF00FF00FF)) | ((v << 0x08) & ~LLCONST(0x00FF00FF00FF00FF));
	v = ((v >> 0x10) & LLCONST(0x0000FFFF0000FFFF)) | ((v << 0x10) & ~LLCONST(0x0000FFFF0000FFFF));
	v = ((v >> 0x10) & LLCONST(0x00000000FFFFFFFF)) | ((v << 0x10) & ~LLCONST(0x00000000FFFFFFFF));
#else
	v = ((v >> 0x01) & 0x55555555) | ((v << 0x01) & ~0x55555555);
	v = ((v >> 0x02) & 0x33333333) | ((v << 0x02) & ~0x33333333);
	v = ((v >> 0x04) & 0x0F0F0F0F) | ((v << 0x04) & ~0x0F0F0F0F);
	v = ((v >> 0x08) & 0x00FF00FF) | ((v << 0x08) & ~0x00FF00FF);
	v = ((v >> 0x10) & 0x0000FFFF) | ((v << 0x10) & ~0x0000FFFF);
#endif
	return v;
}

//////////////////////////////////////////////////////////////////////////
// check if a number is power of 2
// roughly; if one (and only one) bit is set
//////////////////////////////////////////////////////////////////////////
extern inline bool isPowerOf2(unsigned long i)
{
	return (i > 0) && (0==(i & (i - 1)));
}

//////////////////////////////////////////////////////////////////////////
// round up to the next power of 2
//////////////////////////////////////////////////////////////////////////
extern inline unsigned long RoundPowerOf2(unsigned long v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
#if defined(_LP64) || defined(_ILP64) || defined(__LP64__) || defined(__ppc64__)
	v |= v >> 32;
#endif
	v++;
	return v;
}

//////////////////////////////////////////////////////////////////////////
// calculate pow n on base 2
//////////////////////////////////////////////////////////////////////////
#ifdef pow2 // just to be sure
#undef pow2
#endif
extern inline unsigned long pow2(unsigned long v)
{
	if( v < NBBY*sizeof(unsigned long) )
		return 1<<v;
	return 0;
}

// square root with newton approximation
// starting condition calculated with two approximations
// sqrt(n) = n^1/2 = n / n^1/2 = n / 2^log2(n^1/2) = n / 2^(log2(n)/2)
// and calculating a/2^b with left shift as a>>b
// which results in a larger value than necessary 
// because the integer log2 returns the floored logarism and is smaller than the real log2
// second approximation is
// sqrt(n) = n^1/2 = 2^(log2(n)/2) which is calculated as 1<<(log2(n)/2)
// resulting in a value smaller than necessary because of the integer log2 
// calculation the mean of those two approximations gets closer to the real value, 
// only slightly faster than the buildin double sqrt and therefore useless
#ifdef isqrt
#undef isqrt
#endif
template<class T> static inline T isqrt(T n)
{
	if(n>0)
	{
		T q=0, xx = (log2(n)/2), qx = ((n>>xx) + (1<<xx))/2;
		do
		{
			q  = qx;
			qx = (q + n/q)/2;
		}
		while( q!=qx && q+1!=qx );
		return q;
	}
	// should set matherr or throw something when negative
	return 0;
}

#endif//__BASETYPES_H__
