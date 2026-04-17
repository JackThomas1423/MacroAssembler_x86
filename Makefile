BUILD   = build
GRAMMAR = grammar
SRC     = src

C_LEX     = x86.lex.c
C_YACC    = x86.tab.c
C_SRCS   := $(wildcard $(SRC)/*.c)


GRAMMAR_SRCS := $(wildcard $(GRAMMAR)/*.yg) $(wildcard $(GRAMMAR)/*.yh)
GEN_SRCS      = $(BUILD)/$(C_LEX) $(BUILD)/$(C_YACC)
ALL_SRCS      = $(GEN_SRCS) $(C_SRCS)

all: $(BUILD) x86_parser

$(BUILD):
	mkdir -p $(BUILD)

x86_parser: $(BUILD) $(ALL_SRCS)
	gcc -Wno-format-truncation -I$(SRC) -o parse $(ALL_SRCS) -ll

$(BUILD)/$(C_LEX): x86.lex
	lex -o $(BUILD)/$(C_LEX) x86.lex

$(BUILD)/$(C_YACC): x86.yacc $(GRAMMAR_SRCS)
	cpp -x c -P x86.yacc -o $(BUILD)/x86.preprocessed.yacc
	yacc -d -o $(BUILD)/$(C_YACC) $(BUILD)/x86.preprocessed.yacc

compile: x86_parser
	rm -f $(BUILD)/output.asm $(BUILD)/output.o
	./parse test.asm >> $(BUILD)/output.asm
	nasm -f elf64 $(BUILD)/output.asm -o $(BUILD)/output.o
	ld $(BUILD)/output.o -o output

clean:
	rm -rf $(BUILD) parse output