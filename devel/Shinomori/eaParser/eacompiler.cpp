
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "parser.h"
#include "eacompiler.h"

#include "base.h"

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
	~MiniString()					{clear();}

	const MiniString &operator=(const MiniString &a)	{ copy(a.string); return *this; }
	const char*	operator=(const char *in)				{ copy(in); return string; }
	operator char*()									{ return string; }
	operator const char*() const						{ return string; }

	bool operator==(const char *b) const {return (0==strcmp(this->string, b));}
	bool operator!=(const char *b) const {return (0!=strcmp(this->string, b));}

	friend bool operator==(const char *a, const MiniString &b) {return (0==strcmp(a, b.string));}
	friend bool operator!=(const char *a, const MiniString &b) {return (0!=strcmp(a, b.string));}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Variant
{
	typedef enum _variant_type
	{
		VAR_NONE = 0,
		VAR_INTEGER,
		VAR_STRING,
		VAR_FLOAT,
		VAR_ARRAY
	} VARTYPE;

	VARTYPE cType;
	size_t cSize;	// Array Elements

	union
	{
		int64		cInteger;
		double		cFloat;

		union
		{
			char*		cString;
			size_t		cStrLen;
		};
		union
		{
			Variant*	cArray;
			size_t		cSize;
		};		
	};

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
			}
			cType = VAR_NONE;
			cInteger = 0;
			cSize = 0;
		}
	}
public:
	///////////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	Variant()
	{
		cType = VAR_NONE;
		cInteger = NULL;
		cSize = 0;
	}
	~Variant()
	{
		clear();
	}
	///////////////////////////////////////////////////////////////////////////
	// Copy/Assignment
	Variant(const Variant &v)
	{
		cType = VAR_NONE;
		cInteger = NULL;
		cSize = 0;
		this->assign(v);
	}

	const Variant& operator=(const Variant &v)	{ return assign(v);	  }
	const Variant& operator=(const int val)		{ return assign(val); }
	const Variant& operator=(const double val)	{ return assign(val); }
	const Variant& operator=(const char* val)	{ return assign(val); }

	///////////////////////////////////////////////////////////////////////////
	// compare
	bool operator ==(const Variant &v) const { return this==&v; }
	bool operator !=(const Variant &v) const { return this==&v; }

	///////////////////////////////////////////////////////////////////////////
	// Type Initialize Variant (aka copy)
	const Variant& assign(const Variant& v)
	{
		this->clear();
		if(v.cType!=VAR_NONE)
		{
			size_t i;
			cType = v.cType;
			switch(cType)
			{
			case VAR_ARRAY:
				cSize = v.cSize;
				cArray = new Variant[cSize];
				for(i=0; i<cSize; i++)
					cArray[i] = v.cArray[i];
			case VAR_STRING:
				cStrLen = v.cStrLen;
				cString = new char[1+cStrLen];
				memcpy(cString, v.cString,1+cStrLen);
			case VAR_INTEGER:
				cInteger = v.cInteger;
			case VAR_FLOAT:
				cFloat = v.cFloat;
			}
		}
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize Integer
	Variant(int val)
	{
		cType = VAR_NONE;
		cInteger = NULL;
		cSize = 0;
		this->assign(val);
	}
	const Variant& assign(int val)
	{
		clear();
		cType = VAR_INTEGER;
		cInteger = val;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize double
	Variant(double val)
	{
		cType = VAR_NONE;
		cInteger = NULL;
		cSize = 0;
		this->assign(val);
	}
	const Variant& assign(double val)
	{
		clear();
		cType = VAR_FLOAT;
		cFloat = val;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Type Initialize String
	Variant(const char* val)
	{
		cType = VAR_NONE;
		cInteger = NULL;
		this->assign(val);
	}
	const Variant& assign(const char* val)
	{
		clear();
		cType = VAR_STRING;
		
		cStrLen = (val) ? strlen(val) : 0;
		cString = new char[cStrLen+1];
		if(val)
			memcpy(cString,val,cStrLen+1);
		else
			*cString=0;
		return *this;
	}
	///////////////////////////////////////////////////////////////////////////
	// Create Array
	void initialize_array(size_t cnt)
	{
		clear();
		if( cnt>1 )
		{
			cType = VAR_ARRAY;
			cSize = cnt;
			cArray = new Variant[cSize];
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Access to elements
	bool isValid() const	{ return cType!=VAR_NONE; }	
	bool isInt() const		{ return cType==VAR_INTEGER; }	
	bool isFloat() const	{ return cType==VAR_FLOAT; }	
	bool isString() const	{ return cType==VAR_STRING; }	
	bool isArray() const	{ return cType==VAR_ARRAY; }
	size_t getSize() const	{ return (cType==VAR_ARRAY) ? cSize : 1; }

	///////////////////////////////////////////////////////////////////////////
	// Access to elements
	Variant& operator[](size_t inx)
	{	// on Arrays return the element, out of bounds return element 0
		// other array accesses return this object
		return (cType==VAR_ARRAY) ? ((inx<cSize) ? cArray[inx] : cArray[0]) : *this;
	}

	///////////////////////////////////////////////////////////////////////////
	// Access operators
	operator int() const	
	{
		switch(cType)
		{
		case VAR_NONE:
		{	// nothing equals 0
			return 0;
		}
		case VAR_INTEGER:
		{	// direct
			return (int)cInteger;
		}
		case VAR_FLOAT:
		{	// cast
			return (int)cFloat;
		}
		case VAR_STRING:
		{	// convert
			return (cString) ? atoi(cString) : 0;
		}
		case VAR_ARRAY:
		{	// first element
			return (int)cArray[0];
		}
		}// end switch
	}
	operator double() const
	{
		switch(cType)
		{
		case VAR_NONE:
		{	// nothing equals 0
			return 0;
		}
		case VAR_INTEGER:
		{	// cast
			return (double)cInteger;
		}
		case VAR_FLOAT:
		{	// direct
			return cFloat;
		}
		case VAR_STRING:
		{	// convert
			return (cString) ? atof(cString) : 0;
		}
		case VAR_ARRAY:
		{	// first element
			return (double)cArray[0];
		}
		}// end switch
	}

	///////////////////////////////////////////////////////////////////////////
	// Conversion
	bool convert2integer()
	{
		*this = (int)(*this);
		return true;
	}
	bool convert2float()
	{
		*this = (double)(*this);
		return true;
	}
	bool convert2string()
	{
		char buffer[128]="";
		switch(cType)
		{
		case VAR_NONE:
		{	
			*this = buffer;
			break;
		}
		case VAR_INTEGER:
		{	
			sprintf(buffer, "%i", (int)cInteger);
			*this = buffer;
			break;
		}
		case VAR_FLOAT:
		{	
			sprintf(buffer, "%.6lf", cFloat);
			*this = buffer;
			break;
		}
		case VAR_STRING:
		{	break;
		}
		case VAR_ARRAY:
		{	// first element
			
			cArray[0].convert2string();

			// take ownership of the string from cArray[0]
			char *temp = cArray[0].cString;
			cArray[0].cString = NULL;

			// clear this
			clear();

			// and change to string
			cType = VAR_STRING;
			cStrLen = (temp) ? strlen(temp) : 0;
			cString = temp;

			break;
		}
		}// end switch
		
		return true;
	}
	bool convert2array(size_t cnt)
	{
		size_t i;
		switch(cType)
		{
		case VAR_NONE:
		{	
			initialize_array(cnt);
			break;
		}
		case VAR_INTEGER:
		{	
			int temp = (int)cInteger;
			initialize_array(cnt);
			for(i=0;i<cnt; i++)
				cArray[i] = temp;
			break;
		}
		case VAR_FLOAT:
		{	
			double temp = cFloat;
			initialize_array(cnt);
			for(i=0;i<cnt; i++)
				cArray[i] = temp;
			break;
		}
		case VAR_STRING:
		{
			// take ownership of the string
			char *temp = cString;
			cString = NULL;

			// clear this
			initialize_array(cnt);

			// copy the string
			for(i=1;i<cnt; i++)
				cArray[i] = temp;

			// give cArray[0] ownership to the string
			cArray[0].cType = VAR_STRING;
			cArray[0].cString = temp;
			cArray[0].cStrLen = (temp) ? strlen(temp):0;
		
			break;

		}
		case VAR_ARRAY:
		{	// this would work for all types but is not practical 
			// until smart pointers are introduced 
			
			// copy this
			Variant temp = *this;
			// 
			initialize_array(cnt);
			for(i=0;i<cnt; i++)
				cArray[i] = temp;

			break;
		}
		}// end switch
		
		return true;
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


class StackProgramm
{
public:
	TArrayDST<unsigned char>	Programm;
	TArrayDST<Variable*>		Vars;

	bool FindVariable(const char *name, size_t &inx)
	{	
		register size_t i;
		for(i=0; i<Vars.size(); i++)
		{
			if( Vars[i] && Vars[i]->cName == name )
			{
				inx=i;
				return true;
			}
		}
		return false;
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
	StackProgramm*			cProg;


	bool process()
	{
		bool run = true;
		bool ok =true;
		while(run && ok)
		{
			switch( cProg->Programm[cPC] )
			{
			case OP_NOP:
			{
				cPC++;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// assignment operations
			// take two stack values and push up one
			case OP_ASSIGN:	// <Op If> '='   <Op>
			{
				cSC--;
				cStack[cSC-1] = cStack[cSC];
				cPC++;
				break;
			}
				
			case OP_ADD:		// <Op AddSub> '+' <Op MultDiv>
			case OP_ASSIGN_ADD:	// <Op If> '+='  <Op>
			{
				cSC--;
//				cStack[cSC-1] += cStack[cSC];
				cPC++;
				break;
			}
			case OP_SUB:		// <Op AddSub> '-' <Op MultDiv>
			case OP_ASSIGN_SUB:	// <Op If> '-='  <Op>
			{
				cSC--;
//				cStack[cSC-1] -= cStack[cSC];
				cPC++;
				break;
			}
			case OP_MUL:		// <Op MultDiv> '*' <Op Unary>
			case OP_ASSIGN_MUL:	// <Op If> '*='  <Op>
			{
				cSC--;
//				cStack[cSC-1] *= cStack[cSC];
				cPC++;
				break;
			}
			case OP_DIV:		// <Op MultDiv> '/' <Op Unary>
			case OP_ASSIGN_DIV:	// <Op If> '/='  <Op>
			{
				cSC--;
//				cStack[cSC-1] /= cStack[cSC];
				cPC++;
				break;
			}
			case OP_MOD:		// <Op MultDiv> '%' <Op Unary>
			{
				cSC--;
//				cStack[cSC-1] /= cStack[cSC];
				cPC++;
				break;
			}
			case OP_BIN_XOR:	// <Op BinXOR> '^' <Op BinAND>
			case OP_ASSIGN_XOR:	// <Op If> '^='  <Op>
			{
				cSC--;
//				cStack[cSC-1] ^= cStack[cSC];
				cPC++;
				break;
			}
			case OP_BIN_AND:		// <Op BinAND> '&' <Op Equate>
			case OP_ASSIGN_AND:	// <Op If> '&='  <Op>
			{
				cSC--;
//				cStack[cSC-1] &= cStack[cSC];
				cPC++;
				break;
			}
			case OP_BIN_OR:		// <Op BinOr> '|' <Op BinXOR>
			case OP_ASSIGN_OR:	// <Op If> '|='  <Op>
			{
				cSC--;
//				cStack[cSC-1] |= cStack[cSC];
				cPC++;
				break;
			}
			case OP_LOG_AND:	// <Op And> '&&' <Op BinOR>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] && (int)cStack[cSC];
				cPC++;
				break;
			}
			case OP_LOG_OR:		// <Op Or> '||' <Op And>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] || (int)cStack[cSC];
				cPC++;
				break;
			}
			case OP_RSHIFT:		// <Op Shift> '>>' <Op AddSub>
			case OP_ASSIGN_RSH:	// <Op If> '>>='  <Op>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] >> (int)cStack[cSC];
				cPC++;
				break;
			}
			case OP_LSHIFT:		// <Op Shift> '<<' <Op AddSub>
			case OP_ASSIGN_LSH:	// <Op If> '<<='  <Op>
			{
				cSC--;
//				cStack[cSC-1] = (int)cStack[cSC-1] << (int)cStack[cSC];
				cPC++;
				break;
			}
			case OP_EQUATE:		// <Op Equate> '==' <Op Compare>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]==cStack[cSC]);
				cPC++;
				break;
			}
			case OP_UNEQUATE:	// <Op Equate> '!=' <Op Compare>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]!=cStack[cSC]);
				cPC++;
				break;
			}
			case OP_ISGT:		// <Op Compare> '>'  <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]>cStack[cSC]);
				cPC++;
				break;
			}
			case OP_ISGTEQ:		// <Op Compare> '>=' <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]>=cStack[cSC]);
				cPC++;
				break;
			}
			case OP_ISLT:		// <Op Compare> '<'  <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]<cStack[cSC]);
				cPC++;
				break;
			}
			case OP_ISLTEQ:		// <Op Compare> '<=' <Op Shift>
			{
				cSC--;
//				cStack[cSC-1] = (cStack[cSC-1]<=cStack[cSC]);
				cPC++;
				break;
			}
			/////////////////////////////////////////////////////////////////
			// select operation
			// take three stack values and push the second or third depending on the first
			case OP_SELECT:	// <Op Or> '?' <Op If> ':' <Op If>
			{
				cSC -= 2;
				cStack[cSC-1] = ((int)cStack[cSC-1]) ? cStack[cSC] : cStack[cSC+1];
				cPC++;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// unary operations
			// take one stack values and push a value
			case OP_NOT:	// '!'    <Op Unary>
			{
				cStack[cSC-1] = !((int)cStack[cSC-1]);
				cPC++;
				break;
			}
			case OP_INVERT:	// '~'    <Op Unary>
			{
				cStack[cSC-1] = ~((int)cStack[cSC-1]);
				cPC++;
				break;
			}
			case OP_NEGATE:	// '-'    <Op Unary>
			{
//				cStack[cSC-1] = -cStack[cSC-1];
				cPC++;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// sizeof operations
			// take one stack values and push the result
								// sizeof '(' <Type> ')' // replaces with OP_PUSH_INT on compile time
			case OP_SIZEOF:		// sizeof '(' Id ')'
			{
//				cStack[cSC-1] = cStack[cSC-1].size();
				cPC++;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// cast operations
			// take one stack values and push the result
			case OP_CAST:	// '(' <Type> ')' <Op Unary>   !CAST
			{	// <Op Unary> is first on the stack, <Type> is second
				cSC--;
//				cStack[cSC-1].cast(cStack[cSC]);
				cPC++;
				break;
			}

			/////////////////////////////////////////////////////////////////
			// Pre operations
			// take one stack variable and push a value
			case OP_PREADD:		// '++'   <Op Unary>
			{	
//				cStack[cSC-1]++;
//				cStack[cSC-1] = cStack[cSC-1].value();
				cPC++;
				break;
			}
			case OP_PRESUB:		// '--'   <Op Unary>
			{	
//				cStack[cSC-1]--;
//				cStack[cSC-1] = cStack[cSC-1].value();
				cPC++;
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
				cPC++;
				break;
			}
			case OP_POSTSUB:	// <Op Pointer> '--'
			{	
//				Variant temp = cStack[cSC-1].value();
//				cStack[cSC-1]--;
//				cStack[cSC-1] = temp;
				cPC++;
				break;
			}



			/////////////////////////////////////////////////////////////////
			// Member Access
			// take a variable and a value from stack and push a varible
			case OP_MEMBER:		// <Op Pointer> '.' <Value>     ! member
			{
				printf("not implemented yet\n");

				cSC--;
				cPC++;
				break;
			}


			/////////////////////////////////////////////////////////////////
			// Array
			// take a variable and a value from stack and push a varible
			case OP_ARRAY:		// <Op Pointer> '[' <Expr> ']'  ! array
			{
				cSC--;
//				cStack[cSC-1] = cStack[cSC-1][ cStack[cSC] ];
				cPC++;
				break;
			}


			/////////////////////////////////////////////////////////////////
			// standard function calls
			// check the values on stack before or inside the call of function
			case OP_CALL:	// Id '(' <Expr> ')'
							// Id '(' ')'
							// Id <Call List> ';'
							// Id ';'
			{

				printf("not implemented yet\n");

				cSC--;
				cPC++;
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
				unsigned char *buf = &(cProg->Programm[cPC]);
				int temp = (int)MakeDWord( buf[1],buf[2],buf[3],buf[4] );
				cStack[cSC] = temp;
				cSC++;
				cPC += 5;
				break;
			}
			case OP_PUSH_STRING:	// followed by a string (pointer)
			{
				unsigned char *buf = &(cProg->Programm[cPC]);
				const char* temp = (const char*)MakeDWord( buf[1],buf[2],buf[3],buf[4] );
				cStack[cSC] = temp;
				cSC++;
				cPC += 5;
				break;
			}
			case OP_PUSH_FLOAT:	// followed by a float
			{
				unsigned char *buf = &(cProg->Programm[cPC]);
				const char* temp = (const char*)MakeDWord( buf[1],buf[2],buf[3],buf[4] );
				cStack[cSC] = atof(temp);
				cSC++;
				cPC += 5;
				break;
			}
			case OP_PUSH_VAR:	// followed by a string containing a variable name
			{
				unsigned char *buf = &(cProg->Programm[cPC]);
				const char* temp = (const char*)MakeDWord( buf[1],buf[2],buf[3],buf[4] );
				cStack[cSC] = atof(temp);
				cSC++;
				cPC += 5;
				break;
			}
			case OP_POP:	// decrements the stack by one
			{
				cSC--;
				cPC++;
				break;
			}
			case VX_LABEL:	// followed my a string var containing the label name
							// 	<Label Stm>     ::= Id ':'
			{	// just skipping
				cPC+=5;
				break;
			}
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


#define CFLAG_NONE		0x00000000
#define CFLAG_LVALUE	0x00000001
#define CFLAG_USE_BREAK	0x00000002
#define CFLAG_USE_CONT	0x00000004




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

bool StackProg(struct _list *baselist, size_t level, unsigned long flags)
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
				accept = true;
				break;

			case PT_HEXLITERAL:
				printf("PT_HEXLITERAL - ");
				printf("%s - %s ", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
				if( 0 == (flags&CFLAG_LVALUE) )
				{
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
					printf("accepted\n");
					accept = true;
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
				if( 0 != (flags&CFLAG_LVALUE) )
					printf("Variable Name accepted: %s - %s\n", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");
				else
					printf("Variable Value accepted: %s - %s\n", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");

				accept = true;
				break;

			default:
				
				printf("Term  - ");
				printf("%s - %s\n", baselist->symbol.Name, (baselist->token.lexeme)?baselist->token.lexeme:"");

				// accept any terminal right now
				accept = true;

				break;
				
			}


		}
		///////////////////////////////////////////////////////////////////////
		else
		{	// nonterminals

			if( baselist->count==1 )
			{	// only one child, just go down
				accept = StackProg(baselist->list[0], level+1, flags);
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
				if( baselist->count !=2 || 
					!CheckTerminal(baselist->list[0], PT_ID) ||		// first has to be a terminal
					!CheckTerminal(baselist->list[1], PT_COLON) )	//
				{
					printf("error in label statement: ");
					PrintChildTerminals(baselist);
					printf("\n");
					
				}
				else
				{	// we have 2 correct terminals here
					printf("accepting label: ");
					PrintChildTerminals(baselist);
					printf("\n");
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
				if( baselist->count ==3 )
				{
					switch( baselist->list[1]->symbol.idx )
					{
					case PT_EQ:
					case PT_PLUSEQ:
					case PT_MINUSEQ:
					case PT_TIMESEQ:
					case PT_DIVEQ:
					case PT_CARETEQ:
					case PT_AMPEQ:
					case PT_PIPEEQ:
					case PT_LTLTEQ:
					case PT_GTGTEQ:
						accept = true;
						break;
					}
				}
				if(accept)
				{	// check target and source
					
					// put first the source
					// so the result will be as single value (int, string or float) on stack
					accept  = StackProg(baselist->list[2], level+1, flags & ~CFLAG_LVALUE);
					
					// then put the target as variable, 
					// this should check for L-Values
					accept &= StackProg(baselist->list[0], level+1, flags | CFLAG_LVALUE);
				}
				printf("PT_OPASSIGN - ");
				if(accept)
				{
					printf("accepting assignment %s: ", baselist->list[1]->symbol.Name);
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
						accept = StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE); // need values
					}
					else if( baselist->count == 2 )
					{	// ID ';'
						accept = true;
					}

					printf("PT_CALLSTM - ");
					if(accept)
					{
						printf("accepting call statement: ");
						PrintChildTerminals(baselist);
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
						accept = StackProg(baselist->list[baselist->count-1], level+1,flags);
				}
				break;
			}
			case PT_CALLLIST:
			{	// call list elements are comma seperated terminals or nonterminals
				accept = true;
				for(i=0; i< baselist->count; i+=2)
				{
					// a variable
					accept &= StackProg(baselist->list[i], level+1,flags);
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
						accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE); // need values
					}
					else if( baselist->count == 3 )
					{	// ID '(' ')'
						accept = true;
					}

					printf("PT_RETVALUES - ");
					if(accept)
					{
						printf("accepting call statement: ");
						PrintChildTerminals(baselist);
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
						accept = StackProg(baselist->list[baselist->count-1], level+1,flags);
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
						accept = StackProg(baselist->list[i], level+1,flags);
						if( !accept ) break;
					}
				}
				else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_IF) )
				{	// if '(' <Expr> ')' <Normal Stm>
					// put in <Expr>
					accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE); // need the result on the stack
					printf("Conditional Jump False -> Label1\n");
					// put in <Normal Stm>
					accept &= StackProg(baselist->list[4], level+1,flags);
					printf("Label1\n");
				}
				else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_IF) )
				{	// if '(' <Expr> ')' <Normal Stm> else <Normal Stm>
					// put in <Expr>
					accept = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE); // need the result on the stack
					printf("Conditional Jump False -> Label1\n");
					// put in <Normal Stm>
					accept &= StackProg(baselist->list[4], level+1,flags);
					printf("Goto -> Label2\n");
					printf("Label1\n");
					accept &= StackProg(baselist->list[6], level+1,flags);
					printf("Label2\n");
				}
				else if( baselist->count ==9 && CheckTerminal(baselist->list[0], PT_FOR) )
				{	// for '(' <Arg> ';' <Arg> ';' <Arg> ')' <Normal Stm>
					// execute <Arg1>
					accept  = StackProg(baselist->list[2], level+1,flags);
					// execute ";" to clear the stack;
					accept &= StackProg(baselist->list[3], level+1,flags);

					printf("Label1\n");
					// execute <Arg2>, need a value
					accept &= StackProg(baselist->list[4], level+1,flags & ~CFLAG_LVALUE);

					printf("Conditional Jump False -> Label2\n");

					// execute the loop body
					accept &= StackProg(baselist->list[8], level+1, flags | CFLAG_USE_BREAK | CFLAG_USE_CONT);

					// execute the incrementor <Arg3>
//!! only postincrements are working with this
					accept &= StackProg(baselist->list[6], level+1,flags);
					// execute ";" to clear the stack;
					accept &= StackProg(baselist->list[5], level+1,flags);

					printf("Goto -> Label1\n");

					printf("Label2\n");

				}
				else if( baselist->count ==5 && CheckTerminal(baselist->list[0], PT_WHILE) )
				{	// while '(' <Expr> ')' <Normal Stm>

					printf("Label1\n");
					// execute <Expr>
					accept  = StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

					printf("Conditional Jump False -> Label2\n");

					// execute <Normal Stm>
					accept &= StackProg(baselist->list[4], level+1,flags);

					printf("Goto -> Label1\n");
					printf("Label2\n");
				}
				else if( baselist->count ==7 && CheckTerminal(baselist->list[0], PT_DO) )
				{	// do <Normal Stm> while '(' <Expr> ')' ';'

					printf("Label1\n");
					// execute <Normal Stm>
					accept  = StackProg(baselist->list[1], level+1,flags);

					// execute <Expr>
					accept &= StackProg(baselist->list[4], level+1,flags & ~CFLAG_LVALUE);
					printf("Conditional Jump True -> Label1\n");
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
			// operands
			case PT_OPADDSUB:
			{	// <Op AddSub> '+' <Op MultDiv>
				// <Op AddSub> '-' <Op MultDiv>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_PLUS:
					printf("PT_PLUS\n");
					break;
				case PT_MINUS:
					printf("PT_MINUS\n");
					break;
				}// end switch
				break;
			}
			case PT_OPMULTDIV:
			{	// <Op MultDiv> '*' <Op Unary>
				// <Op MultDiv> '/' <Op Unary>
				// <Op MultDiv> '%' <Op Unary>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_TIMES:
					printf("PT_TIMES\n");
					break;
				case PT_DIV:
					printf("PT_DIV\n");
					break;
				case PT_PERCENT:
					printf("PT_PERCENT\n");
					break;
				}// end switch
				break;
			}
			case PT_OPAND:
			{	// <Op And> '&&' <Op BinOR>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				if( baselist->list[1]->symbol.idx == PT_AMPAMP )
					printf("PT_OPAND\n");
				break;
			}
			case PT_OPOR:
			{	// <Op Or> '||' <Op And>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				if( baselist->list[1]->symbol.idx == PT_PIPEPIPE )
					printf("PT_PIPEPIPE\n");
				break;
			}

			case PT_OPBINAND:
			{	// <Op BinAND> '&' <Op Equate>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				if( baselist->list[1]->symbol.idx == PT_AMP )
					printf("PT_OPBINAND\n");
				break;
			}
			case PT_OPBINOR:
			{	// <Op BinOr> '|' <Op BinXOR>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				if( baselist->list[1]->symbol.idx == PT_PIPE )
					printf("PT_OPBINOR\n");
				break;
			}
			case PT_OPBINXOR:
			{	// <Op BinXOR> '^' <Op BinAND>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				if( baselist->list[1]->symbol.idx == PT_CARET )
					printf("PT_OPBINOR\n");
				break;
			}
			case PT_OPCAST:
			{	// '(' <Type> ')' <Op Unary>

				// put the operands on stack, first the value then the target type
				accept  = StackProg(baselist->list[3], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE);

				printf("PT_OPCAST\n");
				break;
			}
			case PT_OPCOMPARE:
			{	// <Op Compare> '>'  <Op Shift>
				// <Op Compare> '>=' <Op Shift>
				// <Op Compare> '<'  <Op Shift>
				// <Op Compare> '<=' <Op Shift>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_GT:
					printf("PT_GT\n");
					break;
				case PT_GTEQ:
					printf("PT_GTEQ\n");
					break;
				case PT_LT:
					printf("PT_LT\n");
					break;
				case PT_LTEQ:
					printf("PT_LTEQ\n");
					break;
				}// end switch
				break;
			}
			
			case PT_OPEQUATE:
			{	// <Op Equate> '==' <Op Compare>
				// <Op Equate> '!=' <Op Compare>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_EQEQ:
					printf("PT_EQUAL\n");
					break;
				case PT_EXCLAMEQ:
					printf("PT_UNEQUAL\n");
					break;
				}// end switch
				break;
			}
			case PT_OPIF:
			{	// <Op Or> '?' <Op If> ':' <Op If>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);

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
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE);	// base variable
					accept &= StackProg(baselist->list[2], level+1,flags | CFLAG_LVALUE);	// member variable or function

					if( CheckNonTerminal(baselist->list[2], PT_ID) )
					{	// have to select the correct variable inside base according to the member name
						// and put this variable on the stack
						printf("PT_MEMBERACCESS variable %s\n", accept?"ok":"failed");
					}
					else if( CheckNonTerminal(baselist->list[2], PT_RETVALUES) )
					{
						printf("PT_MEMBERACCESS function %s\n", accept?"ok":"failed");
					}
					else
					{
						printf("PT_MEMBERACCESS failed\n");
					}
					
					break;
				case PT_LBRACKET:
					// put the operands on stack
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE);	// variable
					accept &= StackProg(baselist->list[2], level+1,flags & ~CFLAG_LVALUE);	// index

					printf("PT_ARRAYACCESS %s %s\n", (flags)?"variable":"value", (accept)?"successful":"failed");
					break;
				}// end switch
				break;
			}
			case PT_OPPOST:
			{	// <Op Pointer> '++'
				// <Op Pointer> '--'

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE); // put the variable itself on stack

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_PLUSPLUS:
					printf("PT_OPPOST_PLUSPLUS, \n");
					break;
				case PT_MINUSMINUS:
					printf("PT_OPPOST_MINUSMINUS\n");
					break;
				}// end switch

				break;
			}
			case PT_OPPRE:
			{	// '++'   <Op Unary>
				// '--'   <Op Unary>

				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE); // put the variable itself on stack

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_PLUSPLUS:
					printf("PT_OPPRE_PLUSPLUS, \n");
					break;
				case PT_MINUSMINUS:
					printf("PT_OPPRE_MINUSMINUS\n");
					break;
				}// end switch

				break;
			}
			case PT_OPSHIFT:
			{	// <Op Shift> '<<' <Op AddSub>
				// <Op Shift> '>>' <Op AddSub>


				// put the operands on stack
				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);

				switch( baselist->list[1]->symbol.idx )
				{
				case PT_PLUSPLUS:
					printf("PT_OPPRE_PLUSPLUS, \n");
					break;
				case PT_MINUSMINUS:
					printf("PT_OPPRE_MINUSMINUS\n");
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
					accept  = StackProg(baselist->list[0], level+1,flags | CFLAG_LVALUE); // put the variable itself on stack
					printf("PT_OPSIZEOF ID, \n");
					break;
				case PT_MINUSMINUS:
					printf("PT_OPSIZEOF TYPE, put the corrosponding value on stack\n");
					break;
				}// end switch

				break;
			}
			case PT_OPUNARY:
			{	// '!'    <Op Unary>
				// '~'    <Op Unary>
				//'-'    <Op Unary>

				accept  = StackProg(baselist->list[0], level+1,flags & ~CFLAG_LVALUE);
				// put the operands on stack
				switch( baselist->list[2]->symbol.idx )
				{
				case PT_EXCLAM:
					printf("PT_OPUNARY_NOT\n");
					break;
				case PT_TILDE:
					printf("PT_OPUNARY_INVERT\n");
					break;
				case PT_MINUS:
					printf("PT_OPUNARY_NEGATE\n");
					break;
				}// end switch
				break;
			}
			case PT_VALUE:
			{	// '(' <Expr> ')'
				accept  = StackProg(baselist->list[1], level+1,flags & ~CFLAG_LVALUE);
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
						accept = StackProg(baselist->list[i], level+1,flags);
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
					accept = StackProg(baselist->list[i], level+1,flags);
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

				// accept non-terminal
				accept = true;
				for(i=0; i<baselist->count; i++)
				{
					accept = StackProg(baselist->list[i], level+1,flags);
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
			{	// not handling now
				accept = true;
				break;

			}
			case PT_FUNCDECL:
			{	// <Scalar> Id '(' <Params>  ')' <Block>
				// <Scalar> Id '(' ')' <Block>

				if( CheckTerminal(baselist->list[1], PT_ID) )
				{
					// generate a new script, all memories and classes

					// compile the <block>
					accept = StackProg(baselist->list[baselist->count-1], 0, 0);
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
					// generate a new script, all memories and classes

					// compile the <block>
					accept = StackProg(baselist->list[baselist->count-1], 0, 0);
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
				PrintChildTerminals(baselist); printf(" not allowed\n");
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

	print_listtree(baselist.list[0],0);

	CompileHeads(baselist.list[0]);

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