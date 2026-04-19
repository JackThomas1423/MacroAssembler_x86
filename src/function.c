#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"

struct function_stack_locals* function_scopes[8];
int funcion_stack_locals_index = -1;

struct function_stack_locals* make_function_scope(const char* id, struct stack_local_chain* chain) {
    struct function_stack_locals* fsc = malloc(sizeof(struct function_stack_locals));
    fsc->function_id = malloc(strlen(id));
    strcpy(fsc->function_id, id);
    fsc->chain = chain;

    if (funcion_stack_locals_index >= 8) {
        fprintf(stderr, "Error: More than 8 functions defined, exceeding limit!\n");
    }

    ++funcion_stack_locals_index;
    function_scopes[funcion_stack_locals_index] = fsc;

    return fsc;
}

struct stack_local_chain* make_local_chain_node(const char* id, unsigned int sz, struct stack_local_chain* parent) {
    struct stack_local_chain* node_ptr = malloc(sizeof(struct stack_local_chain));
    node_ptr->local_id = (char*)malloc(strlen(id));
    strcpy(node_ptr->local_id, id);
    node_ptr->size = sz;
    node_ptr->next = parent;
    return node_ptr;
}

unsigned int get_local_chain_size(struct stack_local_chain* chain) {
    if (chain == NULL) return 0;
    unsigned int total_size = 0;
    struct stack_local_chain* ptr = chain;
    do {
        total_size += ptr->size;
        if (ptr->next == NULL) break;
        ptr = ptr->next;
    } while(1);
    return total_size;
}