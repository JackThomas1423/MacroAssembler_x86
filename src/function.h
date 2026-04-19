#ifndef FUNCTION_H
#define FUNCTION_H

struct stack_local_chain {
    char* local_id;
    unsigned int size;
    struct stack_local_chain* next;
};

struct function_stack_locals {
    char* function_id;
    struct stack_local_chain* chain;
};

extern struct function_stack_locals* function_scopes[8];
extern int funcion_stack_locals_index;

struct function_stack_locals* make_function_scope(const char* id, struct stack_local_chain* chain);

struct stack_local_chain* make_local_chain_node(const char* id, unsigned int sz, struct stack_local_chain* parent);
unsigned int get_local_chain_size(struct stack_local_chain* chain);

#endif
