# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC := gcc
CFLAGS  := -g -Wall -lm
TARGET := 1612340
BIN_PATH := ./bin

all: $(TARGET)

1612340: proxy.c
	$(CC) $(CFLAGS) proxy.c -o $(BIN_PATH)/$@

clean:
	rm -rf $(BIN_PATH)/$(TARGET)