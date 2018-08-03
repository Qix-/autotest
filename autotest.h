#ifndef AUTOTEST_H__
#define AUTOTEST_H__
#pragma once

#ifndef AUTOTEST_BUILDING
#	error "You don't need to include autotest.h - just link against the library."
#endif

typedef void (*test_case_fn)(void);

typedef struct test_case {
	test_case_fn fn;
	const char *name;
	int skip;
	int has_signal_check;
	int allow_segv;
	int allow_abrt;
	int must_fail;
	struct test_case *next;
} test_case;

__attribute__((noreturn)) void bailout(const char *fmt, ...);

void discover_tests(const char *arg0, test_case **cases);

int populate_test_case(test_case *tcase, const char *name, test_case_fn fn);

#endif
