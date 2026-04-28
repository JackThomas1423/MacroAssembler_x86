#include <stdio.h>
#include <stdlib.h>
#include <parser.hpp>
#include <src/jmal_ast.hpp>

using namespace yy;

extern "C" FILE *yyin;

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

    yy::parser p;
    p.parse();

    if (yyin && yyin != stdin)
        fclose(yyin);

    jmal_program_dump(jmal_program);

    jmal_program_free(jmal_program);

    return 0;
}