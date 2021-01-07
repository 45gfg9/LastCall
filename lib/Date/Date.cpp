#include "Date.h"

const byte DAYS_IN_MONTH[] {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define LP(m, y) ((m) == 2 && !((y) % 4))
#define ACC(v, s, d, y)         \
    for (int m = s; m < d; m++) \
    v += DAYS_IN_MONTH[m - 1] + LP(m, y)

bool Date::operator==(const Date &o) const
{
    return (y == o.y) && (m == o.m) && (d == o.d);
}

bool Date::operator<(const Date &o) const
{
    return (y < o.y) ||
           (y == o.y && m < o.m) ||
           (y == o.y && m == o.m && d < o.d);
}

bool Date::operator>(const Date &o) const
{
    return (y < o.y) ||
           (y == o.y && m > o.m) ||
           (y == o.y && m == o.m && d > o.d);
}

bool Date::operator<=(const Date &o) const
{
    return !(*this > o);
}

bool Date::operator>=(const Date &o) const
{
    return !(*this < o);
}

int Date::operator-(const Date &o) const
{
    return o.daysTo(*this);
}

int Date::daysTo(const Date &o) const
{
    if (*this == o)
        return 0;
    bool neg = false;
    const Date *src = this;
    const Date *dst = &o;
    if (*this > o)
    {
        neg = true;
        src = &o;
        dst = this;
    }

    int ds = 0;
    if (src->y == dst->y)
    {
        ACC(ds, src->m, dst->m, src->y);
    }
    else {
        ACC(ds, src->m, 13, src->y);
        ACC(ds, 1, dst->m, dst->y);
        for (int y = src->y + 1; y < dst->y; y++)
            ds += 365 + LP(2, y);
    }
    ds += dst->d - src->d;

    return neg ? -ds : ds;
}