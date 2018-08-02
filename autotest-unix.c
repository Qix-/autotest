#include <stdio.h>

typedef void (test_case)(void);

int discover_tests(const char *arg0, test_case **cases) {
	(void) arg0;
	(void) cases;
	puts("hello");
	return 0;
}
