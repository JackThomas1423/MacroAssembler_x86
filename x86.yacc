%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(void);
void yyerror(const char *s);
extern int lineno;
extern char* flush_line(void);

static char last_mnemonic[32] = "";
int loopCreationIndex = 0;

%}

%union {
    int num;
    char str[256];
}

/* Tokens for instructions */
%token MOV MOVL MOVQ ADD SUB INC DEC MUL IMUL DIV IDIV
%token AND OR XOR NOT SHL SHR SAR
%token JMP JE JZ JNE JNZ JL JG JLE JGE JA JB JAE JBE
%token CALL RET PUSH POP
%token CMP TEST LEA NOP CMPSB
%token LOOP LOOPE LOOPNE STOSD STOSB SCASB MOVSB MOVSD MOVZX MOVSX
%token AS IF VAR WHILE GOTO EXIT PRINT REPEAT SINGLE REVERSE LARROW RARROW GEQ LEQ NEQ EQ

/* Directives and keywords */
%token <str> BITS SECTION GLOBAL EXTERN EQU RESB RESW RESD RESQ

/* Tokens for registers - 64-bit */
%token RAX RBX RCX RDX RSI RDI RBP RSP
%token R8 R9 R10 R11 R12 R13 R14 R15

/* Tokens for registers - 32-bit */
%token EAX EBX ECX EDX ESI EDI EBP ESP

/* Tokens for registers - 16-bit */
%token AX BX CX DX SI DI BP SP

/* Tokens for registers - 8-bit */
%token AL AH BL BH CL CH DL DH

/* Size qualifiers */
%token <str> BYTE WORD DWORD QWORD PTR
%token <str> REPE REPNE REP

/* Tokens for literals and punctuation */
%token <num> NUMBER
%token <str> LABEL STRING
%token COMMA COLON NEWLINE
%token <str> LBRACKET RBRACKET LPAREN RPAREN
%token PLUS MINUS MULTIPLY DIVIDE

/* Type declarations for nonterminals */
%type <str> instruction zero_operand_mnem one_operand_mnem two_operand_mnem register label operand memory_operand
%type <str> size_prefix prefix
%type <str> directive
%type <str> cond_op conditional cond_body
%type <num> size_qualifier math_expr opt_reverse

/* Operator precedence */
%left PLUS MINUS
%left MULTIPLY DIVIDE

%start program

%%

opt_newline:
    /* empty */
    | NEWLINE
    ;

program:
    /* empty */
    |
    program instruction opt_newline
    { printf("%s\n", flush_line()); }
    |
    program label opt_newline
    { printf("%s\n", flush_line()); }
    |
    program directive opt_newline
    { printf("%s\n", flush_line()); }
    |
    program macro opt_newline
    { flush_line(); }
    |
    program NEWLINE
    { flush_line(); }
    ;

directive:
    BITS NUMBER
    | LBRACKET BITS NUMBER RBRACKET
    | SECTION LABEL
    | GLOBAL LABEL
    | GLOBAL LABEL COMMA LABEL
    | GLOBAL LABEL COMMA LABEL COMMA LABEL
    | EXTERN LABEL
    | LABEL EQU NUMBER
    | LABEL COLON RESB NUMBER
    | LABEL COLON RESW NUMBER
    | LABEL COLON RESD NUMBER
    | LABEL COLON RESQ NUMBER
    | LABEL RESB NUMBER
    | LABEL RESW NUMBER
    | LABEL RESD NUMBER
    | LABEL RESQ NUMBER
    ;

instruction:
    zero_operand_mnem
    {
        strcpy(last_mnemonic, $1);
        strcpy($$, $1);
    }
    |
    prefix zero_operand_mnem
    {
        strcpy(last_mnemonic, $2);
        snprintf($$, 256, "%s %s", $1, $2);
    }
    |
    one_operand_mnem operand
    {
        strcpy(last_mnemonic, $1);
        snprintf($$, 256, "%s %s", $1, $2);
    }
    |
    one_operand_mnem
    {
        strcpy(last_mnemonic, $1);
        strcpy($$, $1);
    }
    |
    two_operand_mnem operand COMMA operand
    {
        strcpy(last_mnemonic, $1);
        snprintf($$, 256, "%s %s, %s", $1, $2, $4);
    }
    |
    prefix two_operand_mnem operand COMMA operand
    {
        strcpy(last_mnemonic, $2);
        snprintf($$, 256, "%s %s %s, %s", $1, $2, $3, $5);
    }
    ;

prefix:
    REPE
    | REPNE
    | REP
    ;

zero_operand_mnem:
    NOP      { strcpy($$, "nop"); }
    | RET    { strcpy($$, "ret"); }
    | CMPSB  { strcpy($$, "cmpsb"); }
    | LOOPE  { strcpy($$, "loope"); }
    | LOOPNE { strcpy($$, "loopne"); }
    | STOSD  { strcpy($$, "stosd"); }
    | STOSB  { strcpy($$, "stosb"); }
    | SCASB  { strcpy($$, "scasb"); }
    | MOVSB  { strcpy($$, "movsb"); }
    | MOVSD  { strcpy($$, "movsd"); }
    ;

one_operand_mnem:
    PUSH   { strcpy($$, "push"); }
    | POP  { strcpy($$, "pop"); }
    | INC  { strcpy($$, "inc"); }
    | DEC  { strcpy($$, "dec"); }
    | NOT  { strcpy($$, "not"); }
    | MUL  { strcpy($$, "mul"); }
    | DIV  { strcpy($$, "div"); }
    | IDIV { strcpy($$, "idiv"); }
    | JMP  { strcpy($$, "jmp"); }
    | JE   { strcpy($$, "je"); }
    | JZ   { strcpy($$, "jz"); }
    | JNE  { strcpy($$, "jne"); }
    | JNZ  { strcpy($$, "jnz"); }
    | JL   { strcpy($$, "jl"); }
    | JG   { strcpy($$, "jg"); }
    | JLE  { strcpy($$, "jle"); }
    | JGE  { strcpy($$, "jge"); }
    | JA   { strcpy($$, "ja"); }
    | JB   { strcpy($$, "jb"); }
    | JAE  { strcpy($$, "jae"); }
    | JBE  { strcpy($$, "jbe"); }
    | CALL { strcpy($$, "call"); }
    | LOOP { strcpy($$, "loop"); }
    ;

two_operand_mnem:
    MOV     { strcpy($$, "mov"); }
    | MOVL  { strcpy($$, "movl"); }
    | MOVQ  { strcpy($$, "movq"); }
    | ADD   { strcpy($$, "add"); }
    | SUB   { strcpy($$, "sub"); }
    | AND   { strcpy($$, "and"); }
    | OR    { strcpy($$, "or"); }
    | XOR   { strcpy($$, "xor"); }
    | SHL   { strcpy($$, "shl"); }
    | SHR   { strcpy($$, "shr"); }
    | SAR   { strcpy($$, "sar"); }
    | CMP   { strcpy($$, "cmp"); }
    | TEST  { strcpy($$, "test"); }
    | LEA   { strcpy($$, "lea"); }
    | MOVZX { strcpy($$, "movzx"); }
    | MOVSX { strcpy($$, "movsx"); }
    | IMUL  { strcpy($$, "imul"); }
    ;

label:
    LABEL COLON
    { strcpy($$, $1); }
    ;

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
    | memory_operand             { snprintf($$, 256, "[%s]", $1);    }
    | size_prefix memory_operand { snprintf($$, 256, "%s [%s]", $1, $2); }
    ;

size_prefix:
    BYTE
    | WORD
    | DWORD
    | QWORD
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

register:
    RAX  { strcpy($$, "rax"); }
    | RBX  { strcpy($$, "rbx"); }
    | RCX  { strcpy($$, "rcx"); }
    | RDX  { strcpy($$, "rdx"); }
    | RSI  { strcpy($$, "rsi"); }
    | RDI  { strcpy($$, "rdi"); }
    | RBP  { strcpy($$, "rbp"); }
    | RSP  { strcpy($$, "rsp"); }
    | R8   { strcpy($$, "r8"); }
    | R9   { strcpy($$, "r9"); }
    | R10  { strcpy($$, "r10"); }
    | R11  { strcpy($$, "r11"); }
    | R12  { strcpy($$, "r12"); }
    | R13  { strcpy($$, "r13"); }
    | R14  { strcpy($$, "r14"); }
    | R15  { strcpy($$, "r15"); }
    | EAX  { strcpy($$, "eax"); }
    | EBX  { strcpy($$, "ebx"); }
    | ECX  { strcpy($$, "ecx"); }
    | EDX  { strcpy($$, "edx"); }
    | ESI  { strcpy($$, "esi"); }
    | EDI  { strcpy($$, "edi"); }
    | EBP  { strcpy($$, "ebp"); }
    | ESP  { strcpy($$, "esp"); }
    | AX   { strcpy($$, "ax"); }
    | BX   { strcpy($$, "bx"); }
    | CX   { strcpy($$, "cx"); }
    | DX   { strcpy($$, "dx"); }
    | SI   { strcpy($$, "si"); }
    | DI   { strcpy($$, "di"); }
    | BP   { strcpy($$, "bp"); }
    | SP   { strcpy($$, "sp"); }
    | AL   { strcpy($$, "al"); }
    | AH   { strcpy($$, "ah"); }
    | BL   { strcpy($$, "bl"); }
    | BH   { strcpy($$, "bh"); }
    | CL   { strcpy($$, "cl"); }
    | CH   { strcpy($$, "ch"); }
    | DL   { strcpy($$, "dl"); }
    | DH   { strcpy($$, "dh"); }
    ;

macro:
    EXIT NUMBER
    {
        printf("    mov rax, 60\n");
        printf("    mov rdi, %d\n", $2);
        printf("    syscall\n");
    }
    | PRINT math_expr
    {
        printf("    mov rax, 1\n");
        printf("    mov rdi, 1\n");
        printf("    mov rsi, rsp\n");
        printf("    mov rdx, %d\n", $2);
        printf("    syscall\n");
    }
    | IF conditional GOTO LABEL
    {
        char lhs[64], op[8], rhs[64];
        sscanf($2, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
        printf("    cmp %s, %s\n", lhs, rhs);
        printf("    %s %s\n", op, $4);
    }
    | WHILE conditional GOTO LABEL
    {
        char lhs[64], op[8], rhs[64];
        sscanf($2, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
        printf("    cmp %s, %s\n", lhs, rhs);
        printf("    %s %s\n", op, $4);
    }
    | WHILE conditional REPEAT SINGLE LBRACKET instruction RBRACKET
    {
        char lhs[64], op[8], rhs[64];
        sscanf($2, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
        char lableBuffer[256];
        snprintf(lableBuffer, 256, ".whileSingle%d", loopCreationIndex);
        loopCreationIndex += 1;
        printf("%s:\n",lableBuffer);
        printf("    cmp %s, %s\n", lhs, rhs);
        printf("    %s %s\n", op, lableBuffer);
        printf("    %s\n",$6);
    }
    | push_macro
    ;

push_macro:
    PUSH opt_reverse STRING AS BYTE
    {
        if ($2 == 0) {
            for (int i = 0; i < strlen($3); i += 1) printf("    push '%c'\n", $3[i]);
        } else {
            for (int i = strlen($3)-1; i >= 0; i -= 1) printf("    push '%c'\n", $3[i]);
        }
    }
    | PUSH opt_reverse STRING AS WORD
    {
        if ($2 == 0) {
            for (int i = 0; i < strlen($3); i += 2) {
                printf("    push 0x%02X%02X\n", (unsigned char)$3[i], (unsigned char)$3[i+1]);
            }
        } else {
            for (int i = strlen($3)-1; i >= 0; i -= 2) {
                printf("    push 0x%02X%02X\n", (unsigned char)$3[i], (unsigned char)$3[i-1]);
            }
        }
    }
    | PUSH opt_reverse STRING AS DWORD
    {
        if ($2 == 0) {
            for (int i = 0; i < strlen($3); i += 4) {
                printf("    push 0x%02X%02X%02X%02X\n", (unsigned char)$3[i], (unsigned char)$3[i+1],
                                                        (unsigned char)$3[i+2], (unsigned char)$3[i+3]);
            }
        } else {
            for (int i = strlen($3)-1; i >= 0; i -= 4) {
                printf("    push 0x%02X%02X%02X%02X\n", (unsigned char)$3[i], (unsigned char)$3[i-1],
                                                        (unsigned char)$3[i-2], (unsigned char)$3[i-3]);
            }
        }
    }
    | PUSH STRING AS QWORD
    {
        int len = strlen($2);
        int paddedSize = (len % 8 == 0) ? (len / 8) : (len / 8) + 1;
        int totalBytes = paddedSize * 8;
        printf("    sub rsp, %d\n", totalBytes);
        
        for (int i = 0; i < totalBytes; i += 4) {
            unsigned char b0 = (i < len) ? (unsigned char)$2[i] : 0;
            unsigned char b1 = (i+1 < len) ? (unsigned char)$2[i+1] : 0;
            unsigned char b2 = (i+2 < len) ? (unsigned char)$2[i+2] : 0;
            unsigned char b3 = (i+3 < len) ? (unsigned char)$2[i+3] : 0;
            int rsp_offset = totalBytes - (i + 4);
            printf("    mov DWORD [rsp + %d], 0x%02X%02X%02X%02X\n", rsp_offset, b0, b1, b2, b3);
        }
    }
    ;

opt_reverse:  { $$ = 0; }
    | REVERSE { $$ = 1; }
    ;

math_expr:
    NUMBER
    | LPAREN math_expr RPAREN      { $$ = $2; }
    | math_expr PLUS math_expr     { $$ = $1 + $3; }
    | math_expr MINUS math_expr    { $$ = $1 - $3; }
    | math_expr DIVIDE math_expr   { $$ = $1 / $3; }
    | math_expr MULTIPLY math_expr { $$ = $1 * $3; }
    ;

conditional:
    LBRACKET cond_body RBRACKET
    { strcpy($$, $2); }
    ;

cond_body:
    register cond_op register
    { snprintf($$, 256, "%s|%s|%s", $1, $2, $3); }
    |
    register cond_op NUMBER
    { snprintf($$, 256, "%s|%s|%d", $1, $2, $3); }
    ;

cond_op:
    RARROW          { strcpy($$, "jg"); }
    | LARROW        { strcpy($$, "jl"); }
    | GEQ           { strcpy($$, "jge"); }
    | LEQ           { strcpy($$, "jle"); }
    | EQ            { strcpy($$, "je"); }
    | NEQ           { strcpy($$, "jne"); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "\n");
    fprintf(stderr, "=== SYNTAX ERROR ===\n");
    fprintf(stderr, "Line %d: %s\n", lineno, s);
    if (strlen(last_mnemonic) > 0)
        fprintf(stderr, "Last recognized mnemonic: %s\n", last_mnemonic);
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
    extern FILE *yyin;

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("fopen");
            return 1;
        }
    }

    int result = yyparse();
    if (result != 0) return result;
    return 0;
}