%require "3.2"
%language "c++"

%define api.value.type variant
%define api.token.constructor
%define parse.error verbose

%code requires {
    #include <string>
    #include <iostream>
    #include <src/templates.hpp>
}

%code {
    yy::parser::symbol_type yylex();
    extern int yylineno;

    static JmalProgram *jmal_program = nullptr;

    // Current instruction being built
    static InstructionStatement *current_instr = nullptr;
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

/* Grammar types */
%type <JmalTypeConstraint>  builtin_type type_constraint
%type <JmalTypeList>        type_union use_def_args
%type <ArgStatement>        arg_decl
%type <UseStatement>        use_def
%type <MacroStatement>      macro_def macro_header
%type <AnyStatement>        macro_body_item rotate_def rep_block instruction ensure_def
%type <std::string>         ensure_op ensure_operand
%type <std::vector<AnyStatement>> macro_body

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
        jmal_program->add_stmt(std::move($1));
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
    {
        jmal_program->add_macro(std::move($1));
    }
    ;

/* %define <name> <value> */
define_dir:
    DIR_DEFINE TOK_IDENT TOK_STRING
    {
        jmal_program->add_define(JmalDefine::make_str($2, $3));
    }
    | DIR_DEFINE TOK_IDENT TOK_INT
    {
        jmal_program->add_define(JmalDefine::make_int($2, $3));
    }
    | DIR_DEFINE TOK_IDENT TOK_FLOAT
    {
        jmal_program->add_define(JmalDefine::make_float($2, $3));
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
        jmal_program->add_stmt(TypeStatement {
            .header = { .name = $2, .constraints = std::move($4) }
        });
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Macro definitions
 * ════════════════════════════════════════════════════════════════════════ */

macro_def:
    macro_header newlines macro_body DIR_ENDMACRO
    {
        $1.children = std::move($3);
        $$ = std::move($1);
    }
    ;

macro_header:
    DIR_MACRO TOK_IDENT TOK_INT TOK_INT
    {
        $$ = MacroStatement{
            .header = {
                .name      = $2,
                .arity_in  = JmalArity::fixed($3),
                .arity_out = JmalArity::fixed($4),
            }
        };
    }
    | DIR_MACRO TOK_IDENT TOK_RANGE TOK_INT
    {
        $$ = MacroStatement{
            .header = {
                .name      = $2,
                .arity_in  = $3,
                .arity_out = JmalArity::fixed($4),
            }
        };
    }
    ;

macro_body:
    macro_body_item
    {
        $$ = std::vector<AnyStatement>{};
        if ($1.index() != std::variant_npos)
            $$.push_back(std::move($1));
    }
    | macro_body macro_body_item
    {
        if ($2.index() != std::variant_npos)
            $1.push_back(std::move($2));
        $$ = std::move($1);
    }
    ;

macro_body_item:
    arg_decl newlines          { $$ = std::move($1); }
    | ref_decl newlines        { $$ = std::monostate{}; }
    | rep_block                { $$ = std::move($1); }
    | ensure_def               { $$ = std::move($1); }
    | use_def newlines         { $$ = std::move($1); }
    | if_def                   { $$ = std::monostate{}; }
    | rotate_def               { $$ = std::move($1); }
    | arg_ref_set              { $$ = std::monostate{}; }
    | literal_block newlines   { $$ = std::monostate{}; }
    | instruction newlines     { $$ = std::move($1); }
    | newlines                 { $$ = std::monostate{}; }
    ;

/*
 * %ensure <lhs> <op> <rhs>
 */
ensure_def:
    DIR_ENSURE ensure_operand ensure_op ensure_operand newlines
    {
        $$ = EnsureStatement{
            .header = { .lhs = $2, .op = $3, .rhs = $4 }
        };
    }
    ;

ensure_op:
    TOK_COMPARE          { $$ = "=="; }
    | TOK_COMPARE_NOT    { $$ = "!="; }
    | TOK_COMPARE_GREATER { $$ = ">="; }
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
 * %rotate <N>
 * %rotate %N
 */
rotate_def:
    DIR_ROTATE TOK_INT newlines
    {
        $$ = RotateStatement{
            .header = { .amount = std::to_string($2) }
        };
    }
    | DIR_ROTATE TOK_ARG_REF newlines
    {
        $$ = RotateStatement{
            .header = { .amount = "%" + std::to_string($2) }
        };
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
        $$ = ArgStatement{
            .header = { .param = $2, .type_constraints = { $4 } }
        };
    }
    | DIR_ARG TOK_ARG_REF TOK_COLON type_union
    {
        $$ = ArgStatement{
            .header = { .param = $2, .type_constraints = std::move($4) }
        };
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
        RepStatement rep;
        rep.children = std::move($4);
        $$ = std::move(rep);
    }
    | DIR_REP if_cond newlines macro_body DIR_ENDREP newlines
    {
        RepStatement rep;
        rep.children = std::move($4);
        $$ = std::move(rep);
    }
    ;

rep_count:
    TOK_INT         { /* fixed count */ }
    | DIR_ARG_COUNT { /* %0 */ }
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
        $$ = UseStatement{
            .header = { .name = $2, .arguments = std::move($3) }
        };
    }
    | DIR_USE TOK_IDENT
    {
        $$ = UseStatement{
            .header = { .name = $2, .arguments = {} }
        };
    }
    ;

use_def_args:
    type_constraint
    {
        $$ = JmalTypeList{ $1 };
    }
    | use_def_args type_constraint
    {
        $1.push_back($2);
        $$ = std::move($1);
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Instructions
 * ════════════════════════════════════════════════════════════════════════ */

instruction:
    TOK_IDENT
    {
        current_instr = new InstructionStatement{
            .header = { .opcode = $1, .operands = {} }
        };
    }
    operand_list
    {
        $$ = std::move(*current_instr);
        delete current_instr;
        current_instr = nullptr;
    }
    ;

operand_list:
    %empty {}
    | operand
    | operand_list TOK_COMMA operand
    ;

operand:
    TOK_IDENT
    {
        current_instr->header.operands.push_back(JmalOperand::ident($1));
    }
    | TOK_INT
    {
        current_instr->header.operands.push_back(JmalOperand::integer($1));
    }
    | TOK_FLOAT
    {
        current_instr->header.operands.push_back(JmalOperand::fp($1));
    }
    | TOK_STRING
    {
        current_instr->header.operands.push_back(JmalOperand::string($1));
    }
    | TOK_ARG_REF
    {
        current_instr->header.operands.push_back(JmalOperand::arg_ref(static_cast<int>($1)));
    }
    | TOK_LBRACKET operand TOK_RBRACKET
    {
        JmalOperand inner = std::move(current_instr->header.operands.back());
        current_instr->header.operands.pop_back();
        current_instr->header.operands.push_back(JmalOperand::address(std::move(inner)));
    }
    ;

/* ════════════════════════════════════════════════════════════════════════
 * Type helpers
 * ════════════════════════════════════════════════════════════════════════ */

type_union:
    type_constraint
    {
        $$ = JmalTypeList{ $1 };
    }
    | type_union TOK_PIPE type_constraint
    {
        $1.push_back($3);
        $$ = std::move($1);
    }
    ;

type_constraint:
    builtin_type    { $$ = $1; }
    | TOK_IDENT     { $$ = JmalTypeConstraint::user($1); }
    | TOK_ARG_REF   { $$ = JmalTypeConstraint::arg_ref($1); }
    | TOK_INT       { $$ = JmalTypeConstraint::lit_int($1); }
    ;

builtin_type:
    TYPE_REGISTER  { $$ = JmalTypeConstraint::builtin(JMAL_TYPE_BUILTIN_REGISTER); }
    | TYPE_STRING  { $$ = JmalTypeConstraint::builtin(JMAL_TYPE_BUILTIN_STRING);   }
    | TYPE_NUMBER  { $$ = JmalTypeConstraint::builtin(JMAL_TYPE_BUILTIN_NUMBER);   }
    | TYPE_ADDRESS { $$ = JmalTypeConstraint::builtin(JMAL_TYPE_BUILTIN_ADDRESS);  }
    ;

%%

namespace yy {
    void parser::error(const std::string& msg) {
        std::cerr << "Error: " << msg << " line " << yylineno << "\n";
    }
}