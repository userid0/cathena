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
#define PS_BREAK         51 /* break */
#define PS_CASE          52 /* case */
#define PS_CHARLITERAL   53 /* CharLiteral */
#define PS_CONST         54 /* const */
#define PS_CONTINUE      55 /* continue */
#define PS_DECLITERAL    56 /* DecLiteral */
#define PS_DEFAULT       57 /* default */
#define PS_DO            58 /* do */
#define PS_DOUBLE        59 /* double */
#define PS_ELSE          60 /* else */
#define PS_FLOATLITERAL  61 /* FloatLiteral */
#define PS_FOR           62 /* for */
#define PS_FUNCTION      63 /* function */
#define PS_GLOBAL        64 /* global */
#define PS_GOTO          65 /* goto */
#define PS_HEXLITERAL    66 /* HexLiteral */
#define PS_ID            67 /* Id */
#define PS_IF            68 /* if */
#define PS_INT           69 /* int */
#define PS_RETURN        70 /* return */
#define PS_SCRIPT        71 /* script */
#define PS_SIZEOF        72 /* sizeof */
#define PS_STRING        73 /* string */
#define PS_STRINGLITERAL 74 /* StringLiteral */
#define PS_SWITCH        75 /* switch */
#define PS_TEMP          76 /* temp */
#define PS_WHILE         77 /* while */
#define PS_ARG           78 /* <Arg> */
#define PS_ARRAY         79 /* <Array> */
#define PS_BLOCK         80 /* <Block> */
#define PS_CALLARG       81 /* <Call Arg> */
#define PS_CALLLIST      82 /* <Call List> */
#define PS_CALLSTM       83 /* <Call Stm> */
#define PS_CARG          84 /* <CArg> */
#define PS_CASESTMS      85 /* <Case Stms> */
#define PS_DECL          86 /* <Decl> */
#define PS_DECLS         87 /* <Decls> */
#define PS_EXNAMEID      88 /* <exName Id> */
#define PS_EXPR          89 /* <Expr> */
#define PS_EXPRLIST      90 /* <Expr List> */
#define PS_FILEID        91 /* <File Id> */
#define PS_FIXVALUE      92 /* <FixValue> */
#define PS_FUNCDECL      93 /* <Func Decl> */
#define PS_FUNCPROTO     94 /* <Func Proto> */
#define PS_GOTOSTMS      95 /* <Goto Stms> */
#define PS_LABELSTM      96 /* <Label Stm> */
#define PS_LCTRSTMS      97 /* <LCtr Stms> */
#define PS_MOD           98 /* <Mod> */
#define PS_NAMEID        99 /* <Name Id> */
#define PS_NORMALSTM     100 /* <Normal Stm> */
#define PS_OPADDSUB      101 /* <Op AddSub> */
#define PS_OPAND         102 /* <Op And> */
#define PS_OPASSIGN      103 /* <Op Assign> */
#define PS_OPBINAND      104 /* <Op BinAND> */
#define PS_OPBINOR       105 /* <Op BinOR> */
#define PS_OPBINXOR      106 /* <Op BinXOR> */
#define PS_OPCAST        107 /* <Op Cast> */
#define PS_OPCOMPARE     108 /* <Op Compare> */
#define PS_OPEQUATE      109 /* <Op Equate> */
#define PS_OPIF          110 /* <Op If> */
#define PS_OPMULTDIV     111 /* <Op MultDiv> */
#define PS_OPOR          112 /* <Op Or> */
#define PS_OPPOINTER     113 /* <Op Pointer> */
#define PS_OPPOST        114 /* <Op Post> */
#define PS_OPPRE         115 /* <Op Pre> */
#define PS_OPSHIFT       116 /* <Op Shift> */
#define PS_OPSIZEOF      117 /* <Op SizeOf> */
#define PS_OPUNARY       118 /* <Op Unary> */
#define PS_PARAM         119 /* <Param> */
#define PS_PARAMS        120 /* <Params> */
#define PS_RETURNSTMS    121 /* <Return Stms> */
#define PS_RETVALUES     122 /* <RetValues> */
#define PS_SCALAR        123 /* <Scalar> */
#define PS_SCRIPTDECL    124 /* <Script Decl> */
#define PS_SPRITEID      125 /* <Sprite Id> */
#define PS_STM           126 /* <Stm> */
#define PS_STMLIST       127 /* <Stm List> */
#define PS_TYPE          128 /* <Type> */
#define PS_TYPES         129 /* <Types> */
#define PS_VALUE         130 /* <Value> */
#define PS_VAR           131 /* <Var> */
#define PS_VARDECL       132 /* <Var Decl> */
#define PS_VARLIST       133 /* <Var List> */

#define PS_DECLS                                                                      0   /* <Decls> ::= <Decl> <Decls> */
#define PS_DECLS2                                                                     1   /* <Decls> ::=  */
#define PS_DECL                                                                       2   /* <Decl> ::= <Func Proto> */
#define PS_DECL2                                                                      3   /* <Decl> ::= <Func Decl> */
#define PS_DECL3                                                                      4   /* <Decl> ::= <Script Decl> */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI                                            5   /* <Func Proto> ::= <Scalar> Id '(' <Types> ')' ';' */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI2                                           6   /* <Func Proto> ::= <Scalar> Id '(' <Params> ')' ';' */
#define PS_FUNCPROTO_ID_LPARAN_RPARAN_SEMI3                                           7   /* <Func Proto> ::= <Scalar> Id '(' ')' ';' */
#define PS_FUNCDECL_ID_LPARAN_RPARAN                                                  8   /* <Func Decl> ::= <Scalar> Id '(' <Params> ')' <Block> */
#define PS_FUNCDECL_ID_LPARAN_RPARAN2                                                 9   /* <Func Decl> ::= <Scalar> Id '(' ')' <Block> */
#define PS_SCRIPTDECL_MINUS_SCRIPT_COMMA                                              10  /* <Script Decl> ::= '-' script <Name Id> <Sprite Id> ',' <Block> */
#define PS_SCRIPTDECL_FUNCTION_SCRIPT                                                 11  /* <Script Decl> ::= function script <Name Id> <Block> */
#define PS_SCRIPTDECL_COMMA_DECLITERAL_COMMA_DECLITERAL_COMMA_DECLITERAL_SCRIPT_COMMA 12  /* <Script Decl> ::= <File Id> ',' DecLiteral ',' DecLiteral ',' DecLiteral script <Name Id> <Sprite Id> ',' <Block> */
#define PS_PARAMS_COMMA                                                               13  /* <Params> ::= <Param> ',' <Params> */
#define PS_PARAMS                                                                     14  /* <Params> ::= <Param> */
#define PS_PARAM_CONST_ID                                                             15  /* <Param> ::= const <Type> Id */
#define PS_PARAM_ID                                                                   16  /* <Param> ::= <Type> Id */
#define PS_TYPES_COMMA                                                                17  /* <Types> ::= <Type> ',' <Types> */
#define PS_TYPES                                                                      18  /* <Types> ::= <Type> */
#define PS_SPRITEID_DECLITERAL                                                        19  /* <Sprite Id> ::= DecLiteral */
#define PS_SPRITEID_MINUS_DECLITERAL                                                  20  /* <Sprite Id> ::= '-' DecLiteral */
#define PS_FILEID_ID_DOT_ID                                                           21  /* <File Id> ::= Id '.' Id */
#define PS_NAMEID                                                                     22  /* <Name Id> ::= <exName Id> */
#define PS_NAMEID_COLONCOLON_ID                                                       23  /* <Name Id> ::= <exName Id> '::' Id */
#define PS_EXNAMEID_ID                                                                24  /* <exName Id> ::= Id */
#define PS_EXNAMEID_APOID                                                             25  /* <exName Id> ::= ApoId */
#define PS_EXNAMEID_ID2                                                               26  /* <exName Id> ::= <exName Id> Id */
#define PS_EXNAMEID_APOID2                                                            27  /* <exName Id> ::= <exName Id> ApoId */
#define PS_VARDECL_SEMI                                                               28  /* <Var Decl> ::= <Type> <Var> <Var List> ';' */
#define PS_VARLIST_COMMA                                                              29  /* <Var List> ::= ',' <Var> <Var List> */
#define PS_VARLIST                                                                    30  /* <Var List> ::=  */
#define PS_VAR_ID                                                                     31  /* <Var> ::= Id <Array> */
#define PS_VAR_ID_EQ                                                                  32  /* <Var> ::= Id <Array> '=' <Op If> */
#define PS_ARRAY_LBRACKET_RBRACKET                                                    33  /* <Array> ::= '[' <Expr> ']' */
#define PS_ARRAY_LBRACKET_RBRACKET2                                                   34  /* <Array> ::= '[' ']' */
#define PS_ARRAY                                                                      35  /* <Array> ::=  */
#define PS_TYPE                                                                       36  /* <Type> ::= <Mod> <Scalar> */
#define PS_TYPE2                                                                      37  /* <Type> ::= <Mod> */
#define PS_MOD_GLOBAL                                                                 38  /* <Mod> ::= global */
#define PS_MOD_TEMP                                                                   39  /* <Mod> ::= temp */
#define PS_SCALAR_STRING                                                              40  /* <Scalar> ::= string */
#define PS_SCALAR_DOUBLE                                                              41  /* <Scalar> ::= double */
#define PS_SCALAR_INT                                                                 42  /* <Scalar> ::= int */
#define PS_BLOCK_LBRACE_RBRACE                                                        43  /* <Block> ::= '{' <Stm List> '}' */
#define PS_STMLIST                                                                    44  /* <Stm List> ::= <Stm> <Stm List> */
#define PS_STMLIST2                                                                   45  /* <Stm List> ::=  */
#define PS_STM                                                                        46  /* <Stm> ::= <Var Decl> */
#define PS_STM2                                                                       47  /* <Stm> ::= <Label Stm> */
#define PS_STM3                                                                       48  /* <Stm> ::= <Normal Stm> */
#define PS_LABELSTM_ID_COLON                                                          49  /* <Label Stm> ::= Id ':' */
#define PS_NORMALSTM_IF_LPARAN_RPARAN                                                 50  /* <Normal Stm> ::= if '(' <Expr> ')' <Normal Stm> */
#define PS_NORMALSTM_IF_LPARAN_RPARAN_ELSE                                            51  /* <Normal Stm> ::= if '(' <Expr> ')' <Normal Stm> else <Normal Stm> */
#define PS_NORMALSTM_WHILE_LPARAN_RPARAN                                              52  /* <Normal Stm> ::= while '(' <Expr> ')' <Normal Stm> */
#define PS_NORMALSTM_FOR_LPARAN_SEMI_SEMI_RPARAN                                      53  /* <Normal Stm> ::= for '(' <Arg> ';' <CArg> ';' <Arg> ')' <Normal Stm> */
#define PS_NORMALSTM_DO_WHILE_LPARAN_RPARAN_SEMI                                      54  /* <Normal Stm> ::= do <Normal Stm> while '(' <Expr> ')' ';' */
#define PS_NORMALSTM_SWITCH_LPARAN_RPARAN_LBRACE_RBRACE                               55  /* <Normal Stm> ::= switch '(' <Expr> ')' '{' <Case Stms> '}' */
#define PS_NORMALSTM                                                                  56  /* <Normal Stm> ::= <Block> */
#define PS_NORMALSTM_SEMI                                                             57  /* <Normal Stm> ::= <Expr> ';' */
#define PS_NORMALSTM2                                                                 58  /* <Normal Stm> ::= <Goto Stms> */
#define PS_NORMALSTM3                                                                 59  /* <Normal Stm> ::= <LCtr Stms> */
#define PS_NORMALSTM4                                                                 60  /* <Normal Stm> ::= <Return Stms> */
#define PS_NORMALSTM_SEMI2                                                            61  /* <Normal Stm> ::= ';' */
#define PS_NORMALSTM5                                                                 62  /* <Normal Stm> ::= <Call Stm> */
#define PS_GOTOSTMS_GOTO_ID_SEMI                                                      63  /* <Goto Stms> ::= goto Id ';' */
#define PS_LCTRSTMS_BREAK_SEMI                                                        64  /* <LCtr Stms> ::= break ';' */
#define PS_LCTRSTMS_CONTINUE_SEMI                                                     65  /* <LCtr Stms> ::= continue ';' */
#define PS_CARG                                                                       66  /* <CArg> ::= <Expr> */
#define PS_CARG2                                                                      67  /* <CArg> ::=  */
#define PS_ARG                                                                        68  /* <Arg> ::= <Expr List> */
#define PS_ARG2                                                                       69  /* <Arg> ::=  */
#define PS_RETURNSTMS_RETURN_SEMI                                                     70  /* <Return Stms> ::= return <Expr> ';' */
#define PS_RETURNSTMS_RETURN_SEMI2                                                    71  /* <Return Stms> ::= return ';' */
#define PS_CASESTMS_CASE_COLON                                                        72  /* <Case Stms> ::= case <Value> ':' <Stm List> <Case Stms> */
#define PS_CASESTMS_DEFAULT_COLON                                                     73  /* <Case Stms> ::= default ':' <Stm List> */
#define PS_CASESTMS                                                                   74  /* <Case Stms> ::=  */
#define PS_CALLSTM_ID_SEMI                                                            75  /* <Call Stm> ::= Id <Call List> ';' */
#define PS_CALLSTM_ID_SEMI2                                                           76  /* <Call Stm> ::= Id ';' */
#define PS_CALLLIST_COMMA                                                             77  /* <Call List> ::= <Call Arg> ',' <Call List> */
#define PS_CALLLIST                                                                   78  /* <Call List> ::= <Call Arg> */
#define PS_CALLARG                                                                    79  /* <Call Arg> ::= <Op If> */
#define PS_CALLARG_MINUS                                                              80  /* <Call Arg> ::= '-' */
#define PS_EXPRLIST_COMMA                                                             81  /* <Expr List> ::= <Expr> ',' <Expr List> */
#define PS_EXPRLIST                                                                   82  /* <Expr List> ::= <Expr> */
#define PS_EXPR                                                                       83  /* <Expr> ::= <Op Assign> */
#define PS_OPASSIGN_EQ                                                                84  /* <Op Assign> ::= <Op If> '=' <Op Assign> */
#define PS_OPASSIGN_PLUSEQ                                                            85  /* <Op Assign> ::= <Op If> '+=' <Op Assign> */
#define PS_OPASSIGN_MINUSEQ                                                           86  /* <Op Assign> ::= <Op If> '-=' <Op Assign> */
#define PS_OPASSIGN_TIMESEQ                                                           87  /* <Op Assign> ::= <Op If> '*=' <Op Assign> */
#define PS_OPASSIGN_DIVEQ                                                             88  /* <Op Assign> ::= <Op If> '/=' <Op Assign> */
#define PS_OPASSIGN_CARETEQ                                                           89  /* <Op Assign> ::= <Op If> '^=' <Op Assign> */
#define PS_OPASSIGN_AMPEQ                                                             90  /* <Op Assign> ::= <Op If> '&=' <Op Assign> */
#define PS_OPASSIGN_PIPEEQ                                                            91  /* <Op Assign> ::= <Op If> '|=' <Op Assign> */
#define PS_OPASSIGN_GTGTEQ                                                            92  /* <Op Assign> ::= <Op If> '>>=' <Op Assign> */
#define PS_OPASSIGN_LTLTEQ                                                            93  /* <Op Assign> ::= <Op If> '<<=' <Op Assign> */
#define PS_OPASSIGN                                                                   94  /* <Op Assign> ::= <Op If> */
#define PS_OPIF_QUESTION_COLON                                                        95  /* <Op If> ::= <Op Or> '?' <Op If> ':' <Op If> */
#define PS_OPIF                                                                       96  /* <Op If> ::= <Op Or> */
#define PS_OPOR_PIPEPIPE                                                              97  /* <Op Or> ::= <Op Or> '||' <Op And> */
#define PS_OPOR                                                                       98  /* <Op Or> ::= <Op And> */
#define PS_OPAND_AMPAMP                                                               99  /* <Op And> ::= <Op And> '&&' <Op BinOR> */
#define PS_OPAND                                                                      100 /* <Op And> ::= <Op BinOR> */
#define PS_OPBINOR_PIPE                                                               101 /* <Op BinOR> ::= <Op BinOR> '|' <Op BinXOR> */
#define PS_OPBINOR                                                                    102 /* <Op BinOR> ::= <Op BinXOR> */
#define PS_OPBINXOR_CARET                                                             103 /* <Op BinXOR> ::= <Op BinXOR> '^' <Op BinAND> */
#define PS_OPBINXOR                                                                   104 /* <Op BinXOR> ::= <Op BinAND> */
#define PS_OPBINAND_AMP                                                               105 /* <Op BinAND> ::= <Op BinAND> '&' <Op Equate> */
#define PS_OPBINAND                                                                   106 /* <Op BinAND> ::= <Op Equate> */
#define PS_OPEQUATE_EQEQ                                                              107 /* <Op Equate> ::= <Op Equate> '==' <Op Compare> */
#define PS_OPEQUATE_EXCLAMEQ                                                          108 /* <Op Equate> ::= <Op Equate> '!=' <Op Compare> */
#define PS_OPEQUATE                                                                   109 /* <Op Equate> ::= <Op Compare> */
#define PS_OPCOMPARE_GT                                                               110 /* <Op Compare> ::= <Op Compare> '>' <Op Shift> */
#define PS_OPCOMPARE_GTEQ                                                             111 /* <Op Compare> ::= <Op Compare> '>=' <Op Shift> */
#define PS_OPCOMPARE_LT                                                               112 /* <Op Compare> ::= <Op Compare> '<' <Op Shift> */
#define PS_OPCOMPARE_LTEQ                                                             113 /* <Op Compare> ::= <Op Compare> '<=' <Op Shift> */
#define PS_OPCOMPARE                                                                  114 /* <Op Compare> ::= <Op Shift> */
#define PS_OPSHIFT_LTLT                                                               115 /* <Op Shift> ::= <Op Shift> '<<' <Op AddSub> */
#define PS_OPSHIFT_GTGT                                                               116 /* <Op Shift> ::= <Op Shift> '>>' <Op AddSub> */
#define PS_OPSHIFT                                                                    117 /* <Op Shift> ::= <Op AddSub> */
#define PS_OPADDSUB_PLUS                                                              118 /* <Op AddSub> ::= <Op AddSub> '+' <Op MultDiv> */
#define PS_OPADDSUB_MINUS                                                             119 /* <Op AddSub> ::= <Op AddSub> '-' <Op MultDiv> */
#define PS_OPADDSUB                                                                   120 /* <Op AddSub> ::= <Op MultDiv> */
#define PS_OPMULTDIV_TIMES                                                            121 /* <Op MultDiv> ::= <Op MultDiv> '*' <Op Unary> */
#define PS_OPMULTDIV_DIV                                                              122 /* <Op MultDiv> ::= <Op MultDiv> '/' <Op Unary> */
#define PS_OPMULTDIV_PERCENT                                                          123 /* <Op MultDiv> ::= <Op MultDiv> '%' <Op Unary> */
#define PS_OPMULTDIV                                                                  124 /* <Op MultDiv> ::= <Op Unary> */
#define PS_OPUNARY_EXCLAM                                                             125 /* <Op Unary> ::= '!' <Op Unary> */
#define PS_OPUNARY_TILDE                                                              126 /* <Op Unary> ::= '~' <Op Unary> */
#define PS_OPUNARY_MINUS                                                              127 /* <Op Unary> ::= '-' <Op Unary> */
#define PS_OPUNARY                                                                    128 /* <Op Unary> ::= <Op Post> */
#define PS_OPUNARY2                                                                   129 /* <Op Unary> ::= <Op Pre> */
#define PS_OPUNARY3                                                                   130 /* <Op Unary> ::= <Op Cast> */
#define PS_OPUNARY4                                                                   131 /* <Op Unary> ::= <Op SizeOf> */
#define PS_OPUNARY5                                                                   132 /* <Op Unary> ::= <Op Pointer> */
#define PS_OPUNARY6                                                                   133 /* <Op Unary> ::= <Value> */
#define PS_OPPOST_PLUSPLUS                                                            134 /* <Op Post> ::= <Op Pointer> '++' */
#define PS_OPPOST_MINUSMINUS                                                          135 /* <Op Post> ::= <Op Pointer> '--' */
#define PS_OPPRE_PLUSPLUS                                                             136 /* <Op Pre> ::= '++' <Op Unary> */
#define PS_OPPRE_MINUSMINUS                                                           137 /* <Op Pre> ::= '--' <Op Unary> */
#define PS_OPCAST_LPARAN_RPARAN                                                       138 /* <Op Cast> ::= '(' <Type> ')' <Op Unary> */
#define PS_OPSIZEOF_SIZEOF_LPARAN_RPARAN                                              139 /* <Op SizeOf> ::= sizeof '(' <Type> ')' */
#define PS_OPSIZEOF_SIZEOF_LPARAN_ID_RPARAN                                           140 /* <Op SizeOf> ::= sizeof '(' Id ')' */
#define PS_OPPOINTER_DOT_ID                                                           141 /* <Op Pointer> ::= <Op Pointer> '.' Id */
#define PS_OPPOINTER_DOT                                                              142 /* <Op Pointer> ::= <Op Pointer> '.' <RetValues> */
#define PS_OPPOINTER_LBRACKET_RBRACKET                                                143 /* <Op Pointer> ::= <Op Pointer> '[' <Expr> ']' */
#define PS_OPPOINTER_ID                                                               144 /* <Op Pointer> ::= Id */
#define PS_VALUE                                                                      145 /* <Value> ::= <FixValue> */
#define PS_VALUE_LPARAN_RPARAN                                                        146 /* <Value> ::= '(' <Expr> ')' */
#define PS_FIXVALUE_HEXLITERAL                                                        147 /* <FixValue> ::= HexLiteral */
#define PS_FIXVALUE_DECLITERAL                                                        148 /* <FixValue> ::= DecLiteral */
#define PS_FIXVALUE_STRINGLITERAL                                                     149 /* <FixValue> ::= StringLiteral */
#define PS_FIXVALUE_CHARLITERAL                                                       150 /* <FixValue> ::= CharLiteral */
#define PS_FIXVALUE_FLOATLITERAL                                                      151 /* <FixValue> ::= FloatLiteral */
#define PS_FIXVALUE                                                                   152 /* <FixValue> ::= <RetValues> */
#define PS_RETVALUES_ID_LPARAN_RPARAN                                                 153 /* <RetValues> ::= Id '(' <Expr List> ')' */
#define PS_RETVALUES_ID_LPARAN_RPARAN2                                                154 /* <RetValues> ::= Id '(' ')' */
