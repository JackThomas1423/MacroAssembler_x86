#include <stdio.h>
#include <stdlib.h>
#include "build/jmal.tab.h"

extern FILE *yyin;

int main(int argc, char **argv)
{
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            perror(argv[1]);
            return 1;
        }
        yyin = f;
    }

    int result = yyparse();   /* 0 = success, 1 = parse error */

    if (yyin && yyin != stdin)
        fclose(yyin);

    return result;
}
