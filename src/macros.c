#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "macros.h"
#include "globals.h"

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

// Pushes the input str in sets of group_size until fully pushed (pads the last group if necessary)
void emit_push(const char *str, unsigned int group_size, int order) {
    switch(group_size) {
        case 1:  push_as_bytes(str, order); break;
        case 2:  push_as_words(str, order); break;
        case 4:  push_as_dwords(str, order); break;
        case 8:  push_as_qwords(str, order); break;
        default: push_as_complex_type(str, group_size, order); break;
    }
}

void push_as_bytes(const char *str, int order) {
    if (order == 0) {
        for (int i = 0; i < (int)strlen(str); i += 1) {
            char* hex_str = ascii_str_to_hex_str(str + i, 1);
            printf("    push %s\n", hex_str);
        }
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 1) {
            char* hex_str = ascii_str_to_hex_str(str + i, 1);
            printf("    push %s\n", hex_str);
        }
    }
}

void push_as_words(const char *str, int order) {
    if (order == 0) {
        for (int i = 0; i < (int)strlen(str); i += 2) {
            char* hex_str = ascii_str_to_hex_str(str + i, 2);
            printf("    push %s\n", hex_str);
        }
    } else {
        // reverses strings in groups of 2
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 2) {
            char swap[3] = {str[i], (i - 1 >= 0) ? str[i - 1] : 0, '\0'};
            char* hex_str = ascii_str_to_hex_str(swap, 2);
            printf("    push %s\n", hex_str);
        }
    }
}

void push_as_dwords(const char *str, int order) {
    if (order == 0) {
        for (int i = 0; i < (int)strlen(str); i += 4) {
            char* hex_str = ascii_str_to_hex_str(str + i, 4);
            printf("    push %s\n", hex_str);
        }
    } else {
        // reverses strings in groups of 4
        for (int i = (int)strlen(str); i >= 0; i -= 4) {
            char swap[5] = {
                str[i],
                (i - 1 >= 0) ? str[i - 1] : 0,
                (i - 2 >= 0) ? str[i - 2] : 0,
                (i - 3 >= 0) ? str[i - 3] : 0,
                '\0'
            };
            char* hex_str = ascii_str_to_hex_str(swap, 4);
            printf("    push %s\n", hex_str);
        }
    }
}

// Note: Groups like qwords exceed the max immediate value for push, so this will push the qword as two dwords

void push_as_qwords(const char *str, int order) {
    int len = strlen(str);
    int paddedSize = (len % 8 == 0) ? (len / 8) : (len / 8) + 1;
    int totalBytes = paddedSize * 8;
    printf("    sub rsp, %d\n", totalBytes);

    if (order == 0) {
        for (int i = 0; i < totalBytes; i += 4) {
            char* hex_str = ascii_str_to_hex_str(str + i, 4);
            int rsp_offset = totalBytes - (i + 4);
            printf("    mov DWORD [rsp + %d], %s\n", rsp_offset, hex_str);
        }
    } else {
        // TODO: failed to reverse properly, fix this. throw error when here
        fprintf(stderr, "\n");
        fprintf(stderr, "=== ERROR: Reversing qwords is not currently supported ===\n");
        fprintf(stderr, "\n");
        exit(1);
    }
}

// Note: Cannot use types larger than 8 bytes
void push_as_complex_type(const char *str, unsigned int group_size, int order) {
    if (order == 0) {
        for (int i = 0; i < (int)strlen(str); i += group_size) {
            char* hex_str = ascii_str_to_hex_str(str + i, group_size);
            printf("    push %s\n", hex_str);
        }
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= group_size) {
            char* hex_str = ascii_str_to_hex_str(str + i - group_size + 1, group_size);
            printf("    push %s\n", hex_str);
        }
    }
}