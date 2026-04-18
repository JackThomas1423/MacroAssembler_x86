operand:
    register
    | NUMBER                 { snprintf($$, 256, "%d", $1); }
    | MINUS NUMBER           { snprintf($$, 256, "-%d", $2); }
    | NUMBER DIVIDE NUMBER   { snprintf($$, 256, "%d / %d", $1, $3); }
    | NUMBER MULTIPLY NUMBER { snprintf($$, 256, "%d * %d", $1, $3); }
    | LABEL
    | LABEL PLUS NUMBER
    | LABEL MINUS NUMBER
    | LABEL PLUS NUMBER PLUS NUMBER
    | LABEL PLUS NUMBER MINUS NUMBER
    | size_prefix register       { snprintf($$, 256, "%s %s", $1, $2); }
    | size_prefix NUMBER         { snprintf($$, 256, "%s %d", $1, $2); }
    | size_prefix LABEL          { snprintf($$, 256, "%s %s", $1, $2); }
    | memory_operand             { snprintf($$, 256, "[%s]", $1); }
    | size_prefix memory_operand { snprintf($$, 256, "%s [%s]", $1, $2); }
    ;

size_prefix:
    BYTE    { strcpy($$, "BYTE");  }
    | WORD  { strcpy($$, "WORD");  }
    | DWORD { strcpy($$, "DWORD"); }
    | QWORD { strcpy($$, "QWORD"); }
    ;

memory_operand:
    LBRACKET register RBRACKET
    { snprintf($$, 256, "%s", $2); }
    |
    LBRACKET NUMBER RBRACKET
    { snprintf($$, 256, "%d", $2); }
    |
    LBRACKET LABEL RBRACKET
    { snprintf($$, 256, "%s", $2); }
    |
    LBRACKET LABEL PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%d", $2, $4); }
    |
    LBRACKET LABEL MINUS NUMBER RBRACKET
    { snprintf($$, 256, "%s-%d", $2, $4); }
    |
    LBRACKET LABEL PLUS register RBRACKET
    { snprintf($$, 256, "%s+%s", $2, $4); }
    |
    LBRACKET LABEL MINUS register RBRACKET
    { snprintf($$, 256, "%s-%s", $2, $4); }
    |
    LBRACKET LABEL PLUS LABEL RBRACKET
    { snprintf($$, 256, "%s+%s", $2, $4); }
    |
    LBRACKET LABEL PLUS LABEL PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%s+%d", $2, $4, $6); }
    |
    LBRACKET LABEL PLUS LABEL MINUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%s-%d", $2, $4, $6); }
    |
    LBRACKET LABEL PLUS NUMBER PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%d+%d", $2, $4, $6); }
    |
    LBRACKET LABEL PLUS NUMBER MINUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%d-%d", $2, $4, $6); }
    |
    LBRACKET LABEL PLUS NUMBER PLUS LABEL RBRACKET
    { snprintf($$, 256, "%s+%d+%s", $2, $4, $6); }
    |
    LBRACKET LABEL PLUS LABEL PLUS LABEL RBRACKET
    { snprintf($$, 256, "%s+%s+%s", $2, $4, $6); }
    |
    LBRACKET register PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%d", $2, $4); }
    |
    LBRACKET register MINUS NUMBER RBRACKET
    { snprintf($$, 256, "%s-%d", $2, $4); }
    |
    LBRACKET register PLUS register RBRACKET
    { snprintf($$, 256, "%s+%s", $2, $4); }
    |
    LBRACKET register PLUS register MINUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%s-%d", $2, $4, $6); }
    |
    LBRACKET register PLUS register MULTIPLY NUMBER RBRACKET
    { snprintf($$, 256, "%s+%s*%d", $2, $4, $6); }
    |
    LBRACKET register PLUS register MULTIPLY NUMBER PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s+%s*%d+%d", $2, $4, $6, $8); }
    |
    LBRACKET register PLUS LABEL RBRACKET
    { snprintf($$, 256, "%s+%s", $2, $4); }
    |
    LBRACKET register MINUS LABEL RBRACKET
    { snprintf($$, 256, "%s-%s", $2, $4); }
    |
    LBRACKET register MULTIPLY NUMBER PLUS NUMBER RBRACKET
    { snprintf($$, 256, "%s*%d+%d", $2, $4, $6); }
    |
    size_qualifier LBRACKET register RBRACKET
    { strcpy($$, $3); }
    |
    size_qualifier LBRACKET memory_operand RBRACKET
    { strcpy($$, $3); }
    ;

size_qualifier:
    BYTE PTR    { $$ = 1; }
    | WORD PTR  { $$ = 2; }
    | DWORD PTR { $$ = 4; }
    | QWORD PTR { $$ = 8; }
    ;
