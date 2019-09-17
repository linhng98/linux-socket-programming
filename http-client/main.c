#include "module/parse_response_header/header_res_parser.h"
#include "module/parse_url/url_parser.h"
#include <arpa/inet.h>
#include <libgen.h>
#include <linux/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSZ 5000

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;
typedef struct in_addr in_addr;

void error_handler(char *message);
void get_URL_info(sockaddr_in *addr, url_info *info);
int check_is_file(char *path);
int download_file(int sockfd, char *exec_dir, url_info *info);
int download_dir(int sockfd, char *exec_dir, url_info *info);

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("missing URL parameter\n");
        return -1;
    }

    sockaddr_in *addr = (sockaddr_in *)malloc(sizeof(sockaddr_in));
    if (addr == NULL)
        error_handler("not enough memory space");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error_handler("create socket fd fail");

    // parse url field from argv
    url_info *info = (url_info *)malloc(sizeof(url_info));
    if (info == NULL)
        error_handler("not enough memory space");
    parse_url(info, argv[1]);

    // get data from URLinfo to sockaddr_in
    get_URL_info(addr, info);

    // connect to remote server
    int ret = connect(sockfd, (sockaddr *)addr, INET_ADDRSTRLEN);
    if (ret < 0)
        error_handler("connect to server fail");

    char *buffer = (char *)malloc(BUFFSZ);
    memset(buffer, '\0', BUFFSZ);

    // send request headers to server
    snprintf(
        buffer, BUFFSZ,
        "GET /%s HTTP/1.1\r\n"
        "Host: %s\r\n"
        //"Accept: "
        //"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        //"Accept-Language: en-US,en;q=0.5\r\n"
        //"Accept-Encoding: gzip, deflate\r\n"
        //"Connection: keep-alive\r\n"
        "\r\n",
        info->path, info->host);

    ret = send(sockfd, buffer, BUFFSZ, 0);
    if (ret < 0)
        error_handler("send data fail");
    memset(buffer, '\0', strlen(buffer));

    char *exec_dir = dirname(
        realpath(argv[0], NULL)); // get abs directory of executable file

    if (check_is_file(info->path))
        download_file(sockfd, exec_dir, info);
    else
        download_dir(sockfd, exec_dir, info);

    printf("protocol: %s\n"
           "host: %s\n"
           "port: %d\n"
           "path: %s\n",
           info->protocol, info->host, info->port, info->path);

    // close file and socket
    close(sockfd);

    // free data allocated
    free(info);
    free(addr);
    free(buffer);
    exit(0);
}

void error_handler(char *message)
{
    perror(message);
    exit(1);
}

void get_URL_info(sockaddr_in *addr, url_info *info)
{
    // declare addr data
    addr->sin_family = AF_INET;
    addr->sin_port = htons(info->port);
    hostent *he = gethostbyname(info->host);
    addr->sin_addr = *((in_addr *)he->h_addr_list[0]);
    bzero(&(addr->sin_zero), 8);
}

int check_is_file(char *path)
{
    char *ptr = path;
    while (*(ptr + 1) != '\0')
        ptr++;

    if (*ptr != '/')
        return 1;
    return 0;
}

int download_file(int sockfd, char *exec_dir, url_info *info)
{
    int ret;

    // get path same level with executable file to save
    char save_path[PATH_MAX]; // use this path to save downloaded file
    sprintf(save_path, "%s/%s", exec_dir,
            basename(strcpy((char *)malloc(PATH_MAX), info->path)));

    // get hhtp headers
    char hbuff[HEADER_SIZE];
    get_headers(hbuff, sockfd);
    printf("%s", hbuff);

    // status code wrong
    if ((ret = get_status_code(hbuff)) != 200)
    {
        printf("status code is %d", ret);
        return -1;
    }

    // get length of file
    char value[100];
    ret = search_header_value(value, "Content-Length", hbuff);
    if (ret < 0)
        return -1; // not file

    int flen = atoi(value);

    int bytes_recv = 0;
    char buffer[BUFFSZ];
    FILE *fn = fopen(save_path, "wb");
    while ((ret = recv(sockfd, buffer, BUFFSZ, 0)) > 0)
    {
        fwrite(buffer, 1, ret, fn);
        memset(buffer, '\0', ret);

        bytes_recv += ret;
        if (bytes_recv == flen)
            break;
    }

    return 0;
}

int download_dir(int sockfd, char *exec_dir, url_info *info)
{
    // get path same level with executable file to save
    char save_path[PATH_MAX]; // use this path to save downloaded dir
    sprintf(save_path, "%s/%s", exec_dir,
            basename(strcpy((char *)malloc(PATH_MAX), info->path)));

    return 0;
}