#ifndef MACROS_H
#define MACROS_H

void emit_exit(int code);
void emit_print(int size);
void emit_if(const char *cond, const char *label);
void emit_while_goto(const char *cond, const char *label);
void emit_while_single(const char *cond, const char *instr);

void emit_push(const char *str, int order);
void emit_pop_to_register(const char *reg, const char *size_prefix);
void emit_pop_bytes(const char *size_prefix);

void emit_swap(const char *size_prefix);

#endif /* MACROS_H */
