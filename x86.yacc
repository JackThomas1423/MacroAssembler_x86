%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/globals.h"
#include "src/macros.h"

int yylex(void);
void yyerror(const char *s);
extern int lineno;
extern char *flush_line(void);

%}

#include "grammar/tokens.yh"

%start program

%%

#include "grammar/program.yg"
#include "grammar/function.yg"
#include "grammar/directives.yg"
#include "grammar/instructions.yg"
#include "grammar/operands.yg"
#include "grammar/macros.yg"
#include "grammar/registers.yg"
#include "grammar/conditionals.yg"

%%
