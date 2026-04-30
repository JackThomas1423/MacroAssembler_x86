// definitions.hpp
// Add all statement types here. Two forms:
//
// Simple (header is a plain type or void):
//   STATEMENT_TYPE(Name, HAS/NO_HEADER, HAS/NO_CHILDREN, HeaderType)
//
// Custom (header struct is auto-generated):
//   STATEMENT_TYPE_CUSTOM(Name, HAS/NO_HEADER, HAS/NO_CHILDREN, PrintExpr)
//       HEADER_FIELD(Type, field_name)
//       ...
//   END_STATEMENT_TYPE(Name)
//
// In PrintExpr, refer to fields as h.field_name

// ─── Simple types ─────────────────────────────────────────────────────────────

STATEMENT_TYPE(Literal, NO_HEADER,  HAS_CHILDREN, void)
STATEMENT_TYPE(Define,  HAS_HEADER, NO_CHILDREN,  std::string)

// ─── Custom types ─────────────────────────────────────────────────────────────

// %use <name> [args]
STATEMENT_TYPE_CUSTOM(Use, HAS_HEADER, NO_CHILDREN,
    h.name)
    HEADER_FIELD(std::string,              name)
    HEADER_FIELD(std::vector<std::string>, arguments)
END_STATEMENT_TYPE(Use)

// %macro <name> <min_args>-<max_args> ... %endmacro
STATEMENT_TYPE_CUSTOM(Macro, HAS_HEADER, HAS_CHILDREN,
    h.name << " " << h.min_args << "-" << h.max_args)
    HEADER_FIELD(std::string, name)
    HEADER_FIELD(int,         min_args)
    HEADER_FIELD(int,         max_args)
END_STATEMENT_TYPE(Macro)

// %arg %N : type | type | ...
STATEMENT_TYPE_CUSTOM(Arg, HAS_HEADER, NO_CHILDREN,
    h.param << " : " << h.type_constraints)
    HEADER_FIELD(std::string,              param)
    HEADER_FIELD(std::vector<std::string>, type_constraints)
END_STATEMENT_TYPE(Arg)

// %ref &N : type | type | ...
STATEMENT_TYPE_CUSTOM(Ref, HAS_HEADER, NO_CHILDREN,
    h.param << " : " << h.type_constraints)
    HEADER_FIELD(std::string,              param)
    HEADER_FIELD(std::vector<std::string>, type_constraints)
END_STATEMENT_TYPE(Ref)

// %ensure <lhs> <op> <rhs>
STATEMENT_TYPE_CUSTOM(Ensure, HAS_HEADER, NO_CHILDREN,
    h.lhs << " " << h.op << " " << h.rhs)
    HEADER_FIELD(std::string, lhs)
    HEADER_FIELD(std::string, op)
    HEADER_FIELD(std::string, rhs)
END_STATEMENT_TYPE(Ensure)

// %rep <count|cond> ... %endrep
STATEMENT_TYPE_CUSTOM(Rep, HAS_HEADER, HAS_CHILDREN,
    h.condition)
    HEADER_FIELD(std::string, condition)
END_STATEMENT_TYPE(Rep)

// %if <cond> ... %endif
STATEMENT_TYPE_CUSTOM(If, HAS_HEADER, HAS_CHILDREN,
    h.lhs << " " << h.op << " " << h.rhs)
    HEADER_FIELD(std::string, lhs)
    HEADER_FIELD(std::string, op)
    HEADER_FIELD(std::string, rhs)
END_STATEMENT_TYPE(If)

// %rotate <N|%N>
STATEMENT_TYPE_CUSTOM(Rotate, HAS_HEADER, NO_CHILDREN,
    h.amount)
    HEADER_FIELD(std::string, amount)
END_STATEMENT_TYPE(Rotate)

// %type <name>: type | type | ...
STATEMENT_TYPE_CUSTOM(Type, HAS_HEADER, NO_CHILDREN,
    h.name << ": " << h.constraints)
    HEADER_FIELD(std::string,              name)
    HEADER_FIELD(std::vector<std::string>, constraints)
END_STATEMENT_TYPE(Type)

// <opcode> [operands]  — a raw instruction inside a macro body or at top level
// Note: operands will need to become a proper variant type once the AST is wired in
STATEMENT_TYPE_CUSTOM(Instruction, HAS_HEADER, NO_CHILDREN,
    h.opcode << " " << h.operands)
    HEADER_FIELD(std::string,              opcode)
    HEADER_FIELD(std::vector<std::string>, operands)
END_STATEMENT_TYPE(Instruction)