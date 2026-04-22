%define.table my_table
    rule1: [register, string]
    rule2: [register, address]
%undef

%define.table my_other_table from my_table
    rule2: [register, register]
%undef

%type my_type: string | number | register

%define.static my_string "This is my string!"

%use my_table
%macro.strict mov 2
    %arg %1 : my_type
    %arg %2 : my_type

    ; Note: do the actuall process here
%endmacro

%use my_other_table
%macro.strict sub 2
    %arg %1 : string, number, register
    %arg %2 : string, number, register

    ; Note: do the actuall process here
%endmacro

%macro push_macro 1-3
    %arg %1 : string, number, register
    %arg %2 : string, number, register
    %arg %3 : string, number, register

    %rep %0
        push %1
        %rotate 1
    %endrep
%endmacro

mov a, [b]
sub a, b

push rax
push_macro rax rbx rcx

push rax
push rbx
push rcx

my_macro a b

push 1
push 2