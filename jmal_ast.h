#ifndef JMAL_AST_H
#define JMAL_AST_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct JmalTypeConstraint      JmalTypeConstraint;
typedef struct JmalTypeConstraintMulti JmalTypeConstraintMulti;
typedef struct JmalTableRule           JmalTableRule;
typedef struct JmalTable               JmalTable;
typedef struct JmalTypeDef             JmalTypeDef;
typedef struct JmalDefine              JmalDefine;
typedef struct JmalArgDecl             JmalArgDecl;
typedef struct JmalRepBlock            JmalRepBlock;
typedef struct JmalOperand             JmalOperand;
typedef struct JmalInstruction         JmalInstruction;
typedef struct JmalMacroBody           JmalMacroBody;
typedef struct JmalMacro               JmalMacro;
typedef struct JmalProgram             JmalProgram;

typedef enum {
    JMAL_TYPE_BUILTIN_REGISTER,
    JMAL_TYPE_BUILTIN_STRING,
    JMAL_TYPE_BUILTIN_NUMBER,
    JMAL_TYPE_BUILTIN_ADDRESS,
    JMAL_TYPE_USER  /* %type my_type: string | number */
} JmalTypeKind;

struct JmalTypeConstraint {
    JmalTypeKind kind;
    char* name;   /* set when kind == JMAL_TYPE_USER, else NULL */
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

struct JmalTableRule {
    char* name;
    JmalTypeConstraintMulti* types;
    int line;
};

struct JmalTable {
    char* name;
    JmalTableRule** rules;
    size_t rule_count;
    int line;
};

static inline JmalTableRule *jmal_table_rule_new(const char *name, JmalTypeConstraintMulti *t, int line)
{
    JmalTableRule *r = malloc(sizeof *r);
    r->name       = strdup(name);
    r->types      = t;
    r->line       = line;
    return r;
}

/*static inline void jmal_table_rule_add_type(JmalTableRule *r, JmalTypeConstraint *t)
{
    r->types = realloc(r->types, (r->type_count + 1) * sizeof *r->types);
    r->types[r->type_count++] = t;
}*/

static inline void jmal_table_rule_free(JmalTableRule *r)
{
    if (!r) return;
    jmal_type_multi_free(r->types);
    free(r->name);
    free(r);
}

static inline JmalTable *jmal_table_new(const char *name, int line)
{
    JmalTable *t   = malloc(sizeof *t);
    t->name        = strdup(name);
    t->rules       = NULL;
    t->rule_count  = 0;
    t->line        = line;
    return t;
}

static inline void jmal_table_add_rule(JmalTable *t, JmalTableRule *r)
{
    t->rules = realloc(t->rules, (t->rule_count + 1) * sizeof *t->rules);
    t->rules[t->rule_count++] = r;
}

static inline void jmal_table_free(JmalTable *t)
{
    if (!t) return;
    for (size_t i = 0; i < t->rule_count; i++)
        jmal_table_rule_free(t->rules[i]);
    free(t->rules);
    free(t->name);
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
    int                 index;         /* the N in %N, 1-based          */
    JmalTypeConstraint **constraints;
    size_t              constraint_count;
    int                 line;
};

static inline JmalArgDecl *jmal_arg_decl_new(int index, int line)
{
    JmalArgDecl *a      = malloc(sizeof *a);
    a->index            = index;
    a->constraints      = NULL;
    a->constraint_count = 0;
    a->line             = line;
    return a;
}

static inline void jmal_arg_decl_add(JmalArgDecl *a, JmalTypeConstraint *t)
{
    a->constraints = realloc(a->constraints,
                             (a->constraint_count + 1) * sizeof *a->constraints);
    a->constraints[a->constraint_count++] = t;
}

static inline void jmal_arg_decl_free(JmalArgDecl *a)
{
    if (!a) return;
    for (size_t i = 0; i < a->constraint_count; i++)
        jmal_type_free(a->constraints[i]);
    free(a->constraints);
    free(a);
}

/*
 * %rep N / %rep %0
 *   push %1
 *   %rotate 1
 * %endrep
 */
struct JmalRepBlock {
    int   use_arg_count;   /* 1 = %rep %0,  0 = %rep <literal N>  */
    int   count;           /* set when use_arg_count == 0          */

    /* body: interleaved instructions and %rotate steps */
    JmalInstruction **instrs;
    size_t            instr_count;

    int  *rotates;         /* rotate amount at each %rotate        */
    int  *rotate_after;    /* rotate_after[i]: after which instr index */
    size_t rotate_count;

    int line;
};

static inline JmalRepBlock *jmal_rep_new(int use_arg_count, int count, int line)
{
    JmalRepBlock *r   = malloc(sizeof *r);
    r->use_arg_count  = use_arg_count;
    r->count          = count;
    r->instrs         = NULL;
    r->instr_count    = 0;
    r->rotates        = NULL;
    r->rotate_after   = NULL;
    r->rotate_count   = 0;
    r->line           = line;
    return r;
}

static inline void jmal_rep_add_instr(JmalRepBlock *r, JmalInstruction *i)
{
    r->instrs = realloc(r->instrs, (r->instr_count + 1) * sizeof *r->instrs);
    r->instrs[r->instr_count++] = i;
}

static inline void jmal_rep_add_rotate(JmalRepBlock *r, int amount)
{
    r->rotates      = realloc(r->rotates,
                              (r->rotate_count + 1) * sizeof *r->rotates);
    r->rotate_after = realloc(r->rotate_after,
                              (r->rotate_count + 1) * sizeof *r->rotate_after);
    r->rotates[r->rotate_count]      = amount;
    r->rotate_after[r->rotate_count] = (int)r->instr_count; /* after current */
    r->rotate_count++;
}

static inline void jmal_rep_free(JmalRepBlock *r)
{
    if (!r) return;
    for (size_t i = 0; i < r->instr_count; i++)
        jmal_instr_free(r->instrs[i]);
    free(r->instrs);
    free(r->rotates);
    free(r->rotate_after);
    free(r);
}

/*
 * A single item inside a macro body — either an arg decl, a rep block,
 * or an instruction.
 */
typedef enum {
    JMAL_BODY_ARG_DECL,
    JMAL_BODY_REP,
    JMAL_BODY_INSTR
} JmalBodyItemKind;

typedef struct {
    JmalBodyItemKind kind;
    union {
        JmalArgDecl     *arg;
        JmalRepBlock    *rep;
        JmalInstruction *instr;
    };
} JmalBodyItem;

static inline void jmal_body_item_free(JmalBodyItem *item)
{
    if (!item) return;
    switch (item->kind) {
        case JMAL_BODY_ARG_DECL: jmal_arg_decl_free(item->arg);   break;
        case JMAL_BODY_REP:      jmal_rep_free(item->rep);         break;
        case JMAL_BODY_INSTR:    jmal_instr_free(item->instr);     break;
    }
    free(item);
}

/* ═══════════════════════════════════════════════════════════════════════
 * %macro / %macro.strict
 *
 * %use <table>          ← optional, recorded here
 * %macro[.strict] <name> <arity>
 *     <body items>
 * %endmacro
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalMacro {
    char   *name;
    int     is_strict;       /* 1 = %macro.strict, 0 = %macro         */

    /* arity: fixed (lo == hi) or range (lo != hi) */
    int     arity_lo;
    int     arity_hi;

    /* optional %use table that was active when this macro was defined */
    char   *use_table;       /* NULL if no %use preceded this macro    */

    JmalBodyItem **body;
    size_t         body_count;

    int line;
};

static inline JmalMacro *jmal_macro_new(const char *name,
                                         int is_strict,
                                         int arity_lo, int arity_hi,
                                         const char *use_table,
                                         int line)
{
    JmalMacro *m  = malloc(sizeof *m);
    m->name       = strdup(name);
    m->is_strict  = is_strict;
    m->arity_lo   = arity_lo;
    m->arity_hi   = arity_hi;
    m->use_table  = use_table ? strdup(use_table) : NULL;
    m->body       = NULL;
    m->body_count = 0;
    m->line       = line;
    return m;
}

static inline void jmal_macro_add_body_item(JmalMacro *m, JmalBodyItem *item)
{
    m->body = realloc(m->body, (m->body_count + 1) * sizeof *m->body);
    m->body[m->body_count++] = item;
}

static inline void jmal_macro_free(JmalMacro *m)
{
    if (!m) return;
    for (size_t i = 0; i < m->body_count; i++)
        jmal_body_item_free(m->body[i]);
    free(m->body);
    free(m->use_table);
    free(m->name);
    free(m);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JmalProgram — the root, owns everything
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalProgram {
    /* --- definitions collected during parsing --- */

    JmalTable   **tables;
    size_t        table_count;

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
    p->tables       = NULL; p->table_count   = 0;
    p->typedefs     = NULL; p->typedef_count = 0;
    p->defines      = NULL; p->define_count  = 0;
    p->macros       = NULL; p->macro_count   = 0;
    p->instrs       = NULL; p->instr_count   = 0;
    p->filename     = filename ? strdup(filename) : strdup("<stdin>");
    return p;
}

/* --- add helpers --- */

static inline void jmal_program_add_table(JmalProgram *p, JmalTable *t)
{ p->tables = realloc(p->tables, (p->table_count+1)*sizeof*p->tables);
  p->tables[p->table_count++] = t; }

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
    for (size_t i = 0; i < p->table_count;   i++) jmal_table_free(p->tables[i]);
    for (size_t i = 0; i < p->typedef_count; i++) jmal_typedef_free(p->typedefs[i]);
    for (size_t i = 0; i < p->define_count;  i++) jmal_define_free(p->defines[i]);
    for (size_t i = 0; i < p->macro_count;   i++) jmal_macro_free(p->macros[i]);
    for (size_t i = 0; i < p->instr_count;   i++) jmal_instr_free(p->instrs[i]);
    free(p->tables);
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
        case JMAL_TYPE_USER:             return "<user>";
    }
    return "?";
}

static inline void jmal_program_dump(const JmalProgram *p)
{
    printf("=== JmalProgram: %s ===\n\n", p->filename);

    printf("-- Tables (%zu) --\n", p->table_count);
    for (size_t i = 0; i < p->table_count; i++) {
        JmalTable *t = p->tables[i];
        printf("  table '%s'\n", t->name);
        for (size_t r = 0; r < t->rule_count; r++) {
            JmalTableRule *rule = t->rules[r];
            printf("    rule '%s': [", rule->name);
            JmalTypeConstraintMulti* type_multi = rule->types;
            for (size_t j = 0; j < type_multi->type_count; j++) {
                JmalTypeConstraint *tc = type_multi->types[j];
                if (tc->kind == JMAL_TYPE_USER)
                    printf("%s", tc->name);
                else
                    printf("%s", jmal_type_kind_str(tc->kind));
                if (j + 1 < type_multi->type_count) printf(", ");
            }
            printf("]\n");
        }
    }

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
        printf("  %smacro '%s' arity %d",
               m->is_strict ? "strict " : "", m->name, m->arity_lo);
        if (m->arity_hi != m->arity_lo) printf("-%d", m->arity_hi);
        if (m->use_table) printf(" (uses table '%s')", m->use_table);
        printf("  [%zu body items]\n", m->body_count);
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