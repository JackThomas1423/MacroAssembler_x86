#include <stdio.h>
#include <string.h>
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

void emit_push_string_byte(const char *str, int reverse) {
    if (reverse == 0) {
        for (int i = 0; i < (int)strlen(str); i += 1)
            printf("    push '%c'\n", str[i]);
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 1)
            printf("    push '%c'\n", str[i]);
    }
}

void emit_push_string_word(const char *str, int reverse) {
    if (reverse == 0) {
        for (int i = 0; i < (int)strlen(str); i += 2)
            printf("    push 0x%02X%02X\n", (unsigned char)str[i], (unsigned char)str[i+1]);
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 2)
            printf("    push 0x%02X%02X\n", (unsigned char)str[i], (unsigned char)str[i-1]);
    }
}

void emit_push_string_dword(const char *str, int reverse) {
    if (reverse == 0) {
        for (int i = 0; i < (int)strlen(str); i += 4)
            printf("    push 0x%02X%02X%02X%02X\n",
                   (unsigned char)str[i],   (unsigned char)str[i+1],
                   (unsigned char)str[i+2], (unsigned char)str[i+3]);
    } else {
        for (int i = (int)strlen(str) - 1; i >= 0; i -= 4)
            printf("    push 0x%02X%02X%02X%02X\n",
                   (unsigned char)str[i],   (unsigned char)str[i-1],
                   (unsigned char)str[i-2], (unsigned char)str[i-3]);
    }
}

void emit_push_string_qword(const char *str) {
    int len = strlen(str);
    int paddedSize = (len % 8 == 0) ? (len / 8) : (len / 8) + 1;
    int totalBytes = paddedSize * 8;
    printf("    sub rsp, %d\n", totalBytes);

    for (int i = 0; i < totalBytes; i += 4) {
        unsigned char b0 = (i     < len) ? (unsigned char)str[i]   : 0;
        unsigned char b1 = (i + 1 < len) ? (unsigned char)str[i+1] : 0;
        unsigned char b2 = (i + 2 < len) ? (unsigned char)str[i+2] : 0;
        unsigned char b3 = (i + 3 < len) ? (unsigned char)str[i+3] : 0;
        int rsp_offset = totalBytes - (i + 4);
        printf("    mov DWORD [rsp + %d], 0x%02X%02X%02X%02X\n",
               rsp_offset, b0, b1, b2, b3);
    }
}
