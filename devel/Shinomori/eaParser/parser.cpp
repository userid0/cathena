
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

//
// Forward declarations
//
void _pop_token(struct _parser* parser);
void _pop_stack(struct _parser* parser);

// Reduction Tree
short _push_rt_element(struct _parser* parser, struct _stack_element* se);
void _check_rt_buffer(struct _parser* parser);
void _set_rt_head(struct _parser* parser, struct _stack_element* se);








//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//
// Reduction Tree functions
//
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////


void print_stack_element(struct _stack_element* se, int indent, const char sign)
{
	int i;
	printf("(%03i)", se->line);
	for(i=0;i<indent;++i)printf("|%c",sign);
	
	printf("%c-<%s>(%i)[%i] ::= %s\n", (se->symbol.Type)?'T':'N', se->symbol.Name, se->symbol.idx, se->nrtchild, se->token.lexeme);
}
void print_rt_tree(struct _parser* parser, int rtpos, int indent)
{
	int j;
	struct _stack_element* se;
	int callindent = indent;

	se = parser->rt+rtpos;
	if( se->nrtchild > 1 )
	{	
		callindent++;
		print_stack_element(se, indent, ' ');
	}

	//for(j=0;j<parser->rt[rtpos].nrtchild;j++)
	for(j=parser->rt[rtpos].nrtchild-1;j>=0;--j)
	{
		if (parser->rt[parser->rt[rtpos].rtchildren[j]].symbol.Type != 1) // non terminal
			print_rt_tree(parser, parser->rt[rtpos].rtchildren[j], callindent);
		else
		{
			se = parser->rt+(parser->rt[rtpos].rtchildren[j]);
//			if( se->token.lexeme[0] != 0 )
			{
				print_stack_element(se, callindent, '-');
			}
		}
	}
	se = parser->rt+rtpos;
	if( se->nrtchild > 1 )
	{
//		print_stack_element(se, indent, ' ');
	}
}









//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//
// Scanner functions
//
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

// Default scanner match function
char scanner_def_matchtoken(struct _parser* parser, void* user, short type, char* name, short symbol)
{
	short c;
	switch(type) {
	case SymbolTypeCommentLine: // ;
		while((c = scanner_get_char(parser)) != EEOF) {
			if (c == 13) {
				scanner_next_char(parser);
				if ((c = scanner_get_char(parser)) == 10) {
					scanner_next_char(parser);
					return 0;
				}
				return 0;
			}
			scanner_next_char(parser);
		}
		return 0;

	case SymbolTypeWhitespace:
		return 0;
	default:
		return 1;
	}
}

// Get the next character from the input stream
char scanner_get_eof(struct _parser* parser)
{
	struct _parse_input* pinput = &parser->input;
	if (pinput->nofs >= pinput->ncount && pinput->cbneedinput)
			pinput->cbneedinput(pinput);
	return (pinput->nofs >= pinput->ncount)?1:0;
}

// Get the next character from the input stream
short scanner_get_char(struct _parser* parser)
{
	return scanner_get_eof(parser)?EEOF:parser->input.buf[parser->input.nofs];
}

// Get the next character from the input stream
void scanner_next_char(struct _parser* parser)
{
	if (parser->input.nofs < parser->input.ncount)
		parser->input.nofs++;

	if( parser->input.buf[parser->input.nofs]==10)
	{
		parser->line++;
	}
}

//
// Scan input for next token
//
short	scanner_scan(struct _parser* parser)
{
	struct _dfa_state* dfa;
	struct _parse_config* pconfig;
	struct _parse_input* pinput;
	char* lexeme;
	int start_ofs;
	int last_accepted;
	int last_accepted_size;
	char invalid = 0;

	pinput = &parser->input;
	lexeme = parser->lexeme;
	pconfig = parser->pconfig;
	dfa = &pconfig->dfa_state[pconfig->init_dfa];
	lexeme[0] = 0;
	last_accepted = -1;
	last_accepted_size = 0;
	start_ofs = pinput->nofs;

	// check for eof
	parser->input.bpreserve = 0;
	if (scanner_get_eof(parser))
		return 0;

	while(1) {
		int i;
		short nedge;
		short c;
		short idx;

		// get char from input stream
		parser->input.bpreserve = (last_accepted == -1)?0:1;
		c = scanner_get_char(parser);
		parser->input.bpreserve = 0;

		// convert to lower case
		if (!pconfig->case_sensitive && c != EEOF)
			c = tolower(c);

		// look for a matching edge
		if (c != EEOF) {
			nedge = dfa->nedge;
			for (i=0; i<nedge; i++) {
				idx = dfa->edge[i].CharSetIndex;
				if (strchr(pconfig->charset[idx], c)) {
					dfa = &pconfig->dfa_state[dfa->edge[i].TargetIndex];
					*lexeme++ = (char)c;
					if (dfa->Accept) {
						last_accepted = dfa->AcceptIndex;
						last_accepted_size = (pinput->nofs - start_ofs) + 1;
					}
					break;
				}
			}
		}
		if ((c == EEOF) || (i == nedge)) {
			// accept, ignore or invalid token
			if (last_accepted != -1) {
				_cbmatchtoken m = parser->cbmatchtoken?parser->cbmatchtoken:scanner_def_matchtoken;
				parser->lexeme[last_accepted_size] = 0;
				if ( !m(parser, parser->user_callback, pconfig->sym[dfa->AcceptIndex].Type, pconfig->sym[last_accepted].Name, (short)last_accepted) ) {
					// ignore, reset state
					lexeme = parser->lexeme; lexeme[0] = 0;
					if (c == EEOF || (last_accepted == -1)) return 0;
					dfa = &pconfig->dfa_state[pconfig->init_dfa];
					last_accepted = -1;
					start_ofs = parser->input.nofs;
					continue;
				}
			}
			break;
		}

		// move to next character
		scanner_next_char(parser);
	}
	if (last_accepted == -1) {
		// invalid
		lexeme = parser->lexeme; lexeme[0] = 0;
		return -1;
	}
	// accept
	return last_accepted;
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//
// Parser functions
//
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

// Get symbol info
char parser_get_symbol(struct _parser* parser, int symbol, struct _symbol* psymbol)
{
	if (symbol >= parser->pconfig->nsym)
		return 0;
	psymbol->idx  = parser->pconfig->sym[symbol].idx;
	psymbol->Name = strdup(parser->pconfig->sym[symbol].Name);
	psymbol->Type = parser->pconfig->sym[symbol].Type;
	
	return 1;
}

//
// Get a lexeme from the stack
//
const char* parser_get_child_lexeme(struct _parser* parser, int index)
{
	return parser->stack[parser->nstackofs+index].token.lexeme;
}

//
// Get a value from the stack
//
void* parser_get_child_userdata(struct _parser* parser, int index)
{
	return parser->stack[parser->nstackofs+index].user_data;
}

//
// Get the current lexeme
//
const char*	parser_get_lexeme(struct _parser* parser)
{
	return parser->lexeme;
}

//
// Get the current value
//
void*	parser_get_userdata(struct _parser* parser)
{
	return parser->symbol_userdata;
}

//
// Set the current value
//
void parser_set_userdata(struct _parser* parser, void* value)
{
	parser->symbol_userdata = value;
}

//
// Set the current rule's lexeme
//
void parser_set_lexeme(struct _parser* parser, const char* lexeme, void* value)
{
	strcpy(parser->lexeme, lexeme);
	parser->symbol_userdata = value;
}

//
// Create a new parser
//
struct _parser* parser_create(struct _parse_config* pconfig)
{	
//	int i;
	struct _parser* parser = (struct _parser*)malloc(sizeof(struct _parser));
	memset(parser, 0, sizeof(struct _parser));
	parser->pconfig = pconfig;
	parser->nstacksize = STACK_SIZE;
	parser->stack = (struct _stack_element*)malloc(sizeof(struct _stack_element)*STACK_SIZE);
	memset(parser->stack, 0, sizeof(struct _stack_element)*STACK_SIZE);
	parser->lalr_state = pconfig->init_lalr;
	parser->nlexeme = 512;
	parser->lexeme = (char*)malloc(512);
	parser->lexeme[0] = 0;
	parser->input.parser = parser;
	parser->input.buf = NULL;
	parser->input.nbufsize = 0;
	parser->input.ncount = 0;
	parser->input.nofs = 0;
	parser->input.cbneedinput = NULL;
	parser->input.user = NULL;
	parser->tokens = (struct _token*)malloc(sizeof(struct _token)*TOKEN_SIZE);

	// Reduction Tree
	parser->rtofs = 1; // 0 is reserved as head
	parser->rtsize = RT_BUFF_SIZE;
	parser->rt = (struct _stack_element*)malloc(sizeof(struct _stack_element)*parser->rtsize);
	memset((void*)parser->rt, 0, sizeof(struct _stack_element)*parser->rtsize);
	// End Reduction Tree

	return parser;
}

//
// delete the parser state
//
void parser_delete(struct _parser* parser)
{
	int i;
	free(parser->lexeme);
	if (parser->stack)
		free(parser->stack);
	while(parser->ntokens>0)
		_pop_token(parser);
	free(parser->tokens);
	memset(parser,0,sizeof(struct _parser));

	// Reduction Tree
	for (i=0;i<parser->rtofs;++i){
		if (parser->rt[i].token.lexeme) free(parser->rt[i].token.lexeme);
		if (parser->rt[i].symbol.Name) free(parser->rt[i].symbol.Name);
		if (parser->rt[i].rtchildren) free(parser->rt[i].rtchildren);
	}
	free(parser->rt);
	// End Reduction Tree
}

//
// Reset the parser
//
void	parser_reset_state(struct _parser* parser)
{
	int i;
	while(parser->nstack>0)
		_pop_stack(parser);
	while(parser->ntokens>0)
		_pop_token(parser);
	parser->lalr_state = parser->pconfig->init_lalr;
	parser->reduce_rule = 0;
	parser->nstack = 0;
	parser->nstackofs = 0;
	parser->reduction = 0;
	parser->input.nofs = 0;

	// Reduction Tree
	for (i=0;i<parser->rtofs;++i){
		if (parser->rt[i].token.lexeme) free(parser->rt[i].token.lexeme);
		if (parser->rt[i].symbol.Name) free(parser->rt[i].symbol.Name);
		if (parser->rt[i].rtchildren) free(parser->rt[i].rtchildren);
		parser->rt[i].token.lexeme = 0;
		parser->rt[i].symbol.Name = 0;
		parser->rt[i].rtchildofs = 0;
	}
	memset((void*)parser->rt, 0, sizeof(struct _stack_element)*parser->rtsize);
	parser->rtofs = 1; // 0 is reserved as head
	// End Reduction Tree
}

//
// Load a parse table from a file
//
struct _parse_config* parser_config_create_file(const char* filename)
{
	char* buf;
	FILE* fin;
	struct _parse_config* result;
#ifdef WIN32
	struct _stat st;
	if (_stat(filename, &st) == -1) return 0;
#else
	struct stat st;
	if (stat(filename, &st) == -1) return 0;
#endif
	fin = fopen(filename, "rb");
	if (!fin) return 0;
	buf = (char*)malloc(st.st_size);
	if (!buf) {fclose(fin); return 0;}
	fread(buf, st.st_size, 1, fin);
	fclose(fin);
	result = parser_config_create_mem(buf, st.st_size);
	free(buf);
	return result;
}


//
// Reduction Tree
//
struct _stack_element* get_rt_entry(struct _parser* parser, short idx){
	if (!parser) return 0;
	if (idx < 0 || idx >= parser->rtofs) return 0;
	return parser->rt + idx;
	
}

short _push_rt_element(struct _parser* parser, struct _stack_element* se){
	_check_rt_buffer(parser);
	memcpy((void*)(parser->rt+(parser->rtofs)), (const void*)se, sizeof(struct _stack_element));
	if (se->symbol.Name)
		parser->rt[parser->rtofs].symbol.Name = strdup(se->symbol.Name);
	if (se->token.lexeme)
		parser->rt[parser->rtofs].token.lexeme = strdup(se->token.lexeme);
	if (se->rtchildren){
		// Only copy essential part of rt index buff
		parser->rt[parser->rtofs].rtchildren = (short*)malloc(sizeof(short)*se->rtchildofs);
		memcpy((void*)parser->rt[parser->rtofs].rtchildren, se->rtchildren, sizeof(short)*se->rtchildofs);
		parser->rt[parser->rtofs].nrtchild = se->nrtchild;
		parser->rt[parser->rtofs].rtchildofs = se->rtchildofs;
	}
	parser->rtofs++;
	return parser->rtofs-1;
}

void _check_rt_buffer(struct _parser* parser){
//
// Check and grow rt buffer if necessary
//
	if (parser->rtofs >= parser->rtsize) {
		parser->rtsize += RT_BUFF_SIZE;
		parser->rt = (struct _stack_element *)realloc((void*)parser->rt, (sizeof(struct _stack_element)*parser->rtsize));
	}
}

void _set_rt_head(struct _parser* parser, struct _stack_element* se){

	if (!parser || !se) return;
	if (parser->rt->symbol.Name)
		free(parser->rt->symbol.Name);
	if (parser->rt->token.lexeme)
		free(parser->rt->token.lexeme);
	if (parser->rt->rtchildren)
		free(parser->rt->rtchildren);
	parser->rt->symbol.Type = se->symbol.Type;
	parser->rt->token.id = se->token.id;
	parser->rt->user_data = se->user_data;
	parser->rt->rule = se->rule;
	parser->rt->state = se->state;

	parser->rt->symbol.idx = se->symbol.idx;
	if (se->symbol.Name)
		parser->rt->symbol.Name = strdup(se->symbol.Name);
	if (se->token.lexeme)
		parser->rt->token.lexeme = strdup(se->token.lexeme);
	if (se->rtchildren){
		parser->rt->rtchildren = (short*)malloc(sizeof(short)*se->rtchildofs);
		memcpy((void*)parser->rt->rtchildren, (const void*)se->rtchildren, sizeof(short)*se->rtchildofs);
		parser->rt->nrtchild = se->nrtchild;
		parser->rt->rtchildofs = se->rtchildofs;
	} else {
		parser->rt->rtchildren = 0;
		parser->rt->nrtchild = 0;
		parser->rt->rtchildofs = 0;
	}
}

//
// Helper functions for loading .cgt file
//
char* getws(char* b, char* s) {
	while(*s++ = *b++) b++;
	b++; return b;
}
char* getsh(char* b, short* s) {
	unsigned char* _b = (unsigned char*)b;
	*s = *_b++;
	*s |= (*_b++) << 8;
	return (char*)_b;
}
char* getvws(char* b, char* str) {
	b++; return getws(b,str);
}
char* skipvws(char* b) {
	b++; while(*b++) b++;
	return ++b;
}
char* getvb(char* b, unsigned char* v) {
	b++; *v = *(unsigned char*)b++;
	return b;
}
char* getvsh(char* b, short* v) {
	b++; return getsh(b,v);
}

//
// Load a parse table from memory
//
struct _parse_config* parser_config_create_mem(char* b, int len)
{
	char		str[1024];
	struct _parse_config*	res;
	char*		bend;
	short		nEntries;
	unsigned char recType;
	short		idx;
	unsigned char byt;
	int			i;

	if (!b || !len) return 0;
	bend = b + len;

	// get header
	b = getws(b, str);

	// check header
	if (strcmp(str, "GOLD Parser Tables/v1.0"))
		return 0;

	// create parser object
	res = (struct _parse_config*)malloc(sizeof(struct _parse_config));
	if (!res) return 0;

	// read records until eof
	while(b < bend) {
		b++; // skip record id

		// read number of entries in record
		b = getsh(b, &nEntries);

		// read record type
		b = getvb(b, &recType);

		switch(recType) {
		case 'P': // Parameters
			b = skipvws(b); // Name
			b = skipvws(b); // Version
			b = skipvws(b); // Author
			b = skipvws(b); // About
			b = getvb(b, &byt); // Case Sensitive?
			b = getvsh(b, &res->start_symbol); // Start Symbol
			res->case_sensitive = byt?1:0;
			break;
		case 'T': // Table Counts
			{
			b = getvsh(b, &res->nsym);
			b = getvsh(b, &res->ncharset);
			b = getvsh(b, &res->nrule);
			b = getvsh(b, &res->ndfa_state);
			b = getvsh(b, &res->nlalr_state);

			// reserve memory
			res->charset = (const char**)malloc(sizeof(char*) * res->ncharset);
			res->dfa_state = (struct _dfa_state*)malloc(sizeof(struct _dfa_state)*res->ndfa_state);
			res->sym = (struct _symbol*)malloc(sizeof(struct _symbol)*res->nsym);
			res->rule = (struct _rule*)malloc(sizeof(struct _rule)*res->nrule);
			res->lalr_state = (struct _lalr_state*)malloc(sizeof(struct _lalr_state)*res->nlalr_state);
			}
			break;
		case 'I': // Initial States
			b = getvsh(b, &res->init_dfa);
			b = getvsh(b, &res->init_lalr);
			break;
		case 'S': // Symbol Entry
			{
			b = getvsh(b, &idx);
			b = getvws(b, str);
			b = getvsh(b, &res->sym[idx].Type);
			res->sym[idx].Name = strdup(str);
			res->sym[idx].idx = idx;
			}
			break;
		case 'C': // Character set Entry
			{
			int slen;
			b = getvsh(b, &idx);
			b = getvws(b, str);
			slen = (int)strlen(str);
			res->charset[idx] = (char*)strdup(str);
			}
			break;
		case 'R': // Rule Table Entry
			b = getvsh(b, &idx);
			b = getvsh(b, &res->rule[idx].NonTerminal);
			b++; // reserved
			res->rule[idx].nsymbol = nEntries-4;
			res->rule[idx].symbol = (short*)malloc(sizeof(struct _symbol)*(nEntries-4));
			for (i=0;i<nEntries-4;i++)
				b = getvsh(b, &res->rule[idx].symbol[i]);
			break;
		case 'D': // DFA State Entry
			b = getvsh(b, &idx);
			b = getvb(b, &byt);
			b = getvsh(b, &res->dfa_state[idx].AcceptIndex);
			res->dfa_state[idx].Accept = byt?1:0;
			b++; // reserved
			res->dfa_state[idx].nedge = (nEntries-5)/3;
			res->dfa_state[idx].edge = (struct _edge*)malloc(sizeof(struct _edge)*((nEntries-5)/3));
			for (i=0; i<nEntries-5; i+=3) {
				b = getvsh(b, &res->dfa_state[idx].edge[i/3].CharSetIndex);
				b = getvsh(b, &res->dfa_state[idx].edge[i/3].TargetIndex);
				b++; // reserved
			}
			break;
		case 'L': // LALR State Entry
			b = getvsh(b, &idx);
			b++; // reserved
			res->lalr_state[idx].naction = (nEntries-3)/4;
			res->lalr_state[idx].action = (struct _action*)malloc(sizeof(struct _action)*((nEntries-3)/4));
			for (i=0;i<nEntries-3;i+=4) {
				b = getvsh(b, &res->lalr_state[idx].action[i/4].SymbolIndex);
				b = getvsh(b, &res->lalr_state[idx].action[i/4].Action);
				b = getvsh(b, &res->lalr_state[idx].action[i/4].Target);
				b++; // reserved
			}
			break;

		default: // unknown record
			return 0;
		}
	}
	return res;
}

//
// Free parser
//
void parser_config_delete(struct _parse_config* pconfig)
{
	int i;
	for (i=0;i<pconfig->ncharset;i++)
		free((void*)pconfig->charset[i]);
	for (i=0;i<pconfig->nrule;i++)
		free(pconfig->rule[i].symbol);
	for (i=0;i<pconfig->ndfa_state;i++)
		free(pconfig->dfa_state[i].edge);
	for (i=0;i<pconfig->nlalr_state;i++)
		free(pconfig->lalr_state[i].action);
	for (i=0;i<pconfig->nsym;i++)
		free(pconfig->sym[i].Name);
	free((char**)pconfig->charset);
	free(pconfig->dfa_state);
	free(pconfig->sym);
	free(pconfig->rule);
	free(pconfig->lalr_state);
}

//
// Check and grow parser stack if necessary
//
void _check_stack(struct _parser* parser)
{
	if (parser->nstack >= parser->nstacksize) {
		struct _stack_element* stack = (struct _stack_element*)malloc(sizeof(struct _stack_element)*(parser->nstacksize + STACK_SIZE));
		memcpy(stack, parser->stack, sizeof(struct _stack_element)*parser->nstacksize);
		parser->nstacksize += STACK_SIZE;
		free(parser->stack);
		parser->stack = stack;
	}
}

//
// Pop an element from the stack
//
void _pop_stack(struct _parser* parser)
{
	if (parser->nstack < 1) return;
	parser->nstack--;
	if (parser->stack[parser->nstack].token.lexeme)
	{
		free(parser->stack[parser->nstack].token.lexeme);
		parser->stack[parser->nstack].token.lexeme=NULL;
	}
	if (parser->stack[parser->nstack].symbol.Name)
	{
		free(parser->stack[parser->nstack].symbol.Name);
		parser->stack[parser->nstack].symbol.Name=NULL;
	}
	if (parser->stack[parser->nstack].rtchildren)
	{
		free(parser->stack[parser->nstack].rtchildren);
		parser->stack[parser->nstack].rtchildren = NULL;
		parser->stack[parser->nstack].nrtchild = 0;
		parser->stack[parser->nstack].rtchildofs = 0;
	}
}

//
// Push an element onto the parse stack
//
void _push_stack(struct _parser* parser, short symValue, struct _symbol *sym, short* rtIdx, short nrtIdx)
{
	struct _stack_element* se;
	_check_stack(parser);
	se = &parser->stack[parser->nstack++];

	se->symbol.idx  = sym->idx;
	se->symbol.Name = strdup(sym->Name);
	se->symbol.Type = sym->Type;
	se->user_data = parser->symbol_userdata;
	se->token.id = symValue;
	se->token.lexeme = strdup(parser->lexeme);
	se->state = parser->lalr_state;
	se->rule = parser->reduce_rule;

	se->line = parser->line;

	se->nrtchild = 0;
	se->rtchildren = 0;
	se->rtchildofs = 0;
	if (rtIdx && nrtIdx){
		se->rtchildren = rtIdx; 
		se->nrtchild = nrtIdx;
		se->rtchildofs = nrtIdx;
	}	
}

//
// Push a token onto the token input stack
//
void _push_token(struct _parser* parser, short symbol, char* lexeme)
{
	parser->tokens[parser->ntokens].id = symbol;
	parser->tokens[parser->ntokens].lexeme = lexeme?strdup(lexeme):0;
	parser->ntokens++;
}

//
// Pop a token from the token input stack
//
void _pop_token(struct _parser* parser)
{
	if (parser->ntokens<1) return;
	if (parser->tokens[parser->ntokens-1].lexeme)
		free(parser->tokens[parser->ntokens-1].lexeme);
	parser->ntokens--;
}

//
// Get the top token from the input stack
//
void _last_token(struct _parser* parser)
{
	if (parser->ntokens<1) return;
	parser->symbol = parser->tokens[parser->ntokens-1].id;
	if (parser->tokens[parser->ntokens-1].lexeme)
		strcpy(parser->lexeme, parser->tokens[parser->ntokens-1].lexeme);
}

//
// Parse
//
short parser_parse(struct _parser* parser)
{
	int i;
	char bfound;
	struct _parse_input* pinput = &parser->input;

	if (parser->reduction) {
		struct _symbol sym;
		struct _rule* rule = &parser->pconfig->rule[parser->reduce_rule];
		short * rtIdx = 0;
		int nrtIdx = 0;

		parser->symbol = rule->NonTerminal;

		// push onto token stack
		_push_token(parser, parser->symbol, 0);

		if (rule->nsymbol){
			// remove terminals from stack
			nrtIdx = rule->nsymbol;
			rtIdx = (short*)malloc(sizeof(short)*nrtIdx);
			for (i=0;i<rule->nsymbol;i++){
				// Push element onto reduction tree
				rtIdx[i] = _push_rt_element(parser, parser->stack+(parser->nstack-1));
				_pop_stack(parser);
			}

			// revert lalr_state
			parser->lalr_state = parser->stack[parser->nstack].state;
		}
		// get symbol information
		parser_get_symbol(parser, parser->symbol, &sym);

		// push non-terminal onto stack
		_push_stack(parser, rule->NonTerminal, &sym, rtIdx, (short)nrtIdx);

		// Reduction tree head (always parser->rt[0])
		_set_rt_head(parser, parser->stack+(parser->nstack-1));
		if (sym.Name) free(sym.Name);

		parser->reduction = 0;
	}

	while(1) {
		if (parser->ntokens<1) {
			// No input tokens on stack, grab one from the input stream
			if ((parser->symbol = scanner_scan(parser)) < 0)
				return -1;
			_push_token(parser, parser->symbol, parser->lexeme);
		} else
			// Retrieve the last token from the input stack
			_last_token(parser);

		bfound = 0;
		for (i=0;(!bfound) && (i<parser->pconfig->lalr_state[parser->lalr_state].naction);i++)
		{
			struct _action* action = &parser->pconfig->lalr_state[parser->lalr_state].action[i];
			if (action->SymbolIndex == parser->symbol) {
				bfound = 1;
				switch(action->Action) {
				case ActionShift:
					{
					//
					// Push a symbol onto the stack
					//
					struct _symbol sym;
					parser_get_symbol(parser, parser->symbol, &sym);

					// push symbol onto stack
					_push_stack(parser, parser->symbol, &sym, 0, 0);
					if (sym.Name) free(sym.Name);

					parser->nstackofs = parser->nstack-1;
					parser->lalr_state = action->Target;

					// pop token from stack
					_pop_token(parser);
					}
					break;

				case ActionReduce:
					{
						
					//
					// Reducing a rule is done in two steps:
					// 1] Setup the stack offset so the calling function
					//    can reference the rule's child lexeme values when
					//    this action returns
					// 2] When this function is called again, we will
					//    remove the child lexemes from the stack, and replace
					//    them with an element representing this reduction
					//
					struct _rule* rule = &parser->pconfig->rule[action->Target];
					parser->lexeme[0] = 0;
					parser->symbol_userdata = 0;
					parser->reduce_rule = action->Target;
					parser->nstackofs = parser->nstack - rule->nsymbol;
					parser->reduction = 1;
					return rule->NonTerminal;
					}

				case ActionGoto:
					// Shift states
					parser->lalr_state = action->Target;
					_pop_token(parser);
					break;

				case ActionAccept:
					// Eof, the main rule has been accepted
					return 0;

				} // switch
			} // if
		} // for
		if (!bfound)
		{
			if (parser->symbol)
				break;
			return 0; // eof
		}
	} // while

	// token not found in rule
	return -1;
}
