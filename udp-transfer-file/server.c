#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 12345
#define BUFFSIZE 1024

void error_handler(char *message);

int main()
{
    int sockfd;
    char buffer[BUFFSIZE];
    char *hello = "hello too";
    struct sockaddr_in servaddr, cliaddr;
    int ret;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    int len;
    ret = recvfrom(sockfd, (char *)buffer, BUFFSIZE, MSG_WAITALL,
                   (struct sockaddr *)&cliaddr, &len);
    buffer[ret] = '\0';
    printf("%d\n%s\n", len, buffer);

    sendto(sockfd, hello, strlen(hello), MSG_CONFIRM,
           (const struct sockaddr *)&cliaddr, len);

    sendto(sockfd, hello, strlen(hello), MSG_CONFIRM,
           (const struct sockaddr *)&cliaddr, len);

    return 0;
}

void error_handler(char *message)
{
    perror(message);
    exit(1);
}