#include "header_res_parser.h"

int get_headers(char *hbuff, int sockfd)
{
    char c;
    char *ptr = hbuff;

    while (recv(sockfd, &c, 1, 0))
    {
        *ptr = c;
        ptr++;

        if (ptr[-1] == '\n' && ptr[-2] == '\r' && ptr[-3] == '\n' &&
            ptr[-4] == '\r')
            break;
    }

    return 0;
}

int search_header_value(char *value, char *header, char *hbuff)
{
    char *pos = strstr(hbuff, header);

    // header not exist
    if (pos == NULL)
        return -1;

    pos += strlen(header) + 2; // strlen(": ") == 2
    sscanf(pos, "%99[^\r]", value);

    return 0;
}

int get_status_code(char *hbuff)
{
    int code;
    sscanf(hbuff, "%*s %d %*s", &code);
    return code;
}