#include "client.h"

//读配置文件
void read_config_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    char key[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];

    // 逐行读取配置文件
    while (fgets(line, sizeof(line), file) != NULL) {
        // 解析每一行，以等号分隔键值对
        if (sscanf(line, "%[^=]=%s", key, value) == 2) {
            // 去除键和值两端的空格
            char *trimmed_key = strtok(key, " \t\n");
            char *trimmed_value = strtok(value, " \t\n");

            // 根据键名赋值给对应的全局变量
            if (strcmp(trimmed_key, "hostname") == 0) {
                strcpy(server_ip, trimmed_value);
            } else if (strcmp(trimmed_key, "port") == 0) {
                port = atoi(trimmed_value);
            } 
        }
    }

    fclose(file);
}

// 获取 CPU 利用率
double get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/stat");
        exit(1);
    }

    long user, nice, system, idle, iowait, irq, softirq;
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(fp);

    long idle_time = idle + iowait;
    long total_time = user + nice + system + idle + iowait + irq + softirq;

    static long prev_total_time = 0, prev_idle_time = 0;
    double cpu_usage = ((double)(total_time - prev_total_time - (idle_time - prev_idle_time)) / (total_time - prev_total_time)) * 100;

    prev_total_time = total_time;
    prev_idle_time = idle_time;

    return cpu_usage;
}

// 将结构体数据转换为字节流
void serialize_system_info(SystemInfo *info, char *buffer, int *length) {
    int offset = 0;

    memcpy(buffer + offset, &(info->cpu_usage), sizeof(double));
    offset += sizeof(double);

    memcpy(buffer + offset, &(info->total_mem), sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(buffer + offset, &(info->used_mem), sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(buffer + offset, &(info->free_mem), sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(buffer + offset, &(info->num_disks), sizeof(int));
    offset += sizeof(int);

    for (int i = 0; i < info->num_disks; i++) {
        memcpy(buffer + offset, &(info->disks[i]), sizeof(DiskInfo));
        offset += sizeof(DiskInfo);
    }

    memcpy(buffer + offset, &(info->timestamp), sizeof(time_t));
    offset += sizeof(time_t);

    *length = offset;
}

// 获取内存使用情况
void get_memory_usage(unsigned long long *total_mem, unsigned long long *free_mem) {
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == -1) {
        perror("Failed to get system memory info");
        exit(1);
    }

    *total_mem = memInfo.totalram;
    *total_mem *= memInfo.mem_unit;

    *free_mem = memInfo.freeram;
    *free_mem *= memInfo.mem_unit;
}

//获取磁盘使用情况
void get_disk_usage(SystemInfo *system_info){
    FILE *file;
    char line[MAX_LINE_LEN];
    const char *file_path = "/proc/mounts";
    regex_t regex;
    int ret;
    DiskInfo *disk_infos = NULL; // 存储磁盘信息的数组
    int num_matched = 0; // 匹配结果数量
    int array_size = 0; // 数组大小
    struct statvfs buf;

    // 编译正则表达式
    ret = regcomp(&regex, "^/dev/(sd|nvme)[[:alnum:]]+", REG_EXTENDED);
    if (ret != 0) {
        fprintf(stderr, "Failed to compile regular expression\n");
        return EXIT_FAILURE;
    }

    // 打开文件
    file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    // 逐行读取文件并匹配
    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        // 执行正则表达式匹配
        ret = regexec(&regex, line, 0, NULL, 0);
        if (ret == 0) {
            // 匹配成功，增加匹配结果数量
            num_matched++;

            // 动态增加存储磁盘信息的数组大小
            if (num_matched > array_size) {
                array_size += 10; // 增加 10 个元素的空间
                disk_infos = realloc(disk_infos, array_size * sizeof(DiskInfo));
                if (disk_infos == NULL) {
                    fprintf(stderr, "Failed to allocate memory\n");
                    return EXIT_FAILURE;
                }
            }

            char device[MAX_LINE_LEN];
            char mount_point[MAX_LINE_LEN];
            if (sscanf(line, "%s %s", device, mount_point) == 2) {
               
                    // 获取每个挂载点的文件系统信息
                    if (statvfs(mount_point, &buf) != -1) {
                        // 计算使用情况
                        
                        unsigned long total_blocks = buf.f_blocks * (buf.f_frsize / 1024); // 总块数（KB）
                        unsigned long free_blocks = buf.f_bfree * (buf.f_frsize / 1024);   // 可用块数（KB）
                        unsigned long used_blocks = total_blocks - free_blocks;             // 已用块数（KB）

                        // 打印使用情况
                        printf("Device: %s\n", device);
                        printf("Mount Point: %s\n", mount_point);
                        printf("Total Space: %lu KB\n", total_blocks);
                        printf("Used Space: %lu KB\n", used_blocks);
                        printf("Free Space: %lu KB\n", free_blocks);
                        printf("\n");
                        // 提取设备名
                        sscanf(line, "/dev/%s", disk_infos[num_matched - 1].disk_name);
                        // 获取每个挂载点的文件系统信息
                        disk_infos[num_matched - 1].total_size = total_blocks/1024;
                        disk_infos[num_matched - 1].used_size = used_blocks/1024;
                        disk_infos[num_matched - 1].free_size = free_blocks/1024;
                    } else {
                        fprintf(stderr, "Failed to get filesystem stats for %s\n", mount_point);
                    }
                
            }
            
        } else if (ret != REG_NOMATCH) {
            // 匹配失败
            fprintf(stderr, "Failed to execute regular expression\n");
        }
    }

    // 关闭文件
    fclose(file);

    // 释放正则表达式资源
    regfree(&regex);

    // 打印匹配结果
    printf("Number of lines matched: %d\n", num_matched);
    for (int i = 0; i < num_matched; i++) {
        printf("Disk Name: %s\n", disk_infos[i].disk_name);
        printf("Total Size: %llu\n", disk_infos[i].total_size);
        printf("Used Size: %llu\n", disk_infos[i].used_size);
        printf("Free Size: %llu\n\n", disk_infos[i].free_size);
    }
    system_info->num_disks = num_matched;
    system_info->disks = disk_infos;
    // 释放存储磁盘信息的数组
    //free(disk_infos);





}
// 写日志
void write_log(const char *log_file, const char *message) {
    FILE *fp = fopen(log_file, "a");
    if (fp == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    time_t now;
    struct tm *local_time;
    char time_str[20];

    time(&now);
    local_time = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

    fprintf(fp, "[%s] %s\n", time_str, message);
    fclose(fp);
}

int encrypt(char *plaintext, int plaintext_len, char **ciphertext) {
    
    int ciphertext_len = plaintext_len;
    *ciphertext = plaintext;

    //加操作...

    return ciphertext_len;
}

int main() {
    // 读取配置文件
    read_config_file("monitor.properties");

    // 打印读取到的配置信息
    printf("Hostname: %s\n", server_ip);
    printf("Port: %d\n", port);

    int client_socket;
    struct sockaddr_in server_address;
    char message[1024];
    SystemInfo system_info;
    const char *log_file = "monitor.log";
    
    // 创建socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 初始化服务器地址结构
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    
    // 转换IP地址
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) == -1) {
        perror("Invalid address / Address not supported");
        exit(EXIT_FAILURE);
    }
    
    // 连接服务器
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("连接到服务器\n");

      // 保持连接
    while (1) {

        sleep(SKIP_TIME);
        // 在这里填充 system_info 结构体数据
        // 获取并输出 CPU 利用率
        double cpu_usage = get_cpu_usage();
        printf("CPU Usage: %.2f%%\n", cpu_usage);
        system_info.cpu_usage = cpu_usage;
        // 获取并输出内存使用情况
        unsigned long long total_mem, free_mem;
        get_memory_usage(&total_mem, &free_mem);
        system_info.cpu_usage = cpu_usage;
        printf("Total Memory: %llu bytes, Free Memory: %llu bytes\n", total_mem, free_mem);
        system_info.total_mem = total_mem/1024/1024;
        system_info.used_mem = (total_mem - free_mem)/1024/1024;
        system_info.free_mem = free_mem/1024/1024;
        get_disk_usage(&system_info);

        time_t now;
        struct tm *local_time;
        char time_str[20];

        time(&now);
        local_time = localtime(&now);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
        system_info.timestamp = now;
        printf("发送时间：%s\n",time_str);

        //写日志
        char cpu_message[50];
        sprintf(cpu_message, "CPU Usage: %.2f%%", cpu_usage);
        write_log(log_file, cpu_message);

        char mem_message[100];
        sprintf(mem_message, "Total Memory: %llu MB, Used Memory: %llu MB, Free Memory: %llu MB", system_info.total_mem,system_info.used_mem, system_info.free_mem);
        write_log(log_file, mem_message);

        printf("Number of lines matched: %d\n", system_info.num_disks);
        DiskInfo *disk_infos = system_info.disks;
        for (int i = 0; i < system_info.num_disks; i++) {
            char disk_message[100];
            sprintf(disk_message, "Device: %s,Total: %llu MB, Used: %llu MB, Free: %llu MB", disk_infos[i].disk_name,disk_infos[i].total_size,disk_infos[i].used_size, disk_infos[i].free_size);
            write_log(log_file, disk_message);
            // printf("Disk Name: %s\n", disk_infos[i].disk_name);
            // printf("Total Size: %llu\n", disk_infos[i].total_size);
            // printf("Used Size: %llu\n", disk_infos[i].used_size);
            // printf("Free Size: %llu\n\n", disk_infos[i].free_size);
        }



        // 将结构体数据转换为字节流
        char buffer[1024];
        int buffer_length;
        serialize_system_info(&system_info, buffer, &buffer_length);

        //数据加密
         char *ciphertext;
         int ciphertext_len =  encrypt(buffer, buffer_length, &ciphertext);


        // 发送数据到服务器
        if (send(client_socket, ciphertext, ciphertext_len, 0) == -1) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
        printf("System info sent\n");

        //接受消息
        char buffer1[1024] = {0};
        int valread;
        if ((valread = recv(client_socket, buffer1, 1024, 0)) == -1) {
            perror("Recv failed");
            exit(EXIT_FAILURE);
        }


    }
        close(client_socket);
        return 0;
}
