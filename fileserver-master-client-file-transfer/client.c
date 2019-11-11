#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSIZE 1024
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 255
#define USAGE "Usage: client [-i <IP>] [-p <PORT>] [-o <OUTPATH>] <FILE> ...\n"
#define VERSION                                                                                    \
    "client v1.0.0\n"                                                                              \
    "Written by Linh, Khue, Tin, Cuong\n"
#define HELP                                                                                       \
    "client [-i <IP>] [-p <PORT>] [-o <OUTPATH>] <FILE> ...\n"                                     \
    "Get fileserver info from master server then download file from fileserver\n"                  \
    "\n"                                                                                           \
    "-i, --host-ip      host ip of master server (default 127.0.0.1)\n"                            \
    "-p, --port         listening port of master server (default 55555)\n"                         \
    "-o, --out-dir      out dir for downloaded file (default ./)\n"                                \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"                                    \
    "\n"                                                                                           \
    "By default, this software can not download directory, just file only :)\n"                    \
    "This version still not support hostname yet :D\n"                                             \
    "To download a file whose name start with a '-', for example '-foo', use command: \n"          \
    "   client [OPTIONS] -- -foo\n"

typedef struct
{
    char ipaddr[16];
    int port;
    char *filename;
} fs_info;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static int init_connection(int *sock, sockaddr_in *servaddr);
static void *thread_download_file(void *params);
static void get_fileserver_info(fs_info *list_fs, int argc, char *argv[]);
static int is_dir(char *path);
static int receive_line_data(char *buffer, int sock);

static struct option long_options[] = {
    {"host-ip", required_argument, 0, 'i'}, {"port", required_argument, 0, 'p'},
    {"out-dir", required_argument, 0, 'o'}, {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},       {0, 0, 0, 0}};
static char *master_ip = "127.0.0.1"; // default master ip
static int master_port = 55555;       // default master port
static char *outdir = ".";            // default out path
static int download_status = -1;      // incomplete download process flag

int main(int argc, char *argv[])
{
    int c;
    int option_index;

    while (1) // loop to scan all argument
    {
        c = getopt_long(argc, argv, "i:p:o:hv", long_options, &option_index);

        if (c == -1) // end of options
            break;
        switch (c)
        {
        case 'i':
            master_ip = optarg;
            break;
        case 'p':
            master_port = (int)strtol(optarg, NULL, 10);
            break;
        case 'o':
            outdir = optarg;
            if (is_dir(outdir) == 0) // invalid dir
            {
                printf("%s is invalid dir\n", outdir);
                exit(EXIT_FAILURE);
            }
            if (outdir[strlen(outdir) - 1] == '/') // strip off '/' character if exist
                outdir[strlen(outdir) - 1] = '\0';
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

    if (optind == argc) // filename argument is missing
    {
        printf("Missing file name.\nTry 'client --help' for more information.\n");
        exit(EXIT_FAILURE);
    }

    int num_conn = argc - optind; // num of filename equal to num connection

    // store list connect socket
    int *clisock = (int *)malloc(sizeof(int) * num_conn);
    // store list thread id
    pthread_t *clithread = (pthread_t *)malloc(sizeof(pthread_t) * num_conn);
    // store list params for each thread
    fs_info *list_fs = (fs_info *)malloc(sizeof(fs_info) * num_conn);
    get_fileserver_info(list_fs, argc, argv);

    // create new thread for each connection
    int count = 0;
    for (int i = optind; i < argc; i++) // create new thread for each fileserver
    {
        if (list_fs[count].port > 0) // check this fileserver valid
            printf("GET %s FROM [%s %d]\n", list_fs[count].filename, list_fs[count].ipaddr, list_fs[count].port);
        else
            printf("GET %s [FILE NOT FOUND]\n",list_fs[count].filename);
        pthread_create(&clithread[count], NULL, thread_download_file, &list_fs[count]);
        count++;
    }

    for (int i = 0; i < num_conn; i++)
        pthread_join(clithread[i], NULL); // wait for all thread complete download
    download_status = 0;                  // complete download

    free(clisock);
    free(clithread);
    free(list_fs);
    exit(EXIT_SUCCESS);
}

int init_connection(int *sock, sockaddr_in *servaddr)
{
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0)
    {
        perror("can not create new socket\n");
        return -1;
    }

    if (connect(*sock, (sockaddr *)servaddr, sizeof(*servaddr)) != 0)
    {
        perror("can not connect to server\n");
        return -2;
    }
    return 0;
}

void *thread_download_file(void *params)
{
    fs_info *info = (fs_info *)params;
    int ret;
    int sock;
    char *command = "GET\r\n";
    char buffer[BUFFSIZE];
    memset(buffer, '\0', BUFFSIZE);
    sockaddr_in fs_addr;
    fs_addr.sin_family = AF_INET;
    fs_addr.sin_addr.s_addr = inet_addr(info->ipaddr);
    fs_addr.sin_port = htons(info->port);

    init_connection(&sock, &fs_addr);
    send(sock, command, strlen(command), 0);               // send command
    send(sock, info->filename, strlen(info->filename), 0); // send filename
    send(sock, "\n", 1, 0);

    char outpath[MAX_PATH_LEN];
    sprintf(outpath, "%s/%s", outdir, info->filename);

    FILE *wfile = fopen(outpath, "wb");
    while (1)
    {
        ret = recv(sock, buffer, BUFFSIZE, 0);
        if (ret <= 0) // error or peer closed
            break;
        fwrite(buffer, 1, ret, wfile);
    }

    fclose(wfile);
    close(sock);
    pthread_exit(NULL);
}

void get_fileserver_info(fs_info *list_fs, int argc, char *argv[])
{
    int sock;
    sockaddr_in maddr;
    char *command = "INFO\r\n";
    char *fnf = "FILE NOT FOUND\r\n";
    int count = 0;
    char buffer[MAX_FILENAME_LEN];
    maddr.sin_addr.s_addr = inet_addr(master_ip);
    maddr.sin_family = AF_INET;
    maddr.sin_port = htons(master_port);

    // connect to master server
    init_connection(&sock, &maddr);
    // send command to master server
    send(sock, command, strlen(command), 0);
    // send all filename to master server

    for (int i = optind; i < argc; i++)
    {
        // link filename to fs_info, then send to master
        list_fs[count].filename = argv[i];
        sprintf(buffer, "%s\n", argv[i]);
        send(sock, buffer, strlen(buffer), 0);
        count++;
    }

    for (int i = 0; i < count; i++)
    {
        receive_line_data(buffer, sock);
        if (strcmp(buffer, fnf) == 0) // file not found
            list_fs[i].port = -1;
        else
        {
            buffer[strlen(buffer) - 1] = '\0'; // remove \n char

            char *token = strtok(buffer, " "); // split ip and port
            strcpy(list_fs[i].ipaddr, token);
            token = strtok(NULL, " ");
            list_fs[i].port = (int)strtol(token, NULL, 10);
        }
    }

    close(sock);
}

int is_dir(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

int receive_line_data(char *buffer, int sock)
{
    int count = 0;
    while (1)
    {
        if (recv(sock, &buffer[count], 1, 0) <= 0)
            return -1;

        if (buffer[count] == '\n')
            break;

        count++;
    }
    buffer[count + 1] = '\0';
    return 0;
}