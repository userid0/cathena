#define PS_EOF           0 /* (EOF) */
#define PS_ERROR         1 /* (Error) */
#define PS_WHITESPACE    2 /* (Whitespace) */
#define PS_COMMENTEND    3 /* (Comment End) */
#define PS_COMMENTLINE   4 /* (Comment Line) */
#define PS_COMMENTSTART  5 /* (Comment Start) */
#define PS_MINUS         6 /* '-' */
#define PS_MINUSMINUS    7 /* '--' */
#define PS_EXCLAM        8 /* '!' */
#define PS_EXCLAMEQ      9 /* '!=' */
#define PS_PERCENT       10 /* '%' */
#define PS_AMP           11 /* '&' */
#define PS_AMPAMP        12 /* '&&' */
#define PS_AMPEQ         13 /* '&=' */
#define PS_LPARAN        14 /* '(' */
#define PS_RPARAN        15 /* ')' */
#define PS_TIMES         16 /* '*' */
#define PS_TIMESEQ       17 /* '*=' */
#define PS_COMMA         18 /* ',' */
#define PS_DOT           19 /* '.' */
#define PS_DIV           20 /* '/' */
#define PS_DIVEQ         21 /* '/=' */
#define PS_COLON         22 /* ':' */
#define PS_COLONCOLON    23 /* '::' */
#define PS_SEMI          24 /* ';' */
#define PS_QUESTION      25 /* '?' */
#define PS_LBRACKET      26 /* '[' */
#define PS_RBRACKET      27 /* ']' */
#define PS_CARET         28 /* '^' */
#define PS_CARETEQ       29 /* '^=' */
#define PS_LBRACE        30 /* '{' */
#define PS_PIPE          31 /* '|' */
#define PS_PIPEPIPE      32 /* '||' */
#define PS_PIPEEQ        33 /* '|=' */
#define PS_RBRACE        34 /* '}' */
#define PS_TILDE         35 /* '~' */
#define PS_PLUS          36 /* '+' */
#define PS_PLUSPLUS      37 /* '++' */
#define PS_PLUSEQ        38 /* '+=' */
#define PS_LT            39 /* '<' */
#define PS_LTLT          40 /* '<<' */
#define PS_LTLTEQ        41 /* '<<=' */
#define PS_LTEQ          42 /* '<=' */
#define PS_EQ            43 /* '=' */
#define PS_MINUSEQ       44 /* '-=' */
#define PS_EQEQ          45 /* '==' */
#define PS_GT            46 /* '>' */
#define PS_GTEQ          47 /* '>=' */
#define PS_GTGT          48 /* '>>' */
#define PS_GTGTEQ        49 /* '>>=' */
#define PS_APOID         50 /* ApoId */
#define PS_AUTO          51 /* auto */
#define PS_BREAK         52 /* break */
#define PS_CASE          53 /* case */
#define PS_CHARLITERAL   54 /* CharLiteral */
#define PS_CONST         55 /* const */
#define PS_CONTINUE      56 /* continue */
#define PS_DECLITERAL    57 /* DecLiteral */
#define PS_DEFAULT       58 /* default */
#define PS_DO            59 /* do */
#define PS_DOUBLE        60 /* double */
#define PS_ELSE          61 /* else */
#define PS_FLOATLITERAL  62 /* FloatLiteral */
#define PS_FOR           63 /* for */
#define PS_FUNCTION      64 /* function */
#define PS_GLOBAL        65 /* global */
#define PS_GOTO          66 /* goto */
#define PS_HEXLITERAL    67 /* HexLiteral */
#define PS_ID            68 /* Id */
#define PS_IF            69 /* if */
#define PS_INT           70 /* int */
#define PS_RETURN        71 /* return */
#define PS_SCRIPT        72 /* script */
#define PS_SIZEOF        73 /* sizeof */
#define PS_STRING        74 /* string */
#define PS_STRINGLITERAL 75 /* StringLiteral */
#define PS_SWITCH        76 /* switch */
#define PS_TEMP          77 /* temp */
#define PS_WHILE         78 /* while */
#define PS_ARG           79 /* <Arg> */
#define PS_ARRAY         80 /* <Array> */
#define PS_BLOCK         81 /* <Block> */
#define PS_CALLARG       82 /* <Call Arg> */
#define PS_CALLLIST      83 /* <Call List> */
#define PS_CALLSTM       84 /* <Call Stm> */
#define PS_CARG          85 /* <CArg> */
#define PS_CASESTMS      86 /* <Case Stms> */
#define PS_DECL          87 /* <Decl> */
#define PS_DECLS         88 /* <Decls> */
#define PS_DPARAM        89 /* <DParam> */
#define PS_DPARAMS       90 /* <DParams> */
#define PS_EXNAMEID      91 /* <exName Id> */
#define PS_EXPR          92 /* <Expr> */
#define PS_EXPRLIST      93 /* <Expr List> */
#define PS_FILEID        94 /* <File Id> */
#define PS_FIXVALUE      95 /* <FixValue> */
#define PS_FUNCDECL      96 /* <Func Decl> */
#define PS_FUNCPROTO     97 /* <Func Proto> */
#define PS_GOTOSTMS      98 /* <Goto Stms> */
#define PS_LABELSTM      99 /* <Label Stm> */
#define PS_LCTRSTMS      100 /* <LCtr Stms> */
#define PS_MOD           101 /* <Mod> */
#define PS_NAMEID        102 /* <Name Id> */
#define PS_NORMALSTM     103 /* <Normal Stm> */
#define PS_OPADDSUB      104 /* <Op AddSub> */
#define PS_OPAND         105 /* <Op And> */
#define PS_OPASSIGN      106 /* <Op Assign> */
#define PS_OPBINAND      107 /* <Op BinAND> */
#define PS_OPBINOR       108 /* <Op BinOR> */
#define PS_OPBINXOR      109 /* <Op BinXOR> */
#define PS_OPCAST        110 /* <Op Cast> */
#define PS_OPCOMPARE     111 /* <Op Compare> */
#define PS_OPEQUATE      112 /* <Op Equate> */
#define PS_OPIF          113 /* <Op If> */
#define PS_OPMULTDIV     114 /* <Op MultDiv> */
#define PS_OPOR          115 /* <Op Or> */
#define PS_OPPOINTER     116 /* <Op Pointer> */
#define PS_OPPOST        117 /* <Op Post> */
#define PS_OPPRE         118 /* <Op Pre> */
#define PS_OPSHIFT       119 /* <Op Shift> */
#define PS_OPSIZEOF      120 /* <Op SizeOf> */
#define PS_OPUNARY       121 /* <Op Unary> */
#define PS_PARAM         122 /* <Param> */
#define PS_PARAMS        123 /* <Params> */
#define PS_RETURNSTMS    124 /* <Return Stms> */
#define PS_RETVALUES     125 /* <RetValues> */
#define PS_SCALAR        126 /* <Scalar> */
#define PS_SCRIPTDECL    127 /* <Script Decl> */
#define PS_SPRITEID      128 /* <Sprite Id> */
#define PS_STM           129 /* <Stm> */
#define PS_STMLIST       130 /* <Stm List> */
#define PS_TYPE          131 /* <Type> */
#define PS_VALUE         132 /* <Value> */
#define PS_VAR           133 /* <Var> */
#define PS_VARDECL       134 /* <Var Decl> */
#define PS_VARLIST       135 /* <Var List> */

#define PS_DECLS                                                                      0   /* <Decls> ::= <Decl> <Decls> */
#define PS_DECLS2                                                                     1   /* <Decls> ::=  */
#define PS_DECL                                                                       2   /* <Decl> ::= <Func Proto> */
#define PS_DECL2                                                                      3   /* <Decl> ::= <Func Decl> */
#define PS_DECL3                                                                      4   /* <Decl> ::= <Script Decl> */
#define PS_DECL4                                                                      5   /* <Decl> ::= <Block> */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI                                            6   /* <Func Proto> ::= <Scalar> Id '(' <DParams> ')' ';' */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI2                                           7   /* <Func Proto> ::= <Scalar> Id '(' <Params> ')' ';' */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI3                                           8   /* <Func Proto> ::= <Scalar> Id '(' ')' ';' */
#define PS_FUNCDECL_ID_LPARAN_RPARAN                                                  9   /* <Func Decl> ::= <Scalar> Id '(' <Params> ')' <Block> */
#define PS_FUNCDECL_ID_LPARAN_RPARAN2                                                 10  /* <Func Decl> ::= <Scalar> Id '(' ')' <Block> */
#define PS_SCRIPTDECL_MINUS_SCRIPT_COMMA                                              11  /* <Script Decl> ::= '-' script <Name Id> <Sprite Id> ',' <Block> */
#define PS_SCRIPTDECL_FUNCTION_SCRIPT                                                 12  /* <Script Decl> ::= function script <Name Id> <Block> */
#define PS_SCRIPTDECL_COMMA_DECLITERAL_COMMA_DECLITERAL_COMMA_DECLITERAL_SCRIPT_COMMA 13  /* <Script Decl> ::= <File Id> ',' DecLiteral ',' DecLiteral ',' DecLiteral script <Name Id> <Sprite Id> ',' <Block> */
#define PS_PARAMS_COMMA                                                               14  /* <Params> ::= <Param> ',' <Params> */
#define PS_PARAMS                                                                     15  /* <Params> ::= <Param> */
#define PS_PARAM_CONST_ID                                                             16  /* <Param> ::= const <Scalar> Id */
#define PS_PARAM_ID                                                                   17  /* <Param> ::= <Scalar> Id */
#define PS_PARAM_CONST_ID2                                                            18  /* <Param> ::= const Id */
#define PS_PARAM_ID2                                                                  19  /* <Param> ::= Id */
#define PS_DPARAMS_COMMA                                                              20  /* <DParams> ::= <DParam> ',' <DParams> */
#define PS_DPARAMS                                                                    21  /* <DParams> ::= <DParam> */
#define PS_DPARAM_CONST                                                               22  /* <DParam> ::= const <Scalar> */
#define PS_DPARAM                                                                     23  /* <DParam> ::= <Scalar> */
#define PS_SPRITEID_DECLITERAL                                                        24  /* <Sprite Id> ::= DecLiteral */
#define PS_SPRITEID_MINUS_DECLITERAL                                                  25  /* <Sprite Id> ::= '-' DecLiteral */
#define PS_FILEID_ID_DOT_ID                                                           26  /* <File Id> ::= Id '.' Id */
#define PS_NAMEID                                                                     27  /* <Name Id> ::= <exName Id> */
#define PS_NAMEID_COLONCOLON_ID                                                       28  /* <Name Id> ::= <exName Id> '::' Id */
#define PS_EXNAMEID_ID                                                                29  /* <exName Id> ::= Id */
#define PS_EXNAMEID_APOID                                                             30  /* <exName Id> ::= ApoId */
#define PS_EXNAMEID_ID2                                                               31  /* <exName Id> ::= Id <exName Id> */
#define PS_EXNAMEID_APOID2                                                            32  /* <exName Id> ::= ApoId <exName Id> */
#define PS_VARDECL_SEMI                                                               33  /* <Var Decl> ::= <Type> <Var List> ';' */
#define PS_VARLIST_COMMA                                                              34  /* <Var List> ::= <Var> ',' <Var List> */
#define PS_VARLIST                                                                    35  /* <Var List> ::= <Var> */
#define PS_VAR_ID                                                                     36  /* <Var> ::= Id <Array> */
#define PS_VAR_ID_EQ                                                                  37  /* <Var> ::= Id <Array> '=' <Op If> */
#define PS_ARRAY_LBRACKET_RBRACKET                                                    38  /* <Array> ::= '[' <Expr> ']' */
#define PS_ARRAY_LBRACKET_RBRACKET2                                                   39  /* <Array> ::= '[' ']' */
#define PS_ARRAY                                                                      40  /* <Array> ::=  */
#define PS_TYPE                                                                       41  /* <Type> ::= <Mod> <Scalar> */
#define PS_TYPE2                                                                      42  /* <Type> ::= <Mod> */
#define PS_TYPE3                                                                      43  /* <Type> ::= <Scalar> */
#define PS_MOD_GLOBAL                                                                 44  /* <Mod> ::= global */
#define PS_MOD_TEMP                                                                   45  /* <Mod> ::= temp */
#define PS_SCALAR_STRING                                                              46  /* <Scalar> ::= string */
#define PS_SCALAR_DOUBLE                                                              47  /* <Scalar> ::= double */
#define PS_SCALAR_INT                                                                 48  /* <Scalar> ::= int */
#define PS_SCALAR_AUTO                                                                49  /* <Scalar> ::= auto */
#define PS_BLOCK_LBRACE_RBRACE                                                        50  /* <Block> ::= '{' <Stm List> '}' */
#define PS_STMLIST                                                                    51  /* <Stm List> ::= <Stm> <Stm List> */
#define PS_STMLIST2                                                                   52  /* <Stm List> ::=  */
#define PS_STM                                                                        53  /* <Stm> ::= <Var Decl> */
#define PS_STM2                                                                       54  /* <Stm> ::= <Label Stm> */
#define PS_STM3                                                                       55  /* <Stm> ::= <Normal Stm> */
#define PS_LABELSTM_ID_COLON                                                          56  /* <Label Stm> ::= Id ':' */
#define PS_NORMALSTM_IF_LPARAN_RPARAN                                                 57  /* <Normal Stm> ::= if '(' <Expr> ')' <Normal Stm> */
#define PS_NORMALSTM_IF_LPARAN_RPARAN_ELSE                                            58  /* <Normal Stm> ::= if '(' <Expr> ')' <Normal Stm> else <Normal Stm> */
#define PS_NORMALSTM_WHILE_LPARAN_RPARAN                                              59  /* <Normal Stm> ::= while '(' <Expr> ')' <Normal Stm> */
#define PS_NORMALSTM_FOR_LPARAN_SEMI_SEMI_RPARAN                                      60  /* <Normal Stm> ::= for '(' <Arg> ';' <CArg> ';' <Arg> ')' <Normal Stm> */
#define PS_NORMALSTM_DO_WHILE_LPARAN_RPARAN_SEMI                                      61  /* <Normal Stm> ::= do <Normal Stm> while '(' <Expr> ')' ';' */
#define PS_NORMALSTM_SWITCH_LPARAN_RPARAN_LBRACE_RBRACE                               62  /* <Normal Stm> ::= switch '(' <Expr> ')' '{' <Case Stms> '}' */
#define PS_NORMALSTM                                                                  63  /* <Normal Stm> ::= <Block> */
#define PS_NORMALSTM_SEMI                                                             64  /* <Normal Stm> ::= <Expr> ';' */
#define PS_NORMALSTM2                                                                 65  /* <Normal Stm> ::= <Goto Stms> */
#define PS_NORMALSTM3                                                                 66  /* <Normal Stm> ::= <LCtr Stms> */
#define PS_NORMALSTM4                                                                 67  /* <Normal Stm> ::= <Return Stms> */
#define PS_NORMALSTM_SEMI2                                                            68  /* <Normal Stm> ::= ';' */
#define PS_NORMALSTM5                                                                 69  /* <Normal Stm> ::= <Call Stm> */
#define PS_GOTOSTMS_GOTO_ID_SEMI                                                      70  /* <Goto Stms> ::= goto Id ';' */
#define PS_LCTRSTMS_BREAK_SEMI                                                        71  /* <LCtr Stms> ::= break ';' */
#define PS_LCTRSTMS_CONTINUE_SEMI                                                     72  /* <LCtr Stms> ::= continue ';' */
#define PS_CARG                                                                       73  /* <CArg> ::= <Expr> */
#define PS_CARG2                                                                      74  /* <CArg> ::=  */
#define PS_ARG                                                                        75  /* <Arg> ::= <Expr List> */
#define PS_ARG2                                                                       76  /* <Arg> ::=  */
#define PS_RETURNSTMS_RETURN_SEMI                                                     77  /* <Return Stms> ::= return <Expr> ';' */
#define PS_RETURNSTMS_RETURN_SEMI2                                                    78  /* <Return Stms> ::= return ';' */
#define PS_CASESTMS_CASE_COLON                                                        79  /* <Case Stms> ::= case <Value> ':' <Stm List> <Case Stms> */
#define PS_CASESTMS_DEFAULT_COLON                                                     80  /* <Case Stms> ::= default ':' <Stm List> <Case Stms> */
#define PS_CASESTMS                                                                   81  /* <Case Stms> ::=  */
#define PS_CALLSTM_ID_SEMI                                                            82  /* <Call Stm> ::= Id <Call List> ';' */
#define PS_CALLSTM_ID_SEMI2                                                           83  /* <Call Stm> ::= Id ';' */
#define PS_CALLLIST_COMMA                                                             84  /* <Call List> ::= <Call Arg> ',' <Call List> */
#define PS_CALLLIST                                                                   85  /* <Call List> ::= <Call Arg> */
#define PS_CALLARG                                                                    86  /* <Call Arg> ::= <Op If> */
#define PS_CALLARG_MINUS                                                              87  /* <Call Arg> ::= '-' */
#define PS_EXPRLIST_COMMA                                                             88  /* <Expr List> ::= <Expr> ',' <Expr List> */
#define PS_EXPRLIST                                                                   89  /* <Expr List> ::= <Expr> */
#define PS_EXPR                                                                       90  /* <Expr> ::= <Op Assign> */
#define PS_OPASSIGN_EQ                                                                91  /* <Op Assign> ::= <Op If> '=' <Op Assign> */
#define PS_OPASSIGN_PLUSEQ                                                            92  /* <Op Assign> ::= <Op If> '+=' <Op Assign> */
#define PS_OPASSIGN_MINUSEQ                                                           93  /* <Op Assign> ::= <Op If> '-=' <Op Assign> */
#define PS_OPASSIGN_TIMESEQ                                                           94  /* <Op Assign> ::= <Op If> '*=' <Op Assign> */
#define PS_OPASSIGN_DIVEQ                                                             95  /* <Op Assign> ::= <Op If> '/=' <Op Assign> */
#define PS_OPASSIGN_CARETEQ                                                           96  /* <Op Assign> ::= <Op If> '^=' <Op Assign> */
#define PS_OPASSIGN_AMPEQ                                                             97  /* <Op Assign> ::= <Op If> '&=' <Op Assign> */
#define PS_OPASSIGN_PIPEEQ                                                            98  /* <Op Assign> ::= <Op If> '|=' <Op Assign> */
#define PS_OPASSIGN_GTGTEQ                                                            99  /* <Op Assign> ::= <Op If> '>>=' <Op Assign> */
#define PS_OPASSIGN_LTLTEQ                                                            100 /* <Op Assign> ::= <Op If> '<<=' <Op Assign> */
#define PS_OPASSIGN                                                                   101 /* <Op Assign> ::= <Op If> */
#define PS_OPIF_QUESTION_COLON                                                        102 /* <Op If> ::= <Op Or> '?' <Op If> ':' <Op If> */
#define PS_OPIF                                                                       103 /* <Op If> ::= <Op Or> */
#define PS_OPOR_PIPEPIPE                                                              104 /* <Op Or> ::= <Op Or> '||' <Op And> */
#define PS_OPOR                                                                       105 /* <Op Or> ::= <Op And> */
#define PS_OPAND_AMPAMP                                                               106 /* <Op And> ::= <Op And> '&&' <Op BinOR> */
#define PS_OPAND                                                                      107 /* <Op And> ::= <Op BinOR> */
#define PS_OPBINOR_PIPE                                                               108 /* <Op BinOR> ::= <Op BinOR> '|' <Op BinXOR> */
#define PS_OPBINOR                                                                    109 /* <Op BinOR> ::= <Op BinXOR> */
#define PS_OPBINXOR_CARET                                                             110 /* <Op BinXOR> ::= <Op BinXOR> '^' <Op BinAND> */
#define PS_OPBINXOR                                                                   111 /* <Op BinXOR> ::= <Op BinAND> */
#define PS_OPBINAND_AMP                                                               112 /* <Op BinAND> ::= <Op BinAND> '&' <Op Equate> */
#define PS_OPBINAND                                                                   113 /* <Op BinAND> ::= <Op Equate> */
#define PS_OPEQUATE_EQEQ                                                              114 /* <Op Equate> ::= <Op Equate> '==' <Op Compare> */
#define PS_OPEQUATE_EXCLAMEQ                                                          115 /* <Op Equate> ::= <Op Equate> '!=' <Op Compare> */
#define PS_OPEQUATE                                                                   116 /* <Op Equate> ::= <Op Compare> */
#define PS_OPCOMPARE_GT                                                               117 /* <Op Compare> ::= <Op Compare> '>' <Op Shift> */
#define PS_OPCOMPARE_GTEQ                                                             118 /* <Op Compare> ::= <Op Compare> '>=' <Op Shift> */
#define PS_OPCOMPARE_LT                                                               119 /* <Op Compare> ::= <Op Compare> '<' <Op Shift> */
#define PS_OPCOMPARE_LTEQ                                                             120 /* <Op Compare> ::= <Op Compare> '<=' <Op Shift> */
#define PS_OPCOMPARE                                                                  121 /* <Op Compare> ::= <Op Shift> */
#define PS_OPSHIFT_LTLT                                                               122 /* <Op Shift> ::= <Op Shift> '<<' <Op AddSub> */
#define PS_OPSHIFT_GTGT                                                               123 /* <Op Shift> ::= <Op Shift> '>>' <Op AddSub> */
#define PS_OPSHIFT                                                                    124 /* <Op Shift> ::= <Op AddSub> */
#define PS_OPADDSUB_PLUS                                                              125 /* <Op AddSub> ::= <Op AddSub> '+' <Op MultDiv> */
#define PS_OPADDSUB_MINUS                                                             126 /* <Op AddSub> ::= <Op AddSub> '-' <Op MultDiv> */
#define PS_OPADDSUB                                                                   127 /* <Op AddSub> ::= <Op MultDiv> */
#define PS_OPMULTDIV_TIMES                                                            128 /* <Op MultDiv> ::= <Op MultDiv> '*' <Op Unary> */
#define PS_OPMULTDIV_DIV                                                              129 /* <Op MultDiv> ::= <Op MultDiv> '/' <Op Unary> */
#define PS_OPMULTDIV_PERCENT                                                          130 /* <Op MultDiv> ::= <Op MultDiv> '%' <Op Unary> */
#define PS_OPMULTDIV                                                                  131 /* <Op MultDiv> ::= <Op Unary> */
#define PS_OPUNARY_EXCLAM                                                             132 /* <Op Unary> ::= '!' <Op Unary> */
#define PS_OPUNARY_TILDE                                                              133 /* <Op Unary> ::= '~' <Op Unary> */
#define PS_OPUNARY_MINUS                                                              134 /* <Op Unary> ::= '-' <Op Unary> */
#define PS_OPUNARY                                                                    135 /* <Op Unary> ::= <Op Post> */
#define PS_OPUNARY2                                                                   136 /* <Op Unary> ::= <Op Pre> */
#define PS_OPUNARY3                                                                   137 /* <Op Unary> ::= <Op Cast> */
#define PS_OPUNARY4                                                                   138 /* <Op Unary> ::= <Op SizeOf> */
#define PS_OPUNARY5                                                                   139 /* <Op Unary> ::= <Op Pointer> */
#define PS_OPUNARY6                                                                   140 /* <Op Unary> ::= <Value> */
#define PS_OPPOST_PLUSPLUS                                                            141 /* <Op Post> ::= <Op Pointer> '++' */
#define PS_OPPOST_MINUSMINUS                                                          142 /* <Op Post> ::= <Op Pointer> '--' */
#define PS_OPPRE_PLUSPLUS                                                             143 /* <Op Pre> ::= '++' <Op Unary> */
#define PS_OPPRE_MINUSMINUS                                                           144 /* <Op Pre> ::= '--' <Op Unary> */
#define PS_OPCAST_LPARAN_RPARAN                                                       145 /* <Op Cast> ::= '(' <Type> ')' <Op Unary> */
#define PS_OPSIZEOF_SIZEOF_LPARAN_RPARAN                                              146 /* <Op SizeOf> ::= sizeof '(' <Type> ')' */
#define PS_OPSIZEOF_SIZEOF_LPARAN_ID_RPARAN                                           147 /* <Op SizeOf> ::= sizeof '(' Id ')' */
#define PS_OPPOINTER_DOT_ID                                                           148 /* <Op Pointer> ::= <Op Pointer> '.' Id */
#define PS_OPPOINTER_DOT                                                              149 /* <Op Pointer> ::= <Op Pointer> '.' <RetValues> */
#define PS_OPPOINTER_LBRACKET_RBRACKET                                                150 /* <Op Pointer> ::= <Op Pointer> '[' <Expr> ']' */
#define PS_OPPOINTER_ID                                                               151 /* <Op Pointer> ::= Id */
#define PS_VALUE                                                                      152 /* <Value> ::= <FixValue> */
#define PS_VALUE_LPARAN_RPARAN                                                        153 /* <Value> ::= '(' <Expr> ')' */
#define PS_VALUE2                                                                     154 /* <Value> ::= <RetValues> */
#define PS_FIXVALUE_HEXLITERAL                                                        155 /* <FixValue> ::= HexLiteral */
#define PS_FIXVALUE_DECLITERAL                                                        156 /* <FixValue> ::= DecLiteral */
#define PS_FIXVALUE_STRINGLITERAL                                                     157 /* <FixValue> ::= StringLiteral */
#define PS_FIXVALUE_CHARLITERAL                                                       158 /* <FixValue> ::= CharLiteral */
#define PS_FIXVALUE_FLOATLITERAL                                                      159 /* <FixValue> ::= FloatLiteral */
#define PS_RETVALUES_ID_LPARAN_RPARAN                                                 160 /* <RetValues> ::= Id '(' <Expr List> ')' */
#define PS_RETVALUES_ID_LPARAN_RPARAN2                                                161 /* <RetValues> ::= Id '(' ')' */
