#include "globals.h"

int loopCreationIndex = 0;

const char* registers_64[16] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};
const char* registers_32[16] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
    "r8d",  "r9d",  "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};
const char* registers_16[16] = {
    "ax", "bx", "cx", "dx", "si", "di", "sp", "bp",
    "r8w",  "r9w",  "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};
const char* registers_8[16] = {
    "al", "bl", "cl", "dl", "sil", "dil", "spl", "bpl",
    "r8b",  "r9b",  "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
};

unsigned int size_prefix_to_bytes(const char* size_prefix) {
    if (strcmp(size_prefix, "BYTE")  == 0) return 1;
    if (strcmp(size_prefix, "WORD")  == 0) return 2;
    if (strcmp(size_prefix, "DWORD") == 0) return 4;
    if (strcmp(size_prefix, "QWORD") == 0) return 8;
    fprintf(stderr, "Error: Invalid size prefix '%s'\n", size_prefix);
    return 0;
}

// Converts an ASCII string to a hexadecimal string based on each character's ASCII value
// For example, "AB" would become "0x4142"
// Note: The returned string is a static buffer, it will be overwritten on subsequent calls
char* ascii_str_to_hex_str(const char* str, unsigned int count) {
    static char hex_str[256];
    hex_str[0] = '\0'; // Initialize the string

    strcat(hex_str, "0x"); // Add hex prefix
    char* ptr = hex_str + 2;
    for (size_t i = 0; i < count; i++) {
        char hex_byte[3]; // 2 hex + null terminator
        sprintf(hex_byte, "%02X", (unsigned char)str[i]);
        strcat(ptr, hex_byte);
        ptr += 2;
    }
    ptr[0] = '\0';

    return hex_str;
}