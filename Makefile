CFLAGS=-O -Wno-unused-result
test_profile: test_profile.o

.PHONY: clean
clean:
	rm test_profile test_profile.o

.PHONY: run
run: test_profile
	export PERF_COUNT_HW_CPU_CYCLES=1; ./test_profile
