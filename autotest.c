#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AUTOTEST_BUILDING
#include "./autotest.h"

extern int discover_tests(const char *arg0, test_case **cases);

static jmp_buf jmpbuf;
static FILE *real_stdout;
static int real_stdout_fd;

int main(int argc, const char **argv) {
	int result;
	test_case *test_cases;

	(void) argc;
	(void) jmpbuf; /* XXX DEBUG */

	real_stdout_fd = dup(1);
	if (real_stdout_fd == -1) {
		printf("Bail out! Could not duplicate standard output file descriptor: %s\n", strerror(errno));
		return 1;
	}

	if (dup2(2, 1) == -1) {
		printf("Bail out! Could not redirect standard output to standard error: %s\n", strerror(errno));
		return 1;
	}

	real_stdout = fdopen(real_stdout_fd, "wb");
	if (real_stdout == 0) {
		const char *msg = "Bail out! Could not open new standard out file descriptor as a file: ";
		write(real_stdout_fd, "Bail out! Could not open new standard out file descriptor as a file: ", strlen(msg));
		write(real_stdout_fd, strerror(errno), strlen(strerror(errno)));
		write(real_stdout_fd, "\n", 1);
		return 1;
	}

	/* get test cases */
	result = discover_tests(argv[0], &test_cases);
	if (result != 0) {
		/* test case discovery should have printed the error already */
		return result;
	}

	fputs("TAP version 13\n", real_stdout);

	if (test_cases) {
		size_t num_cases = 0;
		const test_case *tcase;
		const test_case *tcase_tofree;
		for (tcase = test_cases; tcase;) {
			++num_cases;

			if (tcase->skipped) {
				fprintf(real_stdout, "ok %lu %s # SKIP Symbol prefixed with underscore (_TEST_%s)\n", num_cases, tcase->name, tcase->name);
			} else {
				tcase->fn();
				fprintf(real_stdout, "ok %lu %s\n", num_cases, tcase->name);
			}

			tcase_tofree = tcase;
			tcase = tcase->next;
			free((void *) tcase_tofree);
		}

		fprintf(real_stdout, "1..%lu\n", num_cases);
	} else {
		fputs("0..0\n", real_stdout);
	}

	return 0;
}
