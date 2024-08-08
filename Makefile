CFLAGS=-O -Wno-unused-result

all: test_profile self-profile preload_test_profile bsearch

test_profile: test_profile.o

self-profile:
	$(CC) self-profile.c -o self-profile.so -fPIC -shared -ldl

preload_test_profile: preload_test_profile.o

bsearch: bsearch.o

.PHONY: clean
clean:
	rm test_profile test_profile.o self-profile.so preload_test_profile.o preload_test_profile bsearch.o bsearch

.PHONY: run
run: test_profile
	export PERF_COUNT_HW_CPU_CYCLES=1; ./test_profile
	export PERF_COUNT_HW_CPU_CYCLES=1; LD_PRELOAD=self-profile.so ./preload_test_profile
	export PERF_COUNT_HW_CPU_CYCLES=1; LD_PRELOAD=self-profile.so ./bsearch
