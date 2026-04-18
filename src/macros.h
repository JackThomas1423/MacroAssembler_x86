#ifndef MACROS_H
#define MACROS_H

void emit_exit(int code);
void emit_print(int size);
void emit_if(const char *cond, const char *label);
void emit_while_goto(const char *cond, const char *label);
void emit_while_single(const char *cond, const char *instr);

void emit_push(const char *str, unsigned int group_size, int order);
void push_as_bytes(const char *str, int order);
void push_as_words(const char *str, int order);
void push_as_dwords(const char *str, int order);
void push_as_qwords(const char *str, int order);
void push_as_complex_type(const char *str, unsigned int group_size, int order);

#endif /* MACROS_H */
