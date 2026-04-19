%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "x86.tab.h"

int lineno = 1;

char current_line[512] = "";

void append_to_line(const char *text) {
    strncat(current_line, text, 511 - strlen(current_line));
}

char* flush_line(void) {
    static char buf[512];
    /* trim trailing whitespace */
    int len = strlen(current_line);
    while (len > 0 && (current_line[len-1] == ' ' || current_line[len-1] == '\t'))
        current_line[--len] = '\0';
    strcpy(buf, current_line);
    current_line[0] = '\0';
    return buf;
}
%}

%%

[ \t]+      { append_to_line(yytext); }
\n          { lineno++; return NEWLINE; }
\r\n        { lineno++; return NEWLINE; }

;[^\n]*     ;
#[^\n]*     ;

\"[^"]*\" {
    append_to_line(yytext);
    int len = strlen(yytext) - 2;
    if (len > 255) len = 255;
    strncpy(yylval.str, yytext + 1, len);
    yylval.str[len] = '\0';
    return STRING;
}

mov|MOV|Mov|mOV|moV|MoV|mOv|MOv              { append_to_line(yytext); return MOV; }
movl|MOVL|Movl|mOVL|movL|MoVL|mOVl|MOVl      { append_to_line(yytext); return MOVL; }
movq|MOVQ|Movq|mOVQ|movQ|MoVQ|mOVq|MOVq      { append_to_line(yytext); return MOVQ; }
add|ADD|Add|aDd|adD|AdD|aDd|ADd               { append_to_line(yytext); return ADD; }
sub|SUB|Sub|sUb|suB|SuB|sUb|SUb               { append_to_line(yytext); return SUB; }
inc|INC|Inc|iNc|inC|InC|iNC|INc              { append_to_line(yytext); return INC; }
dec|DEC|Dec|dEc|deC|DeC|dEC|DEc              { append_to_line(yytext); return DEC; }
mul|MUL|Mul|mUl|muL|MuL|mUl|MUl             { append_to_line(yytext); return MUL; }
imul|IMUL|Imul|iMul|imUl|imuL|ImUl|iMUL|IMul { append_to_line(yytext); return IMUL; }
div|DIV|Div|dIv|diV|DiV|dIv|DIv             { append_to_line(yytext); return DIV; }
idiv|IDIV|Idiv|iDiv|idIv|idiV|IdIv|iDIV|IDIv { append_to_line(yytext); return IDIV; }
and|AND|And|aNd|anD|AnD|aNd|ANd             { append_to_line(yytext); return AND; }
or|OR|Or|oR                                  { append_to_line(yytext); return OR; }
xor|XOR|Xor|xOr|xoR|XoR|xOr|XOR            { append_to_line(yytext); return XOR; }
not|NOT|Not|nOt|noT|NoT|nOt|NOt             { append_to_line(yytext); return NOT; }
shl|SHL|Shl|sHl|shL|ShL|sHl|SHL            { append_to_line(yytext); return SHL; }
shr|SHR|Shr|sHr|shR|ShR|sHr|SHR            { append_to_line(yytext); return SHR; }
sal|SAL|Sal|sAl|saL|SaL|sAl|SAl            { append_to_line(yytext); return SHL; }
sar|SAR|Sar|sAr|saR|SaR|sAr|SAr            { append_to_line(yytext); return SAR; }
jmp|JMP|Jmp|jMp|jmP|JmP|jMp|JMP           { append_to_line(yytext); return JMP; }
je|JE|Je|jE                                { append_to_line(yytext); return JE; }
jz|JZ|Jz|jZ                                { append_to_line(yytext); return JZ; }
jne|JNE|Jne|jNe|jnE|JnE|jNe|JNe           { append_to_line(yytext); return JNE; }
jnz|JNZ|Jnz|jNz|jnZ|JNz|jNZ|JNz           { append_to_line(yytext); return JNZ; }
jl|JL|Jl|jL                                { append_to_line(yytext); return JL; }
jg|JG|Jg|jG                                { append_to_line(yytext); return JG; }
jle|JLE|Jle|jLe|jlE|JlE|jLe|JLE           { append_to_line(yytext); return JLE; }
jge|JGE|Jge|jGe|jgE|JgE|jGe|JGE           { append_to_line(yytext); return JGE; }
ja|JA|Ja|jA                                { append_to_line(yytext); return JA; }
jb|JB|Jb|jB                                { append_to_line(yytext); return JB; }
jae|JAE|Jae|jAe|jaE|JaE|jAe|JAE           { append_to_line(yytext); return JAE; }
jbe|JBE|Jbe|jBe|jbE|JbE|jBe|JBE           { append_to_line(yytext); return JBE; }
call|CALL|Call|cAll|caLl|cAll|cALl|CALl   { append_to_line(yytext); return CALL; }
ret|RET|Ret|rEt|reT|ReT|rEt|RET            { append_to_line(yytext); return RET; }
push|PUSH|Push|pUsh|puSh|pUsh|pUSh|PUsh   { append_to_line(yytext); return PUSH; }
pop|POP|Pop|pOp|poP|PoP|pOp|POP            { append_to_line(yytext); return POP; }
cmp|CMP|Cmp|cMp|cmP|CmP|cMp|CMP           { append_to_line(yytext); return CMP; }
test|TEST|Test|tEst|teSt|teSt|tESt|TEst   { append_to_line(yytext); return TEST; }
lea|LEA|Lea|lEa|leA|LeA|lEa|LEA            { append_to_line(yytext); return LEA; }
nop|NOP|Nop|nOp|noP|NoP|nOp|NOP            { append_to_line(yytext); return NOP; }
cmpsb|CMPSB|Cmpsb|cMpsb|cmPsb|cmPSb|cmpsB|CMpsb|cMPsb { append_to_line(yytext); return CMPSB; }
loop|LOOP|Loop|lOop|loOp|looP|LOop|lOOp   { append_to_line(yytext); return LOOP; }
loope|LOOPE|Loope|lOope|loOpe|loopE|LOope|lOOpe|lOOpE|LOOpe { append_to_line(yytext); return LOOPE; }
loopne|LOOPNE|Loopne|lOopne|loOpne|loopNe|loopnE|LOopne|lOOpne|lOOpNe { append_to_line(yytext); return LOOPNE; }
stosd|STOSD|Stosd|sTOsd|stOsd|stOSd|stosD|STOsd|sTOSd|sTOSD { append_to_line(yytext); return STOSD; }
stosb|STOSB|Stosb|sTOsb|stOsb|stOSb|stosB|STOsb|sTOSb|sTOSB { append_to_line(yytext); return STOSB; }
scasb|SCASB|Scasb|sCasb|scAsb|scASb|scasB|SCasb|sCAsb|sCASB { append_to_line(yytext); return SCASB; }
movsb|MOVSB|Movsb|mOvsb|moVsb|moVSb|movsB|MOvsb|mOVsb|mOVSB { append_to_line(yytext); return MOVSB; }
movsd|MOVSD|Movsd|mOvsd|moVsd|moVSd|movSD|MOvsd|mOVsd|mOVSD { append_to_line(yytext); return MOVSD; }
movzx|MOVZX|Movzx|mOvzx|moVzx|movZx|movzX|MOvzx|mOVzx|mOVZx|mOVzX { append_to_line(yytext); return MOVZX; }
movsx|MOVSX|Movsx|mOvsx|moVsx|movSx|movsX|MOvsx|mOVsx|mOVSx|mOVsx { append_to_line(yytext); return MOVSX; }

byte|BYTE|Byte|bYte|byTe|bytE|BYte|bYTe|BYTE  { append_to_line(yytext); return BYTE; }
word|WORD|Word|wOrd|woRd|worD|WOrd|wORd|WORD  { append_to_line(yytext); return WORD; }
dword|DWORD|Dword|dWord|dwOrd|dwoRd           { append_to_line(yytext); return DWORD; }
qword|QWORD|Qword|qWord|qwOrd|qwoRd           { append_to_line(yytext); return QWORD; }
ptr|PTR|Ptr|pTr|ptR|PtR|pTr|PTR               { append_to_line(yytext); return PTR; }

bits|BITS|Bits|bIts|biTs|bitS|BIts|bITs|bItS|BITS { append_to_line(yytext); return BITS; }
section|SECTION|Section|sEction|secTion|sectIon|sectioN { append_to_line(yytext); return SECTION; }
global|GLOBAL|Global|gLobal|glObal|gloBal|globAl|globaL { append_to_line(yytext); return GLOBAL; }
extern|EXTERN|Extern|eXtern|exTern|extErn|exterN { append_to_line(yytext); return EXTERN; }
equ|EQU|Equ|eQu|eqU|EQu|eQU { append_to_line(yytext); return EQU; }
resb|RESB|Resb|rEsb|reSb|RESb|rESB { append_to_line(yytext); return RESB; }
resw|RESW|Resw|rEsw|reWs|RESw|rESW { append_to_line(yytext); return RESW; }
resd|RESD|Resd|rEsd|reSd|RESd|rESD { append_to_line(yytext); return RESD; }
resq|RESQ|Resq|rEsq|reSq|RESq|rESQ { append_to_line(yytext); return RESQ; }
repe|REPE|Repe|rEpe|rePe|repE|REpe|rEPe|rEPE { append_to_line(yytext); return REPE; }
repne|REPNE|Repne|rEpne|rePne|repNe|repnE|REpne|rEPne|rEPNe { append_to_line(yytext); return REPNE; }
rep|REP|Rep|rEp|reP|REp|rEP { append_to_line(yytext); return REP; }

as      { append_to_line(yytext); return AS;      }
if      { append_to_line(yytext); return IF;      }
var     { append_to_line(yytext); return VAR;     }
swap    { append_to_line(yytext); return SWAP;    }
goto    { append_to_line(yytext); return GOTO;    }
exit    { append_to_line(yytext); return EXIT;    }
back    { append_to_line(yytext); return BACK;    }
print   { append_to_line(yytext); return PRINT;   }
front   { append_to_line(yytext); return FRONT;   }
while   { append_to_line(yytext); return WHILE;   }
leave   { append_to_line(yytext); return LEAVE;   }
repeat  { append_to_line(yytext); return REPEAT;  }
single  { append_to_line(yytext); return SINGLE;  }

syscall { append_to_line(yytext); return SYSCALL; }

rax|RAX|Rax|rAx|raX|RaX|rAx|RAx { append_to_line(yytext); return RAX; }
rbx|RBX|Rbx|rBx|rbX|RbX|rBx|RBx { append_to_line(yytext); return RBX; }
rcx|RCX|Rcx|rCx|rcX|RcX|rCx|RCx { append_to_line(yytext); return RCX; }
rdx|RDX|Rdx|rDx|rdX|RdX|rDx|RDx { append_to_line(yytext); return RDX; }
rsi|RSI|Rsi|rSi|rsI|RsI|rSi|RSi { append_to_line(yytext); return RSI; }
rdi|RDI|Rdi|rDi|rdI|RdI|rDi|RDi { append_to_line(yytext); return RDI; }
rbp|RBP|Rbp|rBp|rbP|RbP|rBp|RBp { append_to_line(yytext); return RBP; }
rsp|RSP|Rsp|rSp|rsP|RsP|rSp|RSp { append_to_line(yytext); return RSP; }
r8|R8   { append_to_line(yytext); return R8; }
r9|R9   { append_to_line(yytext); return R9; }
r10|R10 { append_to_line(yytext); return R10; }
r11|R11 { append_to_line(yytext); return R11; }
r12|R12 { append_to_line(yytext); return R12; }
r13|R13 { append_to_line(yytext); return R13; }
r14|R14 { append_to_line(yytext); return R14; }
r15|R15 { append_to_line(yytext); return R15; }

eax|EAX|Eax|eAx|eaX|EaX|eAx|EAx { append_to_line(yytext); return EAX; }
ebx|EBX|Ebx|eBx|ebX|EbX|eBx|EBx { append_to_line(yytext); return EBX; }
ecx|ECX|Ecx|eCx|ecX|EcX|eCx|ECx { append_to_line(yytext); return ECX; }
edx|EDX|Edx|eDx|edX|EdX|eDx|EDx { append_to_line(yytext); return EDX; }
esi|ESI|Esi|eSi|esI|EsI|eSi|ESi { append_to_line(yytext); return ESI; }
edi|EDI|Edi|eDi|edI|EdI|eDi|EDi { append_to_line(yytext); return EDI; }
ebp|EBP|Ebp|eBp|ebP|EbP|eBp|EBp { append_to_line(yytext); return EBP; }
esp|ESP|Esp|eSp|esP|EsP|eSp|ESp { append_to_line(yytext); return ESP; }

ax|AX|Ax|aX { append_to_line(yytext); return AX; }
bx|BX|Bx|bX { append_to_line(yytext); return BX; }
cx|CX|Cx|cX { append_to_line(yytext); return CX; }
dx|DX|Dx|dX { append_to_line(yytext); return DX; }
si|SI|Si|sI { append_to_line(yytext); return SI; }
di|DI|Di|dI { append_to_line(yytext); return DI; }
bp|BP|Bp|bP { append_to_line(yytext); return BP; }
sp|SP|Sp|sP { append_to_line(yytext); return SP; }

al|AL|Al|aL { append_to_line(yytext); return AL; }
ah|AH|Ah|aH { append_to_line(yytext); return AH; }
bl|BL|Bl|bL { append_to_line(yytext); return BL; }
bh|BH|Bh|bH { append_to_line(yytext); return BH; }
cl|CL|Cl|cL { append_to_line(yytext); return CL; }
ch|CH|Ch|cH { append_to_line(yytext); return CH; }
dl|DL|Dl|dL { append_to_line(yytext); return DL; }
dh|DH|Dh|dH { append_to_line(yytext); return DH; }

0[xX][0-9a-fA-F]+ { append_to_line(yytext); yylval.num = strtol(yytext, NULL, 16); return NUMBER; }
0[bB][01]+         { append_to_line(yytext); yylval.num = strtol(yytext+2, NULL, 2); return NUMBER; }
[0-9]+             { append_to_line(yytext); yylval.num = atol(yytext); return NUMBER; }

'.'    { append_to_line(yytext); yylval.num = yytext[1]; return NUMBER; }
'\\n'  { append_to_line(yytext); yylval.num = '\n'; return NUMBER; }
'\\t'  { append_to_line(yytext); yylval.num = '\t'; return NUMBER; }
'\\r'  { append_to_line(yytext); yylval.num = '\r'; return NUMBER; }
'\\\\'  { append_to_line(yytext); yylval.num = '\\'; return NUMBER; }

,   { append_to_line(yytext); return COMMA;    }
:   { append_to_line(yytext); return COLON;    }
\[  { append_to_line(yytext); return LBRACKET; }
\]  { append_to_line(yytext); return RBRACKET; }
\(  { append_to_line(yytext); return LPAREN;   }
\)  { append_to_line(yytext); return RPAREN;   }
\+  { append_to_line(yytext); return PLUS;     }
-   { append_to_line(yytext); return MINUS;    }
\*  { append_to_line(yytext); return MULTIPLY; }
\/  { append_to_line(yytext); return DIVIDE;   }
\<  { append_to_line(yytext); return LARROW;   }
\>  { append_to_line(yytext); return RARROW;   }
\>=  { append_to_line(yytext); return GEQ; }
\<=  { append_to_line(yytext); return LEQ; }
!=   { append_to_line(yytext); return NEQ; }
=    { append_to_line(yytext); return EQ;  }

\.[a-zA-Z_][a-zA-Z0-9_]*|[a-zA-Z_][a-zA-Z0-9_]* {
    append_to_line(yytext);
    strncpy(yylval.str, yytext, 31);
    yylval.str[31] = '\0';
    return LABEL;
}

\@[a-zA-Z_][a-zA-Z0-9_]*|[a-zA-Z_][a-zA-Z0-9_]* {
    append_to_line(yytext);
    int len = strlen(yytext) - 1;
    if (len > 255) len = 255;
    strncpy(yylval.str, yytext + 1, len);
    yylval.str[len] = '\0';
    return FUNC;
}

.   { append_to_line(yytext); }

%%

int yywrap() {
    return 1;
}