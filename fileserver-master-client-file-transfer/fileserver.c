#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSIZE 512
#define BACKLOG 128 // maximum queue incoming connection
#define MAX_PATH_LEN 4096
#define USAGE "Usage: fileserver [-i <IP>] [-m <PORT>] [-p <PORT>] -d <DIR>\n"
#define VERSION                                                                                    \
    "fileserver v1.0.0\n"                                                                          \
    "Written by Linh, Khue, Tin, Cuong\n"
#define HELP                                                                                       \
    "fileserver [-i <IP>] [-m <PORT>] [-p <PORT>] -d <DIR>\n"                                      \
    "allow client download file in storage directory \n"                                           \
    "\n"                                                                                           \
    "-i, --master-ip    master server ip (default 127.0.0.1)\n"                                    \
    "-m, --master-port  master server listening port (default 55555)\n"                            \
    "-p, --listen-port  listen port  (default 44444)\n"                                            \
    "-d, --dir          storage directory to download \n"                                          \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

static void init_server_socket(int *sockfd, sockaddr_in *servaddr, socklen_t *addrlen);
static void *thread_serv_client(void *params);
static void *thread_sync_master();
static int is_file(char *path);
static int get_number_of_file(char *dir);
static void get_host_ip(char *ipaddr);
static int check_directory(char *dirname);
static int receive_line_data(char *buffer, int sock);
static unsigned long get_file_size(char *filename);

static struct option long_options[] = {{"master-ip", required_argument, 0, 'i'},
                                       {"master-port", required_argument, 0, 'm'},
                                       {"listen-port", required_argument, 0, 'p'},
                                       {"dir", required_argument, 0, 'd'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};
static char *master_ip = "127.0.0.1";
static int master_port = 55555;
static int listen_port = 44444;
static char *storage_dir = NULL;

int main(int argc, char *argv[])
{
    int c;
    int option_index;
    int ret;

    while (1) // loop to scan all argument
    {
        c = getopt_long(argc, argv, "i:m:p:d:hv", long_options, &option_index);

        if (c == -1) // end of options
            break;
        switch (c)
        {
        case 'i':
            master_ip = optarg;
            break;
        case 'm':
            master_port = (int)strtol(optarg, NULL, 10);
            break;
        case 'p':
            listen_port = (int)strtol(optarg, NULL, 10);
            break;
        case 'd':
            storage_dir = optarg;
            break;
        case 'h':
            printf(HELP);
            exit(EXIT_SUCCESS);
        case 'v':
            printf(VERSION);
            exit(EXIT_SUCCESS);
        case '?':
            printf("Try 'fileserver --help' for more information.\n");
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }

    if (storage_dir == NULL)
    {
        printf("missing -d option\n");
        printf(USAGE);
        exit(EXIT_FAILURE);
    }
    else if ((ret = check_directory(storage_dir)) != -2) // invalid dir
    {
        switch (ret)
        {
        case 0:
            printf("%s is empty dir\n", storage_dir);
            break;
        case -1:
            printf("%s is not a dir\n", storage_dir);
            break;
        case -3:
            printf("%s is not writeable\n", storage_dir);
            break;
        }
        exit(EXIT_FAILURE);
    }

    if (storage_dir[strlen(argv[1]) - 1] == '/') // strip off dash character
        storage_dir[strlen(argv[1]) - 1] = '\0';

    int sockfd;
    sockaddr_in servaddr;
    socklen_t addrlen = sizeof(servaddr);
    init_server_socket(&sockfd, &servaddr, &addrlen);

    pthread_t sync_thread; // this thread sync fileserver with master server
    if (pthread_create(&sync_thread, NULL, &thread_sync_master, NULL) != 0)
    {
        perror("create new thread failed"); // create new thread fail
        exit(EXIT_FAILURE);
    }
    pthread_join(sync_thread, NULL);

    while (1)
    {
        ret = accept(sockfd, (sockaddr *)&servaddr, &addrlen); // new client connect

        if (ret > 0) // new client connected
        {
            int *clientsock = (int *)malloc(sizeof(int));
            *clientsock = ret;

            pthread_t new_thread; // create new thread serve client
            if (pthread_create(&new_thread, NULL, &thread_serv_client, clientsock) != 0)
            {
                perror("create new thread failed"); // create new thread fail
                exit(EXIT_FAILURE);
            }
        }
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void init_server_socket(int *sockfd, sockaddr_in *servaddr, socklen_t *addrlen)
{
    // Creating socket file descriptor
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Filling server information
    servaddr->sin_family = AF_INET; // IPv4
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port = htons(listen_port);

    // Bind the socket with the server address
    if (bind(*sockfd, (sockaddr *)servaddr, *addrlen) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(*sockfd, BACKLOG) < 0)
    {
        perror("listen on socket fail");
        exit(EXIT_FAILURE);
    }
}

void *thread_serv_client(void *params)
{
    pthread_detach(pthread_self()); // detach this thread from main thread

    int clisock = *(int *)params;
    free(params);

    int ret;
    char buffer[BUFFSIZE];
    char *ping = "PING\r\n";
    char *ok = "OK\r\n";
    char *fail = "FAIL\r\n";
    char *get_req = "GET\r\n";

    if (receive_line_data(buffer, clisock) < 0) // receive command
        goto END_THREAD;

    if (strcmp(buffer, ping) == 0) // master ping check state
    {
        if (send(clisock, ok, strlen(ok), 0) <= 0)
            goto END_THREAD;
    }
    else if (strcmp(buffer, get_req) == 0)
    {
        if (receive_line_data(buffer, clisock) < 0) // receive filename
            goto END_THREAD;
        buffer[strlen(buffer) - 1] = '\0';
        char path[BUFFSIZE * 2];
        sprintf(path, "%s/%s", storage_dir, buffer);
        printf("GET %s\n", path);

        if (is_file(path) != 0) // is file
        {
            // send file size
            sprintf(buffer, "%lu\r\n", get_file_size(path));
            if (send(clisock, buffer, strlen(buffer), 0) <= 0)
                goto END_THREAD;
            // send data file
            FILE *rfile = fopen(path, "rb");
            while (1)
            {
                ret = fread(buffer, 1, BUFFSIZE, rfile);
                if (ret <= 0) // error or EOF
                    break;
                if (send(clisock, buffer, ret, 0) <= 0)
                    goto END_THREAD;
            }
            fclose(rfile);
        }
        else
            send(clisock, fail, strlen(fail), 0);
    }

END_THREAD:
    close(clisock);
    pthread_exit(NULL);
}

void *thread_sync_master()
{
    int sock;
    sockaddr_in masaddr;
    char buffer[BUFFSIZE];
    masaddr.sin_addr.s_addr = inet_addr(master_ip);
    masaddr.sin_family = AF_INET;
    masaddr.sin_port = htons(master_port);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("can not create new socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (sockaddr *)&masaddr, sizeof(masaddr)) != 0)
    {
        perror("can not connect to server");
        exit(EXIT_FAILURE);
    }

    // send header
    char ip[20];
    get_host_ip(ip);
    sprintf(buffer,
            "SYNC\r\n"
            "%s\r\n"
            "%d\r\n"
            "%d\r\n",
            ip, listen_port, get_number_of_file(storage_dir));
    if (send(sock, buffer, strlen(buffer), 0) <= 0)
        exit(EXIT_FAILURE);

    // send filename
    DIR *d;
    struct dirent *dir;
    d = opendir(storage_dir);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            sprintf(buffer, "%s/%s", storage_dir, dir->d_name);
            if (is_file(buffer) != 0)
            {
                if (send(sock, dir->d_name, strlen(dir->d_name), 0) <= 0)
                    exit(EXIT_FAILURE);
                if (send(sock, "\n", 1, 0) <= 0)
                    exit(EXIT_FAILURE);
            }
        }
        closedir(d);
    }

    // recv ok respond
    receive_line_data(buffer, sock);

    if (strcmp(buffer, "OK\r\n") != 0)
    {
        printf("sync to master failed\n");
        exit(EXIT_FAILURE);
    }
    printf("sync to master success\n");

    close(sock);
    pthread_exit(NULL);
}

int is_file(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int get_number_of_file(char *path)
{
    DIR *d;
    struct dirent *dir;
    char fpath[MAX_PATH_LEN];
    int count = 0;

    d = opendir(path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            sprintf(fpath, "%s/%s", path, dir->d_name);
            if (is_file(fpath) != 0)
                count++;
        }
        closedir(d);
    }
    return count;
}

void get_host_ip(char *ipaddr)
{
    char hostbuffer[256];
    struct hostent *host_entry;

    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);

    // To convert an Internet network
    // address into ASCII string
    strcpy(ipaddr, inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0])));
}

int check_directory(char *dirname)
{
    /*
     *   0: empty directory
     *  -1: directory not exist
     *  -2: directory exist but not empty
     *  -3: directory exist but not writeable
     */
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(dirname);
    if (dir == NULL) // Not a directory or doesn't exist
        return -1;
    // check write able or not
    if (access(dirname, W_OK) != 0)
        return -3; // not write permission
    while ((d = readdir(dir)) != NULL)
    {
        if (++n > 2)
            break;
    }
    closedir(dir);
    if (n <= 2) // Directory Empty
        return 0;
    else
        return -2; // not empty
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

unsigned long get_file_size(char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
