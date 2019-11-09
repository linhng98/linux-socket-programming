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
    int sock;
    char *filename;
} thread_df_params;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static void init_connection(int *sock, sockaddr_in *servaddr);
static void *thread_download_file(void *params);
int is_dir(char *path);

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
    sockaddr_in servaddr;
    servaddr.sin_addr.s_addr = inet_addr(master_ip);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(master_port);

    // store list connect socket
    int *clisock = (int *)malloc(sizeof(int) * num_conn);
    // store list thread id
    pthread_t *clithread = (pthread_t *)malloc(sizeof(pthread_t) * num_conn);
    // store list params for each thread
    thread_df_params *t_params = (thread_df_params *)malloc(sizeof(thread_df_params) * num_conn);

    printf("HOST: %s\nIP: %d\nPATH: %s\n", master_ip, master_port, outdir);
    // create new thread for each connection
    int count = 0;
    for (int i = optind; i < argc; i++) // retrieve the rest filename
    {
        init_connection(&clisock[count], &servaddr);

        t_params[count].sock = clisock[count];
        t_params[count].filename = argv[i];

        pthread_create(&clithread[count], NULL, thread_download_file, &t_params[count]);
        count++;
    }

    for (int i = 0; i < num_conn; i++)
        pthread_join(clithread[i], NULL); // wait for all thread complete download
    download_status = 0;                  // complete download

    free(clisock);
    free(clithread);
    free(t_params);
    exit(EXIT_SUCCESS);
}

void init_connection(int *sock, sockaddr_in *servaddr)
{
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0)
    {
        perror("can not create new socket\n");
        exit(EXIT_FAILURE);
    }

    if (connect(*sock, (sockaddr *)servaddr, sizeof(*servaddr)) != 0)
    {
        perror("can not connect to server\n");
        exit(EXIT_FAILURE);
    }
}

void *thread_download_file(void *params)
{
    thread_df_params *info = (thread_df_params *)params;
    int ret;
    char buffer[BUFFSIZE];
    memset(buffer, '\0', BUFFSIZE);

    send(info->sock, info->filename, strlen(info->filename), 0);

    char outpath[MAX_PATH_LEN];
    sprintf(outpath, "%s/%s", outdir, info->filename);

    FILE *wfile = fopen(outpath, "wb");
    while (1)
    {
        ret = recv(info->sock, buffer, BUFFSIZE, 0);
        if (ret <= 0) // error or peer closed
            break;
        fwrite(buffer, 1, ret, wfile);
    }

    fclose(wfile);
    close(info->sock);
    pthread_exit(NULL);
}

int is_dir(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}
