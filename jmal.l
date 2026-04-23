%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jmal.tab.h"

static char *strip_quotes(const char *s, int len)
{
    /* s points at the opening '"', len includes both quotes */
    char *buf = malloc(len - 1);          /* len-2 chars + NUL */
    memcpy(buf, s + 1, len - 2);
    buf[len - 2] = '\0';
    return buf;
}

/* Current source location – update as you read tokens. */
int yyline = 1;
int yycol  = 1;

%}

/* ── flex options ───────────────────────────────────────────── */
%option noyywrap
%option noinput nounput
%option yylineno
%option warn
%option nodefault

/* ── named character classes ────────────────────────────────── */
DIGIT       [0-9]
HEXDIGIT    [0-9a-fA-F]
ALPHA       [a-zA-Z]
ALNUM       [a-zA-Z0-9]
IDENT_START [a-zA-Z_]
IDENT_CONT  [a-zA-Z0-9_]

/* ── token fragments ─────────────────────────────────────────── */
IDENT       {IDENT_START}{IDENT_CONT}*

/* Numbers */
DEC_INT     [0-9]+
HEX_INT     0[xX]{HEXDIGIT}+
OCT_INT     0[oO][0-7]+
BIN_INT     0[bB][01]+
FLOAT       [0-9]+\.[0-9]+([eE][+-]?[0-9]+)?

/* String literal (no escape sequences – extend if needed) */
STRING      \"([^\"\n\\]|\\.)*\"

/* arg placeholder: %1 .. %9 and %0 (arg count) */
ARG_REF     "%"[0-9]+

%%

    /* ────────────────────────────────────────────────────────── *
     *  Whitespace & comments                                     *
     * ────────────────────────────────────────────────────────── */

[ \t\r]+            { /* skip horizontal whitespace */ }
\n                  { return TOK_NEWLINE; }
;[^\n]*             { /* line comment – discard */ }


    /* ────────────────────────────────────────────────────────── *
     *  Directives  (%keyword)                                    *
     * ────────────────────────────────────────────────────────── */

"%define.table"     { return DIR_DEFINE_TABLE;  }
"%define"           { return DIR_DEFINE;         }
"%undef"            { return DIR_UNDEF;          }
"%type"             { return DIR_TYPE;           }
"%use"              { return DIR_USE;            }
"%macro.strict"     { return DIR_MACRO_STRICT;   }
"%macro"            { return DIR_MACRO;          }
"%endmacro"         { return DIR_ENDMACRO;       }
"%arg"              { return DIR_ARG;            }
"%rep"              { return DIR_REP;            }
"%endrep"           { return DIR_ENDREP;         }
"%rotate"           { return DIR_ROTATE;         }
"%0"                { return DIR_ARG_COUNT;      }  /* total arg count */

    /* ────────────────────────────────────────────────────────── *
     *  Arg references  %1 %2 … %9                               *
     * ────────────────────────────────────────────────────────── */

{ARG_REF}           {
                        yylval.ival = atoi(yytext + 1);
                        return TOK_ARG_REF;
                    }

    /* ────────────────────────────────────────────────────────── *
     *  Keywords inside directives                               *
     * ────────────────────────────────────────────────────────── */

"register"          { return TYPE_REGISTER;  }
"string"            { return TYPE_STRING;    }
"number"            { return TYPE_NUMBER;    }
"address"           { return TYPE_ADDRESS;   }

    /* ────────────────────────────────────────────────────────── *
     *  Numeric literals                                         *
     * ────────────────────────────────────────────────────────── */

{HEX_INT}           { yylval.ival = (int)strtol(yytext + 2, NULL, 16);
                      return TOK_INT;  }

{OCT_INT}           { yylval.ival = (int)strtol(yytext + 2, NULL, 8);
                      return TOK_INT;  }

{BIN_INT}           { yylval.ival = (int)strtol(yytext + 2, NULL, 2);
                      return TOK_INT;  }

{DEC_INT}           { yylval.ival = atoi(yytext);
                      return TOK_INT;  }

{FLOAT}             { yylval.fval = atof(yytext);
                      return TOK_FLOAT; }

    /* ────────────────────────────────────────────────────────── *
     *  String literals                                          *
     * ────────────────────────────────────────────────────────── */

{STRING}            { yylval.sval = strip_quotes(yytext, yyleng);
                      return TOK_STRING; }

    /* ────────────────────────────────────────────────────────── *
     *  Identifiers                                              *
     * ────────────────────────────────────────────────────────── */

{IDENT}             { yylval.sval = strdup(yytext);
                      return TOK_IDENT;  }

    /* ────────────────────────────────────────────────────────── *
     *  Arity range:  N-M   e.g. 1-3                            *
     *  Must come BEFORE single '-' so "1-3" is one token.      *
     * ────────────────────────────────────────────────────────── */

{DEC_INT}"-"{DEC_INT}   {
                            /* Split on '-' */
                            char *dash = strchr(yytext, '-');
                            yylval.range.lo = atoi(yytext);
                            yylval.range.hi = atoi(dash + 1);
                            return TOK_ARITY_RANGE;
                        }

    /* ────────────────────────────────────────────────────────── *
     *  Punctuation & operators                                  *
     * ────────────────────────────────────────────────────────── */

","                 { return TOK_COMMA;    }
":"                 { return TOK_COLON;    }
"|"                 { return TOK_PIPE;     }
"["                 { return TOK_LBRACKET; }
"]"                 { return TOK_RBRACKET; }
"("                 { return TOK_LPAREN;   }
")"                 { return TOK_RPAREN;   }
"+"                 { return TOK_PLUS;     }
"-"                 { return TOK_MINUS;    }
"*"                 { return TOK_STAR;     }
"/"                 { return TOK_SLASH;    }

    /* ────────────────────────────────────────────────────────── *
     *  Catch-all: illegal character                             *
     * ────────────────────────────────────────────────────────── */

.                   {
                        fprintf(stderr,
                            "jmal: unexpected character '%s' on line %d\n",
                            yytext, yylineno);
                    }

%%