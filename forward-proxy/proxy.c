#include <fcntl.h> // for open
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for close

#define PORT 8080
#define SA struct sockaddr

// Driver function
int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    while (1)
    {
        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA *)&cli, (socklen_t *)&len);
        if (connfd < 0)
        {
            printf("server acccept failed...\n");
            exit(0);
        }
        else
            printf("server acccept the client...\n");

        char buffer[1024];
        memset(buffer, '\0', sizeof(buffer));
        int count = 0;
        int ret;

        while (1)
        {
            ret = recv(connfd, &buffer[count], 1, 0);
            if (ret == 0) // client disconnected
            {
                printf("client disconnected\n");
                break;
            }

            count++;

            if (count >= 4 && strcmp(&buffer[count - 4], "\r\n\r\n") == 0) // end request
            {
                buffer[count] = '\0';
                printf("%s", buffer);
                break;
            }
        }
    }

    // After chatting close the socket
    close(sockfd);
}
