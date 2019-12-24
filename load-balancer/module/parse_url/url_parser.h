#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#define PROLEN 10
#define MAX_HOSTLEN 255 

typedef struct
{
    char protocol[PROLEN];
    char host[MAX_HOSTLEN];
    int port;
    char path[PATH_MAX];
} url_info;

void parse_url(url_info *info, const char *full_url);

#endif /* URL_PARSER_H */