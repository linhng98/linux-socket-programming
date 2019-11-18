#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SRV_PORT 44444
#define SRV_IP "127.0.0.1"
#define MSG_SIZE 100

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main()
{
    int sock;
    sockaddr_in servaddr;
    char buffer[MSG_SIZE];

    servaddr.sin_addr.s_addr = inet_addr(SRV_IP);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SRV_PORT);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket fail");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect to server fail");
        exit(EXIT_FAILURE);
    }

    printf("input: ");
    fgets(buffer, MSG_SIZE, stdin);

    send(sock, buffer, strlen(buffer), 0);

    recv(sock, buffer, MSG_SIZE, 0);

    printf("reply from server: %s", buffer);

    close(sock);
    exit(EXIT_SUCCESS);
}