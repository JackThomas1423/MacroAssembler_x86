#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern int loopCreationIndex;

const char* get_register_for_group_size(unsigned int group_size);
char* ascii_str_to_hex_str(const char* str, unsigned int count);

#endif /* GLOBALS_H */
