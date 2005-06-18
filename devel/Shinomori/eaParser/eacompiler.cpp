
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
	char*		string;

	void copy(const char *in)
	{	clear();
		if(in) 
		{	string=new char[strlen(in)+1];
			if(string)
				strcpy(string, in);
		}
	}
	void clear()
	{	if(string)
		{	delete[] string;
			string=NULL;
		}
	}

public:
	MiniString(const char *in=NULL)	{string=NULL; copy(in);}
	MiniString(const MiniString &in){string=NULL; copy(in.string);}
	virtual ~MiniString()			{clear();}

	const MiniString &operator=(const MiniString &a)	{ copy(a.string); return *this; }
	const char*	operator=(const char *in)				{ copy(in); return string; }
	const char* get()									{ return string; }
	operator char*()									{ return string; }
	operator const char*() const						{ return string; }

	bool operator==(const char *b) const		{return (0==strcmp(this->string, b));}
	bool operator==(const MiniString &b) const	{return (0==strcmp(this->string, b.string));}
	bool operator!=(const char *b) const		{return (0!=strcmp(this->string, b));}
	bool operator!=(const MiniString &b) const	{return (0!=strcmp(this->string, b.string));}
	bool operator> (const char *b) const		{return (0> strcmp(this->string, b));}
	bool operator> (const MiniString &b) const	{return (0> strcmp(this->string, b.string));}
	bool operator< (const char *b) const		{return (0< strcmp(this->string, b));}
	bool operator< (const MiniString &b) const	{return (0< strcmp(this->string, b.string));}
	bool operator>=(const char *b) const		{return (0>=strcmp(this->string, b));}
	bool operator>=(const MiniString &b) const	{return (0>=strcmp(this->string, b.string));}
	bool operator<=(const char *b) const		{return (0<=strcmp(this->string, b));}
	bool operator<=(const MiniString &b) const	{return (0<=strcmp(this->string, b.string));}

	friend bool operator==(const char *a, const MiniString &b) {return (0==strcmp(a, b.string));}
	friend bool operator!=(const char *a, const MiniString &b) {return (0!=strcmp(a, b.string));}
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
	// debug
	void dump()
	{
		size_t i;
		for(i=0; i<cProgramm.size(); i++)
		{
			printf("%3i ", cProgramm[i]);
			if(15==i%16) printf("\n");
		}
	}


	///////////////////////////////////////////////////////////////////////////
	// label functions
	const char* insertLabel(const char* name, int pos=-1)
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
			return (const char*)(cLabelList[inx]);
		}
		return NULL;
	}
	int getLabel(const char* name)
	{
		size_t inx;
		if( cLabelList.find(name,0, inx) )
		{
			return cLabelList[inx].pos;
		}
		return -1;
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
					replaceInt(cmd,pos);
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
				case OP_PUSH_INT:
					start += 4;
					break;
				// commands followed by a string (pointer size)
				case OP_GOTO:
				case OP_PUSH_VAR:
				case OP_PUSH_VALUE:
				case OP_PUSH_STRING:
					start += sizeof(size_t);
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
		cProgramm.insert(val, inx++);
		return inx;
	}
	size_t replaceCommand(unsigned char val, size_t &inx)
	{
		cProgramm[inx++] = val;
		return inx;
	}
	size_t appendCommand(unsigned char val)
	{
		size_t pos = cProgramm.size();
		cProgramm.append(val);
		return pos;
	}

	unsigned char getChar(size_t &inx)
	{
		if( inx < cProgramm.size() )
			return cProgramm[inx++];
		return 0;
	}
	size_t insertChar(unsigned char val, size_t &inx)
	{
		cProgramm.insert(val, inx++);
		return inx;
	}
	size_t replaceChar(unsigned char val, size_t &inx)
	{
		cProgramm[inx++] = val;
		return inx;
	}
	size_t appendChar(unsigned char val)
	{
		size_t pos = cProgramm.size();
		cProgramm.append(val);
		return pos;
	}
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
		cProgramm.insert(GetByte(val,0), inx++);
		cProgramm.insert(GetByte(val,1), inx++);
		cProgramm.insert(GetByte(val,2), inx++);
		cProgramm.insert(GetByte(val,3), inx++);
		return inx;
	}
	size_t replaceInt(int val, size_t &inx)
	{	// setting a 32bit integer
		cProgramm[inx++] = GetByte(val,0);
		cProgramm[inx++] = GetByte(val,1);
		cProgramm[inx++] = GetByte(val,2);
		cProgramm[inx++] = GetByte(val,3);
		return inx;
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
		union
		{
			float valf;
			char buf[4];
		}storage;
		storage.valf=val;

		cProgramm.insert(storage.buf[0], inx++ );
		cProgramm.insert(storage.buf[1], inx++ );
		cProgramm.insert(storage.buf[2], inx++ );
		cProgramm.insert(storage.buf[3], inx++ );

		return inx;
	}
	size_t replaceFloat(float val, size_t &inx)
	{	// setting a 32bit float
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

		return inx;
	}
	size_t appendFloat(float val)
	{	// setting a 32bit float
		union
		{
			float valf;
			char buf[4];
		}storage;
		storage.valf=val;
		size_t pos = cProgramm.size();
		cProgramm.append(storage.buf[0]);
		cProgramm.append(storage.buf[1]);
		cProgramm.append(storage.buf[2]);
		cProgramm.append(storage.buf[3]);

		return pos;
	}
	const char* getString(size_t &inx)
	{	// getting a pointer, 64bit ready
		// msb to lsb order for faster reading
		if( inx+sizeof(size_t) <= cProgramm.size() )
		{
			size_t i, vali = 0;
			for(i=0; i<sizeof(size_t); i++)
				vali = vali<<8 | cProgramm[inx++];
			return (const char*)vali;
		}
		return "";
	}
	size_t insertString(const char*val, size_t &inx)
	{	// setting a pointer, 64bit ready
		// msb to lsb order for faster reading
		size_t i, vali = (size_t)val;
		for(i=0; i<sizeof(size_t); i++)
		{
			cProgramm.insert( vali&0xFF, inx+sizeof(size_t)-1-i );
			vali >>= 8;
		}
		inx += sizeof(size_t);
		return inx;
	}
	size_t appendString(const char*val)
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
	size_t getCurrentPosition()	{ return cProgramm.size(); }

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
	TArrayDCT<CFunction>	cFunctionTable;
	TArrayDCT<CScript>		cScriptTable;
	TslistDCT<MiniString>	cStringTable;
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

CScriptEnvironment env;


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
			case VX_LABEL:	// followed my a string var containing the label name
							// 	<Label Stm>     ::= Id ':'
			{	// just skipping the string
				cPC+=4;
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


#define CFLAG_NONE		0x00000000	// no restrictions
#define CFLAG_LVALUE	0x00000001	// a variable is required
#define CFLAG_USE_BREAK	0x00000002	// allow break + jump offset 
#define CFLAG_USE_CONT	0x00000004	// allow continue + jump offset 



bool CheckTerminal(struct _list *baselist, int idx)
{
	return ( baselist && baselist->symbol.Type==1 && baselist->symbol.idx==idx );
}
bool CheckNonTerminal(struct _list *baselist, int idx)
{
	return ( baselist && baselist->symbol.Type==0 && baselist->symbol.idx==idx );
}

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

bool StackProg(struct _list *baselist, size_t level, unsigned long flags, CProgramm& prog)
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
					ip = env.insert2Stringtable( str );
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
					const char *ip = env.insert2Stringtable( baselist->token.lexeme );
					if(ip)
					{
						if( 0 != (flags&CFLAG_LVALUE) )
						{
							prog.appendCommand(OP_PUSH_VAR);
							prog.appendString( ip );

							printf("Variable Name accepted: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
						}
						else
						{
							prog.appendCommand(OP_PUSH_VALUE);
							prog.appendString( ip );

							printf("Variable Value accepted: %s - %s\n", baselist->symbol.Name, baselist->token.lexeme);
						}
						accept = true;
					}
					else
					{
						printf("error when inserting to stringtable\n");
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
				accept = StackProg(baselist->list[0], level+1, flags, prog);
			}
			else
			// check the childs
			switch(baselist->symbol.idx)
			{
			///////////////////////////////////////////////////////////////////
			// a label
			case PT_LABELSTM:
			{	// expecting 2 terminals in here, the first is the labelname, the second a ":"
				printf("PT_LABELSTM - ");

				size_t pos = prog.getCurrentPosition();
				const char*ip = prog.insertLabel( baselist->list[0]->token.lexeme, pos );
				if(ip)
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
				const char*ip = prog.insertLabel( baselist->list[1]->token.lexeme );
				if(ip)
				{
					prog.appendString( ip );
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
				
				// put first the source
				// so the result will be as single value (int, string or float) on stack
				accept  = StackProg(baselist->list[2], level+1, flags & ~CFLAG_LVALUE, prog);
				
				// then put the target as variable, 
				// this should check for L-Values
				accept &= StackProg(baselist->list[0], level+1, flags | CFLAG_LVALUE, prog);

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
					// go through childs now
					if( baselist->count == 3 )
					{	// ID <something> ';'
						// process the <something>, need values
						accept = StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog); // need values
					}
					else if( baselist->count == 2 )
					{	// ID ';'
						accept = true;
					}

					unsigned char command = OP_NOP;
					int id = env.getFunctionID( baselist->list[0]->token.lexeme );
					if( id>=0 )
					{	// found in function table
						command = OP_CALLBUILDIN;
					}
					else
					{
						id = env.getScriptID( baselist->list[0]->token.lexeme );
						if( id>=0 )
						{
							command = OP_CALLSCRIPT;
						}
						else
						{
							accept=false;
						}
					}

					printf("PT_CALLSTM - ");
					if(accept)
					{
						// push the command
						prog.appendCommand(command);
						// push the function ID
						prog.appendInt(id);


						printf("accepting call statement: ");
						PrintChildTerminals(baselist);
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
						accept &= StackProg(baselist->list[baselist->count-1], level+1,flags, prog);
				}
				break;
			}
			case PT_CALLLIST:
			{	// call list elements are comma seperated terminals or nonterminals
				accept = true;
				for(i=0; i< baselist->count; i+=2)
				{
					// a variable
					accept &= StackProg(baselist->list[i], level+1,flags, prog);
					// followed by a comma (exept the last)
					if( i+1<baselist->count )
						accept &= CheckTerminal(baselist->list[i+1], PT_COMMA);
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
					// go through childs now
					
					if( baselist->count == 4 )
					{	// ID '(' <something> ')'
						// process the <something>, need values
						accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog); // need values
					}
					else if( baselist->count == 3 )
					{	// ID '(' ')'
						accept = true;
					}

					unsigned char command = OP_NOP;
					int id = env.getFunctionID( baselist->list[0]->token.lexeme );
					if( id>=0 )
					{	// found in function table
						command = OP_CALLBUILDIN;
					}
					else
					{
						id = env.getScriptID( baselist->list[0]->token.lexeme );
						if( id>=0 )
						{
							command = OP_CALLSCRIPT;
						}
						else
						{
							accept=false;
						}
					}

					printf("PT_RETVALUES - ");
					if(accept)
					{
						// push the command
						prog.appendCommand(command);
						// push the function ID
						prog.appendInt(id);

						printf("accepting call statement: ");
						PrintChildTerminals(baselist);
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
						accept &= StackProg(baselist->list[baselist->count-1], level+1,flags, prog);
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
						accept = StackProg(baselist->list[i], level+1,flags, prog);
						if( !accept ) break;
					}
				}
				else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_IF) )
				{	// if '(' <Expr> ')' <Normal Stm>
					// put in <Expr>
					accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog); // need the result on the stack
					printf("Conditional Jump False -> Label1\n");
					prog.appendCommand(OP_NIF);
					size_t inspos1 = prog.appendInt(0);	// placeholder
					// put in <Normal Stm>
					accept &= StackProg(baselist->list[4], level+1,flags, prog);
					printf("Label1\n");
					// calculate and insert the correct jump 
					prog.replaceInt( prog.getCurrentPosition() ,inspos1);
				}
				else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_IF) )
				{	// if '(' <Expr> ')' <Normal Stm> else <Normal Stm>
					// put in <Expr>
					accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog); // need the result on the stack
					printf("Conditional Jump False -> Label1\n");
					prog.appendCommand(OP_NIF);
					size_t inspos1 = prog.appendInt(0);	// placeholder
					// put in <Normal Stm>
					accept &= StackProg(baselist->list[4], level+1,flags, prog);
					printf("Goto -> Label2\n");
					prog.appendCommand(VX_GOTO);
					size_t inspos2 = prog.appendInt(0);	// placeholder
					printf("Label1\n");
					// calculate and insert the correct jump 
					prog.replaceInt( prog.getCurrentPosition() ,inspos1);
					// put in <Normal Stm2>
					accept &= StackProg(baselist->list[6], level+1,flags, prog);
					printf("Label2\n");
					prog.replaceInt( prog.getCurrentPosition(), inspos2);			
				}
				else if( baselist->count ==9 && CheckTerminal(baselist->list[0], PT_FOR) )
				{	// for '(' <Arg> ';' <Arg> ';' <Arg> ')' <Normal Stm>
					// execute <Arg1>
					accept  = StackProg(baselist->list[2], level+1,flags, prog);
					// execute ";" to clear the stack;
					accept &= StackProg(baselist->list[3], level+1,flags, prog);
					printf("Label1\n");
					size_t tarpos1 = prog.getCurrentPosition();// position marker
					// execute <Arg2>, need a value
					accept &= StackProg(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog);

					printf("Conditional Jump False -> Label2\n");
					prog.appendCommand(OP_NIF);
					size_t inspos2 = prog.appendInt(0);	// placeholder

					// execute the loop body
					size_t rstart = prog.getCurrentPosition();
					accept &= StackProg(baselist->list[8], level+1, flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog);
					size_t rend   = prog.getCurrentPosition();

					// execute the incrementor <Arg3>
					accept &= StackProg(baselist->list[6], level+1,flags, prog);
					// execute ";" to clear the stack;
					accept &= StackProg(baselist->list[5], level+1,flags, prog);

					printf("Goto -> Label1\n");
					size_t srcpos1=prog.getCurrentPosition();
					prog.appendCommand(VX_GOTO);
					prog.appendInt(tarpos1);

					printf("Label2\n");
					size_t tarpos2 = prog.getCurrentPosition();
					prog.replaceInt( tarpos2 ,inspos2);

					// execute ";" to clear the stack;
					accept &= StackProg(baselist->list[3], level+1,flags, prog);

					// convert break -> goto Label2
					// convert continue -> goto Label1
					prog.replaceJumps(rstart,rend,VX_BREAK,tarpos2);
					prog.replaceJumps(rstart,rend,VX_CONT,tarpos1);

				}
				else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_WHILE) )
				{	// while '(' <Expr> ')' <Normal Stm>

					printf("Label1\n");
					size_t tarpos1 = prog.getCurrentPosition();// position marker

					// execute <Expr>
					accept  = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

					printf("Conditional Jump False -> Label2\n");
					prog.appendCommand(OP_NIF);
					size_t inspos2 = prog.appendInt(0);	// placeholder offset

					// execute <Normal Stm>
					size_t rstart = prog.getCurrentPosition();
					accept &= StackProg(baselist->list[4], level+1,flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog);
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
					accept  = StackProg(baselist->list[1], level+1,flags | CFLAG_USE_BREAK | CFLAG_USE_CONT, prog);
					size_t rend = prog.getCurrentPosition();

					// execute <Expr>
					accept &= StackProg(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog);
					printf("Conditional Jump True -> Label1\n");
					prog.appendCommand(OP_IF);
					prog.appendInt(tarpos1);

					size_t tarpos2 = prog.getCurrentPosition();
					accept &= StackProg(baselist->list[6], level+1,flags & ~CFLAG_LVALUE, prog);

					// convert break -> goto Label2
					// convert continue -> goto Label1
					prog.replaceJumps(rstart,rend,VX_BREAK,tarpos2);
					prog.replaceJumps(rstart,rend,VX_CONT,tarpos1);

				}
				else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_SWITCH) )
				{	// switch '(' <Expr> ')' '{' <Case Stms> '}'
					// <Case Stms> ::= case <Value> ':' <Stm List> <Case Stms>
					//               | default ':' <Stm List>
//!! todo
					printf("Switch not yet implemented\n");
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
					prog.appendCommand(VX_BREAK);
					prog.appendInt( 0 );
					
					accept = true;
				}
				else if( (baselist->list[0] && baselist->list[0]->symbol.idx == PT_BREAK) && (0!=(flags & CFLAG_USE_BREAK)) )
				{
					prog.appendCommand(VX_CONT);
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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[3], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				printf("Conditional Jump False -> Label1\n");
				prog.appendCommand(OP_NIF);
				size_t inspos1 = prog.appendInt(0);	// placeholder
				// put in <Op If1>
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);
				printf("Goto -> Label2\n");
				prog.appendCommand(VX_GOTO);
				size_t inspos2 = prog.appendInt(0);	// placeholder
				printf("Label1\n");
				// calculate and insert the correct jump offset
				prog.replaceInt( prog.getCurrentPosition() ,inspos1);
				// put in <Op If2>
				accept &= StackProg(baselist->list[4], level+1,flags & ~CFLAG_LVALUE, prog);
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
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog);	// base variable
					accept &= StackProg(baselist->list[2], level+1,flags | CFLAG_LVALUE, prog);	// member variable or function

					if( CheckNonTerminal(baselist->list[2], PT_ID) )
					{	// have to select the correct variable inside base according to the member name
						// and put this variable on the stack

						prog.appendCommand(OP_MEMBER);
						printf("PT_MEMBERACCESS variable %s\n", accept?"ok":"failed");
					}
					else if( CheckNonTerminal(baselist->list[2], PT_RETVALUES) )
					{
						// function call is already done
						printf("PT_MEMBERACCESS function %s\n", accept?"ok":"failed");
					}
					else
					{
						printf("PT_MEMBERACCESS failed\n");
					}
					
					break;
				case PT_LBRACKET:
					// put the operands on stack
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog);	// variable
					accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);	// index

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
				accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog); // put the variable itself on stack

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
				accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog); // put the variable itself on stack

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
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE, prog);

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
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE, prog); // put the variable itself on stack
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

				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE, prog);
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
				accept  = StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE, prog);
				break;
			}
			///////////////////////////////////////////////////////////////////
			// operands
			case PT_EXPRLIST:
			{	// go through childs, spare the comma
				accept = true;
				for(i=0; i<baselist->count; i++)
				{
					if( !CheckTerminal(baselist->list[i], PT_COMMA) )
					{
						accept = StackProg(baselist->list[i], level+1,flags, prog);
						if( !accept ) break;
					}
				}
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
					accept = StackProg(baselist->list[i], level+1,flags, prog);
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
					accept = StackProg(baselist->list[i], level+1,flags, prog);
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
unsigned short CompileScriptSpriteID(struct _list *spriteid)
{
	if( spriteid && spriteid->count==1 && spriteid->symbol.idx==PT_SPRITEID)
		return atoi(spriteid->list[0]->token.lexeme );
	else if( spriteid && spriteid->symbol.idx==PT_DECLITERAL)
		return atoi(spriteid->token.lexeme );
	else
		return 0xFFFF;
		
}
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

bool CompileHeads(struct _list *baselist)
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

				env.addFunction( baselist->list[1]->token.lexeme, 0 );

				printf("function declaration: ");
				PrintChildTerminals(baselist);
				printf("\n");
				accept = true;
				break;
			}
			case PT_BLOCK:
			{	// single <Block> without header (for small scripts)
				CProgramm prog;
				accept = StackProg(baselist, 0, 0, prog);
				prog.appendCommand(OP_END);

				printf("\n");
				printf("binary output\n");
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
					// generate a new function script
					int pos = env.addFunction( baselist->list[1]->token.lexeme, 0 );
					if( pos>=0 )
					{	// compile the <block>
						CProgramm &prog = env.function(pos);
						accept = StackProg(baselist->list[baselist->count-1], 0, 0, prog);
						prog.appendCommand(OP_END);


						printf("\n");
						printf("binary output\n");
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
					int pos= env.addScript(id, name, map, x, y, dir, sprite);
					if( pos>=0 )
					{	// compile the <block>
						CProgramm &prog = env.script(pos);
						accept = StackProg(baselist->list[baselist->count-1], 0, 0, prog);
						prog.appendCommand(OP_END);

						printf("\n");
						printf("binary output\n");
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
					accept = CompileHeads(baselist->list[i]);
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




#include <iostream.h>

void startcompile(struct _parser* parser)
{
	struct _list baselist;
	memset(&baselist,0,sizeof(struct _list));

	reduce_tree(parser, 0, &baselist,0);
fflush(stdout);
	print_listtree(baselist.list[0],0);
fflush(stdout);
	CompileHeads(baselist.list[0]);
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