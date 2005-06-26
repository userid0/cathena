
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "parser.h"



//
// Action Types
//
#define ActionShift		1
#define ActionReduce	2
#define ActionGoto		3
#define ActionAccept	4






///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Reduction Tree functions
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CParser::print_stack_element(CStackElement* se, int indent, const char sign)
{
	int i;
	printf("(%3i/%4i)", se->cToken.line, se->cToken.column);
	for(i=0;i<indent;++i)printf("|%c",sign);
	
	printf("%c-<%s>(%i)[%i] ::= %s (len=%i)\n", (se->symbol.Type)?'T':'N', (const char*)se->symbol.Name, se->symbol.idx, se->rtchildren.size(), (const char*)se->cToken.cLexeme, (se->cToken.cLexeme)?strlen(se->cToken.cLexeme):0);
}
void CParser::print_rt_tree(int rtpos, int indent)
{
	int j;
	CStackElement* se;
	int callindent = indent;

	se = &(this->rt[rtpos]);
	if( se->rtchildren.size()> 1 )
	{	
		callindent++;
		print_stack_element(se, indent, ' ');
	}

	if( this->rt[rtpos].rtchildren.size()>0 )
	{
		for(j=this->rt[rtpos].rtchildren.size()-1;j>=0;--j)
		{
			if (this->rt[this->rt[rtpos].rtchildren[j]].symbol.Type != 1) // non terminal
				this->print_rt_tree(this->rt[rtpos].rtchildren[j], callindent);
			else
			{
				se = &(this->rt[(this->rt[rtpos].rtchildren[j])]);
				this->print_stack_element(se, callindent, '-');
			}
		}
	}
}

void CParser::print_rt()
{
	size_t i,k;
printf ("print rt\n");
	for(i=0; i<this->rt.size(); i++)
	{
		printf("%03i - (%i) %s, childs:", i, this->rt[i].symbol.idx, (const char*)this->rt[i].symbol.Name);

		for(k=0; k<this->rt[i].rtchildren.size(); k++)
			printf("%i ", this->rt[i].rtchildren[k]);

		printf("\n");
	}
}

void CParser::print_stack()
{
	size_t i,k;
printf ("print stack\n");
	for(i=0; i<this->cStack.size(); i++)
	{
		printf("%03i - (%i) %s, childs:", i, this->cStack[i].symbol.idx, (const char*)this->cStack[i].symbol.Name);

		for(k=0; k<this->cStack[i].rtchildren.size(); k++)
			printf("%i ", this->cStack[i].rtchildren[k]);

		printf("\n");
	}
}








///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Scanner functions
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Default scanner match function
///////////////////////////////////////////////////////////////////////////////
bool CParser::DefaultMatchFunction(short type, const MiniString& name, short symbol)
{
	switch(type)
	{
	case SymbolTypeWhitespace:
		return false;
	default:
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////
// This function is called when the scanner matches a token
//
// Our implementation handles the "Comment Start" symbol, and scans through
// the input until the token closure is found
//
// All other tokens are refered to the default match function
///////////////////////////////////////////////////////////////////////////////
bool CParser::MatchFunction(short type, const MiniString& name, short symbol)
{
	short c;
	switch(type) {
	case SymbolTypeCommentLine: // ;
	{
		while((c = this->input.get_char()) != EEOF)
		{
			if(c == 13)
			{
				this->input.next_char();
				if ((c = this->input.get_char()) == 10)
				{
					this->input.next_char();
					return  false;
				}
				return  false;
			}
			this->input.next_char();
		}
		return false;
	}
	case SymbolTypeCommentStart: // /* */
		while((c = this->input.get_char()) != EEOF) {
			if (c == '*') {
				this->input.next_char();
				c = this->input.get_char();
				if (c != EEOF) {
					if (c == '/') {
						this->input.next_char();
						return false;
					}
				}
			}
			this->input.next_char();
		}
		return false;
	default:
		return this->DefaultMatchFunction(type,name,symbol);
	}
}








///////////////////////////////////////////////////////////////////////////////
// Get the next character from the input stream
///////////////////////////////////////////////////////////////////////////////
bool CParseInput::get_eof()
{
	if(this->nofs >= this->ncount+this->nbufofs)
			this->cbgetinput();
	return (this->nofs >= this->ncount+this->nbufofs);
}

///////////////////////////////////////////////////////////////////////////////
// Get the next character from the input stream
///////////////////////////////////////////////////////////////////////////////
short CParseInput::get_char()
{
	return this->get_eof()?EEOF:this->buf[this->nofs - this->nbufofs];
}
///////////////////////////////////////////////////////////////////////////////
// Get the next character from the input stream
///////////////////////////////////////////////////////////////////////////////
void CParseInput::next_char()
{
	if( this->nofs < this->ncount+this->nbufofs)
		this->nofs++;

	char c = this->buf[this->nofs-this->nbufofs];
	if( c==10 )
	{
		this->line++;
		this->column=0;
	}
	else if( c!=13 )
		this->column++;
}

///////////////////////////////////////////////////////////////////////////////
// Scan input for next token
///////////////////////////////////////////////////////////////////////////////
short CParseInput::scan(CParser& parser, CToken& target)
{
	const CDFAState* dfa;
	int start_ofs;
	int last_accepted;
	int last_accepted_size;
	char invalid = 0;

	dfa = &parser.pconfig->dfa_state[parser.pconfig->init_dfa];
	target.clear();
	target.line   = this->line;
	target.column = this->column;

	last_accepted = -1;
	last_accepted_size = 0;
	start_ofs = this->nofs;

	// check for eof
	this->bpreserve = 0;
	if(this->get_eof())
		return 0;

	while(1) {
		int i;
		short nedge;
		short c;
		short idx;

		// get char from input stream
		this->bpreserve = (last_accepted == -1)?0:1;
		c = this->get_char();
		this->bpreserve = 0;

		// convert to lower case
		if (!parser.pconfig->case_sensitive && c != EEOF)
			c = tolower(c);

		// look for a matching edge
		if (c != EEOF) {
			nedge = dfa->cEdge.size();
			for (i=0; i<nedge; i++) {
				idx = dfa->cEdge[i].CharSetIndex;
				if (strchr(parser.pconfig->charset[idx], c)) {
					dfa = &parser.pconfig->dfa_state[dfa->cEdge[i].TargetIndex];
					target.cLexeme.append( (char)c );
					if (dfa->Accept) {
						last_accepted = dfa->AcceptIndex;
						last_accepted_size = (this->nofs - start_ofs) + 1;
					}
					break;
				}
			}
		}
		if ((c == EEOF) || (i == nedge)) {
			// accept, ignore or invalid token
			if (last_accepted != -1) {
				if(last_accepted_size>0)
					target.cLexeme.resize(last_accepted_size);
				else
					printf("Warning negative Lexeme Resize, skipping\n");

				if( !parser.MatchFunction(parser.pconfig->sym[dfa->AcceptIndex].Type, parser.pconfig->sym[last_accepted].Name, (short)last_accepted) )
				{	// ignore, reset state
					target.clear();
					target.line   = this->line;
					target.column = this->column;

					if (c == EEOF || (last_accepted == -1))
						return 0;

					dfa = &parser.pconfig->dfa_state[parser.pconfig->init_dfa];
					last_accepted = -1;
					start_ofs = this->nofs;
					continue;
				}
			}
			break;
		}
		// move to next character
		this->next_char();
	}
	if (last_accepted == -1)
	{	// invalid
		target.clear();
		target.id=-1;
	}
	else
	{	// accept
		target.id=last_accepted;
	}
	return target.id;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Parser functions
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Get symbol info
///////////////////////////////////////////////////////////////////////////////
bool CParser::get_symbol(size_t syminx, CSymbol& symbol)
{
	if(syminx >= this->pconfig->sym.size())
		return false;
	symbol = this->pconfig->sym[syminx];
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Create a new parser
///////////////////////////////////////////////////////////////////////////////
bool CParser::create(CParseConfig* pconfig)
{	
	this->pconfig = pconfig;
	this->cStack.realloc(STACK_SIZE);

	this->lalr_state = pconfig->init_lalr;
	this->tokens.realloc(TOKEN_SIZE);

	// Reduction Tree
	this->rt.realloc(RT_BUFF_SIZE);
	this->rt.resize(1);// 0 is reserved as head
	// End Reduction Tree

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Reset the parser
///////////////////////////////////////////////////////////////////////////////
void CParser::reinit()
{	// basically clear stack and reduction tree
	// actually only the first stack element is the link to 
	// the current elements on the reduction tree
	// so deleting this stack element removes this link
	// but we need at least on of these elements on the stack 
	// because it contains the finalizing reduction rule
	// so we just empty the children list of the first and delete the others
	size_t i;
	for(i=0; i<this->cStack.size(); i++)
	{
		if( this->cStack[i].rtchildren.size()>0 )
		{
			if(i==0)// just clear the children in the first rule
				this->cStack[i].rtchildren.resize(0);
			else	// otherwise just remove the element from stack
				this->cStack.removeindex(i);
			break;
		}
	}

	// delete the complete reduction tree
	this->rt.resize(1);	// 0 is reserved as head
}


///////////////////////////////////////////////////////////////////////////////
// Reset the parser
///////////////////////////////////////////////////////////////////////////////
void CParser::reset()
{
	this->cStack.clear();
	this->tokens.clear();

	this->lalr_state = this->pconfig->init_lalr;
	this->reduce_rule = 0;
	this->nstackofs = 0;
	this->reduction = false;
	this->input.nofs = 0;
	this->input.nbufofs = 0;

	// Reduction Tree
	this->rt.clear();
	this->rt.resize(1);	// 0 is reserved as head
	// End Reduction Tree
}

///////////////////////////////////////////////////////////////////////////////
// Reduction Tree
///////////////////////////////////////////////////////////////////////////////
CStackElement* CParser::get_rt_entry(size_t idx)
{
	if(idx >= this->rt.size())
		return NULL;
	return &(this->rt[idx]);
}

///////////////////////////////////////////////////////////////////////////////
// Push an element onto the parse stack
///////////////////////////////////////////////////////////////////////////////
void CParser::push_stack(short symValue, short* rtIdx, short nrtIdx)
{
	size_t sz = this->cStack.size();

	CStackElement* se;
	this->cStack.realloc(1,STACK_SIZE);
	this->cStack.resize(sz+1);
	se = &this->cStack[sz];

	// get symbol information
	this->get_symbol(symValue, se->symbol);

	se->cToken    = this->cScanToken;
	se->cToken.id = symValue;

	se->state = this->lalr_state;
	se->rule = this->reduce_rule;

	se->rtchildren.clear();
	if(rtIdx && nrtIdx){
		se->rtchildren.append(rtIdx,nrtIdx);
		delete[] rtIdx;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Parse
///////////////////////////////////////////////////////////////////////////////
short CParser::parse()
{
	size_t i;
	char bfound;
	CParseInput* pinput = &this->input;

	if(this->reduction)
	{
/*		CRule* rule = &this->pconfig->rule[this->reduce_rule];
		short* rtIdx = 0;
		size_t nrtIdx = 0;

		// push onto token stack
		this->tokens.push( CToken(rule->NonTerminal) );

		if( rule->cSymbol.size() )
		{	// remove terminals from stack
			nrtIdx = rule->cSymbol.size();
			rtIdx = new short[nrtIdx];
			this->rt.realloc(nrtIdx, RT_BUFF_SIZE);
			for (i=0;i<nrtIdx;i++)
			{
				// save position of rt stack
				rtIdx[i] = this->rt.size(); 
				// revert lalr_state
				this->lalr_state = this->cStack.last().state;				
				// Push element onto reduction tree
				this->rt.push(this->cStack.last());

				this->cStack.pop();
			}
		}

		// push non-terminal onto stack
		this->push_stack(rule->NonTerminal, rtIdx, (short)nrtIdx);
*/
///////// changed the above
		CStackElement* se;
		CRule* rule = &this->pconfig->rule[this->reduce_rule];
		size_t nrtIdx = rule->cSymbol.size();
		size_t pos;

		// push onto token stack
		this->tokens.push( CToken(rule->NonTerminal) );
		
		if( this->cStack.size()>=nrtIdx )
		{
			pos = this->cStack.size()-nrtIdx;
		}
		else
		{
			pos = 0;
			nrtIdx = this->cStack.size();
		}

		// insert an element in the stack a the target position
		// all elements behind will be poped from stack and pushed on the rt tree
		// and inserted as child elements in the new inserted node
		this->cStack.realloc(1,STACK_SIZE);
		this->cStack.insert(pos);
		se = &this->cStack[pos];
		se->rtchildren.resize(nrtIdx);
		if(nrtIdx)
		{	// remove terminals from stack
			this->rt.realloc(nrtIdx, RT_BUFF_SIZE);
			for (i=0;i<nrtIdx;i++)
			{
				// revert lalr_state
				this->lalr_state = this->cStack.last().state;
				// save child position on rt stack
				se->rtchildren[i] = this->rt.size(); 
				// Push element onto reduction tree
				this->rt.push(this->cStack.last());

				this->cStack.pop();
			}
		}
		// build the other data of the new nonterminal

		// get symbol information
		this->get_symbol(rule->NonTerminal, se->symbol);
		se->cToken    = this->cScanToken;
		se->cToken.id = rule->NonTerminal;
		se->state = this->lalr_state;
		se->rule = this->reduce_rule;
////////// up to here

		// Reduction tree head (always this->rt[0])
		this->rt[0] = this->cStack.last();

		this->reduction = false;
	}

	while(1)
	{
		if(this->tokens.size()<1)
		{	// No input tokens on stack, grab one from the input stream
			if( pinput->scan(*this, this->cScanToken) < 0 )
				return -1;

			this->tokens.push( this->cScanToken );
		}
		else
		{	// Retrieve the last token from the input stack
			size_t sz=this->tokens.size();
			if(sz>0)
			{	
				this->cScanToken = this->tokens[sz-1];
			}
		}

		bfound = 0;
		for (i=0;(!bfound) && (i<this->pconfig->lalr_state[this->lalr_state].cAction.size());i++)
		{
			CAction* action = &this->pconfig->lalr_state[this->lalr_state].cAction[i];
			if(action->SymbolIndex == this->cScanToken.id) {
				bfound = 1;
				switch(action->Action) {
				case ActionShift:
				{	// Push a symbol onto the stack

				//	this->push_stack(action->SymbolIndex, 0, 0);
					CStackElement* se;
					size_t sz = this->cStack.size();

					this->cStack.realloc(1,STACK_SIZE);
					this->cStack.resize(sz+1);
					se = &this->cStack[sz];

					// get symbol information
					this->get_symbol(action->SymbolIndex, se->symbol);

					se->cToken    = this->cScanToken;
					se->cToken.id = action->SymbolIndex;

					se->state = this->lalr_state;
					se->rule = this->reduce_rule;
					se->rtchildren.clear();

					this->nstackofs = this->cStack.size()-1;
					this->lalr_state = action->Target;
					// pop token from stack
					this->tokens.pop();
					break;
				}
				case ActionReduce:
				{	// Reducing a rule is done in two steps:
					// 1] Setup the stack offset so the calling function
					//    can reference the rule's child lexeme values when
					//    this action returns
					// 2] When this function is called again, we will
					//    remove the child lexemes from the stack, and replace
					//    them with an element representing this reduction
					//
					CRule* rule = &this->pconfig->rule[action->Target];
					this->cScanToken.clear();

					this->reduce_rule = action->Target;
					if( this->cStack.size() > rule->cSymbol.size() )
						this->nstackofs = this->cStack.size() - rule->cSymbol.size();
					else
						this->nstackofs = 0;
					this->reduction = true;
					return rule->NonTerminal;
				}
				case ActionGoto:
				{	// Shift states
					this->lalr_state = action->Target;
					this->tokens.pop();
					break;
				}
				case ActionAccept:
					// Eof, the main rule has been accepted
					return 0;
				} // switch
			} // if
		} // for
		if (!bfound)
		{
			if(this->cScanToken.id)
				break;
			return 0; // eof
		}
	} // while
	// token not found in rule
	return -1;
}





///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Parse Configuration Class
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Helper functions for loading .cgt files
///////////////////////////////////////////////////////////////////////////////
unsigned char* getws(unsigned char* b, char* s)
{
	while(*s++ = *b++) b++;
	b++; return b;
}
unsigned char* getsh(unsigned char* b, short* s)
{
	*s = *b++;
	*s |= (*b++) << 8;
	return b;
}
unsigned char* getvws(unsigned char* b, char* str)
{
	b++; return getws(b,str);
}
unsigned char* skipvws(unsigned char* b)
{
	b++; while(*b++) b++;
	return ++b;
}
unsigned char* getvb(unsigned char* b, unsigned char* v)
{
	b++; *v = *b++;
	return b;
}
unsigned char* getvsh(unsigned char* b, short* v)
{
	b++; return getsh(b,v);
}

///////////////////////////////////////////////////////////////////////////////
// Load a parse table from memory
///////////////////////////////////////////////////////////////////////////////
bool CParseConfig::create(unsigned char* b, size_t len)
{
	char str[1024];
	unsigned char* bend;
	short nEntries;
	unsigned char recType;
	short idx;
	unsigned char byt;
	size_t i;

	if(!b || !len) return false;
	bend = b + len;

	// get header
	b = getws(b, str);

	// check header
	if(0!=strcmp(str, "GOLD Parser Tables/v1.0"))
		return false;

	// read records until eof
	while(b < bend)
	{
		b++; // skip record id

		// read number of entries in record
		b = getsh(b, &nEntries);

		// read record type
		b = getvb(b, &recType);

		switch(recType) {
		case 'P': // Parameters
		{
			b = skipvws(b); // Name
			b = skipvws(b); // Version
			b = skipvws(b); // Author
			b = skipvws(b); // About
			b = getvb(b, &byt); // Case Sensitive?
			b = getvsh(b, &this->start_symbol); // Start Symbol
			this->case_sensitive = byt?true:false;
			break;
		}
		case 'T': // Table Counts
		{
			short dummy;
			b = getvsh(b, &dummy);
			this->sym.resize(dummy);

			b = getvsh(b, &dummy);
			this->charset.resize(dummy);

			b = getvsh(b, &dummy);
			this->rule.resize(dummy);

			b = getvsh(b, &dummy);
			this->dfa_state.resize(dummy);
			
			b = getvsh(b, &dummy);
			this->lalr_state.resize(dummy);
			break;
		}
		case 'I': // Initial States
		{
			b = getvsh(b, &this->init_dfa);
			b = getvsh(b, &this->init_lalr);
			break;
		}
		case 'S': // Symbol Entry
		{
			b = getvsh(b, &idx);
			b = getvws(b, str);
			b = getvsh(b, &this->sym[idx].Type);
			this->sym[idx].Name = str;
			this->sym[idx].idx = idx;
			break;
		}
		case 'C': // Character set Entry
		{
			b = getvsh(b, &idx);
			b = getvws(b, str);
			this->charset[idx] = str;
			break;
		}
		case 'R': // Rule Table Entry
		{
			b = getvsh(b, &idx);
			b = getvsh(b, &this->rule[idx].NonTerminal);
			b++; // reserved

			this->rule[idx].cSymbol.resize(((nEntries-4)>0)?(nEntries-4):0);
			for(i=0;i<this->rule[idx].cSymbol.size();i++)
				b = getvsh(b, &(this->rule[idx].cSymbol[i]));
			break;
		}
		case 'D': // DFA State Entry
		{
			b = getvsh(b, &idx);
			b = getvb(b, &byt);
			b = getvsh(b, &this->dfa_state[idx].AcceptIndex);
			this->dfa_state[idx].Accept = byt?1:0;
			b++; // reserved
			this->dfa_state[idx].cEdge.resize(((nEntries-5)/3)>0?((nEntries-5)/3):0);
			for (i=0; i<this->dfa_state[idx].cEdge.size(); i++)
			{
				b = getvsh(b, &this->dfa_state[idx].cEdge[i].CharSetIndex);
				b = getvsh(b, &this->dfa_state[idx].cEdge[i].TargetIndex);
				b++; // reserved
			}
			break;
		}
		case 'L': // LALR State Entry
		{
			b = getvsh(b, &idx);
			b++; // reserved
			this->lalr_state[idx].cAction.resize((((nEntries-3)/4)>0)?((nEntries-3)/4):0);
			for (i=0;i<this->lalr_state[idx].cAction.size();i++)
			{
				b = getvsh(b, &this->lalr_state[idx].cAction[i].SymbolIndex);
				b = getvsh(b, &this->lalr_state[idx].cAction[i].Action);
				b = getvsh(b, &this->lalr_state[idx].cAction[i].Target);
				b++; // reserved
			}
			break;
		}
		default: // unknown record
			return false;
		}
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////
// Load a parse table from a file
///////////////////////////////////////////////////////////////////////////////
bool CParseConfig::create(const char* filename)
{
	unsigned char* buf;
	FILE* fin;
	bool result=false;
#ifdef _WIN32
	struct _stat st;
	if (_stat(filename, &st) == -1) return false;
#else
	struct stat st;
	if (stat(filename, &st) == -1) return false;
#endif
	fin = fopen(filename, "rb");
	if(fin)
	{
		buf = new unsigned char[st.st_size];
		if(buf)
		{
			fread(buf, st.st_size, 1, fin);
			result = this->create(buf, st.st_size);
			delete[] buf;
		}
		fclose(fin);
	}
	return result;
}


