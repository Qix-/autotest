#ifndef AUTOTEST_TEST_CASE_H__
#define AUTOTEST_TEST_CASE_H__
#pragma once

typedef struct test_case {
	void (*fn)(void);
	const char *name;
	int skipped;
	struct test_case *next;
} test_case;

#endif
