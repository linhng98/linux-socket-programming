#include "./module/url_parser/parse_url.h"
#include <linux/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("missing URL parameter\n");
        return -1;
    }

    url_info *info = (url_info *)malloc(sizeof(url_info));
    parse_url(info, argv[1]);
    printf("%s %s %d %s\n", info->protocol, info->host, info->port, info->path);

    free(info);
    return 0;
}
