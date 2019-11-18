# Forward proxy socket c
---
## 1. Feature
| Achive                             | Point  |
| ---------------------------------- | ------ |
| Process GET/HEAD method            | 20     |
| Filtering URLs                     | 15     |
| Process multi request at same time | 5      |
| Reap zombie process                | 5      |
| No Memory leak                     | 5      |
| Signal handling                    | 15     |
| High available                     | 5      |
| Filter valid request               | 5      |
|                                    | **75** |
---
## 2. Struct and function meaning 
```c
typedef struct
{
    char protocol[10];
    char host[MAX_HOSTLEN];
    int port;
    char path[PATH_MAX];
} url_info;
```
- Struct url_info use to store info parsed from requested url
```c
typedef struct
{
    char startline[300];
    char http_header[MAX_HEADER_SIZE];
} http_message_header;
```
- Struct http_message_header use to store reponse headers from host
```c
static void sig_INT_handler(int signum);
```
- Process SIGINT, prevent terminate proxy
```c
static void sig_USR1_handler(int signum);
```
- Print proxy statistic
```c
static void sig_USR2_handler(int signum);
```
- Send SIGUSR2 to shutdown proxy
```c
static void parse_url(url_info *info, const char *full_url);
```
- Parse url from http request
```c
static int recv_headers(http_message_header *head, int sfd);
```
- Receive http request headers
```c
static void get_URL_info(sockaddr_in *addr, url_info *info);
```
- Parse url info from struct info to addr
```c
static int check_valid_request(url_info *info, http_message_header *head);
```
- Check request headers valid or not
```c
static void get_http_method(char *method, http_message_header *head);
```
- Get http method from request headers
---
## 3. References
- https://stackoverflow.com/questions/28457525/how-do-you-kill-zombie-process-using-wait
- http://man7.org/linux/man-pages/man7/signal.7.html
- http://man7.org/linux/man-pages/man2/fork.2.html
- http://man7.org/linux/man-pages/man2/poll.2.html
- http://man7.org/linux/man-pages/man2/socket.2.html