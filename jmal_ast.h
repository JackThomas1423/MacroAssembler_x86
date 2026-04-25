#ifndef JMAL_AST_H
#define JMAL_AST_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct JmalTypeConstraint      JmalTypeConstraint;
typedef struct JmalTypeConstraintMulti JmalTypeConstraintMulti;
typedef struct JmalTypeDef             JmalTypeDef;
typedef struct JmalDefine              JmalDefine;
typedef struct JmalArgDecl             JmalArgDecl;
typedef struct JmalRepBlock            JmalRepBlock;
typedef struct JmalRotate              JmalRotate;
typedef struct JmalUse                 JmalUse;
typedef struct JmalOperand             JmalOperand;
typedef struct JmalInstruction         JmalInstruction;
typedef struct JmalStatement           JmalStatement;
typedef struct JmalStatementMulti      JmalStatementMulti;
typedef struct JmalMacro               JmalMacro;
typedef struct JmalProgram             JmalProgram;

typedef unsigned int                   JmalArgRef;

typedef enum {
    JMAL_TYPE_BUILTIN_REGISTER,
    JMAL_TYPE_BUILTIN_STRING,
    JMAL_TYPE_BUILTIN_NUMBER,
    JMAL_TYPE_BUILTIN_ADDRESS,
    JMAL_TYPE_LIT_INT,
    JMAL_TYPE_ARG_REF,
    JMAL_TYPE_USER             /* %type my_type: string | number */
} JmalTypeKind;

struct JmalTypeConstraint {
    JmalTypeKind kind;
    union {
        char* name;   /* set when kind == JMAL_TYPE_USER, else NULL */
        int value;
    };
    int line;
};

struct JmalTypeConstraintMulti {
    JmalTypeConstraint** types;
    size_t type_count;
};

static inline JmalTypeConstraintMulti *jmal_type_make_multi(JmalTypeConstraint *a)
{
    JmalTypeConstraintMulti *t = malloc(sizeof *t);
    t->type_count = 0;
    t->types = NULL;
    t->types = realloc(t->types, (t->type_count + 1) * sizeof *t->types);
    t->types[t->type_count++] = a;
    return t;
}

static inline void jmal_type_multi_add_type(JmalTypeConstraintMulti *r, JmalTypeConstraint *n)
{
    r->types = realloc(r->types, (r->type_count + 1) * sizeof *r->types);
    r->types[r->type_count++] = n;
}

static inline JmalTypeConstraint *jmal_type_builtin(JmalTypeKind k, int line)
{
    JmalTypeConstraint *t = malloc(sizeof *t);
    t->kind = k;
    t->name = NULL;
    t->line = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_arg_ref(JmalArgRef arg_ref, int line)
{
    JmalTypeConstraint *t = malloc(sizeof *t);
    t->kind = JMAL_TYPE_ARG_REF;
    t->value = arg_ref;
    t->line = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_lit_int(int v, int line)
{
    JmalTypeConstraint *t = malloc(sizeof *t);
    t->kind = JMAL_TYPE_LIT_INT;
    t->value = v;
    t->line = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_user(const char *name, int line)
{
    JmalTypeConstraint *t = malloc(sizeof *t);
    t->kind = JMAL_TYPE_USER;
    t->name = strdup(name);
    t->line = line;
    return t;
}

static inline void jmal_type_free(JmalTypeConstraint *t)
{
    if (!t) return;
    if (t->kind == JMAL_TYPE_USER)
        free(t->name);
    free(t);
}

static inline void jmal_type_multi_free(JmalTypeConstraintMulti *t)
{
    if (!t) return;
    for (size_t i = 0; i < t->type_count; i++)
        jmal_type_free(t->types[i]);
    free(t->types);
    free(t);
}

struct JmalTypeDef {
    char* name;
    JmalTypeConstraintMulti* members;
    int line;
};

static inline JmalTypeDef *jmal_typedef_new(const char *name, JmalTypeConstraintMulti *tc, int line)
{
    JmalTypeDef *d  = malloc(sizeof *d);
    d->name         = strdup(name);
    d->members      = tc;
    d->line         = line;
    return d;
}

static inline void jmal_typedef_free(JmalTypeDef *d)
{
    if (!d) return;
    jmal_type_multi_free(d->members);
    free(d->name);
    free(d);
}

typedef enum {
    JMAL_DEFINE_STRING,
    JMAL_DEFINE_INT,
    JMAL_DEFINE_FLOAT
} JmalDefineKind;

struct JmalDefine {
    char* name;
    JmalDefineKind kind;
    char*  str_val;   /*JMAL_DEFINE_STRING*/
    int    int_val;   /*JMAL_DEFINE_INT*/
    double flt_val;   /*JMAL_DEFINE_FLOAT*/
    int    line;
};

static inline JmalDefine *jmal_define_str(const char *name, const char *val, int line)
{
    JmalDefine *d = malloc(sizeof *d);
    d->name    = strdup(name);
    d->kind    = JMAL_DEFINE_STRING;
    d->str_val = strdup(val);
    d->int_val = 0;
    d->flt_val = 0.0;
    d->line    = line;
    return d;
}

static inline JmalDefine *jmal_define_int(const char *name, int val, int line)
{
    JmalDefine *d = malloc(sizeof *d);
    d->name    = strdup(name);
    d->kind    = JMAL_DEFINE_INT;
    d->str_val = NULL;
    d->int_val = val;
    d->flt_val = 0.0;
    d->line    = line;
    return d;
}

static inline JmalDefine *jmal_define_float(const char *name, double val, int line)
{
    JmalDefine *d = malloc(sizeof *d);
    d->name    = strdup(name);
    d->kind    = JMAL_DEFINE_FLOAT;
    d->str_val = NULL;
    d->int_val = 0;
    d->flt_val = val;
    d->line    = line;
    return d;
}

static inline void jmal_define_free(JmalDefine *d)
{
    if (!d) return;
    free(d->str_val);
    free(d->name);
    free(d);
}

struct JmalUse {
    char *name;
    JmalTypeConstraintMulti *args;
    int line;
};

static inline JmalUse *jmal_use_new(const char *name, JmalTypeConstraintMulti *tm, int line)
{
    JmalUse *u  = malloc(sizeof *u);
    u->name     = strdup(name);
    u->args     = tm;
    u->line     = line;
    return u;
}

static inline void jmal_use_free(JmalUse *u)
{
    if (!u) return;
    jmal_type_multi_free(u->args);
    free(u->name);
    free(u);
}

typedef enum {
    JMAL_ROTATE_INT,   /* %rotate <literal N>  */
    JMAL_ROTATE_ARG    /* %rotate <arg_name>   */
} JmalRotateType;

struct JmalRotate {
    JmalRotateType kind;
    union {
        int        count;         /* JMAL_ROTATE_INT */
        JmalArgRef argRef;        /* JMAL_ROTATE_ARG */
    };
};

static inline JmalRotate *jmal_rotate_int(int count)
{
    JmalRotate *r  = malloc(sizeof *r);
    r->kind        = JMAL_ROTATE_INT;
    r->count       = count;
    return r;
}

static inline JmalRotate *jmal_rotate_arg(unsigned int arg_ref)
{
    JmalRotate *r  = malloc(sizeof *r);
    r->kind        = JMAL_ROTATE_ARG;
    r->argRef      = arg_ref;
    return r;
}

static inline void jmal_rotate_free(JmalRotate *r)
{
    if (!r) return;
    free(r);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Operands  (used in instructions and macro body instructions)
 *
 * Examples:
 *   rax          → JMAL_OPERAND_IDENT
 *   42           → JMAL_OPERAND_INT
 *   3.14         → JMAL_OPERAND_FLOAT
 *   "hello"      → JMAL_OPERAND_STRING
 *   %1           → JMAL_OPERAND_ARG_REF
 *   [b]          → JMAL_OPERAND_ADDRESS  (wraps another operand)
 * ═══════════════════════════════════════════════════════════════════════ */

typedef enum {
    JMAL_OPERAND_IDENT,
    JMAL_OPERAND_INT,
    JMAL_OPERAND_FLOAT,
    JMAL_OPERAND_STRING,
    JMAL_OPERAND_ARG_REF,    /* %N inside a macro body */
    JMAL_OPERAND_ADDRESS     /* [operand] */
} JmalOperandKind;

struct JmalOperand {
    JmalOperandKind kind;
    char* sval;           /* IDENT / STRING */
    int ival;             /* INT / ARG_REF  */
    double fval;          /* FLOAT          */
    JmalOperand* inner;   /* ADDRESS → the operand inside [ ] */
    int line;
};

static inline JmalOperand *jmal_operand_ident(const char *s, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_IDENT;
    o->sval  = strdup(s);
    o->ival  = 0; o->fval = 0.0; o->inner = NULL; o->line = line;
    return o;
}
static inline JmalOperand *jmal_operand_int(int v, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_INT;
    o->ival  = v;
    o->sval  = NULL; o->fval = 0.0; o->inner = NULL; o->line = line;
    return o;
}
static inline JmalOperand *jmal_operand_float(double v, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_FLOAT;
    o->fval  = v;
    o->sval  = NULL; o->ival = 0; o->inner = NULL; o->line = line;
    return o;
}
static inline JmalOperand *jmal_operand_string(const char *s, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_STRING;
    o->sval  = strdup(s);
    o->ival  = 0; o->fval = 0.0; o->inner = NULL; o->line = line;
    return o;
}
static inline JmalOperand *jmal_operand_arg_ref(int index, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_ARG_REF;
    o->ival  = index;
    o->sval  = NULL; o->fval = 0.0; o->inner = NULL; o->line = line;
    return o;
}
static inline JmalOperand *jmal_operand_address(JmalOperand *inner, int line)
{
    JmalOperand *o = malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_ADDRESS;
    o->inner = inner;
    o->sval  = NULL; o->ival = 0; o->fval = 0.0; o->line = line;
    return o;
}

static inline void jmal_operand_free(JmalOperand *o)
{
    if (!o) return;
    jmal_operand_free(o->inner);
    free(o->sval);
    free(o);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Instructions  (top-level and inside macro bodies)
 *
 * Examples:
 *   mov a, [b]          → opcode="mov",  operands=[ident,address]
 *   push rax            → opcode="push", operands=[ident]
 *   push 1              → opcode="push", operands=[int]
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalInstruction {
    char          *opcode;
    JmalOperand  **operands;
    size_t         operand_count;
    int            line;
};

static inline JmalInstruction *jmal_instr_new(const char *opcode, int line)
{
    JmalInstruction *i  = malloc(sizeof *i);
    i->opcode           = strdup(opcode);
    i->operands         = NULL;
    i->operand_count    = 0;
    i->line             = line;
    return i;
}

static inline void jmal_instr_add_operand(JmalInstruction *i, JmalOperand *o)
{
    i->operands = realloc(i->operands,
                          (i->operand_count + 1) * sizeof *i->operands);
    i->operands[i->operand_count++] = o;
}

static inline void jmal_instr_free(JmalInstruction *i)
{
    if (!i) return;
    for (size_t j = 0; j < i->operand_count; j++)
        jmal_operand_free(i->operands[j]);
    free(i->operands);
    free(i->opcode);
    free(i);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Macro body items
 *
 * A macro body is a sequence of:
 *   - %arg declarations
 *   - %rep … %endrep blocks
 *   - plain instructions (using %N arg refs as operands)
 * ═══════════════════════════════════════════════════════════════════════ */

/*
 * %arg %N : type, type, ...
 */
struct JmalArgDecl {
    JmalArgRef argRef;            /* the N in %N, 1-based          */
    JmalTypeConstraintMulti *constraints;
    int line;
};

static inline JmalArgDecl *jmal_arg_decl_new(JmalArgRef arg_ref, JmalTypeConstraintMulti *tc, int line)
{
    JmalArgDecl *a      = malloc(sizeof *a);
    a->argRef           = arg_ref;
    a->constraints      = tc;
    a->line             = line;
    return a;
}

static inline void jmal_arg_decl_free(JmalArgDecl *a)
{
    if (!a) return;
    jmal_type_multi_free(a->constraints);
    free(a);
}

struct JmalRepBlock {
    JmalStatementMulti *statements;
    int line;
};

typedef enum {
    JMAL_STATEMENT_ARG_DECL,
    JMAL_STATEMENT_TYPEDEF,
    JMAL_STATEMENT_DEFINE,
    JMAL_STATEMENT_ROTATE,
    JMAL_STATEMENT_INSTR,
    JMAL_STATEMENT_REP,
    JMAL_STATEMENT_USE
} JmalStatementType;

struct JmalStatement {
    JmalStatementType kind;

    union {
        JmalArgDecl     *arg;
        JmalTypeDef     *type;
        JmalDefine      *def;
        JmalRotate      *rotate;
        JmalInstruction *instr;
        JmalRepBlock    *rep;
        JmalUse         *use;
    };
};

/* ── JmalStatement constructors ──────────────────────────────────────── */

static inline JmalStatement *jmal_stmt_arg(JmalArgDecl *a)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_ARG_DECL;
    s->arg  = a;
    return s;
}

static inline JmalStatement *jmal_stmt_typedef(JmalTypeDef *t)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_TYPEDEF;
    s->type = t;
    return s;
}

static inline JmalStatement *jmal_stmt_define(JmalDefine *d)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_DEFINE;
    s->def  = d;
    return s;
}

static inline JmalStatement *jmal_stmt_rotate(JmalRotate *r)
{
    JmalStatement *s  = malloc(sizeof *s);
    s->kind   = JMAL_STATEMENT_ROTATE;
    s->rotate = r;
    return s;
}

static inline JmalStatement *jmal_stmt_instr(JmalInstruction *i)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind  = JMAL_STATEMENT_INSTR;
    s->instr = i;
    return s;
}

static inline JmalStatement *jmal_stmt_rep(JmalRepBlock *r)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_REP;
    s->rep  = r;
    return s;
}

static inline JmalStatement *jmal_stmt_use(JmalUse *u)
{
    JmalStatement *s = malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_USE;
    s->use  = u;
    return s;
}

/* Forward declaration needed because jmal_stmt_free and jmal_rep_free
 * are mutually recursive (rep owns statements, statements may own reps). */
static inline void jmal_stmt_free(JmalStatement *s);

/* ── JmalStatementMulti ───────────────────────────────────────────────── */

struct JmalStatementMulti {
    JmalStatement **stmts;
    size_t          stmt_count;
};

static inline JmalStatementMulti *jmal_stmt_make_multi(JmalStatement *s)
{
    JmalStatementMulti *m = malloc(sizeof *m);
    m->stmt_count = 0;
    m->stmts      = NULL;
    m->stmts      = realloc(m->stmts, (m->stmt_count + 1) * sizeof *m->stmts);
    m->stmts[m->stmt_count++] = s;
    return m;
}

static inline void jmal_stmt_multi_add(JmalStatementMulti *m, JmalStatement *s)
{
    m->stmts = realloc(m->stmts, (m->stmt_count + 1) * sizeof *m->stmts);
    m->stmts[m->stmt_count++] = s;
}

static inline void jmal_stmt_multi_free(JmalStatementMulti *m)
{
    if (!m) return;
    for (size_t i = 0; i < m->stmt_count; i++)
        jmal_stmt_free(m->stmts[i]);
    free(m->stmts);
    free(m);
}

/* ── JmalRepBlock ─────────────────────────────────────────────────────── */

static inline JmalRepBlock *jmal_rep_new(int line)
{
    JmalRepBlock *r    = malloc(sizeof *r);
    r->statements      = NULL;
    r->line            = line;
    return r;
}

static inline void jmal_rep_add_statement_multi(JmalRepBlock *r, JmalStatementMulti *sm)
{
    r->statements = sm;
}

static inline void jmal_rep_free(JmalRepBlock *r)
{
    if (!r) return;
    jmal_stmt_multi_free(r->statements);
    free(r);
}

/* ── JmalStatement free ───────────────────────────────────────────────── */

static inline void jmal_stmt_free(JmalStatement *s)
{
    if (!s) return;
    switch (s->kind) {
        case JMAL_STATEMENT_ARG_DECL: jmal_arg_decl_free(s->arg);    break;
        case JMAL_STATEMENT_TYPEDEF:  jmal_typedef_free(s->type);     break;
        case JMAL_STATEMENT_DEFINE:   jmal_define_free(s->def);       break;
        case JMAL_STATEMENT_ROTATE:   jmal_rotate_free(s->rotate);    break;
        case JMAL_STATEMENT_INSTR:    jmal_instr_free(s->instr);      break;
        case JMAL_STATEMENT_REP:      jmal_rep_free(s->rep);          break;
        case JMAL_STATEMENT_USE:      jmal_use_free(s->use);          break;
    }
    free(s);
}

/* ── JmalMacro ────────────────────────────────────────────────────────── */

struct JmalMacro {
    char   *name;

    /* arity: fixed (lo == hi) or range (lo != hi) */
    int     arity_lo;
    int     arity_hi;

    JmalStatementMulti *statements;

    int line;
};

static inline JmalMacro *jmal_macro_new(const char *name, int arity_lo, int arity_hi, int line)
{
    JmalMacro *m   = malloc(sizeof *m);
    m->name        = strdup(name);
    m->arity_lo    = arity_lo;
    m->arity_hi    = arity_hi;
    m->statements  = NULL;
    m->line        = line;
    return m;
}

static inline void jmal_macro_add_statement(JmalMacro *m, JmalStatement *s)
{
    if (!m->statements)
        m->statements = jmal_stmt_make_multi(s);
    else
        jmal_stmt_multi_add(m->statements, s);
}

static inline void jmal_macro_add_statement_multi(JmalMacro *m, JmalStatementMulti *sm)
{
    if(!m->statements)
        m->statements = sm;
    else
        printf("Warning: code for merging multi statements not implemented, skipping this action\n");
}

static inline void jmal_macro_free(JmalMacro *m)
{
    if (!m) return;
    jmal_stmt_multi_free(m->statements);
    free(m->name);
    free(m);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JmalProgram — the root, owns everything
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalProgram {
    JmalTypeDef **typedefs;
    size_t        typedef_count;

    JmalDefine  **defines;
    size_t        define_count;

    JmalMacro   **macros;
    size_t        macro_count;

    /* --- top-level instruction stream --- */
    JmalInstruction **instrs;
    size_t            instr_count;

    /* --- source file name (for error reporting) --- */
    char *filename;
};

static inline JmalProgram *jmal_program_new(const char *filename)
{
    JmalProgram *p  = malloc(sizeof *p);
    p->typedefs     = NULL; p->typedef_count = 0;
    p->defines      = NULL; p->define_count  = 0;
    p->macros       = NULL; p->macro_count   = 0;
    p->instrs       = NULL; p->instr_count   = 0;
    p->filename     = filename ? strdup(filename) : strdup("<stdin>");
    return p;
}

static inline void jmal_program_add_typedef(JmalProgram *p, JmalTypeDef *t)
{ p->typedefs = realloc(p->typedefs, (p->typedef_count+1)*sizeof*p->typedefs);
  p->typedefs[p->typedef_count++] = t; }

static inline void jmal_program_add_define(JmalProgram *p, JmalDefine *d)
{ p->defines = realloc(p->defines, (p->define_count+1)*sizeof*p->defines);
  p->defines[p->define_count++] = d; }

static inline void jmal_program_add_macro(JmalProgram *p, JmalMacro *m)
{ p->macros = realloc(p->macros, (p->macro_count+1)*sizeof*p->macros);
  p->macros[p->macro_count++] = m; }

static inline void jmal_program_add_instr(JmalProgram *p, JmalInstruction *i)
{ p->instrs = realloc(p->instrs, (p->instr_count+1)*sizeof*p->instrs);
  p->instrs[p->instr_count++] = i; }

/* --- teardown --- */

static inline void jmal_program_free(JmalProgram *p)
{
    if (!p) return;
    for (size_t i = 0; i < p->typedef_count; i++) jmal_typedef_free(p->typedefs[i]);
    for (size_t i = 0; i < p->define_count;  i++) jmal_define_free(p->defines[i]);
    for (size_t i = 0; i < p->macro_count;   i++) jmal_macro_free(p->macros[i]);
    for (size_t i = 0; i < p->instr_count;   i++) jmal_instr_free(p->instrs[i]);
    free(p->typedefs);
    free(p->defines);
    free(p->macros);
    free(p->instrs);
    free(p->filename);
    free(p);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Debug dump  (prints a human-readable summary to stdout)
 * ═══════════════════════════════════════════════════════════════════════ */

#include <stdio.h>

static inline const char *jmal_type_kind_str(JmalTypeKind k)
{
    switch (k) {
        case JMAL_TYPE_BUILTIN_REGISTER: return "register";
        case JMAL_TYPE_BUILTIN_STRING:   return "string";
        case JMAL_TYPE_BUILTIN_NUMBER:   return "number";
        case JMAL_TYPE_BUILTIN_ADDRESS:  return "address";
        case JMAL_TYPE_LIT_INT:          return "<lit int>";
        case JMAL_TYPE_ARG_REF:          return "<%%arg>";
        case JMAL_TYPE_USER:             return "<user>";
    }
    return "?";
}

static inline void jmal_program_dump(const JmalProgram *p)
{
    printf("=== JmalProgram: %s ===\n", p->filename);

    printf("\n-- TypeDefs (%zu) --\n", p->typedef_count);
    for (size_t i = 0; i < p->typedef_count; i++) {
        JmalTypeDef *d = p->typedefs[i];
        printf("  type '%s': ", d->name);
        JmalTypeConstraintMulti* type_multi = d->members;
        for (size_t j = 0; j < type_multi->type_count; ++j) {
            JmalTypeConstraint *tc = type_multi->types[j];
            if (tc->kind == JMAL_TYPE_USER)
                printf("%s", tc->name);
            else
                printf("%s", jmal_type_kind_str(tc->kind));
            if (j + 1 < type_multi->type_count) printf(" | ");
        }
        printf("\n");
    }

    printf("\n-- Defines (%zu) --\n", p->define_count);
    for (size_t i = 0; i < p->define_count; i++) {
        JmalDefine *d = p->defines[i];
        switch (d->kind) {
            case JMAL_DEFINE_STRING: printf("  define '%s' = \"%s\"\n", d->name, d->str_val); break;
            case JMAL_DEFINE_INT:    printf("  define '%s' = %d\n",     d->name, d->int_val); break;
            case JMAL_DEFINE_FLOAT:  printf("  define '%s' = %f\n",     d->name, d->flt_val); break;
        }
    }

    printf("\n-- Macros (%zu) --\n", p->macro_count);
    for (size_t i = 0; i < p->macro_count; i++) {
        JmalMacro *m = p->macros[i];
        printf("  macro '%s' arity %d", m->name, m->arity_lo);
        if (m->arity_hi != m->arity_lo) printf("-%d", m->arity_hi);
        printf("  [%zu statements]\n", m->statements ? m->statements->stmt_count : 0);
    }

    printf("\n-- Top-level instructions (%zu) --\n", p->instr_count);
    for (size_t i = 0; i < p->instr_count; i++) {
        JmalInstruction *instr = p->instrs[i];
        printf("  %s", instr->opcode);
        for (size_t j = 0; j < instr->operand_count; j++) {
            printf(j == 0 ? "  " : ", ");
            JmalOperand *o = instr->operands[j];
            switch (o->kind) {
                case JMAL_OPERAND_IDENT:   printf("%s",    o->sval); break;
                case JMAL_OPERAND_INT:     printf("%d",    o->ival); break;
                case JMAL_OPERAND_FLOAT:   printf("%f",    o->fval); break;
                case JMAL_OPERAND_STRING:  printf("\"%s\"",o->sval); break;
                case JMAL_OPERAND_ARG_REF: printf("%%%d",  o->ival); break;
                case JMAL_OPERAND_ADDRESS: printf("[...]");          break;
            }
        }
        printf("\n");
    }
    printf("\n");
}

#endif /* JMAL_AST_H */