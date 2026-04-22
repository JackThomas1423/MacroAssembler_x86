%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* yylex / yyerror are defined in the generated lexer and below */
int  yylex(void);
void yyerror(const char *msg);

extern int yylineno;   /* provided by flex %option yylineno */
%}

%union {
    char  *sval;
    int    ival;
    double fval;
    struct { int lo; int hi; } range;
}

/* ── Token declarations ───────────────────────────────────────────────── */

/* Directives */
%token DIR_DEFINE_TABLE
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

/* Keywords */
%token KW_FROM

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

/* ── Start symbol ─────────────────────────────────────────────────────── */
%start program

%%

program:
    /* empty */
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
    TOK_NEWLINE
    | newlines TOK_NEWLINE
    ;

directive:
    define_table_dir
    | define_dir
    | undef_dir
    | type_dir
    | use_dir
    | macro_def
    ;

/* %define.table <name>  [from <name>]
 *     rule: [type, ...]
 * %undef */
define_table_dir:
    DIR_DEFINE_TABLE TOK_IDENT newlines table_rules DIR_UNDEF
    {
        printf("table '%s' defined\n", $2);
        free($2);
    }

    | DIR_DEFINE_TABLE TOK_IDENT KW_FROM TOK_IDENT newlines table_rules DIR_UNDEF
    {
        printf("table '%s' derived from '%s'\n", $2, $4);
        free($2); free($4);
    }
    ;

table_rules:
    /* empty */
    | table_rules table_rule newlines
    ;

table_rule:
    TOK_IDENT TOK_COLON TOK_LBRACKET type_list TOK_RBRACKET
    {
        printf("  table rule '%s'\n", $1);
        free($1);
    }
    ;

/* %define <name> <value> */
define_dir:
    DIR_DEFINE TOK_IDENT TOK_STRING
    { printf("define '%s' = \"%s\"\n", $2, $3); free($2); free($3); }
    | DIR_DEFINE TOK_IDENT TOK_INT
    { printf("define '%s' = %d\n", $2, $3); free($2); }
    ;

/* %undef  (standalone — also consumed inside table blocks above) */
undef_dir:
    DIR_UNDEF
    { printf("undef\n"); }
    ;

/* %type <name>: type | type | ... */
type_dir:
    DIR_TYPE TOK_IDENT TOK_COLON type_union
    { printf("type '%s' defined\n", $2); free($2); }
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
    /* empty */
    | macro_body macro_body_item
    ;

macro_body_item:
    arg_decl newlines
    | rep_block
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
    ;

rep_body:
    /* empty */
    | rep_body rep_body_item
    ;

rep_body_item:
    instruction newlines
    | DIR_ROTATE TOK_INT newlines { printf("    rotate %d\n", $2); }
    | newlines
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Instructions  (opcode + zero or more operands)
 * ════════════════════════════════════════════════════════════════════════ */

instruction:
    TOK_IDENT operand_list
    { printf("instr '%s'\n", $1); free($1); }
    ;

operand_list:
    /* no operands */
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
    builtin_type
    | type_union TOK_PIPE builtin_type
    ;

/* comma-separated list used in %arg and table rules */
type_list:
    type_constraint
    | type_list TOK_COMMA type_constraint
    ;

/* A constraint is either a built-in primitive or a user-defined type name */
type_constraint:
    builtin_type
    | TOK_IDENT   { free($1); }   /* user-defined type */
    ;

builtin_type:
    TYPE_REGISTER   { printf("  type: register\n"); }
    | TYPE_STRING   { printf("  type: string\n");   }
    | TYPE_NUMBER   { printf("  type: number\n");   }
    | TYPE_ADDRESS  { printf("  type: address\n");  }
    ;

%%

/* ════════════════════════════════════════════════════════════════════════
 * yyerror — called by Bison on a parse error
 * ════════════════════════════════════════════════════════════════════════ */
void yyerror(const char *msg)
{
    fprintf(stderr, "parse error on line %d: %s\n", yylineno, msg);
}
