%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jmal_ast.h"

/* yylex / yyerror are defined in the generated lexer and below */
int  yylex(void);
void yyerror(const char *msg);

extern int yylineno;   /* provided by flex %option yylineno */
extern JmalProgram *jmal_program;
%}

%union {
    char  *sval;
    int    ival;
    double fval;
    struct { int lo; int hi; } range;
    struct JmalTypeConstraint *tcp; /* type constraint pointer */
    struct JmalTypeConstraintMulti *tmcp; /* type multi constraint pointer */
}

/* ── Token declarations ───────────────────────────────────────────────── */

/* Directives */
%token DIR_LITERAL
%token DIR_ENDLITERAL
%token DIR_DEFINE
%token DIR_UNDEF
%token DIR_TYPE
%token DIR_USE
%token DIR_MACRO_STRICT
%token DIR_MACRO
%token DIR_ENDMACRO
%token DIR_ARG
%token DIR_REP
%token DIR_ENDREP
%token DIR_ROTATE
%token DIR_ARG_COUNT

/* Built-in primitive types */
%token TYPE_REGISTER
%token TYPE_STRING
%token TYPE_NUMBER
%token TYPE_ADDRESS

/* Literals & identifiers – note the <field> links to the %union */
%token <sval> TOK_IDENT
%token <sval> TOK_STRING
%token <ival> TOK_INT
%token <fval> TOK_FLOAT
%token <ival> TOK_ARG_REF
%token <range> TOK_ARITY_RANGE

/* Punctuation */
%token TOK_NEWLINE
%token TOK_COMMA
%token TOK_COLON
%token TOK_PIPE
%token TOK_LBRACKET
%token TOK_RBRACKET
%token TOK_LPAREN
%token TOK_RPAREN
%token TOK_PLUS
%token TOK_MINUS
%token TOK_STAR
%token TOK_SLASH

/* Declared grammar types */
%type <tcp> builtin_type type_constraint
%type <tmcp> type_union

/* ── Start symbol ─────────────────────────────────────────────────────── */
%start program

%%

program:
    %empty {}
    | program statement
    ;

/* A statement is any top-level construct followed by at least one newline.
 * Blank lines are handled by the newlines rule below. */
statement:
    directive newlines
    | instruction newlines
    | newlines
    ;

newlines:
    %empty {}
    | TOK_NEWLINE
    | newlines TOK_NEWLINE
    ;

directive:
    define_dir
    | undef_dir
    | type_dir
    | use_dir
    | macro_def
    ;

/* %define <name> <value> */
define_dir:
    DIR_DEFINE TOK_IDENT TOK_STRING
    {
        JmalDefine* definition_dir = jmal_define_str($2, $3, yylineno);
        jmal_program_add_define(jmal_program, definition_dir);
        free($2);
        free($3);
    }
    | DIR_DEFINE TOK_IDENT TOK_INT
    {
        JmalDefine* definition_dir = jmal_define_int($2, $3, yylineno);
        jmal_program_add_define(jmal_program, definition_dir);
        free($2);
    }
    ;

/* %undef  (standalone — also consumed inside table blocks above) */
undef_dir:
    DIR_UNDEF
    { printf("undef\n"); }
    ;

/* %type <name>: type | type | ... */
type_dir:
    DIR_TYPE TOK_IDENT TOK_COLON type_union
    {
        JmalTypeDef* type_dir = jmal_typedef_new($2, $4, yylineno);
        jmal_program_add_typedef(jmal_program, type_dir);
        free($2);
    }
    ;

/* %use <table_name> */
use_dir:
    DIR_USE TOK_IDENT
    { printf("use table '%s'\n", $2); free($2); }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Macro definitions
 * ════════════════════════════════════════════════════════════════════════ */

macro_def:
    macro_header newlines macro_body DIR_ENDMACRO
    { printf("endmacro\n"); }
    ;

macro_header:
    DIR_MACRO TOK_IDENT TOK_INT
    { printf("macro '%s' arity %d\n", $2, $3); free($2); }
    | DIR_MACRO TOK_IDENT TOK_ARITY_RANGE
    { printf("macro '%s' arity %d-%d\n", $2, $3.lo, $3.hi); free($2); }
    | DIR_MACRO_STRICT TOK_IDENT TOK_INT
    { printf("macro.strict '%s' arity %d\n", $2, $3); free($2); }
    | DIR_MACRO_STRICT TOK_IDENT TOK_ARITY_RANGE
    { printf("macro.strict '%s' arity %d-%d\n", $2, $3.lo, $3.hi); free($2); }
    ;

macro_body:
    %empty {}
    | macro_body macro_body_item
    ;

macro_body_item:
    arg_decl newlines
    | rep_block
    | use_def
    | literal_block newlines
    | instruction newlines
    | newlines
    ;

/* %arg %N : type_constraint */
arg_decl:
    DIR_ARG TOK_ARG_REF TOK_COLON type_constraint
    { printf("  arg %%%d\n", $2); }
    |   DIR_ARG TOK_ARG_REF TOK_COLON type_union
    { printf("  arg %%%d\n", $2); }
    ;

/* %rep %0 … %endrep */
rep_block:
    DIR_REP rep_count newlines rep_body DIR_ENDREP newlines
    { printf("  rep block\n"); }
    ;

rep_count:
    TOK_INT         { /* fixed count */ }
    | DIR_ARG_COUNT { /* %0 — repeat once per argument */ }
    | TOK_ARG_REF   { /* %1-9 */}
    ;

rep_body:
    %empty {}
    | rep_body rep_body_item
    ;

rep_body_item:
    instruction newlines
    | DIR_ROTATE TOK_INT newlines     { printf("    rotate %d\n", $2); }
    | DIR_ROTATE TOK_ARG_REF newlines { printf("    rotate %%%d\n", $2); }
    | use_def newlines
    | newlines
    ;

literal_block:
    DIR_LITERAL newlines macro_body newlines DIR_ENDLITERAL
    ;

use_def:
    DIR_USE TOK_IDENT use_def_items newlines
    {
        printf("  use: %s\n", $2);
    }
    ;

use_def_items:
    type_constraint { }
    | TOK_ARG_REF   { printf("  arg: %%%d\n", $1); }
    | TOK_INT       { printf("  num: %d\n", $1); }
    | TOK_IDENT     { printf("  ident: %s\n", $1); }
    | use_def_items use_def_items
    ;

    /* ════════════════════════════════════════════════════════════════════════
 * Instructions  (opcode + zero or more operands)
 * ════════════════════════════════════════════════════════════════════════ */

instruction:
    TOK_IDENT operand_list
    {
        printf("instr '%s'\n", $1);
        free($1);
    }
    ;

operand_list:
    %empty {}
    | operand
    | operand_list TOK_COMMA operand
    ;

operand:
    TOK_IDENT           { printf("  operand: ident '%s'\n", $1); free($1); }
    | TOK_INT           { printf("  operand: int %d\n",   $1); }
    | TOK_FLOAT         { printf("  operand: float %f\n", $1); }
    | TOK_STRING        { printf("  operand: str \"%s\"\n", $1); free($1); }
    | TOK_ARG_REF       { printf("  operand: arg-ref %%%d\n", $1); }
    | TOK_LBRACKET operand TOK_RBRACKET   /* memory address [x] */
    { printf("  operand: address\n"); }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Type helpers
 * ════════════════════════════════════════════════════════════════════════ */

/* pipe-separated union used in %type:  string | number | register */
type_union:
    type_constraint
    {
        $$ = jmal_type_make_multi($1);
    }
    | type_union TOK_PIPE builtin_type
    {
        jmal_type_multi_add_type($1, $3);
        $$ = $1;
    }
    ;

/* A constraint is either a built-in primitive or a user-defined type name */
type_constraint:
    builtin_type
    | TOK_IDENT
    {
        $$ = jmal_type_user($1, yylineno);
        free($1);
    }
    ;

builtin_type:
    TYPE_REGISTER   { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_REGISTER, yylineno); }
    | TYPE_STRING   { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_STRING, yylineno);   }
    | TYPE_NUMBER   { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_NUMBER, yylineno);   }
    | TYPE_ADDRESS  { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_ADDRESS, yylineno);  }
    ;

%%

/* ════════════════════════════════════════════════════════════════════════
 * yyerror — called by Bison on a parse error
 * ════════════════════════════════════════════════════════════════════════ */
void yyerror(const char *msg)
{
    fprintf(stderr, "parse error on line %d: %s\n", yylineno, msg);
}
