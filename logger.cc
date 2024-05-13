#include "logger.h"
#include "Timestamp.h"


Logger& Logger::GetInstance()
{
    static Logger instance;
    return instance;
}

void Logger::SetLogLevel(int level)
{
    m_loglevel = level;
}

void Logger::log(std::string msg)
{
    switch (m_loglevel)
    {
    case INFO:
        std::cout << "INFO" ;
        break;
    case FATAL:
        std::cout << "FATAL" ;
        break;
    case ERROR:
        std::cout << "ERROR" ;
        break;
    case DEBUG:
        std::cout << "DEBUG" ;
        break;
    default:
        break;
    }
    
    std::cout << TimeStamp::now().toString() << " : " << msg << std::endl;
}