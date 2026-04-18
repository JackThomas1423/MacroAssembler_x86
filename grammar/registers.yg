register:
    register_64
    | register_32
    | register_16
    | register_8
    ;

register_64:
    RAX   { strcpy($$, "rax"); }
    | RBX { strcpy($$, "rbx"); }
    | RCX { strcpy($$, "rcx"); }
    | RDX { strcpy($$, "rdx"); }
    | RSI { strcpy($$, "rsi"); }
    | RDI { strcpy($$, "rdi"); }
    | RBP { strcpy($$, "rbp"); }
    | RSP { strcpy($$, "rsp"); }
    | R8  { strcpy($$, "r8");  }
    | R9  { strcpy($$, "r9");  }
    | R10 { strcpy($$, "r10"); }
    | R11 { strcpy($$, "r11"); }
    | R12 { strcpy($$, "r12"); }
    | R13 { strcpy($$, "r13"); }
    | R14 { strcpy($$, "r14"); }
    | R15 { strcpy($$, "r15"); }
    ;

register_32:
    EAX { strcpy($$, "eax"); }
    | EBX { strcpy($$, "ebx"); }
    | ECX { strcpy($$, "ecx"); }
    | EDX { strcpy($$, "edx"); }
    | ESI { strcpy($$, "esi"); }
    | EDI { strcpy($$, "edi"); }
    | EBP { strcpy($$, "ebp"); }
    | ESP { strcpy($$, "esp"); }
    ;

register_16:
    AX    { strcpy($$, "ax"); }
    | BX  { strcpy($$, "bx"); }
    | CX  { strcpy($$, "cx"); }
    | DX  { strcpy($$, "dx"); }
    | SI  { strcpy($$, "si"); }
    | DI  { strcpy($$, "di"); }
    | BP  { strcpy($$, "bp"); }
    | SP  { strcpy($$, "sp"); }
    ;

register_8:
    AL    { strcpy($$, "al"); }
    | AH  { strcpy($$, "ah"); }
    | BL  { strcpy($$, "bl"); }
    | BH  { strcpy($$, "bh"); }
    | CL  { strcpy($$, "cl"); }
    | CH  { strcpy($$, "ch"); }
    | DL  { strcpy($$, "dl"); }
    | DH  { strcpy($$, "dh"); }
    ;