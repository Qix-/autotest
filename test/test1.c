#include <assert.h>
#include <stdio.h>

void TEST_hello(void) {
	puts("Hello, world!");
}

__attribute__ ((visibility ("default"))) void _TEST_skipped(void) {
	assert(0 && "this should be skipped...");
}