// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_SOCKET_H_
#define _SOCKET_H_

#include "base.h"
#include "malloc.h"
#include "timer.h"

extern unsigned long ticks;
extern time_t tick_;
//extern time_t stall_time_;



// just kill the exeption here, will port the exeptions later
#define exception_bound 
#define exception_memory


///////////////////////////////////////////////////////////////////////////////
// IP number stuff
///////////////////////////////////////////////////////////////////////////////
class ipaddress
{
    union
    {
        uchar   bdata[4];
        ulong   ldata;
    };
public:
    ipaddress():ldata(INADDR_LOOPBACK)          {}
    ipaddress(ulong a):ldata(a)                 {}
    ipaddress(const ipaddress& a):ldata(a.ldata){}
    ipaddress(int a, int b, int c, int d)
	{
		ldata =	 (a&0xFF) << 0x18
				|(b&0xFF) << 0x10
				|(c&0xFF) << 0x08
				|(d&0xFF)        ;
	}
    ipaddress& operator= (ulong a)              { ldata = a; return *this; }
    ipaddress& operator= (const ipaddress& a)   { ldata = a.ldata; return *this; }
    uchar& operator [] (int i)                  
	{ 
#ifdef CHECK_BOUNDS
		if( (i>=4) || (i<0) )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("ipaddress: out of bound access");
#else//!CHECK_EXCEPTIONS
			static uchar dummy;
			return dummy=0;
#endif//!CHECK_EXCEPTIONS
		}
#endif//CHECK_BOUNDS

		if( MSB_FIRST == CheckByteOrder() )
			return bdata[i];
		else
			return bdata[3-i];
	}
    const uchar operator [] (int i) const
	{ 
#ifdef CHECK_BOUNDS
		if( (i>=4) || (i<0) )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("ipaddress: out of bound access");
#else//!CHECK_EXCEPTIONS
			return 0;
#endif//!CHECK_EXCEPTIONS
		}
#endif//CHECK_BOUNDS

		if( MSB_FIRST == CheckByteOrder() )
			return bdata[i];
		else
			return bdata[3-i];
	}

	void toBuffer(uchar *buf) const
	{	// implement little endian buffer format
		// IPs are placed in network byte order to the buffer
		if(buf)
		{
			buf[0] = (uchar)((ldata >> 0x18)&0xFF);
			buf[1] = (uchar)((ldata >> 0x10)&0xFF);
			buf[2] = (uchar)((ldata >> 0x08)&0xFF);
			buf[3] = (uchar)((ldata        )&0xFF);
		}
	}
	void fromBuffer(const uchar *buf)
	{	// implement little endian buffer format
		// IPs are placed in network byte order to the buffer
		if(buf)
		{
			ldata =	 buf[0] << 0x18
					|buf[1] << 0x10
					|buf[2] << 0x08
					|buf[3]        ;
		}
	}

    operator ulong() const                      { return ldata; }
};







/////////////////////////////////////////////////////////////////////
//
//  Basic FIFO Template
//
/////////////////////////////////////////////////////////////////////
template<class T> class TFIFO// : public global, public noncopyable		!!todo!! dont forget to remove when merging with base objects
{
public:
	TFIFO()				{}
	virtual ~TFIFO()	{}
	/////////////////////////////////////////////////////////////////
	virtual bool	isEmpty() const = 0;
	virtual bool	isFull() const = 0;
	virtual size_t	usedSize() const = 0;
	virtual size_t	nextusedSize() const = 0;
	virtual size_t	freeSize() const = 0;
	virtual size_t	nextfreeSize() const = 0;
	size_t	Count() {return usedSize();}
	/////////////////////////////////////////////////////////////////
	virtual const T& operator[](size_t i) const = 0;
	virtual T& operator()(size_t i) = 0;
	/////////////////////////////////////////////////////////////////
	virtual bool write(const T* p, size_t sz) = 0;
	virtual bool read(T* p, size_t sz) = 0;
	/////////////////////////////////////////////////////////////////
	virtual bool push(const T& p) = 0;
	virtual bool top(T& p) = 0;
	virtual T top() = 0;
	virtual bool pop(T& p) = 0;
	virtual T pop() = 0;
	/////////////////////////////////////////////////////////////////
};

/////////////////////////////////////////////////////////////////////
//
// Dynamically resizable FIFO Template
// rotating behaviour so no need to memmove data
//
/////////////////////////////////////////////////////////////////////
template<class T> class TFIFODST : public TFIFO<T>
{
	///////////////////////////////////////////////////////////////////////////
	T*		cBuffer;
	size_t	cSZ;
	size_t	cRD;
	size_t	cWR;

private:
	///////////////////////////////////////////////////////////////////////////
	// overload with a suitable function for copying your types
	// can be either memcpy for simple types or a loop with assignments for complex
	virtual void Copy(T* tar, const T* src, size_t sz)
	{
		memcpy(tar,src,sz*sizeof(T));
	}

	///////////////////////////////////////////////////////////////////////////
	// realloc for sz _MORE_ data able to fit into the fifo
	void ReAlloc(size_t sz)
	{
		size_t newsize;
		if(  freeSize() < sz )
		{	// grow rule
			size_t tarsize = usedSize() + sz;
			newsize = 16 + cSZ*2;
			while( newsize < tarsize ) newsize *= 2;
		}
		else if( cSZ>16 && usedSize() < cSZ/4 )
		{	// shrink rule
			newsize = cSZ/2;
		}
		else // no change
			return;

		T *newfield = new T[newsize];
		if(newfield==NULL)
			throw exception_memory("TFIFOD: memory allocation failed");

		// copy defragmented fifo content to read-in-line
		if( cRD > cWR )
		{
			Copy(newfield+0,       cBuffer+cRD, cSZ-cRD); // between read ptr and buffer end
			Copy(newfield+cSZ-cRD, cBuffer    , cWR    ); // between buffer start and write ptr
			cWR = cWR+cSZ-cRD;
		}
		else
		{
			Copy(newfield+0,       cBuffer+cRD, cWR-cRD); // between read ptr and write ptr
			cWR = cWR-cRD;
		}
		cRD = 0;
		cSZ = newsize;

		delete[] cBuffer;
		cBuffer = newfield;
	}

public:
	///////////////////////////////////////////////////////////////////////////
	TFIFODST(size_t sz=16): cBuffer(new T[sz]),cSZ(sz),cRD(0),cWR(0)
	{}
	~TFIFODST()	{if(cBuffer) delete[] cBuffer;}

	///////////////////////////////////////////////////////////////////////////
	// true when empty
	virtual bool	isEmpty() const		{return (cRD==cWR);}
	///////////////////////////////////////////////////////////////////////////
	// true when full (actually when only one place is left)
	virtual bool	isFull() const		{return (cRD==((cWR+1)%cSZ));}
	///////////////////////////////////////////////////////////////////////////
	// returns the overall used size
	virtual size_t	usedSize() const	{return (cWR>=cRD) ? (   cWR-cRD):(cSZ-cRD+cWR);}
	// returns the size of the next used data block
	virtual size_t	nextusedSize() const{return (cWR>=cRD) ? (   cWR-cRD):(cSZ-cRD    );}
	///////////////////////////////////////////////////////////////////////////
	// returns the overall free size, 
	virtual size_t	freeSize() const	{if(cWR==cRD) return cSZ; return (cWR>cRD) ? (cSZ-cWR+cRD-1):(    cRD-cWR-1);}
	///////////////////////////////////////////////////////////////////////////
	// returns the size of the next free data block
	virtual size_t	nextfreeSize() const{return (cWR>=cRD) ? (cSZ-cWR    ):(    cRD-cWR-1);}
	///////////////////////////////////////////////////////////////////////////
	T* ReserveRead(const size_t wish, size_t &sz)
	{ 
		sz = nextusedSize();
		if( wish < sz ) sz = wish;
		// wish greater cannot be granted because of 
		// either buffer fragmentation or possible buffer underflow 

		T* ret = cBuffer+cRD;
		cRD+=sz;
		if(cRD>=cSZ) cRD-=cSZ;

		return ret;
	}
	void UndoReserveRead(const size_t sz)
	{
		if(cRD >= sz) // otherwise something is really fishy
			cRD -= sz;
	}
	T* ReserveWrite(const size_t wish, size_t &sz)	
	{ 
		size_t totalfree = freeSize();

		// if the data won't fit in we can realloc the buffer already
		// there will be a second ReserveWrite call anyway would now split the date
		if( totalfree < wish )
			ReAlloc(wish);

		sz = nextfreeSize();
		if( wish < sz ) sz = wish;
		// wish greater cannot be granted because of buffer fragmentation
		// possible buffer overflow was already handled above

		T* ret = cBuffer+cWR;
		cWR+=sz;
		if(cWR>=cSZ) cWR-=cSZ;

		return ret;
	}
	void UndoReserveWrite(const size_t sz)
	{
		if(cWR >= sz) // otherwise something is really fishy
			cWR -= sz;
	}

	///////////////////////////////////////////////////////////////////////////
	virtual const T& operator[](size_t i) const
	{	// read operator, goes on read pointer
		static T dummy;
		if(cWR>=cRD)
		{	
			if( (cWR-cRD) > i ) 
			{	// enough space left between write pointer and read pointer
				return cBuffer[cRD+i];
			}
			else	// does not fit into the buffer
			{
#ifdef CHECK_BOUNDS
				// throw something instead
				throw exception_bound("FIFO underflow");
#endif
				return dummy;
			}
		}
		else
		{	
			if( (cSZ-cRD) > i ) 
			{	// enough space left between write pointer and end of buffer
				return cBuffer[cRD+i];
			}
			else if( cWR > (i-(cSZ-cRD)) )
			{	// split it up to two potions
				// check if it fits
				return cBuffer[i-(cSZ-cRD)];
			}
			else	// does not fit into the buffer
			{
#ifdef CHECK_BOUNDS
				// throw something instead
				throw exception_bound("FIFO underflow");
#endif
				return dummy;
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////
	virtual T& operator()(size_t i)
	{	// write operator, goes on write pointer
		// ignore the max-1 case here
		if(cWR>=cRD)
		{	
			if( (cSZ-cWR) > i ) 
			{	// enough space left between write pointer and end of buffer
				return cBuffer[cWR+i];
			}
			else if( cRD > (i-(cSZ-cWR)) )
			{	// split it up to two potions
				// check if it fits
				return cBuffer[i-(cSZ-cWR)];
			}
			else	// does not fit into the buffer
			{
				ReAlloc(i);
				return cBuffer[cWR+i];
			}
		}
		else
		{	
			if( (cRD-cWR) > i ) 
			{	// enough space left between write pointer and read pointer
				return cBuffer[cWR+i];
				
			}
			else	// does not fit into the buffer
			{
				ReAlloc(i);
				return cBuffer[cWR+i];
			}
		}	
	}

	///////////////////////////////////////////////////////////////////////////
	virtual bool write(const T* p, size_t sz)
	{
		if(p && sz)
		if(cWR>=cRD)
		{	
			if( ((cSZ-cWR) > sz) || (((cSZ-cWR) == sz) && cRD>0) )
			{	// enough space left between write pointer and end of buffer
				Copy(cBuffer+cWR, p, sz);
				cWR += sz;
				if(cWR>=cSZ) cWR=0;
			}
			else if( cRD > (sz-(cSZ-cWR)) )
			{	// split it up to two potions
				// check if it fits
				Copy(cBuffer+cWR, p,            cSZ-cWR);
				Copy(cBuffer,     p+cSZ-cWR, sz-(cSZ-cWR));
				cWR = sz-(cSZ-cWR);
			}
			else	// does not fit into the buffer
			{
				ReAlloc(sz);
				Copy(cBuffer+cWR, p, sz);
				cWR += sz;
			}
		}
		else
		{	
			if( (cRD-cWR) > sz ) 
			{	// enough space left between write pointer and read pointer
				Copy(cBuffer+cWR, p, sz);
				cWR += sz;
			}
			else	// does not fit into the buffer
			{
				ReAlloc(sz);
				Copy(cBuffer+cWR, p, sz);
				cWR += sz;
			}
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	virtual bool read(T* p, size_t sz)
	{
		if(p && sz)
		if(cWR>=cRD)
		{	
			if( (cWR-cRD) >= sz ) 
			{	// enough space left between write pointer and read pointer
				Copy(p, cBuffer+cRD, sz);
				cRD += sz;
			}
			else	// does not fit into the buffer
			{
#ifdef CHECK_BOUNDS
				// throw something instead
				throw exception_bound("FIFO underflow");
#endif
				return false;
			}
		}
		else
		{	
			if( (cSZ-cRD) >= sz ) 
			{	// enough space left between write pointer and end of buffer
				Copy(p, cBuffer+cRD, sz);
				cRD += sz;
				if(cRD>=cSZ) cRD=0;
			}
			else if( cWR >= (sz-(cSZ-cRD)) )
			{	// split it up to two potions
				// check if it fits
				Copy(p,        cBuffer+cRD,     cSZ-cRD);
				Copy(p+cSZ-cRD, cBuffer,     sz-(cSZ-cRD));
				cRD = sz-(cSZ-cRD);
			}
			else	// does not fit into the buffer
			{
#ifdef CHECK_BOUNDS
				// throw something instead
				throw exception_bound("FIFO underflow");
#endif
				return false;
			}

		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	virtual bool push(const T& p)
	{

		if( (cSZ<16) || (((cWR+1)%cSZ)==cRD) )
		{	// buffer full
			// actually not really full but we cannot write at the last empty field
			// because this is necessary to enable an empty/full detection
			ReAlloc(1);
		}

		// write between write and read pointer
		// or between write pointer and end of buffer
		cBuffer[cWR++] = p;
		if(cWR>=cSZ) cWR=0;
		return true;
	}
	///////////////////////////////////////////////////////////////////////////
	virtual bool top(T& p)
	{
		if(cWR==cRD)
		{	// buffer empty
#ifdef CHECK_BOUNDS
			throw exception_bound("FIFO underflow");
#endif
			return false;
		}
		else 
		{	// read between read and write pointer
			// or between read pointer and end of buffer
			p = cBuffer[cRD];
			return true;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	virtual T top()
	{
#ifdef CHECK_BOUNDS
		if(cWR==cRD)
		{	// buffer empty
			// throw something instead
			throw exception_bound("FIFO underflow");
		}
#endif
		return cBuffer[cRD];
	}
	///////////////////////////////////////////////////////////////////////////
	virtual bool pop(T& p)
	{
		if(cWR==cRD)
		{	// buffer empty
#ifdef CHECK_BOUNDS
			throw exception_bound("FIFO underflow");
#endif
			return false;
		}
		else 
		{	// read between read and write pointer
			// or between read pointer and end of buffer
			p = cBuffer[cRD++];
			if(cRD>=cSZ) cRD=0;
			return true;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	virtual T pop()
	{
		if(cWR==cRD)
		{	// buffer empty
#ifdef CHECK_BOUNDS
			throw exception_bound("FIFO underflow");
#endif
			return cBuffer[cRD];
		}
		else 
		{	// read between read and write pointer
			// or between read pointer and end of buffer
			T *p = cBuffer+cRD;
			cRD++;
			if(cRD>=cSZ) cRD=0;
			return *p;
		}
	}
};

class CFIFO : public TFIFODST<unsigned char>
{

	char * cStrBuf;
	size_t cStrSZ;

public:
	CFIFO():cStrBuf(NULL),cStrSZ(0)
	{}
	~CFIFO()
	{
		if(cStrBuf) 
		{
			delete[] cStrBuf;
			cStrBuf = NULL;
			cStrSZ = 0;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	unsigned char operator = (const unsigned char ch)
	{	// no check push will throw anyway
		push(ch);
		return ch;
	}
	char operator = (const char ch)
	{	
		push((unsigned char)ch);
		return ch;
	}
	///////////////////////////////////////////////////////////////////////////
	unsigned short operator = (const unsigned short sr)
	{	// implement little endian buffer format
		push( (unsigned char)(sr         ) ); 
		push( (unsigned char)(sr  >> 0x08) );
		return sr;
	}
	short operator = (const short sr)
	{	// implement little endian buffer format
		push( (unsigned char)(sr         ) ); 
		push( (unsigned char)(sr  >> 0x08) );
		return sr;
	}
	///////////////////////////////////////////////////////////////////////////
	unsigned long operator = (const unsigned long ln)
	{	// implement little endian buffer format
		push( (unsigned char)(ln          ) );
		push( (unsigned char)(ln  >> 0x08 ) );
		push( (unsigned char)(ln  >> 0x10 ) );
		push( (unsigned char)(ln  >> 0x18 ) );
		return ln;
	}
	long operator = (const long ln)
	{	// implement little endian buffer format
		push( (unsigned char)(ln          ) );
		push( (unsigned char)(ln  >> 0x08 ) );
		push( (unsigned char)(ln  >> 0x10 ) );
		push( (unsigned char)(ln  >> 0x18 ) );
		return ln;
	}
	///////////////////////////////////////////////////////////////////////////
	ipaddress operator = (const ipaddress ip)
	{	// implement little endian buffer format
		// IPs are given in network byte order to the buffer
		push( (unsigned char)(ip[3]) );
		push( (unsigned char)(ip[2]) );
		push( (unsigned char)(ip[1]) );
		push( (unsigned char)(ip[0]) );
		return ip;
	}


	///////////////////////////////////////////////////////////////////////////
	int64 operator = (const int64 lx)
	{	// implement little endian buffer format
		push( (unsigned char)(lx          ) );
		push( (unsigned char)(lx  >> 0x08 ) );
		push( (unsigned char)(lx  >> 0x10 ) );
		push( (unsigned char)(lx  >> 0x18 ) );
		push( (unsigned char)(lx  >> 0x20 ) );
		push( (unsigned char)(lx  >> 0x28 ) );
		push( (unsigned char)(lx  >> 0x30 ) );
		push( (unsigned char)(lx  >> 0x38 ) );
		return lx;
	}
	uint64 operator = (const uint64 lx)
	{	// implement little endian buffer format
		push( (unsigned char)(lx          ) );
		push( (unsigned char)(lx  >> 0x08 ) );
		push( (unsigned char)(lx  >> 0x10 ) );
		push( (unsigned char)(lx  >> 0x18 ) );
		push( (unsigned char)(lx  >> 0x20 ) );
		push( (unsigned char)(lx  >> 0x28 ) );
		push( (unsigned char)(lx  >> 0x30 ) );
		push( (unsigned char)(lx  >> 0x38 ) );
		return lx;
	}

	///////////////////////////////////////////////////////////////////////////
	operator unsigned char ()
	{	// no check, pop will return 0 or throw anyway
		return pop();
	}
	operator char()
	{	// no check, pop will return 0 anyway
		return pop();
	}
	///////////////////////////////////////////////////////////////////////////
	operator unsigned short()
	{	// implement little endian buffer format
		unsigned short sr;
#ifdef CHECK_BOUNDS
		if(usedSize()<2)
			throw exception_bound("FIFO underflow");
#endif
		sr  = ((unsigned short)pop()        ); 
		sr |= ((unsigned short)pop() << 0x08);
		return sr;
	}
	operator short ()
	{	// implement little endian buffer format
		short sr;
#ifdef CHECK_BOUNDS
		if(usedSize()<2)
			throw exception_bound("FIFO underflow");
#endif
		sr  = ((short)pop()        ); 
		sr |= ((short)pop() << 0x08);
		return sr;
	}
	///////////////////////////////////////////////////////////////////////////
	operator unsigned long ()
	{	// implement little endian buffer format
		unsigned long ln;
#ifdef CHECK_BOUNDS
		if(usedSize()<4)
			throw exception_bound("FIFO underflow");
#endif
		ln  = ((unsigned long)pop()        ); 
		ln |= ((unsigned long)pop() << 0x08);
		ln |= ((unsigned long)pop() << 0x10);
		ln |= ((unsigned long)pop() << 0x18);
		return ln;
	}
	operator long ()
	{	// implement little endian buffer format
		long ln;
#ifdef CHECK_BOUNDS
		if(usedSize()<4)
			throw exception_bound("FIFO underflow");
#endif
		ln  = ((long)pop()        ); 
		ln |= ((long)pop() << 0x08);
		ln |= ((long)pop() << 0x10);
		ln |= ((long)pop() << 0x18);
		return ln;
	}
	///////////////////////////////////////////////////////////////////////////

	operator ipaddress ()
	{	// implement little endian buffer format
#ifdef CHECK_BOUNDS
		if(usedSize()<4)
			throw exception_bound("FIFO underflow");
#endif
		ipaddress ip( pop(),pop(),pop(),pop() );
		return  ip;
	}
	///////////////////////////////////////////////////////////////////////////
	operator int64 ()
	{	// implement little endian buffer format
		int64 lx;
#ifdef CHECK_BOUNDS
		if(usedSize()<8)
			throw exception_bound("FIFO underflow");
#endif
		lx  = ((int64)pop()        ); 
		lx |= ((int64)pop() << 0x08);
		lx |= ((int64)pop() << 0x10);
		lx |= ((int64)pop() << 0x18);
		lx |= ((int64)pop() << 0x20);
		lx |= ((int64)pop() << 0x28);
		lx |= ((int64)pop() << 0x30);
		lx |= ((int64)pop() << 0x38);
		return lx;
	}
	operator uint64 ()
	{	// implement little endian buffer format
		uint64 lx;
#ifdef CHECK_BOUNDS
		if(usedSize()<8)
			throw exception_bound("FIFO underflow");
#endif
		lx  = ((uint64)pop()        ); 
		lx |= ((uint64)pop() << 0x08);
		lx |= ((uint64)pop() << 0x10);
		lx |= ((uint64)pop() << 0x18);
		lx |= ((uint64)pop() << 0x20);
		lx |= ((uint64)pop() << 0x28);
		lx |= ((uint64)pop() << 0x30);
		lx |= ((uint64)pop() << 0x38);
		return lx;
	}
	///////////////////////////////////////////////////////////////////////////

	const char* operator = (const char * c)
	{	// write will throw exception_bound automatically
		if(c) write((const unsigned char*)c, strlen(c)+1);
		return c;
	}


	operator const char*()
	{
		size_t i, used = usedSize();
		// find the EOS
		// cannot do anything else then going through the array
		for(i=0; i<used; i++)
			if( this->operator[](i) == 0 )
				break;
		if(i<used)
		{	
			// make sure the string buffer is long enough
			if( cStrSZ < i+1 )
			{	// buffer is not sufficient, reallocate
				if(cStrBuf) delete[] cStrBuf;
				cStrSZ = i+1;
				cStrBuf = new char[i+1];
			} 
			// reuse the existing buffer otherwise
			read((unsigned char*)cStrBuf, i+1);
		}
		else
		{	// otherwise we did not find an EOS, 
			// maybe there is no string in the buffer or it is not complete yet
			if(NULL==cStrBuf)
			{	// make a default buffer in not exist
				cStrSZ = 64; 
				cStrBuf = new char[64];
			}
			cStrBuf[0] = 0;
		}
		return cStrBuf;
	}

	///////////////////////////////////////////////////////////////////////////
};







// Class for assigning/reading words from a buffer
class objW
{
	unsigned char *ip;
public:
	objW():ip(NULL)										{}
	objW(unsigned char* x):ip(x)						{}
	objW(unsigned char* x,int pos):ip(x+pos)			{}
	objW(char* x):ip((unsigned char*)x)					{}
	objW(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objW& init(unsigned char* x)			{ip=x;return *this;}
	objW& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objW& init(char* x)						{ip=(unsigned char*)x;return *this;}
	objW& init(char* x,int pos)				{ip=(unsigned char*)(x+pos);return *this;}

	operator unsigned short() const
	{return this->operator()();
	}
	unsigned short operator()()	const
	{
		if(ip)
		{	
			return	 ( ((unsigned short)(ip[0]))        )
					|( ((unsigned short)(ip[1])) << 0x08);
			
		}
		return 0;
	}
	objW& operator=(const objW& objw)
	{
		if(ip && objw.ip)
		{	
			memcpy(ip, objw.ip, 2);
		}
		return *this;
	}
	unsigned short operator=(unsigned short valin)
	{	
		if(ip)
		{
			ip[0] = ((valin & 0x00FF)          );
			ip[1] = ((valin & 0xFF00)  >> 0x08 );
		}
		return valin;
	}
};

// Class for assigning/reading dwords from a buffer
class objL
{
	unsigned char *ip;
public:
	objL():ip(NULL)										{}
	objL(unsigned char* x):ip(x)						{}
	objL(unsigned char* x,int pos):ip(x+pos)			{}
	objL(char* x):ip((unsigned char*)x)					{}
	objL(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objL& init(unsigned char* x)			{ip=x;return *this;}
	objL& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objL& init(char* x)						{ip=(unsigned char*)x;return *this;}
	objL& init(char* x,int pos)				{ip=(unsigned char*)(x+pos);return *this;}


	operator unsigned long() const
	{return this->operator()();
	}
	unsigned long operator()()	const
	{
		if(ip)
		{	
			return	 ( ((unsigned long)(ip[0]))        )
					|( ((unsigned long)(ip[1])) << 0x08)
					|( ((unsigned long)(ip[2])) << 0x10)
					|( ((unsigned long)(ip[3])) << 0x18);
			
		}
		return 0;
	}

	objL& operator=(const objL& objl)
	{
		if(ip && objl.ip)
		{	
			memcpy(ip, objl.ip, 4);
		}
		return *this;
	}
	unsigned long operator=(unsigned long valin)
	{	
		if(ip)
		{
			ip[0] = (unsigned char)((valin & 0x000000FF)          );
			ip[1] = (unsigned char)((valin & 0x0000FF00)  >> 0x08 );
			ip[2] = (unsigned char)((valin & 0x00FF0000)  >> 0x10 );
			ip[3] = (unsigned char)((valin & 0xFF000000)  >> 0x18 );
		}
		return valin;
	}
};

// Class for assigning/reading words from a buffer 
// without changing byte order
// necessary for IP numbers, which are always in network byte order
// it is implemented this way and I currently won't interfere with that
// even if it is quite questionable

// changes: 
// transfered all IP addresses in the programms to host byte order
// since IPs are transfered in network byte order
// we cannot use objL but its complementary here
class objLIP	
{
	unsigned char *ip;
public:
	objLIP():ip(NULL)									{}
	objLIP(unsigned char* x):ip(x)						{}
	objLIP(unsigned char* x,int pos):ip(x+pos)			{}
	objLIP(char* x):ip((unsigned char*)x)				{}
	objLIP(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objLIP& init(unsigned char* x)			{ip=x;return *this;}
	objLIP& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objLIP& init(char* x)					{ip=(unsigned char*)x;return *this;}
	objLIP& init(char* x,int pos)			{ip=(unsigned char*)(x+pos);return *this;}

	operator unsigned long() const
	{return this->operator()();
	}
	unsigned long operator()()	const
	{
		if(ip)
		{	
//			register unsigned long tmp;
//			memcpy(&tmp,ip,4);
//			return tmp;
			return	 ( ((unsigned long)(ip[3]))        )
					|( ((unsigned long)(ip[2])) << 0x08)
					|( ((unsigned long)(ip[1])) << 0x10)
					|( ((unsigned long)(ip[0])) << 0x18);


		}
		return 0;
	}
	objLIP& operator=(const objLIP& objl)
	{
		if(ip && objl.ip)
		{	
			memcpy(ip, objl.ip, 4);
		}
		return *this;
	}
	unsigned long operator=(unsigned long valin)
	{	
		if(ip)
		{
//			memcpy(ip, &valin, 4);

			ip[3] = (unsigned char)((valin & 0x000000FF)          );
			ip[2] = (unsigned char)((valin & 0x0000FF00)  >> 0x08 );
			ip[1] = (unsigned char)((valin & 0x00FF0000)  >> 0x10 );
			ip[0] = (unsigned char)((valin & 0xFF000000)  >> 0x18 );
		}
		return valin;
	}
};


// define declaration

#define RFIFOP(fd,pos) (session[fd]->rdata+session[fd]->rdata_pos+(pos))
#define RFIFOB(fd,pos) (*((unsigned char*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))
#define RFIFOW(fd,pos) (objW(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOL(fd,pos) (objL(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOLIP(fd,pos) (objLIP(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOREST(fd) ((int)(session[fd]->rdata_size-session[fd]->rdata_pos))
#define RFIFOSPACE(fd) ((int)(session[fd]->rdata_max-session[fd]->rdata_size))
#define RBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define RBUFB(p,pos) (*((unsigned char*)RBUFP((p),(pos))))
#define RBUFW(p,pos) (objW(p,pos))
#define RBUFL(p,pos) (objL(p,pos))
#define RBUFLIP(p,pos) (objLIP(p,pos))

#define WFIFOSPACE(fd) (session[fd]->wdata_max-session[fd]->wdata_size)
#define WFIFOP(fd,pos) (session[fd]->wdata+session[fd]->wdata_size+(pos))
#define WFIFOB(fd,pos) (*((unsigned char*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))
#define WFIFOW(fd,pos) (objW(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WFIFOL(fd,pos) (objL(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WFIFOLIP(fd,pos) (objLIP(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define WBUFB(p,pos) (*((unsigned char*)WBUFP((p),(pos))))
#define WBUFW(p,pos) (objW((p),(pos)))
#define WBUFL(p,pos) (objL((p),(pos)))
#define WBUFLIP(p,pos) (objLIP((p),(pos)))



#ifdef __INTERIX
#define FD_SETSIZE 4096
#endif	// __INTERIX


/* Removed Cygwin FD_SETSIZE declarations, now are directly passed on to the compiler through Makefile [Valaris] */


// Struct declaration
struct socket_data{
	struct {
		bool connected : 1;			// true when connected
		bool remove : 1;			// true when to be removed
		bool marked : 1;			// true when deleayed removal is initiated (optional)
	}flag;

	unsigned char *rdata;		// buffer
	size_t rdata_max;			// size of buffer
	size_t rdata_size;			// size of data
	size_t rdata_pos;			// iterator within data

	time_t rdata_tick;			// tick of last read

	unsigned char *wdata;		// buffer
	size_t wdata_max;			// size of buffer
	size_t wdata_size;			// size of data

	unsigned long client_ip;	// just an ip in host byte order is enough (4byte instead of 16)

	int (*func_recv)(int);
	int (*func_send)(int);
	int (*func_parse)(int);
	int (*func_term)(int);
	int (*func_console)(char*);
	void* session_data;
};

// Data prototype declaration

#ifdef WIN32
		#undef FD_SETSIZE
		#define FD_SETSIZE 4096
#endif

extern struct socket_data *session[FD_SETSIZE];
extern int fd_max;



extern inline bool session_isValid(int fd)
{
	return ( (fd>=0) && (fd<FD_SETSIZE) && (NULL!=session[fd]) );
}
extern inline bool session_isActive(int fd)
{
	return ( session_isValid(fd) && session[fd]->flag.connected );
}
extern inline bool session_isRemoved(int fd)
{
	return ( session_isValid(fd) && session[fd]->flag.remove );
}
extern inline bool session_isMarked(int fd)
{
	return ( session_isValid(fd) && session[fd]->flag.marked );
}
extern inline bool session_Remove(int fd)
{
	if( session_isValid(fd) && !session[fd]->flag.marked )
		session[fd]->flag.remove = true;
	return session[fd]->flag.remove;
}
extern inline bool session_SetTermFunction(int fd, int (*term)(int))
{
	if( session_isValid(fd) )
		session[fd]->func_term = term;
	return true;
}

bool session_SetWaitClose(int fd, unsigned long timeoffset);
bool session_Delete(int fd);

// Function prototype declaration

int make_listen    (unsigned long ip, unsigned short port);
int make_connection(unsigned long ip, unsigned short port);

int realloc_fifo(int fd, size_t rfifo_size,size_t wfifo_size);

int WFIFOSET(int fd, size_t len);
int RFIFOSKIP(int fd, size_t len);

int do_sendrecv(int next);

void socket_init(void);
void socket_final(void);

void flush_fifos();

int start_console(void);

void set_defaultparse(int (*defaultparse)(int));
void set_defaultconsoleparse(int (*defaultparse)(char*));

extern unsigned long addr_[16];	// ip addresses of local host (host byte order)
extern unsigned int naddr_;		// # of ip addresses


#endif	// _SOCKET_H_
