#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *protocol;
    char *host;
    char *port;
    char *path;
} url_info;

void parse_url(url_info *info, const char *full_url)
{
    // use this tmp instead full_url for data safety
    char *tmp = strcpy((char *)malloc(strlen(full_url) + 1), full_url);
    char *ptr = tmp; // this pointer point to current position when using strtok

    // get protocol info
    if (strstr(ptr, "://")) // if protocol field exist
    {
        // Ex: http://www.domain.com:8080/path/to/file
        ptr = strtok(tmp, ":/");
        info->protocol = strcpy((char *)malloc(strlen(ptr) + 1), ptr);

        ptr = strtok(NULL, "/"); // Ex: www.domain.com:8080
    }

    // get host info
    if (strstr(ptr, ":")) // if port field exist
    {
        // get host
        ptr = strtok(NULL, ":");
        info->host = strcpy((char *)malloc(strlen(ptr) + 1), ptr);
        // get port
        ptr = strtok(NULL, "/");
        info->port = strcpy((char *)malloc(strlen(ptr) + 1), ptr);
    }
    else
    {
        // Ex: www.domain.com/path/to/file
        ptr = strtok(NULL, "/");
        info->host = strcpy((char *)malloc(strlen(ptr) + 1), ptr);
    }

    // get path
    ptr = strtok(NULL, "");
    info->path = strcpy((char *)malloc(strlen(ptr) + 1), ptr);

    free(tmp);
}

int main(int argc, char *argv[])
{
    url_info *info = (url_info *)malloc(sizeof(url_info));
    if (info == NULL || argv[1] == NULL)
        return -1;

    parse_url(info, argv[1]);
    printf("Protocol: %s\nHost: %s\nPort: %s\nPath: %s\n", info->protocol,
           info->host, info->port, info->path);

    free(info->protocol);
    free(info->host);
    free(info->port);
    free(info->path);
    free(info);

    return 0;
}