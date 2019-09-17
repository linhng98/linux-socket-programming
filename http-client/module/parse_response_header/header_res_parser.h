#ifndef HEADER_RES_PARSER_H
#define HEADER_RES_PARSER_H

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#define HEADER_SIZE 5000

typedef struct
{
    char protocol[10];
    int status_code;
    char status_text[40];
} status_line;

typedef struct
{
    char name[40];
    char value[100];
} header;

int get_headers(char* hbuff, int sockfd);
int search_header_value(char* value, char* header, char* hbuff);
int get_status_code(char* hbuff);

#endif /* HEADER_RES_PARSER_H */
