#ifndef JMAL_AST_HPP
#define JMAL_AST_HPP

/* strdup is POSIX, not C11 — expose it before any system header is pulled in */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <assert.h>
#define JMAL_TODO(msg) (fprintf(stderr, "TODO: %s  (%s:%d)\n", (msg), __FILE__, __LINE__), assert(0))

typedef void (*JmalVecFreeFn)(void *);

typedef struct {
    void  **items;
    size_t  count;
} JmalVec;

static inline JmalVec jmal_vec_init(void)
{
    JmalVec v = { NULL, 0 };
    return v;
}

static inline void jmal_vec_push(JmalVec *v, void *item)
{
    v->items = (void**)realloc(v->items, (v->count + 1) * sizeof *v->items);
    v->items[v->count++] = item;
}

static inline void jmal_vec_free(JmalVec *v, JmalVecFreeFn fn)
{
    if (fn)
        for (size_t i = 0; i < v->count; i++)
            fn(v->items[i]);
    free(v->items);
    v->items = NULL;
    v->count = 0;
}

typedef struct JmalArity {
    int lo;
    int hi;
} JmalArity;

static inline JmalArity jmal_arity_fixed(int n)       { JmalArity a = { n, n };    return a; }
static inline JmalArity jmal_arity_range(int lo, int hi) { JmalArity a = { lo, hi }; return a; }
static inline int       jmal_arity_is_range(JmalArity a) { return a.lo != a.hi; }

typedef struct JmalTypeConstraint      JmalTypeConstraint;
typedef struct JmalTypeConstraintMulti JmalTypeConstraintMulti;
typedef struct JmalTypeDef             JmalTypeDef;
typedef struct JmalDefine              JmalDefine;
typedef struct JmalArgDecl             JmalArgDecl;
typedef struct JmalUse                 JmalUse;
typedef struct JmalOperand             JmalOperand;
typedef struct JmalInstruction         JmalInstruction;
typedef struct JmalStatement           JmalStatement;
typedef struct JmalStatementMulti      JmalStatementMulti;
typedef struct JmalMacro               JmalMacro;
typedef struct JmalProgram             JmalProgram;

typedef unsigned int                   JmalArgRef;

/* ═══════════════════════════════════════════════════════════════════════
 * Type constraints
 *
 * JmalTypeConstraint carries one of:
 *   BUILTIN_*            no extra data
 *   JMAL_TYPE_LIT_INT    .value (int)
 *   JMAL_TYPE_ARG_REF    .value (int, the %N index)
 *   JMAL_TYPE_USER       .name  (heap-allocated string)
 * ═══════════════════════════════════════════════════════════════════════ */

typedef enum {
    JMAL_TYPE_BUILTIN_REGISTER,
    JMAL_TYPE_BUILTIN_STRING,
    JMAL_TYPE_BUILTIN_NUMBER,
    JMAL_TYPE_BUILTIN_ADDRESS,
    JMAL_TYPE_LIT_INT,
    JMAL_TYPE_ARG_REF,
    JMAL_TYPE_USER         /* %type my_type: string | number */
} JmalTypeKind;

struct JmalTypeConstraint {
    JmalTypeKind kind;
    union {
        char *name;   /* JMAL_TYPE_USER */
        int   value;  /* JMAL_TYPE_LIT_INT, JMAL_TYPE_ARG_REF */
    };
    int line;
};

struct JmalTypeConstraintMulti {
    JmalVec types;   /* items are JmalTypeConstraint* */
};

static inline JmalTypeConstraintMulti *jmal_type_make_multi(JmalTypeConstraint *a)
{
    JmalTypeConstraintMulti *t = (JmalTypeConstraintMulti*)malloc(sizeof *t);
    t->types = jmal_vec_init();
    jmal_vec_push(&t->types, a);
    return t;
}

static inline void jmal_type_multi_add_type(JmalTypeConstraintMulti *r, JmalTypeConstraint *n)
{
    jmal_vec_push(&r->types, n);
}

/* Convenience accessors so existing call sites stay readable */
#define jmal_type_multi_count(tm)   ((tm)->types.count)
#define jmal_type_multi_get(tm, i)  ((JmalTypeConstraint*)(tm)->types.items[i])

static inline JmalTypeConstraint *jmal_type_builtin(JmalTypeKind k, int line)
{
    JmalTypeConstraint *t = (JmalTypeConstraint*)malloc(sizeof *t);
    t->kind  = k;
    t->name  = NULL;
    t->line  = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_arg_ref(JmalArgRef arg_ref, int line)
{
    JmalTypeConstraint *t = (JmalTypeConstraint*)malloc(sizeof *t);
    t->kind  = JMAL_TYPE_ARG_REF;
    t->value = (int)arg_ref;
    t->line  = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_lit_int(int v, int line)
{
    JmalTypeConstraint *t = (JmalTypeConstraint*)malloc(sizeof *t);
    t->kind  = JMAL_TYPE_LIT_INT;
    t->value = v;
    t->line  = line;
    return t;
}

static inline JmalTypeConstraint *jmal_type_user(const char *name, int line)
{
    JmalTypeConstraint *t = (JmalTypeConstraint*)malloc(sizeof *t);
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
    jmal_vec_free(&t->types, (JmalVecFreeFn)jmal_type_free);
    free(t);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JmalTypeDef   (%type name: member | member | ...)
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalTypeDef {
    char                   *name;
    JmalTypeConstraintMulti *members;
    int                     line;
};

static inline JmalTypeDef *jmal_typedef_new(const char *name, JmalTypeConstraintMulti *tc, int line)
{
    JmalTypeDef *d = (JmalTypeDef*)malloc(sizeof *d);
    d->name        = strdup(name);
    d->members     = tc;
    d->line        = line;
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
    char          *name;
    JmalDefineKind kind;
    union {
        char   *str_val;   /* JMAL_DEFINE_STRING */
        int     int_val;   /* JMAL_DEFINE_INT    */
        double  flt_val;   /* JMAL_DEFINE_FLOAT  */
    };
    int line;
};

static inline JmalDefine *jmal_define_str(const char *name, const char *val, int line)
{
    JmalDefine *d  = (JmalDefine*)malloc(sizeof *d);
    d->name        = strdup(name);
    d->kind        = JMAL_DEFINE_STRING;
    d->str_val     = strdup(val);
    d->line        = line;
    return d;
}

static inline JmalDefine *jmal_define_int(const char *name, int val, int line)
{
    JmalDefine *d  = (JmalDefine*)malloc(sizeof *d);
    d->name        = strdup(name);
    d->kind        = JMAL_DEFINE_INT;
    d->int_val     = val;
    d->line        = line;
    return d;
}

static inline JmalDefine *jmal_define_float(const char *name, double val, int line)
{
    JmalDefine *d  = (JmalDefine*)malloc(sizeof *d);
    d->name        = strdup(name);
    d->kind        = JMAL_DEFINE_FLOAT;
    d->flt_val     = val;
    d->line        = line;
    return d;
}

static inline void jmal_define_free(JmalDefine *d)
{
    if (!d) return;
    if (d->kind == JMAL_DEFINE_STRING) free(d->str_val);
    free(d->name);
    free(d);
}

struct JmalUse {
    char                   *name;
    JmalTypeConstraintMulti *args;   /* NULL when no args supplied */
    int                     line;
};

static inline JmalUse *jmal_use_new(const char *name, JmalTypeConstraintMulti *tm, int line)
{
    JmalUse *u = (JmalUse*)malloc(sizeof *u);
    u->name    = strdup(name);
    u->args    = tm;
    u->line    = line;
    return u;
}

static inline void jmal_use_free(JmalUse *u)
{
    if (!u) return;
    jmal_type_multi_free(u->args);
    free(u->name);
    free(u);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Operands  (used in instructions, inside and outside macro bodies)
 *
 * Examples:
 *   rax        JMAL_OPERAND_IDENT
 *   42         JMAL_OPERAND_INT
 *   3.14       JMAL_OPERAND_FLOAT
 *   "hello"    JMAL_OPERAND_STRING
 *   %1         JMAL_OPERAND_ARG_REF
 *   [b]        JMAL_OPERAND_ADDRESS  (wraps another operand)
 *
 * All scalar payload lives in a single union — no more zeroing sval/ival/fval
 * in every constructor.
 * ═══════════════════════════════════════════════════════════════════════ */

typedef enum {
    JMAL_OPERAND_IDENT,
    JMAL_OPERAND_INT,
    JMAL_OPERAND_FLOAT,
    JMAL_OPERAND_STRING,
    JMAL_OPERAND_ARG_REF,    /* %N inside a macro body */
    JMAL_OPERAND_ADDRESS     /* [operand]              */
} JmalOperandKind;

struct JmalOperand {
    JmalOperandKind kind;
    union {
        char        *sval;   /* IDENT, STRING            */
        int          ival;   /* INT, ARG_REF             */
        double       fval;   /* FLOAT                    */
        JmalOperand *inner;  /* ADDRESS → operand inside [ ] */
    };
    int line;
};

static inline JmalOperand *jmal_operand_ident(const char *s, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind = JMAL_OPERAND_IDENT;
    o->sval = strdup(s);
    o->line = line;
    return o;
}

static inline JmalOperand *jmal_operand_int(int v, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind = JMAL_OPERAND_INT;
    o->ival = v;
    o->line = line;
    return o;
}

static inline JmalOperand *jmal_operand_float(double v, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind = JMAL_OPERAND_FLOAT;
    o->fval = v;
    o->line = line;
    return o;
}

static inline JmalOperand *jmal_operand_string(const char *s, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind = JMAL_OPERAND_STRING;
    o->sval = strdup(s);
    o->line = line;
    return o;
}

static inline JmalOperand *jmal_operand_arg_ref(int index, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind = JMAL_OPERAND_ARG_REF;
    o->ival = index;
    o->line = line;
    return o;
}

static inline JmalOperand *jmal_operand_address(JmalOperand *inner, int line)
{
    JmalOperand *o = (JmalOperand*)malloc(sizeof *o);
    o->kind  = JMAL_OPERAND_ADDRESS;
    o->inner = inner;
    o->line  = line;
    return o;
}

static inline void jmal_operand_free(JmalOperand *o)
{
    if (!o) return;
    switch (o->kind) {
        case JMAL_OPERAND_IDENT:
        case JMAL_OPERAND_STRING:  free(o->sval);              break;
        case JMAL_OPERAND_ADDRESS: jmal_operand_free(o->inner); break;
        default: break;
    }
    free(o);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JmalInstruction  (opcode + zero or more operands)
 *
 * Examples:
 *   mov a, [b]    opcode="mov",  operands=[ident, address]
 *   push rax      opcode="push", operands=[ident]
 *   push 1        opcode="push", operands=[int]
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalInstruction {
    char    *opcode;
    JmalVec  operands;   /* items are JmalOperand* */
    int      line;
};

static inline JmalInstruction *jmal_instr_new(const char *opcode, int line)
{
    JmalInstruction *i = (JmalInstruction*)malloc(sizeof *i);
    i->opcode          = strdup(opcode);
    i->operands        = jmal_vec_init();
    i->line            = line;
    return i;
}

static inline void jmal_instr_add_operand(JmalInstruction *i, JmalOperand *o)
{
    jmal_vec_push(&i->operands, o);
}

/* Convenience accessor */
#define jmal_instr_operand_count(i)   ((i)->operands.count)
#define jmal_instr_operand_get(i, n)  ((JmalOperand*)(i)->operands.items[n])

static inline void jmal_instr_free(JmalInstruction *i)
{
    if (!i) return;
    jmal_vec_free(&i->operands, (JmalVecFreeFn)jmal_operand_free);
    free(i->opcode);
    free(i);
}

struct JmalArgDecl {
    JmalArgRef               argRef;        /* the N in %N, 1-based */
    JmalTypeConstraintMulti *constraints;
    int                      line;
};

static inline JmalArgDecl *jmal_arg_decl_new(JmalArgRef arg_ref, JmalTypeConstraintMulti *tc, int line)
{
    JmalArgDecl *a = (JmalArgDecl*)malloc(sizeof *a);
    a->argRef      = arg_ref;
    a->constraints = tc;
    a->line        = line;
    return a;
}

static inline void jmal_arg_decl_free(JmalArgDecl *a)
{
    if (!a) return;
    jmal_type_multi_free(a->constraints);
    free(a);
}

typedef enum {
    JMAL_ROTATE_INT,   /* %rotate <literal N> */
    JMAL_ROTATE_ARG    /* %rotate <arg_ref>   */
} JmalRotateKind;

typedef enum {
    JMAL_STATEMENT_ARG_DECL,
    JMAL_STATEMENT_TYPEDEF,
    JMAL_STATEMENT_DEFINE,
    JMAL_STATEMENT_ROTATE,
    JMAL_STATEMENT_INSTR,
    JMAL_STATEMENT_REP,
    JMAL_STATEMENT_USE
} JmalStatementKind;

struct JmalStatementMulti {
    JmalVec stmts;   /* items are JmalStatement* */
};

struct JmalStatement {
    JmalStatementKind kind;
    int               line;
    union {
        JmalArgDecl     *arg;
        JmalTypeDef     *type;
        JmalDefine      *def;
        JmalInstruction *instr;
        JmalUse         *use;

        /* %rotate — folded in from JmalRotate */
        struct {
            JmalRotateKind rotate_kind;
            union {
                int        count;   /* JMAL_ROTATE_INT */
                JmalArgRef argRef;  /* JMAL_ROTATE_ARG */
            };
        } rotate;

        /* %rep…%endrep — folded in from JmalRepBlock */
        struct {
            JmalStatementMulti *body;
        } rep;
    };
};

/* ── JmalStatement constructors ──────────────────────────────────────── */

static inline JmalStatement *jmal_stmt_arg(JmalArgDecl *a, int line)
{
    JmalStatement *s = (JmalStatement*)malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_ARG_DECL;
    s->line = line;
    s->arg  = a;
    return s;
}

static inline JmalStatement *jmal_stmt_typedef(JmalTypeDef *t, int line)
{
    JmalStatement *s = (JmalStatement *)malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_TYPEDEF;
    s->line = line;
    s->type = t;
    return s;
}

static inline JmalStatement *jmal_stmt_define(JmalDefine *d, int line)
{
    JmalStatement *s = (JmalStatement *)malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_DEFINE;
    s->line = line;
    s->def  = d;
    return s;
}

static inline JmalStatement *jmal_stmt_rotate_int(int count, int line)
{
    JmalStatement *s        = (JmalStatement *)malloc(sizeof *s);
    s->kind                 = JMAL_STATEMENT_ROTATE;
    s->line                 = line;
    s->rotate.rotate_kind   = JMAL_ROTATE_INT;
    s->rotate.count         = count;
    return s;
}

static inline JmalStatement *jmal_stmt_rotate_arg(JmalArgRef arg_ref, int line)
{
    JmalStatement *s        = (JmalStatement *)malloc(sizeof *s);
    s->kind                 = JMAL_STATEMENT_ROTATE;
    s->line                 = line;
    s->rotate.rotate_kind   = JMAL_ROTATE_ARG;
    s->rotate.argRef        = arg_ref;
    return s;
}

static inline JmalStatement *jmal_stmt_instr(JmalInstruction *i, int line)
{
    JmalStatement *s = (JmalStatement *)malloc(sizeof *s);
    s->kind  = JMAL_STATEMENT_INSTR;
    s->line  = line;
    s->instr = i;
    return s;
}

static inline JmalStatement *jmal_stmt_use(JmalUse *u, int line)
{
    JmalStatement *s = (JmalStatement *)malloc(sizeof *s);
    s->kind = JMAL_STATEMENT_USE;
    s->line = line;
    s->use  = u;
    return s;
}

/* ── JmalStatementMulti ───────────────────────────────────────────────── */
static inline void jmal_stmt_free(JmalStatement *s);

static inline JmalStatementMulti *jmal_stmt_make_multi(JmalStatement *s)
{
    JmalStatementMulti *m = (JmalStatementMulti *)malloc(sizeof *m);
    m->stmts = jmal_vec_init();
    jmal_vec_push(&m->stmts, s);
    return m;
}

static inline void jmal_stmt_multi_add(JmalStatementMulti *m, JmalStatement *s)
{
    jmal_vec_push(&m->stmts, s);
}

/* Convenience accessors */
#define jmal_stmt_multi_count(m)   ((m)->stmts.count)
#define jmal_stmt_multi_get(m, i)  ((JmalStatement*)(m)->stmts.items[i])

static inline void jmal_stmt_multi_free(JmalStatementMulti *m)
{
    if (!m) return;
    jmal_vec_free(&m->stmts, (JmalVecFreeFn)jmal_stmt_free);
    free(m);
}

/* ── %rep constructor (uses JmalStatementMulti, must come after it) ──── */

static inline JmalStatement *jmal_stmt_rep(JmalStatementMulti *body, int line)
{
    JmalStatement *s = (JmalStatement *)malloc(sizeof *s);
    s->kind     = JMAL_STATEMENT_REP;
    s->line     = line;
    s->rep.body = body;
    return s;
}

/* ── JmalStatement free ───────────────────────────────────────────────── */

static inline void jmal_stmt_free(JmalStatement *s)
{
    if (!s) return;
    switch (s->kind) {
        case JMAL_STATEMENT_ARG_DECL: jmal_arg_decl_free(s->arg);          break;
        case JMAL_STATEMENT_TYPEDEF:  jmal_typedef_free(s->type);           break;
        case JMAL_STATEMENT_DEFINE:   jmal_define_free(s->def);             break;
        case JMAL_STATEMENT_ROTATE:   /* nothing heap-allocated inside */    break;
        case JMAL_STATEMENT_INSTR:    jmal_instr_free(s->instr);            break;
        case JMAL_STATEMENT_REP:      jmal_stmt_multi_free(s->rep.body);    break;
        case JMAL_STATEMENT_USE:      jmal_use_free(s->use);                break;
    }
    free(s);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JmalMacro
 * ═══════════════════════════════════════════════════════════════════════ */

struct JmalMacro {
    char               *name;
    JmalArity           arity_in;
    JmalArity           arity_out;
    JmalStatementMulti *body;
    int                 line;
};

static inline JmalMacro *jmal_macro_new(const char *name,
                                         JmalArity in, JmalArity out,
                                         int line)
{
    JmalMacro *m  = (JmalMacro *)malloc(sizeof *m);
    m->name       = strdup(name);
    m->arity_in   = in;
    m->arity_out  = out;
    m->body       = NULL;
    m->line       = line;
    return m;
}

static inline void jmal_macro_set_body(JmalMacro *m, JmalStatementMulti *body)
{
    if (m->body)
        JMAL_TODO("merge of multiple statement-multi blocks into one macro body");
    m->body = body;
}

static inline void jmal_macro_free(JmalMacro *m)
{
    if (!m) return;
    jmal_stmt_multi_free(m->body);
    free(m->name);
    free(m);
}

struct JmalProgram {
    JmalVec             typedefs;   /* JmalTypeDef*  */
    JmalVec             defines;    /* JmalDefine*   */
    JmalVec             macros;     /* JmalMacro*    */
    JmalStatementMulti *body;       /* top-level statement stream */
    char               *filename;
};

static inline JmalProgram *jmal_program_new(const char *filename)
{
    JmalProgram *p = (JmalProgram *)malloc(sizeof *p);
    p->typedefs    = jmal_vec_init();
    p->defines     = jmal_vec_init();
    p->macros      = jmal_vec_init();
    p->body        = NULL;
    p->filename    = filename ? strdup(filename) : strdup("<stdin>");
    return p;
}

static inline void jmal_program_add_typedef(JmalProgram *p, JmalTypeDef *t)
{
    jmal_vec_push(&p->typedefs, t);
}

static inline void jmal_program_add_define(JmalProgram *p, JmalDefine *d)
{
    jmal_vec_push(&p->defines, d);
}

static inline void jmal_program_add_macro(JmalProgram *p, JmalMacro *m)
{
    jmal_vec_push(&p->macros, m);
}

/* Add a single top-level statement to the program body */
static inline void jmal_program_add_stmt(JmalProgram *p, JmalStatement *s)
{
    if (!p->body)
        p->body = jmal_stmt_make_multi(s);
    else
        jmal_stmt_multi_add(p->body, s);
}

static inline void jmal_program_free(JmalProgram *p)
{
    if (!p) return;
    jmal_vec_free(&p->typedefs, (JmalVecFreeFn)jmal_typedef_free);
    jmal_vec_free(&p->defines,  (JmalVecFreeFn)jmal_define_free);
    jmal_vec_free(&p->macros,   (JmalVecFreeFn)jmal_macro_free);
    jmal_stmt_multi_free(p->body);
    free(p->filename);
    free(p);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Debug dump  (prints a human-readable summary to stdout)
 * ═══════════════════════════════════════════════════════════════════════ */

static inline const char *jmal_type_kind_str(JmalTypeKind k)
{
    switch (k) {
        case JMAL_TYPE_BUILTIN_REGISTER: return "register";
        case JMAL_TYPE_BUILTIN_STRING:   return "string";
        case JMAL_TYPE_BUILTIN_NUMBER:   return "number";
        case JMAL_TYPE_BUILTIN_ADDRESS:  return "address";
        case JMAL_TYPE_LIT_INT:          return "<lit int>";
        case JMAL_TYPE_ARG_REF:          return "<%arg>";
        case JMAL_TYPE_USER:             return "<user>";
    }
    return "?";
}

static inline void jmal_program_dump(const JmalProgram *p)
{
    printf("=== JmalProgram: %s ===\n", p->filename);

    printf("\n-- TypeDefs (%zu) --\n", p->typedefs.count);
    for (size_t i = 0; i < p->typedefs.count; i++) {
        JmalTypeDef *d = (JmalTypeDef*)p->typedefs.items[i];
        printf("  type '%s': ", d->name);
        for (size_t j = 0; j < jmal_type_multi_count(d->members); j++) {
            JmalTypeConstraint *tc = jmal_type_multi_get(d->members, j);
            if (tc->kind == JMAL_TYPE_USER)
                printf("%s", tc->name);
            else
                printf("%s", jmal_type_kind_str(tc->kind));
            if (j + 1 < jmal_type_multi_count(d->members)) printf(" | ");
        }
        printf("\n");
    }

    printf("\n-- Defines (%zu) --\n", p->defines.count);
    for (size_t i = 0; i < p->defines.count; i++) {
        JmalDefine *d = (JmalDefine*)p->defines.items[i];
        switch (d->kind) {
            case JMAL_DEFINE_STRING: printf("  define '%s' = \"%s\"\n", d->name, d->str_val); break;
            case JMAL_DEFINE_INT:    printf("  define '%s' = %d\n",     d->name, d->int_val); break;
            case JMAL_DEFINE_FLOAT:  printf("  define '%s' = %f\n",     d->name, d->flt_val); break;
        }
    }

    printf("\n-- Macros (%zu) --\n", p->macros.count);
    for (size_t i = 0; i < p->macros.count; i++) {
        JmalMacro *m = (JmalMacro*)p->macros.items[i];
        printf("  macro '%s' in:%d", m->name, m->arity_in.lo);
        if (jmal_arity_is_range(m->arity_in))  printf("-%d", m->arity_in.hi);
        printf("  out:%d", m->arity_out.lo);
        if (jmal_arity_is_range(m->arity_out)) printf("-%d", m->arity_out.hi);
        printf("  [%zu statements]\n",
               m->body ? jmal_stmt_multi_count(m->body) : 0);
    }

    size_t top_count = p->body ? jmal_stmt_multi_count(p->body) : 0;
    printf("\n-- Top-level statements (%zu) --\n", top_count);
    for (size_t i = 0; i < top_count; i++) {
        JmalStatement *s = jmal_stmt_multi_get(p->body, i);
        if (s->kind != JMAL_STATEMENT_INSTR) continue;
        JmalInstruction *instr = s->instr;
        printf("  %s", instr->opcode);
        for (size_t j = 0; j < jmal_instr_operand_count(instr); j++) {
            printf(j == 0 ? "  " : ", ");
            JmalOperand *o = jmal_instr_operand_get(instr, j);
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