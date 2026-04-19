#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

extern int loopCreationIndex;

extern const char* registers_64[16];
extern const char* registers_32[16];
extern const char* registers_16[16];
extern const char* registers_8[16];

unsigned int size_prefix_to_bytes(const char* size_prefix);

char* ascii_str_to_hex_str(const char* str, unsigned int count);

#endif /* GLOBALS_H */
