#include "server.h"

// 将字节流解析为结构体数据
void deserialize_system_info(char *buffer, int length, SystemInfo *info) {
    int offset = 0;

    memcpy(&(info->cpu_usage), buffer + offset, sizeof(double));
    offset += sizeof(double);

    memcpy(&(info->total_mem), buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&(info->used_mem), buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&(info->free_mem), buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    memcpy(&(info->num_disks), buffer + offset, sizeof(int));
    offset += sizeof(int);

    info->disks = (DiskInfo *)malloc(info->num_disks * sizeof(DiskInfo));
    for (int i = 0; i < info->num_disks; i++) {
        memcpy(&(info->disks[i]), buffer + offset, sizeof(DiskInfo));
        offset += sizeof(DiskInfo);
    }

    memcpy(&(info->timestamp), buffer + offset, sizeof(time_t));
    offset += sizeof(time_t);

    // memcpy(&(info->client_ip), buffer + offset, sizeof(struct in_addr));
    // offset += sizeof(struct in_addr);
}

void *handle_client(void *arg) {

    Args args = *((Args *)arg);
    int client_socket = *(args.fp);
    struct in_addr sin_addr = args.sin_addr;
    SystemInfo system_info;


    //int client_socket = *((int *)arg);
    char buffer[1024] = {0};
    //SystemInfo system_info;
    // while (1) {
        // 接收消息
        int valread;
        if ((valread = recv(client_socket, buffer, 1024, 0)) == -1) {
            perror("Recv failed");
            exit(EXIT_FAILURE);
        }
        
        if (valread == 0) {
            printf("客户端关闭连接\n");
            clientcount--;
            return;
           // break;
        }

        //printf("接收到的消息: %s\n", buffer);

         // 解析数据为结构体
         printf("接受到的字节数：%ld",valread);
        //void * nowbuffer = malloc(valread);
        deserialize_system_info(buffer, valread, &system_info);
        //free(nowbuffer);

        //插入系统信息sql语句
        char system_info_sql[200];
        int used = system_info.used_mem;
        int total = system_info.total_mem;
        double dused= (double)used;
        double dtotal=(double)total;
        printf("int leixing %d %d double leixing %.2f %.2f",used,total,dused,dtotal);
        double mem_pre =  (dused)/(dtotal);
        printf("mem_pre : %.2f \n",mem_pre);
        char time_str1[20];
        strftime(time_str1, sizeof(time_str1), "%Y-%m-%d %H:%M:%S", localtime(&system_info.timestamp));
        sprintf(system_info_sql, "insert into `app01_systeminfo` (`HOST_NAME`,`CPU_PER`,`MEM_PER`,`CREATE_TIME`,`STATE`,`REMARK`) values ('%s',%.2f,%.2f,'%s','1','test');",inet_ntoa(sin_addr), system_info.cpu_usage, mem_pre, time_str1 );
        printf("system_info_sql %s\n",system_info_sql);
         // 插入新的数据实例
        if (mysql_query(conn_ptr, system_info_sql) != 0) {
            fprintf(stderr, "插入MySQL失败！\n");
            //exit(1);
        } else {
            fprintf(stdout, "插入MySQL成功！\n");
            //printf("插入成功");
        }
        // 释放结果集
        //mysql_free_result(result);
        //inet_ntoa(sin_addr)
        //system_info.timestamp 


        //插入内存信息sql语句
        char mem_info_sql[200];
        sprintf(mem_info_sql, "insert into `app01_memstate` (`HOST_NAME`,`TOTAL`,`USED`,`FREE`,`USE_PER`,`DATE_STR`,`CREATE_TIME`) values ('%s', '%llu','%llu','%llu',%.2f,'%ld','%s');",inet_ntoa(sin_addr), system_info.total_mem, system_info.used_mem, system_info.free_mem, mem_pre, system_info.timestamp, time_str1);
        printf("mem_info_sql %s\n",mem_info_sql);
         // 插入新的数据实例
        if (mysql_query(conn_ptr, mem_info_sql) != 0) {
            fprintf(stderr, "插入内存信息失败！\n");
            //exit(1);
        } else {
            fprintf(stdout, "插入内存信息成功！\n");
            //printf("插入成功");
        }
         
        char disk_message[100];
        DiskInfo *disk_infos = system_info.disks;
        for (int i = 0; i < system_info.num_disks; i++) {
            //插入磁盘信息sql语句
            double disk_pre = (double)disk_infos[i].used_size / (double)disk_infos[i].total_size;
            char disk_info_sql[200];
            sprintf(disk_info_sql, "insert into `app01_diskstate` (`HOST_NAME`,`DISK_Name`,`SIZE`,`USED`,`AVAIL`,`USE_PER`,`DATE_STR`,`CREATE_TIME`) values ('%s', '%s', '%llu', '%llu', '%llu', %.2f,'%ld','%s');",inet_ntoa(sin_addr), disk_infos[i].disk_name, disk_infos[i].total_size, disk_infos[i].used_size, disk_infos[i].free_size, disk_pre,system_info.timestamp, time_str1);
            printf("disk_info_sql %s\n",disk_info_sql);
             // 插入新的数据实例
            if (mysql_query(conn_ptr, disk_info_sql) != 0) {
                fprintf(stderr, "插入磁盘信息失败！%d/%d\n",i,system_info.num_disks);
                //exit(1);
            } else {
                fprintf(stdout, "插入磁盘信息成功！%d/%d\n",i,system_info.num_disks);
                //printf("插入成功");
            }
        }


        // 打印接收到的信息
        printf("Client IP: %s\n", inet_ntoa(sin_addr));
        printf("CPU Usage: %.2f\n", system_info.cpu_usage);
        printf("Total Memory(MB): %llu\n", system_info.total_mem);
        printf("Used Memory(MB): %llu\n", system_info.used_mem);
        printf("Free Memory(MB): %llu\n", system_info.free_mem);
        printf("Number of Disks: %d\n", system_info.num_disks);
        for (int i = 0; i < system_info.num_disks; i++) {
            printf("Disk %d - Name: %s, Total: %llu, Used: %llu, Free: %llu\n", i+1, system_info.disks[i].disk_name,
                system_info.disks[i].total_size, system_info.disks[i].used_size, system_info.disks[i].free_size);
        }

        printf("Timestamp: %ld\n", system_info.timestamp);
        struct tm *local_time;
        char time_str[20];
        local_time = localtime(&system_info.timestamp);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
        printf("发送时间：[%s]\n", time_str);
        //printf("Client IP: %s\n", inet_ntoa(system_info.client_ip));

        //释放
        free(system_info.disks);
        
        // 发送消息
        char response[] = "Message received!";
        if (send(client_socket, response, strlen(response), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            pthread_exit(NULL);
        }
    // }
}
/*
描述:设置IO为非阻塞模式
参数:代表服务器与客户端建立连接通道的套接字或者服务器监听套接字
返回值:整型
*/
int setnonblocking(int sockfd){
	int ret = 0;
    ret = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)| O_NONBLOCK);
    return ret;
}

/*
描述:添加新的事件到内核事件列表中
参数:代表服务器与客户端建立连接通道的套接字或者服务器监听套接字
返回值:无
*/
void addfd(int epollfd, int fd){
	int ret = 0;
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
	ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    ret = setnonblocking(fd);
	if(-1 == ret)
	{
		perror("setnonblocking!\n");
	}
    printf("\nfd added to epoll!\n");
}

/*
描述:接受客户端建立连接的请求
参数:代表服务器与客户端建立连接通道的套接字
返回值:无
*/
void Serveraccept(){
	int len = sizeof(client_addr);
	int i = 0;
	int ret = 0;
	int epoll_events_count = 0;
	int clientfd = 0;

	//初始化线程池，最多20个线程
    threadpool_init(&pool, 20); 
	
	while(1)
    {
        //epoll_events_count表示就绪事件的数目
		epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0)
		{
            perror("epoll_wait failure");
            break;
        }
		
		printf("\nepoll_events_count = %d\n", epoll_events_count);

        //处理这epoll_events_count个就绪事件
        for(i = 0; i < epoll_events_count; i++)
        {
            //新用户连接
            if(events[i].data.fd == server_socket)
            {
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                clientfd = accept(server_socket, ( struct sockaddr* )&client_addr, &len);
                printf("client connection from: %s : % d(IP : port), clientfd = %d \n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),clientfd);
                
				//把这个新的客户端添加到内核事件列表
				addfd(epfd, clientfd);
				clientcount++;
                // 服务端用list保存用户连接
                //clients_list.push_back(clientfd);

                printf("Add new clientfd = %d to epoll\n", clientfd);
				printf("Now there are %d client!\n\n",clientcount);

            }
            //客户端唤醒,处理用户发来的消息
            else
            {   
                args.fp = &events[i].data.fd;
                args.sin_addr = client_addr.sin_addr;
				threadpool_add_task(&pool, handle_client, &args);
            }

        }

    }

}

//数据库
void init_mysql(){
    int ret = 0;
    int first_row = 1;
    conn_ptr = mysql_init(NULL); //初始化
    if (!conn_ptr)
    {
        printf("mysql_init failed!\n");
        return -1;
    }
 
    ret = mysql_options(conn_ptr, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout); //设置超时选项
    if (ret)
    {
        printf("Options Set ERRO!\n");
    }
    conn_ptr = mysql_real_connect(conn_ptr, "10.5.150.102", "root", "123456", "monitor", 3306, NULL, 0); //连接MySQL testdb数据库，端口为3306
    if (conn_ptr)
    {
        printf("Connection Succeed!\n");
 
    }
    else //错误处理
    {
        printf("Connection Failed!\n");
        if (mysql_errno(conn_ptr))
        {
            printf("Connect Erro:%d %s\n", mysql_errno(conn_ptr), mysql_error(conn_ptr)); //返回错误代码、错误消息
        }
        return -2;
    }
}

int main() {


    init_mysql();



    //int server_socket;
    struct sockaddr_in server_address, client_address;
    char buffer[1024] = {0};
    //Args args;
    
    // 创建socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置套接字选项，允许重用地址
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 初始化服务器地址结构
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    
    // 绑定地址
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 监听
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    //在内核中创建事件表
    epfd = epoll_create(EPOLL_SIZE);

    if(epfd < 0) 
	{ 
		perror("epfd error"); 
		exit(-1);
	}

    printf("epoll created, epollfd = %d\n", epfd);

    //往内核事件表里添加事件
	addfd(epfd, server_socket);
	printf("Add server = %d to epoll\n",server_socket);
    
    printf("等待客户端连接...\n");
    Serveraccept();
	return 0;
    

}
