#include "Timestamp.h"
#include <iostream>

TimeStamp::TimeStamp() : microSecondsSinceEpoch_(0) 
{

}

TimeStamp::TimeStamp(int microSecondsSinceEpoch){
    microSecondsSinceEpoch_ = microSecondsSinceEpoch;
}

TimeStamp TimeStamp::now()
{
    return TimeStamp(time(NULL));
}

std::string TimeStamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf,128,"%04d/%02d/%02d %02d:%02d:%02d",tm_time->tm_year + 1900,
                                                    tm_time->tm_mon + 1,
                                                    tm_time->tm_mday,
                                                    tm_time->tm_hour,
                                                    tm_time->tm_min,
                                                    tm_time->tm_sec);
    return buf;
}

int main()
{
    std::cout << TimeStamp::now().toString() << std::endl;
    return 0;
}