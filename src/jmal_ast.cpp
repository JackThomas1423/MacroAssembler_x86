#include "jmal_ast.hpp"

#include <cstdio>
#include <stdexcept>

// ===========================================================================
// JmalTypeConstraint
// ===========================================================================

JmalTypeConstraint* JmalTypeConstraint::builtin(JmalTypeKind k, int line)
{
    return new JmalTypeConstraint{k, std::monostate{}, line};
}

JmalTypeConstraint* JmalTypeConstraint::arg_ref(JmalArgRef ref, int line)
{
    return new JmalTypeConstraint{JmalTypeKind::ArgRef, static_cast<int>(ref), line};
}

JmalTypeConstraint* JmalTypeConstraint::lit_int(int v, int line)
{
    return new JmalTypeConstraint{JmalTypeKind::LitInt, v, line};
}

JmalTypeConstraint* JmalTypeConstraint::user(const std::string& name, int line)
{
    return new JmalTypeConstraint{JmalTypeKind::User, name, line};
}

// ===========================================================================
// JmalTypeConstraintMulti
// ===========================================================================

JmalTypeConstraintMulti::~JmalTypeConstraintMulti()
{
    for (JmalTypeConstraint* t : types)
        delete t;
}

JmalTypeConstraintMulti* JmalTypeConstraintMulti::make(JmalTypeConstraint* first)
{
    auto* m = new JmalTypeConstraintMulti;
    m->types.push_back(first);
    return m;
}

// ===========================================================================
// JmalTypeDef
// ===========================================================================

JmalTypeDef::~JmalTypeDef()
{
    delete members;
}

JmalTypeDef* JmalTypeDef::make(const std::string& name,
                                JmalTypeConstraintMulti* members,
                                int line)
{
    return new JmalTypeDef{name, members, line};
}

// ===========================================================================
// JmalDefine
// ===========================================================================

JmalDefine* JmalDefine::make_str(const std::string& name, const std::string& val, int line)
{
    return new JmalDefine{name, JmalDefineKind::String, val, line};
}

JmalDefine* JmalDefine::make_int(const std::string& name, int val, int line)
{
    return new JmalDefine{name, JmalDefineKind::Int, val, line};
}

JmalDefine* JmalDefine::make_float(const std::string& name, double val, int line)
{
    return new JmalDefine{name, JmalDefineKind::Float, val, line};
}

// ===========================================================================
// JmalUse
// ===========================================================================

JmalUse::~JmalUse()
{
    delete args;
}

JmalUse* JmalUse::make(const std::string& name, JmalTypeConstraintMulti* args, int line)
{
    return new JmalUse{name, args, line};
}

// ===========================================================================
// JmalOperand
// ===========================================================================

JmalOperand::~JmalOperand()
{
    if (kind == JmalOperandKind::Address)
        delete std::get<JmalOperand*>(payload);
}

JmalOperand* JmalOperand::ident(const std::string& s, int line)
{
    return new JmalOperand{JmalOperandKind::Ident, s, line};
}

JmalOperand* JmalOperand::integer(int v, int line)
{
    return new JmalOperand{JmalOperandKind::Int, v, line};
}

JmalOperand* JmalOperand::fp(double v, int line)
{
    return new JmalOperand{JmalOperandKind::Float, v, line};
}

JmalOperand* JmalOperand::string(const std::string& s, int line)
{
    return new JmalOperand{JmalOperandKind::String, s, line};
}

JmalOperand* JmalOperand::arg_ref(int index, int line)
{
    return new JmalOperand{JmalOperandKind::ArgRef, index, line};
}

JmalOperand* JmalOperand::address(JmalOperand* inner, int line)
{
    return new JmalOperand{JmalOperandKind::Address, inner, line};
}

// ===========================================================================
// JmalInstruction
// ===========================================================================

JmalInstruction::~JmalInstruction()
{
    for (JmalOperand* o : operands)
        delete o;
}

JmalInstruction* JmalInstruction::make(const std::string& opcode, int line)
{
    return new JmalInstruction{opcode, {}, line};
}

// ===========================================================================
// JmalArgDecl
// ===========================================================================

JmalArgDecl::~JmalArgDecl()
{
    delete constraints;
}

JmalArgDecl* JmalArgDecl::make(JmalArgRef ref, JmalTypeConstraintMulti* constraints, int line)
{
    return new JmalArgDecl{ref, constraints, line};
}

// ===========================================================================
// JmalStatementMulti
// ===========================================================================

JmalStatementMulti::~JmalStatementMulti()
{
    for (JmalStatement* s : stmts)
        delete s;
}

JmalStatementMulti* JmalStatementMulti::make(JmalStatement* first)
{
    auto* m = new JmalStatementMulti;
    m->stmts.push_back(first);
    return m;
}

// ===========================================================================
// JmalStatement
// ===========================================================================

JmalStatement::~JmalStatement()
{
    switch (kind) {
        case JmalStatementKind::ArgDecl: delete arg;      break;
        case JmalStatementKind::Typedef: delete type;     break;
        case JmalStatementKind::Define:  delete def;      break;
        case JmalStatementKind::Instr:   delete instr;    break;
        case JmalStatementKind::Rep:     delete rep_body; break;
        case JmalStatementKind::Use:     delete use;      break;
        case JmalStatementKind::Rotate:  /* no heap data */ break;
        case JmalStatementKind::Ensure:  /* strings are value members */ break;
    }
}

JmalStatement* JmalStatement::make_arg(JmalArgDecl* a, int line)
{
    auto* s  = new JmalStatement;
    s->kind  = JmalStatementKind::ArgDecl;
    s->line  = line;
    s->arg   = a;
    return s;
}

JmalStatement* JmalStatement::make_typedef(JmalTypeDef* t, int line)
{
    auto* s  = new JmalStatement;
    s->kind  = JmalStatementKind::Typedef;
    s->line  = line;
    s->type  = t;
    return s;
}

JmalStatement* JmalStatement::make_define(JmalDefine* d, int line)
{
    auto* s  = new JmalStatement;
    s->kind  = JmalStatementKind::Define;
    s->line  = line;
    s->def   = d;
    return s;
}

JmalStatement* JmalStatement::make_rotate_int(int count, int line)
{
    auto* s           = new JmalStatement;
    s->kind           = JmalStatementKind::Rotate;
    s->line           = line;
    s->rotate_kind    = JmalRotateKind::Int;
    s->rotate_count   = count;
    return s;
}

JmalStatement* JmalStatement::make_rotate_arg(JmalArgRef ref, int line)
{
    auto* s           = new JmalStatement;
    s->kind           = JmalStatementKind::Rotate;
    s->line           = line;
    s->rotate_kind    = JmalRotateKind::Arg;
    s->rotate_argRef  = ref;
    return s;
}

JmalStatement* JmalStatement::make_instr(JmalInstruction* i, int line)
{
    auto* s  = new JmalStatement;
    s->kind  = JmalStatementKind::Instr;
    s->line  = line;
    s->instr = i;
    return s;
}

JmalStatement* JmalStatement::make_rep(JmalStatementMulti* body, int line)
{
    auto* s      = new JmalStatement;
    s->kind      = JmalStatementKind::Rep;
    s->line      = line;
    s->rep_body  = body;
    return s;
}

JmalStatement* JmalStatement::make_use(JmalUse* u, int line)
{
    auto* s  = new JmalStatement;
    s->kind  = JmalStatementKind::Use;
    s->line  = line;
    s->use   = u;
    return s;
}

JmalStatement* JmalStatement::make_ensure(const std::string& lhs,
                                          const std::string& op,
                                          const std::string& rhs,
                                          int line)
{
    auto* s        = new JmalStatement;
    s->kind        = JmalStatementKind::Ensure;
    s->line        = line;
    s->ensure_lhs  = lhs;
    s->ensure_op   = op;
    s->ensure_rhs  = rhs;
    return s;
}

// ===========================================================================
// JmalMacro
// ===========================================================================

JmalMacro::~JmalMacro()
{
    delete body;
}

void JmalMacro::set_body(JmalStatementMulti* b)
{
    if (body)
        JMAL_TODO("merge of multiple statement-multi blocks into one macro body");
    body = b;
}

JmalMacro* JmalMacro::make(const std::string& name, JmalArity in, JmalArity out, int line)
{
    return new JmalMacro{name, in, out, nullptr, line};
}

// ===========================================================================
// JmalProgram
// ===========================================================================

JmalProgram::JmalProgram(const std::string& fname)
    : body(nullptr), filename(fname)
{}

JmalProgram::~JmalProgram()
{
    for (JmalTypeDef* t : typedefs) delete t;
    for (JmalDefine*  d : defines)  delete d;
    for (JmalMacro*   m : macros)   delete m;
    delete body;
}

void JmalProgram::add_stmt(JmalStatement* s)
{
    if (!s) return;
    if (!body)
        body = JmalStatementMulti::make(s);
    else
        body->add(s);
}

// ===========================================================================
// JmalProgram::dump
// ===========================================================================

static const char* type_kind_str(JmalTypeKind k);  /* defined below */

static void dump_operand(const JmalOperand* o)
{
    switch (o->kind) {
        case JmalOperandKind::Ident:   std::printf("%s",     o->sval().c_str()); break;
        case JmalOperandKind::Int:     std::printf("%d",     o->ival());         break;
        case JmalOperandKind::Float:   std::printf("%f",     o->fval());         break;
        case JmalOperandKind::String:  std::printf("\"%s\"", o->sval().c_str()); break;
        case JmalOperandKind::ArgRef:  std::printf("%%%d",   o->ival());         break;
        case JmalOperandKind::Address:
            std::printf("[");
            dump_operand(o->inner());
            std::printf("]");
            break;
    }
}

static void dump_type_constraint(const JmalTypeConstraint* tc)
{
    if (tc->kind == JmalTypeKind::User)
        std::printf("%s", tc->name().c_str());
    else if (tc->kind == JmalTypeKind::ArgRef)
        std::printf("%%%d", tc->value());
    else if (tc->kind == JmalTypeKind::LitInt)
        std::printf("%d", tc->value());
    else
        std::printf("%s", type_kind_str(tc->kind));
}

/* Forward declaration so dump_stmt can recurse into rep bodies. */
static void dump_stmts(const JmalStatementMulti* body, int indent);

static void dump_stmt(const JmalStatement* s, int indent)
{
    std::string pad(indent * 2, ' ');
    switch (s->kind) {
        case JmalStatementKind::Instr: {
            const JmalInstruction* instr = s->instr;
            std::printf("%s%s", pad.c_str(), instr->opcode.c_str());
            for (std::size_t j = 0; j < instr->operand_count(); ++j) {
                std::printf(j == 0 ? "  " : ", ");
                dump_operand(instr->operand_get(j));
            }
            std::printf("\n");
            break;
        }
        case JmalStatementKind::ArgDecl: {
            const JmalArgDecl* a = s->arg;
            std::printf("%s%%arg %%%u :", pad.c_str(), a->argRef);
            for (std::size_t j = 0; j < a->constraints->count(); ++j) {
                if (j) std::printf(" |");
                std::printf(" ");
                dump_type_constraint(a->constraints->get(j));
            }
            std::printf("\n");
            break;
        }
        case JmalStatementKind::Typedef: {
            const JmalTypeDef* t = s->type;
            std::printf("%s%%type %s :", pad.c_str(), t->name.c_str());
            for (std::size_t j = 0; j < t->members->count(); ++j) {
                if (j) std::printf(" |");
                std::printf(" ");
                dump_type_constraint(t->members->get(j));
            }
            std::printf("\n");
            break;
        }
        case JmalStatementKind::Define: {
            const JmalDefine* d = s->def;
            std::printf("%s%%define %s ", pad.c_str(), d->name.c_str());
            switch (d->kind) {
                case JmalDefineKind::String: std::printf("\"%s\"", d->str_val().c_str()); break;
                case JmalDefineKind::Int:    std::printf("%d", d->int_val());               break;
                case JmalDefineKind::Float:  std::printf("%f", d->flt_val());               break;
            }
            std::printf("\n");
            break;
        }
        case JmalStatementKind::Rotate:
            if (s->rotate_kind == JmalRotateKind::Int)
                std::printf("%s%%rotate %d\n", pad.c_str(), s->rotate_count);
            else
                std::printf("%s%%rotate %%%u\n", pad.c_str(), s->rotate_argRef);
            break;
        case JmalStatementKind::Use: {
            const JmalUse* u = s->use;
            std::printf("%s%%use %s", pad.c_str(), u->name.c_str());
            if (u->args) {
                for (std::size_t j = 0; j < u->args->count(); ++j) {
                    std::printf(" ");
                    dump_type_constraint(u->args->get(j));
                }
            }
            std::printf("\n");
            break;
        }
        case JmalStatementKind::Ensure:
            std::printf("%s%%ensure %s %s %s\n",
                        pad.c_str(),
                        s->ensure_lhs.c_str(),
                        s->ensure_op.c_str(),
                        s->ensure_rhs.c_str());
            break;
        case JmalStatementKind::Rep:
            std::printf("%s%%rep\n", pad.c_str());
            dump_stmts(s->rep_body, indent + 1);
            std::printf("%s%%endrep\n", pad.c_str());
            break;
    }
}

static void dump_stmts(const JmalStatementMulti* body, int indent)
{
    if (!body) return;
    for (std::size_t i = 0; i < body->count(); ++i)
        dump_stmt(body->get(i), indent);
}

static const char* type_kind_str(JmalTypeKind k)
{
    switch (k) {
        case JmalTypeKind::BuiltinRegister: return "register";
        case JmalTypeKind::BuiltinString:   return "string";
        case JmalTypeKind::BuiltinNumber:   return "number";
        case JmalTypeKind::BuiltinAddress:  return "address";
        case JmalTypeKind::LitInt:          return "<lit int>";
        case JmalTypeKind::ArgRef:          return "<%arg>";
        case JmalTypeKind::User:            return "<user>";
    }
    return "?";
}

void JmalProgram::dump() const
{
    std::printf("=== JmalProgram: %s ===\n", filename.c_str());

    std::printf("\n-- TypeDefs (%zu) --\n", typedefs.size());
    for (const JmalTypeDef* d : typedefs) {
        std::printf("  type '%s': ", d->name.c_str());
        for (std::size_t j = 0; j < d->members->count(); ++j) {
            const JmalTypeConstraint* tc = d->members->get(j);
            if (tc->kind == JmalTypeKind::User)
                std::printf("%s", tc->name().c_str());
            else
                std::printf("%s", type_kind_str(tc->kind));
            if (j + 1 < d->members->count())
                std::printf(" | ");
        }
        std::printf("\n");
    }

    std::printf("\n-- Defines (%zu) --\n", defines.size());
    for (const JmalDefine* d : defines) {
        switch (d->kind) {
            case JmalDefineKind::String:
                std::printf("  define '%s' = \"%s\"\n", d->name.c_str(), d->str_val().c_str());
                break;
            case JmalDefineKind::Int:
                std::printf("  define '%s' = %d\n", d->name.c_str(), d->int_val());
                break;
            case JmalDefineKind::Float:
                std::printf("  define '%s' = %f\n", d->name.c_str(), d->flt_val());
                break;
        }
    }

    std::printf("\n-- Macros (%zu) --\n", macros.size());
    for (const JmalMacro* m : macros) {
        std::printf("  macro '%s' in:%d", m->name.c_str(), m->arity_in.lo);
        if (m->arity_in.is_range())  std::printf("-%d", m->arity_in.hi);
        std::printf("  out:%d", m->arity_out.lo);
        if (m->arity_out.is_range()) std::printf("-%d", m->arity_out.hi);
        std::printf("  [%zu statements]\n",
                    m->body ? m->body->count() : std::size_t{0});
        dump_stmts(m->body, 2);
    }

    const std::size_t top_count = body ? body->count() : 0;
    std::printf("\n-- Top-level statements (%zu) --\n", top_count);
    dump_stmts(body, 1);
    std::printf("\n");
}