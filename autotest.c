#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./test-case.h"

extern int discover_tests(const char *arg0, test_case **cases);

int main(int argc, const char **argv) {
	int i;
	int verbose = 0;
	int tap = 0;
	int result;
	test_case *test_cases;

	(void) tap; /* XXX DEBUG */
	(void) verbose; /* XXX DEBUG */

	/* parse arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--tap") == 0) {
			tap = 1;
		} else if (strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else if (strcmp(argv[i], "--help") == 0) {
			fprintf(stderr, "usage: %s [--verbose] [--tap]\n", argv[0]);
			return 2;
		} else {
			fprintf(stderr, "%s: error: unknown flag: %s\n", argv[0], argv[i]);
			return 2;
		}
	}

	/* get test cases */
	result = discover_tests(argv[0], &test_cases);
	if (test_cases != 0) {
		/* test case discovery should have printed the error already */
		return result;
	}

	if (test_cases) {
		free(test_cases);
	} else {
		fprintf(stderr, "%s: warning: no test cases\n", argv[0]);
	}

	return 0;
}
