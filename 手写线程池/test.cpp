#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template<typename T> 
class threadpool{
public:
        threadpool(connection_pool * conpool,int thread_num,int max_queue);
        ~threadpool();
        bool append(T*);//添加队列
private:
        void run();//工作函数
        static void * worker(void *arg);//线程的回调函数,传入的参数是this指针，使用前需要强转回来

private:
    sem m_queuestat;//信号量
    pthread_t* m_thread;//创建的线程数组的指针
    locker m_queuelocker;//锁
    std::list<T*> m_queue;//工作队列
    int m_max_queue;//工作队列最大长度
    int m_thread_num;//线程池中线程数量
    connection_pool * m_connpool;//数据库
};
template<typename T>
bool threadpool<T>::append(T* request){
    m_queuelocker.lock();
    if(m_queue.size()>=m_max_queue) {
        m_queuelocker.unlock();
        return false;
    }    
    m_queue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();//唤醒工作线程
    return true;
}

template<typename T>
threadpool<T>::threadpool(connection_pool * conpool,int thread_num,int max_queue):m_thread(nullptr),m_thread_num(thread_num),m_max_queue(max_queue),m_connpool(conpool){
    if(thread_num<=0||max_queue<=0) throw std::exception();
    m_thread = new pthread_t[thread_num];
    if(!m_thread) throw std::exception();//创建线程池数组失败
    for(int i=0;i<thread_num;i++){
        if(pthread_create(m_thread+i,NULL,worker,this)!=0){
            delete [] m_thread;
            throw std::exception();
        }
        if(pthread_detach(m_thread[i])){
            delete [] m_thread;
            throw std::exception();
        }
    }
}


template<typename T>
void* threadpool<T>::worker(void* arg){

    //将参数强转为线程池类，调用成员方法
   threadpool* pool=(threadpool*)arg;
    pool->run();
    return pool;
}


template<typename T>
void threadpool<T>::run(void ){
    while(1){
        m_queuestat.wait();//信号量等待
        m_queuelocker.lock();
        if(m_queue.empty()){
            m_queuelocker.unlock();//解锁退出
            continue;
        }
        T * request = m_queue.front();
        m_queue.pop_front();
         m_queuelocker.unlock();//拿出来后可以先解锁
        if(!request)  continue;
        request->mysql = m_connpool->GetConnection();//取出数据库连接池中的一条数据库连接放入该客户端连接的变量中
        request->process();//处理
        m_connpool->ReleaseConnection(request->mysql);//释放该数据库连接
    }
}