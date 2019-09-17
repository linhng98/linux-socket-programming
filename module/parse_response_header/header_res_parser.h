#ifndef HEADER_RES_PARSER_H
#define HEADER_RES_PARSER_H

#include <stdio.h>
#include <linux/socket.h>

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

typedef struct
{
    header h;
    node_h *p_node_h;
} node_h;

typedef struct
{
    node_h *head;
    node_h *tail;
} list_header;

typedef struct
{
    status_line sts;
    list_header lh;
} resp_headers;

int parse_res_headers(resp_headers* resp, FILE* fn);
int search_header_value(char* value, char* name, list_header* lh);
int clean_list_header(list_header* lh);
#endif /* HEADER_RES_PARSER_H */
