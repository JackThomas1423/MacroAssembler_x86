STATEMENT_TYPE(Literal, NO_HEADER, HAS_CHILDREN, void)

STATEMENT_TYPE_CUSTOM(Define, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string, name)
    HEADER_FIELD(JmalTypeConstraint, item)
END_STATEMENT_TYPE(Define)

// %use <name> [args...]
STATEMENT_TYPE_CUSTOM(Use, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string,   name)
    HEADER_FIELD(JmalTypeList,  arguments)
END_STATEMENT_TYPE(Use)

// %macro <name> <in_arity> <out_arity> ... %endmacro
STATEMENT_TYPE_CUSTOM(Macro, HAS_HEADER, HAS_CHILDREN)
    HEADER_FIELD(std::string, name)
    HEADER_FIELD(JmalArity,   arity_in)
    HEADER_FIELD(JmalArity,   arity_out)
END_STATEMENT_TYPE(Macro)

// %arg %N : type | type | ...
STATEMENT_TYPE_CUSTOM(Arg, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(unsigned int,  param)
    HEADER_FIELD(JmalTypeList,  type_constraints)
END_STATEMENT_TYPE(Arg)

// %ref &N : type | type | ...
STATEMENT_TYPE_CUSTOM(Ref, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(int,           param)
    HEADER_FIELD(JmalTypeList,  type_constraints)
END_STATEMENT_TYPE(Ref)

// %ensure <lhs> <op> <rhs>
STATEMENT_TYPE_CUSTOM(Ensure, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string, lhs)
    HEADER_FIELD(std::string, op)
    HEADER_FIELD(std::string, rhs)
END_STATEMENT_TYPE(Ensure)

// %rep <count|cond> ... %endrep
STATEMENT_TYPE_CUSTOM(Rep, HAS_HEADER, HAS_CHILDREN)
    HEADER_FIELD(JmalTypeConstraint, condition)
END_STATEMENT_TYPE(Rep)

// %if <cond> ... %endif
STATEMENT_TYPE_CUSTOM(If, HAS_HEADER, HAS_CHILDREN)
    HEADER_FIELD(std::string, lhs)
    HEADER_FIELD(std::string, op)
    HEADER_FIELD(std::string, rhs)
END_STATEMENT_TYPE(If)

// %rotate <N|%N>
STATEMENT_TYPE_CUSTOM(Rotate, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string, amount)
END_STATEMENT_TYPE(Rotate)

// %type <name>: type | type | ...
STATEMENT_TYPE_CUSTOM(Type, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string,   name)
    HEADER_FIELD(JmalTypeList,  constraints)
END_STATEMENT_TYPE(Type)

// <opcode> [operands]
STATEMENT_TYPE_CUSTOM(Instruction, HAS_HEADER, NO_CHILDREN)
    HEADER_FIELD(std::string,     opcode)
    HEADER_FIELD(JmalOperandList, operands)
END_STATEMENT_TYPE(Instruction)