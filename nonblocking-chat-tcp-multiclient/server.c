#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 55555 // port number server use to communicate
#define BUFFSIZE 1024
#define MAX_CLIENT 129   /* maximum number of client that server can serve */
                         /* (exclude listen socket)*/
#define POLL_TIMEOUT 100 // poll wait event occur for 100 millisec
#define GREETING "hello client from server, i'm still alive thanks!!"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct pollfd pollfd;

static void error_handler(char *message);
static void init_struct_pollfd(pollfd *fds, int listen_sockfd);
static int get_header_request(char *headers, int lenh, int fd);

static int num_connected_cli = 0;
static int num_connecting_cli = 0;

int main()
{
    /**********************/
    /*  Declare variable  */
    /**********************/
    int sockfd;
    char buffer[BUFFSIZE];
    sockaddr_in servaddr;
    int addr_len = sizeof(servaddr);
    int ret;
    int cli_socklen;
    sockaddr_in list_cliaddr[MAX_CLIENT];
    pollfd fds[MAX_CLIENT];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_handler("create socket fail");
    if ((ret = fcntl(sockfd, F_SETFL, O_NONBLOCK)) < 0)
        error_handler("set nonblocking socket fail");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if ((ret = bind(sockfd, (sockaddr *)&servaddr, addr_len)) < 0)
        error_handler("bind socket fail");

    if ((ret = listen(sockfd, 100)) < 0)
        error_handler("listen on socket fail");

    init_struct_pollfd(fds, sockfd);

    while (1)
    {
        ret = poll(fds, MAX_CLIENT, POLL_TIMEOUT);

        if (ret < 0) // error happen, terminate program
            error_handler("poll function fail");
        else if (ret == 0) // timeout, continue poll again
            continue;
        else // some event occur in our list fd
        {
            /*
             * check event convention:
             *   - fd < 0: mean can add new socket to that variable
             *   - fd > 0 && revents == 0: ignore that variable
             */

            // check new client connect
            if (fds[0].revents == POLLIN)
            {
                do
                {
                    // accept connecting client event
                    ret = accept(sockfd, (sockaddr *)&servaddr, &addr_len);
                    if (ret < 0) // some error occur
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            /*
                             * EAGAIN or EWOULDBLOCK
                             * The socket is marked nonblocking and no
                             * connections are present to be accepted.
                             */
                            break;
                        else
                            error_handler("accept socket fail");
                    }

                    // reached maximum number connecting
                    if (num_connecting_cli == MAX_CLIENT - 1)
                    {
                        // sleep 100 millisec, waiting for empty space
                        usleep(1000 * 100);
                        break;
                    }

                    for (int i = 1; i < MAX_CLIENT; i++)
                        if (fds[i].fd == -1) // this element still empty
                        {
                            fds[i].fd = ret;
                            num_connecting_cli++;
                            num_connected_cli++;
                            break;
                        }
                } while (1);
            }

            for (int i = 1; i < MAX_CLIENT; i++)
                if (fds[i].fd > 0 && fds[i].revents != 0)
                {
                    int clisock = fds[i].fd;

                    if (fds[i].revents == POLLIN) // process client request
                    {
                        char headers[64];
                        memset(headers, '\0', sizeof(headers));

                        ret = get_header_request(headers, sizeof(headers),
                                                 clisock);

                        if (ret == -1) // client closed socket
                        {
                            close(clisock);
                            fds[i].fd = -1;
                            num_connecting_cli--;
                        }
                        else
                        {
                            printf("connecting client: %d\n", num_connecting_cli);
                            if (strcmp(headers, "PING\r\n") == 0)
                                send(clisock, GREETING, strlen(GREETING), 0);
                        }
                    }
                }
        }
    }

    close(sockfd);
    return 0;
}

void error_handler(char *message)
{
    perror(message);
    exit(1);
}

static void init_struct_pollfd(pollfd *fds, int listen_sockfd)
{
    // init file discriptor for array fds
    /*
     * If the value of a file descriptor is less than 0, the events element is
     * ignored and the revents element is set to 0.
     */
    fds[0].fd = listen_sockfd;
    fds[0].events = POLLIN;

    for (int i = 1; i < MAX_CLIENT; i++)
    {
        fds[i].fd = -1; // unused socket
        fds[i].events = POLLIN;
    }
}

static int get_header_request(char *headers, int lenh, int fd)
{
    int count = 0;
    while (count < lenh - 1)
    {
        int ret = recv(fd, &headers[count], 1, 0);

        if (ret == 0) // client closed socket
            return -1;
        if (ret < 0) // some error occurred
            error_handler("receiv data socket fail");

        count += ret;

        if (count >= 2 && headers[count - 1] == '\n' &&
            headers[count - 2] == '\r') // end of request message
            break;
    }
    return 0;
}