#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSG_SIZE 100
#define PROJ_ID 69
#define NETDSRV_PORT 55555
#define NETDSRV_IP "127.0.0.1"

// structure for message queue
struct mesg_buffer
{
    long msg_type;
    char msg_text[MSG_SIZE];
} message;

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main()
{
    key_t key;
    int msgid;
    int sock;
    sockaddr_in servaddr;

    servaddr.sin_addr.s_addr = inet_addr(NETDSRV_IP);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(NETDSRV_PORT);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket fail");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect to netdsrv server fail");
        exit(EXIT_FAILURE);
    }

    // ftok to generate unique key
    key = ftok("./", PROJ_ID);

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);

    while (1)
    {
        // msgrcv to receive message
        msgrcv(msgid, &message, sizeof(message), 1, 0);
        printf("received message \"%s\" from server1\n", message.msg_text);
        if (send(sock, message.msg_text, strlen(message.msg_text), 0) < 0)
        {
            printf("netdsrv disconnected\n");
            break;
        }
    }

    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);
    close(sock);
    return 0;
}
