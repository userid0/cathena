
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "parser.h"
#include "eacompiler.h"

#include "base.h"
#include "basesafeptr.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int axtoi(const char *hexStg)
{
	int ret = 0;
	int cnt = 0;
	if( hexStg && hexStg[0]=='0' && hexStg[1]=='x' )
	{	
		hexStg+=2;
		while(*hexStg && (cnt++ < 2*sizeof(int)) )
		{
			if( (*hexStg>='0' && *hexStg<='9') )
				ret = (ret<<4) | 0x0F & (*hexStg - '0');
			else if( (*hexStg>='A' && *hexStg<='F') )
				ret = (ret<<4) | 0x0F & (*hexStg - 'A' + 10);
			else if( (*hexStg>='a' && *hexStg<='f') )
				ret = (ret<<4) | 0x0F & (*hexStg - 'a' + 10);
			else
				break;
			hexStg++;
		}
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct _list
{
	size_t			count;
	struct _list**	list;

	struct _symbol	symbol;
	struct _token	token;
};

struct _list* create_list()
{
	struct _list* l= (struct _list*)malloc( sizeof(struct _list) );
	if(l) memset(l,0,sizeof(struct _list));
	return l;
}


void add_listelement(struct _list* baselist, struct _list* child)
{
	if(baselist && child)
	{
		struct _list** cp = (struct _list**)malloc( (baselist->count+1)*sizeof(struct _list*) );
		memcpy(cp,baselist->list, baselist->count * sizeof(struct _list*));
		cp[baselist->count++] = child;
		free(baselist->list);
		baselist->list = cp;
	}
}


void reduce_tree(struct _parser* parser, int rtpos, struct _list* baselist, int flat)
{
	int j;
	struct _stack_element* se = parser->rt+rtpos;
	struct _stack_element* child;


	if( se->nrtchild==1 )
	{
		reduce_tree(parser, se->rtchildren[0], baselist, 0);
	}
	else
	{
		struct _list *newlist;
		if( flat==0)
		{
			newlist = create_list();
			newlist->symbol.idx  = se->symbol.idx;
			newlist->symbol.Name = strdup(se->symbol.Name);
			newlist->symbol.Type = se->symbol.Type;

			newlist->token.id	  = se->token.id;
			newlist->token.lexeme = strdup(se->token.lexeme);
			add_listelement(baselist, newlist);
		}
		else
			newlist = baselist;

		for(j=se->nrtchild-1;j>=0;--j)
		{
			child = parser->rt+(se->rtchildren[j]);
			if(child->symbol.Type != 1) // non terminal
			{
				flat = 0;
				// list layout
				if( se->nrtchild==2 &&
					child->symbol.idx == se->symbol.idx )
					flat = 1;
				else if( se->nrtchild==3 &&
					child->symbol.idx == se->symbol.idx &&
					PT_COMMA==(parser->rt+(se->rtchildren[1]))->symbol.idx )
					flat = 1;
				reduce_tree(parser, se->rtchildren[j], newlist, flat);			
			}
			else
			{
				struct _list *newterm = create_list();

				newterm->symbol.idx  = child->symbol.idx;
				newterm->symbol.Name = strdup(child->symbol.Name);
				newterm->symbol.Type = child->symbol.Type;

				newterm->token.id	  = child->token.id;
				newterm->token.lexeme = strdup(child->token.lexeme);

				add_listelement(newlist, newterm);
			}
		}
	}
}

void print_listtree(struct _list *baselist, size_t level)
{
	size_t i;
	if(baselist)
	{
		for(i=0; i<level; i++) printf("| ");
		printf("%c-<%s>(%i)[%i] ::= %s\n", 
			(baselist->symbol.Type)?'T':'N', 
			baselist->symbol.Name, 
			baselist->symbol.idx, 
			baselist->count, 
			baselist->token.lexeme);

		for(i=0; i<baselist->count; i++)
			print_listtree(baselist->list[i], level+1);
	}
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// MiniString
//
/////////////////////////////////////////////////////////////////////////////
class MiniString
{
	TPtrAutoRef< TArrayDST<char> > cStrPtr;

	void copy(const char *c)
	{	
		cStrPtr->resize(0);
		if(c) 
			cStrPtr->copy(c,strlen(c)+1,0);
		else
			cStrPtr->append(0);
	}
	void clear()
	{
		cStrPtr->resize(0);
		cStrPtr->append(0);
	}

	int compareTo(const MiniString &s) const 
	{	// compare with memcpy including the End-of-String
		// which is faster than doing a strcmp
		if( cStrPtr != s.cStrPtr )
		{
			if(s.cStrPtr->size()>1 && cStrPtr->size()>1) 
				return memcmp(*this, s, cStrPtr->size());

			if(s.cStrPtr->size()==0 && cStrPtr->size()==0) return 0;
			if(s.cStrPtr->size()==0) return 1;
			return -1;
		}
		return 0;
	}
	int compareTo(const char *c) const 
	{	// compare with memcpy including the End-of-String
		// which is faster than doing a strcmp
		if(c && cStrPtr->size()>0) return memcmp(*this,c, cStrPtr->size());
		if((!c || *c==0) && cStrPtr->size()==0) return 0;
		if((!c || *c==0)) return 1;
		return -1;
	}
public:
	MiniString(const char *c=NULL)		{ copy(c);}
	MiniString(const MiniString &str)	{ cStrPtr = str.cStrPtr; }
	virtual ~MiniString()				{ }

	const MiniString &operator=(const MiniString &str)	{ cStrPtr = str.cStrPtr; return *this; }
	const char*	operator=(const char *c)				{ copy(c); return cStrPtr->array(); }
	const char* get()									{ return cStrPtr->array(); }
	operator const char*() const						{ return cStrPtr->array(); }

	bool operator==(const char *b) const		{return (0==compareTo(b));}
	bool operator==(const MiniString &b) const	{return (0==compareTo(b));}
	bool operator!=(const char *b) const		{return (0!=compareTo(b));}
	bool operator!=(const MiniString &b) const	{return (0!=compareTo(b));}
	bool operator> (const char *b) const		{return (0> compareTo(b));}
	bool operator> (const MiniString &b) const	{return (0> compareTo(b));}
	bool operator< (const char *b) const		{return (0< compareTo(b));}
	bool operator< (const MiniString &b) const	{return (0< compareTo(b));}
	bool operator>=(const char *b) const		{return (0>=compareTo(b));}
	bool operator>=(const MiniString &b) const	{return (0>=compareTo(b));}
	bool operator<=(const char *b) const		{return (0<=compareTo(b));}
	bool operator<=(const MiniString &b) const	{return (0<=compareTo(b));}

	friend bool operator==(const char *a, const MiniString &b) {return (0==b.compareTo(a));}
	friend bool operator!=(const char *a, const MiniString &b) {return (0!=b.compareTo(a));}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Variant
{
	class _value
	{
		friend class Variant;

		typedef enum _value_type
		{
			VAR_NONE = 0,
			VAR_INTEGER,
			VAR_STRING,
			VAR_FLOAT,
			VAR_ARRAY,
		} VARTYPE;

		VARTYPE cType;
		union
		{
			// integer
			int64			cInteger;
			// double
			double			cFloat;
			// string
			struct
			{
				char*		cString;
				size_t		cStrLen;
			};
			// array
			struct
			{
				_value*		cArray;
				size_t		cSize;
			};
		};
	public:
		///////////////////////////////////////////////////////////////////////
		//
		_value() : cType(VAR_NONE), cInteger(0)
		{}
		~_value()
		{
			clear();
		}
		///////////////////////////////////////////////////////////////////////
		//
		void clear()
		{	
			if(cInteger)
			{
				switch(cType)
				{
				case VAR_ARRAY:
					if(cArray) delete [] cArray;
				case VAR_STRING:
					if(cString) delete [] cString;
				//case VAR_INTEGER:
				//case VAR_FLOAT:
				//case VAR_VARIABLE:
				}
				cType = VAR_NONE;
				cInteger = 0;
				cSize = 0;
			}
		}
		///////////////////////////////////////////////////////////////////////
		bool isValid() const	{ return cType != VAR_NONE; }	
		bool isInt() const		{ return cType == VAR_INTEGER; }	
		bool isFloat() const	{ return cType == VAR_FLOAT; }	
		bool isString() const	{ return cType == VAR_STRING; }	
		bool isArray() const	{ return cType == VAR_ARRAY; }
		size_t getSize() const	{ return (cType== VAR_ARRAY) ? cSize : 1; }
		///////////////////////////////////////////////////////////////////////
		void setarray(size_t cnt)
		{

		}


	};

	TPtrCommon< _value > cValue;

public:
	///////////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	Variant()	{}
	~Variant()	{}
	///////////////////////////////////////////////////////////////////////////
	// Copy/Assignment
	Variant(const Variant &v)					{ assign(v); }
	Variant(int val)							{ assign(val); }
	Variant(double val)							{ assign(val); }
	Variant(const char* val)					{ assign(val); }
	const Variant& operator=(const Variant &v)	{ return assign(v);	  }
	const Variant& operator=(const int val)		{ return assign(val); }
	const Variant& operator=(const double val)	{ return assign(val); }
	const Variant& operator=(const char* val)	{ return assign(val); }

	///////////////////////////////////////////////////////////////////////////
	// compare
	bool operator ==(const Variant &v) const 
	{
		if( cValue != v.cValue )
		{	// different pointers, compare content
			return cValue.get() == v.cValue.get();
		}
		return true; 
	}
	bool operator !=(const Variant &v) const 
	{ 
		if( cValue != v.cValue )
		{	// different pointers, compare content
			return cValue.get() != v.cValue.get();
		}
		return false;
	}

	///////////////////////////////////////////////////////////////////////////
	// Type Initialize Variant (aka copy)
	const Variant& assign(const Variant& v)
	{
		cValue = v.cValue;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize Integer
	const Variant& assign(int val)
	{
//		cValue = val;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize double
	const Variant& assign(double val)
	{
//		cValue = val;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize String
	const Variant& assign(const char* val)
	{
//		cValue = val;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Create Array
	void setarray(size_t cnt)
	{
		cValue->setarray(cnt);
	}

	///////////////////////////////////////////////////////////////////////////
	// Access to elements
	bool isValid() const	{ return cValue->isValid(); }
	bool isInt() const		{ return cValue->isInt(); }
	bool isFloat() const	{ return cValue->isFloat(); }
	bool isString() const	{ return cValue->isString(); }
	bool isArray() const	{ return cValue->isArray(); }
	size_t getSize() const	{ return cValue->getSize(); }

	///////////////////////////////////////////////////////////////////////////
	// Access to elements
	const Variant& operator[](size_t inx) const
	{	// on Arrays return the element, out of bounds return element 0
		// other array accesses return this object
		//return (cValue.cType==cValue.VAR_ARRAY) ? ((inx<cValue.cSize) ? cValue.cArray[inx] : cValue.cArray[0]) : *this;
		return *this;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////




class Variable
{

	Variable(const Variable&);					// no copy
	const Variable& operator=(const Variable&);	// no assign

public:
	MiniString	cName;
	Variant		cValue;
	///////////////////////////////////////////////////////////////////////////
	// Construct/Destruct
	Variable(const char* name) : cName(name)	{  }
	~Variable()	{  }

	///////////////////////////////////////////////////////////////////////////
	// PreInitialize Array
	bool setsize(size_t cnt)
	{
	//	cValue.setarray(cnt);

	}
	///////////////////////////////////////////////////////////////////////////
	// Set Values
	void set(int val)				{ cValue = val; }
	void set(double val)			{ cValue = val; }
	void set(const char* val)		{ cValue = val; }
	void set(const Variant & val)	{ cValue = val; }

	///////////////////////////////////////////////////////////////////////////
	// Set Value
	operator Variant()				{ return cValue; }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


class CProgramm
{
protected:
	class CLabel : public MiniString
	{	
	public:
		int		pos;
		size_t	use;

		CLabel(const char* n=NULL, int p=-1) : MiniString(n), pos(p),use(0)	{}
		virtual ~CLabel()	{}

	};

	TslistDCT<CLabel>			cLabelList;	// label list

	TArrayDST<unsigned char>	cProgramm;	// the stack programm
	size_t						cVarCnt;	// number of temporal variables
public:
	///////////////////////////////////////////////////////////////////////////
	// construct destruct
	CProgramm() : cVarCnt(0)
	{}
	virtual ~CProgramm()
	{}

	///////////////////////////////////////////////////////////////////////////
	// compares
	virtual operator==(const CProgramm& p) const	{ return this==&p; }
	virtual operator!=(const CProgramm& p) const	{ return this!=&p; }

	///////////////////////////////////////////////////////////////////////////
	// label functions
	int insertLabel(const char* name, int pos=-1)
	{
		size_t inx;
		cLabelList.insert(name);
		if( cLabelList.find(name,0, inx) )
		{
			if(pos>=0 && cLabelList[inx].pos>=0)
				return NULL;
			else if(pos>=0)
				cLabelList[inx].pos = pos;
			cLabelList[inx].use++;
			return inx;
		}
		return -1;
	}
	bool isLabel(const char* name, size_t &inx)
	{
		return cLabelList.find(name,0, inx);
	}
	int getLabelInx(const char* name)
	{
		size_t inx;
		if( cLabelList.find(name,0, inx) )
		{
			return cLabelList[inx].pos;
		}
		return -1;
	}
	const char *getLabelName(size_t inx)
	{
		if( inx<cLabelList.size() )
			return cLabelList[inx];
		return "";
	}
	int getLabelTarget(size_t inx)
	{
		if( inx<cLabelList.size() )
			return cLabelList[inx].pos;
		return (cProgramm.size()>0) ? cProgramm.size()-1 : 0;
	}
	bool ConvertLabels()
	{
		unsigned char cmd;
		size_t i, pos;
		int inx;
		i=0;
		while( i<cProgramm.size() )
		{	// need to copy the position, 
			// it gets incremented on access internally
			pos=i;
			cmd = getCommand(i);
			if( cmd==OP_GOTO )
			{	
				inx = getInt(i);
				inx = getLabelTarget( inx );

				replaceCommand(VX_GOTO,pos);
				replaceInt(inx,pos);
			}
			if( cmd==VX_LABEL )
			{	
				inx = getInt(i);
				inx = getLabelTarget( inx );

				replaceCommand(OP_PUSH_INT,pos);
				replaceInt(inx,pos);
			}
			else
			{
				switch( cmd )
				{
				// commands followed by an int or float (4 byte)
				case OP_NIF:
				case OP_IF:
				case OP_CALLBUILDIN:
				case OP_CALLSCRIPT:
				case OP_PUSH_FLOAT:
				case VX_BREAK:
				case VX_CONT:
				case VX_GOTO:
				case OP_GOTO:
				case OP_PUSH_INT:
				case OP_PUSH_TEMPVAR:
				case OP_PUSH_TEMPVALUE:
					i += 4;
					break;
				// commands followed by a string (pointer size)
				case OP_PUSH_VAR:
				case OP_PUSH_VALUE:
				case OP_PUSH_STRING:
					getString(i);
					break;
				}
			}
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// replacing temporary jump targets
	bool replaceJumps(size_t start, size_t end, unsigned char cmd, int val)
	{
		size_t pos;
		while( start<end && start<cProgramm.size() )
		{	// need to copy the position, 
			// it gets incremented on access internally
			pos=start;
			unsigned char c = getCommand(start);
			if( c==cmd )
			{	
				replaceCommand(VX_GOTO,pos);

				pos=start;
				if( 0==getInt(start) )
					replaceInt(val,pos);
			}
			else
			{
				switch( c )
				{
				// commands followed by an int or float (4 byte)
				case OP_NIF:
				case OP_IF:
				case OP_CALLBUILDIN:
				case OP_CALLSCRIPT:
				case OP_PUSH_FLOAT:
				case VX_BREAK:
				case VX_CONT:
				case VX_GOTO:
				case OP_GOTO:
				case OP_PUSH_INT:
				case OP_PUSH_TEMPVAR:
				case OP_PUSH_TEMPVALUE:
					start += 4;
					break;
				// commands followed by a string (pointer size)
				case OP_PUSH_VAR:
				case OP_PUSH_VALUE:
				case OP_PUSH_STRING:
					getString(start);
					break;
				}
			}
		}
		return true;
	}
	
	///////////////////////////////////////////////////////////////////////////
	// access functions
	unsigned char getCommand(size_t &inx)
	{
		if( inx < cProgramm.size() )
			return cProgramm[inx++];
		return OP_END;
	}
	size_t insertCommand(unsigned char val, size_t &inx)
	{
		size_t pos = inx;
		cProgramm.insert(val, 1, inx++);
		return pos;
	}
	size_t replaceCommand(unsigned char val, size_t &inx)
	{
		size_t pos = inx;
		cProgramm[inx++] = val;
		return pos;
	}
	size_t appendCommand(unsigned char val)
	{
		size_t pos = cProgramm.size();
		cProgramm.append(val);
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	unsigned char getChar(size_t &inx)
	{
		if( inx < cProgramm.size() )
			return cProgramm[inx++];
		return 0;
	}
	size_t insertChar(unsigned char val, size_t &inx)
	{
		size_t pos = inx;
		cProgramm.insert(val, 1, inx++);
		return pos;
	}
	size_t replaceChar(unsigned char val, size_t &inx)
	{
		size_t pos = inx;
		cProgramm[inx++] = val;
		return pos;
	}
	size_t appendChar(unsigned char val)
	{
		size_t pos = cProgramm.size();
		cProgramm.append(val);
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	short getShort(size_t &inx)
	{	// getting a 16bit integer
		if( inx+1 < cProgramm.size() )
		{	
			return 	  ((unsigned long)cProgramm[inx++])
					| ((unsigned long)cProgramm[inx++]<<0x08);
		}
		return 0;
	}
	size_t insertShort(short val, size_t &inx)
	{	// setting a 16bit integer
		size_t pos = inx;
		cProgramm.insert(GetByte(val,0), 1, inx++);
		cProgramm.insert(GetByte(val,1), 1, inx++);
		return pos;
	}
	size_t replaceShort(short val, size_t &inx)
	{	// setting a 16bit integer
		size_t pos = inx;
		cProgramm[inx++] = GetByte(val,0);
		cProgramm[inx++] = GetByte(val,1);
		return pos;
	}
	size_t appendShort(int val)
	{	// setting a 16bit integer
		size_t pos = cProgramm.size();
		cProgramm.append(GetByte(val,0));
		cProgramm.append(GetByte(val,1));
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	int getAddr(size_t &inx)
	{	// getting a 24bit integer
		if( inx+2 < cProgramm.size() )
		{	
			return 	  ((unsigned long)cProgramm[inx++])
					| ((unsigned long)cProgramm[inx++]<<0x08)
					| ((unsigned long)cProgramm[inx++]<<0x10);
		}
		return 0;
	}
	size_t insertAddr(int val, size_t &inx)
	{	// setting a 24bit integer
		size_t pos = inx;
		cProgramm.insert(GetByte(val,0), 1, inx++);
		cProgramm.insert(GetByte(val,1), 1, inx++);
		cProgramm.insert(GetByte(val,2), 1, inx++);
		return pos;
	}
	size_t replaceAddr(int val, size_t &inx)
	{	// setting a 24bit integer
		size_t pos = inx;
		cProgramm[inx++] = GetByte(val,0);
		cProgramm[inx++] = GetByte(val,1);
		cProgramm[inx++] = GetByte(val,2);
		return pos;
	}
	size_t appendAddr(int val)
	{	// setting a 24bit integer
		size_t pos = cProgramm.size();
		cProgramm.append(GetByte(val,0));
		cProgramm.append(GetByte(val,1));
		cProgramm.append(GetByte(val,2));
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	int getInt(size_t &inx)
	{	// getting a 32bit integer
		if( inx+3 < cProgramm.size() )
		{	
			return 	  ((unsigned long)cProgramm[inx++])
					| ((unsigned long)cProgramm[inx++]<<0x08)
					| ((unsigned long)cProgramm[inx++]<<0x10)
					| ((unsigned long)cProgramm[inx++]<<0x18);
		}
		return 0;
	}
	size_t insertInt(int val, size_t &inx)
	{	// setting a 32bit integer
		size_t pos = inx;
		cProgramm.insert(GetByte(val,0), 1, inx++);
		cProgramm.insert(GetByte(val,1), 1, inx++);
		cProgramm.insert(GetByte(val,2), 1, inx++);
		cProgramm.insert(GetByte(val,3), 1, inx++);
		return pos;
	}
	size_t replaceInt(int val, size_t &inx)
	{	// setting a 32bit integer
		size_t pos = inx;
		cProgramm[inx++] = GetByte(val,0);
		cProgramm[inx++] = GetByte(val,1);
		cProgramm[inx++] = GetByte(val,2);
		cProgramm[inx++] = GetByte(val,3);
		return pos;
	}
	size_t appendInt(int val)
	{	// setting a 32bit integer
		size_t pos = cProgramm.size();
		cProgramm.append(GetByte(val,0));
		cProgramm.append(GetByte(val,1));
		cProgramm.append(GetByte(val,2));
		cProgramm.append(GetByte(val,3));
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	float getFloat(size_t &inx)
	{	// getting a 32bit float
		if( inx+3 < cProgramm.size() )
		{
			union
			{
				float valf;
				char buf[4];
			}storage;

			storage.buf[0] = cProgramm[inx++];
			storage.buf[1] = cProgramm[inx++];
			storage.buf[2] = cProgramm[inx++];
			storage.buf[3] = cProgramm[inx++];
			return storage.valf;
		}
		return 0;
	}
	size_t insertFloat(float val, size_t &inx)
	{	// setting a 32bit float
		size_t pos = inx;
		union
		{
			float valf;
			char buf[4];
		}storage;
		storage.valf=val;

		cProgramm.insert(storage.buf[0], 1, inx++ );
		cProgramm.insert(storage.buf[1], 1, inx++ );
		cProgramm.insert(storage.buf[2], 1, inx++ );
		cProgramm.insert(storage.buf[3], 1, inx++ );
		return pos;
	}
	size_t replaceFloat(float val, size_t &inx)
	{	// setting a 32bit float
		size_t pos = inx;
		union
		{
			float valf;
			char buf[4];
		}storage;
		storage.valf=val;

		cProgramm[inx++] = storage.buf[0];
		cProgramm[inx++] = storage.buf[1];
		cProgramm[inx++] = storage.buf[2];
		cProgramm[inx++] = storage.buf[3];
		return pos;
	}
	size_t appendFloat(float val)
	{	// setting a 32bit float
		size_t pos = cProgramm.size();
		union
		{
			float valf;
			char buf[4];
		}storage;
		storage.valf=val;

		cProgramm.append(storage.buf[0]);
		cProgramm.append(storage.buf[1]);
		cProgramm.append(storage.buf[2]);
		cProgramm.append(storage.buf[3]);

		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	const char* getTableString(size_t &inx)
	{	// getting a pointer, 64bit ready
		// msb to lsb order for faster reading
		if( inx+sizeof(size_t) <= cProgramm.size() )
		{
			size_t i, vali = 0;
			for(i=0; i<sizeof(size_t); i++)
				vali = vali<<8 | ((size_t)cProgramm[inx++]);
			return (const char*)vali;
		}
		return "";
	}
	size_t insertTableString(const char*val, size_t &inx)
	{	// setting a pointer, 64bit ready
		// msb to lsb order for faster reading
		size_t i, vali = (size_t)val;
		size_t pos = inx;
		for(i=0; i<sizeof(size_t); i++)
		{
			cProgramm.insert( vali&0xFF, 1, inx+sizeof(size_t)-1-i );
			vali >>= 8;
		}
		inx += sizeof(size_t);
		return pos;
	}
	size_t replaceTableString(const char*val, size_t &inx)
	{	// setting a pointer, 64bit ready
		// msb to lsb order for faster reading
		size_t i, vali = (size_t)val;
		size_t pos = inx;
		for(i=0; i<sizeof(size_t); i++)
		{
			cProgramm[inx+sizeof(size_t)-1-i] = ( vali&0xFF );
			vali >>= 8;
		}
		inx += sizeof(size_t);
		return pos;
	}
	size_t appendTableString(const char*val)
	{	// setting a pointer, 64bit ready
		// msb to lsb order for faster reading
		size_t i, vali = (size_t)val;
		size_t pos = cProgramm.size();
		for(i=0; i<sizeof(size_t); i++)
		{
			cProgramm.append( (vali>>(8*(sizeof(size_t)-1-i)))&0xFF);
		}
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	const char* getString(size_t &inx)
	{	
		register size_t i=inx;
		const char *str = (const char *)&(cProgramm[i]);
		// search for the EOS marker
		while( cProgramm[i] && i<cProgramm.size() )
			i++;
		if( i>=cProgramm.size() ) 
		{	// did not found a marker, there is some serious error
			inx++;
			return "";
		}
		else
		{
			inx=i+1;
			return str;
		}
	}
	size_t insertString(const char*val, size_t &inx)
	{	
		size_t pos = inx;
		size_t sz = 0;
		// insert the EOS marker
		cProgramm.insert( (unsigned char)0, 1, inx );
		if(val)
		{
			sz = strlen(val);
			cProgramm.insert( (unsigned char*)val, sz, inx);
		}
		inx += 1+sz;
		return pos;
	}
	size_t appendString(const char*val)
	{
		size_t pos = cProgramm.size();
		if(val)
		{
			cProgramm.append( (unsigned char*)val, 1+strlen(val) );
		}
		else
		{	// just append a EOS marker
			cProgramm.append( 0 );
		}
		return pos;
	}
	///////////////////////////////////////////////////////////////////////////
	size_t getCurrentPosition()	{ return cProgramm.size(); }

	///////////////////////////////////////////////////////////////////////////
	size_t nextCommand(size_t pos)
	{
		switch( getCommand(pos) )
		{
		// commands with no parameters
		case OP_NOP:
		case OP_ASSIGN:
		case OP_ASSIGN_ADD:
		case OP_ASSIGN_SUB:
		case OP_ASSIGN_MUL:
		case OP_ASSIGN_DIV:
		case OP_ASSIGN_XOR:
		case OP_ASSIGN_AND:
		case OP_ASSIGN_OR:
		case OP_ASSIGN_RSH:
		case OP_ASSIGN_LSH:
		case OP_SELECT:
		case OP_LOG_OR:
		case OP_LOG_AND:
		case OP_BIN_OR:
		case OP_BIN_XOR:
		case OP_BIN_AND:
		case OP_EQUATE:
		case OP_UNEQUATE:
		case OP_ISGT:
		case OP_ISGTEQ:
		case OP_ISLT:
		case OP_ISLTEQ:
		case OP_LSHIFT:
		case OP_RSHIFT:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_NOT:
		case OP_INVERT:
		case OP_NEGATE:
		case OP_SIZEOF:
		case OP_CAST:
		case OP_PREADD:
		case OP_PRESUB:
		case OP_POSTADD:
		case OP_POSTSUB:
		case OP_MEMBER:
		case OP_ARRAY:
		case OP_RESIZE:
		case OP_CLEAR:
		case OP_NIF:
		case OP_IF:
		case OP_POP:
		case OP_END:
			return pos;

		// commands with int (float) parameters (4 bytes)
		case OP_START:
		case OP_PUSH_INT:
		case OP_PUSH_FLOAT:
		case VX_BREAK:
		case VX_CONT:
		case VX_GOTO:
		case VX_LABEL:
		case OP_GOTO:
		case OP_PUSH_TEMPVAR:
		case OP_PUSH_TEMPVALUE:
			return pos+4;

		// commands with int and char parameters (5 bytes)
		case OP_CALLSCRIPT:
		case OP_CALLBUILDIN:
			return pos+5;

		// commands with string parameters (sizeof(pointer) bytes)
		case OP_PUSH_STRING:
		case OP_PUSH_VAR:
		case OP_PUSH_VALUE:
			getString(pos);
			return pos;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	size_t getParameter(size_t pos)
	{
		switch( getCommand(pos) )
		{
		// commands with no parameters
		case OP_NOP:
		case OP_ASSIGN:
		case OP_ASSIGN_ADD:
		case OP_ASSIGN_SUB:
		case OP_ASSIGN_MUL:
		case OP_ASSIGN_DIV:
		case OP_ASSIGN_XOR:
		case OP_ASSIGN_AND:
		case OP_ASSIGN_OR:
		case OP_ASSIGN_RSH:
		case OP_ASSIGN_LSH:
		case OP_SELECT:
		case OP_LOG_OR:
		case OP_LOG_AND:
		case OP_BIN_OR:
		case OP_BIN_XOR:
		case OP_BIN_AND:
		case OP_EQUATE:
		case OP_UNEQUATE:
		case OP_ISGT:
		case OP_ISGTEQ:
		case OP_ISLT:
		case OP_ISLTEQ:
		case OP_LSHIFT:
		case OP_RSHIFT:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MOD:
		case OP_NOT:
		case OP_INVERT:
		case OP_NEGATE:
		case OP_SIZEOF:
		case OP_CAST:
		case OP_PREADD:
		case OP_PRESUB:
		case OP_POSTADD:
		case OP_POSTSUB:
		case OP_MEMBER:
		case OP_ARRAY:
		case OP_RESIZE:
		case OP_CLEAR:
		case OP_NIF:
		case OP_IF:
		case OP_POP:
		case OP_END:
			return 0;

		// commands with int (float) parameters (4 bytes)
		case OP_START:
		case OP_PUSH_INT:
		case OP_PUSH_FLOAT:
		case OP_CALLSCRIPT:
		case OP_CALLBUILDIN:
		case VX_BREAK:
		case VX_CONT:
		case VX_GOTO:
		case VX_LABEL:
		case OP_GOTO:

		// commands with string parameters (sizeof(pointer) bytes)
		case OP_PUSH_STRING:
		case OP_PUSH_VAR:
		case OP_PUSH_VALUE:
			return pos;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// debug
	void dump()
	{
		size_t i;

		printf("binary output:\n");

		for(i=0; i<cProgramm.size(); i++)
		{
			printf("%3i  ", cProgramm[i]);
			if(15==i%16) printf("\n");
		}
		printf("\n\n");
		printf("command sequence:\n");
		for(i=0; i<cProgramm.size(); printCommand(i),printf("\n"));

		printf("\n");
		printf("labels:\n");
		for(i=0; i<cLabelList.size(); i++)
		{
			printf("'%s' jump to %i (used: %i)\n", (const char*)cLabelList[i], cLabelList[i].pos, cLabelList[i].use);
		}
		printf("\n");


	}
	void printCommand(size_t &pos)
	{
		printf("%4i: ",pos);
		switch( getCommand(pos) )
		{
		// commands with no parameters
		case OP_NOP:
			printf("nop"); break;
		case OP_ASSIGN:
			printf("assign"); break;
		case OP_ASSIGN_ADD:
			printf("assign add"); break;
		case OP_ASSIGN_SUB:
			printf("assign sub"); break;
		case OP_ASSIGN_MUL:
			printf("assign mul"); break;
		case OP_ASSIGN_DIV:
			printf("assign div"); break;
		case OP_ASSIGN_XOR:
			printf("assign xor"); break;
		case OP_ASSIGN_AND:
			printf("assign and"); break;
		case OP_ASSIGN_OR:
			printf("assign or"); break;
		case OP_ASSIGN_RSH:
			printf("assign rightshift"); break;
		case OP_ASSIGN_LSH:
			printf("assign leftshift"); break;
		case OP_SELECT:
			printf("select"); break;
		case OP_LOG_OR:
			printf("logic or"); break;
		case OP_LOG_AND:
			printf("logic and"); break;
		case OP_BIN_OR:
			printf("binary or"); break;
		case OP_BIN_XOR:
			printf("binary xor"); break;
		case OP_BIN_AND:
			printf("binary and"); break;
		case OP_EQUATE:
			printf("equal"); break;
		case OP_UNEQUATE:
			printf("uneqal"); break;
		case OP_ISGT:
			printf("compare greater then"); break;
		case OP_ISGTEQ:
			printf("compare greater/equal then"); break;
		case OP_ISLT:
			printf("compare less then"); break;
		case OP_ISLTEQ:
			printf("compare less/equal then"); break;
		case OP_LSHIFT:
			printf("leftshift"); break;
		case OP_RSHIFT:
			printf("rightshift"); break;
		case OP_ADD:
			printf("add"); break;
		case OP_SUB:
			printf("sub"); break;
		case OP_MUL:
			printf("mul"); break;
		case OP_DIV:
			printf("div"); break;
		case OP_MOD:
			printf("modulo"); break;
		case OP_NOT:
			printf("logic not"); break;
		case OP_INVERT:
			printf("binary invert"); break;
		case OP_NEGATE:
			printf("arithmetic negate"); break;
		case OP_SIZEOF:
			printf("sizeof"); break;
		case OP_CAST:
			printf("cast"); break;
		case OP_PREADD:
			printf("preop add"); break;
		case OP_PRESUB:
			printf("preop sub"); break;
		case OP_POSTADD:
			printf("postop add"); break;
		case OP_POSTSUB:
			printf("postop sub"); break;
		case OP_MEMBER:
			printf("member access"); break;
		case OP_ARRAY:
			printf("array access"); break;
		case OP_RESIZE:
			printf("array resize"); break;
		case OP_CLEAR:
			printf("clear variable"); break;
		case OP_POP:
			printf("pop stack"); break;
		case OP_END:
			printf("quit"); break;

		// commands with int (float) parameters (4 bytes)
		case OP_START:
			printf("start (progsize=%i)", getInt(pos)); break;
		case OP_PUSH_INT:
			printf("push int '%i'", getInt(pos)); break;
		case OP_PUSH_FLOAT:
			printf("push float '%f'", getFloat(pos)); break;
		case VX_BREAK:
			printf("break (error: not converted) '%i'", getInt(pos)); break;
		case VX_CONT:
			printf("continue (error: not converted) '%i'", getInt(pos)); break;
		case VX_GOTO:
			printf("jump to '%i'", getInt(pos)); break;
		case OP_PUSH_TEMPVAR:
			printf("push temp variable '%i'", getInt(pos)); break;
		case OP_PUSH_TEMPVALUE:
			printf("push value '%i'", getInt(pos)); break;


		case OP_NIF:
			printf("conditional jump on false to '%i'", getInt(pos)); break;
		case OP_IF:
			printf("conditional jump on true to '%i'", getInt(pos)); break;
		case OP_GOTO:
		{
			size_t inx = getInt(pos);
			printf("goto label '%s' jump to %i", getLabelName(inx), getLabelTarget(inx)); break;
		}
		case VX_LABEL:
		{
			size_t inx = getInt(pos);
			printf("label '%s' jump to %i", getLabelName(inx), getLabelTarget(inx)); break;
		}
			

		// commands with int and char parameters (5 bytes)
		case OP_CALLSCRIPT:
		{	
			int p = getInt(pos);
			int a = getChar(pos);
			printf("call script '%i' (%i args)", p, a); break;
		}
		case OP_CALLBUILDIN:
		{
			int p = getInt(pos);
			int a = getChar(pos);
			printf("call buildin '%i' (%i args)", p, a); break;
		}


		// commands with string parameters (sizeof(pointer) bytes)
		case OP_PUSH_STRING:
			printf("push string '%s'", getString(pos)); break;
		case OP_PUSH_VAR:
			printf("push variable '%s'", getString(pos)); break;
		case OP_PUSH_VALUE:
			printf("push value '%s'", getString(pos)); break;
		}
	}
};


class CBuildin
{
	MiniString	cID;
	size_t		cParaCnt;
public:
	///////////////////////////////////////////////////////////////////////////
	// construct destruct
	CBuildin() : cID(NULL),cParaCnt(0)
	{}
	CBuildin(const char* n, size_t p=0) : cID(n),cParaCnt(p)
	{}
	virtual ~CBuildin()
	{}
	///////////////////////////////////////////////////////////////////////////
	// compares
	virtual operator==(const char *name) const	{ return 0==strcmp(cID,name); }
	virtual operator!=(const char *name) const	{ return 0!=strcmp(cID,name); }
	virtual operator==(const CBuildin& p) const	{ return this==&p; }
	virtual operator!=(const CBuildin& p) const	{ return this!=&p; }
	///////////////////////////////////////////////////////////////////////////
	// accesses
	size_t Param()		{ return cParaCnt; }
	const char* Name()	{ return cID; }

	///////////////////////////////////////////////////////////////////////////
	// function call entrance
	int entrypoint()
	{
		// get/check arguments

		function();

		// return to script interpreter

	}

	///////////////////////////////////////////////////////////////////////////
	// user function which is called on activation
	// overload with your function
	virtual int function()	{ return 0; }
};

class CFunction : public CProgramm
{
	// <Func Decl>  ::= <Scalar> Id '(' <Params>  ')' <Block>	// fixed parameter count
    //                | 'function' 'script' <Name Id> <Block>	// variable parameter
	MiniString	cID;
	size_t		cParaCnt;
public:
	///////////////////////////////////////////////////////////////////////////
	// construct destruct
	CFunction() : cID(NULL),cParaCnt(0)
	{}
	CFunction(const char* n, size_t p=0) : cID(n),cParaCnt(p)
	{}
	virtual ~CFunction()
	{}
	///////////////////////////////////////////////////////////////////////////
	// compares
	virtual operator==(const char *name) const	{ return 0==strcmp(cID,name); }
	virtual operator!=(const char *name) const	{ return 0!=strcmp(cID,name); }
	virtual operator==(const CFunction& p) const	{ return this==&p; }
	virtual operator!=(const CFunction& p) const	{ return this!=&p; }
	///////////////////////////////////////////////////////////////////////////
	// accesses
	size_t Param()		{ return cParaCnt; }
	const char* Name()	{ return cID; }

};

class CScript : public CProgramm
{
	//	<Script Decl> ::= '-' 'script' <Name Id> <Sprite Id> ',' <Block>
    //                  | <File Id> ',' DecLiteral ',' DecLiteral ',' DecLiteral 'script' <Name Id> <Sprite Id> ',' <Block>
	MiniString cID;
	MiniString cName;
	MiniString cMap;
	unsigned short cX;
	unsigned short cY;
	unsigned char cDir;
	unsigned short cSprite;
public:
	///////////////////////////////////////////////////////////////////////////
	// construct destruct
	CScript() : cID(NULL),cName(NULL),cMap(NULL),cX(0),cY(0),cDir(0),cSprite(0)
	{}
	CScript(const char* id, const char* name, const char* map, unsigned short x,unsigned short y, unsigned char dir, unsigned short sprite)
		: cID(id),cName(name),cMap(map),cX(x),cY(y),cDir(dir),cSprite(sprite)
	{}
	virtual ~CScript()
	{}
	///////////////////////////////////////////////////////////////////////////
	// compares
	virtual operator==(const char *name) const	{ return 0==strcmp(cID,name); }
	virtual operator!=(const char *name) const	{ return 0!=strcmp(cID,name); }
	virtual operator==(const CScript& p) const	{ return this==&p; }
	virtual operator!=(const CScript& p) const	{ return this!=&p; }

};


class CScriptEnvironment
{
	class CConstant : public MiniString
	{	
	public:
		int		cValue;	// should be a variant

		CConstant(const char* n=NULL, int v=0) : MiniString(n), cValue(v)	{}
		virtual ~CConstant()	{}
	};

	class CParameter : public MiniString
	{	
	public:
		size_t		cID;

		CParameter(const char* n=NULL, size_t i=0) : MiniString(n), cID(i)	{}
		virtual ~CParameter()	{}
	};

	TslistDCT<CConstant>	cConstTable;		// table of constants
	TslistDCT<CParameter>	cParamTable;		// table of parameters


	TArrayDCT<CBuildin>		cBuildinTable;		// table of buildin functions
	TArrayDCT<CFunction>	cFunctionTable;		// table of script functions
	TArrayDCT<CScript>		cScriptTable;		// table of scripts (npc)
	TslistDCT<MiniString>	cStringTable;		// table o strings
	// compile time only
	// table of constants
	// table of parameter keywords

public:
	CScriptEnvironment()	{}
	~CScriptEnvironment()	{}

	const char* insert2Stringtable(const char*str)
	{
		size_t pos;
		cStringTable.insert( str );
		if( cStringTable.find(str,0,pos) )
		{	// look up the sting to get it's index
			return cStringTable[pos];
		}
		return NULL;
	}

	int getFunctionID(const char* name) const
	{
		for(size_t i=0; i<cFunctionTable.size(); i++)
		{
			if( cFunctionTable[i] == name )
				return i;
		}
		return -1;
	}
	int getScriptID(const char* name)
	{
		for(size_t i=0; i<cScriptTable.size(); i++)
		{
			if( cScriptTable[i] == name )
				return i;
		}
		return -1;
	}
	int addFunction(const char *name, size_t param)
	{
		size_t pos;
		for(pos=0; pos<cFunctionTable.size(); pos++)
		{
			if( cFunctionTable[pos] == name )
				return pos;
		}
		if( cFunctionTable.append( CFunction(name,param) ) )
			return pos;
		return -1;
	}
	int addScript(const char* id, const char* name, const char* map, unsigned short x,unsigned short y, unsigned char dir, unsigned short sprite)
	{
		size_t pos;
		for(pos=0; pos<cScriptTable.size(); pos++)
		{
			if( cScriptTable[pos] == name )
				return pos;
		}

		if( cScriptTable.append( CScript(id, name, map, x, y, dir, sprite) ) )
			return pos;
		return -1;
	}
	CFunction	&function(size_t pos)
	{	// will automatically throw on out-of-bound
		return cFunctionTable[pos];	
	}
	CScript		&script(size_t pos)
	{	// will automatically throw on out-of-bound
		return cScriptTable[pos];	
	}

};


///////////////////////////////////////////////////////////////////////////////
// the user stack
///////////////////////////////////////////////////////////////////////////////
class UserStack : private noncopyable
{
	///////////////////////////////////////////////////////////////////////////
	TstackDCT<Variant>		cStack;		// the stack
	size_t					cSC;		// stack counter
	size_t					cSB;		// initial stack start index
	size_t					cParaBase;	// function parameter start index

	size_t					cPC;		// Programm Counter
	CProgramm*				cProg;


	bool process()
	{
		bool run = true;
		bool ok =true;
		while(cProg && run && ok)
		{
			switch( cProg->getCommand(cPC) )
			{
			case OP_NOP:
			{
				break;
			}

			/////////////////////////////////////////////////////////////////
			// assignment operations
			// take two stack values and push up one
			case OP_ASSIGN:	// <Op If> '='   <Op>
			{
				cSC--;
				cStack[cSC-1] = cStack[cSC];
				break;
			}
				
			case OP_ADD:		// <Op AddSub> '+' <Op MultDiv>
			case OP_ASSIGN_ADD:	// <Op If> '+='  <Op>
			{
				cSC--;
//				cStack[cSC-1] += cStack[cSC];
				break;
			}
			case OP_SUB:		// <Op AddSub> '-' <Op MultDiv>
			case OP_ASSIGN_SUB:	// <Op If> '-='  <Op>
			{
				cSC--;
//				cStack[cSC-1] -= cStack[cSC];
				break;
			}
			case OP_MUL:		// <Op MultDiv> '*' <Op Unary>
			case OP_ASSIGN_MUL:	// <Op If> '*='  <Op>
			{
				cSC--;
//				cStack[cSC-1] *= cStack[cSC];
				break;
			}
			case OP_DIV:		// <Op MultDiv> '/' <Op Unary>
			case OP_ASSIGN_DIV:	// <Op If> '/='  <Op>
			{
				cSC--;
//				cStack[cSC-1] /= cStack[cSC];
				break;
			}
			case OP_MOD:		// <Op MultDiv> '%' <Op Unary>
			{
				cSC--;
//				cStack[cSC-1] /= cStack[cSC];
				break;
			}
			case OP_BIN_XOR:	// <Op BinXOR> '^' <Op BinAND>
			case OP_ASSIGN_XOR:	// <Op If> '^='  <Op>
			{
				cSC--;
//				cStack[cSC-1] ^= cStack[cSC];
				break;
			}
			case OP_BIN_AND:		// <Op BinAND> '&' <Op Equate>
			case OP_ASSIGN_AND:	// <Op If> '&='  <Op>
			{
				cSC--;
//				cStack[cSC-1] &= cStack[cSC];
				break;
			}
			case OP_BIN_OR:		// <Op BinOr> '|' <Op BinXOR>
			case OP_ASSIGN_OR:	// <Op If> '|='  <Op>
			{
				cSC--;
//				cStack[cSC-1] |= cStack[cSC];
				break;
			}
			case OP_LOG_AND:	// <Op And> '&&' <Op BinOR>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] && (int)cStack[cSC];
				break;
			}
			case OP_LOG_OR:		// <Op Or> '||' <Op And>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] || (int)cStack[cSC];
				break;
			}
			case OP_RSHIFT:		// <Op Shift> '>>' <Op AddSub>
			case OP_ASSIGN_RSH:	// <Op If> '>>='  <Op>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] >> (int)cStack[cSC];
				break;
			}
			case OP_LSHIFT:		// <Op Shift> '<<' <Op AddSub>
			case OP_ASSIGN_LSH:	// <Op If> '<<='  <Op>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] << (int)cStack[cSC];
				break;
			}
			case OP_EQUATE:		// <Op Equate> '==' <Op Compare>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]==cStack[cSC]);
				break;
			}
			case OP_UNEQUATE:	// <Op Equate> '!=' <Op Compare>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]!=cStack[cSC]);
				break;
			}
			case OP_ISGT:		// <Op Compare> '>'  <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]>cStack[cSC]);
				break;
			}
			case OP_ISGTEQ:		// <Op Compare> '>=' <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]>=cStack[cSC]);
				break;
			}
			case OP_ISLT:		// <Op Compare> '<'  <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]<cStack[cSC]);
				break;
			}
			case OP_ISLTEQ:		// <Op Compare> '<=' <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]<=cStack[cSC]);
				break;
			}
			/////////////////////////////////////////////////////////////////
			// select operation
			// take three stack values and push the second or third depending on the first
			case OP_SELECT:	// <Op Or> '?' <Op If> ':' <Op If>
			{
				cSC -= 2;
//				cStack[cSC-1] = ((int)cStack[cSC-1]) ? cStack[cSC] : cStack[cSC+1];
				break;
			}

			/////////////////////////////////////////////////////////////////
			// unary operations
			// take one stack values and push a value
			case OP_NOT:	// '!'    <Op Unary>
			{
//				cStack[cSC-1] = !((int)cStack[cSC-1]);
				break;
			}
			case OP_INVERT:	// '~'    <Op Unary>
			{
//				cStack[cSC-1] = ~((int)cStack[cSC-1]);
				break;
			}
			case OP_NEGATE:	// '-'    <Op Unary>
			{
//				cStack[cSC-1] = -cStack[cSC-1];
				break;
			}

			/////////////////////////////////////////////////////////////////
			// sizeof operations
			// take one stack values and push the result
								// sizeof '(' <Type> ')' // replaces with OP_PUSH_INT on compile time
			case OP_SIZEOF:		// sizeof '(' Id ')'
			{
//				cStack[cSC-1] = cStack[cSC-1].size();
				break;
			}

			/////////////////////////////////////////////////////////////////
			// cast operations
			// take one stack values and push the result
			case OP_CAST:	// '(' <Type> ')' <Op Unary>   !CAST
			{	// <Op Unary> is first on the stack, <Type> is second
				cSC--;
//				cStack[cSC-1].cast(cStack[cSC]);
				break;
			}

			/////////////////////////////////////////////////////////////////
			// Pre operations
			// take one stack variable and push a value
			case OP_PREADD:		// '++'   <Op Unary>
			{	
//				cStack[cSC-1]++;
//				cStack[cSC-1] = cStack[cSC-1].value();
				break;
			}
			case OP_PRESUB:		// '--'   <Op Unary>
			{	
//				cStack[cSC-1]--;
//				cStack[cSC-1] = cStack[cSC-1].value();
				break;
			}
			/////////////////////////////////////////////////////////////////
			// Post operations
			// take one stack variable and push a value
			case OP_POSTADD:	// <Op Pointer> '++'
			{	
//				Variant temp = cStack[cSC-1].value();
//				cStack[cSC-1]++;
//				cStack[cSC-1] = temp;
				break;
			}
			case OP_POSTSUB:	// <Op Pointer> '--'
			{	
//				Variant temp = cStack[cSC-1].value();
//				cStack[cSC-1]--;
//				cStack[cSC-1] = temp;
				break;
			}



			/////////////////////////////////////////////////////////////////
			// Member Access
			// take a variable and a value from stack and push a varible
			case OP_MEMBER:		// <Op Pointer> '.' <Value>     ! member
			{
				printf("not implemented yet\n");

				cSC--;
				break;
			}


			/////////////////////////////////////////////////////////////////
			// Array
			// take a variable and a value from stack and push a varible
			case OP_ARRAY:		// <Op Pointer> '[' <Expr> ']'  ! array
			{
				cSC--;
//				cStack[cSC-1] = cStack[cSC-1][ cStack[cSC] ];
				break;
			}


			/////////////////////////////////////////////////////////////////
			// standard function calls
			// check the values on stack before or inside the call of function
			case OP_CALLSCRIPT:
							// Id '(' <Expr> ')'
							// Id '(' ')'
							// Id <Call List> ';'
							// Id ';'
			{

				printf("not implemented yet\n");
				cSC--;
				break;
			}
			/////////////////////////////////////////////////////////////////
			// standard function calls
			// check the values on stack before or inside the call of function
			case OP_CALLBUILDIN:
							// Id '(' <Expr> ')'
							// Id '(' ')'
							// Id <Call List> ';'
							// Id ';'
			{

				printf("not implemented yet\n");
				cSC--;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// conditional branch
			// take one value from stack 
			// and add 1 or the branch offset to the programm counter
			case OP_NIF:	// if '(' <Expr> ')' <Normal Stm>
			{

				break;
			}
			case OP_IF:		// if '(' <Expr> ')' <Normal Stm>
			{

				break;
			}
			/////////////////////////////////////////////////////////////////
			// unconditional branch
			// add the branch offset to the programm counter
			case OP_GOTO:	// goto Id ';'
			{

				break;
			}
			case VX_GOTO:	// goto position
			{

				break;
			}


			/////////////////////////////////////////////////////////////////
			// explicit stack pushes
			// Values pushed on stack directly
			// HexLiteral
			// DecLiteral
			// StringLiteral
			// CharLiteral
			// FloatLiteral
			// Id
			// <Call Arg>  ::= '-'

			case OP_PUSH_INT:	// followed by an integer
			{
				
				int temp = cProg->getInt(cPC);
				cStack[cSC] = temp;
				cSC++;
				break;
			}
			case OP_PUSH_STRING:	// followed by a string (pointer)
			{
				const char* temp = cProg->getString(cPC);
				cStack[cSC] = temp;
				cSC++;
				break;
			}
			case OP_PUSH_FLOAT:	// followed by a float
			{
				float temp = cProg->getFloat(cPC);
				cStack[cSC] = temp;
				cSC++;
				break;
			}
			case OP_PUSH_VAR:	// followed by a string containing a variable name
			{
				unsigned char vartype = cProg->getChar(cPC);
				const char*   varname = cProg->getString(cPC);
//				cStack[cSC] = findvariable(varname, vartype);
				cSC++;
				break;
			}
			case OP_POP:	// decrements the stack by one
			{
				cSC--;
				break;
			}
			case VX_LABEL:
			{
				break;
			}
			case OP_END:
			default:
			{
				run=false;
				ok=false;
				break;
			}
			}// end switch
		}// end while
	}


public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	UserStack() : cSC(0), cSB(0), cParaBase(0), cPC(0), cProg(NULL)
	{}
	~UserStack()
	{}

	///////////////////////////////////////////////////////////////////////////
	// start a programm
	bool Call(size_t programm_id)
	{		

	
	}
	///////////////////////////////////////////////////////////////////////////
	// comming back from a callback
	void Continue(Variant retvalue)
	{


	}

};




///////////////////////////////////////////////////////////////////////////////
// compiler
///////////////////////////////////////////////////////////////////////////////
class CScriptCompiler
{
	///////////////////////////////////////////////////////////////////////////
	// class types

	///////////////////////////////////////////////////////
	// compile flags
	enum
	{
		CFLAG_NONE		= 0x00000000,	// no restrictions
		CFLAG_LVALUE	= 0x00000001,	// a variable is required
		CFLAG_GLOBAL	= 0x00000002,	// a global variable/value is required
		CFLAG_USE_BREAK	= 0x00000004,	// allow break + jump offset 
		CFLAG_USE_CONT	= 0x00000008	// allow continue + jump offset 

	};
	///////////////////////////////////////////////////////
	// variable types
	typedef enum
	{
		VAR_TEMP,		// temp variable
		VAR_GACCOUNT,	// global account variable
		VAR_GCHAR,		// global character variable
		VAR_GSERVER		// global server variable
	} vartype;
	///////////////////////////////////////////////////////
	// simple temporary list element
	class _stmlist
	{
	public:
		struct _list*	value;
		struct _list*	stm;
		size_t			gotomarker;

		_stmlist(struct _list *v=NULL, struct _list *s=NULL)
			: value(v), stm(s), gotomarker(0)
		{}
		~_stmlist()	{}

		bool operator==(const _stmlist &v) const { return this==&v; }
	};

	///////////////////////////////////////////////////////
	// variable name storage
	class CVariable : public MiniString
	{	
	public:
		size_t		id;
		vartype		type;
		size_t		use;

		CVariable(const char* n=NULL, size_t id=0xFFFFFFFF, vartype t=VAR_TEMP)
			: MiniString(n), id(id), type(t), use(0)
		{}
		virtual ~CVariable()	{}
	};


	///////////////////////////////////////////////////////////////////////////
	// class data
	TslistDCT<CVariable>	cTempVar;		// variable list
	CScriptEnvironment		&cEnv;			// the current script environment



	///////////////////////////////////////////////////////////////////////////
	// variable functions
	int insertVariable(const char* name, vartype t)
	{
		size_t inx;
		size_t id = cTempVar.size();
		cTempVar.insert( CVariable(name,id,t) );
		if( cTempVar.find(name, 0, inx) )
		{
			return inx;
		}
		return -1;
	}
	bool isVariable(const char* name, size_t &inx)
	{
		return cTempVar.find(name,0, inx);
	}

	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
public:
	CScriptCompiler(CScriptEnvironment &e):cEnv(e)	{}
	~CScriptCompiler()	{}

private:

	///////////////////////////////////////////////////////////////////////////
	// terminal checking functions
	bool CheckTerminal(struct _list *baselist, int idx)
	{
		return ( baselist && baselist->symbol.Type==1 && baselist->symbol.idx==idx );
	}
	bool CheckNonTerminal(struct _list *baselist, int idx)
	{
		return ( baselist && baselist->symbol.Type==0 && baselist->symbol.idx==idx );
	}
	///////////////////////////////////////////////////////////////////////////
	// debug functions
	void PrintTerminal(struct _list *baselist)
	{
		if( baselist )
		{
			const char *lp = (baselist->symbol.Type==1)?baselist->token.lexeme:baselist->symbol.Name;
			printf("%s", (lp)?lp:"");
		}
	}
	void PrintChildTerminals(struct _list *baselist)
	{
		if( baselist )
		{
			const char *lp;
			size_t i;
			for(i=0; i<baselist->count; i++)
			{
				if( baselist->list[i] )
				{
					lp = (baselist->list[i]->symbol.Type==1)?baselist->list[i]->token.lexeme:baselist->list[i]->symbol.Name;
					printf("%s ", (lp)?lp:"");
				}
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// main compile loop, is called recursively with all parse tree nodes
	bool CompileMain(struct _list *baselist, size_t level, unsigned long flags, CProgramm& prog, int& userval)
	{
		size_t i;
		bool accept = false;
		if(baselist)
		{
			///////////////////////////////////////////////////////////////////////
			
			if( baselist->symbol.Type==1 )
			{	// terminals
				
				switch(baselist->symbol.idx)
				{
				case PT_LBRACE:
					printf("PT_LBRACE - ");
					printf("increment skope counter\n");
					accept = true;
					break;
				case PT_RBRACE:
					printf("PT_RBRACE - ");
					printf("decrement skope counter, clear local variables\n");
					accept = true;
					break;
				case PT_SEMI:
					printf("PT_SEMI - ");
					printf("clear stack\n");
					prog.appendCommand(OP_POP);
					accept = true;
					break;

				case PT_HEXLITERAL:
					printf("PT_HEXLITERAL - ");
					printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						prog.appendCommand(OP_PUSH_INT);
						prog.appendInt( axtoi(baselist->token.lexeme) );

						printf("accepted\n");
						accept = true;
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	

					break;

				case PT_DECLITERAL:
					printf("PT_DECLITERAL - ");
					printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						prog.appendCommand(OP_PUSH_INT);
						prog.appendInt( atoi(baselist->token.lexeme) );

						printf("accepted\n");
						accept = true;
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	

					break;

				case PT_CHARLITERAL:
					printf("PT_CHARLITERAL - ");
					printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						prog.appendCommand(OP_PUSH_INT);
						prog.appendInt( (baselist->token.lexeme)? baselist->token.lexeme[1]:0 );

						printf("accepted\n");
						accept = true;
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	

					break;

				case PT_FLOATLITERAL:
					printf("PT_FLOATLITERAL - ");
					printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						prog.appendCommand(OP_PUSH_FLOAT);
						prog.appendFloat( (float)atof(baselist->token.lexeme) );

						printf("accepted\n");
						accept = true;
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	

					break;

				case PT_STRINGLITERAL:
					printf("PT_STRINGLITERAL - ");
					printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						
						// get the string without leading quotation mark
						const char *ip;
						char *str = (baselist->token.lexeme) ? baselist->token.lexeme+1 : " ";
						size_t endpos = strlen(str)-1;
						str[endpos]=0; // cut off the trailing quotation mark
						ip = cEnv.insert2Stringtable( str );
						str[endpos]='"'; // put back the trailing quotation mark
						
						if( ip )
						{	
							prog.appendCommand(OP_PUSH_STRING);
							prog.appendString( ip );

							printf("accepted\n");
							accept = true;
						}
						else
						{
							printf("error when inserting to stringtable\n");
						}
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	
					break;

				case PT_MINUS:
					printf("PT_MINUS - ");
					if( 0 == (flags&CFLAG_LVALUE) )
					{
						prog.appendCommand(OP_PUSH_INT);
						prog.appendInt( 0 );

						printf(" menu element accepted\n");
						accept = true;
					}	
					else
					{
						printf("left hand assignment, not accepted\n");
					}	
					break;
					

				case PT_ID:
					printf("PT_ID - ");
					if( baselist->token.lexeme )
					{
						size_t inx;
						if( prog.isLabel(baselist->token.lexeme, inx) )
						{	// a label
							prog.appendCommand(VX_LABEL);
							prog.appendInt( inx );
							accept = true;
						}
						else
						{	// a variable name
							if( baselist->token.lexeme )
							{
								// temp variable or global storage
								size_t inx;
								bool first = false;
								accept = isVariable(baselist->token.lexeme,inx);
								if( !accept )
								{	// first encounter on an undeclared variable
									// assume declaration as "auto temp"
									printf("first encounter of variable '%s', assuming type of \"auto temp\"\n", baselist->token.lexeme);
									first = true;

									insertVariable( baselist->token.lexeme, VAR_TEMP);

									accept = isVariable(baselist->token.lexeme,inx);
								}

								if(accept)
								{
									if( 0!=(flags&CFLAG_GLOBAL) && cTempVar[inx].type == VAR_TEMP)
									{	// only a temp variable but have been asked for a global one

										printf("Global Variable required: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
										accept = false;
									}
								}

								if(accept)
								{
									if( cTempVar[inx].type == VAR_TEMP )
									{	// a local temp variable 
										if( first || 0 != (flags&CFLAG_LVALUE) )
										{
											prog.appendCommand(OP_PUSH_TEMPVAR);
											prog.appendInt( cTempVar[inx].id );

											printf("Local Variable Name accepted: %s - %s (%i)\n", baselist->symbol.Name, baselist->token.lexeme, cTempVar[inx].id);

											if(first)
											{
												prog.appendCommand(OP_CLEAR);
												printf("initialisation of local variable\n");
											}
										}
										else
										{
											prog.appendCommand(OP_PUSH_TEMPVALUE);
											prog.appendInt( cTempVar[inx].id );

											printf("Local Variable Value accepted: %s - %s (%i)\n", baselist->symbol.Name, baselist->token.lexeme, cTempVar[inx].id);
										}
									}
									else
									{	// a global variable
										if( 0 != (flags&CFLAG_LVALUE) )
										{
											prog.appendCommand(OP_PUSH_VAR);
											prog.appendString( baselist->token.lexeme );

											printf("Global Variable Name accepted: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
										}
										else
										{
											prog.appendCommand(OP_PUSH_VALUE);
											prog.appendString( baselist->token.lexeme );

											printf("Global Variable Value accepted: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
										}
									}
								}
							}
							if(!accept)
							{
								printf("Error in variable statement: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
							}
						}

					}
					break;

				default:
					
					printf("Term  - ");
					printf("%s - %s\n", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");

					// accept any terminal for debug only
					//accept = true;

					break;
				}
			}
			///////////////////////////////////////////////////////////////////////
			else
			{	// nonterminals

				if( baselist->count==1 )
				{	// only one child, just go down
					accept = CompileMain(baselist->list[0], level+1, flags, prog, userval);
				}
				else
				// check the childs
				switch(baselist->symbol.idx)
				{
				///////////////////////////////////////////////////////////////////
				// a variable declaration
				case PT_VARDECL:
				{	// <Var Decl> ::= <Type> <Var List>  ';'
					// ignore <Type> here as it was done in the pre-run
					// just go into the <Var List>

					// compile the variable list
					accept = CompileMain(baselist->list[1], level+1, flags, prog, userval);
					break;
				}
				case PT_VAR:
				{	// <Var>      ::= Id <Array>
				 	//              | Id <Array> '=' <Op If>
					// basically accept this statement
					accept = true;
					bool onStack = false;

					if(baselist->list[1] && baselist->list[1]->symbol.idx==PT_ARRAY && baselist->list[1]->count>0)
					{	// compile the array resizer
						// put the target as variable, 
						// this should check for L-Values
						accept &= CompileMain(baselist->list[0], level+1, flags | CFLAG_LVALUE, prog, userval);
						// put the size argument
						accept &= CompileMain(baselist->list[1]->list[1], level+2, flags & ~CFLAG_LVALUE, prog, userval);

						printf("PT_OPRESIZE\n");
						prog.appendCommand(OP_RESIZE);

						onStack = true;
					}

					if( accept && baselist->count==4 )
					{	// assignment

						if( !onStack )
						{	// put the target as variable, 
							// this should check for L-Values
							accept &= CompileMain(baselist->list[0], level+1, flags | CFLAG_LVALUE, prog, userval);
						}
						
						// put the source
						// the result will be as single value (int, string or float) on stack
						accept &= CompileMain(baselist->list[3], level+1, flags & ~CFLAG_LVALUE, prog, userval);

						printf("PT_OPASSIGN\n");
						prog.appendCommand(OP_ASSIGN);
						onStack = true;
					}
					else
					{	// need to clear local variables but leave globals as they are
						size_t inx;
						accept = isVariable(baselist->list[0]->token.lexeme,inx);
						if( accept && cTempVar[inx].type==VAR_TEMP )
						{
							if( !onStack )
							{	// put the target as variable, 
								// this should check for L-Values
								accept &= CompileMain(baselist->list[0], level+1, flags | CFLAG_LVALUE, prog, userval);
							}
							
							prog.appendCommand(OP_CLEAR);
							printf("clear variable\n");

							onStack = true;
						}
					}

					if( onStack )
					{	// clear the stack
						prog.appendCommand(OP_POP);
						printf("clear stack\n");
					}
					break;
				}

				///////////////////////////////////////////////////////////////////
				// a label
				case PT_LABELSTM:
				{	// expecting 2 terminals in here, the first is the labelname, the second a ":"
					printf("PT_LABELSTM - ");

					size_t pos = prog.getCurrentPosition();
					int inx = prog.insertLabel( baselist->list[0]->token.lexeme, pos );
					if(inx>=0)
					{
						accept = true;
						printf("accepting label: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					else
					{
						printf("error in label statement: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// goto control statements
				case PT_GOTOSTMS:
				{	// <Goto Stms>  ::= goto Id ';'

					prog.appendCommand(OP_GOTO);
					int inx = prog.insertLabel( baselist->list[1]->token.lexeme );
					if(inx>=0)
					{
						prog.appendInt( inx );
						accept = true;
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// assignment operators
				case PT_OPASSIGN:
				{	// expecting 3 terminals in here, the first and third can be terminals or nonterminales
					// the second desides which assignment is to choose
					// check terminal count and operation type
					
					// put the target as variable, 
					// this should check for L-Values
					accept  = CompileMain(baselist->list[0], level+1, flags | CFLAG_LVALUE, prog, userval);

					// put the source
					// the result will be as single value (int, string or float) on stack
					accept &= CompileMain(baselist->list[2], level+1, flags & ~CFLAG_LVALUE, prog, userval);

					// push the command
					switch( baselist->list[1]->symbol.idx )
					{
					case PT_EQ:			prog.appendCommand(OP_ASSIGN);     break;
					case PT_PLUSEQ:		prog.appendCommand(OP_ASSIGN_ADD); break;
					case PT_MINUSEQ:	prog.appendCommand(OP_ASSIGN_SUB); break;
					case PT_TIMESEQ:	prog.appendCommand(OP_ASSIGN_MUL); break;
					case PT_DIVEQ:		prog.appendCommand(OP_ASSIGN_DIV); break;
					case PT_CARETEQ:	prog.appendCommand(OP_ASSIGN_XOR); break;
					case PT_AMPEQ:		prog.appendCommand(OP_ASSIGN_AND); break;
					case PT_PIPEEQ:		prog.appendCommand(OP_ASSIGN_OR);  break;
					case PT_GTGTEQ:		prog.appendCommand(OP_ASSIGN_RSH); break;
					case PT_LTLTEQ:		prog.appendCommand(OP_ASSIGN_LSH); break;
					}

					printf("PT_OPASSIGN - ");
					if(accept)
					{
						printf("accepting assignment %s: ", baselist->list[1]->symbol.Name, prog);
						PrintChildTerminals(baselist);
						printf("\n");
						accept = true;
					}
					else
					{
						printf("error in assignment %s: ", baselist->list[1]->symbol.Name);
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// call statement eA style without brackets
				case PT_CALLSTM:
				{	// call contains at least one parameter
					// the first must be terminal PT_ID
					if( baselist->count >0 && 
						CheckTerminal(baselist->list[0], PT_ID) )
					{	// first terminal is ok
						unsigned char command = OP_NOP;
						int id = -1;
						int paramcnt=0;

						printf("PT_CALLSTM - ");

						// go through childs now
						if( baselist->count == 3 )
						{	// ID <something> ';'
							// process the <something>, need values
							accept = CompileMain(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog, paramcnt); // need values

							// if not a call list as parameter
							if( !CheckNonTerminal(baselist->list[1], PT_CALLLIST) )
								paramcnt++;

						}
						else if( baselist->count == 2 )
						{	// ID ';'
							accept = true;
						}
						if(paramcnt<0 || paramcnt>255)
						{	
							printf("number of parameters invalid (%i) - ", paramcnt);
							accept=false;
						}
						if(accept)
						{
							id = cEnv.getFunctionID( baselist->list[0]->token.lexeme );
							if( id>=0 )
							{	// found in function table
								command = OP_CALLBUILDIN;
								if( (size_t)paramcnt < cEnv.function(id).Param() )
								{
									printf("number of parameters insufficient (%i given, %i needed) - ",paramcnt,cEnv.function(id).Param());
									accept = false;
								}
							}
							else
							{
								id = cEnv.getScriptID( baselist->list[0]->token.lexeme );
								if( id>=0 )
								{
									command = OP_CALLSCRIPT;
								}
								else
								{
									printf("Function does not exist - ");
									accept=false;
								}
							}
						}

						if(accept)
						{
							// push the command
							prog.appendCommand(command);
							// push the function ID
							prog.appendInt(id);
							prog.appendChar(paramcnt);
							printf("accepting call statement: ");
							PrintChildTerminals(baselist);
							printf(" - %i arguments", paramcnt);
							printf("\n");
						}
						else
						{
							printf("error in call statement: ");
							PrintChildTerminals(baselist);
							printf("\n");
						}

						// process the final ';' if exist
						if( CheckTerminal(baselist->list[baselist->count-1], PT_SEMI) )
							accept &= CompileMain(baselist->list[baselist->count-1], level+1,flags, prog, userval);
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// function calls
				case PT_RETVALUES:
				{
					if( baselist->count >0 && 
						CheckTerminal(baselist->list[0], PT_ID) )
					{	// first terminal is ok
						unsigned char command = OP_NOP;
						int id = -1;
						int paramcnt=0;

						printf("PT_RETVALUES - ");

						// go through childs now		
						if( baselist->count == 4 )
						{	// ID '(' <something> ')'
							// process the <something>, need values
							accept = CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, paramcnt); // need values

							// if not a expression list as parameter
							if( !CheckNonTerminal(baselist->list[2], PT_EXPRLIST) )
								paramcnt++;

						}
						else if( baselist->count == 3 )
						{	// ID '(' ')'
							accept = true;
						}
						if(paramcnt<0 || paramcnt>255)
						{	
							printf("number of parameters not allowed (%i)\n", paramcnt);
							accept=false;
						}

						if(accept)
						{
							id = cEnv.getFunctionID( baselist->list[0]->token.lexeme );
							if( id>=0 )
							{	// found in function table
								command = OP_CALLBUILDIN;
								if( (size_t)paramcnt < cEnv.function(id).Param() )
								{
									printf("number of parameters insufficient (%i given, %i needed) - ",paramcnt,cEnv.function(id).Param());
									accept = false;
								}
							}
							else
							{
								id = cEnv.getScriptID( baselist->list[0]->token.lexeme );
								if( id>=0 )
								{
									command = OP_CALLSCRIPT;
								}
								else
								{
									accept=false;
								}
							}
						}
						if(accept)
						{
							// push the command
							prog.appendCommand(command);
							// push the function ID
							prog.appendInt(id);
							prog.appendChar(paramcnt);

							printf("accepting call statement: ");
							PrintChildTerminals(baselist);
							printf(" - %i arguments", paramcnt);
							printf("\n");
						}
						else
						{
							printf("error in call statement: ");
							PrintChildTerminals(baselist);
							if( OP_NOP==command )
								printf(" - Function does not exist");
							printf("\n");
						}

						// process the final ';' if exist
						if( CheckTerminal(baselist->list[baselist->count-1], PT_SEMI) )
							accept &= CompileMain(baselist->list[baselist->count-1], level+1,flags, prog, userval);
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// comma seperated operands
				case PT_VARLIST:
				case PT_CALLLIST:
				case PT_EXPRLIST:
				{	// go through childs, spare the comma
					accept = true;
					for(i=0; i<baselist->count && accept; i++)
					{
						if( !CheckTerminal(baselist->list[i], PT_COMMA) )
						{
							accept = CompileMain(baselist->list[i], level+1,flags, prog, userval);

							if( !CheckNonTerminal(baselist->list[i], PT_CALLLIST) &&
								!CheckNonTerminal(baselist->list[i], PT_EXPRLIST) )
								userval++;
						}
					}
					break;
				}

				///////////////////////////////////////////////////////////////////
				// call statement eA style without brackets
				case PT_NORMALSTM:
				{	// can be:
					// if '(' <Expr> ')' <Normal Stm>
					// if '(' <Expr> ')' <Normal Stm> else <Normal Stm>
					// for '(' <Arg> ';' <Arg> ';' <Arg> ')' <Normal Stm>
					// while '(' <Expr> ')' <Normal Stm>
					// do <Normal Stm> while '(' <Expr> ')' ';'
					// switch '(' <Expr> ')' '{' <Case Stms> '}'
					// <Expr> ';'
					// ';'              !Null statement
					if( baselist->count <=2  )
					{	// <Expr> ';' or ';', just call the childs
						accept = true;
						for(i=0; i<baselist->count; i++)
						{
							accept = CompileMain(baselist->list[i], level+1,flags, prog, userval);
							if( !accept ) break;
						}
					}
					else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_IF) )
					{	// if '(' <Expr> ')' <Normal Stm>
						// put in <Expr>
						accept = CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval); // need the result on the stack
						printf("Conditional Jump False -> Label1\n");
						prog.appendCommand(OP_NIF);
						size_t inspos1 = prog.appendInt(0);	// placeholder
						// put in <Normal Stm>
						accept &= CompileMain(baselist->list[4], level+1,flags, prog, userval);
						printf("Label1\n");
						// calculate and insert the correct jump 
						prog.replaceInt( prog.getCurrentPosition() ,inspos1);
					}
					else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_IF) )
					{	// if '(' <Expr> ')' <Normal Stm> else <Normal Stm>
						// put in <Expr>
						accept = CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval); // need the result on the stack
						printf("Conditional Jump False -> Label1\n");
						prog.appendCommand(OP_NIF);
						size_t inspos1 = prog.appendInt(0);	// placeholder
						// put in <Normal Stm>
						accept &= CompileMain(baselist->list[4], level+1,flags, prog, userval);
						printf("Goto -> Label2\n");
						prog.appendCommand(VX_GOTO);
						size_t inspos2 = prog.appendInt(0);	// placeholder
						printf("Label1\n");
						// calculate and insert the correct jump 
						prog.replaceInt( prog.getCurrentPosition() ,inspos1);
						// put in <Normal Stm2>
						accept &= CompileMain(baselist->list[6], level+1,flags, prog, userval);
						printf("Label2\n");
						prog.replaceInt( prog.getCurrentPosition(), inspos2);			
					}
					else if( baselist->count ==9 && CheckTerminal(baselist->list[0], PT_FOR) )
					{	// for '(' <Arg> ';' <Arg> ';' <Arg> ')' <Normal Stm>
						// execute <Arg1>
						accept  = CompileMain(baselist->list[2], level+1,flags, prog, userval);
						// execute ";" to clear the stack;
						accept &= CompileMain(baselist->list[3], level+1,flags, prog, userval);
						printf("Label1\n");
						size_t tarpos1 = prog.getCurrentPosition();// position marker
						// execute <Arg2>, need a value
						accept &= CompileMain(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog, userval);

						printf("Conditional Jump False -> Label2\n");
						prog.appendCommand(OP_NIF);
						size_t inspos2 = prog.appendInt(0);	// placeholder

						// execute the loop body
						size_t rstart = prog.getCurrentPosition();
						accept &= CompileMain(baselist->list[8], level+1, flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog, userval);
						size_t rend   = prog.getCurrentPosition();

						size_t tarpos3 = prog.getCurrentPosition();// position marker
						// execute the incrementor <Arg3>
						accept &= CompileMain(baselist->list[6], level+1,flags, prog, userval);
						// execute ";" to clear the stack;
						accept &= CompileMain(baselist->list[5], level+1,flags, prog, userval);

						printf("Goto -> Label1\n");
						size_t srcpos1=prog.getCurrentPosition();
						prog.appendCommand(VX_GOTO);
						prog.appendInt(tarpos1);

						printf("Label2\n");
						size_t tarpos2 = prog.getCurrentPosition();
						prog.replaceInt( tarpos2 ,inspos2);

						// convert break -> goto Label2
						// convert continue -> goto Label1
						prog.replaceJumps(rstart,rend,VX_BREAK,tarpos2);
						prog.replaceJumps(rstart,rend,VX_CONT,tarpos3);

					}
					else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_WHILE) )
					{	// while '(' <Expr> ')' <Normal Stm>

						printf("Label1\n");
						size_t tarpos1 = prog.getCurrentPosition();// position marker

						// execute <Expr>
						accept  = CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

						printf("Conditional Jump False -> Label2\n");
						prog.appendCommand(OP_NIF);
						size_t inspos2 = prog.appendInt(0);	// placeholder offset

						// execute <Normal Stm>
						size_t rstart = prog.getCurrentPosition();
						accept &= CompileMain(baselist->list[4], level+1,flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog, userval);
						size_t rend = prog.getCurrentPosition();

						printf("Goto -> Label1\n");
						prog.appendCommand(VX_GOTO);
						prog.appendInt(tarpos1);

						printf("Label2\n");
						size_t tarpos2 = prog.getCurrentPosition();
						prog.replaceInt( tarpos2 ,inspos2);
						// convert break -> goto Label2
						// convert continue -> goto Label1
						prog.replaceJumps(rstart,rend,VX_BREAK,tarpos2);
						prog.replaceJumps(rstart,rend,VX_CONT,tarpos1);

					}
					else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_DO) )
					{	// do <Normal Stm> while '(' <Expr> ')' ';'

						printf("Label1\n");
						size_t tarpos1 = prog.getCurrentPosition();// position marker

						// execute <Normal Stm>
						size_t rstart = prog.getCurrentPosition();
						accept  = CompileMain(baselist->list[1], level+1,flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog, userval);
						size_t rend = prog.getCurrentPosition();

						// execute <Expr>
						accept &= CompileMain(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog, userval);
						printf("Conditional Jump True -> Label1\n");
						prog.appendCommand(OP_IF);
						prog.appendInt(tarpos1);

						size_t tarpos2 = prog.getCurrentPosition();
						accept &= CompileMain(baselist->list[6], level+1,flags & ~CFLAG_LVALUE, prog, userval);

						// convert break -> goto Label2
						// convert continue -> goto Label1
						prog.replaceJumps(rstart,rend,VX_BREAK,tarpos2);
						prog.replaceJumps(rstart,rend,VX_CONT,tarpos1);

					}
					else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_SWITCH) )
					{	// switch '(' <Expr> ')' '{' <Case Stms> '}'
						// <Case Stms> ::= case <Value> ':' <Stm List> <Case Stms>
						//               | default ':' <Stm List> <Case Stms>

						char varname[128];
						size_t inx;
						snprintf(varname, 128,"_#casetmp%04i", prog.getCurrentPosition());
						insertVariable( varname, VAR_TEMP);
						accept = isVariable(varname,inx);
						if( accept )
						{
							inx = cTempVar[inx].id;

							printf("create temporary variable %s (%i)\n", varname, inx);

							prog.appendCommand(OP_PUSH_TEMPVAR);
							prog.appendInt( cTempVar[inx].id );

							printf("push temporary variable %s (%i)\n", varname, inx);

							// compile <Expr>
							accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);
							prog.appendCommand(OP_ASSIGN);
							printf("OP_ASSIGN\n", varname, inx);
							// save the result from <Expr> into the temp variable
						}

						// process the case statements
						TArrayDCT<_stmlist> stmlist;
						int hasdefault=-1;

						if( accept )
						{	// use a if-else-if struture to process the case; 
							// but first reorder the tree to a list
							struct _list *casestm = baselist->list[5];
							while(casestm && casestm->symbol.idx==PT_CASESTMS)
							{
								if( casestm->count==5 )
								{	// case <Value> ':' <Stm List> <Case Stms>
									stmlist.append( _stmlist(casestm->list[1], casestm->list[3]) );
									casestm = casestm->list[4];
								}
								else if( casestm->count==4 )
								{	//default ':' <Stm List> <Case Stms>

									if(hasdefault>=0)
									{
										printf("error: multiple case marker specified\n");
										accept = false;
										break;
									}
									hasdefault=stmlist.size();

									stmlist.append( _stmlist(NULL,casestm->list[2]) );
									casestm = casestm->list[3];
								}
								else
									casestm = NULL;					
							}
						}

						size_t defpos=0;
						if( accept )
						{	// go through the list and build the if-else-if

							for(i=0; i<stmlist.size()&& accept; i++)
							{
								if( stmlist[i].value )
								{	// normal case statements, not the default case

									// push the temprary variable
									prog.appendCommand(OP_PUSH_TEMPVALUE);
									prog.appendInt( inx );
									printf("push temporary variable %s (%i)\n", varname, inx);

									// push the case value
									accept = CompileMain(stmlist[i].value, level+1,flags, prog, userval);

									printf("PT_EQUAL\n");
									prog.appendCommand(OP_EQUATE);
					
									printf("Conditional Jump True -> CaseLabel%i\n", i);
									prog.appendCommand(OP_IF);
									stmlist[i].gotomarker = prog.appendInt(0);	// placeholder
								}
							}
							printf("Goto -> LabelEnd/Default\n");
							prog.appendCommand(VX_GOTO);
							defpos = prog.appendInt(0);	// placeholder
							if(hasdefault>=0)
								stmlist[hasdefault].gotomarker = defpos;
						}
						if(accept)
						{	// build the body of the case statements
							size_t address;
							size_t rstart = prog.getCurrentPosition();
							for(i=0; i<stmlist.size()&& accept; i++)
							{
								if( stmlist[i].stm )
								{	
									// fill in the target address
									address = prog.getCurrentPosition();
									if( stmlist[i].gotomarker>0 )
										prog.replaceInt(address, stmlist[i].gotomarker);

									// compile the statement
									accept = CompileMain(stmlist[i].stm, level+1,flags | CFLAG_USE_BREAK, prog, userval);
								}
							}
							size_t rend = prog.getCurrentPosition();

							if(hasdefault<0)
							{	// no default case, so redirect the last goto to the end
								prog.replaceInt(rend, defpos);
							}
							// convert break -> goto REND
							prog.replaceJumps(rstart,rend,VX_BREAK, rend );
						}
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// break/continue control statements
				case PT_LCTRSTMS:
				{	// 'break' ';'
					// 'continue' ';'
					if( (baselist->list[0] && baselist->list[0]->symbol.idx == PT_CONTINUE) && (0!=(flags & CFLAG_USE_CONT)) )
					{
						prog.appendCommand(VX_CONT);
						prog.appendInt( 0 );
						
						accept = true;
					}
					else if( (baselist->list[0] && baselist->list[0]->symbol.idx == PT_BREAK) && (0!=(flags & CFLAG_USE_BREAK)) )
					{
						prog.appendCommand(VX_BREAK);
						prog.appendInt( 0 );

						accept = true;
					}
					if(!accept)
						printf("keyword '%s' not allowed in this scope\n", baselist->list[0]->token.lexeme);
					break;
				}

				///////////////////////////////////////////////////////////////////
				// operands
				case PT_OPADDSUB:
				{	// <Op AddSub> '+' <Op MultDiv>
					// <Op AddSub> '-' <Op MultDiv>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_PLUS:
						printf("PT_PLUS\n");
						prog.appendCommand(OP_ADD);
						break;
					case PT_MINUS:
						printf("PT_MINUS\n");
						prog.appendCommand(OP_SUB);
						break;
					}// end switch
					break;
				}
				case PT_OPMULTDIV:
				{	// <Op MultDiv> '*' <Op Unary>
					// <Op MultDiv> '/' <Op Unary>
					// <Op MultDiv> '%' <Op Unary>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_TIMES:
						printf("PT_TIMES\n");
						prog.appendCommand(OP_MUL);
						break;
					case PT_DIV:
						printf("PT_DIV\n");
						prog.appendCommand(OP_DIV);
						break;
					case PT_PERCENT:
						printf("PT_PERCENT\n");
						prog.appendCommand(OP_MOD);
						break;
					}// end switch
					break;
				}
				case PT_OPAND:
				{	// <Op And> '&&' <Op BinOR>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					if( baselist->list[1]->symbol.idx == PT_AMPAMP )
					{
						printf("PT_OPAND\n");
						prog.appendCommand(OP_LOG_AND);
					}
					break;
				}
				case PT_OPOR:
				{	// <Op Or> '||' <Op And>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					if( baselist->list[1]->symbol.idx == PT_PIPEPIPE )
					{
						printf("PT_PIPEPIPE\n");
						prog.appendCommand(OP_LOG_OR);
					}
					break;
				}

				case PT_OPBINAND:
				{	// <Op BinAND> '&' <Op Equate>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					if( baselist->list[1]->symbol.idx == PT_AMP )
					{
						printf("PT_OPBINAND\n");
						prog.appendCommand(OP_BIN_AND);
					}
					break;
				}
				case PT_OPBINOR:
				{	// <Op BinOr> '|' <Op BinXOR>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					if( baselist->list[1]->symbol.idx == PT_PIPE )
					{
						printf("PT_OPBINOR\n");
						prog.appendCommand(OP_BIN_OR);
					}
					break;
				}
				case PT_OPBINXOR:
				{	// <Op BinXOR> '^' <Op BinAND>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					if( baselist->list[1]->symbol.idx == PT_CARET )
					{
						printf("PT_OPBINOR\n");
						prog.appendCommand(OP_BIN_XOR);
					}
					break;
				}
				case PT_OPCAST:
				{	// '(' <Type> ')' <Op Unary>

					// put the operands on stack, first the value then the target type
					accept  = CompileMain(baselist->list[3], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					printf("PT_OPCAST\n");
					prog.appendCommand(OP_CAST);
					break;
				}
				case PT_OPCOMPARE:
				{	// <Op Compare> '>'  <Op Shift>
					// <Op Compare> '>=' <Op Shift>
					// <Op Compare> '<'  <Op Shift>
					// <Op Compare> '<=' <Op Shift>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_GT:
						printf("PT_GT\n");
						prog.appendCommand(OP_ISGT);
						break;
					case PT_GTEQ:
						printf("PT_GTEQ\n");
						prog.appendCommand(OP_ISGTEQ);
						break;
					case PT_LT:
						printf("PT_LT\n");
						prog.appendCommand(OP_ISLT);
						break;
					case PT_LTEQ:
						printf("PT_LTEQ\n");
						prog.appendCommand(OP_ISLTEQ);
						break;
					}// end switch
					break;
				}
				
				case PT_OPEQUATE:
				{	// <Op Equate> '==' <Op Compare>
					// <Op Equate> '!=' <Op Compare>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_EQEQ:
						printf("PT_EQUAL\n");
						prog.appendCommand(OP_EQUATE);
						break;
					case PT_EXCLAMEQ:
						printf("PT_UNEQUAL\n");
						prog.appendCommand(OP_UNEQUATE);
						break;
					}// end switch
					break;
				}
				case PT_OPIF:
				{	// <Op Or> '?' <Op If> ':' <Op If>
					// => if( or ) opif1 else opif2
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					printf("Conditional Jump False -> Label1\n");
					prog.appendCommand(OP_NIF);
					size_t inspos1 = prog.appendInt(0);	// placeholder
					// put in <Op If1>
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					printf("Goto -> Label2\n");
					prog.appendCommand(VX_GOTO);
					size_t inspos2 = prog.appendInt(0);	// placeholder
					printf("Label1\n");
					// calculate and insert the correct jump offset
					prog.replaceInt( prog.getCurrentPosition() ,inspos1);
					// put in <Op If2>
					accept &= CompileMain(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					printf("Label2\n");
					prog.replaceInt( prog.getCurrentPosition() ,inspos2);

					printf("PT_SELECT\n");
					break;
				}
				case PT_OPPOINTER:
				{	// <Op Pointer> '.' Id             ! member variable
					// <Op Pointer> '.' <RetValues>    ! member function
					// <Op Pointer> '[' <Expr> ']'     ! array

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_DOT:
						// put the operands on stack
						accept  = CompileMain(baselist->list[0], level+1,flags | CFLAG_LVALUE|CFLAG_GLOBAL, prog, userval);	// base variable
						
						if( CheckTerminal(baselist->list[2], PT_ID) )
						{	// have to select the correct variable inside base according to the member name
							// and put this variable on the stack

							// need to check if member variable is valid
							accept &= CompileMain(baselist->list[2], level+1,flags | CFLAG_LVALUE|CFLAG_GLOBAL, prog, userval);	// member variable

							prog.appendCommand(OP_MEMBER);
							printf("PT_MEMBERACCESS variable %s\n", accept?"ok":"failed");
						}
						else if( CheckNonTerminal(baselist->list[2], PT_RETVALUES) )
						{
							accept &= CompileMain(baselist->list[2], level+1,flags, prog, userval);	// member function
							printf("PT_MEMBERACCESS function %s\n", accept?"ok":"failed");
						}
						else
						{
							printf("PT_MEMBERACCESS failed\n");
						}
						
						break;
					case PT_LBRACKET:
						// put the operands on stack
						accept  = CompileMain(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog, userval);	// variable
						accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);	// index

						prog.appendCommand(OP_ARRAY);
						printf("PT_ARRAYACCESS %s %s\n", (flags)?"variable":"value", (accept)?"successful":"failed");
						break;
					}// end switch
					break;
				}
				case PT_OPPOST:
				{	// <Op Pointer> '++'
					// <Op Pointer> '--'

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog, userval); // put the variable itself on stack

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_PLUSPLUS:
						printf("PT_OPPOST_PLUSPLUS, \n");
						prog.appendCommand(OP_POSTADD);
						break;
					case PT_MINUSMINUS:
						printf("PT_OPPOST_MINUSMINUS\n");
						prog.appendCommand(OP_POSTSUB);
						break;
					}// end switch

					break;
				}
				case PT_OPPRE:
				{	// '++'   <Op Unary>
					// '--'   <Op Unary>

					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog, userval); // put the variable itself on stack

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_PLUSPLUS:
						printf("PT_OPPRE_PLUSPLUS, \n");
						prog.appendCommand(OP_PREADD);
						break;
					case PT_MINUSMINUS:
						printf("PT_OPPRE_MINUSMINUS\n");
						prog.appendCommand(OP_PRESUB);
						break;
					}// end switch

					break;
				}
				case PT_OPSHIFT:
				{	// <Op Shift> '<<' <Op AddSub>
					// <Op Shift> '>>' <Op AddSub>
					// put the operands on stack
					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					accept &= CompileMain(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog, userval);

					switch( baselist->list[1]->symbol.idx )
					{
					case PT_GTGT:
						printf("PT_GTGT, \n");
						prog.appendCommand(OP_RSHIFT);
						break;
					case PT_LTLT:
						printf("PT_LTLT\n");
						prog.appendCommand(OP_LSHIFT);
						break;
					}// end switch

					break;
				}
				case PT_OPSIZEOF:
				{	// sizeof '(' <Type> ')'
					// sizeof '(' Id ')'

					// put the operands on stack
					switch( baselist->list[2]->symbol.idx )
					{
					case PT_ID:
						accept  = CompileMain(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog, userval); // put the variable itself on stack
						prog.appendCommand(OP_SIZEOF);
						printf("PT_OPSIZEOF ID, \n");
						break;
					default:
						prog.appendCommand(OP_PUSH_INT);
						prog.appendInt( 1 );
						printf("PT_OPSIZEOF TYPE, put the corrosponding value on stack\n");
						break;
					}// end switch

					break;
				}
				case PT_OPUNARY:
				{	// '!'    <Op Unary>
					// '~'    <Op Unary>
					//'-'    <Op Unary>

					accept  = CompileMain(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					// put the operands on stack
					switch( baselist->list[2]->symbol.idx )
					{
					case PT_EXCLAM:
						printf("PT_OPUNARY_NOT\n");
						prog.appendCommand(OP_NOT);
						break;
					case PT_TILDE:
						printf("PT_OPUNARY_INVERT\n");
						prog.appendCommand(OP_INVERT);
						break;
					case PT_MINUS:
						printf("PT_OPUNARY_NEGATE\n");
						prog.appendCommand(OP_NEGATE);
						break;
					}// end switch
					break;
				}
				case PT_VALUE:
				{	// '(' <Expr> ')'
					accept  = CompileMain(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog, userval);
					break;
				}

				///////////////////////////////////////////////////////////////////
				// statements with no own handler that just go through their childs
				case PT_STMLIST:
				case PT_BLOCK:
				{	// check if childs are accepted
					accept = true;
					for(i=0; i<baselist->count; i++)
					{
						accept = CompileMain(baselist->list[i], level+1,flags, prog, userval);
						if( !accept ) break;
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// default prints unhandled statements
				default:
				{
					printf("NTerm - ");
					printf("%s - %s\n", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");

					// accept non-terminal but go through their childs
					accept = true;
					for(i=0; i<baselist->count; i++)
					{
						accept = CompileMain(baselist->list[i], level+1,flags, prog, userval);
						if( !accept ) break;
					}
					break;
				}
				}// switch
			}
			///////////////////////////////////////////////////////////////////////
		}
		fflush(stdout);
		return accept;
	}
	///////////////////////////////////////////////////////////////////////////
	// compile variable type declaration
	bool CompileVarType(struct _list *baselist, vartype &type, int &num)
	{
		size_t i;
		bool accept = false;
		if( baselist)
		{
			if( baselist->symbol.Type==1 )
			{	// terminals
				switch( baselist->symbol.idx )
				{
				case PT_AUTO:
				case PT_DOUBLE:
				case PT_STRING:
				case PT_INT:
					num = baselist->symbol.idx;
					accept = true;
					break;
				case PT_TEMP:
					type = VAR_TEMP;
					accept = true;
					break;
				case PT_GLOBAL:
					type = VAR_GSERVER;
					accept = true;
					break;
				}
			}
			else
			{	// nonterminal
				for(i=0; i<baselist->count && accept; i++)
					accept =CompileVarType(baselist->list[i], type, num);

			}
		}
		return accept;
	}

	///////////////////////////////////////////////////////////////////////////
	// compile the variable declaration
	bool CompileVarList(struct _list *baselist, size_t level, unsigned long flags, CProgramm& prog, int& userval, vartype type)
	{	// <Var List> ::= <Var> ',' <Var List>
		//              | <Var>

		bool accept = false;
		if(baselist)
		{
			struct _list *var;
			if( baselist->symbol.idx==PT_VARLIST )
				var = baselist->list[0];
			else 
				var = baselist;	
			if(var && var->symbol.idx==PT_VAR)
			{
				if( var->count>0 )
				{	// <Var>  ::= Id <Array>
					//			| Id <Array> '=' <Op If>
					// only do the id here
					if( var->list[0]->symbol.idx==PT_ID )
					{
						size_t id;
						if( isVariable(var->list[0]->token.lexeme, id) )
						{
							printf( "variable '%s'already defined\n", var->list[0]->token.lexeme);
						}
						else
						{
							insertVariable( var->list[0]->token.lexeme, type);
							accept = true;
							printf( "accepting variable '%s' of type %i\n", var->list[0]->token.lexeme, type);
						}
					}
				}
			}
			if( baselist->count==3 )
			{	// var ',' varlist
				// go into "varlist"
				accept &= CompileVarList(baselist->list[2], level, flags, prog, userval, type);
			}
		}
		return accept;
	}
	///////////////////////////////////////////////////////////////////////////
	// initial pre-run through the tree to find necesary elements
	bool CompileLabels(struct _list *baselist, size_t level, unsigned long flags, CProgramm& prog, int& userval)
	{
		size_t i;
		bool accept = false;
		if(baselist)
		{
			///////////////////////////////////////////////////////////////////
			if( baselist->symbol.Type==1 )
			{	// terminals
				accept = true;
			}
			///////////////////////////////////////////////////////////////////
			else
			{	// nonterminals

				if( baselist->count==1 )
				{	// only one child, just go down
					accept = CompileLabels(baselist->list[0], level+1, flags, prog, userval);
				}
				else
				// check the childs
				switch(baselist->symbol.idx)
				{
				///////////////////////////////////////////////////////////////////
				// a variable declaration
				case PT_VARDECL:
				{	// <Var Decl> ::= <Type> <Var List>  ';'
				 	// <Type>     ::= <Mod> <Scalar>
				 	//              | <Mod> 
				 	//              |       <Scalar>
				 	// <Mod>      ::= global
				 	//              | temp
				 	// <Scalar>   ::= string
				 	//              | double
				 	//              | int
				 	//              | auto

					vartype type = VAR_TEMP;
					int num=0;	// variable storage type, ignored for now
					// find all subterminals in first subtree
					if( CompileVarType(baselist->list[0], type, num) )
					{
						// compile the variable list
						accept = CompileVarList(baselist->list[1], level+1, flags, prog, userval, type);				
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// a label
				case PT_LABELSTM:
				{	// expecting 2 terminals in here, the first is the labelname, the second a ":"
					int inx = prog.insertLabel( baselist->list[0]->token.lexeme);
					if(inx>=0)
					{
						accept = true;
						printf("accepting label: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					else
					{
						printf("error in label statement: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;
				}
				///////////////////////////////////////////////////////////////////
				// default prints unhandled statements
				default:
				{
					// accept non-terminal but go through their childs
					accept = true;
					for(i=0; i<baselist->count; i++)
					{
						accept = CompileLabels(baselist->list[i], level+1,flags, prog, userval);
						if( !accept ) break;
					}
					break;
				}
				}// switch
			}
			///////////////////////////////////////////////////////////////////////
		}
		fflush(stdout);
		return accept;
	}
	///////////////////////////////////////////////////////////////////////////
	// builds name and id from a ExName node
	bool CompileScriptExNameID(struct _list *nameid, char name[])
	{
		bool ret = false;
		if( nameid && nameid->symbol.idx==PT_EXNAMEID)
		{	// inside a <exName Id>
			size_t i;
			for(i=0; i<nameid->count; i++)
			{
				if( nameid->list[i]->symbol.idx==PT_ID || nameid->list[i]->symbol.idx==PT_APOID)
				{
					strcat(name, nameid->list[i]->token.lexeme );
					strcat(name, " ");
					ret = true;
				}
				else if( nameid->count > 1 && nameid->list[i]->symbol.idx==PT_EXNAMEID)
					ret &= CompileScriptExNameID(nameid->list[i], name);
				else
				{
					ret = false;
					break;
				}
			}
		}
		return ret;
	}
	///////////////////////////////////////////////////////////////////////////
	// generate name and id from a NameId node
	bool CompileScriptNameID(struct _list *baselist, char id[], char name[])
	{
		bool ret=false;
		name[0]=0;
		id[0]=0;

		if(baselist && ( baselist->symbol.idx== PT_EXNAMEID || baselist->symbol.idx== PT_NAMEID) )
		{
			struct _list *nameid=baselist;
			// go into the first
			if( nameid->count==3 && nameid->symbol.idx==PT_NAMEID )
			{
				if( nameid->list[2]->symbol.idx==PT_ID )
					strcpy(id,nameid->list[2]->token.lexeme);	// terminal ID

				if( nameid->list[0]->symbol.idx==PT_EXNAMEID)
					nameid=nameid->list[0];						// nonterminal <exName Id>
				else
					nameid=NULL;
			}
			else if(nameid->count==1 && nameid->symbol.idx==PT_NAMEID)
			{	// non-collapsed parse tree
				nameid=nameid->list[0];
			}
			
			CompileScriptExNameID(nameid,name);

			if(strlen(name)>0)
				name[strlen(name)-1]=0;

			if( baselist->symbol.idx != PT_NAMEID || baselist->count!=3 )
			{	// identical name and id 
				strcpy(id,name);
			}
		}
		return ret;
	}
	///////////////////////////////////////////////////////////////////////////
	// generate the sprite number from a SpriteId node
	unsigned short CompileScriptSpriteID(struct _list *spriteid)
	{
		if( spriteid && spriteid->count==1 && spriteid->symbol.idx==PT_SPRITEID)
			return atoi(spriteid->list[0]->token.lexeme );
		else if( spriteid && spriteid->symbol.idx==PT_DECLITERAL)
			return atoi(spriteid->token.lexeme );
		else
			return 0xFFFF;
			
	}
	///////////////////////////////////////////////////////////////////////////
	// generate a filename from a FileId node
	bool CompileScriptFileID(struct _list *fileid, char map[])
	{
		if( fileid->count==3 && fileid->symbol.idx==PT_FILEID )
		{
			strcpy(map, fileid->list[0]->token.lexeme);
			strcat(map, ".");
			strcat(map, fileid->list[2]->token.lexeme);
			return true;
		}
		else
		{
			map[0]=0;
			return false;
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// compile headers of ea script syntax
	bool CompileScriptHeader(struct _list *baselist, char id[], char name[], char map[], unsigned short &x, unsigned short &y, unsigned char &dir, unsigned short &sprite)
	{	// '-' 'script' <Name Id> <Sprite Id> ',' <Block>
		// 'function' 'script' <Name Id> <Block>
		// <File Id> ',' DecLiteral ',' DecLiteral ',' DecLiteral 'script' <Name Id> <Sprite Id> ',' <Block>
		// 
		// <Name Id>   ::= <exName Id>
		//               | <exName Id> '::' Id
		// 
		// <exName Id> ::= Id
		//               | ApoId
		//               | Id <exName Id> 
		//               | ApoId <exName Id> 
		// <Sprite Id> ::= DecLiteral
		//               | '-' DecLiteral
		// 
		// <File Id>   ::= Id '.' Id

		bool ret = false;
		if(baselist)
		{
			if( baselist->count==6  && CheckTerminal(baselist->list[1], PT_SCRIPT) )
			{	// version 1
				map[0]=0;
				x=y=0xFFFF;
				dir=0;
				sprite = CompileScriptSpriteID(baselist->list[3]);
				ret = CompileScriptNameID( baselist->list[2], id, name);
			}
			else if( baselist->count==4  && CheckTerminal(baselist->list[1], PT_SCRIPT) )
			{	// version 2
				map[0]=0;
				x=y=0xFFFF;
				dir=0;
				sprite = 0xFFFF;
				ret = CompileScriptNameID( baselist->list[2], id, name);
			}
			else if(baselist->count==12 && CheckTerminal(baselist->list[7], PT_SCRIPT) )
			{	// version 3
				ret = CompileScriptFileID( baselist->list[0], map);
				ret&= CompileScriptNameID( baselist->list[8], id, name);
				x		= atoi( baselist->list[2]->token.lexeme );
				y		= atoi( baselist->list[4]->token.lexeme );
				dir		= atoi( baselist->list[6]->token.lexeme );
				sprite	= CompileScriptSpriteID(baselist->list[9]);			
			}
		}
		return ret;
	}
	///////////////////////////////////////////////////////////////////////////
	// counting number of parameters in nonterminal DParams and Params
	size_t CompileParams(struct _list *baselist)
	{	
		size_t cnt = 0, i;

		if( CheckNonTerminal(baselist, PT_DPARAMS) ||
			CheckNonTerminal(baselist, PT_PARAMS) )
		{	// go through childs, spare the comma
			for(i=0; i<baselist->count; i++)
			{
				if( !CheckTerminal(baselist->list[i], PT_COMMA) )
				{
					cnt += CompileParams(baselist->list[i]);

					if( !CheckNonTerminal(baselist->list[i], PT_DPARAMS) &&
						!CheckNonTerminal(baselist->list[i], PT_PARAMS) )
						cnt++;
				}
			}
		}
		return cnt;
	}

	///////////////////////////////////////////////////////////////////////////
	// compiler entry point
public:
	bool CompileTree(struct _list *baselist)
	{
		size_t i;
		bool accept = false;
		if(baselist)
		{
			///////////////////////////////////////////////////////////////////////
			
			if( baselist->symbol.Type==1 )
			{	// terminals
				
				// generally not accepted here
			}
			///////////////////////////////////////////////////////////////////////
			else
			{	// nonterminals
				switch(baselist->symbol.idx)
				{
				///////////////////////////////////////////////////////////////////
				// 
				case PT_FUNCPROTO:
				{	// ::= <Scalar> Id '(' <DParams>  ')' ';'
					//	 | <Scalar> Id '(' <Params> ')' ';'
					//	 | <Scalar> Id '(' ')' ';'

					// count parameters (will return 0 when using an incorrect nonterminal, which is correct)
					size_t cnt = CompileParams(baselist->list[3]);
					size_t id  = cEnv.addFunction( baselist->list[1]->token.lexeme, cnt);
					if( id>=0 )
					{
						printf("function declaration: ");
						PrintChildTerminals(baselist);
						printf(" (id=%i)\n",id);
						accept = true;
					}
					break;
				}
				case PT_BLOCK:
				{	// single <Block> without header (for small scripts)
					int userval;
					CProgramm prog;

					cTempVar.clear();

					prog.appendCommand(OP_START);
					size_t sizepos=prog.appendInt(0);
					if( CompileLabels(baselist, 0, 0, prog, userval) )
						accept = CompileMain(baselist, 0, 0, prog, userval);
					prog.appendCommand(OP_END);
					prog.replaceInt(prog.getCurrentPosition(),sizepos);
					prog.ConvertLabels();

					printf("\n");
					prog.dump();
					printf("\n");

					if( accept )
					{
						printf("accept block: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					else
					{
						printf("failed block: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;

				}
				case PT_FUNCDECL:
				{	// <Scalar> Id '(' <Params>  ')' <Block>
					// <Scalar> Id '(' ')' <Block>

					if( CheckTerminal(baselist->list[1], PT_ID) )
					{
						// count parameters (will return 0 when using an incorrect nonterminal, which is correct)
						size_t cnt = CompileParams(baselist->list[3]);
						// generate a new function script
						int pos = cEnv.addFunction( baselist->list[1]->token.lexeme, cnt );
						if( pos>=0 )
						{	// compile the <block>
							int userval;
							CProgramm &prog = cEnv.function(pos);

							cTempVar.clear();

							prog.appendCommand(OP_START);
							size_t sizepos=prog.appendInt(0);
							if( CompileLabels(baselist, 0, 0, prog, userval) )
								accept = CompileMain(baselist->list[baselist->count-1], 0, 0, prog, userval);
							prog.appendCommand(OP_END);
							prog.replaceInt(prog.getCurrentPosition(),sizepos);
							prog.ConvertLabels();


							printf("\n");
							prog.dump();
							printf("\n");
						}

					}

					if( accept )
					{
						printf("accept function: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					else
					{
						printf("failed function: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;

				}
				case PT_SCRIPTDECL:
				{	// '-' 'script' <Name Id> <Sprite Id> ',' <Block>
					// 'function' 'script' <Name Id> <Block>
					// <File Id> ',' DecLiteral ',' DecLiteral ',' DecLiteral 'script' <Name Id> <Sprite Id> ',' <Block>


					if( (baselist->count==6  && CheckTerminal(baselist->list[1], PT_SCRIPT)) ||
						(baselist->count==4  && CheckTerminal(baselist->list[1], PT_SCRIPT)) ||
						(baselist->count==12 && CheckTerminal(baselist->list[7], PT_SCRIPT))  )
					{
						char name[128], id[128], map[128];
						unsigned short x,y,sprite;
						unsigned char dir;
						CompileScriptHeader(baselist,id, name, map, x, y, dir, sprite);

						// generate a new script
						int pos= cEnv.addScript(id, name, map, x, y, dir, sprite);
						if( pos>=0 )
						{	// compile the <block>
							int userval;
							CProgramm &prog = cEnv.script(pos);

							cTempVar.clear();

							prog.appendCommand(OP_START);
							size_t sizepos=prog.appendInt(0);
							if( CompileLabels(baselist, 0, 0, prog, userval) )
								accept = CompileMain(baselist->list[baselist->count-1], 0, 0, prog, userval);
							prog.appendCommand(OP_END);
							prog.replaceInt(prog.getCurrentPosition(),sizepos);
							prog.ConvertLabels();


							printf("\n");
							prog.dump();
							printf("\n");
						}
					}

					if( accept )
					{
						printf("accept script: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					else
					{
						printf("failed script: ");
						PrintChildTerminals(baselist);
						printf("\n");
					}
					break;
				}

				case PT_DECLS:
				{	// go through all declarations
					for(i=0; i<baselist->count; i++)
					{
						accept = CompileTree(baselist->list[i]);
						if( !accept ) break;
					}
					break;
				}
				default:
				{	
					PrintChildTerminals(baselist); printf(" not allowed on this scope\n");
					break;
				}
				}// switch
			}
			///////////////////////////////////////////////////////////////////////
		}
		return accept;
	}



};


void startcompile(struct _parser* parser)
{
	struct _list baselist;
	memset(&baselist,0,sizeof(struct _list));

	CScriptEnvironment env;
	CScriptCompiler compiler(env);


	reduce_tree(parser, 0, &baselist,0);
fflush(stdout);
	print_listtree(baselist.list[0],0);
fflush(stdout);
	compiler.CompileTree(baselist.list[0]);
fflush(stdout);
	printf("\nready\n");

/*
	size_t i;

	memset(programm,0,sizeof(programm));
	progcnt=0;
	memset(constants,0,sizeof(constants));
	constcnt=0;


	compile(parser,0,0, NULL);

	printf("(%i)-----------------------------------\n", progcnt);
	for(i=0; i<progcnt; i++)
		printf("%i - op(%i)\n", i, programm[i].type);

*/
}