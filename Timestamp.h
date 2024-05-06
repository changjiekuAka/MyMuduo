#pragma once

#include <time.h>
#include <string>

class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int microSecondsSinceEpoch);
    static TimeStamp now();
    std::string toString() const;
private:
    __int64_t microSecondsSinceEpoch_;
};