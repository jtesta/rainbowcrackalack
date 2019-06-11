#ifndef _CLOCK_H
#define _CLOCK_H

#include <time.h>

#ifdef _WIN32
int clock_gettime(clockid_t clock_id, struct timespec *tp);
#endif

double get_elapsed(struct timespec *start);
void seconds_to_human_time(char *buf, unsigned int buf_size, double seconds);
void start_timer(struct timespec *start);

#endif
