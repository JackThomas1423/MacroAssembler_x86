#ifndef JMAL_TOKENS_H
#define JMAL_TOKENS_H

/* ── Semantic value union ──────────────────────────────────────────────────
 *
 * Bison expects this to be typedef'd as YYSTYPE *before* it sees any %token
 * declarations that use <field> syntax, so put it here.
 * In your .y preamble write:
 *
 *   %{
 *   #include "jmal_tokens.h"
 *   %}
 *   %union { ... }   ← OR just use the typedef below with %define api.value.type
 *
 * Simplest approach: paste the union into your .y %union block and keep this
 * header for the shared token codes only.
 */

typedef struct {
    int lo;
    int hi;
} JmalRange;

typedef union {
    char     *sval;    /* heap-allocated string; caller must free() */
    int       ival;
    double    fval;
    JmalRange range;   /* arity range  N-M                           */
} YYSTYPE;

#define YYSTYPE YYSTYPE   /* prevent double-typedef in some compilers */

/* ── Directives ───────────────────────────────────────────────────────── */
#define DIR_DEFINE_TABLE   256
#define DIR_DEFINE         257
#define DIR_UNDEF          258
#define DIR_TYPE           259
#define DIR_USE            260
#define DIR_MACRO_STRICT   261
#define DIR_MACRO          262
#define DIR_ENDMACRO       263
#define DIR_ARG            264
#define DIR_REP            265
#define DIR_ENDREP         266
#define DIR_ROTATE         267
#define DIR_ARG_COUNT      268   /* %0 — total number of args passed */

/* ── Keywords ─────────────────────────────────────────────────────────── */
#define KW_FROM            270   /* "from" in  %define.table X from Y */

/* ── Built-in primitive types ─────────────────────────────────────────── */
#define TYPE_REGISTER      280
#define TYPE_STRING        281
#define TYPE_NUMBER        282
#define TYPE_ADDRESS       283

/* ── Literals & identifiers ───────────────────────────────────────────── */
#define TOK_IDENT          290   /* yylval.sval */
#define TOK_STRING         291   /* yylval.sval  (quotes stripped)  */
#define TOK_INT            292   /* yylval.ival  */
#define TOK_FLOAT          293   /* yylval.fval  */
#define TOK_ARG_REF        294   /* yylval.ival  (1-based index)    */
#define TOK_ARITY_RANGE    295   /* yylval.range.lo / .hi           */

/* ── Punctuation ──────────────────────────────────────────────────────── */
#define TOK_NEWLINE        300
#define TOK_COMMA          301
#define TOK_COLON          302
#define TOK_PIPE           303
#define TOK_LBRACKET       304   /* [ */
#define TOK_RBRACKET       305   /* ] */
#define TOK_LPAREN         306   /* ( */
#define TOK_RPAREN         307   /* ) */
#define TOK_PLUS           308
#define TOK_MINUS          309
#define TOK_STAR           310
#define TOK_SLASH          311

/*
 * Token name table – useful for error messages in your parser.
 * Define JMAL_TOKEN_NAMES_IMPL in exactly one .c file before including
 * this header to get the definition; elsewhere you get the extern.
 */
#ifdef JMAL_TOKEN_NAMES_IMPL
const char *jmal_token_name(int tok) {
    switch (tok) {
        case DIR_DEFINE_TABLE:  return "%define.table";
        case DIR_DEFINE:        return "%define";
        case DIR_UNDEF:         return "%undef";
        case DIR_TYPE:          return "%type";
        case DIR_USE:           return "%use";
        case DIR_MACRO_STRICT:  return "%macro.strict";
        case DIR_MACRO:         return "%macro";
        case DIR_ENDMACRO:      return "%endmacro";
        case DIR_ARG:           return "%arg";
        case DIR_REP:           return "%rep";
        case DIR_ENDREP:        return "%endrep";
        case DIR_ROTATE:        return "%rotate";
        case DIR_ARG_COUNT:     return "%0";
        case KW_FROM:           return "from";
        case TYPE_REGISTER:     return "register";
        case TYPE_STRING:       return "string";
        case TYPE_NUMBER:       return "number";
        case TYPE_ADDRESS:      return "address";
        case TOK_IDENT:         return "identifier";
        case TOK_STRING:        return "string-literal";
        case TOK_INT:           return "integer";
        case TOK_FLOAT:         return "float";
        case TOK_ARG_REF:       return "arg-ref";
        case TOK_ARITY_RANGE:   return "arity-range";
        case TOK_NEWLINE:       return "newline";
        case TOK_COMMA:         return ",";
        case TOK_COLON:         return ":";
        case TOK_PIPE:          return "|";
        case TOK_LBRACKET:      return "[";
        case TOK_RBRACKET:      return "]";
        case TOK_LPAREN:        return "(";
        case TOK_RPAREN:        return ")";
        case TOK_PLUS:          return "+";
        case TOK_MINUS:         return "-";
        case TOK_STAR:          return "*";
        case TOK_SLASH:         return "/";
        default:                return "unknown";
    }
}
#else
extern const char *jmal_token_name(int tok);
#endif /* JMAL_TOKEN_NAMES_IMPL */

#endif /* JMAL_TOKENS_H */
