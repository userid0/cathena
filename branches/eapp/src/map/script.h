// $Id: script.h,v 1.2 2004/09/25 05:32:19 MouseJstr Exp $
#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include "basestring.h"

// predeclaration
struct map_session_data;
class CScriptEngine;


extern struct Script_Config
{
	unsigned verbose_mode : 1;
	unsigned warn_func_no_comma : 1;
	unsigned warn_cmd_no_comma : 1;
	unsigned warn_func_mismatch_paramnum : 1;
	unsigned warn_cmd_mismatch_paramnum : 1;
	unsigned event_requires_trigger : 1;
	unsigned event_script_type : 1;
	unsigned _unused7 : 1;

	size_t check_cmdcount;
	size_t check_gotocount;
	char die_event_name[24];
	char kill_event_name[24];
	char login_event_name[24];
	char logout_event_name[24];
	char mapload_event_name[24];
} script_config;


///////////////////////////////////////////////////////////////////////////////
// a label marks a position within a script with a name
class CLabel : public basics::string<>
{
public:
	///////////////////////////////////////////////////////////////////////////
	// class data
						// name of the label (the lable itself if the string)
	size_t		cPos;	// position within the programm

	///////////////////////////////////////////////////////////////////////////
	// constructors/destructor
	CLabel()	{}
	CLabel(const basics::string<>& s) : basics::string<>(s)	{}
	CLabel(const basics::string<>& s, size_t p) : basics::string<>(s), cPos(p)	{}
	~CLabel()	{}
	///////////////////////////////////////////////////////////////////////////
	// compare operators for sorting derived from MiniString
};

///////////////////////////////////////////////////////////////////////////////
// a script data stub
// contains the parsed programm and a label list
class _CScript : public basics::global
{
protected:
	///////////////////////////////////////////////////////////////////////////
	// friends
	friend class CScript;
	friend class CParser;
	friend class basics::TPtrCount<_CScript>;
	friend class basics::TPtrAutoCount<_CScript>;
	friend class CScriptEngine;

	///////////////////////////////////////////////////////////////////////////
	// class data
	basics::string<>			cName;		// name of the script
	basics::TArrayDST<uchar>	cProgramm;	// the programm
	basics::TslistDCT<CLabel>	cLabels;	// list of labels (sorted by name)

	///////////////////////////////////////////////////////////////////////////
	// construction, only friends are allowed to create
	_CScript()
	{ }

	void clear()	
	{
		cName.clear();
		cProgramm.clear();
		cLabels.clear();
	}
	void finalize()
	{	// remove non-callable labels 
		size_t i=0;
		while( i<cLabels.size() )
		{
			if( cLabels[i] && tolower( ((uchar)cLabels[i][0]) ) == 'o' && tolower( ((uchar)cLabels[i][1]) ) == 'n' )
				i++;
			else
				cLabels.removeindex(i);
		}
		// resize the program and label buffer to fit the actual program size
		cLabels.realloc();
		cProgramm.realloc();
	}
	size_t getCurrentPos()			{ return cProgramm.size(); }

public:
	///////////////////////////////////////////////////////////////////////////
	// normal destructor
	~_CScript()	{ }

	///////////////////////////////////////////////////////////////////////////
	// class access done by the friends
};

///////////////////////////////////////////////////////////////////////////////
// a script container
// consists of a reference counting pointer to the actual script object
// which is automatically destructed when the script is not referenced anymore
class CScript : public basics::global
{
	///////////////////////////////////////////////////////////////////////////
	// friends
	friend class CScriptEngine;
	friend class CParser;

	///////////////////////////////////////////////////////////////////////////
	// internal memory
	///////////////////////////////////////////////////////////////////////////
	class CStringBuffer
	{
		class CStrData
		{
		public:
			int					type;
			basics::string<>	str;
			int					backpatch;
			int					label;
			int					val;
			int					next;
			int (*func)(CScriptEngine &);

			CStrData() : type(0),backpatch(0),label(0),val(0),next(0),func(NULL)	{}
			CStrData(int t, const basics::string<>& s, int b, int l, int v, int n,  int (*f)(CScriptEngine &))
				: type(t),str(s),backpatch(b),label(l),val(v),next(n),func(f)	{}

			// unused but necessary for list template
			bool operator==(const CStrData a) const { return this->str==a.str; }
		};

		int							cStrHash[16];
		basics::TArrayDCT<CStrData>	cStrData;
	public:
		CStringBuffer();
		~CStringBuffer()
		{  }

		///////////////////////////////////////////////////////////////////////
		// initialisation members
		///////////////////////////////////////////////////////////////////////
		void loadBuildinFunc(void);
		void loadConstDB(void);
		void loadMapReg(void);
		void init();
		///////////////////////////////////////////////////////////////////////
		// access members
		///////////////////////////////////////////////////////////////////////
		size_t size()					{ return cStrData.size(); }
		CStrData& operator[](size_t i)	{ return cStrData[i]; }

		///////////////////////////////////////////////////////////////////////
		// StrData members
		///////////////////////////////////////////////////////////////////////
	private:
		unsigned char calcHash(const char *str);
	public:
		int searchString(const char *p);
		int addString(const basics::string<>& str);
	};

	static CStringBuffer	cStrData;			// static string buffer

protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TPtrAutoCount<_CScript>	cScript;
public:
	///////////////////////////////////////////////////////////////////////////
	// construction/destruction
	CScript()
	{  }
	~CScript()
	{  }

	size_t size()	{return cScript->cProgramm.size(); }	
	uchar& operator[](size_t i)	{ return cScript->cProgramm[i]; }
	void init(const basics::string<>& name);

	int getInt(unsigned int &pos);
	int getCommand(unsigned int &pos);
};



///////////////////////////////////////////////////////////////////////////////
// Script Parser
// behaves like a script internally
class CParser : private CScript
{
private:
	///////////////////////////////////////////////////////////////////////////
	// parse data
	///////////////////////////////////////////////////////////////////////////
	size_t				cStartLine;
	const char*			cStartPtr;
	size_t				cParseCommandIf;
	int					cParseCommand;

public:
	///////////////////////////////////////////////////////////////////////////
	// construction/destruction
	///////////////////////////////////////////////////////////////////////////
	CParser() : cStartLine(0), cStartPtr(NULL), cParseCommandIf(0), cParseCommand(0) {  }
	~CParser()	{}

	///////////////////////////////////////////////////////////////////////////
	// parsing members
	///////////////////////////////////////////////////////////////////////////
	CScript parseScript(const char *name, const char *src, size_t line);
private:
	void appendByte(unsigned char a)
	{
		cScript->cProgramm.append(a);
	}
	void appendCommand(int a);
	void appendInt(int a);
	void appendLabel(int l);
	void setLabel(size_t l, size_t pos);
	bool skipSpaceComment(const char *&p);
	bool skipWord(const char *&p);
	bool parseSimpleExpr(const char *&p);
	bool parseSubExpr(const char *&p, int limit);
	bool parseExpr(const char *&p);
	bool parseLine(const char *&p);
	void ErrorMessage(const char *mes, const char *pos);
};



///////////////////////////////////////////////////////////////////////////////
// simple class only since we use c-style allocation and clearing at the moment
class CScriptEngine : public basics::noncopyable
{
	///////////////////////////////////////////////////////////////////////////
	// friends
	friend class CValue;
private:
	///////////////////////////////////////////////////////////////////////////
	// ���s�n
	enum STATES		{ OFF=0,RUN, STOP,END,RERUNLINE,GOTO,RETFUNC,ENVSWAP }; // 0...6 ->3bit
	enum NPCSTATE	{ NONE=0, NPC_GIVEN=1, NPC_DEFAULT=2 };					// 0...2 ->2bit
public:
	enum
	{
		C_NOP=0,C_POS,C_INT,C_PARAM,C_FUNC,C_STR,C_CONSTSTR,C_ARG,
		C_NAME,C_EOL, C_RETINFO,

		C_LOR,C_LAND,C_LE,C_LT,C_GE,C_GT,C_EQ,C_NE,   //operator
		C_XOR,C_OR,C_AND,C_ADD,C_SUB,C_MUL,C_DIV,C_MOD,C_NEG,C_LNOT,C_NOT,C_R_SHIFT,C_L_SHIFT
	};


	///////////////////////////////////////////////////////////////////////////
	// simple Variant type with 2 components
	class CValue
	{
		friend class CScriptEngine;
	public:
		///////////////////////////////////////////////////////////////////////
		int type;
		union
		{
			int num;
			const char *str;
		};

		///////////////////////////////////////////////////////////////////////
		// constructions
		CValue() :type(C_INT), num(0)					{}
		CValue(int t, int n) :type(t), num(n)			{}
		CValue(int t, const char *s) :type(t), str(s)	{}
		CValue(int n) : type(C_INT), num(n)				{}
		CValue(const char* s) : type(C_STR), str(NULL)
		{
			if(s)
			{
				str = new char[1+strlen(s)];
				memcpy(const_cast<char*>(str), s, 1+strlen(s));
			}
			else
			{	// empty const string
				this->type = C_CONSTSTR;
				this->str =  "";
			}
		}
		CValue(const CValue& a) : type(a.type), str(NULL)
		{
			this->type = a.type;
			switch(a.type)
			{
			case CScriptEngine::C_CONSTSTR:
				this->str = a.str;
				break;
			case CScriptEngine::C_STR:
				if(a.str)
				{
					this->str = new char[1+strlen(a.str)];
					memcpy(const_cast<char*>(str), a.str, 1+strlen(a.str));
				}
				break;
			default:
				this->num = a.num;
				break;
			}
		}
		///////////////////////////////////////////////////////////////////////
		// destructor
		~CValue()	{ clear(); }

		///////////////////////////////////////////////////////////////////////
		// assignments
		CValue& operator=(const CValue& a)
		{
			this->clear();
			this->type = a.type;
			switch(a.type)
			{
			case CScriptEngine::C_CONSTSTR:
				this->str = a.str;
				break;
			case CScriptEngine::C_STR:
				if(a.str)
				{
					this->str = new char[1+strlen(a.str)];
					memcpy(const_cast<char*>(str), a.str, 1+strlen(a.str));
				}
			default:
				this->num = a.num;
				break;
			}
			return *this;
		}
		CValue& operator=(int n)
		{
			clear();
			this->type = C_INT;
			this->num = n;
			return *this;
		}
		CValue& operator=(const char* s)
		{
			clear();
			if(s)
			{	// copy
				this->type = C_STR;
				this->str = new char[1+strlen(s)];
				memcpy(const_cast<char*>(str), s, 1+strlen(s));
			}
			else
			{	// empty const string
				this->type = C_CONSTSTR;
				this->str =  "";
			}
			return *this;
		}

		/////////////////////////////////////////////////////////////
		// this is NOT a common shift operator
		// it "shifts" the content from one element to the other and clears the source
		CValue& operator<<(CValue& a)
		{
			this->clear();
			memcpy(this, &a, sizeof(CValue));
			a.type=C_INT;
			a.num=0;
			return *this;
		}
		///////////////////////////////////////////////////////////////////////
		static void xmove(CValue &target, CValue &source)
		{
			target.clear();
			memcpy(&target,&source,sizeof(CValue));
			source.type=C_INT;
			source.num=0;
		}
		///////////////////////////////////////////////////////////////////////
		bool isString()
		{
			return (type==CScriptEngine::C_STR || type==CScriptEngine::C_CONSTSTR);
		}
		///////////////////////////////////////////////////////////////////////
		void clear()
		{
			if( type==CScriptEngine::C_STR && str )
			{
				delete[] const_cast<char*>(str);
				type = C_INT;
				num = 0;
			}
		}
	};
private:
	///////////////////////////////////////////////////////////////////////////
	// helper for queueing calling scripts
	// use it dynamically only
	class CCallScript : public basics::noncopyable
	{	
	protected:
		// only CScriptEngine and derived can create and modify this class
		friend class CScriptEngine;		
		// single linked list
		CCallScript* next;
		///////////////////////////////////////////////////////////////////////
		// fully construction with automatic enqueueing
		CCallScript(CCallScript *&root, const char* s, size_t p, uint32 r, uint32 o)
			: next(NULL),script(s), pos(p), rid(r), oid(o)
		{	// queue the element at the end of the list starting at root
			if(!root)
				root = this;
			else
			{
				CCallScript *hook=root;
				while(hook->next) hook=hook->next;
				hook->next=this;
			}
		}
		///////////////////////////////////////////////////////////////////////
		// dequeue from root node
		static CCallScript* dequeue(CCallScript *&root)
		{	// dequeue and return the first element from the queue starting at root
			CCallScript *ret=root;
			if(ret)
			{
				root=ret->next;
				ret->next=NULL;
			}
			return ret;
		}
		virtual void setStack(size_t &def,size_t &ptr, size_t max, CValue*&stack)
		{	// dummy, used for transfering a queued stack on derived class
		}
	///////////////////////////////////////////////////////////////////////////
	public:
		virtual ~CCallScript()
		{
			if(next) delete next;
		}
		///////////////////////////////////////////////////////////////////////
		// public data
		const char* script;
		size_t pos;
		uint32 rid;
		uint32 oid;
	};
	///////////////////////////////////////////////////////////////////////////
	// helper for queueing calling scripts with associated stack
	// use it dynamically only
	class CCallStack : public CCallScript
	{
	public:
		size_t defsp;
		size_t stack_ptr;
		size_t stack_max;
		CValue* stack_data;
		CCallStack(CCallScript *&root, const char* s, size_t p, size_t d, uint32 r, uint32 o, size_t ptr, size_t max, CValue* data)
			: CCallScript(root,s,p,r,o),defsp(d),stack_ptr(ptr), stack_max(max), stack_data(data)
		{   }
		virtual ~CCallStack()
		{
			if(stack_data) delete[] stack_data;
		}
		virtual void setStack(size_t &def, size_t &ptr, size_t &max, CValue*&stack)
		{	// transfer the queued stack
			def = defsp;
			ptr = stack_ptr;
			max = stack_max;
			if(stack) delete[] stack;
			stack = stack_data;
			// clear the internals
			stack_data = NULL;
			stack_ptr=0;
			stack_max=0;
		}
	};



	///////////////////////////////////////////////////////////////////////////
	// data section
private:
	CCallScript* queue;			// script execution queue;
	size_t stack_ptr;
	size_t stack_max;
	CValue *stack_data;			// execution stack

	///////////////////////////////////////////////////////////////////////////

	static uint32 defoid;		// id of the default npc

	bool rerun_flag : 1;		// reruning line (used for inputs, menu, select and close)
	bool messageopen : 1;		// set when the client has got some message window to display
	// visualc does not allow bitfield types other than ints
	//NPCSTATE npcstate : 2;		// what npc is used
	//STATES state : 3;			// state of execution (externally visible states are OFF or STOP)
	unsigned npcstate : 2;		// what npc is used
	unsigned state : 3;			// state of execution (externally visible states are OFF or STOP)
	unsigned _unused : 1;

	const char *script;			// the executed programm
	size_t pos;					// position within the programm
	size_t start;				// starting stack pointer for the current command
	size_t end;					// end stack pointer for the current programm (end-start) = number of parameters
	size_t defsp;				// starting sp before running the command (for checking stack violations)

	
public:
	CValue	cExtData;			// additional data from external source

	struct map_session_data* sd;// the mapsession of the caller
	uint32 rid;			// bl.id of the hosting pc
	uint32 oid;			// bl.id of the executed npc



	///////////////////////////////////////////////////////////////////////////
public:
	CScriptEngine() : queue(NULL), stack_ptr(0),stack_max(0),stack_data(NULL),
		rerun_flag(false), messageopen(false), npcstate(NONE), state(OFF), script(NULL), sd(NULL)
	{ }
	CScriptEngine::~CScriptEngine()
	{
		if(queue) { delete(queue); queue=NULL; }
		if(stack_data) { delete[] stack_data; stack_data=NULL; }
		stack_ptr=0;
		stack_max=0;
	}

	void temporaty_close() //!! remove when c++ allocation is enabled
	{
		if(queue) { delete(queue); queue=NULL; }
		if(stack_data) { delete[] stack_data; stack_data=NULL; }
		stack_ptr=0;
		stack_max=0;
		rerun_flag=false;
		messageopen=false;
		npcstate=NONE;
		state=OFF;
		script=NULL;
		sd=NULL;
	}

	void temporaty_init() //!! remove when c++ allocation is enabled
	{
		queue=NULL;
		stack_ptr=0;
		stack_max=0;
		stack_data=NULL;
		rerun_flag=false;
		messageopen=false;
		npcstate=NONE;
		state=OFF;
		script=NULL;
		sd=NULL;
	}

	///////////////////////////////////////////////////////////////////////////
	// number of parameters
	size_t Arguments()	
	{
		return (end>start)?(end-start):0; 
	}
	///////////////////////////////////////////////////////////////////////////
	// stack access
	CValue&	operator[](size_t i)
	{
		if(start+i > end || start+i >stack_max)
		{	// return a dummy value if out of range
			static CValue dummy;
			dummy.type=C_INT;
			dummy.num=0;
			return dummy;
		}
		return stack_data[start+i];
	}

	void ConvertName(CValue &data);
	int GetInt(CValue &data);
	const char* GetString(CValue &data);

	void push_val(int type, int val);
	void push_str(int type, const char *str);
	void push_copy(size_t pos);
	void pop_stack(size_t start, size_t end);
private:
	///////////////////////////////////////////////////////////////////////////
	// check stacksize and reallocate if necessary
	void alloc()
	{
		if(!stack_data || (stack_ptr >= stack_max))
		{
			stack_max += 64;
			CValue *temp = new CValue[stack_max];
			if(stack_data)
			{
				for(size_t i=0; i<stack_ptr;++i)
					temp[i] << stack_data[i];
				delete [] stack_data;
			}
			stack_data = temp;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// operations
	void op_2int(int op);
	void op_2str(int op);
	void op_2(int op);
	void op_1(int op);
	int run_func();

	int run_main();

public:
	///////////////////////////////////////////////////////////////////////////
	// main entry points
	static int run(const char *rootscript, size_t pos, uint32 rid, uint32 oid);
	int restart(uint32 npcid)
	{	
		if( this->state==STOP && (npcid == this->oid || npcid == this->defoid) )
			return CScriptEngine::run(this->script, this->pos, this->rid, this->oid);
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	// checks the npc, create a default npc and spawn it when necessary
	uint32 send_defaultnpc(bool send=true);

	///////////////////////////////////////////////////////////////////////////
	// status access/query
	bool isRunning()	{ return OFF!=this->state; }

	bool clearAll()
	{	// stop and clear running/queued scripts (OnDeath)
		if( this->isRunning() )
		{
			if(this->queue) delete(this->queue);
			this->state	= OFF;
			this->script	= NULL;
			this->oid		= 0;
			this->clearMessage();
			// clear the npc, if the default npc was used
			if(this->npcstate == NPC_DEFAULT)
				this->send_defaultnpc(false);
		}
		return true;
	}

	bool isMessage()	{ return this->messageopen; }
	bool setMessage()	{ return (this->messageopen=true); }
	bool clearMessage()	{ return (this->messageopen=false); }
	void Quit()			{ this->state = END; }
	void Stop()			{ this->state = STOP; }
	void Return()		{ this->state = RETFUNC; }
	void EnvSwap()		{ this->state = ENVSWAP; }
	void Goto(size_t p)	
	{
		pos = p; 
		state = GOTO; 
	}
	bool Rerun()
	{
		if( 0==this->rerun_flag )
		{
			this->rerun_flag = 1;
			this->state = RERUNLINE;
		}
		else
			this->rerun_flag = 0;
		return this->rerun_flag;
	}

	///////////////////////////////////////////////////////////////////////////
	// friends with access to internals
	friend int buildin_callfunc(CScriptEngine &st);
	friend int buildin_callsub(CScriptEngine &st);
	friend int buildin_getarg(CScriptEngine &st);
	friend int buildin_return(CScriptEngine &st);
	friend int buildin_menu(CScriptEngine &st);
	friend int buildin_select(CScriptEngine &st);
	friend int buildin_input(CScriptEngine &st);
	friend int buildin_setarray(CScriptEngine &st);
	friend int buildin_if(CScriptEngine &st);
};






///////////////////////////////////////////////////////////////////////////////
// variables will change
struct script_reg
{
	int index;
	int data;

	// default constructor
	script_reg() : 
		index(0),
		data(0)
	{}
};

struct script_regstr
{
	int index;
	char data[256];

	// default constructor
	script_regstr() : 
		index(0)
	{
		data[0] = 0;
	}
};




char *parse_script(unsigned char *src, size_t line);

int set_var(const char *name, void *v);
int set_var(struct map_session_data &sd, const char *name, void *v);

struct dbt* script_get_label_db();
struct dbt* script_get_userfunc_db();

int script_config_read(const char *cfgName);
int do_init_script();
int do_final_script();

extern char mapreg_txt[256];

#endif

