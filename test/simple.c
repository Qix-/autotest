#include <assert.h>
#include <stdio.h>

void TEST_hello(void) {
	puts("Hello, world!");
}

void _TEST_skipped(void) {
	assert(0 && "this should be skipped...");
}

void TEST_ABRT_abort(void) {
	assert(0 && "this should abort");
}

void TEST_SEGV_segfault(void) {
	int *literally_death = (int *) 0x0;
	*literally_death = 1337;
}

void TEST_FAIL_failure(void) {
	TEST_SEGV_segfault();
}
