CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L -g
TARGET = hw1shell
OBJS = hw1shell.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c hw1shell.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
