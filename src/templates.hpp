#pragma once

#include <cassert>
#include <cstdio>
#include <memory>
#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>
#include <string>

// ─── Diagnostic helper ───────────────────────────────────────────────────────

#ifdef ASSERT_TODO
#  define JMAL_TODO(msg)                                            \
    do {                                                            \
        std::fprintf(stderr, "TODO: %s  (%s:%d)\n",                 \
                     (msg), __FILE__, __LINE__);                    \
        assert(false);                                              \
    } while (false)
#else
#  define JMAL_TODO(msg)                                            \
    do {                                                            \
        std::fprintf(stderr, "Warning: TODO: %s  (%s:%d)\n",        \
                     (msg), __FILE__, __LINE__);                    \
    } while (false)
#endif

// ─── JmalArity ───────────────────────────────────────────────────────────────

struct JmalArity {
    int lo;
    int hi;

    static JmalArity fixed(int n)            { return {n, n};     }
    static JmalArity range(int lo_, int hi_) { return {lo_, hi_}; }
    bool is_range() const                    { return lo != hi;   }

    friend std::ostream& operator<<(std::ostream& os, const JmalArity& a) {
        if (a.is_range()) return os << a.lo << "-" << a.hi;
        return os << a.lo;
    }
};

inline JmalArity jmal_arity_fixed(int n)          { return JmalArity::fixed(n);      }
inline JmalArity jmal_arity_range(int lo, int hi) { return JmalArity::range(lo, hi); }

// ─── JmalTypeConstraint ──────────────────────────────────────────────────────

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
    std::monostate,  // builtins
    int,             // LitInt, ArgRef
    std::string      // User
>;

struct JmalTypeConstraint {
    JmalTypeKind    kind;
    JmalTypePayload payload;

    const std::string& name()  const { return std::get<std::string>(payload); }
    int                value() const { return std::get<int>(payload);         }

    static JmalTypeConstraint builtin(JmalTypeKind k)               { return {k, std::monostate{}}; }
    static JmalTypeConstraint arg_ref(unsigned int ref)             { return {JmalTypeKind::ArgRef,  static_cast<int>(ref)}; }
    static JmalTypeConstraint lit_int(int v)                        { return {JmalTypeKind::LitInt,  v}; }
    static JmalTypeConstraint user(const std::string& name)         { return {JmalTypeKind::User,    name}; }

    friend std::ostream& operator<<(std::ostream& os, const JmalTypeConstraint& t) {
        switch (t.kind) {
            case JmalTypeKind::BuiltinRegister: return os << "register";
            case JmalTypeKind::BuiltinString:   return os << "string";
            case JmalTypeKind::BuiltinNumber:   return os << "number";
            case JmalTypeKind::BuiltinAddress:  return os << "address";
            case JmalTypeKind::LitInt:          return os << t.value();
            case JmalTypeKind::ArgRef:          return os << "%" << t.value();
            case JmalTypeKind::User:            return os << t.name();
        }
        return os;
    }
};

using JmalTypeList = std::vector<JmalTypeConstraint>;

inline std::ostream& operator<<(std::ostream& os, const JmalTypeList& v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i > 0) os << " | ";
        os << v[i];
    }
    return os;
}

// Bison call-site helpers
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_REGISTER = JmalTypeKind::BuiltinRegister;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_STRING   = JmalTypeKind::BuiltinString;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_NUMBER   = JmalTypeKind::BuiltinNumber;
inline constexpr JmalTypeKind JMAL_TYPE_BUILTIN_ADDRESS  = JmalTypeKind::BuiltinAddress;

// ─── JmalOperand ─────────────────────────────────────────────────────────────

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
    std::string,              // Ident, String
    int,                      // Int, ArgRef
    double,                   // Float
    std::shared_ptr<JmalOperand>  // Address (owned)
>;

struct JmalOperand {
    JmalOperandKind    kind;
    JmalOperandPayload payload;

    const std::string& sval()  const { return std::get<std::string>(payload);              }
    int                ival()  const { return std::get<int>(payload);                       }
    double             fval()  const { return std::get<double>(payload);                    }
    const JmalOperand& inner() const { return *std::get<std::shared_ptr<JmalOperand>>(payload); }

    static JmalOperand ident  (const std::string& s) { return {JmalOperandKind::Ident,   s}; }
    static JmalOperand integer(int v)                 { return {JmalOperandKind::Int,     v}; }
    static JmalOperand fp     (double v)              { return {JmalOperandKind::Float,   v}; }
    static JmalOperand string (const std::string& s)  { return {JmalOperandKind::String,  s}; }
    static JmalOperand arg_ref(int idx)               { return {JmalOperandKind::ArgRef,  idx}; }
    static JmalOperand address(JmalOperand inner)     {
        return {JmalOperandKind::Address, std::make_shared<JmalOperand>(std::move(inner))};
    }

    friend std::ostream& operator<<(std::ostream& os, const JmalOperand& o) {
        switch (o.kind) {
            case JmalOperandKind::Ident:   return os << o.sval();
            case JmalOperandKind::Int:     return os << o.ival();
            case JmalOperandKind::Float:   return os << o.fval();
            case JmalOperandKind::String:  return os << '"' << o.sval() << '"';
            case JmalOperandKind::ArgRef:  return os << "%" << o.ival();
            case JmalOperandKind::Address: return os << "[" << o.inner() << "]";
        }
        return os;
    }
};

using JmalOperandList = std::vector<JmalOperand>;

inline std::ostream& operator<<(std::ostream& os, const JmalOperandList& v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i > 0) os << ", ";
        os << v[i];
    }
    return os;
}

// ─── JmalDefine ──────────────────────────────────────────────────────────────

using JmalDefineValue = std::variant<std::string, int, double>;
enum class JmalDefineKind { String, Int, Float };

struct JmalDefine {
    std::string     name;
    JmalDefineKind  kind;
    JmalDefineValue value;

    const std::string& str_val() const { return std::get<std::string>(value); }
    int                int_val() const { return std::get<int>(value);          }
    double             flt_val() const { return std::get<double>(value);       }

    static JmalDefine make_str  (const std::string& name, const std::string& val)
        { return {name, JmalDefineKind::String, val}; }
    static JmalDefine make_int  (const std::string& name, int val)
        { return {name, JmalDefineKind::Int,    val}; }
    static JmalDefine make_float(const std::string& name, double val)
        { return {name, JmalDefineKind::Float,  val}; }
};

// ─── Flags ───────────────────────────────────────────────────────────────────

#define HAS_HEADER   true
#define NO_HEADER    false
#define HAS_CHILDREN true
#define NO_CHILDREN  false

// ─── Type enum (auto-generated from definitions.hpp) ─────────────────────────

enum class Type {
#define STATEMENT_TYPE(Name, ...)        Name,
#define STATEMENT_TYPE_CUSTOM(Name, ...) Name,
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include "definitions.hpp"
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE
    Error,
};

constexpr std::string_view type_name(Type t) {
    switch (t) {
#define STATEMENT_TYPE(Name, ...)        case Type::Name: return #Name;
#define STATEMENT_TYPE_CUSTOM(Name, ...) case Type::Name: return #Name;
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include "definitions.hpp"
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE
        case Type::Error: return "Error";
    }
    return "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << type_name(type);
}

// ─── Traits ──────────────────────────────────────────────────────────────────

template <Type T> struct has_header   : std::false_type {};
template <Type T> struct has_children : std::false_type {};

// ─── Statement template ──────────────────────────────────────────────────────

struct AnyStatement;

template <Type T, typename Header = std::monostate>
struct Statement {
    static_assert(T != Type::Error, "Cannot instantiate Statement with Type::Error");
    static_assert(
        !has_header<T>::value || !std::is_same_v<Header, std::monostate>,
        "Statement declared HAS_HEADER but Header type is std::monostate"
    );
    static_assert(
        has_header<T>::value || std::is_same_v<Header, std::monostate>,
        "Statement declared NO_HEADER but a Header type was provided"
    );

    static constexpr Type type = T;

    [[no_unique_address]]
    std::conditional_t<has_header<T>::value, Header, std::monostate> header;

    [[no_unique_address]]
    std::conditional_t<has_children<T>::value,
        std::vector<AnyStatement>,
        std::monostate
    > children;

    void display(int indent = 0) const {
        std::string pad(indent * 2, ' ');
        std::cout << pad << type;

        if constexpr (has_header<T>::value)
            std::cout << " [" << header << "]";

        std::cout << "\n";

        if constexpr (has_children<T>::value)
            for (const auto& child : children)
                child.display(indent + 1);
    }
};

// ─── Pass 1: generate custom header structs ───────────────────────────────────

#define STATEMENT_TYPE(...)
#define STATEMENT_TYPE_CUSTOM(Name, WithHeader, WithChildren, PrintExpr) \
    struct Name##Header {                                                 \
        static std::ostream& print(std::ostream& os, const Name##Header& h) { \
            return os << PrintExpr;                                       \
        }
#define HEADER_FIELD(FieldType, Field) \
        FieldType Field;
#define END_STATEMENT_TYPE(Name)                                                \
    friend std::ostream& operator<<(std::ostream& os, const Name##Header& h) { \
        return Name##Header::print(os, h);                                      \
    }                                                                           \
};
#include "definitions.hpp"
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE

// ─── Pass 2: specialize traits + declare aliases ──────────────────────────────

#define STATEMENT_TYPE(Name, WithHeader, WithChildren, HeaderType)              \
    template <> struct has_header<Type::Name>                                   \
        : std::bool_constant<WithHeader>  {};                                   \
    template <> struct has_children<Type::Name>                                 \
        : std::bool_constant<WithChildren>{};                                   \
    using Name##Statement = Statement<Type::Name,                               \
        std::conditional_t<WithHeader, HeaderType, std::monostate>>;

#define STATEMENT_TYPE_CUSTOM(Name, WithHeader, WithChildren, ...)              \
    template <> struct has_header<Type::Name>                                   \
        : std::bool_constant<WithHeader>  {};                                   \
    template <> struct has_children<Type::Name>                                 \
        : std::bool_constant<WithChildren>{};                                   \
    using Name##Statement = Statement<Type::Name, Name##Header>;

#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include "definitions.hpp"
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE

// ─── Pass 3: build AnyStatement variant ──────────────────────────────────────

#define STATEMENT_TYPE(Name, ...)        Name##Statement,
#define STATEMENT_TYPE_CUSTOM(Name, ...) Name##Statement,
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
struct AnyStatement : std::variant<
#include "definitions.hpp"
    std::monostate  // trailing sentinel to avoid dangling comma
> {
    using variant::variant;

    void display(int indent = 0) const {
        std::visit([indent](const auto& s) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(s)>, std::monostate>)
                s.display(indent);
        }, *this);
    }
};
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE

// ─── JmalProgram ─────────────────────────────────────────────────────────────

struct JmalProgram {
    std::vector<JmalDefine>      defines;
    std::vector<MacroStatement>  macros;
    std::vector<AnyStatement>    body;
    std::string                  filename;

    explicit JmalProgram(const std::string& filename) : filename(filename) {}

    void add_define(JmalDefine d)       { defines.push_back(std::move(d));  }
    void add_macro (MacroStatement m)   { macros .push_back(std::move(m));  }
    void add_stmt  (AnyStatement s)     { body   .push_back(std::move(s));  }

    void dump() const {
        std::cout << "Program: " << filename << "\n";
        for (const auto& d : defines)
            std::cout << "  define: " << d.name << "\n";
        for (const auto& m : macros)
            m.display(1);
        for (const auto& s : body)
            s.display(1);
    }
};