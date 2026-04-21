macro:
    EXIT NUMBER
    { emit_exit($2); }

    | PRINT math_expr
    { emit_print($2, false); }

    | PRINT math_expr AS PTR
    { emit_print($2, true); }

    | IF conditional GOTO LABEL
    { emit_if($2, $4); }

    | WHILE conditional GOTO LABEL
    { emit_while_goto($2, $4); }

    | WHILE conditional REPEAT SINGLE LBRACKET instruction RBRACKET
    { emit_while_single($2, $6); }

    | SWAP size_prefix
    { emit_swap($2); }

    | MEM NUMBER AS BYTE
    { emit_mem($2); }

    | FREE NUMBER
    { emit_free($2); }

    | DUP AS size_prefix
    { emit_dup($3); }

    | DUP AS PTR
    { emit_dup("PTR"); }

    | GLOBAL LABEL COLON complex_literal_raw
    { printf("%s db %s\n", $2, $4); }

    | push_macro_variations

    | pop_macro_variations

    ;

push_macro_variations:
    PUSH REVERSE complex_literal AS size_prefix
    { emit_push_literal($3, $5, 0, true); }

    | PUSH complex_literal AS size_prefix
    { emit_push_literal($2, $4, 0, false); }

    | PUSH REVERSE complex_literal AS size_prefix PAD NUMBER
    { emit_push_literal($3, $5, $7, true); }

    | PUSH complex_literal AS size_prefix PAD NUMBER
    { emit_push_literal($2, $4, $6, false); }

    | PUSH LABEL AS PTR
    { emit_push_stack_local($2); }
    ;

pop_macro_variations:
    POP register AS size_prefix
    { emit_pop_to_register($2, $4); }

    | POP size_prefix
    { emit_pop_bytes($2); }

    | POP register
    { printf("    pop %s\n", $2); }
    ;