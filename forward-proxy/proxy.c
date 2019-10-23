#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h> // for open
#include <linux/limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> // for close

#define PORT 8080
#define MAX_HOSTLEN 255
#define MAX_HEADER_SIZE 4096
#define BUFFSIZE 2048
#define POLL_TIMEOUT 1000 // 1000 millisec
#define CODE_403 "HTTP/1.1 403 Forbidden\r\n\r\n"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;
typedef struct in_addr in_addr;
typedef struct pollfd pollfd;
typedef struct timeval timeval;

typedef struct
{
    char protocol[10];
    char host[MAX_HOSTLEN];
    int port;
    char path[PATH_MAX];
} url_info;

typedef struct
{
    char startline[300];
    char http_header[MAX_HEADER_SIZE];
} http_message_header;

static int count_sub_string(const char *a, const char *b);
static void parse_url(url_info *info, const char *full_url);
static int recv_headers(http_message_header *head, int sfd);
void get_URL_info(sockaddr_in *addr, url_info *info);
int check_valid_request(url_info *info, http_message_header *head);
void get_http_method(char *method, http_message_header *head);
int check_valid_hostname(char *host);

int main(int argc, char const *argv[])
{
    int sockfd, len;
    sockaddr_in servaddr, cli;
    pid_t child;
    pollfd fds[1];

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(-1);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(-1);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 0)) != 0)
    {
        printf("Listen failed...\n");
        exit(-1);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    fcntl(sockfd, F_SETFL, O_NONBLOCK); // set socket to non-blocking mode
    fds[0].events = POLLIN;
    fds[0].fd = sockfd;

    while (1)
    {
        int pid;
        do // loop to reap zombie process (if exist)
        {
            pid = waitpid(-1, NULL, WNOHANG);
        } while (pid > 0);

        if (poll(fds, 1, POLL_TIMEOUT) == 0) // no new event coming
            continue;

        // Accept the data packet from client and verification
        int connfd = accept(sockfd, (sockaddr *)&cli, (socklen_t *)&len);
        if (connfd < 0) // sockfd already set to non-blocking
            continue;   // no incoming connection

        child = fork();
        if (child < 0) // create child process fail
        {
            close(connfd);
            continue;
        }
        else if (child == 0) // in child process zone, do something
        {
            int ret; // use to store returned value

            timeval tv = {5, 0};
            setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (timeval *)&tv, sizeof(timeval));

            // get headers http from client
            http_message_header heads;
            if (recv_headers(&heads, connfd) < 0)
            {
                send(connfd, CODE_403, strlen(CODE_403), 0);
                exit(-1);
            }

            // parse url from headers
            char url[300];
            url_info info;
            sscanf(heads.startline, "%*s %s %*s", url);
            parse_url(&info, url);

            if (check_valid_request(&info, &heads) < 0) // invalid request
            {
                send(connfd, CODE_403, strlen(CODE_403), 0);
                exit(-1);
            }

            // create hostaddr and connect
            sockaddr_in hostaddr;
            get_URL_info(&hostaddr, &info);
            int clifd = socket(AF_INET, SOCK_STREAM, 0);
            if (clifd == -1)
            {
                printf("socket creation failed...\n");
                exit(-1);
            }

            if (connect(clifd, (sockaddr *)&hostaddr, sizeof(hostaddr)) < 0)
            {
                printf("connection failed\n");
                exit(-1);
            }

            char req_head[300 + MAX_HEADER_SIZE];
            sprintf(req_head, "%s%s", heads.startline, heads.http_header);

            if (send(clifd, req_head, strlen(req_head), 0) < 0)
            {
                printf("send headers failed\n");
                exit(-1);
            }

            char buff[BUFFSIZE];
            while (1)
            {
                char c;
                if (recv(connfd, &c, 1, MSG_PEEK | MSG_DONTWAIT) == 0) // client disconnected
                    break;

                ret = recv(clifd, buff, BUFFSIZE, MSG_DONTWAIT); // set nonblocking
                if (ret < 0)                                     // message still not arrive
                    continue;

                if (send(connfd, buff, ret, 0) == 0) // client disconnected
                    break;

                memset(buff, '\0', ret);
            }

            close(clifd);
            close(connfd);

            exit(0); // exit child process
        }
        else // in parent process, close socket
            close(connfd);
    }
    close(sockfd);
    return 0;
}

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
            sscanf(full_url, "%99[^:]://%99[^/]/%99[^\n]", info->protocol, info->host, info->path);
        else // no protocol, have port
            sscanf(full_url, "%99[^:]:%99d/%99[^\n]", info->host, &info->port, info->path);
        break;
    case 2: // have both protocol and port
        sscanf(full_url, "%99[^:]://%99[^:]:%99d/%99[^\n]", info->protocol, info->host, &info->port,
               info->path);
        break;
    default:
        printf("invalid URL format!\n");
    }
}

static int recv_headers(http_message_header *head, int sfd)
{
    int count;
    int ret;
    memset(head->startline, '\0', sizeof(head->startline));
    memset(head->http_header, '\0', sizeof(head->http_header));

    // recv start line
    count = 0;
    while (1)
    {
        ret = recv(sfd, &head->startline[count], 1, 0);
        if (ret < 0) // get nothing in last 5 sec
        {
            printf("i get nothing last 5 sec !!!\n");
            return -1;
        }

        if (ret == 0) // client disconnected
        {
            printf("client disconnected\n");
            return -1;
        }
        count++;
        if (count >= 2 && strcmp(&head->startline[count - 2], "\r\n") == 0)
            break;
    }

    // recv headers field
    count = 0;
    while (1)
    {
        ret = recv(sfd, &head->http_header[count], 1, 0);
        if (ret == 0) // client disconnected
            return -1;
        count++;
        if (count >= 4 && strcmp(&head->http_header[count - 4], "\r\n\r\n") == 0) // end request
            break;
    }

    return 0;
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

int check_valid_request(url_info *info, http_message_header *head)
{
    printf("%s", head->startline);
    if (strcmp(info->protocol, "http") != 0)
    {
        printf("(wrong protocol !!!!)\n");
        return -1;
    }

    char method[10];
    get_http_method(method, head);
    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        printf("(invalid request method !!!)\n");
        return -1;
    }

    if (check_valid_hostname(info->host) < 0)
    {
        printf("(invalid host name !!!)\n");
        return -1;
    }
    return 0;
}

void get_http_method(char *method, http_message_header *head)
{
    char tmp[300];
    strcpy(tmp, head->startline);
    strcpy(method, strtok(tmp, " "));
}

int check_valid_hostname(char *host)
{
    int count = 0;
    for (int i = 0; i < strlen(host); i++)
        if (host[i] == '.')
            count++;

    if (count <= 1)
        return -1;
    return 0;
}