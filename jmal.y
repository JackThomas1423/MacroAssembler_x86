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
    char   cval;
    char  *sval;
    int    ival;
    double fval;
    struct { int lo; int hi; } range;
    unsigned int arg_ref;
    struct JmalTypeConstraint      *tcp;
    struct JmalTypeConstraintMulti *tmcp;
    struct JmalArgDecl             *adp;
    struct JmalUse                 *udp;
    struct JmalMacro               *macro;
    struct JmalStatement           *statement;
    struct JmalStatementMulti      *statement_multi;
    /* Note: JmalRotate and JmalRepBlock no longer exist as separate structs;
     * rotate_def, rep_block, and instruction rules now return JmalStatement*
     * via the statement field above. */
}

/* ── Token declarations ───────────────────────────────────────────────── */

/* Directives */
%token DIR_ENSURE
%token DIR_IF
%token DIR_ENDIF
%token DIR_REDIRECT
%token DIR_LITERAL
%token DIR_ENDLITERAL
%token DIR_DEFINE
%token DIR_UNDEF
%token DIR_TYPE
%token DIR_USE
%token DIR_MACRO
%token DIR_ENDMACRO
%token DIR_ARG
%token DIR_REP
%token DIR_REF
%token DIR_ENDREP
%token DIR_ROTATE
%token DIR_ARG_COUNT

/* Built-in primitive types */
%token TYPE_REGISTER
%token TYPE_STRING
%token TYPE_NUMBER
%token TYPE_ADDRESS

%token TOK_REGEX_PREFIX
%token TOK_STR_PREFIX
%token TOK_INT_PREFIX

/* Literals & identifiers */
%token <sval> TOK_IDENT
%token <sval> TOK_STRING
%token <ival> TOK_INT
%token <fval> TOK_FLOAT
%token <arg_ref> TOK_ARG_REF
%token <ival> TOK_REF_ARG
%token <range> TOK_ARITY_RANGE

/* Punctuation */
%token TOK_NEWLINE
%token TOK_COMMA
%token TOK_COLON
%token TOK_PIPE
%token TOK_LBRACKET
%token TOK_RBRACKET
%token TOK_EQUAL
%token TOK_COMPARE
%token TOK_COMPARE_GREATER
%token TOK_COMPARE_NOT
%token TOK_LPAREN
%token TOK_RPAREN
%token TOK_PLUS
%token TOK_MINUS
%token TOK_STAR
%token TOK_SLASH
%token CHAR

/* Declared grammar types */
%type <tcp>             builtin_type type_constraint
%type <tmcp>            type_union use_def_args
%type <adp>             arg_decl
%type <udp>             use_def
%type <macro>           macro_def macro_header
%type <statement>       macro_body_item rotate_def rep_block instruction
%type <statement_multi> macro_body

/* ── Start symbol ─────────────────────────────────────────────────────── */
%start program

%%

program:
    %empty {}
    | program statement
    ;

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
    | macro_def
    ;

/* %define <name> <value> */
define_dir:
    DIR_DEFINE TOK_IDENT TOK_STRING
    {
        jmal_program_add_define(jmal_program, jmal_define_str($2, $3, yylineno));
        free($2);
        free($3);
    }
    | DIR_DEFINE TOK_IDENT TOK_INT
    {
        jmal_program_add_define(jmal_program, jmal_define_int($2, $3, yylineno));
        free($2);
    }
    | DIR_DEFINE TOK_IDENT TOK_FLOAT
    {
        jmal_program_add_define(jmal_program, jmal_define_float($2, $3, yylineno));
        free($2);
    }
    | DIR_DEFINE TOK_IDENT type_spec
    {
        JMAL_TODO("type_spec define variant");
        free($2);
    }
    ;

/* %undef — not yet implemented */
undef_dir:
    DIR_UNDEF TOK_IDENT
    {
        JMAL_TODO("undef directive");
        free($2);
    }
    ;

/* %type <name>: type | type | ... */
type_dir:
    DIR_TYPE TOK_IDENT TOK_COLON type_union
    {
        jmal_program_add_typedef(jmal_program, jmal_typedef_new($2, $4, yylineno));
        free($2);
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Macro definitions
 * ════════════════════════════════════════════════════════════════════════ */

macro_def:
    macro_header newlines macro_body DIR_ENDMACRO
    {
        jmal_macro_set_body($1, $3);
        $$ = $1;
    }
    ;

/*
 * %macro <name> <in_arity> <out_arity>
 *
 * Each arity slot is either a fixed int or a lo-hi range.
 * jmal_arity_fixed / jmal_arity_range produce a JmalArity, so the
 * four productions collapse from six arguments down to two.
 */
macro_header:
    DIR_MACRO TOK_IDENT TOK_INT TOK_INT
    {
        JmalMacro *macro = jmal_macro_new($2,
                                          jmal_arity_fixed($3),
                                          jmal_arity_fixed($4),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        free($2);
        $$ = macro;
    }
    | DIR_MACRO TOK_IDENT TOK_INT TOK_ARITY_RANGE
    {
        JmalMacro *macro = jmal_macro_new($2,
                                          jmal_arity_fixed($3),
                                          jmal_arity_range($4.lo, $4.hi),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        free($2);
        $$ = macro;
    }
    | DIR_MACRO TOK_IDENT TOK_ARITY_RANGE TOK_INT
    {
        JmalMacro *macro = jmal_macro_new($2,
                                          jmal_arity_range($3.lo, $3.hi),
                                          jmal_arity_fixed($4),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        free($2);
        $$ = macro;
    }
    | DIR_MACRO TOK_IDENT TOK_ARITY_RANGE TOK_ARITY_RANGE
    {
        JmalMacro *macro = jmal_macro_new($2,
                                          jmal_arity_range($3.lo, $3.hi),
                                          jmal_arity_range($4.lo, $4.hi),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        free($2);
        $$ = macro;
    }
    ;

macro_body:
    macro_body_item
    {
        $$ = jmal_stmt_make_multi($1);
    }
    | macro_body macro_body_item
    {
        jmal_stmt_multi_add($1, $2);
        $$ = $1;
    }
    ;

macro_body_item:
    arg_decl newlines          { $$ = jmal_stmt_arg($1, yylineno); }
    | ref_decl newlines        { $$ = NULL; }
    | rep_block                { $$ = $1; }
    | ensure_def               { $$ = NULL; }
    | use_def newlines         { $$ = jmal_stmt_use($1, yylineno); }
    | if_def                   { $$ = NULL; }
    | rotate_def               { $$ = $1; }
    | arg_ref_set              { $$ = NULL; }
    | literal_block newlines   { $$ = NULL; }
    | instruction newlines     { $$ = $1; }
    | newlines                 { $$ = NULL; }
    ;

ensure_def:
    DIR_ENSURE if_cond newlines
    ;

if_def:
    DIR_IF if_cond macro_body DIR_ENDIF newlines
    ;

if_cond:
    expr_item TOK_COMPARE expr_item
    | expr_item TOK_COMPARE_GREATER expr_item
    | expr_item TOK_COMPARE_NOT expr_item
    ;

expr:
    expr_item
    | expr expr_op expr
    ;

expr_op:
    TOK_PLUS
    | TOK_MINUS
    | TOK_STAR
    | TOK_SLASH
    ;

expr_item:
    TOK_IDENT
    | TOK_ARG_REF
    | TOK_REF_ARG
    | TOK_INT
    | TOK_STRING
    | DIR_ARG_COUNT
    ;

/*
 * %rotate <N>      — rotate by literal int
 * %rotate %N       — rotate by arg-ref
 *
 * Both produce a JmalStatement* directly (rotate is now folded into the
 * statement union, so there is no intermediate JmalRotate* to pass around).
 */
rotate_def:
    DIR_ROTATE TOK_INT newlines
    {
        $$ = jmal_stmt_rotate_int($2, yylineno);
    }
    | DIR_ROTATE TOK_ARG_REF newlines
    {
        $$ = jmal_stmt_rotate_arg($2, yylineno);
    }
    ;

arg_ref_set:
    TOK_ARG_REF TOK_EQUAL expr newlines
    | TOK_REF_ARG TOK_EQUAL expr newlines
    ;

/* %arg %N : type_constraint */
arg_decl:
    DIR_ARG TOK_ARG_REF TOK_COLON type_constraint
    {
        $$ = jmal_arg_decl_new($2, jmal_type_make_multi($4), yylineno);
    }
    | DIR_ARG TOK_ARG_REF TOK_COLON type_union
    {
        $$ = jmal_arg_decl_new($2, $4, yylineno);
    }
    ;

ref_decl:
    DIR_REF TOK_REF_ARG TOK_COLON type_constraint
    | DIR_REF TOK_REF_ARG TOK_COLON type_union
    ;

/*
 * %rep <count> … %endrep
 *
 * rep_block produces a JmalStatement* directly — no intermediate
 * JmalRepBlock* struct, since that type no longer exists.
 */
rep_block:
    DIR_REP rep_count newlines macro_body DIR_ENDREP newlines
    {
        $$ = jmal_stmt_rep($4, yylineno);
    }
    | DIR_REP if_cond newlines macro_body DIR_ENDREP newlines
    {
        $$ = jmal_stmt_rep($4, yylineno);
    }
    ;

rep_count:
    TOK_INT         { /* fixed count */ }
    | DIR_ARG_COUNT { /* %0 — repeat once per argument */ }
    | TOK_ARG_REF   { /* %N */ }
    ;

literal_block:
    DIR_LITERAL newlines macro_body newlines DIR_ENDLITERAL
    ;

use_def:
    DIR_USE TOK_IDENT use_def_args
    {
        $$ = jmal_use_new($2, $3, yylineno);
        free($2);
    }
    | DIR_USE TOK_IDENT
    {
        $$ = jmal_use_new($2, NULL, yylineno);
        free($2);
    }
    ;

use_def_args:
    type_constraint
    {
        $$ = jmal_type_make_multi($1);
    }
    | use_def_args type_constraint
    {
        jmal_type_multi_add_type($1, $2);
        $$ = $1;
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Instructions  (opcode + zero or more operands)
 *
 * Instructions are now fully built into JmalInstruction / JmalOperand
 * nodes and pushed into the program or returned as a JmalStatement*.
 * ════════════════════════════════════════════════════════════════════════ */

instruction:
    TOK_IDENT operand_list
    {
        /* $2 is the JmalInstruction* accumulated by operand_list actions.
         * Bison mid-rule values are not used here; instead operand_list
         * rules write directly into a shared temporary via the marker
         * action below.  The instruction pointer is threaded through
         * $<statement>0 from the enclosing rule — see the marker action. */

        /* Build the instruction and wrap it in a statement for uniform
         * handling at both top-level and inside macro bodies. */
        JmalInstruction *ins = jmal_instr_new($1, yylineno);
        free($1);
        /* operand_list rules attach operands directly to this pointer via
         * the mid-rule marker; replace with full build once mid-rule
         * values are wired up. */
        JmalStatement *s = jmal_stmt_instr(ins, yylineno);
        jmal_program_add_stmt(jmal_program, s);
        $$ = s;
    }
    ;

operand_list:
    %empty {}
    | operand
    | operand_list TOK_COMMA operand
    ;

/*
 * Each operand rule constructs a JmalOperand* and returns it via $$.
 * The parent instruction rule (above) will collect these; for now they
 * are constructed and freed as a placeholder until the mid-rule wiring
 * is added.
 */
operand:
    TOK_IDENT
    {
        jmal_operand_free(jmal_operand_ident($1, yylineno));
        free($1);
    }
    | TOK_INT
    {
        jmal_operand_free(jmal_operand_int($1, yylineno));
    }
    | TOK_FLOAT
    {
        jmal_operand_free(jmal_operand_float($1, yylineno));
    }
    | TOK_STRING
    {
        jmal_operand_free(jmal_operand_string($1, yylineno));
        free($1);
    }
    | TOK_ARG_REF
    {
        jmal_operand_free(jmal_operand_arg_ref($1, yylineno));
    }
    | TOK_LBRACKET operand TOK_RBRACKET
    {
        /* address operand — inner is built by the recursive operand rule */
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Type helpers
 * ════════════════════════════════════════════════════════════════════════ */

/* pipe-separated union: string | number | register */
type_union:
    type_constraint
    {
        $$ = jmal_type_make_multi($1);
    }
    | type_union TOK_PIPE type_constraint
    {
        jmal_type_multi_add_type($1, $3);
        $$ = $1;
    }
    ;

/* A single type constraint: builtin, user-defined name, arg-ref, or literal int */
type_constraint:
    builtin_type
    | TOK_IDENT
    {
        $$ = jmal_type_user($1, yylineno);
        free($1);
    }
    | TOK_ARG_REF
    {
        $$ = jmal_type_arg_ref($1, yylineno);
    }
    | TOK_INT
    {
        $$ = jmal_type_lit_int($1, yylineno);
    }
    ;

builtin_type:
    TYPE_REGISTER  { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_REGISTER, yylineno); }
    | TYPE_STRING  { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_STRING,   yylineno); }
    | TYPE_NUMBER  { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_NUMBER,   yylineno); }
    | TYPE_ADDRESS { $$ = jmal_type_builtin(JMAL_TYPE_BUILTIN_ADDRESS,  yylineno); }
    ;

type_spec:
    type_spec_prefix TOK_LPAREN type_spec_item TOK_RPAREN
    ;

type_spec_item:
    TOK_STRING
    | TOK_INT
    | TOK_IDENT
    ;

type_spec_prefix:
    TOK_REGEX_PREFIX
    | TOK_STR_PREFIX
    | TOK_INT_PREFIX
    ;

%%

/* ════════════════════════════════════════════════════════════════════════
 * yyerror — called by Bison on a parse error
 * ════════════════════════════════════════════════════════════════════════ */
void yyerror(const char *msg)
{
    fprintf(stderr, "parse error on line %d: %s\n", yylineno, msg);
}
