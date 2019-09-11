#include <ctype.h>
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

int count_sub_string(const char *a, const char *b)
{
    int count = 0;
    const char *pos = a;
    while (pos = strstr(pos, b))
    {
        count++;
        pos += strlen(b);
    }
    return count;
}

void parse_url(url_info *info, const char *full_url)
{
    info->protocol = (char *)malloc(10);
    info->host = (char *)malloc(255);
    info->port = 0;
    info->path = (char *)malloc(2000);

    switch (count_sub_string(full_url, ":"))
    {
    case 0:
        sscanf(full_url, "%99[^/]/%99[^\n]", info->host, info->path);
        break;
    case 1:
        if (count_sub_string(full_url, "://"))
            sscanf(full_url, "%99://%99/%99[^\n]", info->protocol, info->host,
                   info->path);
        else
            sscanf(full_url, "%s:%d/%s[^\n]", info->host, &info->port,
                   info->path);
        break;
    case 2:
        sscanf(full_url, "%99[^:]://%99[^:]:%99d/%99[^\n]", info->protocol,
               info->host, &info->port, info->path);
        break;
    default:
        printf("invalid URL format!\n");
    }
}

int main(int argc, char *argv[])
{
    url_info *info = (url_info *)malloc(sizeof(url_info));
    if (info == NULL || argv[1] == NULL)
        return -1;

    parse_url(info, argv[1]);
    printf("Protocol: %s\nHost: %s\nPort: %d\nPath: %s\n", info->protocol,
           info->host, info->port, info->path);

    free(info->protocol);
    free(info->host);
    free(info->path);
    free(info);

    return 0;
}
/*
#include <stdio.h>

int main(void)
{
    const char text[] = "http://www.abc.xyz:8888/servlet/rece";
    char ip[100];
    char pro[10];
    int port = 80;
    char page[100];
    sscanf(text, "%99[^:]://%99[^:]:%99d/%99[^\n]",pro, ip, &port, page);
    printf("protocol = \"%s\"\n", pro);
    printf("ip = \"%s\"\n", ip);
    printf("port = \"%d\"\n", port);
    printf("page = \"%s\"\n", page);
    return 0;
}
*/
