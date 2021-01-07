#ifndef __DATE_H__
#define __DATE_H__

#include <Arduino.h>

struct Date
{
    const byte y, m, d;

    bool operator==(const Date &o) const;
    bool operator<(const Date &o) const;
    bool operator>(const Date &o) const;
    bool operator<=(const Date &o) const;
    bool operator>=(const Date &o) const;

    int operator-(const Date &o) const;

    int daysTo(const Date &o) const;
};

#endif