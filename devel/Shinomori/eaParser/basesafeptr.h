#ifndef SAFEcPtrH_
#define SAFEcPtrH_



unsigned int atomicincrement( unsigned int* pcount )
{
	if(pcount)
	{
		*pcount++;
		return *pcount;
	}
	return 0;
}
unsigned int atomicdecrement( unsigned int* pcount )
{
	if(pcount)
	{
		*pcount--;
		return *pcount;
	}
	return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////
// basic pointer interface
/////////////////////////////////////////////////////////////////////////////////////////
template <class X> class TPtr : public noncopyable
{
protected:
	TPtr()	{}
public:
	virtual ~TPtr()	{}
	virtual const X* get() const = 0;

	virtual const X& readaccess() const = 0;
	virtual X& writeaccess() = 0;

	virtual bool operator ==(const TPtr& p)	const	{return false;}
	virtual bool operator !=(const TPtr& p)	const	{return true;}
	virtual bool operator ==(void *p) const = 0;
	virtual bool operator !=(void *p) const = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////
// Simple Auto/Copy-Pointer
// it creates a default object on first access if not initializes
// automatic free on object delete
// does not use a counting object
/////////////////////////////////////////////////////////////////////////////////////////
template <class X> class TPtrAuto : public TPtr<X>
{
private:
	X* itsPtr;
	void create()					{ if(!itsPtr) itsPtr = new X; }
	void copy(const TPtr<X>& p)
	{
		if(itsPtr)	delete itsPtr;
		X* pp=p.get();
		if( pp )
			itsPtr = new X(*pp);
	}
public:
	explicit TPtrAuto(X* p = NULL) throw() : itsPtr(p)	{}
	virtual ~TPtrAuto()				{if(itsPtr) delete itsPtr;}

	TPtrAuto(const TPtr<X>& p) throw() : itsPtr(NULL)	{ copy(p); }
	const TPtr& operator=(const TPtr<X>& p)				{ copy(p); return *this; }

	virtual const X& readaccess() const			{const_cast<TPtrAuto<X>*>(this)->create(); return *itsPtr;}
	virtual X& writeaccess()					{const_cast<TPtrAuto<X>*>(this)->create(); return *itsPtr;}
	virtual const X* get() const				{const_cast<TPtrAuto<X>*>(this)->create(); return itsPtr;}
	virtual const X& operator*() const throw()	{const_cast<TPtrAuto<X>*>(this)->create(); return *itsPtr;}
	virtual const X* operator->() const throw() {const_cast<TPtrAuto<X>*>(this)->create(); return itsPtr;}
	X& operator*()	throw()						{const_cast<TPtrAuto<X>*>(this)->create(); return *itsPtr;}
	X* operator->()	throw()						{const_cast<TPtrAuto<X>*>(this)->create(); return itsPtr;}
	operator const X&() const throw()			{const_cast<TPtrAuto<X>*>(this)->create(); return *itsPtr;}


	virtual bool operator ==(void *p) const { return itsPtr==p; }
	virtual bool operator !=(void *p) const { return itsPtr!=p; }
};


/////////////////////////////////////////////////////////////////////////////////////////
// Count-Pointer
// pointer copies are counted
// when reference counter becomes zero the object is automatically deleted
// if there is no pointer, it will return NULL
/////////////////////////////////////////////////////////////////////////////////////////
template <class X> class TPtrCount : public TPtr<X>
{
protected:
	class cCounter
	{
	public:
		unsigned int	count;
		X*				ptr;

		cCounter(X* p, unsigned c = 1) : ptr(p), count(c) {}
		~cCounter()	{ if(ptr) delete ptr; }

		const cCounter& operator=(X* p)
		{	// take ownership of the given pointer
			if(ptr) delete ptr;
			ptr = p;
			return *this;
		}
		cCounter* aquire()
		{
			atomicincrement( &count );
			return this;
		}
		cCounter* release()
		{
			if( atomicdecrement( &count ) == 0 )
			{
				delete this;
				return NULL;
			}
			return this;
		}
	}* itsCounter;

	virtual void acquire(const TPtrCount& r) throw()
	{	// check if not already pointing to the same object
		if( this->itsCounter != r.itsCounter ||  NULL==itsCounter )
		{	// release this thing
			if(itsCounter) itsCounter->release();
			// aquite and increment the given pointer
			if( r.itsCounter )
			{
				itsCounter = r.itsCounter;
				itsCounter->aquire();
			}
			else
			{	// new empty counter to link the pointers
				itsCounter = new cCounter(NULL);
				const_cast<TPtrCount&>(r).itsCounter = itsCounter;
				itsCounter->aquire();
			}
		}
	}
	void release()
	{	// decrement the count,
		if(itsCounter) itsCounter = itsCounter->release();
	}


public:
	TPtrCount() : itsCounter(NULL)
	{}
	explicit TPtrCount(X* p) : itsCounter(NULL)
	{	// allocate a new counter
		if (p) itsCounter = new cCounter(p);
	}
	TPtrCount(const TPtrCount& r) : itsCounter(NULL)
	{
		acquire(r);
	}
	virtual ~TPtrCount()	
	{
		release();
	}
	const TPtrCount& operator=(const TPtrCount& r)
	{
		acquire(r);
		return *this;
	}
	TPtrCount& operator=(X* p)
	{	// take ownership of the given pointer
		if( itsCounter )
		{
			*itsCounter = p;
		}
		else
		{
			itsCounter = new cCounter(p);
		}
		return *this;
	}

	const size_t getRefCount() const { return (itsCounter) ? itsCounter->count : 1;}

	bool isunique()	const throw() {return (itsCounter ? (itsCounter->count == 1):true);}
	bool make_unique()	  throw()
	{
		if( !isunique() )
		{
			cCounter *cnt = new cCounter(NULL);
			// copy the object if exist
			if(itsCounter->ptr)
				cnt->ptr = new X(*(itsCounter->ptr));
			release();
			itsCounter = cnt;
		}
		return true;
	}
	virtual bool operator ==(const TPtrCount& p) const { return itsCounter == p.itsCounter; }
	virtual bool operator !=(const TPtrCount& p) const { return itsCounter != p.itsCounter; }
	virtual bool operator ==(void *p) const { return (itsCounter) ? (itsCounter->ptr==p) : (itsCounter==p); }
	virtual bool operator !=(void *p) const { return (itsCounter) ? (itsCounter->ptr!=p) : (itsCounter!=p); }

//	operator bool() const	{ return  itsCounter ?  NULL!=itsCounter->ptr : false; }

	virtual const X& readaccess() const			{ return *itsCounter->ptr; }
	virtual X& writeaccess()					{ return *itsCounter->ptr; }

	virtual const X* get() const				{ return  itsCounter ?  itsCounter->ptr : 0; }
	virtual const X& operator*() const throw()	{ return *itsCounter->ptr; }
	virtual const X* operator->() const throw()	{ return  itsCounter ?  itsCounter->ptr : 0; }
	virtual X& operator*() throw()				{ return *itsCounter->ptr; }
	virtual X* operator->() throw()				{ return  itsCounter ?  itsCounter->ptr : 0; }
	virtual operator const X&() const throw()	{ return *itsCounter->ptr; }
};

/////////////////////////////////////////////////////////////////////////////////////////
// Count-Pointer
// pointer copies are counted
// when reference counter becomes zero the object is automatically deleted
// creates a default object if not exist
/////////////////////////////////////////////////////////////////////////////////////////
template <class X> class TPtrAutoCount : public TPtrCount<X>
{
	void create()
	{	// check if we have an object to access, create one if not
		if(!itsCounter)			itsCounter		= new cCounter(NULL);
		// usable objects need a default constructor
		if(!itsCounter->ptr)	itsCounter->ptr	= new X; 
	}

public:
	explicit TPtrAutoCount(X* p = NULL) : TPtrCount<X>(p)
	{}
	TPtrAutoCount(const TPtrCount<X>& r) : TPtrCount<X>(r)
	{}
	virtual ~TPtrAutoCount()	
	{}
	const TPtrAutoCount<X>& operator=(const TPtrCount<X>& r)
	{
		acquire(r);
		return *this;
	}
	virtual const X& readaccess() const			{ const_cast<TPtrAutoCount<X>*>(this)->create(); return *itsCounter->ptr; }
	virtual X& writeaccess()					{ const_cast<TPtrAutoCount<X>*>(this)->create(); return *itsCounter->ptr; }
	virtual const X* get() const				{ const_cast<TPtrAutoCount<X>*>(this)->create(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual const X& operator*() const throw()	{ const_cast<TPtrAutoCount<X>*>(this)->create(); return *itsCounter->ptr; }
	virtual const X* operator->() const throw()	{ const_cast<TPtrAutoCount<X>*>(this)->create(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual X& operator*() throw()				{ const_cast<TPtrAutoCount<X>*>(this)->create(); return *itsCounter->ptr; }
	virtual X* operator->() throw()				{ const_cast<TPtrAutoCount<X>*>(this)->create(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual operator const X&() const throw()	{ const_cast<TPtrAutoCount<X>*>(this)->create(); return *itsCounter->ptr; }
};

/////////////////////////////////////////////////////////////////////////////////////////
// Count-Auto-Pointer with copy-on-write scheme
// pointer copies are counted
// when reference counter becomes zero the object is automatically deleted
// creates a default object if not exist
/////////////////////////////////////////////////////////////////////////////////////////
template <class X> class TPtrAutoRef : public TPtrCount<X>
{
	void create()
	{	// check if we have an object to access, create one if not
		if(!itsCounter)			itsCounter		= new cCounter(NULL);
		// usable objects need a default constructor
		if(!itsCounter->ptr)	itsCounter->ptr	= new X; 
	}
public:
	explicit TPtrAutoRef(X* p = NULL) : TPtrCount<X>(p)
	{}
	TPtrAutoRef(const TPtrCount<X>& r) : TPtrCount<X>(r)
	{}
	virtual ~TPtrAutoRef()	
	{}
	const TPtrAutoRef& operator=(const TPtrCount<X>& r)
	{
		acquire(r);
		return *this;
	}

	virtual const X& readaccess() const	
	{ 
		const_cast< TPtrAutoRef* >(this)->create();	
		// no need to aquire, is done on reference creation
		return *itsCounter->ptr;
	}
	virtual X& writeaccess()
	{
		(this)->create();
		make_unique();
		// no need to aquire, is done on reference creation
		return *itsCounter->ptr;
	}
	virtual const X* get() const					{ readaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual const X& operator*()	const throw()	{ return readaccess(); }
	virtual const X* operator->()	const throw()	{ readaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual X& operator*()	throw()					{ return writeaccess();}
	virtual X* operator->()	throw()					{ writeaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual operator const X&() const throw()		{ return readaccess(); }
};

/////////////////////////////////////////////////////////////////////////////////////////
// Pointer with loadable scheme
/////////////////////////////////////////////////////////////////////////////////////////
enum POINTER_TYPE
{
	AUTOCOUNT,	// counting and create-on-access
	AUTOREF		// counting, create-on-access and copy-on-write (default)
};

template <class X> class TPtrCommon : public TPtrCount<X>
{
	bool cAutoRef : 1;
	void create()
	{	// check if we have an object to access, create one if not
		if(!itsCounter)			itsCounter		= new cCounter(NULL);
		// usable objects need a default constructor
		if(!itsCounter->ptr)	itsCounter->ptr	= new X; 
	}
public:

	explicit TPtrCommon(X* p = NULL) : TPtrCount<X>(p), cAutoRef(true)
	{}
	TPtrCommon(const TPtrCount<X>& r) : TPtrCount<X>(r), cAutoRef(true)
	{}
	virtual ~TPtrCommon()	
	{}
	const TPtrCommon& operator=(const TPtrCount<X>& r)
	{
		acquire(r);
		return *this;
	}

	void setaccess(POINTER_TYPE pt)	
	{ 
		cAutoRef = (pt == AUTOREF);
	}

	virtual const X& readaccess() const
	{ 
		const_cast< TPtrCommon* >(this)->create();	
		// no need to aquire, is done on reference creation
		return *itsCounter->ptr;
	}
	virtual X& writeaccess()
	{
		(this)->create();
		if(cAutoRef) make_unique();
		// no need to aquire, is done on reference creation
		return *itsCounter->ptr;
	}
	virtual const X* get() const					{ readaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual const X& operator*()	const throw()	{ return readaccess(); }
	virtual const X* operator->()	const throw()	{ readaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual X& operator*()	throw()					{ return writeaccess();}
	virtual X* operator->()	throw()					{ writeaccess(); return itsCounter ? itsCounter->ptr : NULL; }
	virtual operator const X&() const throw()		{ return readaccess(); }
};




/*
/////////////////////////////////////////////////////////////////////
// reference counting pointer template
// counts references on the used data
// automatic cleanup when the last data is destroyed
// implements a copy-on-write scheme
/////////////////////////////////////////////////////////////////////
template<class T> class TPtrRef
{
	/////////////////////////////////////////////////////////////////
	// the reference manages refernce counting and automatic cleanup
	// the user data is sored as an member element
	/////////////////////////////////////////////////////////////////
	class CRefCnt
	{
		friend class TPtrRef<T>;
		/////////////////////////////////////////////////////////////
		// class data:
		/////////////////////////////////////////////////////////////
		T		cData;
		size_t	cRefCnt;
		#ifndef SINGLETHREAD
		rwlock	cRWlock;
		#endif
public:
		/////////////////////////////////////////////////////////////
		// Constructors
		// aquire the reference automatically on creation
		// a destructor is not necessary can use defaults
		/////////////////////////////////////////////////////////////
		CRefCnt():cRefCnt(0)					{aquire();}
		CRefCnt(const T& t):cData(t),cRefCnt(0)	{aquire();}
		/////////////////////////////////////////////////////////////
		// aquire a reference 
		// and increment the reference counter
		/////////////////////////////////////////////////////////////
		void aquire()
		{
			atomicincrement(&cRefCnt);
		}
		/////////////////////////////////////////////////////////////
		// release a reference 
		// delete when the last reference is gone
		/////////////////////////////////////////////////////////////
		void release()
		{
			if( 0==atomicdecrement(&cRefCnt) )
				delete this;
		}
		/////////////////////////////////////////////////////////////
		// reference counter access and check funtion
		/////////////////////////////////////////////////////////////
		bool isShared()	const {return (cRefCnt>1);}
		const size_t getRefCount() const {return cRefCnt;}
		/////////////////////////////////////////////////////////////
		// read/write lock for syncronisation on multithread environment
		/////////////////////////////////////////////////////////////
		void ReadLock() 
		{
			#ifndef SINGLETHREAD
			cRWlock.rdlock(); 
			#endif
		}
		void WriteLock()
		{
			#ifndef SINGLETHREAD
			cRWlock.wrlock(); 
			#endif
		}
		void UnLock()	
		{
			#ifndef SINGLETHREAD
			cRWlock.unlock(); 
			#endif
		}
	};
	/////////////////////////////////////////////////////////////////

	CRefCnt	*cRef;
public:
	/////////////////////////////////////////////////////////////////
	// constructors and destructor
	// 
	/////////////////////////////////////////////////////////////////
	TPtrRef() : cRef(NULL)
	{}
	TPtrRef(TPtrRef& ptr) : cRef(ptr.cRef)
	{
		if (cRef != 0) {
			cRef->aquire();
		}
	}
	TPtrRef(const T& obj) : cRef( new CRefCnt(obj) )
	{	// no need to aquire, is done on reference creation
	}
	~TPtrRef()
	{
		if (cRef) cRef->release();
	}
	/////////////////////////////////////////////////////////////////
	// assignment operator
	// release an existing Reference then copy and aquire the given
	/////////////////////////////////////////////////////////////////
	TPtrRef& operator=(const TPtrRef& prt)
	{
		if (cRef != prt.cRef) {
			if (cRef) cRef->release(); // let go the old ref
			cRef = prt.cRef;
			if (cRef) cRef->aquire(); // and aquire the new one
		}
		
		// !!todo!! check if necessary
	//	if(cRef == NULL)  // prt was empty
	//	{	// so we create a new reference
	//		cRef = new CRefCnt;
	//		// do not aquire, the reference creation is already aquiring one time
	//		// and also put it to the calling ptr (cast needed for writing)
	//		((CRefCnt*)prt.cRef) = cRef;
	//		cRef->aquire(); 
	//	}
		return *this;
	}
	/////////////////////////////////////////////////////////////////
	// just for inspection and test purpose
	/////////////////////////////////////////////////////////////////
	const size_t getRefCount() const { return (cRef) ? cRef->getRefCount() : 1;}
	/////////////////////////////////////////////////////////////////
	// compare for 2 pointers if the share an identical reference
	/////////////////////////////////////////////////////////////////
	bool operator ==(const TPtrRef& ref) const 
	{
		return (cRef == ref.cRef);
	}
	bool operator !=(const TPtrRef& ref) const 
	{
		return (cRef != ref.cRef);
	}

	/////////////////////////////////////////////////////////////////
	// returns the data element for read access
	// can be used to write the sharing data
	/////////////////////////////////////////////////////////////////
	void Check()
	{
		if(!cRef) cRef = new CRefCnt;
	}
	T& ReadAccess() const
	{ 
		// need to cast pointers to allow the creation of an empty element if not exist
		const_cast< TPtrRef<T>* >(this)->Check();	
		// no need to aquire, is done on reference creation
		return cRef->cData;
	}
	/////////////////////////////////////////////////////////////////
	// write access function
	// copy the data before returning the element
	/////////////////////////////////////////////////////////////////
	T& WriteAccess()
	{
		if(!cRef) 
			cRef = new CRefCnt;
		else if( cRef->isShared() )
		{	cRef->release();
			cRef = new CRefCnt(cRef->cData);
		}
		// no need to aquire, is done on reference creation
		return cRef->cData;
	}
	/////////////////////////////////////////////////////////////////
	// automatic read/write access function pairs
	// when access is const the compiler choose read access
	// when access is not const it will be write access
	// proper function declarations necessary to identify
	// const and non const access to the data
	/////////////////////////////////////////////////////////////////
	const T* operator->() const
	{	
		return &(this->ReadAccess());
	}
	/////////////////////////////////////////////////////////////////
	T* operator->()
	{
		return &(this->WriteAccess());
	}
	/////////////////////////////////////////////////////////////////
	const T& operator*() const
	{	
		return (this->ReadAccess());
	}
	/////////////////////////////////////////////////////////////////
	T& operator*()
	{
		return (this->WriteAccess());
	}
	/////////////////////////////////////////////////////////////////
	// allow direct access to the data
	// to use for write access to the global onject
	/////////////////////////////////////////////////////////////////
	T* global()
	{
		if(!cRef) 
			cRef = new CRefCnt; // no need to aquire, is done on reference creation			
		return cRef->cData;
	}

	/////////////////////////////////////////////////////////////////
	// read/write lock for syncronisation on multithread environment
	/////////////////////////////////////////////////////////////////
	void ReadLock()	{if(cRef) cRef->ReadLock(); }
	void WriteLock(){if(cRef) cRef->WriteLock(); }
	void UnLock()	{if(cRef) cRef->UnLock(); }
};
*/
/////////////////////////////////////////////////////////////////////




/*
// strong/weak pointer maybe not that usefull
// but I'm still hesitating to remove them
/////////////////////////////////////////////////////////////////////////////////////////
//
// handy helpers
//
/////////////////////////////////////////////////////////////////////////////////////////

struct RefCounts {
	RefCounts () : totalRefs_ (1), strongRefs_ (1) 
	{}

	size_t totalRefs_;
	size_t strongRefs_;
};

enum Null {
	null = 0
};



/////////////////////////////////////////////////////////////////////////////////////////
// Strong and Weak reference pointers
// strong pointers count the reference 
// weak pointers only using the reference to decide if an object exist or not
/////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// forward definitions

template <class T> class TPtrWeak;
template <class T> class TPtrStrong;

///////////////////////////////////////////////////////
// TPtrStrong
template <class T>
class TPtrStrong {
public:
	TPtrStrong () : cPtr (0), cRefCount (0) {
	}

	TPtrStrong (const TPtrStrong<T>& other) {
		cPtr = other.cPtr;
		cRefCount = other.cRefCount;

		if (cPtr != 0) {
			atomicincrement (cRefCount->strongRefs_);
			atomicincrement (cRefCount->totalRefs_);
		}
	}

	template <class O>
	TPtrStrong (const TPtrWeak<O>& other) {
		cPtr = other.cPtr;
		cRefCount = other.cRefCount;

		if (cPtr != 0) {
			atomicincrement (cRefCount->strongRefs_);
			atomicincrement (cRefCount->totalRefs_);
		}
	}

	TPtrStrong& operator = (const TPtrStrong<T>& other) {
		if (cPtr != other.cPtr) {
			releaseRef ();

			cPtr = other.cPtr;
			cRefCount = other.cRefCount;

			if (cPtr != 0) {
				atomicincrement (cRefCount->strongRefs_);
				atomicincrement (cRefCount->totalRefs_);
			}
		}

		return *this;
	}

	TPtrStrong& operator = (const TPtrWeak<T>& other) {
		if (cPtr != other.cPtr) {
			releaseRef ();

			cPtr = other.cPtr;
			cRefCount = other.cRefCount;

			if (cPtr != 0) {
				atomicincrement (cRefCount->strongRefs_);
				atomicincrement (cRefCount->totalRefs_);
			}
		}

		return *this;
	}

	TPtrStrong& operator = (Null) {
		releaseRef ();

		return *this;
	}

	~TPtrStrong () {
		releaseRef ();
	}

	T& operator * () const {
		assert (cPtr != 0);

		return *cPtr;
	}

	T* operator -> () const {
		assert (cPtr != 0);

		return cPtr;
	}

	void swap (TPtrStrong<T>& other) {
		std::swap (cPtr, other.cPtr);
		std::swap (cRefCount, other.cRefCount);
	}

private:
	TPtrStrong (T* ptr, RefCounts* refCounts) : cPtr (ptr), cRefCount (refCounts) {
	}

	void releaseRef () {
		if (cPtr == 0) {
			return;
		}

		{
			bool release = atomicdecrement (cRefCount->strongRefs_);
			if (release) {
				delete cPtr;
			}

			cPtr = 0;
		}

		{
			bool release = atomicdecrement (cRefCount->totalRefs_);
			if (release) {
				delete cRefCount;
			}

			cRefCount = 0;
		}
	}

	T* cPtr;
	RefCounts* cRefCount;

	template <class T>
	friend TPtrStrong<T> create ();
	
	template <class T, class P1>
	friend TPtrStrong<T> create (P1 p1);

	template <class T, class P1, class P2>
	friend TPtrStrong<T> create (P1 p1, P2 p2);

	template <class T, class P1, class P2, class P3>
	friend TPtrStrong<T> create (P1 p1, P2 p2, P3 p3);

	template <class T, class P1, class P2, class P3, class P4>
	friend TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4);

	template <class T, class P1, class P2, class P3, class P4, class P5>
	friend TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5);

	template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
	friend TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6);

	template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
	friend TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7);

	template <class T>
	friend TPtrStrong<T> wrapPtr (T* t);

	friend class TPtrStrong;
	friend class TPtrWeak;

	template<class T, class U>
	friend bool operator == (const TPtrStrong<T>& a, const TPtrStrong<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrStrong<T>& a, const TPtrStrong<U>& b);

	template<class T>
	friend bool operator == (const TPtrStrong<T>& a, Null);

	template<class T>
	friend bool operator == (Null, const TPtrStrong<T>& a);

	template<class T>
	friend bool operator != (const TPtrStrong<T>& a, Null);

	template<class T>
	friend bool operator != (Null, const TPtrStrong<T>& a);

	template<class T, class U>
	friend bool operator == (const TPtrStrong<T>& a, const TPtrWeak<U>& b);

	template<class T, class U>
	friend bool operator == (const TPtrWeak<T>& a, const TPtrStrong<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrStrong<T>& a, const TPtrWeak<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrWeak<T>& a, const TPtrStrong<U>& b);

	template <class T, class F>
	friend TPtrStrong<T> staticCast (const TPtrStrong<F>& from);

	template <class T, class F>
	friend TPtrStrong<T> constCast (const TPtrStrong<F>& from);

	template <class T, class F>
	friend TPtrStrong<T> dynamicCast (const TPtrStrong<F>& from);

	template <class T, class F>
	friend TPtrStrong<T> checkedCast (const TPtrStrong<F>& from);

	template <class T, class F>
	friend TPtrStrong<T> queryCast (const TPtrStrong<F>& from);
};

///////////////////////////////////////////////////////
// TPtrWeak

template <class T>
class TPtrWeak {
public:
	TPtrWeak () : cPtr (0), cRefCount (0) {
	}

	TPtrWeak (const TPtrWeak<T>& other) {
		cPtr = other.cPtr;
		cRefCount = other.cRefCount;

		if (cPtr != 0) {
			atomicincrement (cRefCount->totalRefs_);
		}
	}

	template <class T>
	TPtrWeak (const TPtrStrong<T>& other) {
		cPtr = other.cPtr;
		cRefCount = other.cRefCount;

		if (cPtr != 0) {
			atomicincrement (cRefCount->totalRefs_);
		}
	}

	TPtrWeak& operator = (const TPtrWeak<T>& other) {
		if (get () != other.get ()) {
			releaseRef ();

			cPtr = other.cPtr;
			cRefCount = other.cRefCount;

			if (cPtr != 0) {
				atomicincrement (cRefCount->totalRefs_);
			}
		}

		return *this;
	}

	TPtrWeak& operator = (Null) {
		releaseRef ();

		return *this;
	}


	template <class T>
	TPtrWeak& operator = (const TPtrStrong<T>& other) {
		if (get () != other.cPtr) {
			releaseRef ();

			cPtr = other.cPtr;
			cRefCount = other.cRefCount;

			if (cPtr != 0) {
				atomicincrement (cRefCount->totalRefs_);
			}
		}

		return *this;
	}

	~TPtrWeak () {
		releaseRef ();
	}

	T& operator * () const {
		assert ( (0!=cRefCount->strongRefs_) && (0!=cPtr) );

		return *get ();
	}

	T* operator -> () const {
		assert ( (0!=cRefCount->strongRefs_) && (0!=cPtr) );

		return get ();
	}

	void swap (TPtrWeak<T>& other) {
		std::swap (cPtr, other.cPtr);
		std::swap (cRefCount, other.cRefCount);
	}

private:
	TPtrWeak (T* ptr, RefCounts* refCounts) : cPtr (ptr), cRefCount (refCounts) {
	}

	T* get () const {
		if( (cRefCount == 0) || (0==cRefCount->strongRefs_)) {
			return 0;
		}

		return cPtr;
	}

	bool isNull () const {
		return get () == 0;
	}

	void releaseRef () {
		if (cPtr == 0) {
			return;
		}

		bool release = atomicdecrement (cRefCount->totalRefs_);
		if (release) {
			delete cRefCount;
		}

		cRefCount = 0;
		cPtr = 0;
	}

	T* cPtr;
	RefCounts* cRefCount;

	friend class TPtrWeak;
	friend class SmartPtr;

	template<class T, class U>
	friend bool operator == (const TPtrWeak<T>& a, const TPtrWeak<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrWeak<T>& a, const TPtrWeak<U>& b);

	template<class T>
	friend bool operator == (const TPtrWeak<T>& a, Null);

	template<class T>
	friend bool operator == (Null, const TPtrWeak<T>& a);

	template<class T>
	friend bool operator != (const TPtrWeak<T>& a, Null);

	template<class T>
	friend bool operator != (Null, const TPtrWeak<T>& a);

	template<class T, class U>
	friend bool operator == (const TPtrStrong<T>& a, const TPtrWeak<U>& b);

	template<class T, class U>
	friend bool operator == (const TPtrWeak<T>& a, const TPtrStrong<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrStrong<T>& a, const TPtrWeak<U>& b);

	template<class T, class U>
	friend bool operator != (const TPtrWeak<T>& a, const TPtrStrong<U>& b);

	template <class T, class F>
	friend TPtrWeak<T> staticCast (const TPtrWeak<F>& from);

	template <class T, class F>
	friend TPtrWeak<T> constCast (const TPtrWeak<F>& from);

	template <class T, class F>
	friend TPtrWeak<T> dynamicCast (const TPtrWeak<F>& from);

	template <class T, class F>
	friend TPtrWeak<T> checkedCast (const TPtrWeak<F>& from);

	template <class T, class F>
	friend TPtrWeak<T> queryCast (const TPtrWeak<F>& from);
};

///////////////////////////////////////////////////////
// globals

template<class T, class U>
inline bool operator == (const TPtrStrong<T>& a, const TPtrStrong<U>& b) {
	return a.cPtr == b.cPtr;
}

template<class T, class U>
inline bool operator == (const TPtrWeak<T>& a, const TPtrWeak<U>& b) {
	return a.get () == b.get ();
}

template<class T, class U>
inline bool operator == (const TPtrStrong<T>& a, const TPtrWeak<U>& b) {
	return a.cPtr == b.get ();
}

template<class T, class U>
inline bool operator == (const TPtrWeak<T>& a, const TPtrStrong<U>& b) {
	return a.get () == b.cPtr;
}

template<class T, class U>
inline bool operator != (const TPtrStrong<T>& a, const TPtrStrong<U>& b) {
	return a.cPtr != b.cPtr;
}

template<class T, class U>
inline bool operator != (const TPtrWeak<T>& a, const TPtrWeak<U>& b) {
	return a.get () != b.get ();
}

template<class T, class U>
inline bool operator != (const TPtrStrong<T>& a, const TPtrWeak<U>& b) {
	return a.cPtr != b.get ();
}

template<class T, class U>
inline bool operator != (const TPtrWeak<T>& a, const TPtrStrong<U>& b) {
	return a.get () != b.cPtr;
}

template<class T>
inline bool operator == (const TPtrStrong<T>& a, Null) {
	return a.cPtr == 0;
}

template<class T>
inline bool operator == (Null, const TPtrStrong<T>& a) {
	return a.cPtr == 0;
}

template<class T>
inline bool operator != (const TPtrStrong<T>& a, Null) {
	return a.cPtr != 0;
}

template<class T>
inline bool operator != (Null, const TPtrStrong<T>& a) {
	return a.cPtr != 0;
}

template<class T>
inline bool operator == (const TPtrWeak<T>& a, Null) {
	return a.isNull ();
}

template<class T>
inline bool operator == (Null, const TPtrWeak<T>& a) {
	return a.isNull ();
}

template<class T>
inline bool operator != (const TPtrWeak<T>& a, Null) {
	return !a.isNull ();
}

template<class T>
inline bool operator != (Null, const TPtrWeak<T>& a) {
	return a.isNull ();
}

//////////////////////////////////////////////////////
// creation functions

template <class T>
TPtrStrong<T> create () {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T;
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1>
TPtrStrong<T> create (P1 p1) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2>
TPtrStrong<T> create (P1 p1, P2 p2) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2, class P3>
TPtrStrong<T> create (P1 p1, P2 p2, P3 p3) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2, p3);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2, class P3, class P4>
TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2, p3, p4);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2, class P3, class P4, class P5>
TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2, p3, p4, p5);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2, p3, p4, p5, p6);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
TPtrStrong<T> create (P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {
	RefCounts* rc = new RefCounts;

	try {
		T* t = new T (p1, p2, p3, p4, p5, p6, p7);
		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete rc;
		throw;
	}
}

template <class T>
TPtrStrong<T> wrapPtr (T* t) {
	if (t == 0) {
		return TPtrStrong<T> ();
	}

	try {
		RefCounts* rc = new RefCounts;

		return TPtrStrong<T> (t, rc);
	}
	catch (...) {
		delete t;
		throw;
	}
}

///////////////////////////////////////////////////////
// casts

template <class T, class F>
TPtrStrong<T> staticCast (const TPtrStrong<F>& from) {
	if (from.cPtr == 0) {
		return TPtrStrong<T>();
	}

	T* ptr = static_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->strongRefs_);
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrStrong<T> (ptr, refCounts);
}

template <class T, class F>
TPtrStrong<T> constCast (const TPtrStrong<F>& from) {
	if (from.cPtr == 0) {
		return TPtrStrong<T>();
	}

	T* ptr = const_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->strongRefs_);
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrStrong<T> (ptr, refCounts);
}

template <class T, class F>
TPtrStrong<T> dynamicCast (const TPtrStrong<F>& from) {
	if (from.cPtr == 0) {
		return TPtrStrong<T>();
	}

	T* ptr = &dynamic_cast <T&> (*from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->strongRefs_);
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrStrong<T> (ptr, refCounts);
}

template <class T, class F>
TPtrStrong<T> queryCast (const TPtrStrong<F>& from) {
	T* ptr = dynamic_cast <T*> (from.cPtr);

	if (ptr == 0) {
		return TPtrStrong<T>();
	}

	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->strongRefs_);
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrStrong<T> (ptr, refCounts);
}

template <class T, class F>
TPtrStrong<T> checkedCast (const TPtrStrong<F>& from) {
	if (from.cPtr == 0) {
		return TPtrStrong<T>();
	}

	assert (dynamic_cast<T*> (from.cPtr) != 0);

	T* ptr = static_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->strongRefs_);
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrStrong<T> (ptr, refCounts);
}

template <class T, class F>
TPtrWeak<T> staticCast (const TPtrWeak<F>& from) {
	if (from.get () == 0) {
		return TPtrWeak<T>();
	}

	T* ptr = static_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrWeak<T> (ptr, refCounts);
}

template <class T, class F>
TPtrWeak<T> constCast (const TPtrWeak<F>& from) {
	if (from.get () == 0) {
		return TPtrWeak<T>();
	}

	T* ptr = const_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrWeak<T> (ptr, refCounts);
}

template <class T, class F>
TPtrWeak<T> dynamicCast (const TPtrWeak<F>& from) {
	if (from.get () == 0) {
		return TPtrWeak<T>();
	}

	T* ptr = &dynamic_cast <T&> (*from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrWeak<T> (ptr, refCounts);
}

template <class T, class F>
TPtrWeak<T> queryCast (const TPtrWeak<F>& from) {
	T* ptr = dynamic_cast <T*> (from.get ());

	if (ptr == 0) {
		return TPtrWeak<T>();
	}

	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrWeak<T> (ptr, refCounts);
}

template <class T, class F>
TPtrWeak<T> checkedCast (const TPtrWeak<F>& from) {
	if (from.get () == 0) {
		return TPtrWeak<T>();
	}

	assert (dynamic_cast<T*> (from.cPtr) != 0);

	T* ptr = static_cast <T*> (from.cPtr);
	RefCounts* refCounts = from.cRefCount;

	if (ptr != 0) {
		atomicincrement (refCounts->totalRefs_);
	}

	return TPtrWeak<T> (ptr, refCounts);
}


template <class T> 
inline void swap (TPtrStrong<T>& t1, TPtrStrong<T>& t2) {
	t1.swap (t2);
}

template <class T> 
inline void swap (TPtrWeak<T>& t1, TPtrWeak<T>& t2) {
	t1.swap (t2);
}


*/

#endif // SAFEcPtrH