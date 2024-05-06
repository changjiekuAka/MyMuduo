#include "noncopyable.h"
#include <string>
#include <iostream>

enum Loglevel
{
    INFO,   //普通信息
    FATAL,  //core信息
    ERROR,  //错误信息
    DEBUG   //调试信息
};

#define LOG_INFO(logmsgformat,...) \
        do \
        {\
            Logger& logger = Logger::GetInstance();   \
            logger.SetLogLevel(INFO); \
            char buf[1024] = {0}; \
            snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
            logger.log(buf); \
        }while(0)

#define LOG_FATAL(logmsgformat,...) \
        do \
        {\
            Logger& logger = Logger::GetInstance();   \
            logger.SetLogLevel(FATAL); \
            char buf[1024] = {0}; \
            snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
            logger.log(buf); \
        }while(0)

#define LOG_ERROR(logmsgformat,...) \
        do \
        {\
            Logger& logger = Logger::GetInstance();   \
            logger.SetLogLevel(ERROR); \
            char buf[1024] = {0}; \
            snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
            logger.log(buf); \
        }while(0)
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgformat,...) \
        do \
        {\
            Logger& logger = Logger::GetInstance();   \
            logger.SetLogLevel(DEBUG); \
            char buf[1024] = {0}; \
            snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
            logger.log(buf); \
        }while(0)
#else
    #define LOG_DEBUG(logmsgformat,...)
#endif

class Logger : noncopyable
{
public:
    static Logger& GetInstance();
    void SetLoglevel(int level);
    void log(std::string msg);
private:
    int m_loglevel;
};