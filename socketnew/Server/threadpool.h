#ifndef _THREAD_POOL_H_                                                                        
#define _THREAD_POOL_H_

#include "condition.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

//封装线程池中的线程需要执行的任务对象
typedef struct task
{
    void *(*run)(void *args);  
    void *arg;              
    struct task *next;      
}task_t;


//线程池结构体
typedef struct threadpool
{
    condition_t ready;    //状态量
    task_t *first;       //任务队列头指针
    task_t *last;        //任务队列尾指针
    int counter;         //线程池中线程的数量
    int idle;            //线程池中空闲线程的数量
    int max_threads;     //最大线程数
    int quit;            //线程池销毁标志位 1/0
}threadpool_t;

extern threadpool_t pool;

//线程池初始化
void threadpool_init(threadpool_t *pool, int max_threads);

//增加一个任务到任务队列中并将它分配给线程或等待分配给线程
int threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg);

//销毁线程池
void threadpool_destroy(threadpool_t *pool);

#endif
