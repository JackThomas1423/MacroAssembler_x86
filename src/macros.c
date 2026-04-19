#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "macros.h"
#include "globals.h"
#include "function.h"

void emit_exit(int code) {
    printf("    mov rax, 60\n");
    printf("    mov rdi, %d\n", code);
    printf("    syscall\n");
}

void emit_print(int size) {
    printf("    mov rax, 1\n");
    printf("    mov rdi, 1\n");
    printf("    mov rsi, rsp\n");
    printf("    mov rdx, %d\n", size);
    printf("    syscall\n");
}

void emit_if(const char *cond, const char *label) {
    char lhs[64], op[8], rhs[64];
    sscanf(cond, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
    printf("    cmp %s, %s\n", lhs, rhs);
    printf("    %s %s\n", op, label);
}

void emit_while_goto(const char *cond, const char *label) {
    char lhs[64], op[8], rhs[64];
    sscanf(cond, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
    printf("    cmp %s, %s\n", lhs, rhs);
    printf("    %s %s\n", op, label);
}

void emit_while_single(const char *cond, const char *instr) {
    char lhs[64], op[8], rhs[64];
    sscanf(cond, "%63[^|]|%7[^|]|%63s", lhs, op, rhs);
    char labelBuffer[256];
    snprintf(labelBuffer, 256, ".whileSingle%d", loopCreationIndex);
    loopCreationIndex += 1;
    printf("%s:\n", labelBuffer);
    printf("    cmp %s, %s\n", lhs, rhs);
    printf("    %s %s\n", op, labelBuffer);
    printf("    %s\n", instr);
}

void emit_push(const char *str, int order) {
    int len = strlen(str);
    printf("    sub rsp, %d\n", len);
    if (order == 0) {
        for (int i = 0; i < len; ++i) {
            char* hex_str = ascii_str_to_hex_str(str + i, 1);
            int rsp_offset = len - (i + 1);
            printf("    mov BYTE [rsp + %d], %s\n", rsp_offset, hex_str);
        }
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 1) {
            char* hex_str = ascii_str_to_hex_str(str + i, 1);
            int rsp_offset = len - ((strlen(str) - i) + 1);
            printf("    mov BYTE [rsp + %d], %s\n", rsp_offset + 1, hex_str);
        }
    }
}

void emit_push_label(const char *str) {
    struct function_stack_locals* scope = function_scopes[funcion_stack_locals_index];
    unsigned int total_space = get_local_chain_size(scope->chain);
    unsigned int offset = 0;
    struct stack_local_chain* ptr = scope->chain;
    do {
        offset += ptr->size;
        if (strcmp(str, ptr->local_id) == 0) break;
        if (ptr->next == NULL) break;
        ptr = ptr->next;
    } while(1);
    printf("    push QWORD [rbp-%d]\n", total_space - offset);
}

void emit_pop_to_register(const char *reg, const char* size_prefix) {
    unsigned int bytes = size_prefix_to_bytes(size_prefix);
    printf("    mov %s, %s [rsp]\n", reg, size_prefix);
    printf("    add rsp, %d\n", bytes);
}

void emit_pop_bytes(const char *size_prefix) {
    unsigned int bytes = size_prefix_to_bytes(size_prefix);
    printf("    add rsp, %d\n", bytes);
}

// swaps the first N bytes (based on size prefix) of the stack around
void emit_swap(const char *size_prefix) {
    unsigned int bytes = size_prefix_to_bytes(size_prefix);
    // Make registers resize based on size prefix
    const char* reg1 = registers_64[0]; // rax
    const char* reg2 = registers_64[1]; // rbx
    if (bytes == 1) {
        reg1 = registers_8[0]; // al
        reg2 = registers_8[1]; // bl
    } else if (bytes == 2) {
        reg1 = registers_16[0]; // ax
        reg2 = registers_16[1]; // bx
    } else if (bytes == 4) {
        reg1 = registers_32[0]; // eax
        reg2 = registers_32[1]; // ebx
    }
    printf("    mov %s, %s [rsp]\n", reg1, size_prefix);
    printf("    mov %s, %s [rsp + %d]\n", reg2, size_prefix, bytes);
    printf("    mov %s [rsp], %s\n", size_prefix, reg2);
    printf("    mov %s [rsp + %d], %s\n", size_prefix, bytes, reg1);
}