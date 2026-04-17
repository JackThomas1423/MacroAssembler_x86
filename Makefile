BUILD = build

all: $(BUILD) x86_parser

$(BUILD):
	mkdir -p $(BUILD)

x86_parser: $(BUILD) $(BUILD)/x86.lex.c $(BUILD)/x86.tab.c
	gcc -Wno-format-truncation -o parse $(BUILD)/x86.lex.c $(BUILD)/x86.tab.c -ll

$(BUILD)/x86.lex.c: x86.lex
	lex -o $(BUILD)/x86.lex.c x86.lex

$(BUILD)/x86.tab.c: x86.yacc
	yacc -d -o $(BUILD)/x86.tab.c x86.yacc

compile: x86_parser
	rm -f $(BUILD)/output.asm $(BUILD)/output.o
	./parse test.asm >> $(BUILD)/output.asm
	nasm -f elf64 $(BUILD)/output.asm -o $(BUILD)/output.o
	ld $(BUILD)/output.o -o output

clean:
	rm -rf $(BUILD) x86_parser output