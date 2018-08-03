#ifndef AUTOTEST_H__
#define AUTOTEST_H__
#pragma once

#ifndef AUTOTEST_BUILDING
#	error "You don't need to include autotest.h - just link against the library."
#endif

typedef struct test_case {
	void (*fn)(void);
	const char *name;
	int skipped;
	struct test_case *next;
} test_case;

__attribute__((noreturn)) void bailout(const char *fmt, ...);

void discover_tests(const char *arg0, test_case **cases);

#endif
