#ifndef MACROS_H
#define MACROS_H

void emit_exit(int code);
void emit_print(int size);
void emit_if(const char *cond, const char *label);
void emit_while_goto(const char *cond, const char *label);
void emit_while_single(const char *cond, const char *instr);
void emit_push_string_byte(const char *str, int reverse);
void emit_push_string_word(const char *str, int reverse);
void emit_push_string_dword(const char *str, int reverse);
void emit_push_string_qword(const char *str);

#endif /* MACROS_H */
