# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC := gcc
CFLAGS  := -g -Wall -lm -pthread
TARGET := master client fileserver
BIN_PATH := ./bin

all: $(TARGET)

master: master.c
	$(CC) $(CFLAGS) master.c -o $(BIN_PATH)/$@

fileserver: fileserver.c
	$(CC) $(CFLAGS) fileserver.c -o $(BIN_PATH)/$@

client: client.c
	$(CC) $(CFLAGS) client.c -o $(BIN_PATH)/$@

clean:
	rm -rf $(BIN_PATH)/$(TARGET)
