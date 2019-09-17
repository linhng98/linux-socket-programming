#include "url_parser.h"

int count_sub_string(const char *a, const char *b)
{
    int count = 0;
    const char *pos = a;
    while ((pos = strstr(pos, b)))
    {
        count++;
        pos += strlen(b);
    }
    return count;
}

void parse_url(url_info *info, const char *full_url)
{
    strcpy(info->protocol, "http");
    info->port = 80;

    switch (count_sub_string(full_url, ":"))
    {
    case 0: // no protocol, no port
        sscanf(full_url, "%99[^/]/%99[^\n]", info->host, info->path);
        break;
    case 1:
        if (count_sub_string(full_url, "://")) // have protocol, no port
            sscanf(full_url, "%99[^:]://%99[^/]/%99[^\n]", info->protocol,
                   info->host, info->path);
        else // no protocol, have port
            sscanf(full_url, "%99[^:]:%99d/%99[^\n]", info->host, &info->port,
                   info->path);
        break;
    case 2: // have both protocol and port
        sscanf(full_url, "%99[^:]://%99[^:]:%99d/%99[^\n]", info->protocol,
               info->host, &info->port, info->path);
        break;
    default:
        printf("invalid URL format!\n");
    }
}