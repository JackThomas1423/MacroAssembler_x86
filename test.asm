; Testfile for assembly SuperSet macros and commands

[BITS 64]

section .text

global _start

_start:
    mov rax, 25
    push back "Hello world! Testing? HELLO",10 as QWORD
    print 32
.skip:
    exit 0