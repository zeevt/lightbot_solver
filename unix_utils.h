#ifndef UNIX_UTILS_H_
#define UNIX_UTILS_H_

#ifdef __APPLE__

#include <mach/mach_time.h>

#define timestamp_t             uint64_t
#define init_timestamper()      mach_timebase_info(&timebase)
#define get_timestamp(t)        t = mach_absolute_time()
#define timestamp_diff(t1,t2)   mach_diff(&t1, &t2)
#define PRINTF_TIMESTAMP_STR    "%.9f"
#define PRINTF_TIMESTAMP_VAL(t) (double)t / 1e9

static mach_timebase_info_data_t timebase = { 0,0 };

static inline void mach_diff(uint64_t *tp1, const uint64_t *tp2)
{
  uint64_t diff = *tp2 - *tp1;
  *tp1 = diff * timebase.numer / timebase.denom;
}

#else

#include <time.h>

#define timestamp_t             struct timespec
#define init_timestamper()
#define get_timestamp(t)        clock_gettime(CLOCK_MONOTONIC, &t)
#define timestamp_diff(t1,t2)   timespec_diff(&t1, &t2)
#define timestamp_add(t1,t2)    timespec_add(&t1, &t2)
#define timestamp_clear(t)      do { t.tv_sec = 0; t.tv_nsec = 0; } while(0)
#define PRINTF_TIMESTAMP_STR    "%d.%09ld"
#define PRINTF_TIMESTAMP_VAL(t) (int)t.tv_sec, t.tv_nsec

#define ONE_BILLION 1000000000

static inline void timespec_diff(struct timespec* tp1, const struct timespec* tp2)
{
  tp1->tv_sec = tp2->tv_sec - tp1->tv_sec;
  tp1->tv_nsec = tp2->tv_nsec - tp1->tv_nsec;
  if (tp1->tv_nsec < 0)
  {
    tp1->tv_sec--;
    tp1->tv_nsec += ONE_BILLION;
  }
}

static inline void timespec_add(struct timespec* dst, const struct timespec* src)
{
  dst->tv_sec += src->tv_sec;
  dst->tv_nsec += src->tv_nsec;
  if (dst->tv_nsec > ONE_BILLION)
  {
    dst->tv_sec++;
    dst->tv_nsec -= ONE_BILLION;
  }
}

#endif /* __APPLE__ */

#endif /* UNIX_UTILS_H_ */
