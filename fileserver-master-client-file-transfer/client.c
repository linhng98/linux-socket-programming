#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSIZE 512
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 255
#define USAGE "Usage: client [-i <IP>] [-p <PORT>] [-o <OUTPATH>] [-l <LIST>] <FILE> ...\n"
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
    "-l, --list         list all file on master server\n"                                          \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"                                    \
    "\n"                                                                                           \
    "By default, this software can not download directory, just file only :)\n"                    \
    "This version still not support hostname yet :D\n"                                             \
    "To download a file whose name start with a '-', for example '-foo', use command: \n"          \
    "   client [OPTIONS] -- -foo\n"

typedef struct
{
    int downloading_flag;
    int percent;
} progress_status;

typedef struct
{
    progress_status sts; // use to track downloading status
    char ipaddr[16];
    int port;       // > 0 mean fileserver exist, -1 mean fileserver holding file not found
    char *filename; // filename which fs is holding
} fs_info;

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static int init_connection(int *sock, sockaddr_in *servaddr);
static void list_all_file();
static void *thread_download_file(void *params);
static void get_fileserver_info(fs_info *list_fs, int argc, char *argv[]);
static int is_dir(char *path);
static int receive_line_data(char *buffer, int sock);
static void print_progress_bar(fs_info *list_fs, int num_conn);

static struct option long_options[] = {{"host-ip", required_argument, 0, 'i'},
                                       {"port", required_argument, 0, 'p'},
                                       {"out-dir", required_argument, 0, 'o'},
                                       {"list", no_argument, 0, 'l'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};
static char *master_ip = "127.0.0.1"; // default master ip
static int master_port = 55555;       // default master port
static char *outdir = ".";            // default out path

int main(int argc, char *argv[])
{
    int c;
    int option_index;
    int lflag = 0; // list file flag

    while (1) // loop to scan all argument
    {
        c = getopt_long(argc, argv, "i:p:o:lhv", long_options, &option_index);

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
        case 'l':
            lflag = 1;
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

    if (lflag == 1) // user want to list all file
    {
        list_all_file();
        exit(EXIT_SUCCESS);
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
    // init list file server
    for (int i = 0; i < num_conn; i++)
    {
        list_fs[i].port = -1; // fileserver not exist
        list_fs[i].sts.downloading_flag = 0;
        list_fs[i].sts.percent = 0;
    }
    get_fileserver_info(list_fs, argc, argv);

    // create new thread for each connection
    int count = 0;
    for (int i = optind; i < argc; i++) // create new thread for each fileserver
    {
        if (list_fs[count].port > 0) // check this fileserver valid
        {
            printf("GET %s FROM [%s %d]\n", list_fs[count].filename, list_fs[count].ipaddr,
                   list_fs[count].port);
            pthread_create(&clithread[count], NULL, thread_download_file, &list_fs[count]);
        }
        else
            printf("GET %s [FILE NOT FOUND]\n", list_fs[count].filename);
        count++;
    }

    // print progress bar
    // get width height of current terminal
    sleep(1); // wait for all thread spawned
    print_progress_bar(list_fs, num_conn);

    for (int i = 0; i < num_conn; i++)
        if (list_fs[i].port > 0)
            pthread_join(clithread[i], NULL); // join all completed thread

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

void list_all_file()
{
    int ret;
    int sock;
    sockaddr_in maddr;
    char buffer[BUFFSIZE];
    char *command = "LIST\r\n";
    maddr.sin_addr.s_addr = inet_addr(master_ip);
    maddr.sin_family = AF_INET;
    maddr.sin_port = htons(master_port);

    // connect to master server
    init_connection(&sock, &maddr);
    // send command to master server
    send(sock, command, strlen(command), 0);
    // send all filename to master server

    while (1)
    {
        if ((ret=recv(sock, buffer, BUFFSIZE, 0)) <= 0)
            break;
        buffer[ret]='\0';
        printf("%s", buffer);
    }

    close(sock);
}

void *thread_download_file(void *params)
{
    fs_info *info = (fs_info *)params;
    info->sts.downloading_flag = 1;
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

    // get file size
    receive_line_data(buffer, sock);
    buffer[strlen(buffer) - 2] = '\0';
    unsigned long filesize = strtol(buffer, NULL, 10);
    unsigned long sum = 0;

    FILE *wfile = fopen(outpath, "wb");
    while (1)
    {
        ret = recv(sock, buffer, BUFFSIZE, 0);
        if (ret <= 0) // error or peer closed
            break;
        sum += ret;
        info->sts.percent = sum * 100 / filesize;
        fwrite(buffer, 1, ret, wfile);
    }

    info->sts.downloading_flag = 0;
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
            continue;
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

void print_progress_bar(fs_info *list_fs, int num_conn)
{
    struct winsize w;
    int bar_len = 40;
    int linelen = 20 + 20 + bar_len + 1 + 5; // %-20s%20[barlen] 100%
    int down_line;
    int brick;
    int count;
    while (1)
    {
        // get currently width size terminal
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        down_line = (linelen / w.ws_col + 1) * num_conn;
        count = 0;

        for (int i = 0; i < num_conn; i++)
        {
            if (list_fs[i].sts.downloading_flag == 0)
                count++;
            brick = bar_len * list_fs[i].sts.percent / 100;
            printf("%-20s%20s", list_fs[i].filename, "[");
            for (int i = 0; i < brick; i++)
                printf("#");
            for (int i = 0; i < bar_len - brick; i++)
                printf("-");
            printf("] %3d%%\r\n", list_fs[i].sts.percent);
        }

        if (count == num_conn) // all thread is completed download
            break;
        usleep(300);
        printf("%c[%dA", 27, down_line); // move cursor up n line
    }
}