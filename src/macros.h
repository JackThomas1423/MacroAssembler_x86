#ifndef MACROS_H
#define MACROS_H

void emit_exit(int code);
void emit_print(int size, bool as_ptr);
void emit_if(const char *cond, const char *label);
void emit_while_goto(const char *cond, const char *label);
void emit_while_single(const char *cond, const char *instr);

void emit_push_literal(const char *str, const char *size_prefix, unsigned int padding, bool reversed);
void emit_push_stack_local(const char *str);

void emit_pop_to_register(const char *reg, const char *size_prefix);
void emit_pop_bytes(const char *size_prefix);

void emit_mem(unsigned int bytes);
void emit_free(unsigned int bytes);

void emit_swap(const char *size_prefix);
void emit_dup(const char *size_prefix);

#endif /* MACROS_H */
