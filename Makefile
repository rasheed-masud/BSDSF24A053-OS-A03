
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g

SRC = src/main.c src/shell.c src/execute.c
OBJ = obj/main.o obj/shell.o obj/execute.o
TARGET = bin/psh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

obj/%.o: src/%.c include/shell.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o $(TARGET)

.PHONY: all clean
