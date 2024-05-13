#include "threadpool.h"                                                                        

/*
作者：周超
日期：2019.3.30
描述：线程池中的工作线程调用函数
参数：线程池结构体变量
返回值：无
*/
void *thread_routine(void *arg)
{
	//设置线程为可分离状态(后台模式)
	pthread_detach(pthread_self());
    struct timespec abstime;//
    int timeout,status;
    printf("thread %d is starting\n", (int)pthread_self());
    threadpool_t *pool = (threadpool_t *)arg;

    //死循环使线程池中空闲的线程可以复用
    while(1)
    {   
        timeout = 0;

        //访问线程池时需要加锁，防止多个线程同时访问线程池状态
        condition_lock(&pool->ready);

        //空闲线程数+1
        pool->idle++;
		
		//任务队列中无任务并且线程池中没有接收到线程销毁的通知，线程阻塞等待(空闲线程)
        while(pool->first == NULL && !pool->quit)
        {   
            printf("thread %d is waiting\n", (int)pthread_self());

            //获取系统当前的时间
            clock_gettime(CLOCK_REALTIME, &abstime);  
            abstime.tv_sec += 2;//等待时长为2S
            //空闲线程阻塞等待2S条件变量触发，等待超时退出
            status = condition_timedwait(&pool->ready, &abstime);
            if(status == ETIMEDOUT)
            {   
                printf("thread %d wait timed out\n", (int)pthread_self());
                timeout = 1;
                break;
            }   
        }   

		//空闲线程数-1
        pool->idle--;
		//任务队列中有任务
        if(pool->first != NULL)
        {   
            //取任务队列中的第一个任务，移除任务，并执行任务
            task_t *t = pool->first;
            pool->first = t->next;
            //由于任务执行需要消耗时间，先解锁让其他线程有权访问线程池
            condition_unlock(&pool->ready);
            //执行任务
            t->run(t->arg);
            //释放内存
            free(t);
            //重新加锁
            condition_lock(&pool->ready);
        }   

        //如果接受到线程池销毁的通知
        if(pool->quit && pool->first == NULL)
        {
            pool->counter--;//当前线程池中的线程数-1

            //若线程池中没有线程，唤醒主线程(主线程在线程池销毁函数中进入等待)
            if(pool->counter == 0)
            {
                condition_signal(&pool->ready);
            }
			//解锁退出
            condition_unlock(&pool->ready);
            break;
        }

        //线程等待超时退出
        if(timeout == 1)
        {
            pool->counter--;//当前线程池中的线程数+1 
            condition_unlock(&pool->ready);//解锁退出
            break;
        }
		
		//解锁
        condition_unlock(&pool->ready);
    }

    printf("thread %d is exiting\n", (int)pthread_self());
    return NULL;

}


/*
作者：周超
日期：2019.3.30
描述：线程池中初始化
参数：线程池结构体变量，最大线程数
返回值：无
*/
void threadpool_init(threadpool_t *pool, int max_threads)
{
    condition_init(&pool->ready);
    pool->first = NULL;
    pool->last =NULL;
    pool->counter =0;
    pool->idle =0;
    pool->max_threads = max_threads;
    pool->quit =0;
}

/*
作者：周超
日期：2019.3.30
描述：增加一个任务到任务队列中并将它分配给线程或等待分配给线程
参数：线程池结构体变量，指向void *类型的函数指针(任务)，函数参数(任务参数)
返回值：整型
*/
int threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg)
{
    pthread_t tid;
    
	//产生一个新的任务
    task_t *newtask = (task_t *)malloc(sizeof(task_t));
    newtask->run = run;
    newtask->arg = arg;
    newtask->next=NULL;//新任务放在队列尾端

    //线程池的状态可以被多个线程共享，操作前需要加锁
    condition_lock(&pool->ready);

    if(pool->first == NULL)//第一个任务加入
    {
        pool->first = newtask;
    }        
    else    
    {
        pool->last->next = newtask;
    }
    pool->last = newtask;  //队列尾指针指向新任务

    //线程池中有空闲线程，唤醒处于等待状态的线程
    if(pool->idle > 0)
    {
        condition_signal(&pool->ready);
    }
    //线程池中无空闲线程并且当前线程池中的线程数小于最大线程数，创建一个新线程取任务
    else if(pool->counter < pool->max_threads)
    {
        pthread_create(&tid, NULL, thread_routine, pool);
        pool->counter++;
    }
    else
    {
        condition_unlock(&pool->ready);
        return -1;
    }
    //解锁
    condition_unlock(&pool->ready);
    return 0;
}


/*
作者：周超
日期：2019.3.30
描述：销毁线程池
参数：线程池结构体变量
返回值：无
*/
void threadpool_destroy(threadpool_t *pool)
{
    //如果已经调用销毁直接返回
    if(pool->quit)
    {
        return;
    }
    //加锁
    condition_lock(&pool->ready);                                                                                                                                                                                                             
    //销毁标记置1
    pool->quit = 1;
    //若当前线程池中的线程数大于0
  if(pool->counter > 0)
    {
        //唤醒处于等待状态中的线程并通知其自杀
        if(pool->idle > 0)
        {
            condition_broadcast(&pool->ready);
        }
        //等待所有工作线程将任务队列中的任务全部完成
        while(pool->counter)
        {
            condition_wait(&pool->ready);
        }
    }
    condition_unlock(&pool->ready);//解锁
    condition_destroy(&pool->ready);//销毁互斥锁和条件变量
}                    
