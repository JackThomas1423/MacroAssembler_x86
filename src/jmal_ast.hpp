#pragma once

#include <cassert>
#include <cstdio>
#include <string>
#include <variant>
#include <vector>

// ---------------------------------------------------------------------------
// Diagnostic helper
// ---------------------------------------------------------------------------

#ifdef JMAL_NO_TODO_ASSERT
#  define JMAL_TODO(msg)                                                \
    do {                                                                \
        std::fprintf(stderr, "warning: TODO: %s  (%s:%d)\n",           \
                     (msg), __FILE__, __LINE__);                        \
    } while (false)
#else
#  define JMAL_TODO(msg)                                                \
    do {                                                                \
        std::fprintf(stderr, "TODO: %s  (%s:%d)\n",                    \
                     (msg), __FILE__, __LINE__);                        \
        assert(false);                                                  \
    } while (false)
#endif

// ---------------------------------------------------------------------------
// JmalArgRef  — the N in %N, 1-based
// ---------------------------------------------------------------------------

using JmalArgRef = unsigned int;

// ---------------------------------------------------------------------------
// JmalArity  — trivial value type so Bison %token / %type work unchanged
// ---------------------------------------------------------------------------

struct JmalArity {
    int lo;
    int hi;

    static JmalArity fixed(int n)             { return {n, n};    }
    static JmalArity range(int lo_, int hi_)  { return {lo_, hi_}; }
    bool is_range() const                     { return lo != hi;  }
};

// C-style helpers kept for jmal.y call-site compatibility
inline JmalArity jmal_arity_fixed(int n)           { return JmalArity::fixed(n);      }
inline JmalArity jmal_arity_range(int lo, int hi)  { return JmalArity::range(lo, hi); }
inline bool      jmal_arity_is_range(JmalArity a)  { return a.is_range();             }

// ---------------------------------------------------------------------------
// JmalTypeConstraint
// ---------------------------------------------------------------------------

enum class JmalTypeKind {
    BuiltinRegister,
    BuiltinString,
    BuiltinNumber,
    BuiltinAddress,
    LitInt,
    ArgRef,
    User,
};

using JmalTypePayload = std::variant<
    std::monostate,  // builtins carry no extra data
    int,             // LitInt / ArgRef
    std::string      // User
>;

struct JmalTypeConstraint {
    JmalTypeKind    kind;
    JmalTypePayload payload;
    int             line;

    const std::string& name()  const { return std::get<std::string>(payload); }
    int                value() const { return std::get<int>(payload);         }

    static JmalTypeConstraint* builtin(JmalTypeKind k, int line);
    static JmalTypeConstraint* arg_ref(JmalArgRef ref, int line);
    static JmalTypeConstraint* lit_int(int v, int line);
    static JmalTypeConstraint* user(const std::string& name, int line);
};

// ---------------------------------------------------------------------------
// JmalTypeConstraintMulti  — owns its elements
// ---------------------------------------------------------------------------

struct JmalTypeConstraintMulti {
    std::vector<JmalTypeConstraint*> types;

    ~JmalTypeConstraintMulti();

    void add(JmalTypeConstraint* t)             { types.push_back(t);  }
    std::size_t count() const                   { return types.size(); }
    JmalTypeConstraint* get(std::size_t i) const { return types[i];    }

    static JmalTypeConstraintMulti* make(JmalTypeConstraint* first);
};

// ---------------------------------------------------------------------------
// JmalTypeDef  (%type name: member | member | ...)
// ---------------------------------------------------------------------------

struct JmalTypeDef {
    std::string              name;
    JmalTypeConstraintMulti* members;  // owned
    int                      line;

    ~JmalTypeDef();

    static JmalTypeDef* make(const std::string& name,
                             JmalTypeConstraintMulti* members,
                             int line);
};

// ---------------------------------------------------------------------------
// JmalDefine  (%define name value)
// ---------------------------------------------------------------------------

using JmalDefineValue = std::variant<std::string, int, double>;

enum class JmalDefineKind { String, Int, Float };

struct JmalDefine {
    std::string     name;
    JmalDefineKind  kind;
    JmalDefineValue value;
    int             line;

    const std::string& str_val() const { return std::get<std::string>(value); }
    int                int_val() const { return std::get<int>(value);          }
    double             flt_val() const { return std::get<double>(value);       }

    static JmalDefine* make_str  (const std::string& name, const std::string& val, int line);
    static JmalDefine* make_int  (const std::string& name, int val,    int line);
    static JmalDefine* make_float(const std::string& name, double val, int line);
};

// ---------------------------------------------------------------------------
// JmalUse  (%use macroname [args...])
// ---------------------------------------------------------------------------

struct JmalUse {
    std::string              name;
    JmalTypeConstraintMulti* args;  // nullable, owned
    int                      line;

    ~JmalUse();

    static JmalUse* make(const std::string& name,
                         JmalTypeConstraintMulti* args,
                         int line);
};

// ---------------------------------------------------------------------------
// JmalOperand
// ---------------------------------------------------------------------------

enum class JmalOperandKind {
    Ident,
    Int,
    Float,
    String,
    ArgRef,
    Address,
};

struct JmalOperand;

using JmalOperandPayload = std::variant<
    std::string,   // Ident, String
    int,           // Int, ArgRef
    double,        // Float
    JmalOperand*   // Address (owned)
>;

struct JmalOperand {
    JmalOperandKind    kind;
    JmalOperandPayload payload;
    int                line;

    ~JmalOperand();

    const std::string& sval()  const { return std::get<std::string>(payload);  }
    int                ival()  const { return std::get<int>(payload);           }
    double             fval()  const { return std::get<double>(payload);        }
    JmalOperand*       inner() const { return std::get<JmalOperand*>(payload);  }

    static JmalOperand* ident  (const std::string& s, int line);
    static JmalOperand* integer(int v,                int line);
    static JmalOperand* fp     (double v,             int line);
    static JmalOperand* string (const std::string& s, int line);
    static JmalOperand* arg_ref(int index,            int line);
    static JmalOperand* address(JmalOperand* inner,   int line);
};

// ---------------------------------------------------------------------------
// JmalInstruction  (opcode + operands)
// ---------------------------------------------------------------------------

struct JmalInstruction {
    std::string               opcode;
    std::vector<JmalOperand*> operands;  // owned
    int                       line;

    ~JmalInstruction();

    void         add_operand(JmalOperand* o)          { operands.push_back(o); }
    std::size_t  operand_count()              const    { return operands.size(); }
    JmalOperand* operand_get(std::size_t i)   const    { return operands[i];     }

    static JmalInstruction* make(const std::string& opcode, int line);
};

// ---------------------------------------------------------------------------
// JmalArgDecl  (%arg %N : type...)
// ---------------------------------------------------------------------------

struct JmalArgDecl {
    JmalArgRef               argRef;
    JmalTypeConstraintMulti* constraints;  // owned
    int                      line;

    ~JmalArgDecl();

    static JmalArgDecl* make(JmalArgRef ref,
                             JmalTypeConstraintMulti* constraints,
                             int line);
};

// ---------------------------------------------------------------------------
// JmalStatement  — tagged union of all statement kinds
// ---------------------------------------------------------------------------

enum class JmalStatementKind {
    ArgDecl,
    Typedef,
    Define,
    Rotate,
    Instr,
    Rep,
    Use,
    Ensure,
};

enum class JmalRotateKind { Int, Arg };

struct JmalStatementMulti;

struct JmalStatement {
    JmalStatementKind kind;
    int               line;

    // Only the field matching `kind` is non-null / meaningful.
    JmalArgDecl*        arg        = nullptr;
    JmalTypeDef*        type       = nullptr;
    JmalDefine*         def        = nullptr;
    JmalInstruction*    instr      = nullptr;
    JmalUse*            use        = nullptr;
    JmalStatementMulti* rep_body   = nullptr;

    JmalRotateKind rotate_kind   = JmalRotateKind::Int;
    int            rotate_count  = 0;
    JmalArgRef     rotate_argRef = 0;

    // Ensure fields  (%ensure <lhs> <op> <rhs>)
    std::string ensure_lhs;
    std::string ensure_op;
    std::string ensure_rhs;

    ~JmalStatement();

    static JmalStatement* make_arg       (JmalArgDecl* a,           int line);
    static JmalStatement* make_typedef   (JmalTypeDef* t,           int line);
    static JmalStatement* make_define    (JmalDefine* d,            int line);
    static JmalStatement* make_rotate_int(int count,                int line);
    static JmalStatement* make_rotate_arg(JmalArgRef ref,           int line);
    static JmalStatement* make_instr     (JmalInstruction* i,       int line);
    static JmalStatement* make_rep       (JmalStatementMulti* body, int line);
    static JmalStatement* make_use       (JmalUse* u,               int line);
    static JmalStatement* make_ensure    (const std::string& lhs,
                                          const std::string& op,
                                          const std::string& rhs,   int line);
};

// ---------------------------------------------------------------------------
// JmalStatementMulti  — owns its elements
// ---------------------------------------------------------------------------

struct JmalStatementMulti {
    std::vector<JmalStatement*> stmts;

    ~JmalStatementMulti();

    void           add(JmalStatement* s)          { stmts.push_back(s);  }
    std::size_t    count()              const      { return stmts.size(); }
    JmalStatement* get(std::size_t i)   const      { return stmts[i];     }

    static JmalStatementMulti* make(JmalStatement* first);
};

// ---------------------------------------------------------------------------
// JmalMacro
// ---------------------------------------------------------------------------

struct JmalMacro {
    std::string         name;
    JmalArity           arity_in;
    JmalArity           arity_out;
    JmalStatementMulti* body;  // owned, nullptr until set
    int                 line;

    ~JmalMacro();

    void set_body(JmalStatementMulti* b);

    static JmalMacro* make(const std::string& name,
                           JmalArity in, JmalArity out,
                           int line);
};

// ---------------------------------------------------------------------------
// JmalProgram  — root of the AST
// ---------------------------------------------------------------------------

struct JmalProgram {
    std::vector<JmalTypeDef*>  typedefs;  // owned
    std::vector<JmalDefine*>   defines;   // owned
    std::vector<JmalMacro*>    macros;    // owned
    JmalStatementMulti*        body;      // owned, nullable
    std::string                filename;

    explicit JmalProgram(const std::string& filename);
    ~JmalProgram();

    void add_typedef(JmalTypeDef* t)   { typedefs.push_back(t); }
    void add_define (JmalDefine*  d)   { defines .push_back(d); }
    void add_macro  (JmalMacro*   m)   { macros  .push_back(m); }
    void add_stmt   (JmalStatement* s);

    void dump() const;
};

// ===========================================================================
// Legacy C-style shims — keep jmal.y call sites compiling without changes
// ===========================================================================

inline JmalTypeConstraint* jmal_type_builtin(JmalTypeKind k, int line)
    { return JmalTypeConstraint::builtin(k, line); }
inline JmalTypeConstraint* jmal_type_arg_ref(JmalArgRef ref, int line)
    { return JmalTypeConstraint::arg_ref(ref, line); }
inline JmalTypeConstraint* jmal_type_lit_int(int v, int line)
    { return JmalTypeConstraint::lit_int(v, line); }
inline JmalTypeConstraint* jmal_type_user(const char* name, int line)
    { return JmalTypeConstraint::user(name, line); }
inline void jmal_type_free(JmalTypeConstraint* t)
    { delete t; }

inline JmalTypeConstraintMulti* jmal_type_make_multi(JmalTypeConstraint* first)
    { return JmalTypeConstraintMulti::make(first); }
inline void jmal_type_multi_add_type(JmalTypeConstraintMulti* m, JmalTypeConstraint* t)
    { m->add(t); }
inline void jmal_type_multi_free(JmalTypeConstraintMulti* m)
    { delete m; }
inline std::size_t jmal_type_multi_count(const JmalTypeConstraintMulti* m)
    { return m->count(); }
inline JmalTypeConstraint* jmal_type_multi_get(const JmalTypeConstraintMulti* m, std::size_t i)
    { return m->get(i); }

inline JmalTypeDef* jmal_typedef_new(const char* name, JmalTypeConstraintMulti* tc, int line)
    { return JmalTypeDef::make(name, tc, line); }
inline void jmal_typedef_free(JmalTypeDef* d)
    { delete d; }

inline JmalDefine* jmal_define_str(const char* name, const char* val, int line)
    { return JmalDefine::make_str(name, val, line); }
inline JmalDefine* jmal_define_int(const char* name, int val, int line)
    { return JmalDefine::make_int(name, val, line); }
inline JmalDefine* jmal_define_float(const char* name, double val, int line)
    { return JmalDefine::make_float(name, val, line); }
inline void jmal_define_free(JmalDefine* d)
    { delete d; }

inline JmalUse* jmal_use_new(const char* name, JmalTypeConstraintMulti* args, int line)
    { return JmalUse::make(name, args, line); }
inline void jmal_use_free(JmalUse* u)
    { delete u; }

inline JmalOperand* jmal_operand_ident  (const char* s,    int line) { return JmalOperand::ident(s, line);        }
inline JmalOperand* jmal_operand_int    (int v,             int line) { return JmalOperand::integer(v, line);      }
inline JmalOperand* jmal_operand_float  (double v,          int line) { return JmalOperand::fp(v, line);           }
inline JmalOperand* jmal_operand_string (const char* s,    int line) { return JmalOperand::string(s, line);       }
inline JmalOperand* jmal_operand_arg_ref(int idx,           int line) { return JmalOperand::arg_ref(idx, line);    }
inline JmalOperand* jmal_operand_address(JmalOperand* inner,int line) { return JmalOperand::address(inner, line);  }
inline void         jmal_operand_free   (JmalOperand* o)              { delete o; }

inline JmalInstruction* jmal_instr_new(const char* opcode, int line)
    { return JmalInstruction::make(opcode, line); }
inline void jmal_instr_add_operand(JmalInstruction* i, JmalOperand* o)
    { i->add_operand(o); }
inline std::size_t  jmal_instr_operand_count(const JmalInstruction* i)
    { return i->operand_count(); }
inline JmalOperand* jmal_instr_operand_get(const JmalInstruction* i, std::size_t n)
    { return i->operand_get(n); }
inline void jmal_instr_free(JmalInstruction* i)
    { delete i; }

inline JmalArgDecl* jmal_arg_decl_new(JmalArgRef ref, JmalTypeConstraintMulti* tc, int line)
    { return JmalArgDecl::make(ref, tc, line); }
inline void jmal_arg_decl_free(JmalArgDecl* a)
    { delete a; }

inline JmalStatementMulti* jmal_stmt_make_multi(JmalStatement* s)
    { return JmalStatementMulti::make(s); }
inline void jmal_stmt_multi_add(JmalStatementMulti* m, JmalStatement* s)
    { m->add(s); }
inline std::size_t    jmal_stmt_multi_count(const JmalStatementMulti* m)
    { return m->count(); }
inline JmalStatement* jmal_stmt_multi_get(const JmalStatementMulti* m, std::size_t i)
    { return m->get(i); }
inline void jmal_stmt_multi_free(JmalStatementMulti* m)
    { delete m; }

inline JmalStatement* jmal_stmt_arg       (JmalArgDecl* a,           int line) { return JmalStatement::make_arg(a, line);          }
inline JmalStatement* jmal_stmt_typedef   (JmalTypeDef* t,           int line) { return JmalStatement::make_typedef(t, line);      }
inline JmalStatement* jmal_stmt_define    (JmalDefine* d,            int line) { return JmalStatement::make_define(d, line);       }
inline JmalStatement* jmal_stmt_rotate_int(int count,                int line) { return JmalStatement::make_rotate_int(count, line); }
inline JmalStatement* jmal_stmt_rotate_arg(JmalArgRef ref,           int line) { return JmalStatement::make_rotate_arg(ref, line);   }
inline JmalStatement* jmal_stmt_instr     (JmalInstruction* i,       int line) { return JmalStatement::make_instr(i, line);        }
inline JmalStatement* jmal_stmt_rep       (JmalStatementMulti* body, int line) { return JmalStatement::make_rep(body, line);       }
inline JmalStatement* jmal_stmt_use       (JmalUse* u,               int line) { return JmalStatement::make_use(u, line);          }
inline void           jmal_stmt_free      (JmalStatement* s)                   { delete s; }
inline JmalStatement* jmal_stmt_ensure    (const std::string& lhs,
                                           const std::string& op,
                                           const std::string& rhs, int line)
    { return JmalStatement::make_ensure(lhs, op, rhs, line); }

inline JmalMacro* jmal_macro_new(const char* name, JmalArity in, JmalArity out, int line)
    { return JmalMacro::make(name, in, out, line); }
inline void jmal_macro_set_body(JmalMacro* m, JmalStatementMulti* body)
    { m->set_body(body); }
inline void jmal_macro_free(JmalMacro* m)
    { delete m; }

inline JmalProgram* jmal_program_new(const char* filename)
    { return new JmalProgram(filename ? filename : "<stdin>"); }
inline void jmal_program_add_typedef(JmalProgram* p, JmalTypeDef* t)   { p->add_typedef(t); }
inline void jmal_program_add_define (JmalProgram* p, JmalDefine*  d)   { p->add_define(d);  }
inline void jmal_program_add_macro  (JmalProgram* p, JmalMacro*   m)   { p->add_macro(m);   }
inline void jmal_program_add_stmt   (JmalProgram* p, JmalStatement* s) { p->add_stmt(s);    }
inline void jmal_program_free       (JmalProgram* p)                   { delete p; }
inline void jmal_program_dump       (const JmalProgram* p)             { p->dump(); }

// ---------------------------------------------------------------------------
// Legacy enum aliases — existing code using JMAL_TYPE_BUILTIN_REGISTER etc.
// continues to compile without changes.
// ---------------------------------------------------------------------------

inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_REGISTER = JmalTypeKind::BuiltinRegister;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_STRING   = JmalTypeKind::BuiltinString;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_NUMBER   = JmalTypeKind::BuiltinNumber;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_ADDRESS  = JmalTypeKind::BuiltinAddress;
inline constexpr JmalTypeKind JMAL_TYPE_LIT_INT          = JmalTypeKind::LitInt;
inline constexpr JmalTypeKind JMAL_TYPE_ARG_REF          = JmalTypeKind::ArgRef;
inline constexpr JmalTypeKind JMAL_TYPE_USER             = JmalTypeKind::User;

inline constexpr JmalDefineKind JMAL_DEFINE_STRING = JmalDefineKind::String;
inline constexpr JmalDefineKind JMAL_DEFINE_INT    = JmalDefineKind::Int;
inline constexpr JmalDefineKind JMAL_DEFINE_FLOAT  = JmalDefineKind::Float;

inline constexpr JmalStatementKind JMAL_STATEMENT_ARG_DECL = JmalStatementKind::ArgDecl;
inline constexpr JmalStatementKind JMAL_STATEMENT_TYPEDEF  = JmalStatementKind::Typedef;
inline constexpr JmalStatementKind JMAL_STATEMENT_DEFINE   = JmalStatementKind::Define;
inline constexpr JmalStatementKind JMAL_STATEMENT_ROTATE   = JmalStatementKind::Rotate;
inline constexpr JmalStatementKind JMAL_STATEMENT_INSTR    = JmalStatementKind::Instr;
inline constexpr JmalStatementKind JMAL_STATEMENT_REP      = JmalStatementKind::Rep;
inline constexpr JmalStatementKind JMAL_STATEMENT_USE      = JmalStatementKind::Use;
inline constexpr JmalStatementKind JMAL_STATEMENT_ENSURE   = JmalStatementKind::Ensure;

inline constexpr JmalOperandKind JMAL_OPERAND_IDENT   = JmalOperandKind::Ident;
inline constexpr JmalOperandKind JMAL_OPERAND_INT     = JmalOperandKind::Int;
inline constexpr JmalOperandKind JMAL_OPERAND_FLOAT   = JmalOperandKind::Float;
inline constexpr JmalOperandKind JMAL_OPERAND_STRING  = JmalOperandKind::String;
inline constexpr JmalOperandKind JMAL_OPERAND_ARG_REF = JmalOperandKind::ArgRef;
inline constexpr JmalOperandKind JMAL_OPERAND_ADDRESS = JmalOperandKind::Address;

inline constexpr JmalRotateKind JMAL_ROTATE_INT = JmalRotateKind::Int;
inline constexpr JmalRotateKind JMAL_ROTATE_ARG = JmalRotateKind::Arg;