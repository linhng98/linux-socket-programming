#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *protocol;
    char *host;
    int port;
    char *path;
} url_info;

int count_sub_string(const char *a, const char *b);
void parse_url(url_info *info, const char *full_url);
void free_urlinfo(url_info *info);

#endif /* URL_PARSER_H */