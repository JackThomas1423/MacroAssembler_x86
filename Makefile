CC        = gcc
CFLAGS    = -Wno-format-truncation -Wall -Wextra -g

BISON_C   = jmal.tab.c
BISON_H   = jmal.tab.h
FLEX_C    = jmal.lex.c

TARGET    = jmal
BUILD_DIR = build

TARGET_SRCS = $(addprefix $(BUILD_DIR)/, $(BISON_C) $(FLEX_C))

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): main.c $(TARGET_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ -lfl

$(BUILD_DIR)/$(BISON_C) $(BUILD_DIR)/$(BISON_H): jmal.y
	bison -o $@ -d jmal.y

$(BUILD_DIR)/$(FLEX_C): jmal.l $(BUILD_DIR)/$(BISON_H)
	flex -o $@ jmal.l

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all lex clean
