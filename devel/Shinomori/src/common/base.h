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
#define SINGLETHREAD		// builds without multithread guards
#define CHECK_EXCEPTIONS	// use exceptions for "exception" handling




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
#define WIN32_LEAN_AND_MEAN	// stuff
#define _WINSOCKAPI_		// prevent inclusion of winsock.h
//#define _USE_32BIT_TIME_T	// use 32 bit time variables on 64bit windows
//#define __USE_W32_SOCKETS
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
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4244) // converting type on return will shorten
#pragma warning(disable : 4310)	// converting constant will shorten
#pragma warning(disable : 4706) // assignment within conditional
#pragma warning(disable : 4127)	// constant assignment
#pragma warning(disable : 4710)	// is no inline function

#include <windows.h>
#include <winsock2.h>
#include <conio.h>
//#include <crtdbg.h>
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
typedef char*           pchar;	//conflict with shitty declarations from mysql
typedef unsigned char*  puchar;
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



//////////////////////////////
// socketlen type definition
//////////////////////////////
#ifndef __socklen_t_defined

#ifdef WIN32
//#if defined(WIN32) || defined(CYGWIN)
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
// txt��log�Ȃǂ̏����o���t�@�C���̉��s�R�[�h
#define RETCODE	"\r\n"	// (CR/LF�FWindows�n)
#define RET RETCODE

#define PATHSEP '\\'



// abnormal function definitions
// no inlining them because of varargs; those might not conflict in defines anyway
#define vsnprintf			_vsnprintf
#define snprintf			_snprintf


#define strcasecmp			stricmp
#define strncasecmp			strnicmp


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

// txt��log�Ȃǂ̏����o���t�@�C���̉��s�R�[�h
#define RETCODE "\n"	// (LF�FUnix�n�j
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

extern inline unsigned long GetCurrentProcessId()
{	
	return getpid();
}

//////////////////////////////
#endif////////////////////////
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
// seek defines do not always exist
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
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
		return (unsigned short)((val & 0x0000FFFF)      );
	case 1:
		return (unsigned short)((val & 0xFFFF0000)>>0x10);
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

extern inline unsigned long pow2(unsigned long v)
{
	if(v<32)
		return 1<<v;
	return 0;
}











//////////////////////////////////////////////////////////////////////////
// empty base classes
//////////////////////////////////////////////////////////////////////////
class noncopyable
{
public:
	noncopyable()	{}
};
class global
{
public:
	global()	{}
};









//!!todo!! replace with full headers when switching to multithread

//////////////////////////////////////////////////////////////////////////
// empty sync classes for single thread 
//////////////////////////////////////////////////////////////////////////
class Mutex: public noncopyable
{
public:
    Mutex()					{}
    ~Mutex()				{}
	bool trylock() const	{return false;}
    void enter() const		{}
    void leave() const		{}
    void lock() const 		{}
    void unlock() const		{}
};

class ScopeLock: public noncopyable
{
public:
    ScopeLock(const Mutex& imtx)	{}
    ~ScopeLock()					{}
};







//////////////////////////////////////////////////////////////////////////
// the basic exception class. 
//////////////////////////////////////////////////////////////////////////
class exception : public global
{
protected:
    char *message;
public:
    exception(const char*   e):message(NULL)
	{
		if(e)
		{	// c function will just fail on memory error
			message = strdup( e );
		}
	}

    virtual ~exception()				{ free(message); }
	operator const char *()				{ return message; }
};


//////////////////////////////////////////////////////////////////////////
// exception for 'out of bound array access'
//////////////////////////////////////////////////////////////////////////
class exception_bound : public exception
{
public:
	exception_bound(const char*   e) : exception(e) {}
	virtual ~exception_bound()						{}
};

//////////////////////////////////////////////////////////////////////////
// exception for 'memory allocation failed'
//////////////////////////////////////////////////////////////////////////
class exception_memory : public exception
{
public:
	exception_memory(const char*   e) : exception(e)	{}
	virtual ~exception_memory()							{}
};

//////////////////////////////////////////////////////////////////////////
// exception for 'failed conversion'
//////////////////////////////////////////////////////////////////////////
class exception_convert: public exception
{
public:
	exception_convert(const char*   e) : exception(e)	{}
    virtual ~exception_convert()						{}
};



//////////////////////////////////////////////////////////////////////////
// variant exception class; 
// may be thrown when a variant is being typecasted to 32-bit int 
// and the value is out of range
//////////////////////////////////////////////////////////////////////////
class exception_variant: public exception
{
public:
	exception_variant(const char*   e) : exception(e)	{}
    virtual ~exception_variant()						{}
};

//////////////////////////////////////////////////////////////////////////
// exception for 'socket failed'
//////////////////////////////////////////////////////////////////////////
class exception_socket : public exception
{
public:
	exception_socket(const char*   e) : exception(e)	{}
	virtual ~exception_socket()							{}
};












///////////////////////////////////////////////////////////////////////////////
// a simple fixed size buffer
// need size argument at compile time
///////////////////////////////////////////////////////////////////////////////
template <class T, size_t SZ> class TBuffer
{
private:
	T	cField[SZ];
public:
	///////////////////////////////////////////////////////////////////////////
	// access a c style array
	operator T*()	{ return cField; }

	///////////////////////////////////////////////////////////////////////////
	// write access to field elements
	T &operator[](size_t inx)
	{
#ifdef CHECK_BOUNDS
		if(inx>SZ)
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TBuffer out of bound");
#else
			static T dummy;
			return dummy;
#endif
		}
#endif
		return cField[inx];
	}
	///////////////////////////////////////////////////////////////////////////
	// read-only access to field elements
	const T &operator[](size_t inx) const
	{
#ifdef CHECK_BOUNDS
		if(inx>SZ)
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TBuffer out of bound");
#else
			return cField[0];
#endif
		}
#endif
		return cField[inx];
	}
};

///////////////////////////////////////////////////////////////////////////////
// basic interface for arrays
///////////////////////////////////////////////////////////////////////////////
template <class T> class TArray : public Mutex, public global
{
	///////////////////////////////////////////////////////////////////////////
	// friends
	friend class Iterator;
	friend class String;
	friend class SubString;
	virtual const T* array() const=0;

protected:
	///////////////////////////////////////////////////////////////////////////
	// copy, move, compare
	virtual void copy(T* tar, const T* src, size_t cnt) = 0;
	virtual void move(T* tar, const T* src, size_t cnt) = 0;
	virtual int  compare(const T* a, const T* b) = 0;

public:
	virtual void realloc(size_t newsize) = 0;

public:
	///////////////////////////////////////////////////////////////////////////
	// constructor / destructor
	TArray()							{}
	virtual ~TArray()					{}

	///////////////////////////////////////////////////////////////////////////
	// direct access to the buffer
	virtual const T* getreadbuffer(size_t &maxcnt) const=0;
	virtual T* getwritebuffer(size_t &maxcnt)			=0;

	virtual bool setreadsize(size_t cnt)				=0;
	virtual bool setwritesize(size_t cnt)				=0;

	///////////////////////////////////////////////////////////////////////////
	// copy cnt elements from list to buf, return number of copied elements
	virtual size_t copytobuffer(T* buf, size_t cnt)		=0;

	///////////////////////////////////////////////////////////////////////////
	// access to element[inx]
	virtual const T& operator[](size_t inx) const 	=0;
	virtual T& operator[](size_t inx)		=0;	

	///////////////////////////////////////////////////////////////////////////
	// (re)allocates a list of cnt elements [0...cnt-1], 
	// leave new elements uninitialized/default constructed
	virtual bool resize(size_t cnt)			=0;	
	///////////////////////////////////////////////////////////////////////////
	// returns number of elements
	virtual size_t size() const				=0;	
	virtual size_t freesize() const				=0;	


	///////////////////////////////////////////////////////////////////////////
	// push/pop access
	virtual bool push(const T& elem)		{ return append(elem); }
	virtual bool push(const TArray<T>& list){ return append(list); }
	virtual bool push(const T* elem, size_t cnt){ return append(elem,cnt); }
	///////////////////////////////////////////////////////////////////////////
	// return the first element and remove it from list
	virtual T& pop()						=0;	
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool pop(T& elem)				=0;	
	///////////////////////////////////////////////////////////////////////////
	// return the first element and do not remove it from list
	virtual T& top() const					=0;	
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool top(T& elem) const			=0;	


	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const T& elem, size_t cnt=1) =0;	
	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const TArray<T>& list) 		=0;	
	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const T* elem, size_t cnt) =0;	

	///////////////////////////////////////////////////////////////////////////
	// remove elements from end of list
	virtual bool remove(size_t cnt=1) 		=0;	
	///////////////////////////////////////////////////////////////////////////
	// remove element [inx]
	virtual bool removeindex(size_t inx)	=0;	
	///////////////////////////////////////////////////////////////////////////
	// remove cnt elements starting from inx
	virtual bool removeindex(size_t inx, size_t cnt)	=0;	
	///////////////////////////////////////////////////////////////////////////
	// remove all elements
	virtual bool clear()					=0;	

	virtual bool move(size_t tarpos, size_t srcpos) = 0;
	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool insert(const T& elem, size_t cnt=1, size_t pos=~0) 		=0;	
	///////////////////////////////////////////////////////////////////////////
	// add cnt elements at position pos (at the end by default)
	virtual bool insert(const T* elem, size_t cnt, size_t pos=~0) 		=0;	
	///////////////////////////////////////////////////////////////////////////
	// add an list of elements at position pos (at the end by default)
	virtual bool insert(const TArray<T>& list, size_t pos=~0)	=0;	

	///////////////////////////////////////////////////////////////////////////
	// copy the given list
	virtual bool copy(const TArray<T>& list, size_t pos=0)	=0;
	///////////////////////////////////////////////////////////////////////////
	// copy the given list
	virtual bool copy(const T* elem, size_t cnt,size_t pos=0)	=0;

	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with list
	virtual bool replace(const TArray<T>& list, size_t pos, size_t poscnt)	=0;	
	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with cnt elements
	virtual bool replace(const T* elem, size_t cnt, size_t pos, size_t poscnt) 	=0;	


	///////////////////////////////////////////////////////////////////////////
	// find an element in the list
	virtual bool find(const T& elem, size_t startpos, size_t& pos) const =0;
	virtual int  find(const T& elem, size_t startpos=0) const =0;
};
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// dynamic size arrays
///////////////////////////////////////////////////////////////////////////////
template <class T> class TArrayDST : public TArray<T>
{
	///////////////////////////////////////////////////////////////////////////
	// friends
	friend class String;
	friend class SubString;
	virtual const T* array() const	{return cField;}
protected:
	///////////////////////////////////////////////////////////////////////////
	// data elements
	T		*cField;	// array
	size_t	cSZ;		// allocates array size
	size_t	cCnt;		// used elements

	///////////////////////////////////////////////////////////////////////////
	// copy and move for simple data types
	virtual void copy(T* tar, const T* src, size_t cnt)
	{
		memcpy(tar,src,cnt*sizeof(T));
	}
	virtual void move(T* tar, const T* src, size_t cnt)
	{
		memmove(tar,src,cnt*sizeof(T));
	}
	virtual int  compare(const T* a, const T* b)	
	{	// dont have a working compare here
		// overload at slist
		return 0;
	}

public:
	void realloc(size_t newsize)
	{	
		ScopeLock scopelock(*this);

		if(  cSZ < newsize )
		{	// grow rule
			size_t tarsize = newsize;
			newsize = 32;
			while( newsize < tarsize ) newsize *= 2;
		}
		else if( cSZ>32 && cCnt < cSZ/4 && newsize < cSZ/2)
		{	// shrink rule
			newsize = cSZ/2;
		}
		else // no change
			return;


		T *newfield = new T[newsize];
		if(newfield==NULL)
			throw exception_memory("TArrayDST: memory allocation failed");

		if(cField)
		{
			copy(newfield, cField, cCnt); // between read ptr and write ptr
			delete[] cField;
		}
		cSZ = newsize;
		cField = newfield;

	}
public:
	///////////////////////////////////////////////////////////////////////////
	// constructor / destructor
	TArrayDST() : cField(NULL),cSZ(0),cCnt(0)	{}
	TArrayDST(size_t sz) : cField(NULL),cSZ(0),cCnt(0)	{ resize(sz); }
	virtual ~TArrayDST()	{}

	///////////////////////////////////////////////////////////////////////////
	// copy constructor and assign (cannot be derived)
	TArrayDST(const TArray<T>& arr) : cField(NULL),cSZ(0),cCnt(0)	{ ScopeLock scopelock(*this); copy(arr); }
	const TArrayDST& operator=(const TArray<T>& arr)				{ ScopeLock scopelock(*this); copy(arr); return *this;}

	///////////////////////////////////////////////////////////////////////////
	// put a element to the list
	virtual bool push(const T& elem)			{ return append(elem); }
	virtual bool push(const TArray<T>& list)	{ return append(list); }
	virtual bool push(const T* elem, size_t cnt){ return append(elem,cnt); }
	///////////////////////////////////////////////////////////////////////////
	// return the last element and remove it from list
	virtual T& pop()
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			cCnt--;
			return cField[cCnt];
		}
#ifdef CHECK_BOUNDS
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST underflow");
#else
			static T dummy;
			return dummy;
#endif
#else
			return cField[0];
#endif
	}
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool pop(T& elem)
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			cCnt--;
			elem = cField[cCnt];
			return true;
		}
		else
		{
#ifdef CHECK_BOUNDS
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST underflow");
#endif
#endif
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// return the first element and do not remove it from list
	virtual T& top() const
	{
		ScopeLock scopelock(*this);
#ifdef CHECK_BOUNDS
		if( cCnt == 0 )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST underflow");
#else
			static T dummy;
			return dummy;
#endif
		}
#endif
			return const_cast<T&>(cField[0]);
	}
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool top(T& elem) const
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			elem = cField[0];
			return true;
		}
		return false;
	}


	///////////////////////////////////////////////////////////////////////////
	// direct access to the buffer, return Pointer and max buffer size
	virtual const T* getreadbuffer(size_t &maxcnt) const
	{
		Mutex::lock();
		if( cCnt >0 )
		{
			maxcnt = cCnt;
			// keep the mutex locked
			return const_cast<T*>(cField);
		}
		maxcnt = 0;
		Mutex::unlock();
		return NULL;
	}
	virtual T* getwritebuffer(size_t &maxcnt)
	{
		Mutex::lock();
		if( cCnt+maxcnt > cSZ )
			realloc( maxcnt+cCnt );
		return const_cast<T*>(cField+cCnt);
	}
	virtual bool setreadsize(size_t cnt)
	{
		bool ret = false;
		if( cnt <= cCnt)
		{
			if( cnt >0 )
			{
				move(cField+0, cField+cnt,cCnt-cnt);
				cCnt -= cnt;
			}
			ret = true;
		}
		Mutex::unlock();
		return ret;
	}
	virtual bool setwritesize(size_t cnt)
	{
		bool ret = false;
		if( cCnt+cnt < cSZ )
		{
			cCnt += cnt;
			ret = true;
		}
		Mutex::unlock();
		return ret;
	}
	///////////////////////////////////////////////////////////////////////////
	// copy cnt elements from list to buf, return number of copied elements
	virtual size_t copytobuffer(T* buf, size_t cnt)
	{
		ScopeLock scopelock(*this);
		if(buf)
		{
			if(cnt>cCnt) cnt = cCnt;
			copy(buf,cField,cnt);
			return cnt;
		}
		return 0;
	}
	///////////////////////////////////////////////////////////////////////////
	// access to element[inx]
	virtual const T& operator[](size_t inx) const
	{
		ScopeLock scopelock(*this);
#ifdef CHECK_BOUNDS
		// check for access to outside memory
		if( inx >= cSZ )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST out of bound");
#else
			static T dummy;
			return dummy;
#endif
		}
#endif
		return cField[inx];
	}
	virtual T &operator[](size_t inx)
	{
		ScopeLock scopelock(*this);
#ifdef CHECK_BOUNDS
		// check for access to outside memory
		if( inx >= cSZ )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST out of bound");
#else
			static T dummy;
			return dummy;
#endif
		}
#endif
		return cField[inx];
	}
	///////////////////////////////////////////////////////////////////////////
	// (re)allocates a list of cnt elements [0...cnt-1], 
	// leave new elements uninitialized/default constructed
	virtual bool resize(size_t cnt)			
	{
		ScopeLock scopelock(*this);
		if(cCnt>cnt) cCnt = cnt;	// shrink
		realloc(cnt);
		cCnt = cnt;					// growing
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// returns number of used elements
	virtual size_t size() const				{ScopeLock scopelock(*this); return cCnt;}	
	virtual size_t freesize() const			{ScopeLock scopelock(*this); return cSZ-cCnt;}	


	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const T& elem, size_t cnt=1)
	{
		ScopeLock scopelock(*this);
		if(cCnt+cnt > cSZ)
			realloc(cCnt+cnt);
		while(cnt--) cField[cCnt++] = elem;
		return true;
	}
	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const TArray<T>& list)
	{	
		ScopeLock scopelock(*this);
		size_t cnt;
		const T* elem = list.getreadbuffer(cnt);
		return append(elem, cnt);
	}
	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool append(const T* elem, size_t cnt)
	{
		ScopeLock scopelock(*this);
		if( elem )
		{
			if( cCnt+cnt > cSZ)
				realloc( cCnt+cnt );
			copy(cField+cCnt,elem,cnt);
			cCnt += cnt;
			return true;
		}
		return false;
	}

	///////////////////////////////////////////////////////////////////////////
	// remove elements from end of list
	virtual bool remove(size_t cnt=1)
	{
		ScopeLock scopelock(*this);
		if( cnt <= cCnt )
		{
			cCnt -= cnt;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// remove element [inx]
	virtual bool removeindex(size_t inx)
	{
		ScopeLock scopelock(*this);
		if(inx < cCnt)
		{
			move(cField+inx,cField+inx+1,cCnt-inx-1);
			cCnt--;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// remove cnt elements starting from inx
	virtual bool removeindex(size_t inx, size_t cnt)
	{
		ScopeLock scopelock(*this);
		if(inx < cCnt)
		{
			if(inx+cnt > cCnt)	cnt = cCnt-inx;
			move(cField+inx,cField+inx+cnt,cCnt-inx-cnt);
			cCnt -= cnt;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// remove all elements
	virtual bool clear()
	{
		ScopeLock scopelock(*this);
		cCnt = 0;
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool insert(const T& elem, size_t cnt=1, size_t pos=~0)
	{
		ScopeLock scopelock(*this);
		if( cCnt+cnt > cSZ )
			realloc(cSZ+cnt);

		if(pos >= cCnt) 
			pos = cCnt;
		else
			move(cField+pos+cnt, cField+pos, cCnt-pos);
		while(cnt--) cField[pos+cnt] = elem;
		cCnt+=cnt;
		return true;

	}
	///////////////////////////////////////////////////////////////////////////
	// add cnt elements at position pos (at the end by default)
	virtual bool insert(const T* elem, size_t cnt, size_t pos=~0)
	{
		ScopeLock scopelock(*this);
		if( elem )
		{	
			if( cCnt+cnt > cSZ )
				realloc(cCnt+cnt);

			if(pos >= cCnt) 
				pos=cCnt;
			else
				move(cField+pos+cnt, cField+pos, cCnt-pos);
			copy(cField+pos,elem,cnt);
			cCnt += cnt;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// add an list of elements at position pos (at the end by default)
	virtual bool insert(const TArray<T>& list, size_t pos=~0)
	{	
		ScopeLock scopelock(*this);
		size_t cnt;
		const T* elem = list.getreadbuffer(cnt);
		return insert(elem,cnt, pos);
	}
	///////////////////////////////////////////////////////////////////////////
	// copy cnt elements at position pos (at the end by default) overwriting existing elements
	virtual bool copy(const T* elem, size_t cnt, size_t pos=0)
	{
		ScopeLock scopelock(*this);
		if( elem )
		{	
			if(pos > cCnt) pos = cCnt;

			if( pos+cnt > cSZ )
				realloc(pos+cnt);
			copy(cField+pos,elem,cnt);
			cCnt = pos+cnt;
			return true;
		}
		return false;
	}
	virtual bool copy(const TArray<T>& list, size_t pos=0)
	{	
		ScopeLock scopelock(*this);
		size_t cnt;
		const T* elem = list.getreadbuffer(cnt);
		return copy(elem,cnt, pos);
	}
	///////////////////////////////////////////////////////////////////////////
	// Moving elements inside the buffer
	// always take the elements from 'from' up to 'cElements'

	virtual bool move(size_t tarpos, size_t srcpos)
	{	
		ScopeLock scopelock(*this);
		if(srcpos>cCnt) srcpos=cCnt; 
		if( cCnt+tarpos > cSZ+srcpos )
			realloc(cCnt+tarpos-srcpos);
		move(cField+tarpos,cField+srcpos,cCnt-srcpos);
		cCnt += tarpos-srcpos;
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with cnt elements
	virtual bool replace(const T* elem, size_t cnt, size_t pos, size_t poscnt)
	{
		ScopeLock scopelock(*this);
		if(pos > cCnt)
		{
			pos = cCnt;
			poscnt = 0;
		}
		if(pos+poscnt > cCnt) 
		{
			poscnt=cCnt-pos;
		}
		if( elem )
		{
			if( cCnt+cnt > cSZ+poscnt)
				realloc(cCnt+cnt-poscnt);

			move(cField+pos+cnt, cField+pos+poscnt,cCnt-pos-poscnt);
			copy(cField+pos,elem,cnt);
			cCnt += cnt-poscnt;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with list
	virtual bool replace(const TArray<T>& list, size_t pos, size_t poscnt)
	{	
		ScopeLock scopelock(*this);
		size_t cnt;
		const T* elem = list.getreadbuffer(cnt);
		return replace(elem,cnt, pos, poscnt);
	}
	///////////////////////////////////////////////////////////////////////////
	// find an element in the list
	virtual bool find(const T& elem, size_t startpos, size_t& pos) const
	{
		ScopeLock scopelock(*this);
		for(size_t i=startpos; i<cCnt; i++)
		{
			if( elem == cField[i] )
			{	pos = i;
				return true;
			}
		}
		return false;
	}
	virtual int  find(const T& elem, size_t startpos=0) const
	{
		ScopeLock scopelock(*this);
		for(size_t i=startpos; i<cCnt; i++)
		{
			if( elem == cField[i] )
			{	
				return i;
			}
		}
		return -1;
	}
};
///////////////////////////////////////////////////////////////////////////////
template <class T> class TFifoDST : public TArrayDST<T>
{
public:
	///////////////////////////////////////////////////////////////////////////
	// constructor / destructor
	TFifoDST()	{}
	virtual ~TFifoDST()	{}
	///////////////////////////////////////////////////////////////////////////
	// copy constructor and assign (cannot be derived)
	TFifoDST(const TArray<T>& arr)					{ ScopeLock scopelock(*this); copy(arr); }
	const TFifoDST& operator=(const TArray<T>& arr)	{ ScopeLock scopelock(*this); copy(arr); return *this; }

	///////////////////////////////////////////////////////////////////////////
	// return the first element and remove it from list
	virtual T& pop()
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			static T elem = cField[0];
			cCnt--;
			move(cField+0, cField+1,cCnt);
			return elem;
		}
#ifdef CHECK_BOUNDS
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST underflow");
#else
			static T dummy;
			return dummy;
#endif
#else
			return cField[0];
#endif
	}
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool pop(T& elem)
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			elem = cField[0];
			cCnt--;
			move(cField+0, cField+1,cCnt);
			return true;
		}
		else
		{
#ifdef CHECK_BOUNDS
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TfifoFST underflow");
#endif
#endif
		}
		return false;
	}

};
///////////////////////////////////////////////////////////////////////////////
template <class T> class TStackDST : public TArrayDST<T>
{
public:
	///////////////////////////////////////////////////////////////////////////
	// constructor / destructor
	TStackDST()				{}
	virtual ~TStackDST()	{}
	///////////////////////////////////////////////////////////////////////////
	// copy constructor and assign (cannot be derived)
	TStackDST(const TArray<T>& arr)						{ ScopeLock scopelock(*this); copy(arr); }
	const TStackDST& operator=(const TArray<T>& arr)	{ ScopeLock scopelock(*this); copy(arr); return *this; }

	///////////////////////////////////////////////////////////////////////////
	// return the first element and do not remove it from list
	virtual T& top() const
	{
		ScopeLock scopelock(*this);
#ifdef CHECK_BOUNDS
		// check for access to outside memory
		if( cCnt == 0 )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TArrayDST underflow");
#else
			static T dummy;
			return dummy;
#endif
		}
#endif
		return const_cast<T&>(cField[cCnt]);
	}
	///////////////////////////////////////////////////////////////////////////
	// as above but with check if element exist
	virtual bool top(T& elem) const
	{
		ScopeLock scopelock(*this);
		if( cCnt > 0 )
		{
			elem = cField[cCnt];
			return true;
		}
		else
		{
#ifdef CHECK_BOUNDS
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("TStackDST underflow");
#endif
#endif
		}
		return false;
	}

};
///////////////////////////////////////////////////////////////////////////////
// basic interface for sorted lists
///////////////////////////////////////////////////////////////////////////////
template <class T> class TslistDST : public TFifoDST<T>
{
	bool cAscending;// sorting order
	bool cAllowDup;	// allow duplicate entries (find might then not find specific elems)

	int compare(const T&a, const T&b)
	{
		if( a>b )		return (cAscending) ?  1:-1;
		else if( a<b )	return (cAscending) ? -1: 1;
		else			return 0;
	}
	virtual int  compare(const T* a, const T* b)	
	{	// no NULL test here
		if(*a > *b)
			return 1;
		else if(*a < *b)
			return -1;
		else
			return 0;
	}
public:
	///////////////////////////////////////////////////////////////////////////
	// destructor
	TslistDST(bool as=true, bool ad=false):cAscending(as),cAllowDup(ad) {}
	virtual ~TslistDST() {}
	///////////////////////////////////////////////////////////////////////////
	// copy constructor and assign (cannot be derived)
	TslistDST(const TArray<T>& arr,bool ad=false):cAllowDup(ad)		{ copy(arr); }
	const TslistDST& operator=(const TArray<T>& arr)				{ copy(arr); return *this; }

	///////////////////////////////////////////////////////////////////////////
	// add an element to the list
	virtual bool push(const T& elem) 				{ return insert(elem); }
	virtual bool push(const TArray<T>& list)		{ return insert(list); }
	virtual bool push(const T* elem, size_t cnt)	{ return insert(elem,cnt); }


	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos at the end by default
	virtual bool append(const T& elem)				{ return insert(elem); }
	virtual bool append(const TArray<T>& list) 		{ return insert(list); }
	virtual bool append(const T* elem, size_t cnt) 	{ return insert(elem,cnt); }


	///////////////////////////////////////////////////////////////////////////
	// add an element at position pos (at the end by default)
	virtual bool insert(const T& elem, size_t pos=~0)
	{
		ScopeLock scopelock(*this);

		// ignore position, insert sorted
		if( cCnt >= cSZ )
			realloc(cSZ+1);

		bool f = find(elem, 0, pos);
		if( !f || cAllowDup )
		{
			move(cField+pos+1, cField+pos, cCnt-pos);
			cCnt++;
			cField[pos] = elem;
			return true;
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	// add cnt elements at position pos (at the end by default)
	virtual bool insert(const T* elem, size_t cnt, size_t pos=~0)
	{
		ScopeLock scopelock(*this);
		if( cCnt+cnt>cSZ )
			realloc(cCnt+cnt);

		for(size_t i=0; i<cnt; i++)
		{	
			insert( elem[i] );
		}
		return true;
	}
	///////////////////////////////////////////////////////////////////////////
	// add an list of elements at position pos (at the end by default)
	virtual bool insert(const TArray<T>& list, size_t pos=~0)
	{
		ScopeLock scopelock(*this);
		if( cCnt+list.size()>cSZ )
			realloc(cCnt+list.size());

		for(size_t i=0; i<list.size(); i++)
		{
			insert( list[i] );
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// copy the given list
	virtual bool copy(const TArray<T>& list)
	{
		ScopeLock scopelock(*this);
		clear();
		return insert(list);
	}
	///////////////////////////////////////////////////////////////////////////
	// copy the given list
	virtual bool copy(const T* elem, size_t cnt)
	{
		ScopeLock scopelock(*this);
		clear();
		return insert(elem, cnt);
	}

	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with list
	virtual bool replace(const TArray<T>& list, size_t pos, size_t poscnt)
	{
		ScopeLock scopelock(*this);
		removeindex(pos,poscnt);
		return insert(list);
	}
	///////////////////////////////////////////////////////////////////////////
	// replace poscnt elements at pos with cnt elements
	virtual bool replace(const T* elem, size_t cnt, size_t pos, size_t poscnt)
	{
		ScopeLock scopelock(*this);
		removeindex(pos,poscnt);
		return insert(elem, cnt);
	}

	///////////////////////////////////////////////////////////////////////////
	// find an element in the list
	virtual bool find(const T& elem, size_t startpos, size_t& pos) const
	{	
		ScopeLock scopelock(*this);
		// do a binary search
		// make some initial stuff
		bool ret = false;
		size_t a= (startpos>=cCnt) ? 0 : startpos;
		size_t b=cCnt-1;
		size_t c;
		pos = 0;

		if( NULL==cField || cCnt < 1)
			ret = false;
		else if( elem == cField[a] ) 
		{	pos=a;
			ret = true;
		}
		else if( elem == cField[b] )
		{	pos = b;
			ret = true;
		}
		else if( cAscending )
		{	//smallest element first
			if( elem < cField[a] )
			{	pos = a;
				ret = false; //less than lower
			}
			else if( elem > cField[b] )
			{	pos = b+1;
				ret = false; //larger than upper
			}
			else
			{	// binary search
				do
				{
					c=(a+b)/2;
					if( elem == cField[c] )
					{	b=c;
						ret = true;
						break;
					}
					else if( elem < cField[c] )
						b=c;
					else
						a=c;
				}while( (a+1) < b );
				pos = b;//return the next larger element to the given or the found element
			}
		}
		else // descending
		{	//smallest element last
			if( elem > cField[a] )
			{	pos = a;
				ret = false; //larger than lower
			}
			else if( elem < cField[b] )	// v1
			{	pos = b+1;
				ret = false; //less than upper
			}
			else
			{	// binary search
				do
				{
					c=(a+b)/2;
					if( elem == cField[c] )
					{	b=c;
						ret = true;
						break;
					}
					else if( elem > cField[c] )
						b=c;
					else
						a=c;
				}while( (a+1) < b );
				pos = b;//return the next smaller element to the given or the found element
			}
		}
		return ret;
	}
	virtual int  find(const T& elem, size_t startpos=0) const
	{
		ScopeLock scopelock(*this);
		size_t pos;
		if( find(elem,startpos, pos) )
			return pos;
		return -1;
	}

};
///////////////////////////////////////////////////////////////////////////////







#endif//_BASE_H_

