# self-profiling

This repository contains one method for implementing self-profiling of
an application.

## Introduction

Self-profiling differs from profiling by allowing an application to
tightly define the range of processing. This avoids extraneous data
that may come from profiling the entire application -- like start-up,
termination, and I/O.

One common drawback to self-profiling is that in order to change which
events are to be counted/recorded, the application source needs to be
changed and the application rebuilt. Rebuilding an application can have
unexpected impacts like changing the alignments of data and/or
instructions, which can significantly impact performance.
The method implemented here is extensible for new events and flexible
in that selecting which events are to be recorded can be done with
environment variables. Implementing general support for new events does
require rebuilding the application. However, once support for all
interesting events is included, the application can report event counts
for any of the set of supported events. In this way, the same _binary_
can be used for all data collection, avoiding possible impacts when an
application is rebuilt.

The method of self-profiling used is architecture-independent. In that
way, results should be reasonably consistent across all systems that
support the `perf_event_open` system call and the generic perf events.
Older methods where the application reads a special register that
counts cycles (or instructions) are (1) architcure-specific so that
different code for each supported architecture is required, resulting in
a higher maintenance burden as well as possibly different results, and
(2) generally deprecated due to security concerns.

## Running the example

An example is included which:
1. Peforms some I/O (a print statement).
1. Sorts some data.
1. Performs some more I/O (more print statements).

In this case, only the data sorting is to be profiled.
Naively using `perf` on the application will include cycles and events
for the I/O, which will greatly distort the results.

### Build the example

```
$ make
```
A new program, `test_profile` is built.

### Use `perf` to count cycles

```
$ perf stat -e cycles ./test_profile
[...]
 Performance counter stats for './test_profile':

           373,817      cycles:u
```

### Use self-profiling to count cycles

```
$ PERF_COUNT_HW_CPU_CYCLES=1 ./test_profile
[...]
PERF_COUNT_HW_CPU_CYCLES(0): 6896
```

## Instrumenting an application (the API)

The best reference is the included example, `test_profile.c`:
```
#include "profile.h"
int main() {
    PROFILE_BEGIN();  /* initialization */
    /* code that will not be profiled */
    PROFILE_START();  /* begin profiling */
    /* code to be profiled */
    PROFILE_STOP();   /* stop profiling */
    /* code that will not be profiled */
    PROFILE_REPORT(); /* print event names and counts */
    PROFILE_END();    /* shut down */
[...]
```

It is also possible to take a snapshot of counters during the run
without stopping the counters:
```
    PROFILE_SNAP();   /* stop profiling */
```
Accessing and manipulating this data is left as an exercise.
Everthing can be seen in `profile.h`.

## Supported events

At the time of this writing, the following events are supported:
- PERF_COUNT_HW_CPU_CYCLES
- PERF_COUNT_HW_INSTRUCTIONS
- PERF_COUNT_HW_BRANCH_INSTRUCTIONS
- PERF_COUNT_HW_BRANCH_MISSES
- "PERF_COUNT_L1D_READ_ACCESS"
- "PERF_COUNT_L1D_READ_MISS"
- "PERF_COUNT_L1D_WRITE_ACCESS"
- "PERF_COUNT_L1D_WRITE_MISS"
- "PERF_COUNT_LL_READ_ACCESS"
- "PERF_COUNT_LL_READ_MISS"
- "PERF_COUNT_LL_WRITE_ACCESS"
- "PERF_COUNT_LL_WRITE_MISS"

## General usage

Instrument your application as in the example provided.
The following important elements need to be included:
1. `#include <self-profile.h>`
   This include file defines the API for your program to use.
1. `PROFILE_BEGIN()`
   This performs some initialization. Generally, add this
   anywhere before the code to be profiled.
1. `PROFILE_START()`
   This starts the performance counters. Put this immediately
   before the code to be profiled.
1. `PROFILE_STOP()`
   This halts the performance counters. Put this immediately
   after the code being profiled.
1. `PROFILE_REPORT()`
   The emits the collected counters to standard output. Put
   this after `PROFILE_STOP`.
1. `PROFILE_END()`
   This frees resources allocated by `PROFILE_BEGIN`. Put
   this after `PROFILE_STOP`.

Note that there are other use-cases not described here,
such as profiling part of a loop where counters may be
paused and restarted, or where a snapshot of current
counters is needed without stopping the counters. The current
implementation may not obviously handle these cases, but the
implementation in `self-profile.h` is not complex and can
easily be adapted.

Then, build your application.

To access the performance counters, you may need to increase
general privileges as set in `/proc/sys/kernel/perf_event_paranoid`.
By default, this will likely be set to "4", but needs to be
less than 4:
```
$ echo 3 | /usr/bin/sudo /usr/bin/tee /proc/sys/kernel/perf_event_paranoid
```

To collect and report data for a set of counters, simply set
the respective environment for the counter before running the
application. The variable names are shown in "Supported events",
above. For example:
```
$ export PERF_COUNT_HW_CPU_CYCLES=1
$ export PERF_COUNT_HW_INSTRUCTIONS=1
$ export PERF_COUNT_HW_BRANCH_INSTRUCTIONS=1
$ export PERF_COUNT_HW_BRANCH_MISSES=1
```

Then, run your application.

## Troubleshooting

### Insufficient privileges

```
$ PERF_COUNT_HW_CPU_CYCLES=1 ./test_profile
Error opening leader 0
perf_event_open: Permission denied
```

The user will need some slightly elevated privileges in order to
enable profiling. The current default privilege level can be seen
in `/proc/sys/kernel/perf_event_paranoid`:
```
$ cat /proc/sys/kernel/perf_event_paranoid
4
```

Lower values means increased privileges. A level of "3" is likely
sufficient for basic counters:
```
echo 3 | /usr/bin/sudo /usr/bin/tee /proc/sys/kernel/perf_event_paranoid
```

### Too many events selected

If the report contains counts which are all zero, but are expected to be
positive, it could be that too many events are selected.
```
PERF_COUNT_HW_CPU_CYCLES(0): 0
PERF_COUNT_HW_INSTRUCTIONS(1): 0
PERF_COUNT_HW_BRANCH_INSTRUCTIONS(4): 0
PERF_COUNT_HW_BRANCH_MISSES(5): 0
PERF_COUNT_L1D_READ_ACCESS(0): 0
PERF_COUNT_L1D_READ_MISS(65536): 0
PERF_COUNT_L1D_WRITE_ACCESS(256): 0
PERF_COUNT_LL_READ_ACCESS(2): 0
PERF_COUNT_LL_READ_MISS(65538): 0
```

Try enabling fewer events.

### Unsupported events

```
$ PERF_COUNT_L1D_WRITE_MISS=1 ./test_profile
Error opening leader 10100
perf_event_open: No such file or directory
```

A selected event is likely unsupported, and cannot be profiled.
