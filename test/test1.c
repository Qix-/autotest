#include <assert.h>
#include <stdio.h>

void TEST_hello(void) {
	puts("Hello, world!");
}

void _TEST_skipped(void) {
	assert(0 && "this should be skipped...");
}

void TEST_failure(void) {
	assert(0 && "this should fail");
}

void TEST_segfault(void) {
	int *literally_death = (int *) 0x0;
	*literally_death = 1337;
}
