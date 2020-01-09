#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFSIZE 512
#define HEADER_SIZE 4000
#define MAX_EVENTS 100    // 100 fd list
#define POLL_TIMEOUT 1000 // 1000 millisec
#define HTTP_TIMEOUT 3    // http timeout 3s
#define NUM_WEBSERVER 3   //
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
typedef struct webserver_info
{
    char ip[20];
    int port;
} webserver_info;

static void init_server_socket(int *sockfd, sockaddr_in *servaddr);
static int get_req_headers(char *hbuff, int sockfd);
static int get_lowest_index(int *arr);
static int search_header_value(char *value, char *header, char *hbuff);

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};

static int listen_port = 8000; // default load balancer port

static webserver_info wsinfo_list[3] = {    // list webserver port and ip
    {"127.0.0.1", 11111}, {"127.0.0.1", 22222}, {"127.0.0.1", 33333}};
static int count_req_webserver[3] = {0};    // use to count request to each webserver

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
            listen_port = (int)strtol(optarg, NULL, 10);
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
    int epollfd, epollfd_ws;
    int nfds;
    int conn_sock;
    char buffer[BUFFSIZE];
    sockaddr_in servaddr;
    sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    struct epoll_event ev, event_list[MAX_EVENTS];
    init_server_socket(&sockfd, &servaddr);

    if ((epollfd = epoll_create(MAX_EVENTS)) < 0) // create epollfd listen for client
    {
        perror("create epollfd fail");
        exit(EXIT_FAILURE);
    }

    if ((epollfd_ws = epoll_create(MAX_EVENTS)) < 0) // create epollfd listen for webserver
    {
        perror("create epollfd webserver fail");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN; // add listen socket to epoll list
    ev.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
    {
        perror("add listen socket to epollfd");
        exit(EXIT_FAILURE);
    }

    int ret;
    char hbuff[HEADER_SIZE];
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
                if (event_list[i].data.fd == sockfd) // client init connection
                {
                    conn_sock = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen); // accept
                    if (conn_sock < 0)
                    {
                        perror("accept socket fail");
                        continue;
                    }
                    // print log time, ip and port
                    time_t rawtime;
                    struct tm *timeinfo;

                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    inet_ntop(AF_INET, &clientaddr.sin_addr, buffer, INET_ADDRSTRLEN); // get ipaddr
                    printf("%02d:%02d:%02d %s %d\n", timeinfo->tm_hour, timeinfo->tm_min,
                           timeinfo->tm_sec, buffer, ntohs(clientaddr.sin_port));

                    ev.events = EPOLLIN;
                    ev.data.fd = conn_sock; // conn sock to list epoll
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) <
                        0) // add conn_sock to list epoll, catch event
                    {
                        perror("add listen socket to epollfd");
                        exit(EXIT_FAILURE);
                    }
                }
                else // new readable request header
                {
                    int clisock = event_list[i].data.fd;
                    ret = get_req_headers(hbuff, clisock);

                    // connect to webserver
                    int idx;
                    if ((idx = get_lowest_index(count_req_webserver)) < 0)
                    { // no webserver is up
                        printf("no webserver is running\n");
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, clisock, &ev);
                        close(clisock);
                    }

                    int wssock = socket(AF_INET, SOCK_STREAM, 0);
                    sockaddr_in wsaddr;
                    inet_pton(AF_INET, wsinfo_list[idx].ip, &wsaddr.sin_addr);
                    wsaddr.sin_family = AF_INET;
                    wsaddr.sin_port = htons(wsinfo_list[idx].port);

                    if (connect(wssock, (struct sockaddr *)&wsaddr, INET_ADDRSTRLEN) < 0)
                    { // connect to webserver fail, server down, remove from list webserver
                        printf("webser %c downed\n", 'A' + idx);
                        count_req_webserver[idx] = -1;
                    }
                    else
                    {
                        // send client req header to webserver
                        if (send(wssock, hbuff, strlen(hbuff), 0) > 0)
                            count_req_webserver[idx]++; // increase num req

                        // receive header from webserver
                        memset(hbuff, '\0', sizeof(hbuff));
                        get_req_headers(hbuff, wssock);

                        // get length of file
                        char value[100];
                        ret = search_header_value(value, "Content-Length", hbuff);

                        // send header to client
                        send(clisock, hbuff, strlen(hbuff), 0);

                        // receive data from webserver then return to client

                        // get body data
                        long content_len = atoi(value);
                        long sum = 0;
                        while (1)
                        {
                            memset(buffer, '\0', BUFFSIZE);

                            if (sum == content_len)
                                break;

                            if ((ret = recv(wssock, buffer, BUFFSIZE - 1, 0)) > 0)
                            {
                                send(clisock, buffer, ret, 0);
                                sum += ret;
                            }
                            else
                                break;
                        }
                    }

                    // close epoll socket
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, clisock, &ev);
                    close(clisock);
                    close(wssock);
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
    servaddr->sin_port = htons(listen_port);
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

static int get_lowest_index(int *arr)
{
    int max = INT_MAX;
    int lowest_idx = -1;
    for (int i = 0; i < NUM_WEBSERVER; i++)
        if (arr[i] < max && arr[i] >= 0)
        {
            lowest_idx = i;
            max = arr[i];
        }
    return lowest_idx;
}

static int search_header_value(char *value, char *header, char *hbuff)
{
    char *pos = strstr(hbuff, header);

    // header not exist
    if (pos == NULL)
        return -1;

    pos += strlen(header) + 2; // strlen(": ") == 2
    sscanf(pos, "%99[^\r]", value);

    return 0;
}