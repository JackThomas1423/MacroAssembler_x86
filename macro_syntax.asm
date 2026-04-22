%define.table my_table
    r1 [register, string]
    r2 [register, address]
%end

%define.table my_other_table from my_table
    r2 [register, register]
%end

%type my_type : string, number, register

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

    %argc as %count

    %rep %arg %count
        push %arg
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