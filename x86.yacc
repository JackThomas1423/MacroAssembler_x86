%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/globals.h"
#include "src/macros.h"
#include "src/function.h"

int yylex(void);
void yyerror(const char *s);
extern int lineno;
extern char *flush_line(void);

%}

#include "grammar/tokens.yacc"

%start program

%%

#include "grammar/program.yacc"
#include "grammar/function.yacc"
#include "grammar/directives.yacc"
#include "grammar/instructions.yacc"
#include "grammar/operands.yacc"
#include "grammar/macros.yacc"
#include "grammar/registers.yacc"
#include "grammar/conditionals.yacc"

%%
