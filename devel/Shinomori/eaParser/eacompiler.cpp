
#include "eacompiler.h"



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
		_value() :cType(VAR_NONE), cInteger(0)
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
		//return (cValue.Type()==cValue.VAR_ARRAY) ? ((inx<cValue.cSize) ? cValue.cArray[inx] : cValue.cArray[0]) : *this;
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
			case OP_CALLSCRIPT1:
			case OP_CALLSCRIPT2:
			case OP_CALLSCRIPT3:
			case OP_CALLSCRIPT4:
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
			case OP_CALLBUILDIN1:
			case OP_CALLBUILDIN2:
			case OP_CALLBUILDIN3:
			case OP_CALLBUILDIN4:
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

			case OP_PUSH_INT1:	// followed by an integer
			case OP_PUSH_INT2:
			case OP_PUSH_INT3:
			case OP_PUSH_INT4:
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




