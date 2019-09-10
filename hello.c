#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>


int ReadHttpStatus(int sock)
{
    char c;
    char buff[1024] = "", *ptr = buff + 1;
    int bytes_received, status;
    printf("Begin Response ..\n");
    while (bytes_received = recv(sock, ptr, 1, 0))
    {
        if (bytes_received == -1)
        {
            perror("ReadHttpStatus");
            exit(1);
        }

        if ((ptr[-1] == '\r') && (*ptr == '\n'))
            break;
        ptr++;
    }
    *ptr = 0;
    ptr = buff + 1;

    sscanf(ptr, "%*s %d ", &status);

    printf("%s\n", ptr);
    printf("status=%d\n", status);
    printf("End Response ..\n");
    return (bytes_received > 0) ? status : 0;
}

//the only filed that it parsed is 'Content-Length'
int ParseHeader(int sock)
{
    char c;
    char buff[1024] = "", *ptr = buff + 4;
    int bytes_received, status;
    printf("Begin HEADER ..\n");
    while (bytes_received = recv(sock, ptr, 1, 0))
    {
        if (bytes_received == -1)
        {
            perror("Parse Header");
            exit(1);
        }

        if ((ptr[-3] == '\r') && (ptr[-2] == '\n') &&
            (ptr[-1] == '\r') && (*ptr == '\n'))
            break;
        ptr++;
    }

    *ptr = 0;
    ptr = buff + 4;
    //printf("%s",ptr);

    if (bytes_received)
    {
        ptr = strstr(ptr, "Content-Length:");
        if (ptr)
        {
            sscanf(ptr, "%*s %d", &bytes_received);
        }
        else
            bytes_received = -1; //unknown size

        printf("Content-Length: %d\n", bytes_received);
    }
    printf("End HEADER ..\n");
    return bytes_received;
}

int main(void)
{

    char domain[] = "sstatic.net";
    char path[] = "stackexchange/img/logos/so/so-logo-med.png";

    int sock, bytes_received;
    char send_data[1024], recv_data[1024], *p;
    struct sockaddr_in server_addr;
    struct hostent *he;

    he = gethostbyname(domain);
    if (he == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    bzero(&(server_addr.sin_zero), 8);

    printf("Connecting ...\n");
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Connect");
        exit(1);
    }

    printf("Sending data ...\n");

    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

    if (send(sock, send_data, strlen(send_data), 0) == -1)
    {
        perror("send");
        exit(2);
    }
    printf("Data sent.\n");

    //fp=fopen("received_file","wb");
    printf("Recieving data...\n\n");

    int contentlengh;

    if (ReadHttpStatus(sock) && (contentlengh = ParseHeader(sock)))
    {

        int bytes = 0;
        FILE *fd = fopen("test.png", "wb");
        printf("Saving data...\n\n");

        while (bytes_received = recv(sock, recv_data, 1024, 0))
        {
            if (bytes_received == -1)
            {
                perror("recieve");
                exit(3);
            }

            fwrite(recv_data, 1, bytes_received, fd);
            bytes += bytes_received;
            printf("Bytes recieved: %d from %d\n", bytes, contentlengh);
            if (bytes == contentlengh)
                break;
        }
        fclose(fd);
    }

    close(sock);
    printf("\n\nDone.\n\n");
    return 0;
}