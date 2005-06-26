
#ifndef _PARSER_H_
#define _PARSER_H_


#include "basestrings.h"








//
// Symbol Types
//
#define SymbolTypeNonterminal		0
#define SymbolTypeTerminal			1
#define SymbolTypeWhitespace		2
#define SymbolTypeEnd End Character	3
#define SymbolTypeCommentStart		4
#define SymbolTypeCommentEnd		5
#define SymbolTypeCommentLine		6
#define SymbolTypeError				7

#define EEOF 512

//
// Initial Buffer Sizes - Paul Hulskamp
//
#define STACK_SIZE 64
#define TOKEN_SIZE 32
#define RT_BUFF_SIZE 128
#define RT_INDEX_SIZE 32


// predeclarations

class CParser;
class CParseInput;
class CParseConfig;


class CSymbol
{
public:
	short		idx;
	short		Type;
	MiniString	Name;

	CSymbol() : idx(0),Type(0)
	{}
	~CSymbol()
	{}
	bool operator==(const CSymbol& a) const { return this==&a; }

	CSymbol(const CSymbol& s) : idx(s.idx),Type(s.Type),Name(s.Name)
	{}
	const CSymbol& operator=(const CSymbol& s)
	{
		idx=s.idx;
		Type=s.Type;
		Name=s.Name;
		return *this;
	}
	void clear()
	{
		idx=0;
		Type=0;
		Name.clear();
	}

};
class CRule
{
public:
	short				NonTerminal;
	TArrayDST<short>	cSymbol;

	CRule() : NonTerminal(0)	{}
	~CRule()					{}
	bool operator==(const CRule& a) const { return this==&a; }
};
class CEdge
{
public:
	short	CharSetIndex;
	short	TargetIndex;
	CEdge() : CharSetIndex(0),TargetIndex(0) {}
	~CEdge()	{}
	bool operator==(const CEdge& e) const { return this==&e; }
};
class CDFAState
{
public:
	char				Accept;
	short				AcceptIndex;
	TArrayDST<CEdge>	cEdge;

	CDFAState() : Accept(0),AcceptIndex(0)	{}
	~CDFAState()	{}
	bool operator==(const CDFAState& a) const { return this==&a; }
};
class CAction
{
public:
	short SymbolIndex;
	short Action;
	short Target;
	CAction() : SymbolIndex(0),Action(0),Target(0)	{}
	~CAction()	{}

	bool operator==(const CAction& a) const { return this==&a; }
};
class CLALRState
{
public:
	TArrayDST<CAction>	cAction;
	CLALRState()	{}
	~CLALRState()	{}
	bool operator==(const CLALRState& a) const { return this==&a; }
};
class CToken
{
public:
	short		id;
	MiniString	cLexeme;
	unsigned short	line;
	unsigned short	column;

	CToken() : id(0),line(0),column(0)	{}
	CToken(short i) : id(i), line(0),column(0)	{}
	CToken(short i, const MiniString& s)
		: id(i), cLexeme(s),line(0),column(0)	{}
	~CToken()	{}

	void clear()
	{
		id=0;
		cLexeme.clear();
	}
	bool operator==(const CToken& a) const { return this==&a; }

	CToken(const CToken& t)
		: id(t.id), cLexeme(t.cLexeme),line(t.line),column(t.column)
	{}
	const CToken& operator=(const CToken& t)
	{
		id=t.id;
		cLexeme=t.cLexeme;
		line=t.line;
		column=t.column;
		return *this;
	}

};

class CStackElement
{
public:
	CSymbol		symbol;
	CToken		cToken;
	short		state;
	short		rule;

	// Reduction Tree
	TArrayDST<short>	rtchildren;
	// End Reduction Tree

	CStackElement()
		: state(0),rule(0)
	{}
	~CStackElement()	{}

	CStackElement(const CStackElement& se)
		: symbol(se.symbol), cToken(se.cToken),state(se.state),rule(se.rule),rtchildren(se.rtchildren)
	{}
	const CStackElement& operator=(const CStackElement& se)
	{
		symbol=se.symbol;
		cToken=se.cToken;
		state=se.state;
		rule=se.rule;
		rtchildren=se.rtchildren;
		return *this;
	}
	bool operator==(const CStackElement& a) const { return this==&a; }

	void clear()
	{
		symbol.clear();
		cToken.clear();
		state=0;
		rule=0;
		rtchildren.resize(0);
	}

};


class CParseInput
{
public:
	///////////////////////////////////////////////////////////////////////////
	// input buffer
	char*	buf;		// buffer
	size_t	nbufsize;	// size of the buffer
	size_t	ncount;		// number of valid characters in the buffer
	size_t	nofs;		// offset of reading char
	size_t	nbufofs;	// offset of the buffer

	///////////////////////////////////////////////////////////////////////////
	// buffer control
	bool	bpreserve;

	///////////////////////////////////////////////////////////////////////////
	// position marker
	unsigned short	line;
	unsigned short	column;

	///////////////////////////////////////////////////////////////////////////
	// user function to get input data
	//
	// This function is called when the scanner needs more data
	//
	// If the pinput->bpreserve flag is set(because the scanner may need
	// to backtrack), then the data in the current input buffer must be
	// preserved.  This is done by increasing the buffer size and copying
	// the old data to the new buffer.
	// If the pinput->bpreserve flag is not set, the new data can be copied
	// over the existing buffer.
	//
	// If the input buffer is empty after this callback returns(because no
	// more data was added), the scanner function will return either:
	//   1] the last accepted token
	//   2] an eof, if no token has been accepted yet

	FILE *cFile;
	MiniString cName;

	virtual void cbgetinput()
	{
		size_t nr;
		if (!feof(cFile)) {
			if( this->bpreserve) {
				// copy the existing buffer to a new, larger one
				char* p = new char[2048 + this->ncount];
				this->nbufsize += 2048;
				memcpy(p, this->buf, this->ncount);
				delete[] this->buf;
				this->buf = p;
				nr = fread(this->buf+this->ncount, 1, 2048, cFile);
				this->ncount += nr; // increase the character count
			} else {
				nr = fread(this->buf, 1, this->nbufsize, cFile);
				this->nbufofs=this->nofs;
				//pinput.nofs = 0;					// reset the offset
				this->ncount = nr; // set the character count
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CParseInput()
		: buf(NULL),nbufsize(0),ncount(0),nofs(0),nbufofs(0),
		bpreserve(0),
		line(1), column(0),
		cFile(NULL)
	{
		buf = new char[2048];
		nbufsize = 2048;
	}
	~CParseInput()	{}

	///////////////////////////////////////////////////////////////////////////
	// file functions
	bool open(const char*filename)
	{
		close();
		cFile = fopen(filename,"rb");
		if(cFile)
		{
			cName = filename;
		}
		return NULL!=cFile;
	}
	bool close()
	{
		if(cFile)
		{	
			fclose(cFile);
			cName.clear();
		}
		return NULL==cFile;
	}
	///////////////////////////////////////////////////////////////////////////
	// access functions
	void next_char();	// increment to next character in input stream
	short get_char();	// get the next character from the input stream
	bool get_eof();		// check for eof on the input stream

	// get the next token
	short scan(CParser& parser, CToken& target); 

};







// parser configuration
class CParseConfig
{
	///////////////////////////////////////////////////////////////////////////
	friend class CParser;
public:
	///////////////////////////////////////////////////////////////////////////
	short			init_dfa;
	short			init_lalr;
	short			start_symbol;
	bool			case_sensitive;
	///////////////////////////////////////////////////////////////////////////
	TArrayDST<MiniString>	charset;
	TArrayDST<CDFAState>	dfa_state;
	TArrayDST<CSymbol>		sym;
	TArrayDST<CRule>		rule;
	TArrayDST<CLALRState>	lalr_state;

public:
	///////////////////////////////////////////////////////////////////////////
	CParseConfig()
		: init_dfa(0),init_lalr(0),start_symbol(0),case_sensitive(true)
	{  }
	CParseConfig(const char* filename)
		: init_dfa(0),init_lalr(0),start_symbol(0),case_sensitive(true)
	{ create(filename); }
	CParseConfig(unsigned char* buffer, size_t len)
		: init_dfa(0),init_lalr(0),start_symbol(0),case_sensitive(true)
	{ create(buffer, len); }

	///////////////////////////////////////////////////////////////////////////
	// create from file
	bool create(const char* filename);
	// create from memory
	bool create(unsigned char* buffer, size_t len);
	///////////////////////////////////////////////////////////////////////////
};

// current parse info
class CParser
{
public:
	bool					reduction;
	short					reduce_rule;
	short					lalr_state;

	CToken					cScanToken;
	TArrayDPT<CStackElement>	cStack;
//	TArrayDCT<CStackElement>	cStack;
	size_t					nstackofs;
	TArrayDPT<CToken>	tokens;
//	TArrayDCT<CToken>	tokens;

	CParseConfig*			pconfig;
	CParseInput				input;


	// Reduction Tree
	TArrayDPT<CStackElement>	rt;
//	TArrayDCT<CStackElement>	rt;
	// End Reduction Tree

	///////////////////////////////////////////////////////////////////////////
	CParser()
		: reduction(false),reduce_rule(0),lalr_state(0),
		nstackofs(0),pconfig(NULL)
	{}

	CParser(CParseConfig* pconfig)
		: reduction(false),reduce_rule(0),lalr_state(0),
		nstackofs(0),pconfig(NULL)
	{ create(pconfig); }


	virtual ~CParser()	{}


	///////////////////////////////////////////////////////////////////////////
	bool create(CParseConfig* pconfig);
	void reset();
	void reinit();

	short parse();
	void push_stack(short symValue, short* rtIdx, short nrtIdx);


	CStackElement* get_rt_entry(size_t idx);


	// Get the current lexeme
	const char*	get_lexeme()
	{
		return this->cScanToken.cLexeme;
	}
	// Set the current rule's lexeme
	void set_lexeme(const char* lexeme)
	{
		this->cScanToken.cLexeme = lexeme;
	}


	// Get a lexeme from the stack
	const char* get_child_lexeme(int index)
	{
		return this->cStack[this->nstackofs+index].cToken.cLexeme;
	}

	bool get_symbol(size_t syminx, CSymbol& symbol);


	void print_stack_element(CStackElement* se, int indent, const char sign);
	void print_rt_tree(int rtpos, int indent);
	void print_rt();
	void print_stack();
	

	///////////////////////////////////////////////////////////////////////////


	bool DefaultMatchFunction(short type, const MiniString& name, short symbol);
	virtual bool MatchFunction(short type, const MiniString& name, short symbol);

};








//
// Scan/Parse functions
//
			// default scanner match function
char scanner_def_matchtoken(CParser& parser, void* user, short type, const char* name, short symbol);





//
// C++ Wrapper class
//
class Parser
{
public:
	Parser(const char* config_filename)
	{
		m_config = 0; m_parser = 0;
		m_config = new CParseConfig(config_filename);
		if (m_config) m_parser = new CParser(m_config);
	}
	~Parser()
	{
		if (m_parser) delete m_parser;
		if (m_config) delete m_config;
		m_parser = 0; m_config = 0;
	}
	void reset()
	{
		m_parser->reset();
	}
	char isopen()
	{
		return (m_config && m_parser)?1:0;
	}
	short parse()
	{
		return m_parser->parse();
	}
	const char* get_child_lexeme(int idx)
	{
		return m_parser->get_child_lexeme(idx);
	}
	CStackElement* get_rt_element(short idx)
	{
		m_parser->get_rt_entry(idx);
	}
#ifdef _DEBUG
	void display_rt(){ m_parser->print_rt_tree(0,0);}
#endif // :DEBUG

	CParseConfig*	m_config;
	CParser*		m_parser;
};

#endif//_PARSER_H_
