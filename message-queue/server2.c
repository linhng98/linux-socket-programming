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
#define PROJ_ID 96

// structure for message queue
struct mesg_buffer
{
    long msg_type;
    char msg_text[MSG_SIZE];
} message;

int main()
{
    key_t key;
    int msgid;
    int sock;

    // ftok to generate unique key
    key = ftok("./", PROJ_ID);

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);

    while (1)
    {
        // msgrcv to receive message
        msgrcv(msgid, &message, sizeof(message), 1, 0);
        // display the message
        printf("received message \"%s\" from netdsrv\n", message.msg_text);
    }

    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);
    close(sock);
    return 0;
}
