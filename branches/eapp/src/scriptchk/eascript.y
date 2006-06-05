
%token APOST
%token MINUS
%token MINUSMINUS
%token EXCLAM
%token EXCLAMEQ
%token PERCENT
%token AMP
%token AMPAMP
%token AMPEQ
%token LPARAN
%token RPARAN
%token TIMES
%token TIMESEQ
%token COMMA
%token DOT
%token DIV
%token DIVEQ
%token COLON
%token COLONCOLON
%token SEMI
%token QUESTION
%token LBRACKET
%token RBRACKET
%token CARET
%token CARETEQ
%token LBRACE
%token PIPE
%token PIPEPIPE
%token PIPEEQ
%token RBRACE
%token TILDE
%token PLUS
%token PLUSPLUS
%token PLUSEQ
%token LT
%token LTLT
%token LTLTEQ
%token LTEQ
%token EQ
%token MINUSEQ
%token EQEQ
%token GT
%token GTEQ
%token GTGT
%token GTGTEQ
%token ACCOUNT
%token AUTO
%token BREAK
%token CASE
%token CHAR
%token CHARLITERAL
%token CONST
%token CONTINUE
%token DECLITERAL
%token DEFAULT
%token DO
%token DOUBLE
%token ELSE
%token END
%token EXTERN
%token FLOATLITERAL
%token FOR
%token GLOBAL
%token GOTO
%token HEXLITERAL
%token ID
%token IF
%token INT
%token OLDDUPHEAD
%token OLDFUNCHEAD
%token OLDMAPFLAGHEAD
%token OLDMINSCRIPTHEAD
%token OLDMONSTERHEAD
%token OLDSCRIPTHEAD
%token OLDSHOPHEAD
%token OLDWARPHEAD
%token RETURN
%token SIZEOF
%token STRING
%token STRINGLITERAL
%token SWITCH
%token TEMP
%token WHILE

%start decls

%%

decls : decl decls 
      | 
      ;

decl : func 
     | script 
     | oldscript 
     | oldfunc 
     | oldmapflag 
     | oldnpc 
     | olddup 
     | oldmob 
     | oldshop 
     | oldwarp 
     | olditem 
     | npc 
     | mob 
     | shop 
     | item 
     | warp 
     | block 
     ;

id : ID 
   ;

oldmapflag : OLDMAPFLAGHEAD 
           ;

oldscript : OLDMINSCRIPTHEAD block 
          ;

oldfunc : OLDFUNCHEAD block 
        ;

oldnpc : OLDSCRIPTHEAD block 
       ;

olddup : OLDDUPHEAD 
       ;

oldshop : OLDSHOPHEAD 
        ;

oldwarp : OLDWARPHEAD 
        ;

oldmob : OLDMONSTERHEAD 
       ;

olditem : DECLITERAL COMMA ids COMMA ids COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA block COMMA block 
        | DECLITERAL COMMA ids COMMA ids COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA block 
        ;

ne : DECLITERAL 
   | MINUS 
   | DEFAULT 
   | 
   ;

ids : idelem 
    | idelem idlimiter ids 
    ;

idlimiter : MINUS 
          | DOT 
          | APOST 
          | 
          ;

idelem : ID 
       | DECLITERAL 
       | FOR 
       ;

script : ID ID block 
       ;

func : scalar ID LPARAN paramse RPARAN SEMI 
     | scalar ID LPARAN paramse RPARAN block 
     ;

npc : mappos COMMA dir ID nameid spriteid DECLITERAL COMMA DECLITERAL COMMA event 
    | mappos COMMA dir ID nameid spriteid event 
    ;

mob : mappos COMMA pos ID nameid spriteid DECLITERAL COMMA DECLITERAL COMMA DECLITERAL COMMA event 
    ;

warp : mappos COMMA dir ID nameid spriteid DECLITERAL COMMA mappos 
     ;

shop : mappos COMMA dir ID nameid spriteid pricelist 
     ;

item : DECLITERAL COMMA ids COMMA STRINGLITERAL COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA ne COMMA event 
     ;

event : id 
      | id COLONCOLON id 
      | block 
      | MINUS 
      ;

dir : DECLITERAL 
    ;

pos : DECLITERAL COMMA DECLITERAL 
    ;

mappos : STRINGLITERAL COMMA DECLITERAL COMMA DECLITERAL 
       ;

pricelist : price COMMA pricelist 
          | price 
          ;

price : DECLITERAL COLON DECLITERAL 
      | DECLITERAL COLON MINUS DECLITERAL 
      ;

paramse : params 
        | 
        ;

params : param COMMA params 
       | param 
       ;

param : conste scalare id 
      ;

conste : CONST 
       | 
       ;

scalare : scalar 
        | 
        ;

spriteid : DECLITERAL COMMA 
         | MINUS DECLITERAL COMMA 
         ;

nameid : STRINGLITERAL 
       | STRINGLITERAL COLONCOLON id 
       ;

var_decl : type var_list SEMI 
         ;

type : mod scalar 
     | scalar mod 
     | scalar 
     | mod 
     ;

var_list : var COMMA var_list 
         | var 
         ;

var : id array 
    | id array EQ multilist 
    | id 
    | id EQ op_if 
    ;

multilist : LBRACE initlist RBRACE 
          ;

initlist : op_if COMMA initlist 
         | multilist COMMA initlist 
         | op_if 
         | multilist 
         ;

array : LBRACKET condition RBRACKET 
      | LBRACKET condition RBRACKET array 
      ;

mod : GLOBAL 
    | EXTERN 
    | TEMP 
    | CHAR 
    | ACCOUNT 
    ;

scalar : STRING 
       | DOUBLE 
       | INT 
       | AUTO 
       ;

block : LBRACE stm_list RBRACE 
      ;

stm_list : stm stm_list 
         | 
         ;

stm : var_decl 
    | label_stm 
    | normal_stm 
    ;

label_stm : id COLON 
          ;

normal_stm : IF LPARAN expr RPARAN normal_stm 
           | IF LPARAN expr RPARAN normal_stm ELSE normal_stm 
           | WHILE LPARAN expr RPARAN normal_stm 
           | FOR LPARAN arg SEMI condition SEMI arg RPARAN normal_stm 
           | DO normal_stm WHILE LPARAN expr RPARAN SEMI 
           | SWITCH LPARAN expr RPARAN LBRACE case_stms RBRACE 
           | block 
           | goto_stms 
           | lctr_stms 
           | return_stms 
           | call_stm 
           | expr_list SEMI 
           | SEMI 
           ;

goto_stms : GOTO id SEMI 
          ;

lctr_stms : BREAK SEMI 
          | CONTINUE SEMI 
          ;

condition : expr 
          | 
          ;

arg : expr_list 
    | id call_list 
    | 
    ;

case_stms : CASE op_if COLON stm_list case_stms 
          | DEFAULT COLON stm_list case_stms 
          | 
          ;

call_stm : id call_list SEMI 
         | id SEMI 
         ;

call_liste : call_list 
           | 
           ;

call_list : call_arg COMMA call_list 
          | call_arg 
          ;

call_arg : expr 
         | MINUS 
         ;

retvalues : id LPARAN call_liste RPARAN 
          ;

return_stms : RETURN arg SEMI 
            | END SEMI 
            ;

expr_list : expr COMMA expr_list 
          | expr 
          ;

expr : op_assign 
     ;

op_assign : op_if EQ op_assign 
          | op_if PLUSEQ op_assign 
          | op_if MINUSEQ op_assign 
          | op_if TIMESEQ op_assign 
          | op_if DIVEQ op_assign 
          | op_if CARETEQ op_assign 
          | op_if AMPEQ op_assign 
          | op_if PIPEEQ op_assign 
          | op_if GTGTEQ op_assign 
          | op_if LTLTEQ op_assign 
          | op_if 
          ;

op_if : op_or QUESTION op_if COLON op_if 
      | op_or 
      ;

op_or : op_or PIPEPIPE op_and 
      | op_and 
      ;

op_and : op_and AMPAMP op_binor 
       | op_binor 
       ;

op_binor : op_binor PIPE op_binxor 
         | op_binxor 
         ;

op_binxor : op_binxor CARET op_binand 
          | op_binand 
          ;

op_binand : op_binand AMP op_equate 
          | op_equate 
          ;

op_equate : op_equate EQEQ op_compare 
          | op_equate EXCLAMEQ op_compare 
          | op_compare 
          ;

op_compare : op_compare GT op_shift 
           | op_compare GTEQ op_shift 
           | op_compare LT op_shift 
           | op_compare LTEQ op_shift 
           | op_shift 
           ;

op_shift : op_shift LTLT op_addsub 
         | op_shift GTGT op_addsub 
         | op_addsub 
         ;

op_addsub : op_addsub PLUS op_multdiv 
          | op_addsub MINUS op_multdiv 
          | op_multdiv 
          ;

op_multdiv : op_multdiv TIMES op_unary 
           | op_multdiv DIV op_unary 
           | op_multdiv PERCENT op_unary 
           | op_unary 
           ;

op_unary : EXCLAM op_unary 
         | TILDE op_unary 
         | MINUS op_unary 
         | PLUS op_unary 
         | op_post 
         | op_pre 
         | op_cast 
         | op_sizeof 
         | op_pointer 
         | value 
         ;

op_post : op_pointer PLUSPLUS 
        | op_pointer MINUSMINUS 
        ;

op_pre : PLUSPLUS op_pointer 
       | MINUSMINUS op_pointer 
       ;

op_cast : LPARAN scalar RPARAN op_unary 
        ;

op_sizeof : SIZEOF LPARAN scalar RPARAN 
          | SIZEOF LPARAN id RPARAN 
          ;

op_pointer : op_pointer DOT id 
           | op_pointer DOT retvalues 
           | op_pointer LBRACKET expr RBRACKET 
           | id 
           | retvalues 
           ;

value : HEXLITERAL 
      | DECLITERAL 
      | FLOATLITERAL 
      | CHARLITERAL 
      | STRINGLITERAL 
      | LPARAN expr RPARAN 
      ;
