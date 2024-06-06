#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>
#include <assert.h>

std::atomic_int Thread::numCreated_ = {0};

Thread::Thread(ThreadFunc func,const std::string& name)
    :started_(false),
    joined_(false),
    func_(std::move(func)),
    tid_(0),
    name_(name)
{

}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return thread_->join();
}
void Thread::start() // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);

    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_();
    }));
    // 使用信号量等待，新线程创建成功tid_被成功赋值，其他线程也就可以通过这个tid_访问这个线程
    sem_wait(&sem);
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[128] = {0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_ = buf;
    }
}