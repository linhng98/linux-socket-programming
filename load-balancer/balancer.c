#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFSIZE 512
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

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};

static int port = 8888; // default load balancer port

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
            port = (int)strtol(optarg, NULL, 10);
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

    printf("port: %d\n", port);

    exit(EXIT_SUCCESS);
}