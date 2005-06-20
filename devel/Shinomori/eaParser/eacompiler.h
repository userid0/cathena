
#ifndef _EACOMPILER_
#define _EACOMPILER_


#include <stdio.h>
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////////
// terminal definitions from parse tree
#define PT_EOF           0 /* (EOF) */
#define PT_ERROR         1 /* (Error) */
#define PT_WHITESPACE    2 /* (Whitespace) */
#define PT_COMMENTEND    3 /* (Comment End) */
#define PT_COMMENTLINE   4 /* (Comment Line) */
#define PT_COMMENTSTART  5 /* (Comment Start) */
#define PT_MINUS         6 /* '-' */
#define PT_MINUSMINUS    7 /* '--' */
#define PT_EXCLAM        8 /* '!' */
#define PT_EXCLAMEQ      9 /* '!=' */
#define PT_PERCENT       10 /* '%' */
#define PT_AMP           11 /* '&' */
#define PT_AMPAMP        12 /* '&&' */
#define PT_AMPEQ         13 /* '&=' */
#define PT_LPARAN        14 /* '(' */
#define PT_RPARAN        15 /* ')' */
#define PT_TIMES         16 /* '*' */
#define PT_TIMESEQ       17 /* '*=' */
#define PT_COMMA         18 /* ',' */
#define PT_DOT           19 /* '.' */
#define PT_DIV           20 /* '/' */
#define PT_DIVEQ         21 /* '/=' */
#define PT_COLON         22 /* ':' */
#define PT_COLONCOLON    23 /* '::' */
#define PT_SEMI          24 /* ';' */
#define PT_QUESTION      25 /* '?' */
#define PT_LBRACKET      26 /* '[' */
#define PT_RBRACKET      27 /* ']' */
#define PT_CARET         28 /* '^' */
#define PT_CARETEQ       29 /* '^=' */
#define PT_LBRACE        30 /* '{' */
#define PT_PIPE          31 /* '|' */
#define PT_PIPEPIPE      32 /* '||' */
#define PT_PIPEEQ        33 /* '|=' */
#define PT_RBRACE        34 /* '}' */
#define PT_TILDE         35 /* '~' */
#define PT_PLUS          36 /* '+' */
#define PT_PLUSPLUS      37 /* '++' */
#define PT_PLUSEQ        38 /* '+=' */
#define PT_LT            39 /* '<' */
#define PT_LTLT          40 /* '<<' */
#define PT_LTLTEQ        41 /* '<<=' */
#define PT_LTEQ          42 /* '<=' */
#define PT_EQ            43 /* '=' */
#define PT_MINUSEQ       44 /* '-=' */
#define PT_EQEQ          45 /* '==' */
#define PT_GT            46 /* '>' */
#define PT_GTEQ          47 /* '>=' */
#define PT_GTGT          48 /* '>>' */
#define PT_GTGTEQ        49 /* '>>=' */
#define PT_APOID         50 /* ApoId */
#define PT_AUTO          51 /* auto */
#define PT_BREAK         52 /* break */
#define PT_CASE          53 /* case */
#define PT_CHARLITERAL   54 /* CharLiteral */
#define PT_CONST         55 /* const */
#define PT_CONTINUE      56 /* continue */
#define PT_DECLITERAL    57 /* DecLiteral */
#define PT_DEFAULT       58 /* default */
#define PT_DO            59 /* do */
#define PT_DOUBLE        60 /* double */
#define PT_ELSE          61 /* else */
#define PT_FLOATLITERAL  62 /* FloatLiteral */
#define PT_FOR           63 /* for */
#define PT_FUNCTION      64 /* function */
#define PT_GLOBAL        65 /* global */
#define PT_GOTO          66 /* goto */
#define PT_HEXLITERAL    67 /* HexLiteral */
#define PT_ID            68 /* Id */
#define PT_IF            69 /* if */
#define PT_INT           70 /* int */
#define PT_RETURN        71 /* return */
#define PT_SCRIPT        72 /* script */
#define PT_SIZEOF        73 /* sizeof */
#define PT_STRING        74 /* string */
#define PT_STRINGLITERAL 75 /* StringLiteral */
#define PT_SWITCH        76 /* switch */
#define PT_TEMP          77 /* temp */
#define PT_WHILE         78 /* while */
#define PT_ARG           79 /* <Arg> */
#define PT_ARRAY         80 /* <Array> */
#define PT_BLOCK         81 /* <Block> */
#define PT_CALLARG       82 /* <Call Arg> */
#define PT_CALLLIST      83 /* <Call List> */
#define PT_CALLSTM       84 /* <Call Stm> */
#define PT_CARG          85 /* <CArg> */
#define PT_CASESTMS      86 /* <Case Stms> */
#define PT_DECL          87 /* <Decl> */
#define PT_DECLS         88 /* <Decls> */
#define PT_DPARAM        89 /* <DParam> */
#define PT_DPARAMS       90 /* <DParams> */
#define PT_EXNAMEID      91 /* <exName Id> */
#define PT_EXPR          92 /* <Expr> */
#define PT_EXPRLIST      93 /* <Expr List> */
#define PT_FILEID        94 /* <File Id> */
#define PT_FIXVALUE      95 /* <FixValue> */
#define PT_FUNCDECL      96 /* <Func Decl> */
#define PT_FUNCPROTO     97 /* <Func Proto> */
#define PT_GOTOSTMS      98 /* <Goto Stms> */
#define PT_LABELSTM      99 /* <Label Stm> */
#define PT_LCTRSTMS      100 /* <LCtr Stms> */
#define PT_MOD           101 /* <Mod> */
#define PT_NAMEID        102 /* <Name Id> */
#define PT_NORMALSTM     103 /* <Normal Stm> */
#define PT_OPADDSUB      104 /* <Op AddSub> */
#define PT_OPAND         105 /* <Op And> */
#define PT_OPASSIGN      106 /* <Op Assign> */
#define PT_OPBINAND      107 /* <Op BinAND> */
#define PT_OPBINOR       108 /* <Op BinOR> */
#define PT_OPBINXOR      109 /* <Op BinXOR> */
#define PT_OPCAST        110 /* <Op Cast> */
#define PT_OPCOMPARE     111 /* <Op Compare> */
#define PT_OPEQUATE      112 /* <Op Equate> */
#define PT_OPIF          113 /* <Op If> */
#define PT_OPMULTDIV     114 /* <Op MultDiv> */
#define PT_OPOR          115 /* <Op Or> */
#define PT_OPPOINTER     116 /* <Op Pointer> */
#define PT_OPPOST        117 /* <Op Post> */
#define PT_OPPRE         118 /* <Op Pre> */
#define PT_OPSHIFT       119 /* <Op Shift> */
#define PT_OPSIZEOF      120 /* <Op SizeOf> */
#define PT_OPUNARY       121 /* <Op Unary> */
#define PT_PARAM         122 /* <Param> */
#define PT_PARAMS        123 /* <Params> */
#define PT_RETURNSTMS    124 /* <Return Stms> */
#define PT_RETVALUES     125 /* <RetValues> */
#define PT_SCALAR        126 /* <Scalar> */
#define PT_SCRIPTDECL    127 /* <Script Decl> */
#define PT_SPRITEID      128 /* <Sprite Id> */
#define PT_STM           129 /* <Stm> */
#define PT_STMLIST       130 /* <Stm List> */
#define PT_TYPE          131 /* <Type> */
#define PT_VALUE         132 /* <Value> */
#define PT_VAR           133 /* <Var> */
#define PT_VARDECL       134 /* <Var Decl> */
#define PT_VARLIST       135 /* <Var List> */

///////////////////////////////////////////////////////////////////////////////
// command opcodes for stack interpreter
//

typedef enum
{
	/////////////////////////////////////////////////////////////////
	// "no operation" as usual
	OP_NOP				=  0,	

	/////////////////////////////////////////////////////////////////
	// assignment operaations
	// take two stack values and push up one
	OP_ASSIGN			=  1,	// <Op If> '='   <Op>
	OP_ASSIGN_ADD		=  2,	// <Op If> '+='  <Op>
	OP_ASSIGN_SUB		=  3,	// <Op If> '-='  <Op>
	OP_ASSIGN_MUL		=  4,	// <Op If> '*='  <Op>
	OP_ASSIGN_DIV		=  5,	// <Op If> '/='  <Op>
	OP_ASSIGN_XOR		=  6,	// <Op If> '^='  <Op>
	OP_ASSIGN_AND		=  7,	// <Op If> '&='  <Op>
	OP_ASSIGN_OR		=  8,	// <Op If> '|='  <Op>
	OP_ASSIGN_RSH		=  9,	// <Op If> '>>=' <Op>
	OP_ASSIGN_LSH		= 10,	// <Op If> '<<=' <Op>

	/////////////////////////////////////////////////////////////////
	// select operation
	// take three stack values and push the second or third depending on the first
	OP_SELECT			= 11,	// <Op Or> '?' <Op If> ':' <Op If>

	/////////////////////////////////////////////////////////////////
	// logic operations
	// take two stack values and push a value
	OP_LOG_OR			= 12,	// <Op Or> '||' <Op And>
	OP_LOG_AND			= 13,	// <Op And> '&&' <Op BinOR>
	OP_BIN_OR			= 14,	// <Op BinOr> '|' <Op BinXOR>
	OP_BIN_XOR			= 15,	// <Op BinXOR> '^' <Op BinAND>
	OP_BIN_AND			= 16,	// <Op BinAND> '&' <Op Equate>

	/////////////////////////////////////////////////////////////////
	// compare operations
	// take two stack values and push a boolean value
	OP_EQUATE			= 17,	// <Op Equate> '==' <Op Compare>
	OP_UNEQUATE			= 18,	// <Op Equate> '!=' <Op Compare>
	OP_ISGT				= 19,	// <Op Compare> '>'  <Op Shift>
	OP_ISGTEQ			= 20,	// <Op Compare> '>=' <Op Shift>
	OP_ISLT				= 21,	// <Op Compare> '<'  <Op Shift>
	OP_ISLTEQ			= 22,	// <Op Compare> '<=' <Op Shift>

	/////////////////////////////////////////////////////////////////
	// shift operations
	// take two stack values and push a value
	OP_LSHIFT			= 23,	// <Op Shift> '<<' <Op AddSub>
	OP_RSHIFT			= 24,	// <Op Shift> '>>' <Op AddSub>

	/////////////////////////////////////////////////////////////////
	// add/sub operations
	// take two stack values and push a value
	OP_ADD				= 25,	// <Op AddSub> '+' <Op MultDiv>
	OP_SUB				= 26,	// <Op AddSub> '-' <Op MultDiv>

	/////////////////////////////////////////////////////////////////
	// mult/div/modulo operations
	// take two stack values and push a value
	OP_MUL				= 27,	// <Op MultDiv> '*' <Op Unary>
	OP_DIV				= 28,	// <Op MultDiv> '/' <Op Unary>
	OP_MOD				= 29,	// <Op MultDiv> '%' <Op Unary>

	/////////////////////////////////////////////////////////////////
	// unary operations
	// take one stack values and push a value
	OP_NOT				= 30,	// '!'    <Op Unary>
	OP_INVERT			= 31,	// '~'    <Op Unary>
	OP_NEGATE			= 32,	// '-'    <Op Unary>

	/////////////////////////////////////////////////////////////////
	// sizeof operations
	// take one stack values and push the result
								// sizeof '(' <Type> ')' // replaces with OP_PUSH_INT on compile time
	OP_SIZEOF			= 33,	// sizeof '(' Id ')'

	/////////////////////////////////////////////////////////////////
	// cast operations
	// take one stack values and push the result
	OP_CAST				= 34,	// '(' <Type> ')' <Op Unary>   !CAST

	/////////////////////////////////////////////////////////////////
	// Pre operations
	// take one stack variable and push a value
	OP_PREADD			= 35,	// '++'   <Op Unary>
	OP_PRESUB			= 36,	// '--'   <Op Unary>

	/////////////////////////////////////////////////////////////////
	// Post operations
	// take one stack variable and push a value
	OP_POSTADD			= 37,	// <Op Pointer> '++'
	OP_POSTSUB			= 38,	// <Op Pointer> '--'

	/////////////////////////////////////////////////////////////////
	// Member Access
	// take a variable and a value from stack and push a varible
	OP_MEMBER			= 39,	// <Op Pointer> '.' <Value>     ! member


	/////////////////////////////////////////////////////////////////
	// Array
	// take a variable and a value from stack and push a varible
	OP_ARRAY			= 40,	// <Op Pointer> '[' <Expr> ']'  ! array


	/////////////////////////////////////////////////////////////////
	// standard function calls
	// check the parameters on stack before or inside the call of function
	OP_CALLSCRIPT		= 42,
	OP_CALLBUILDIN		= 43,
								// Id '(' <Expr> ')'
								// Id '(' ')'
								// Id <Call List> ';'
								// Id ';'
	// followed by the function id
	// followed by the number of valid parameters


	/////////////////////////////////////////////////////////////////
	// conditional branch
	// take one value from stack 
	// and add 1 or the branch offset to the programm counter
	OP_NIF				= 44,	// if '(' <Expr> ')' <Normal Stm>
	OP_IF				= 45,	// if '(' <Expr> ')' <Normal Stm>
	/////////////////////////////////////////////////////////////////
	// unconditional branch
	// add the branch offset to the programm counter
	OP_GOTO				= 46,	// goto Id ';'

	// maybe also implementing short and far jumps


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

	OP_PUSH_INT			=47,	// followed by an integer
	OP_PUSH_STRING		=48,	// followed by a string (pointer)
	OP_PUSH_FLOAT		=49,	// followed by a float
	OP_PUSH_VAR			=50,	// followed by a string containing the variable name
	OP_PUSH_VALUE		=51,	// followed by a string containing the variable name
	OP_PUSH_TEMPVAR		=52,	// followed by the number of the temp variable 
	OP_PUSH_TEMPVALUE	=53,	// followed by the number of the temp variable 
	OP_RESIZE			=54,	// resize a var array
	OP_CLEAR			=55,	// clear a variable
	OP_POP				=56,	// clear the stack

	OP_START			=57,	// Program Start
	OP_END				=58,	// Quit the interpreter immediately


	// others
	VX_LABEL			=59,	// temporary node followed by a temporary target address
	VX_BREAK			=60,	// temporary node followed by a temporary target address
	VX_CONT				=61,	// temporary node followed by a temporary target address

	VX_GOTO				=62,	// direct jump node followed by the target address



};








void startcompile(struct _parser* parser);

///////////////////////////////////////////////////////////////////////////////
/*
	necessary code transformations on control structures

	break;	  => ignored by default (warn)
	continue; => ignored by default (warn)

	<Label Stm>     ::= Id ':'
	=> 
	register in label list


	if '(' <Expr> ')' <Normal Stm>
	=>

	<Expr>				// puts the result on the stack
	_opnif_ label1		// take one value from stack returning the offset (1 or the branch offset)
	<Normal Stm1>		// 
	label1				// branch offset is leading here


	if '(' <Expr> ')' <Normal Stm> else <Normal Stm>
	=>

	<Expr>				// puts the result on the stack
	_opnif_ label1		// take one value from stack returning the offset (1 or the branch offset)
	<Normal Stm1>		// 
	goto label2			// jump over Normal Stm2
	label1				// branch offset1 is leading here
	<Normal Stm2>		// 
	label2				// branch offset2 is leading here


	while '(' <Expr> ')' <Normal Stm>
	=>

	label1				// branch offset1 is leading here
	<Expr>				// puts the result on the stack
	_opnif_ label2		// take one value from stack returning the offset (1 or the branch offset)
	<Normal Stm> 
	goto label1			// jump offset1
	label2				// branch offset2 is leading here

	break;	  => goto label2
	continue; => goto label1



	for '(' <Arg> ';' <Arg> ';' <Arg> ')' <Normal Stm>
	=>
	

	<Arg1>
	label1
	<Arg2>
	_opif!_ label2
	<Normal Stm>
	<Arg3>
	goto label1
	label2

	break;	  => goto label2
	continue; => goto label1

	
	do <Normal Stm> while '(' <Expr> ')' ';'
	=>
	
	label1
	<Normal Stm>
	<Expr> -> _opif_ label1
	label 2

	break;	  => goto label2
	continue; => goto label1



	switch '(' <Expr> ')' '{' <Case Stms> '}'
	<Case Stms>  ::= case <Value> ':' <Stm List> <Case Stms>
               | default ':' <Stm List>
               |
	=>

	<Exprtemp> = <Expr>
	<Exprtemp> == <Case Stms1>.<Value> -> _opif_ label1
	<Exprtemp> == <Case Stms2>.<Value> -> _opif_ label1
	<Exprtemp> == <Case Stms3>.<Value> -> _opif_ label1
	goto label_default
	label1
	<Stm List1>
	label2
	<Stm List2>
	label3
	<Stm List3>
	label_default
	<Stm Listdef>
	label_end

	break;	  => goto goto label_end





*/
///////////////////////////////////////////////////////////////////////////////
#endif//_EACOMPILER_