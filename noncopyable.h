#pragma once

/***
 * 
 * noncopyable对象被继承后可以正常的创建和析构、但是派生类不能够
 * 拷贝构造和赋值操作
 ***/   

class noncopyable
{
public:
    noncopyable(const noncopyable& ) = delete;
    noncopyable& operator=(const noncopyable& ) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};