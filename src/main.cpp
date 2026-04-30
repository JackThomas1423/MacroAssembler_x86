#include <stdio.h>
#include <parser.hpp>
#include <src/templates.hpp>

using namespace yy;

extern "C" FILE *yyin;

JmalProgram *jmal_program = nullptr;

int main(int argc, char **argv)
{
    const char *filename = argc > 1 ? argv[1] : nullptr;

    jmal_program = new JmalProgram(filename ? filename : "<stdin>");

    if (filename) {
        FILE *f = fopen(filename, "r");
        if (!f) {
            perror(filename);
            delete jmal_program;
            return 1;
        }
        yyin = f;
    }
    std::cout << "Parsing..." << std::endl;

    yy::parser p;
    p.parse();

    if (yyin && yyin != stdin)
        fclose(yyin);

    std::cout << "Dumping program contents..." << std::endl;
    jmal_program->dump();

    delete jmal_program;

    return 0;
}