#include "globals.h"

int loopCreationIndex = 0;

const char* get_register_for_group_size(unsigned int group_size) {
    switch (group_size) {
        case 1: return "BYTE";
        case 2: return "WORD";
        case 4: return "DWORD";
        case 8: return "QWORD";
        default:
            fprintf(stderr, "Unsupported group size: %u\n", group_size);
            return "BYTE"; // Default to BYTE for safety
    }
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