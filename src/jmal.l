%{

#include <parser.hpp>
#include <string>
#include <stdexcept>

int yyline = 1;
int yycol  = 1;

#undef  YY_DECL
#define YY_DECL yy::parser::symbol_type yylex()

static std::string unescape_string(const char *raw)
{
    /* raw is  "..."  — skip opening quote, stop before closing quote */
    std::string result;
    const char *p = raw + 1;
    while (*p && !(p[0] == '"' && p[-1] != '\\')) {
        if (*p == '\\') {
            ++p;
            switch (*p) {
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"':  result += '"';  break;
                default:   result += '\\'; result += *p; break;
            }
        } else {
            result += *p;
        }
        ++p;
    }
    return result;
}

%}

%option noyywrap
%option noinput nounput
%option yylineno
%option warn
%option nodefault

HEXDIGIT    [0-9a-fA-F]
IDENT_START [a-zA-Z_]
IDENT_CONT  [a-zA-Z0-9_]
IDENT       {IDENT_START}{IDENT_CONT}*
DEC_INT     [0-9]+
FLOAT       [0-9]+\.[0-9]+([eE][+-]?[0-9]+)?
STRING      \"([^\"\n\\]|\\.)*\"
ARG_REF     "%"[0-9]+
REF_ARG     "&"[0-9]+

%%

[ \t\r]+            { /* skip horizontal whitespace */ }
\n                  { return yy::parser::make_TOK_NEWLINE();  }
;[^\n]*             { /* line comment – discard */ }

"%redirect"         { return yy::parser::make_DIR_REDIRECT();       }
"%ensure"           { return yy::parser::make_DIR_ENSURE();         }
"%if"               { return yy::parser::make_DIR_IF();             }
"%endif"            { return yy::parser::make_DIR_ENDIF();          }
"%literal"          { return yy::parser::make_DIR_LITERAL();        }
"%endliteral"       { return yy::parser::make_DIR_ENDLITERAL();     }
"%define"           { return yy::parser::make_DIR_DEFINE();         }
"%undef"            { return yy::parser::make_DIR_UNDEF();          }
"%type"             { return yy::parser::make_DIR_TYPE();           }
"%use"              { return yy::parser::make_DIR_USE();            }
"%macro"            { return yy::parser::make_DIR_MACRO();          }
"%endmacro"         { return yy::parser::make_DIR_ENDMACRO();       }
"%arg"              { return yy::parser::make_DIR_ARG();            }
"%ref"              { return yy::parser::make_DIR_REF();            }
"%rep"              { return yy::parser::make_DIR_REP();            }
"%endrep"           { return yy::parser::make_DIR_ENDREP();         }
"%rotate"           { return yy::parser::make_DIR_ROTATE();         }
"%0"                { return yy::parser::make_DIR_ARG_COUNT();      }

{ARG_REF}           { return yy::parser::make_TOK_ARG_REF(
                          static_cast<unsigned int>(std::stoul(yytext + 1))); }

{REF_ARG}           { return yy::parser::make_TOK_REF_ARG(
                          std::stoi(yytext + 1)); }

"register"          { return yy::parser::make_TYPE_REGISTER();  }
"string"            { return yy::parser::make_TYPE_STRING();    }
"number"            { return yy::parser::make_TYPE_NUMBER();    }
"address"           { return yy::parser::make_TYPE_ADDRESS();   }

"regex"             { return yy::parser::make_TOK_REGEX_PREFIX(); }
"str"               { return yy::parser::make_TOK_STR_PREFIX();   }
"int"               { return yy::parser::make_TOK_INT_PREFIX();   }

    /* ── Ranged arity: must be matched before plain DEC_INT ── */
{DEC_INT}"-"{DEC_INT} {
                        /* std::string avoids raw pointer arithmetic */
                        std::string text(yytext);
                        auto dash = text.find('-');
                        int lo = std::stoi(text.substr(0, dash));
                        int hi = std::stoi(text.substr(dash + 1));
                        return yy::parser::make_TOK_RANGE(JmalArity{lo, hi});
                      }

{DEC_INT}             { return yy::parser::make_TOK_INT(std::stoi(yytext));   }

{FLOAT}               { return yy::parser::make_TOK_FLOAT(std::stod(yytext)); }

    /* Strip surrounding quotes and process escapes */
{STRING}              { return yy::parser::make_TOK_STRING(unescape_string(yytext)); }

{IDENT}               { return yy::parser::make_TOK_IDENT(std::string(yytext)); }

","                 { return yy::parser::make_TOK_COMMA();           }
":"                 { return yy::parser::make_TOK_COLON();           }
"="                 { return yy::parser::make_TOK_EQUAL();           }
"=="                { return yy::parser::make_TOK_COMPARE();         }
">="                { return yy::parser::make_TOK_COMPARE_GREATER(); }
"!="                { return yy::parser::make_TOK_COMPARE_NOT();     }
"|"                 { return yy::parser::make_TOK_PIPE();            }
"["                 { return yy::parser::make_TOK_LBRACKET();        }
"]"                 { return yy::parser::make_TOK_RBRACKET();        }
"("                 { return yy::parser::make_TOK_LPAREN();          }
")"                 { return yy::parser::make_TOK_RPAREN();          }
"+"                 { return yy::parser::make_TOK_PLUS();            }
"-"                 { return yy::parser::make_TOK_MINUS();           }
"*"                 { return yy::parser::make_TOK_STAR();            }
"/"                 { return yy::parser::make_TOK_SLASH();           }

    /* ────────────────────────────────────────────────────────── *
     *  Catch-all: illegal character                             *
     * ────────────────────────────────────────────────────────── */

.                   {
                        fprintf(stderr,
                            "jmal: unexpected character '%s' on line %d\n",
                            yytext, yylineno);
                    }

%%
