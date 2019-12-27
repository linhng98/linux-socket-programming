#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFSIZE 512
#define HEADER_SIZE 4000
#define MAX_EVENTS 100    // 100 fd list
#define POLL_TIMEOUT 1000 // 1000 millisec
#define HTTP_TIMEOUT 3    // http timeout 3s
#define USAGE "Usage: balancer [-p <PORT>]\n"
#define VERSION                                                                                    \
    "balancer v1.0.0\n"                                                                            \
    "Written by Linh, Khue\n"
#define HELP                                                                                       \
    "balancer [-p <PORT>]\n"                                                                       \
    "Load balancer forward request to webserver\n"                                                 \
    "\n"                                                                                           \
    "-p, --port         listening port of webserver server (default 8888)\n"                       \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"                                    \
    "\n"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static void init_server_socket(int *sockfd, sockaddr_in *servaddr);
static int get_req_headers(char *hbuff, int sockfd);

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};

static int port = 80; // default load balancer port

int main(int argc, char *argv[])
{
    int c;
    int option_index;

    while (1)
    {
        c = getopt_long(argc, argv, "p:hv", long_options, &option_index);
        if (c == -1) // end of options
            break;
        switch (c)
        {
        case 'p':
            port = (int)strtol(optarg, NULL, 10);
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

    // init server socket
    int sockfd;
    int epollfd;
    int nfds;
    int conn_sock;
    sockaddr_in servaddr;
    struct epoll_event ev, event_list[MAX_EVENTS];
    init_server_socket(&sockfd, &servaddr);

    if (epollfd = epoll_create(MAX_EVENTS) < 0) // create epollfd
    {
        perror("create epollfd fail");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN; // add listen socket to epoll list
    ev.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
    {
        perror("add listen socket to epollfd");
        exit(EXIT_FAILURE);
    }

    while (1)
    { // start polling, waiting for event
        nfds = epoll_wait(epollfd, event_list, MAX_EVENTS, POLL_TIMEOUT);
        if (nfds < 0)
        {
            perror("epoll wait error");
            exit(EXIT_FAILURE);
        }
        else if (nfds == 0) // timeout
            continue;
        else
        {
            for (int i = 0; i < nfds; i++)
            {
                if (event_list[i].data.fd == sockfd) // new connection coming
                {
                    conn_sock = accept(sockfd, NULL, NULL); // accept
                    if (conn_sock < 0)
                    {
                        perror("accept socket fail");
                        continue;
                    }

                    ev.events = EPOLLIN;
                    ev.data.fd = conn_sock; // conn sock to list epoll
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) < 0)
                    {
                        perror("add listen socket to epollfd");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    
                }
            }
        }
    }

    close(epollfd);
    exit(EXIT_SUCCESS);
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
    fcntl(*sockfd, F_SETFL, O_NONBLOCK);

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
    return ret; // ret <= 0 (error or connection closed)
}