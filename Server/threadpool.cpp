
#include "threadpool.h"

ThreadPool::ThreadPool()
    :m_done(false)
{
    //硬件所能支持的最大线程数量
    unsigned const thread_count = std::thread::hardware_concurrency();
    try{
        for(unsigned i = 0; i<thread_count;++i){
            m_threads.push_back(new std::thread(&ThreadPool::worker_thread,this));
        }
    }catch(...){
        m_done=true;
        throw;
    }
}
void ThreadPool::worker_thread()
{
    while(!m_done){
        std::function<void()> task;
        if(m_workQueue.try_pop(task)){
            std::cout<<"\n=============开启一个新线程============="<<std::endl;
            task();
        }else std::this_thread::yield();
    }
}

ThreadPool::~ThreadPool()
{
    m_done=true;
    for(auto& item:m_threads){
        if(item->joinable()){
            item->join();
        }
    }
}

void ThreadPool::submit(std::function<void()> f)
{
    m_workQueue.push(std::function<void()>(f));
}

