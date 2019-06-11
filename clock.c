/*
 * Rainbow Crackalack: clock.c
 * Copyright (C) 2019  Joe Testa <jtesta@positronsecurity.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "clock.h"


/* Windows does not have a clock_gettime() implementation.  The following
 * implementation was taken from winpthreads (src/clock.c) (public domain):
 * <https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/src/clock.c> */
#ifdef _WIN32
#include <windows.h>

#define POW10_9 1000000000
int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    LARGE_INTEGER pf, pc;

    if (clock_id == CLOCK_MONOTONIC)
        {
            if (QueryPerformanceFrequency(&pf) == 0)
                return -1;

            if (QueryPerformanceCounter(&pc) == 0)
                return -1;

            tp->tv_sec = pc.QuadPart / pf.QuadPart;
            tp->tv_nsec = (int) (((pc.QuadPart % pf.QuadPart) * POW10_9  + (pf.QuadPart >> 1)) / pf.QuadPart);
            if (tp->tv_nsec >= POW10_9) {
                tp->tv_sec ++;
                tp->tv_nsec -= POW10_9;
            }

            return 0;
        }
    return -1;
}
#endif


/* Gets the elapsed seconds from a timer (see start_timer()). */
double get_elapsed(struct timespec *start) {
  struct timespec end;


  if (clock_gettime(CLOCK_MONOTONIC, &end)) {
    fprintf(stderr, "Error while calling clock_gettime(): %s (%d)\n", strerror(errno), errno);
    return 0.0;
  }

  return (end.tv_sec + (end.tv_nsec / 1000000000.0)) - (start->tv_sec + (start->tv_nsec / 1000000000.0));
}


/* Converts number of seconds into human-readable time, such as "X mins, Y secs". */
void seconds_to_human_time(char *buf, unsigned int buf_size, double seconds) {
#define ONE_MINUTE (60)
#define ONE_HOUR (ONE_MINUTE * 60)
#define ONE_DAY (ONE_HOUR * 24)
  unsigned int seconds_uint = (unsigned int)seconds;
  if (seconds_uint < ONE_MINUTE)
    snprintf(buf, buf_size - 1, "%.1f secs", seconds);
  else if ((seconds_uint >= ONE_MINUTE) && (seconds_uint < ONE_HOUR))
    snprintf(buf, buf_size - 1, "%u mins, %u secs", seconds_uint / ONE_MINUTE, seconds_uint % ONE_MINUTE);
  else if ((seconds_uint >= ONE_HOUR) && (seconds_uint < ONE_DAY))
    snprintf(buf, buf_size - 1, "%u hours, %u mins", (unsigned int)(seconds_uint / ONE_HOUR), (unsigned int)((seconds_uint % ONE_HOUR) / ONE_MINUTE));
  else if (seconds_uint >= ONE_DAY)
    snprintf(buf, buf_size - 1, "%u days, %u hours", (unsigned int)(seconds_uint / ONE_DAY), (unsigned int)((seconds_uint % ONE_DAY) / ONE_HOUR));
}


/* Starts a timer. */
void start_timer(struct timespec *start) {
  if (clock_gettime(CLOCK_MONOTONIC, start)) {
    fprintf(stderr, "Error while calling clock_gettime(): %s (%d)\n", strerror(errno), errno);
    start->tv_sec = 0;
    start->tv_nsec = 0;
  }
}
