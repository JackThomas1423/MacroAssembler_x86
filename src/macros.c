#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"
#include "globals.h"
#include "function.h"

void emit_exit(int code) {
    printf("    mov rax, 60\n");
    printf("    mov rdi, %d\n", code);
    printf("    syscall\n");
}

void emit_print(int size, bool as_ptr) {
    if (as_ptr) { printf("    pop rsi\n"); }
    else { printf("    mov rsi, rsp\n"); }
    printf("    mov rax, 1\n");
    printf("    mov rdi, 1\n");
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

void emit_push_literal(const char *str, const char *size_prefix, unsigned int padding, bool reversed) {
    char* str_buffer = (char*)str;
    if (reversed) {
        int length = strlen(str);
        str_buffer = malloc(length);
        for (int i = 0; i < length; ++i) str_buffer[i] = str[length - (i + 1)];
    }

    unsigned int size = size_prefix_to_bytes(size_prefix);
    int is_qword = (strcmp(size_prefix, "QWORD") == 0);
    const char* effective_prefix = is_qword ? "DWORD" : size_prefix;
    if (is_qword) size = 4;

    int len = strlen(str_buffer);
    int stride = size + padding;
    int num_groups = (len + size - 1) / size;
    // for QWORD, padding only applies every two groups
    int padded_pairs = is_qword ? (num_groups + 1) / 2 : num_groups;
    int total_bytes = (num_groups * size) + (padded_pairs * padding);

    printf("    sub rsp, %d\n", total_bytes);

    int byte_pos = 0;
    for (int g = 0; g < num_groups; g++) {
        int apply_padding = is_qword ? (g % 2 == 1) : 1;
        int slot_size = size + (apply_padding ? padding : 0);
        int rsp_offset = total_bytes - byte_pos - slot_size;

        char *hex_str = ascii_str_to_hex_str(str_buffer + (g * size), size);
        printf("    mov %s [rsp + %d], %s\n", effective_prefix, rsp_offset + (apply_padding ? padding : 0), hex_str);
        free(hex_str);

        if (apply_padding) {
            for (unsigned int p = 0; p < padding; p++)
                printf("    mov BYTE [rsp + %d], 0\n", rsp_offset + p);
        }

        byte_pos += slot_size;
    }

    if (reversed) free(str_buffer);
}

void emit_push_stack_local(const char *str) {
    struct function_stack_locals* scope = function_scopes[funcion_stack_locals_index];
    unsigned int total_space = get_local_chain_size(scope->chain);
    unsigned int offset = 0;
    struct stack_local_chain* ptr = scope->chain;
    do {
        if (strcmp(str, ptr->local_id) == 0) break;
        offset += ptr->size;
        if (ptr->next == NULL) break;
        ptr = ptr->next;
    } while(1);
    printf("    lea rax, QWORD [rbp-%d]\n", total_space - offset);
    printf("    push rax\n");
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

void emit_mem(unsigned int bytes) {
    printf("    mov rax, 9\n");
    printf("    xor rdi, rdi\n");
    printf("    mov rsi, %d\n", bytes);
    printf("    mov rdx, 3\n");
    printf("    mov r10, 34\n");
    printf("    mov r8, -1\n");
    printf("    xor r9, r9\n");
    printf("    syscall\n");
    printf("    push rax\n");
}

void emit_free(unsigned int bytes) {
    printf("    mov rax, 11\n");
    printf("    mov rsi, 4096\n");
    printf("    pop rdi\n");
    printf("    syscall\n");
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

void emit_dup(const char* size_prefix) {
    unsigned int bytes = size_prefix_to_bytes(size_prefix);
    const char* reg = registers_64[0]; // rax
    if (bytes == 1) {
        reg = registers_8[0]; // al
    } else if (bytes == 2) {
        reg = registers_16[0]; // ax
    } else if (bytes == 4) {
        reg = registers_32[0]; // eax
    }
    printf("    mov %s, %s [rsp]\n", reg, size_prefix);
    printf("    sub rsp, %d\n", bytes);
    printf("    mov %s [rsp], %s\n", size_prefix, reg);
}