function_wrapper:
    FUNC COLON func_wh LBRACKET func_wh func_stack_frame_res_block func_wh RBRACKET
    {
        struct stack_local_chain* slc = $6;
        struct function_stack_locals* fsc = make_function_scope($1, $6);

        unsigned int chain_size = get_local_chain_size(slc);

        printf("%s:\n", $1);
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", chain_size);
    }
    program LEAVE
    {
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");   
    }
    | FUNC COLON func_wh
    {
        printf("%s:\n", $1);
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
    }
    program LEAVE
    {
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");   
    }
    ;

func_wh:
    | func_wh NEWLINE
    ;

func_stack_frame_res_block:
    func_res_member
    { $$ = $1; }
    | func_stack_frame_res_block COMMA func_wh func_stack_frame_res_block
    {
        struct stack_local_chain* a = $1;
        struct stack_local_chain* b = $4;
        a->next = b;
        $$ = a;
    }
    ;

func_res_member:
    LABEL COLON RESB NUMBER
    { $$ = make_local_chain_node($1, $4, NULL); }
    | LABEL COLON RESW NUMBER
    { $$ = make_local_chain_node($1, $4 * 2, NULL); }
    | LABEL COLON RESD NUMBER
    { $$ = make_local_chain_node($1, $4 * 4, NULL); }
    | LABEL COLON RESQ NUMBER
    { $$ = make_local_chain_node($1, $4 * 8, NULL); }
    ;