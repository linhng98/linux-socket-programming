# Http download file client
> Nguyễn Văn Linh
> 1612340 
---
## Mục lục
- [Tiến độ thực hiện](#ti%e1%ba%bfn-%c4%91%e1%bb%99-th%e1%bb%b1c-hi%e1%bb%87n)
- [Ý nghĩa các hàm](#%c3%9d-ngh%c4%a9a-c%c3%a1c-h%c3%a0m)
- [Kết quả](#c%c3%a1ch-ch%e1%ba%a1y-ch%c6%b0%c6%a1ng-tr%c3%acnh-v%c3%a0-k%e1%ba%bft-qu%e1%ba%a3)
- [Quá trình gửi nhận TCP](#qu%c3%a1-tr%c3%acnh-g%e1%bb%adi-nh%e1%ba%adn-g%c3%b3i-tin-tcp)
- [Tham khảo](#tham-kh%e1%ba%a3o)
---
## Tiến độ thực hiện
- [x] Linux OS
- [x] C/C++
- [x] Makefile
- [x] Tổ chức thư mục
- [x] Download file
- [x] Download directory
- [ ] Recursive download 
---
## Ý nghĩa các hàm
```C 
void error_handler(char *message)
```
- Hàm xuất ra lỗi và thoát chương trình

```C
void parse_url(url_info *info, const char *full_url);
```
- Hàm parse url từ parameter, kết quả parse được lưu vào struct url_info 
```C
void get_URL_info(sockaddr_in *addr, url_info *info);
````
- Hàm get_URL_info nhận vào struct url_info, lấy thông tin hostname, port lưu vào struct sockaddr_in
```C
int get_headers(char* hbuff, int sockfd);
```
- Hàm nhận respond headers từ server, header được lưu vào mảng hbuff
```C
int search_header_value(char* value, char* header, char* hbuff);
````
- Hàm search header value, lấy value từ danh sách các header đã được lưu trong hbuff (ví dụ: Content-Length) 
```C
int get_status_code(char* hbuff);
````
- Lấy status code, check success or fail
```C
int check_is_file(char *path);
````
- Kiểm tra path format là dir hay là file
```C
void ParseHref(char *f1, char *f2);
````
- Hàm parsehref nhận vào file f2(html body), parse link vào file f1
```C
int download_file(sockaddr_in *addr, char *save_path, url_info *info);
````
- Tải file

```C
int download_dir(sockaddr_in *addr, char *save_dir, url_info *info);
````
- Tải html body, parse href, sau đó gọi hàm download_file cho từng link

---
## Cách chạy chương trình và kết quả
- Compile
```Bash
$ make
```
- Download directory
```Bash
$ ./1612340 http://students.iitk.ac.in/programmingclub/course/lectures/
```
- Download file
```Bash
$ ./1612340 http://students.iitk.ac.in/programmingclub/course/lectures/1.%20Introduction%20to%20C%20language%20and%20Linux.pdf
```
- kết quả
```Bash
[linh@ArchLinux 1612340]$ ls
exe  report  src
[linh@ArchLinux 1612340]$ cd exe 
[linh@ArchLinux exe]$ ls
1612340
[linh@ArchLinux exe]$ ./1612340 http://students.iitk.ac.in/programmingclub/course/lectures/                                                       
1.%20Introduction%20to%20C%20language%20and%20Linux.pdf    321.28 KiB    00:01  [####################]100%
1.%20Introduction%20to%20C%20language%20and%20Linux.ppt    406.50 KiB    00:01  [####################]100%
1.%20Introduction%20to%20C%20language%20and%20Linux.pptx    158.32 KiB    00:01  [####################]100%
2.%20Data,%20Operators,%20IO.pdf                      503.56 KiB    00:01  [####################]100%
2.%20Data,%20Operators,%20IO.ppt                      852.00 KiB    00:02  [####################]100%
2.%20Data,%20Operators,%20IO.pptx                     114.34 KiB    00:01  [####################]100%
3.%20ConditionalExpressions,%20Loops.pdf              361.35 KiB    00:01  [####################]100%
3.%20ConditionalExpressions,%20Loops.pptx              90.86 KiB    00:00  [####################]100%
4.%20Functions,%20scope.pdf                            82.54 KiB    00:00  [####################]100%
4.%20Functions,%20scope.ppt                            64.00 KiB    00:00  [####################]100%
5.%20Arrays,%20Pointers%20and%20Strings.pdf           404.67 KiB    00:01  [####################]100%
5.%20Arrays,%20Pointers%20and%20Strings.ppt           491.00 KiB    00:01  [####################]100%
5.%20Arrays,%20Pointers%20and%20Strings.pptx          136.86 KiB    00:01  [####################]100%
6.%20More%20on%20Pointers.pdf                         313.20 KiB    00:01  [####################]100%
6.%20More%20on%20Pointers.ppt                         324.50 KiB    00:01  [####################]100%
6.%20More%20on%20Pointers.pptx                        104.92 KiB    00:01  [####################]100%
7.%20Dynamic%20Allocation.pdf                         247.87 KiB    00:01  [####################]100%
7.%20Dynamic%20Allocation.ppt                         191.00 KiB    00:01  [####################]100%
7.%20Dynamic%20Allocation.pptx                         69.39 KiB    00:00  [####################]100%
8.%20Structures,%20file%20IO,%20recursion.pdf          99.09 KiB    00:01  [####################]100%
8.%20Structures,%20file%20IO,%20recursion.ppt         330.50 KiB    00:01  [####################]100%
8.%20Structures,%20file%20IO,%20recursion.pptx         78.57 KiB    00:01  [####################]100%
9.Preproccessing,%20libc,%20searching,%20sorting.pdf     56.94 KiB    00:01  [####################]100%
9.Preproccessing,%20libc,%20searching,%20sorting.ppt    139.00 KiB    00:01  [####################]100%
Lab%20Session%201.docx                                 13.56 KiB    00:00  [####################]100%
Lab%20Session%201.pdf                                 172.34 KiB    00:00  [####################]100%
Lab%20Session%202.docx                                 13.91 KiB    00:00  [####################]100%
Lab%20Session%202.pdf                                 399.88 KiB    00:01  [####################]100%
Lab%20Session%203.docx                                 13.15 KiB    00:00  [####################]100%
Lab%20Session%203.pdf                                 246.19 KiB    00:01  [####################]100%
Lab%20Session%204.pdf                                 216.40 KiB    00:00  [####################]100%
Supplementary%20Problems%201.doc                       32.50 KiB    00:00  [####################]100%
Supplementary%20Problems%201.docx                      15.94 KiB    00:01  [####################]100%
Supplementary%20Problems%201.pdf                      266.45 KiB    00:01  [####################]100%
[linh@ArchLinux exe]$ ls
1612340  1612340_lectures
[linh@ArchLinux exe]$ 

```
---
## Quá trình gửi nhận gói tin TCP

---
## Tham khảo
- https://www.geeksforgeeks.org/socket-programming-cc/
- http://www.linuxhowtos.org/C_C++/socket.htm
- https://stackoverflow.com/questions/30470505/http-request-using-sockets-in-c
- https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa