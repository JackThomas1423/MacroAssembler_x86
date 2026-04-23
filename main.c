#include <stdio.h>
#include <stdlib.h>
#include "build/jmal.tab.h"
#include "jmal_ast.h"

extern FILE *yyin;

JmalProgram *jmal_program = NULL;

int main(int argc, char **argv)
{
    const char *filename = argc > 1 ? argv[1] : NULL;

    jmal_program = jmal_program_new(filename);

    if (filename) {
        FILE *f = fopen(filename, "r");
        if (!f) {
            perror(filename);
            jmal_program_free(jmal_program);
            return 1;
        }
        yyin = f;
    }

    yyparse();

    if (yyin && yyin != stdin)
        fclose(yyin);

    jmal_program_dump(jmal_program);

    jmal_program_free(jmal_program);

    return 0;
}