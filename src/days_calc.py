#!/usr/bin/env python3

# A bare Date class
class Date:
    def __init__(self, y: int, m: int, d: int):
        self.y = y
        self.m = m
        self.d = d

    def __eq__(self, o):
        return self.y == o.y and self.m == o.m and self.d == o.d

    def __lt__(self, o):
        return (self.y < o.y or
                self.y == o.y and self.m < o.m or
                self.y == o.y and self.m == o.m and self.d < o.d)

    def __gt__(self, o):
        return (self.y > o.y or
                self.y == o.y and self.m > o.m or
                self.y == o.y and self.m == o.m and self.d > o.d)

    def __le__(self, o):
        return not self > o

    def __ge__(self, o):
        return not self < o

    def __sub__(self, dst) -> int:
        return dst.daysTo(self)

    def daysTo(self, dst) -> int:
        def lp(m, y):
            return m == 2 and not (y % 4)

        DIM = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
        src = self
        if src == dst:
            return 0
        neg = False
        if src > dst:
            neg = True
            src, dst = dst, src
        ds = 0
        if src.y == dst.y:
            for m in range(src.m, dst.m):
                ds += DIM[m - 1] + lp(m, src.y)
        else:
            for m in range(src.m, 13):
                ds += DIM[m - 1] + lp(m, src.y)
            for m in range(1, dst.m):
                ds += DIM[m - 1] + lp(m, dst.y)
            for y in range(src.y + 1, dst.y):
                ds += 365 + lp(2, y)
        ds += dst.d - src.d
        return -ds if neg else ds

if __name__ == '__main__':
    base = Date(2021, 1, 1)
    target = Date(2022, 6, 7) # Change this to your desired date
    print(target - base)
