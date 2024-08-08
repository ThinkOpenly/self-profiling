#ifndef PROFILE_H
#define PROFILE_H
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// Define performance events and file descriptors
#define EVENT_TYPE_NAME_HARDWARE(t,e) {#e,t,e}
#define EVENT_TYPE_NAME_CACHE(t,e,n) {n,t,e}
#define EVENT_HW_CACHE(id,op,rc) ((id)|((op)<<8)|((rc)<<16))
static struct {
    const char *_event_name;
    __u32 _event_type;
    __u64 _event_config;
} _profile_events[] = {
    EVENT_TYPE_NAME_HARDWARE(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CPU_CYCLES),
    EVENT_TYPE_NAME_HARDWARE(PERF_TYPE_HARDWARE,PERF_COUNT_HW_INSTRUCTIONS),
    EVENT_TYPE_NAME_HARDWARE(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_INSTRUCTIONS),
    EVENT_TYPE_NAME_HARDWARE(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_MISSES),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_L1D,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_ACCESS),
        "PERF_COUNT_L1D_READ_ACCESS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_L1D,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_MISS),
        "PERF_COUNT_L1D_READ_MISS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_L1D,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_ACCESS),
        "PERF_COUNT_L1D_WRITE_ACCESS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_L1D,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_MISS),
        "PERF_COUNT_L1D_WRITE_MISS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_LL,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_ACCESS),
        "PERF_COUNT_LL_READ_ACCESS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_LL,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_MISS),
        "PERF_COUNT_LL_READ_MISS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_LL,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_ACCESS),
        "PERF_COUNT_LL_WRITE_ACCESS"),
    EVENT_TYPE_NAME_CACHE(PERF_TYPE_HW_CACHE,
        EVENT_HW_CACHE(PERF_COUNT_HW_CACHE_LL,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_MISS),
        "PERF_COUNT_LL_WRITE_MISS")
};
static int _profile_fd[sizeof(_profile_events)/sizeof(_profile_events[0])] = {-1};
static int _profile_fds = 0;
static struct _profile_read_format {
    __u64 _event_count;
    struct {
        __u64 _event_value;
    } _event_values[sizeof(_profile_events)/sizeof(_profile_events[0])];
} _profile_start, _profile_last;

#define PROFILE_BEGIN() { \
    struct perf_event_attr _perf_event_attr; \
    memset(&_perf_event_attr, 0, sizeof(struct perf_event_attr)); \
    _perf_event_attr.size = sizeof(struct perf_event_attr); \
    _perf_event_attr.read_format = PERF_FORMAT_GROUP; \
    _perf_event_attr.disabled = 1; \
    _perf_event_attr.exclude_kernel = 1; \
    _perf_event_attr.exclude_hv = 1; \
    _profile_fd[0] = -1; \
    _profile_fds = 0; \
    for (int _index = 0; _index < sizeof(_profile_events)/sizeof(_profile_events[0]); _index++) { \
        if (getenv(_profile_events[_index]._event_name) == NULL) continue; \
        _perf_event_attr.type = _profile_events[_index]._event_type; \
        _perf_event_attr.config = _profile_events[_index]._event_config; \
        _profile_fd[_profile_fds] = perf_event_open(&_perf_event_attr, 0, -1, _profile_fd[0], 0); \
        if (_profile_fd[_profile_fds] == -1) { \
            fprintf(stderr, "Error opening leader %llx\n", _perf_event_attr.config); \
            perror("perf_event_open"); \
            exit(1); \
        } \
        _profile_fds++; \
    } \
    ioctl(_profile_fd[0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP); \
}

#define PROFILE_START() { \
    ioctl(_profile_fd[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP); \
    read(_profile_fd[0], &_profile_start, sizeof(_profile_start)); \
}

#define PROFILE_SNAP() { \
    read(_profile_fd[0], &_profile_last, sizeof(_profile_last)); \
}

#define PROFILE_STOP() { \
    read(_profile_fd[0], &_profile_last, sizeof(_profile_last)); \
    ioctl(_profile_fd[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP); \
}

#define PROFILE_END() { \
    for (int i = _profile_fds; i > 0; i--) { \
        close(_profile_fd[i-1]); \
    } \
}

#define PROFILE_REPORT() { \
    int _fds = 0; \
    for (int _index = 0; _index < sizeof(_profile_events)/sizeof(_profile_events[0]); _index++) { \
        if (getenv(_profile_events[_index]._event_name) == NULL) continue; \
        printf("%s(%llu): %llu\n", _profile_events[_index]._event_name, _profile_events[_index]._event_config, _profile_last._event_values[_fds]._event_value - _profile_start._event_values[_fds]._event_value); \
        _fds++; \
    } \
}

#endif
