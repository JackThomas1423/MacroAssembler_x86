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

// %use <name> [args...]
STATEMENT_TYPE_CUSTOM(Use, HAS_HEADER, NO_CHILDREN,
    h.name << " " << h.arguments)
    HEADER_FIELD(std::string,   name)
    HEADER_FIELD(JmalTypeList,  arguments)
END_STATEMENT_TYPE(Use)

// %macro <name> <in_arity> <out_arity> ... %endmacro
STATEMENT_TYPE_CUSTOM(Macro, HAS_HEADER, HAS_CHILDREN,
    h.name << " " << h.arity_in << " " << h.arity_out)
    HEADER_FIELD(std::string, name)
    HEADER_FIELD(JmalArity,   arity_in)
    HEADER_FIELD(JmalArity,   arity_out)
END_STATEMENT_TYPE(Macro)

// %arg %N : type | type | ...
STATEMENT_TYPE_CUSTOM(Arg, HAS_HEADER, NO_CHILDREN,
    h.param << " : " << h.type_constraints)
    HEADER_FIELD(unsigned int,  param)
    HEADER_FIELD(JmalTypeList,  type_constraints)
END_STATEMENT_TYPE(Arg)

// %ref &N : type | type | ...
STATEMENT_TYPE_CUSTOM(Ref, HAS_HEADER, NO_CHILDREN,
    h.param << " : " << h.type_constraints)
    HEADER_FIELD(int,           param)
    HEADER_FIELD(JmalTypeList,  type_constraints)
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
    HEADER_FIELD(JmalTypeConstraint, condition)
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
    HEADER_FIELD(std::string,   name)
    HEADER_FIELD(JmalTypeList,  constraints)
END_STATEMENT_TYPE(Type)

// <opcode> [operands]
STATEMENT_TYPE_CUSTOM(Instruction, HAS_HEADER, NO_CHILDREN,
    h.opcode << " " << h.operands)
    HEADER_FIELD(std::string,     opcode)
    HEADER_FIELD(JmalOperandList, operands)
END_STATEMENT_TYPE(Instruction)