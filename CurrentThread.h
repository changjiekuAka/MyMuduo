#pragma once
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>

namespace CurrentThread
{

    /*
        __thread 是系统提供的
        thread_local是C++11提供的

        这里的t_cachedTid是一个全局变量但是被__thread修饰后，这个变量被当前线程的修改其他线程看不见
        也就是说这个变量是每个线程都有一个自己的，也就是自己的线程tid
    */
    extern __thread int t_cachedTid;
    
    // 获取tid的是一个系统调用的过程，需要从用户态切换到内核态十分消耗效率
    // 所以每次获取完先缓存起来
    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid  == 0,0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}