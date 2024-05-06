#include "logger.h"


Logger& Logger::GetInstance()
{
    static Logger& instance;
    return instance;
}

void Logger::SetLoglevel(int level)
{
    m_loglevel = level;
}

void Logger::log(std::string msg)
{
    switch (m_loglevel)
    {
    case INFO:

        break;
    case FATAL:

        break;
    case ERROR:

        break;
    case DEBUG:

        break;
    
    default:
        break;
    }
    
}