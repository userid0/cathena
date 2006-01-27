#ifndef __BASELISTS__
#define __BASELISTS__

#include "basetypes.h"
#include "baseobjects.h"
#include "baseexceptions.h"


///////////////////////////////////////////////////////////////////////////////
// tet function
void test_lists();


///////////////////////////////////////////////////////////////////////////////
// doubled linked list
///////////////////////////////////////////////////////////////////////////////
class CDLinkRoot;

class CDLinkNode : public global, public noncopyable
{
	friend class CDLinkRoot;

	CDLinkNode	*mpPrev;
	CDLinkNode	*mpNext;
public:
	///////////////////////////////////////////////////////////////////////////
	CDLinkNode() : mpPrev(NULL), mpNext(NULL)
	{}
	CDLinkNode(CDLinkNode& root) : mpPrev(&root), mpNext(root.mpNext)
	{	// double link with one anchor,
		// add in front of all others
		root.mpNext = this;
		if(this->mpNext) this->mpNext->mpPrev = this;
	}
	CDLinkNode(CDLinkNode& head, CDLinkNode& tail) : mpPrev(tail.mpPrev), mpNext(&tail)
	{	// double link with two anchors
		// add at the end of the existing list
		if(!head.mpNext)
		{
			this->mpPrev = &head;
			head.mpNext = tail.mpPrev=this;
		}
		else if(this->mpPrev)
		{
			tail.mpPrev = this->mpPrev->mpNext = this;
		}
	}
	virtual ~CDLinkNode()	{ unlink(); }
	///////////////////////////////////////////////////////////////////////////
	void link(CDLinkNode& root)
	{
		unlink();
		this->mpPrev = &root;
		this->mpNext = root.mpNext;
		root.mpNext = this;
		if(this->mpNext) this->mpNext->mpPrev = this;
	}
	void link(CDLinkNode& head, CDLinkNode& tail)
	{
		unlink();
		this->mpPrev = tail.mpPrev;
		this->mpNext = &tail;
		if(!head.mpNext)
		{	// first node insertion with neither head<->tail connected
			this->mpPrev = &head;
			head.mpNext = tail.mpPrev=this;
		}
		else if(this->mpPrev)
		{
			tail.mpPrev = this->mpPrev->mpNext = this;
		}
	}
	void unlink()
	{
		if(this->mpPrev != this->mpNext)
		{	// just normal node
			if(this->mpPrev) this->mpPrev->mpNext=this->mpNext;
			if(this->mpNext) this->mpNext->mpPrev=this->mpPrev;
		}
		else if(this->mpPrev)
		{	// last node on the root
			this->mpPrev->mpNext =NULL;
			this->mpPrev->mpPrev =NULL;
		}
		this->mpNext=NULL;
		this->mpPrev=NULL;
	}

	CDLinkNode* next()	{ return this->mpNext; }
	CDLinkNode* prev()	{ return this->mpPrev; }
};


///////////////////////////////////////////////////////////////////////////////
//!! TODO: add complete list interface
class CDLinkRoot
{
protected:
	CDLinkNode root;

	CDLinkRoot()	{}
public:
	~CDLinkRoot()	{}
protected:
	void insert(CDLinkNode& n)
	{
		n.link(root,root);
	}
	void remove(CDLinkNode& n)
	{
		n.unlink();
	}
	bool isEmpty()
	{
		return root.mpNext == root.mpPrev;
	}

	CDLinkNode& operator[](size_t inx)
	{
		CDLinkNode* first=root.mpNext;
		while( inx && first )
		{
			first = first->mpNext;
			inx--;
		}
		if( !first ||  first == &root )
			throw exception_bound("CDLinkNode out of bound");
		return *first;
	}
	const CDLinkNode& operator[](size_t inx) const
	{
		CDLinkNode* first=root.mpNext;
		while( inx && first )
		{
			first = first->mpNext;
			inx--;
		}
		if( !first ||  first == &root )
			throw exception_bound("CDLinkNode out of bound");
		return *first;
	}
	void removeindex(size_t inx)
	{
		CDLinkNode* first=root.mpNext;
		while( inx && first )
		{
			first = first->mpNext;
			inx--;
		}
		if( first &&  first != &root )
			first->unlink();
	}
	CDLinkNode* top()
	{
		if( root.mpNext &&  root.mpNext != &root )
		{
			return root.mpNext;
		}
		else
		{
			return NULL;
		}
	}
	bool top(CDLinkNode*& pn)
	{
		if( root.mpNext &&  root.mpNext != &root )
		{
			pn = root.mpNext;
			return true;
		}
		else
		{
			pn = NULL;
			return false;
		}
	}
	bool pop(CDLinkNode*& pn)
	{
		if( root.mpNext &&  root.mpNext != &root )
		{
			pn = root.mpNext;
			pn->unlink();
			return true;
		}
		else
		{
			pn = NULL;
			return false;
		}
	}
	CDLinkNode* pop()
	{
		if( root.mpNext && root.mpNext != &root )
		{
			CDLinkNode* elem = root.mpNext;
			elem->unlink();
			return elem;
		}
		else
		{
			return NULL;
		}
	}

	bool push(CDLinkNode& n)
	{
		this->insert(n);
		return true;
	}
};


template <class T> class TDLinkRoot : public CDLinkRoot
{
public:
	TDLinkRoot()	{}
	~TDLinkRoot()	{}

	void insert(T& n)						{ CDLinkRoot::insert(n); }
	void remove(T& n)						{ CDLinkRoot::remove(n); }
	bool isEmpty()							{ return CDLinkRoot::isEmpty(); }
	T& operator[](size_t inx)				{ return CDLinkRoot::operator[](inx); }
	const T& operator[](size_t inx) const	{ return CDLinkRoot::operator[](inx); }
	void removeindex(size_t inx)			{ CDLinkRoot::removeindex(inx); }
	T* top()								{ return dynamic_cast<T*>( CDLinkRoot::top() ); }
	bool top(T*& pn)						{ return CDLinkRoot::top( (CDLinkNode*&)pn ); }
	bool pop(T*& pn)						{ return CDLinkRoot::pop( (CDLinkNode*&)pn ); }
	T* pop()								{ return dynamic_cast<T*>( CDLinkRoot::pop() ); }
	bool push(T& n)							{ return CDLinkRoot::push(n); }
};



#endif//__BASELISTS__
