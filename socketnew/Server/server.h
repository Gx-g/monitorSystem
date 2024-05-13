#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <mysql/mysql.h>

#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include "threadpool.h"
#define PORT 5555


MYSQL *conn_ptr;
MYSQL_RES *res_ptr;
MYSQL_ROW sqlrow;
unsigned int timeout = 7; //超时时间7秒

//新增
threadpool_t pool;
struct sockaddr_in server_addr;//存放服务器绑定需要的协议族、端口号、IP地址
struct sockaddr_in client_addr;//存放连接到服务器的客户端的协议族、端口号、IP地址

#define EPOLL_SIZE 1024 
int epfd;//epoll句柄，红黑树根结点
struct epoll_event events[EPOLL_SIZE];//存放某一时刻内核事件表中就绪的事件
int server_socket;//代表服务器与客户端连接通道的套接字
int clientcount;


// 定义磁盘信息结构体
typedef struct {
    char disk_name[20];     // 磁盘名称
    uint64_t total_size;    // 总大小（字节）
    uint64_t used_size;     // 已使用大小（字节）
    uint64_t free_size;     // 未使用大小（字节）
} DiskInfo;

// 定义系统资源信息结构体
typedef struct {
    double cpu_usage;        // CPU 使用率
    uint64_t total_mem;     // 内存总大小（字节）
    uint64_t used_mem;      // 已使用内存大小（字节）
    uint64_t free_mem;      // 未使用内存大小（字节）
    int num_disks;          // 磁盘数量
    DiskInfo *disks;        // 指向磁盘信息数组的指针

    time_t timestamp;       // 时间戳
    //struct in_addr client_ip; // 客户端IP地址
} SystemInfo;

//socket传参
typedef struct  {
            int *fp;
            struct in_addr sin_addr;
}Args;
Args args;


void init_mysql();//数据库
void Serveraccept();//接受客户端建立连接的请求
void addfd(int epollfd, int fd);//添加新的事件到内核事件列表中
int setnonblocking(int sockfd);//设置IO为非阻塞模式
void *handle_client(void *arg); //接收客户端发送的信息
void deserialize_system_info(char *buffer, int length, SystemInfo *info);//// 将字节流解析为结构体数据
#endif