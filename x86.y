%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/globals.h"
#include "src/macros.h"
#include "src/function.h"

extern int yylex(void);
void yyerror(const char *s);
extern int lineno;
extern char *flush_line(void);

%}

#include "grammar/tokens.y"

%start program

%%

#include "grammar/program.y"
#include "grammar/function.y"
#include "grammar/directives.y"
#include "grammar/instructions.y"
#include "grammar/operands.y"
#include "grammar/macros.y"
#include "grammar/registers.y"
#include "grammar/conditionals.y"

%%
