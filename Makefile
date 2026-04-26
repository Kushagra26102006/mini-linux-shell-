CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
TARGET  = minishell
SRCS    = minishell.c commands.c
HEADERS = commands.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo ""
	@echo "  ✔  Expert Build Successful → ./$(TARGET)"
	@echo ""

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
	@echo "  ✔  Cleanup complete."
