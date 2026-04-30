#pragma once

#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>
#include <string>
#include <optional>

// ─── Flags ───────────────────────────────────────────────────────────────────

#define HAS_HEADER   true
#define NO_HEADER    false
#define HAS_CHILDREN true
#define NO_CHILDREN  false

#define DEFINITIONS "definitions.hpp"

// ─── Type enum (auto-generated from statements.def) ──────────────────────────

enum class Type {
#define STATEMENT_TYPE(Name, ...)        Name,
#define STATEMENT_TYPE_CUSTOM(Name, ...) Name,
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include DEFINITIONS
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE
    Error,
};

#define STATEMENT_TYPE(Name, ...)        static_assert(Type::Name != Type::Error, #Name " collides with reserved Type::Error");
#define STATEMENT_TYPE_CUSTOM(Name, ...) static_assert(Type::Name != Type::Error, #Name " collides with reserved Type::Error");
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include DEFINITIONS
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE

constexpr std::string_view type_name(Type t) {
    switch (t) {
#define STATEMENT_TYPE(Name, ...)        case Type::Name: return #Name;
#define STATEMENT_TYPE_CUSTOM(Name, ...) case Type::Name: return #Name;
#define HEADER_FIELD(...)
#define END_STATEMENT_TYPE(...)
#include DEFINITIONS
#undef STATEMENT_TYPE
#undef STATEMENT_TYPE_CUSTOM
#undef HEADER_FIELD
#undef END_STATEMENT_TYPE
        case Type::Error: return "Error";
    }
    return "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) os << " | ";
        os << v[i];
    }
    return os;
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
    static_assert(!has_header<T>::value || !std::is_same_v<Header, std::monostate>, "Statement declared HAS_HEADER but Header type is std::monostate");

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
#define STATEMENT_TYPE_CUSTOM(Name, WithHeader, WithChildren, PrintExpr)        \
    struct Name##Header {                                                        \
        static std::ostream& print(std::ostream& os, const Name##Header& h) {  \
            return os << PrintExpr;                                             \
        }
#define HEADER_FIELD(FieldType, Field) \
        FieldType Field;
#define END_STATEMENT_TYPE(Name)                                                \
    friend std::ostream& operator<<(std::ostream& os, const Name##Header& h) { \
        return Name##Header::print(os, h);                                      \
    }                                                                           \
};
#include DEFINITIONS
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
#include DEFINITIONS
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
#include DEFINITIONS
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


// example for integration later

/*int main() {
    MacroStatement ms;
    ms.header = { .name = "equ", .min_args = 3, .max_args = 1 };
    ms.children.push_back(ArgStatement{ .header = { .param = "%1", .type_constraint = "number" } });
    ms.children.push_back(ArgStatement{ .header = { .param = "%2", .type_constraint = "func"   } });
    ms.children.push_back(ArgStatement{ .header = { .param = "%3", .type_constraint = "number" } });
    ms.children.push_back(
        LiteralStatement {
            .children = {
                UseStatement{ .header = { .name = "solve", .arguments = "%1 %2 %3" } },
            },
        }
    );

    ms.display();
    return 0;
}*/