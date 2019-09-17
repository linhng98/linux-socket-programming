# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = gcc
CFLAGS  = -g -Wall
TARGET = 1612340
OBJ = main.o url_parser.o header_parser.o
URL_PRS_PATH = ./module/parse_url/
HD_PRS_PATH = ./module/parse_response_header/
BIN_PATH = ./bin/
OBJ_PATH = ./object/

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_PATH)/*.o -o $(BIN_PATH)/$@

main.o: main.c $(URL_PRS_PATH)/url_parser.h $(HD_PRS_PATH)/header_res_parser.h
	$(CC) $(CFLAGS) -c $< -o $(OBJ_PATH)/$@

url_parser.o: $(URL_PRS_PATH)/url_parser.c $(URL_PRS_PATH)/url_parser.h
	$(CC) $(CFLAGS) -c $< -o $(OBJ_PATH)/$@

header_parser.o: $(HD_PRS_PATH)/header_res_parser.c $(HD_PRS_PATH)/header_res_parser.h
	$(CC) $(CFLAGS) -c $< -o $(OBJ_PATH)/$@

clean:
	rm -rf $(OBJ_PATH)/*.o $(BIN_PATH)/$(TARGET)