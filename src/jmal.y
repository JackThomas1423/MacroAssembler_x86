%require "3.2"
%language "c++"

%define api.value.type variant
%define api.token.constructor
%define parse.error verbose

%code requires {
    #include <string>
    #include <iostream>
    #include <src/jmal_ast.hpp>
}

%code {
    // Forward-declare the lexer function Bison will call.
    // With api.token.constructor it returns a full token object.
    yy::parser::symbol_type yylex();
    extern int yylineno;   /* provided by flex %option yylineno */
    extern JmalProgram *jmal_program;

    /* Current instruction being built — set by the instruction rule's
     * mid-rule marker and consumed by operand rules. */
    static JmalInstruction *current_instr = nullptr;
}

/* ── Token declarations ───────────────────────────────────────────────── */

/* Directives */
%token DIR_REDIRECT
%token DIR_ENSURE
%token DIR_IF
%token DIR_ENDIF
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
%token <std::string>   TOK_IDENT
%token <std::string>   TOK_STRING
%token <int>           TOK_INT
%token <double>        TOK_FLOAT
%token <unsigned int>  TOK_ARG_REF
%token <int>           TOK_REF_ARG
%token <JmalArity>     TOK_RANGE

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

/* Declared grammar types — use C++ pointer types directly, no 'struct' tag */
%type <JmalTypeConstraint*>       builtin_type type_constraint
%type <JmalTypeConstraintMulti*>  type_union use_def_args
%type <JmalArgDecl*>              arg_decl
%type <JmalUse*>                  use_def
%type <JmalMacro*>                macro_def macro_header
%type <JmalStatement*>            macro_body_item rotate_def rep_block instruction ensure_def
%type <std::string>               ensure_op ensure_operand
%type <JmalStatementMulti*>       macro_body

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
    {
        jmal_program_add_stmt(jmal_program, $1);
    }
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
        jmal_program_add_define(jmal_program,
            jmal_define_str($2.c_str(), $3.c_str(), yylineno));
    }
    | DIR_DEFINE TOK_IDENT TOK_INT
    {
        jmal_program_add_define(jmal_program,
            jmal_define_int($2.c_str(), $3, yylineno));
    }
    | DIR_DEFINE TOK_IDENT TOK_FLOAT
    {
        jmal_program_add_define(jmal_program,
            jmal_define_float($2.c_str(), $3, yylineno));
    }
    ;

/* %undef — not yet implemented */
undef_dir:
    DIR_UNDEF TOK_IDENT
    {
        JMAL_TODO("undef directive");
    }
    ;

/* %type <name>: type | type | ... */
type_dir:
    DIR_TYPE TOK_IDENT TOK_COLON type_union
    {
        jmal_program_add_typedef(jmal_program,
            jmal_typedef_new($2.c_str(), $4, yylineno));
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
        JmalMacro *macro = jmal_macro_new($2.c_str(),
                                          jmal_arity_fixed($3),
                                          jmal_arity_fixed($4),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        $$ = macro;
    }
    | DIR_MACRO TOK_IDENT TOK_RANGE TOK_INT
    {
        JmalMacro *macro = jmal_macro_new($2.c_str(),
                                          $3,
                                          jmal_arity_fixed($4),
                                          yylineno);
        jmal_program_add_macro(jmal_program, macro);
        $$ = macro;
    }
    ;

macro_body:
    macro_body_item
    {
        $$ = $1 ? jmal_stmt_make_multi($1) : new JmalStatementMulti;
    }
    | macro_body macro_body_item
    {
        if ($2) jmal_stmt_multi_add($1, $2);
        $$ = $1;
    }
    ;

macro_body_item:
    arg_decl newlines          { $$ = jmal_stmt_arg($1, yylineno); }
    | ref_decl newlines        { $$ = nullptr; }
    | rep_block                { $$ = $1; }
    | ensure_def               { $$ = $1; }
    | use_def newlines         { $$ = jmal_stmt_use($1, yylineno); }
    | if_def                   { $$ = nullptr; }
    | rotate_def               { $$ = $1; }
    | arg_ref_set              { $$ = nullptr; }
    | literal_block newlines   { $$ = nullptr; }
    | instruction newlines     { $$ = $1; }
    | newlines                 { $$ = nullptr; }
    ;

/*
 * %ensure <lhs> <op> <rhs>
 *
 * lhs / rhs are either an arg-ref (%N), an ident (type name / register),
 * a string literal, or an integer literal — all stringified for storage.
 */
ensure_def:
    DIR_ENSURE ensure_operand ensure_op ensure_operand newlines
    {
        $$ = jmal_stmt_ensure($2, $3, $4, yylineno);
    }
    ;

ensure_op:
    TOK_COMPARE         { $$ = std::string("=="); }
    | TOK_COMPARE_NOT   { $$ = std::string("!="); }
    | TOK_COMPARE_GREATER { $$ = std::string(">="); }
    ;

ensure_operand:
    TOK_ARG_REF  { $$ = "%" + std::to_string($1); }
    | TOK_IDENT  { $$ = $1; }
    | TOK_STRING { $$ = $1; }
    | TOK_INT    { $$ = std::to_string($1); }
    ;

if_def:
    DIR_IF if_cond macro_body DIR_ENDIF newlines
    {
        JMAL_TODO("if statement");
    }
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
    {
        JMAL_TODO("set expr");
    }
    | TOK_REF_ARG TOK_EQUAL expr newlines
    {
        JMAL_TODO("set expr");
    }
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
    {
        JMAL_TODO("ref declaration");
    }
    | DIR_REF TOK_REF_ARG TOK_COLON type_union
    {
        JMAL_TODO("ref declaration");
    }
    ;

/*
 * %rep <count> … %endrep
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
    {
        JMAL_TODO("literal block");
    }
    ;

use_def:
    DIR_USE TOK_IDENT use_def_args
    {
        $$ = jmal_use_new($2.c_str(), $3, yylineno);
    }
    | DIR_USE TOK_IDENT
    {
        $$ = jmal_use_new($2.c_str(), nullptr, yylineno);
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
 * A mid-rule action creates the JmalInstruction* and stores it in
 * current_instr so that each operand rule can call jmal_instr_add_operand
 * directly — no separate collector needed.
 * ════════════════════════════════════════════════════════════════════════ */

instruction:
    TOK_IDENT
    {
        /* Mid-rule: create the instruction and register it as the active
         * target so operand rules can push into it immediately. */
        current_instr = jmal_instr_new($1.c_str(), yylineno);
    }
    operand_list
    {
        /* Wrap completed instruction in a statement and return it.
         * The caller (top-level statement or macro_body_item) decides
         * where it lives — adding it here too would double-own it. */
        $$ = jmal_stmt_instr(current_instr, yylineno);
        current_instr = nullptr;
    }
    ;

operand_list:
    %empty {}
    | operand
    | operand_list TOK_COMMA operand
    ;

/*
 * Each operand rule constructs a JmalOperand* and appends it to
 * current_instr via jmal_instr_add_operand.  The address rule wraps
 * its inner operand — but because the inner operand was already pushed
 * by the recursive call we use a $<>-typed mid-rule to intercept it.
 */
operand:
    TOK_IDENT
    {
        jmal_instr_add_operand(current_instr,
            jmal_operand_ident($1.c_str(), yylineno));
    }
    | TOK_INT
    {
        jmal_instr_add_operand(current_instr,
            jmal_operand_int($1, yylineno));
    }
    | TOK_FLOAT
    {
        jmal_instr_add_operand(current_instr,
            jmal_operand_float($1, yylineno));
    }
    | TOK_STRING
    {
        jmal_instr_add_operand(current_instr,
            jmal_operand_string($1.c_str(), yylineno));
    }
    | TOK_ARG_REF
    {
        jmal_instr_add_operand(current_instr,
            jmal_operand_arg_ref(static_cast<int>($1), yylineno));
    }
    | TOK_LBRACKET
    {
        /* Mid-rule: stash and temporarily null the instruction pointer so
         * the inner operand rule pushes nowhere; we'll harvest it after. */
        /* We keep current_instr alive but record the count before the
         * inner operand is pushed so we can pop it off and rewrap it. */
    }
    operand TOK_RBRACKET
    {
        /* The inner operand was just appended as the last item.  Pop it
         * off, wrap it in an address operand, and push the wrapper. */
        size_t n = jmal_instr_operand_count(current_instr);
        JmalOperand *inner = jmal_instr_operand_get(current_instr, n - 1);
        /* Shrink the vector by one without freeing (inner is still alive) */
        current_instr->operands.pop_back();
        jmal_instr_add_operand(current_instr,
            jmal_operand_address(inner, yylineno));
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
        $$ = jmal_type_user($1.c_str(), yylineno);
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

%%

namespace yy {
    void parser::error(const std::string& msg) {
        std::cerr << "Error: " << msg << " line " << yylineno << "\n";
    }
}
