#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define SERVER_IP "192.168.1.9"
#define BUFFSIZE 1024

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main()
{
    int sockfd;
    sockaddr_in servaddr;
    char buffer[BUFFSIZE];
    memset(buffer, '\0', BUFFSIZE);
    FILE *fp;
    int ret;

    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    int len;
    sendto(sockfd, "hello server", BUFFSIZE, MSG_CONFIRM, (sockaddr *)&servaddr,
           sizeof(servaddr));

    ret = recvfrom(sockfd, buffer, BUFFSIZE, MSG_WAITALL, (sockaddr *)&servaddr,
                   &len);
    printf("%d\n%s\n", len, buffer);
    memset(buffer, '\0', BUFFSIZE);

    ret = recvfrom(sockfd, buffer, BUFFSIZE, MSG_WAITALL, (sockaddr *)&servaddr,
                   &len);
    printf("%d\n%s\n", len, buffer);
    memset(buffer, '\0', BUFFSIZE);
    close(sockfd);
}