#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char protocol[10];
    char host[255];
    int port;
    char path[2000];
} url_info;

void parse_url(url_info *info, const char *full_url);

#endif /* URL_PARSER_H */