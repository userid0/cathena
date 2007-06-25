#ifndef __BASEOBJECTS_H__
#define __BASEOBJECTS_H__

#include "basetypes.h"

//////////////////////////////////////////////////////////////////////////
/// basic abstract classes
//////////////////////////////////////////////////////////////////////////

NAMESPACE_BEGIN(basics)

int unixerrno();
const char* unixerrmsg(int code);


// possibly undefined error numbers on win32
#ifdef WIN32
#ifndef ENOTBLK
#define ENOTBLK 15
#endif
#ifndef EUCLEAN
#define EUCLEAN 35
#endif
#ifndef ETXTBSY
#define ETXTBSY 26
#endif
#endif


//////////////////////////////////////////////////////////////////////////
/// base class to prevent copy/assign
//////////////////////////////////////////////////////////////////////////
class noncopyable 
{
// my gnu ver. 3.4 don't like private constructors
// looks like some gcc really insists on having this members readable
// even if not used and not disturbing in any way, 
// stupid move, but ok, go on, read it 
// but the protected declaration will then generate errors in linker instead of compiler 
// and the source is then harder to find
#ifdef __GNUC__
protected:
#else
private:
#endif
	noncopyable(const noncopyable&);
	const noncopyable& operator= (const noncopyable&);
protected:
	noncopyable() {}
public:
	~noncopyable() {}
};


//////////////////////////////////////////////////////////////////////////
/// base class to prevent allocation
//////////////////////////////////////////////////////////////////////////
class nonallocable 
{	// prevent allocation
	void* operator new(size_t s);
	void* operator new[](size_t s);
#if defined(__GNUC__) && __GNUC__ < 3
// problems with gcc prior to v3
protected:
#endif//defined(__GNUC__) && __GNUC__ < 3
	void operator delete(void* p);
	void operator delete[](void* p);
protected:
	nonallocable() {}
public:
	~nonallocable() {}
};

///////////////////////////////////////////////////////////////////////////////
/// default compare operators using object pointers
//////////////////////////////////////////////////////////////////////////
class defaultcmp
{
protected:
	defaultcmp()	{}
public:
	bool operator==(const defaultcmp& a) const	{ return this==&a; }
	bool operator!=(const defaultcmp& a) const	{ return this!=&a; }
	bool operator>=(const defaultcmp& a) const	{ return this>=&a; }
	bool operator> (const defaultcmp& a) const	{ return this> &a; }
	bool operator<=(const defaultcmp& a) const	{ return this<=&a; }
	bool operator< (const defaultcmp& a) const	{ return this< &a; }
};


///////////////////////////////////////////////////////////////////////////////
/// reduced compare set
//////////////////////////////////////////////////////////////////////////
class reducedcmp
{
protected:
	reducedcmp()	{}
	virtual ~reducedcmp()	{}
public:
	virtual bool operator==(const defaultcmp& a) const=0;
	bool operator!=(const defaultcmp& a) const	{ return !this->operator==(a); }
	bool operator>=(const defaultcmp& a) const	{ return !this->operator< (a); }
	bool operator> (const defaultcmp& a) const	{ return !this->operator< (a) && !this->operator==(a); }
	bool operator<=(const defaultcmp& a) const	{ return  this->operator< (a) ||  this->operator==(a); }
	virtual bool operator< (const defaultcmp& a) const=0;
};



//////////////////////////////////////////////////////////////////////////
/// base class for clonable objects
//////////////////////////////////////////////////////////////////////////
class clonable
{
public:
	virtual ~clonable()	{}
	virtual clonable* clone(void) const = 0;
};



//////////////////////////////////////////////////////////////////////////
/// base class for objects that can be counted on global scope
//////////////////////////////////////////////////////////////////////////
class global 
{
private:
#ifdef COUNT_GLOBALS
	static int sGlobalCount;
#ifdef STORE_GLOBALS
	static void globaladd(global* ptr);
	static void globaldel(global* ptr);
#endif//STORE_GLOBALS
#ifdef DEBUG
	class _globalcount
	{
	public:
		_globalcount();
		~_globalcount();

		static void finalcheck();
	};
	static _globalcount gc;
	friend class _globalcount;
#endif//DEBUG
#endif//COUNT_GLOBALS

#ifdef COUNT_GLOBALS
protected:
	global()
	{
		atomicincrement(&sGlobalCount);
#ifdef STORE_GLOBALS
		globaladd(this);
#endif//STORE_GLOBALS
	}
public:
#ifdef STORE_GLOBALS
	virtual
#endif//STORE_GLOBALS
	~global()
	{
		atomicdecrement(&sGlobalCount);
#ifdef STORE_GLOBALS
		globaldel(this);
#endif//STORE_GLOBALS
#ifdef DEBUG
		_globalcount::finalcheck();
#endif//DEBUG
	}
	static int getcount()	{ return sGlobalCount; }
	static void debugprint();
#else//!COUNT_GLOBALS
protected:
	global()				{ }
public:
	~global()				{ }
	static int getcount()	{ return 0; }
	static void debugprint(){}
#endif//!COUNT_GLOBALS
};



///////////////////////////////////////////////////////////////////////////////
/// assign container.
/// used as return type for container subscript operators
template <typename T>
class assign_cont
{
protected:
	T& cObj;
public:
	explicit assign_cont<T>(T& o) : cObj(o)						{}
	assign_cont<T>(const assign_cont<T>& ac) : cObj(ac.cObj)	{}
	virtual ~assign_cont()	{}

	const assign_cont<T>& operator=(const assign_cont<T>& ac)
	{
		this->assign(ac.cObj);
		return *this;
	}
	const assign_cont<T>& operator=(const T& o)
	{
		this->assign(o);
		return *this;
	}
	virtual const T& assign(const T& o)
	{
		this->cObj = o;
		return this->cObj;
	}
	operator const T&() const	{ return this->cObj; }
};

///////////////////////////////////////////////////////////////////////////////
/// assign container. boolean specialisation
/// assuming that these are implemented as bitmasks on ulongs
template <>
class assign_cont<bool>
{
	ulong&	cObj;
	ulong	cBit;
public:
	explicit assign_cont<bool>(ulong& o, uchar b) : cObj(o), cBit(1ul<<b)			{}
	assign_cont<bool>(const assign_cont<bool>& ac) : cObj(ac.cObj), cBit(ac.cBit)	{}
	virtual ~assign_cont<bool>()	{}

	bool operator=(const assign_cont<bool>& ac)
	{
		return this->assign( ac );
	}
	bool operator=(const bool o)
	{
		return this->assign(o);
	}
	virtual bool assign(const bool o)
	{
		if(o)
			this->cObj |=  this->cBit;
		else
			this->cObj &= ~this->cBit;
		return 0;
	}
	const assign_cont<bool>& operator |=(bool o)
	{
		if(o)
			this->cObj |= this->cBit;
		return *this;
	}
	const assign_cont<bool>& operator &=(bool o)
	{
		if(!o)
			this->cObj &= ~this->cBit;
		return *this;
	}
	const assign_cont<bool>& operator ^=(bool o)
	{
		if(o)
			this->cObj ^= this->cBit;
		return *this;
	}
	operator bool() const	{ return (0!=(this->cObj&this->cBit)); }
};

inline void swap(assign_cont<bool>& x, assign_cont<bool>& y)
{
	bool tmp =(bool)x;
	x = y;
	y = tmp;
}



//////////////////////////////////////////////////////////////////////////
/// 64bit and user save pointer/number sharing structure.
/// use with care
typedef struct _numptr
{
	bool	isptr;
	union{
		void *	ptr;
		ssize_t	num;
	};
	_numptr():isptr(false),ptr(NULL)				{}	// clear as default
	explicit _numptr(void*a):isptr(a!=NULL),ptr(a)	{}	// take over the void pointer, block const pointers here to signal the user
	_numptr(ssize_t a):isptr(false),num(a)			{}	// initialisation from numbers !!DONT CAST POINTERS TO INTS!!

	ssize_t	integer() const
	{
		return (this->isptr)?0:this->num;
	}
	ssize_t& integer()
	{
		if( this->isptr )
			this->num = 0;
		return this->num;
	}
	void* pointer() const
	{
		return (this->isptr)?this->ptr:NULL;
	}
	void *& pointer()
	{
		if( !this->isptr )
			this->ptr = NULL;
		return this->ptr;
	}
} numptr;



///////////////////////////////////////////////////////////////////////////////
/// modifies a value on a scope change. (actually on object destruction)
/// removes the necessity of writing bunches of value settings before returns
/// on spagetti code like frequently found especially here
///////////////////////////////////////////////////////////////////////////////
template <typename T>
class TScopeChange
{
	T& val;
	T tar;
public:
	TScopeChange(T& v, const T&t) : val(v), tar(t)	{}
	~TScopeChange()			{ val=tar; }
	void disable()			{ tar = val; }
	void set(const T& t)	{ tar = t; }
};



///////////////////////////////////////////////////////////////////////////////
/// Singleton Templates
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Simple Singleton for default creatable objects only
///////////////////////////////////////////////////////////////////////////////
template <typename T>
class TSimpleSingleton
{
public:
	TSimpleSingleton()		{ }
	~TSimpleSingleton()		{ }

	static T& aquire()
	{
		static T singleton;
		return singleton;
	}
	operator T&()	{ return TSimpleSingleton::aquire(); }
};

///////////////////////////////////////////////////////////////////////////////
/// Singleton with templated create functions
///////////////////////////////////////////////////////////////////////////////
template <typename T>
class TSingleton
{
	static T*& getpointer()
	{
		static T*singleton_ptr=NULL;
		return singleton_ptr;
	}
public:
	TSingleton()						
	{
		if( NULL==this->getpointer() )
			this->create();
	}
	TSingleton(const TSingleton&)
	{ }
	template <typename P1>
	TSingleton(P1& p1)
	{
		this->create(p1);
	}
	template <typename P1, typename P2>
	TSingleton(P1& p1, P2& p2)
	{
		this->create(p1,p2);
	}
	template <typename P1, typename P2, typename P3>
	TSingleton(P1& p1, P2& p2, P3& p3)
	{
		this->create(p1,p2,p3);
	}
	template <typename P1, typename P2, typename P3, typename P4>
	TSingleton(P1& p1, P2& p2, P3& p3, P4& p4)
	{
		this->create(p1,p2,p3,p4);
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	TSingleton(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5)
	{
		this->create(p1,p2,p3,p4,p5);
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	TSingleton(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6)
	{
		this->create(p1,p2,p3,p4,p5,p6);
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	TSingleton(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6, P7& p7)
	{
		this->create(p1,p2,p3,p4,p5,p6,p7);
	}

	~TSingleton()		{ }
	static T& aquire()
	{
		return *(TSingleton::getpointer());
	}
	operator T&()	{ return this->aquire(); }

	bool isvalid()			{ return NULL!=this->getpointer(); }
	bool destroy()			
	{
		T*& ptr= this->getpointer();
		if( ptr )
		{
			delete ptr;
			ptr=NULL;
			return true;
		}
		return false;
	}
	bool create()
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T;
			return true;
		}
		return false;
	}
	template <typename P1>
	bool create(P1& p1)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2>
	bool create(P1& p1, P2& p2)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2, typename P3>
	bool create(P1& p1, P2& p2, P3& p3)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2,p3);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2, typename P3, typename P4>
	bool create(P1& p1, P2& p2, P3& p3, P4& p4)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2,p3,p4);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	bool create(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2,p3,p4,p5);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	bool create(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2,p3,p4,p5,p6);
			return true;
		}
		return false;
	}
	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	bool create(P1& p1, P2& p2, P3& p3, P4& p4, P5& p5, P6& p6, P7& p7)
	{
		T*& ptr= this->getpointer();
		if( !ptr )
		{
			ptr = new T(p1,p2,p3,p4,p5,p6,p7);
			return true;
		}
		return false;
	}
};





///////////////////////////////////////////////////////////////////////////////
/// delete notification.
template<typename T>
struct TDeleteNotify
{
private:
	bool valid;
	T value;

	void throw_when_invalid()
	{
#if defined(DEBUG)
		if( !valid ) 
		{
#if defined(CHECK_EXCEPTIONS)
			throw "access on destroyed object";
#else
			sprintf(stderr, "access on destroyed object");
#endif
		}
#endif
	}
public:
	TDeleteNotify() : valid(true), value()
	{}
	~TDeleteNotify()
	{
		valid=false;
	}

	TDeleteNotify(const TDeleteNotify& d) : valid(true), value(d.v)
	{}
	const T& operator=(const TDeleteNotify& d)
	{
		throw_when_invalid();
		value = d;
		return value;
	}

	TDeleteNotify(const T& v) : valid(true), value(v)
	{}
	const T& operator=(const T& v)
	{
		throw_when_invalid();
		value = v;
		return value;
	}

	operator const T&() const	{ throw_when_invalid(); return value; }
	operator T&()				{ throw_when_invalid(); return value; }

	const T& operator()() const	{ throw_when_invalid(); return value; }
	T& operator()()				{ throw_when_invalid(); return value; }

	bool is_valid() const		{ return valid; }
};


NAMESPACE_END(basics)

#endif//__BASEOBJECTS_H__

