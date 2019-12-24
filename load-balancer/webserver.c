#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "module/parse_header/header_parser.h"
#include "module/parse_url/url_parser.h"

#define BUFFSIZE 512
#define USAGE "Usage: webserver [-p <PORT>] [-d <RESOURCE_DIR>]\n"
#define VERSION                                                                                    \
    "webserver v1.0.0\n"                                                                           \
    "Written by Linh, Khue\n"
#define HELP                                                                                       \
    "webserver [-p <PORT>] [-d <RESOURCE_DIR>]\n"                                                  \
    "Serve GET request from client\n"                                                              \
    "\n"                                                                                           \
    "-p, --port         listening port of webserver server (default 11111)\n"                      \
    "-d, --dir          resource dir (default ./)\n"                                               \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"                                    \
    "\n"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static int is_dir(char *path);
static void init_server_socket(int *sockfd, sockaddr_in *servaddr);

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"dir", required_argument, 0, 'd'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};

static int port = 11111; // default webserver port
static char *dir = "../resource";  // default resource dir

int main(int argc, char *argv[])
{
    int c;
    int option_index;

    while (1)
    {
        c = getopt_long(argc, argv, "p:d:hv", long_options, &option_index);
        if (c == -1) // end of options
            break;
        switch (c)
        {
        case 'p':
            port = (int)strtol(optarg, NULL, 10);
            break;
        case 'd':
            dir = optarg;
            if (is_dir(dir) == 0) // invalid dir
            {
                printf("%s is invalid dir\n", dir);
                exit(EXIT_FAILURE);
            }
            if (dir[strlen(dir) - 1] == '/') // strip off '/' character if exist
                dir[strlen(dir) - 1] = '\0';
            break;
        case 'h':
            printf(HELP);
            exit(EXIT_SUCCESS);
        case 'v':
            printf(VERSION);
            exit(EXIT_SUCCESS);
        case '?':
            printf("Try 'client --help' for more information.\n");
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }

    printf("dir: %s\nport: %d\n", dir, port);

    int sockfd;
    sockaddr_in servaddr;
    char buffer[BUFFSIZE];
    int ret;
    init_server_socket(&sockfd, &servaddr);

    while (1)
    {
        int newsock = accept(sockfd, NULL, NULL);

        while (1)
        {
            ret = recv(newsock, buffer, BUFFSIZE, 0);

            if (ret == 0)
                break;
            else if (ret < 0)
            {
                perror("error when reading socket");
                break;
            }

            printf("%s", buffer);
        }

        close(newsock);
    }

    exit(EXIT_SUCCESS);
}

int is_dir(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

void init_server_socket(int *sockfd, sockaddr_in *servaddr)
{
    // assign ip, port
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(port);
    servaddr->sin_family = AF_INET;

    // create new socket
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket fail");
        exit(EXIT_FAILURE);
    }
    setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    // Binding newly created socket to given IP and verification
    if ((bind(*sockfd, (sockaddr *)servaddr, sizeof(*servaddr))) != 0)
    {
        perror("socket bind failed...");
        exit(EXIT_FAILURE);
    }

    // Now server is ready to listen and verification
    if ((listen(*sockfd, 0)) != 0)
    {
        perror("Listen failed...\n");
        exit(EXIT_FAILURE);
    }
}
