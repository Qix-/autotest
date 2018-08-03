#ifndef AUTOTEST_H__
#define AUTOTEST_H__
#pragma once

#ifndef AUTOTEST_BUILDING
#	error "You don't need to include autotest.h - just link against the library."
#endif

void bailout(const char *fmt, ...) __attribute__((noreturn));

typedef struct test_case {
	void (*fn)(void);
	const char *name;
	int skipped;
	struct test_case *next;
} test_case;

#endif
