/* time.h */
#ifndef CLAUDEC99_TIME_H
#define CLAUDEC99_TIME_H

typedef long time_t;
typedef long clock_t;

struct tm { int tm_sec; int tm_min; int tm_hour; int tm_mday; int tm_mon; int tm_year; int tm_wday; int tm_yday; int tm_isdst; };

#define CLOCKS_PER_SEC 1000000L

time_t time(time_t *);
clock_t clock(void);
struct tm *localtime(time_t *);
int strftime(char *, long, const char *, struct tm *);

#endif
