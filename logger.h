#include "noncopyable.h"
#include <string>

enum Loglevel
{
    INFO,   //普通信息
    FATAL,  //core信息
    ERROR,  //错误信息
    DEBUG   //调试信息
};


class Logger : noncopyable
{
public:
    static Logger& GetInstance();
    void SetLoglevel(int level);
    void log(std::string msg);
private:
    int m_loglevel;
};