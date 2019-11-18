# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC := gcc
CFLAGS  := -g -Wall
TARGET := client server1 netdclient netdsrv server2
BIN_PATH := ./bin

all: $(TARGET)

client: client.c
	$(CC) $(CFLAGS) client.c -o $(BIN_PATH)/$@

server1: server1.c
	$(CC) $(CFLAGS) server1.c -o $(BIN_PATH)/$@

netdclient: netdclient.c
	$(CC) $(CFLAGS) netdclient.c -o $(BIN_PATH)/$@

netdsrv: netdsrv.c
	$(CC) $(CFLAGS) netdsrv.c -o $(BIN_PATH)/$@

server2: server2.c
	$(CC) $(CFLAGS) server2.c -o $(BIN_PATH)/$@

clean:
	rm -rf $(BIN_PATH)/$(TARGET)
