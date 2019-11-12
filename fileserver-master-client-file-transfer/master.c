#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 128   // maximum queue incoming connection
#define BUFFSIZE 512 // size of buffer
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 255
#define USAGE "Usage: master [-p <PORT>]\n"
#define VERSION                                                                                    \
    "master v1.0.0\n"                                                                              \
    "Written by Linh, Khue, Tin, Cuong\n"
#define HELP                                                                                       \
    "Usage: master -p <PORT>\n"                                                                    \
    "Master server centralize fileserver info storage\n"                                           \
    "\n"                                                                                           \
    "-p, --port         listen port  (default 55555)\n"                                            \
    "-d, --dir          directory to store fileserver info (must is empty dir) \n"                 \
    "                   (default $HOME/.master)"                                                   \
    "-h, --help         display help message then exit\n"                                          \
    "-v, --version      output version information then exit\n"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct node_fs // node fileserver info
{
    char ipaddr[16];
    int port;
    struct node_fs *next_fs;
} node_fs;
typedef struct // list node fileserver
{
    node_fs *head;
    node_fs *tail;
} list_fs;

static int check_directory(char *dirname);
static void init_server_socket(int *sockfd, sockaddr_in *addr, socklen_t *addrlen);
static int connect_to_fileserver(int *sockfd, sockaddr_in *fs_addr);
static void *thread_serv_request(void *fd);
static node_fs *create_node_fileserver(char *ip, int port);
static node_fs *get_fileserver_info(char *filename);
static void *thread_check_connecting_fileserver();
static void init_list_fileserver();
static void add_node_fileserver(node_fs *fs);
static void delete_node_fileserver(node_fs *fs);
static void remove_fileserver(node_fs *fs);
static void create_new_fileserver(node_fs *fs);
static int receive_line_data(char *buffer, int sock);

static struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                       {"dir", required_argument, 0, 'd'},
                                       {"help", no_argument, 0, 'h'},
                                       {"version", no_argument, 0, 'v'},
                                       {0, 0, 0, 0}};
static int listen_port = 55555;
static char *master_dir = NULL;
static list_fs lfs; // linked list fileserver
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static int count_fs = 0;

int main(int argc, char *argv[])
{
    int c;
    int option_index;
    int ret;
    // loop to scan all argument
    while (1)
    {
        c = getopt_long(argc, argv, "p:d:hv", long_options, &option_index);

        if (c == -1) // end of options
            break;
        switch (c)
        {
        case 'p':
            listen_port = (int)strtol(optarg, NULL, 10);
            break;
        case 'd':
            master_dir = optarg;
            ret = check_directory(master_dir);
            if (ret < 0)
            {
                if (ret == -1) // error when try to open dir
                    printf("error when try to open dir %s\n", master_dir);
                else if (ret == -2) // dir not empty
                    printf("%s is not empty\n", master_dir);
                else if (ret == -3) // no write permission
                    printf("%s: no write permission\n", master_dir);
                exit(EXIT_FAILURE);
            }

            if (master_dir[strlen(master_dir) - 1] == '/') // strip off '/' character if exist
                master_dir[strlen(master_dir) - 1] = '\0';
            break;
        case 'h':
            printf(HELP);
            exit(EXIT_SUCCESS);
        case 'v':
            printf(VERSION);
            exit(EXIT_SUCCESS);
        case '?':
            printf("Try 'master --help' for more information.\n");
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }

    if (master_dir == NULL) // use default dir $HOME/.master
    {
        master_dir = strcat(getpwuid(getuid())->pw_dir, "/.master");
        ret = check_directory(master_dir);
        if (ret == -3) // not writable
        {
            printf("%s: no write permission\n", master_dir);
            exit(EXIT_FAILURE);
        }
        else if (ret == -2) // not empty
        {
            printf("%s is not empty dir\n", master_dir);
            exit(EXIT_FAILURE);
        }
        else if (ret == -1)
        {
            switch (errno)
            {
            case (EACCES): // access denied
                printf("%s: access denied\n", master_dir);
                exit(EXIT_FAILURE);
            case (ENOTDIR): // is not dir, file or maybe fucking else
                printf("%s is not dir\n", master_dir);
                exit(EXIT_FAILURE);
            case (ENOENT): //  dir not exist, create one
                mkdir(master_dir, 0777);
                break;
            }
        }
    }

    int sockfd;
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    init_server_socket(&sockfd, &addr, &addrlen);
    init_list_fileserver();

    pthread_t check_fs; // this thread loops to check fs still connecting or not
    if (pthread_create(&check_fs, NULL, &thread_check_connecting_fileserver, NULL) != 0)
    {
        perror("create new thread failed"); // create new thread fail
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ret = accept(sockfd, (sockaddr *)&addr, &addrlen);

        if (ret > 0) // new request comming
        {
            int *clientsock = (int *)malloc(sizeof(int));
            *clientsock = ret;

            pthread_t new_thread; // create new thread serve client
            if (pthread_create(&new_thread, NULL, &thread_serv_request, clientsock) != 0)
            {
                perror("create new thread failed"); // create new thread fail
                exit(EXIT_FAILURE);
            }
        }
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
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

void init_server_socket(int *sockfd, sockaddr_in *addr, socklen_t *addrlen)
{
    // Creating socket file descriptor
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    // Filling server information
    addr->sin_family = AF_INET; // IPv4
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(listen_port);

    // Bind the socket with the server address
    if (bind(*sockfd, (sockaddr *)addr, *addrlen) < 0)
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

int connect_to_fileserver(int *sockfd, sockaddr_in *fs_addr)
{
    struct timeval timeout;
    timeout.tv_sec = 5; // timeout 5s
    timeout.tv_usec = 0;

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0)
        return -1;
    // set timeout for this socket
    setsockopt(*sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (connect(*sockfd, (sockaddr *)fs_addr, sizeof(*fs_addr)) != 0)
        return -2;
    return 0;
}

void *thread_serv_request(void *fd)
{
    pthread_detach(pthread_self()); // detach this thread from main thread

    int sock = *(int *)fd;
    char buffer[BUFFSIZE];
    int target;
    char *fnf = "FILE NOT FOUND\r\n";
    char *ukn = "UNKNOW COMMAND\r\n";
    char *fail = "FAIL\r\n";
    char *ok = "OK\r\n";

    /*
     *   To do: get request command, then classify which target request come from, target
     *          variable will be assigned one of these value
     *       - 1: This request come from client
     *       - 2: This request come from fileserver
     *       - 0: Unknow target
     */
    if (receive_line_data(buffer, sock) < 0)
        goto END_THREAD;
    if (strcmp(buffer, "INFO\r\n") == 0) // client request file info
        target = 1;
    else if (strcmp(buffer, "SYNC\r\n") == 0) // fileserver sync
        target = 2;
    else // unknow request command
    {
        send(sock, ukn, strlen(ukn), 0);
        target = 0;
    }

    if (target == 1) // serve client
    {
        /*
         *   To do: Loop to get all filename
         *       - Find out which fileserver holding this filename
         *       - Send fileserver ip and port if success, otherwise return "FILE NOT FOUND\n"
         */

        node_fs *fs;
        printf("RECEIVED INFO REQUEST FROM CLIENT [");
        while (1)
        {
            if (receive_line_data(buffer, sock) < 0)
                break;

            buffer[strlen(buffer) - 1] = '\0';
            printf(" %s", buffer);            // printf filename to screen;
            fs = get_fileserver_info(buffer); // search which fileserver is holding this file
            if (fs == NULL)                   // file not found
                send(sock, fnf, strlen(fnf), 0);
            else
            {
                char fs_info[30];
                sprintf(fs_info, "%s %d\n", fs->ipaddr, fs->port);
                send(sock, fs_info, strlen(fs_info), 0);
            }
        }
        printf(" ]\n");
    }
    else if (target == 2)
    {
        /*
         *   To do:
         *       - Retrive fileserver ip and port
         *       - Get all filename
         *       - Check this fileserver exist or not
         *           + Yes then update list file (still not support this feature)
         *           + No then create new directory
         */

        char fs_ip[16];
        int fs_port;
        char filename[MAX_FILENAME_LEN];
        FILE *nf;
        // get ip address of fileserver
        if (receive_line_data(buffer, sock) < 0)
            goto END_THREAD;
        buffer[strlen(buffer) - 2] = '\0';
        strcpy(fs_ip, buffer);

        // get listening port of fileserver
        if (receive_line_data(buffer, sock) < 0)
            goto END_THREAD;
        buffer[strlen(buffer) - 2] = '\0';
        fs_port = (int)strtol(buffer, NULL, 10);
        printf("RECEIVED SYNC REQUEST FROM [%s %d] ", fs_ip, fs_port);

        // get number of filename
        if (receive_line_data(buffer, sock) < 0)
            goto END_THREAD;
        buffer[strlen(buffer) - 2] = '\0';
        int numFile = (int)strtol(buffer, NULL, 10);

        // check dir exist or not
        sprintf(buffer, "%s/%s_%d", master_dir, fs_ip, fs_port);
        if (check_directory(buffer) != -1) // directory already exist
        {
            send(sock, fail, strlen(fail), 0);
            printf("(FAIL)\r\n");
            goto END_THREAD;
        }
        // dir still not exist, create new
        node_fs *new_fs = create_node_fileserver(fs_ip, fs_port);
        create_new_fileserver(new_fs);

        for (int i = 0; i < numFile; i++)
        {
            receive_line_data(filename, sock);
            filename[strlen(filename) - 1] = '\0';
            sprintf(buffer, "%s/%s_%d/%s", master_dir, fs_ip, fs_port, filename); // create new file
            nf = fopen(buffer, "w");
            fclose(nf);
        }
        send(sock, ok, strlen(ok), 0);
        printf("(SUCCESS)\n");
        printf("connecting fileserver: %d\n", count_fs);
    }

END_THREAD:
    close(sock);
    free(fd);
    pthread_exit(NULL);
}

node_fs *create_node_fileserver(char *fs_ip, int fs_port)
{
    node_fs *newfs = (node_fs *)malloc(sizeof(node_fs));
    strcpy(newfs->ipaddr, fs_ip);
    newfs->port = fs_port;
    newfs->next_fs = NULL;
    return newfs;
}

node_fs *get_fileserver_info(char *filename)
{
    node_fs *nfs = lfs.head;
    while (nfs != NULL)
    {
        char filepath[MAX_PATH_LEN];
        sprintf(filepath, "%s/%s_%d/%s", master_dir, nfs->ipaddr, nfs->port, filename);
        if (access(filepath, F_OK) == 0)
            return nfs;
        nfs = nfs->next_fs;
    }
    return NULL;
}

void *thread_check_connecting_fileserver()
{
    pthread_detach(pthread_self()); // detach this thread from main thread

    int sock;
    sockaddr_in fs_addr;
    node_fs *node;
    node_fs *temp;
    char *ping = "PING\r\n";
    char *ok = "OK\r\n";
    int ret;
    char buffer[BUFFSIZE];
    fs_addr.sin_family = AF_INET;

    while (1)
    {
        node = lfs.head;
        while (node != NULL)
        {
            fs_addr.sin_addr.s_addr = inet_addr(node->ipaddr);
            fs_addr.sin_port = htons(node->port);
            memset(buffer, '\0', BUFFSIZE);

            temp = node;
            node = node->next_fs;
            if (connect_to_fileserver(&sock, &fs_addr) == 0) // connect success
            {
                send(sock, ping, strlen(ping), 0);
                ret = recv(sock, buffer, BUFFSIZE, 0);
                if (ret <= 0 || strcmp(buffer, ok) != 0) // timeout or invalid ping respond
                    remove_fileserver(temp);
            }
            else // connect fail, delete fileserver from list and master dir
            {
                printf("REMOVE FILESERVER [%s %d]\n", temp->ipaddr, temp->port);
                remove_fileserver(temp);
                printf("connecting fileserver: %d\n", count_fs);
            }
            close(sock);
        }
        sleep(2); // sleep 2 second after ping again
    }

    pthread_exit(NULL);
}

void init_list_fileserver()
{
    lfs.head = NULL;
    lfs.tail = NULL;
}

void add_node_fileserver(node_fs *newfs)
{
    pthread_mutex_lock(&gLock);

    if (lfs.head == NULL) // list fs empty
    {
        lfs.head = newfs;
        lfs.tail = newfs;
    }
    else
    {
        lfs.tail->next_fs = newfs; // append new fileserver to lfs
        lfs.tail = lfs.tail->next_fs;
    }
    count_fs++;
    pthread_mutex_unlock(&gLock);
}

void delete_node_fileserver(node_fs *delfs)
{
    pthread_mutex_lock(&gLock);

    node_fs *temp = lfs.head;
    if (lfs.head == delfs) // delete position at head
        lfs.head = lfs.head->next_fs;
    else
    {
        while (temp->next_fs != delfs)
            temp = temp->next_fs;
        if (lfs.tail == delfs) // delete position at tail
            lfs.tail = temp;
        temp->next_fs = delfs->next_fs;
    }
    free(delfs);
    count_fs--;
    pthread_mutex_unlock(&gLock);
}

void remove_fileserver(node_fs *fs)
{
    // delete fileserver dir and node from list
    char command[MAX_PATH_LEN];
    sprintf(command, "rm -rf %s/%s_%d", master_dir, fs->ipaddr, fs->port);
    system(command);
    delete_node_fileserver(fs);
}

void create_new_fileserver(node_fs *fs)
{
    // create new dir fileserver, add node to list
    char path_fs[MAX_PATH_LEN];
    sprintf(path_fs, "%s/%s_%d", master_dir, fs->ipaddr, fs->port);
    mkdir(path_fs, 0777);
    add_node_fileserver(fs);
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
