; Testfile for assembly SuperSet macros and commands

[BITS 64]

section .text

global _start

_start:
    mov rax, 25
    push 10
    push reverse "Hello!" as DWORD
    print 24
.skip:
    exit 0