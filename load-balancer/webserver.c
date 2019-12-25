#include <arpa/inet.h>
#include <getopt.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFSIZE 512
#define HEADER_SIZE 4000
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
typedef struct timeval timeval;

static int is_dir(char *path);
static void init_server_socket(int *sockfd, sockaddr_in *servaddr);
static int get_req_headers(char *hbuff, int sockfd);
static char *concat(const char *s1, const char *s2);
static unsigned long get_file_size(char *path);

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"dir", required_argument, 0, 'd'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};

static int port = 11111; // default webserver port
static char *dir = NULL; // default resource dir

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

    char abs_dir[PATH_MAX];
    if (dir == NULL || (dir = realpath(dir, abs_dir)) == NULL)
    {
        printf("Missing resource dir or dir not exist.\nTry 'webserver --help' for more "
               "information.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    sockaddr_in servaddr;
    char buffer[BUFFSIZE];
    char header[HEADER_SIZE];
    int ret;
    init_server_socket(&sockfd, &servaddr);

    while (1)
    {
        int newsock = accept(sockfd, NULL, NULL);
        /*timeval tv;
        tv.tv_sec = 3; // wait recv 3s timeout
        tv.tv_usec = 0;
        setsockopt(newsock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);*/

        int seq = 0;
        while (1)
        {
            memset(header, '\0', sizeof(header));
            char req_file[PATH_MAX];

            ret = get_req_headers(header, newsock); // get request header
            sscanf(header, "%*s %s %*s", req_file);
            if (strcmp(req_file, "/") == 0)
                strcpy(req_file, "/index.html");

            if (ret == 0) // error when reading header
                break;
            if (ret < 0)
            {
                perror("error when reading request socket");
                break;
            }

            char *path_file = concat(dir, req_file);
            FILE *f = fopen(path_file, "rb");

            if (f != NULL) // file exist
            {
                char *success_res_header = "HTTP/1.1 200 OK\r\n"
                                           //"Keep-Alive: timeout=5\r\n"
                                           "Connection: keep-alive\r\n";
                send(newsock, success_res_header, strlen(success_res_header), 0);

                sprintf(buffer, "Content-Length: %lu\r\n\r\n", get_file_size(path_file));
                send(newsock, buffer, strlen(buffer), 0);
                while ((ret = fread(buffer, 1, BUFFSIZE, f)) > 0)
                {
                    buffer[ret] = '\0';
                    if (send(newsock, buffer, ret, 0) <= 0)
                        break;
                }
                fclose(f);
            }
            else if (seq == 0) // file not found and seq == 0 (first GET request)
            {                  // redirect 404 error
                char *redirect_404 = "HTTP/1.1 301 Moved Permanently\r\n"
                                     "Location: /404.html\r\n\r\n";
                send(newsock, redirect_404, strlen(redirect_404), 0);
            }
            else // unknow file, send nothing
                send(newsock, "\r\n", 2, 0);

            free(path_file);
            seq++;
        }

        close(newsock);
        printf("close connection\n");
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

int get_req_headers(char *hbuff, int sockfd)
{
    int ret;
    char c;
    char *ptr = hbuff;

    while ((ret = recv(sockfd, &c, 1, 0)) > 0)
    {
        *ptr = c;
        ptr++;
        if (ptr[-1] == '\n' && ptr[-2] == '\r' && ptr[-3] == '\n' && ptr[-4] == '\r')
            break;
    }
    hbuff[strlen(hbuff)] = '\0';
    printf("%s\n", hbuff);
    return ret; // ret <= 0 (error or connection closed)
}

char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

unsigned long get_file_size(char *path)
{
    struct stat st;
    stat(path, &st);
    return st.st_size;
}