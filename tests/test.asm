; Example x86 assembly file for testing the parser

main:
    mov rax, 0x400000
    add rax, rbx
    push rbp
    mov rbp, rsp
    mov rcx, 0xA
    
loop_start:
    dec rcx
    jne loop_start
    
    lea rax, [rsi + 5]
    mov [rbp - 8], rbx
    add [rax + rcx*8 + 16], rdx
    
    pop rbp
    ret
