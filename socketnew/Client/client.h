#ifndef _CLIENT_H
#define _CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <fcntl.h>


#define PORT 5555
#define SERVER_IP "10.5.150.102"
#define MAX_LINE_LEN 1000

#define SKIP_TIME 10

#define MAX_LINE_LENGTH 100

// 全局变量，用于存储配置信息
char server_ip[MAX_LINE_LENGTH];
int port;

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
} SystemInfo;

void read_config_file(const char *filename);//读配置文件

double get_cpu_usage();// 获取 CPU 利用率

void serialize_system_info(SystemInfo *info, char *buffer, int *length);// 将结构体数据转换为字节流

void get_memory_usage(unsigned long long *total_mem, unsigned long long *free_mem);// 获取内存使用情况

void get_disk_usage(SystemInfo *system_info);//获取磁盘使用情况

void write_log(const char *log_file, const char *message);// 写日志

int encrypt(char *plaintext, int plaintext_len, char *ciphertext);//加密
#endif