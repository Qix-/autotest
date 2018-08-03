#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#	define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AUTOTEST_BUILDING
#include "./autotest.h"

static jmp_buf *jmpbuf_harness;
static sigjmp_buf *jmpbuf_test;
static FILE *real_stdout;
static int real_stdout_fd;
static struct sigaction sa;

__attribute__((noreturn)) void bailout(const char *fmt, ...) {
	va_list ap;
	fputs("Bail out! ", real_stdout);
	va_start(ap, fmt);
	vfprintf(real_stdout, fmt, ap);
	va_end(ap);
	fputs("\n", real_stdout);
	longjmp(*jmpbuf_harness, 1);
	__builtin_unreachable();
}

int populate_test_case(test_case *tcase, const char *name, test_case_fn fn) {
	if (*name == '\0') return 1;

	tcase->skip = 0;
	tcase->allow_segv = 0;
	tcase->allow_abrt = 0;
	tcase->must_fail = 0;
	tcase->name = NULL;
	tcase->next = NULL;

	if (*name == '_') {
		++name;
		tcase->skip = 1;
	}

	if (strncmp(name, "TEST_", 5) == 0) {
		name += 5;
	} else {
		return 1;
	}

	while (1) {
		if (strncmp(name, "SEGV_", 5) == 0) {
			tcase->allow_segv = 1;
			name += 5;
			continue;
		}

		if (strncmp(name, "ABRT_", 5) == 0) {
			tcase->allow_abrt = 1;
			name += 5;
			continue;
		}

		if (strncmp(name, "FAIL_", 5) == 0) {
			tcase->must_fail = 1;
			name += 5;
			continue;
		}

		break;
	}

	if (*name == '\0') {
		/* don't allow zero-length names */
		return 1;
	}

	tcase->name = name;
	tcase->fn = fn;

	return 0;
}

__attribute__((noreturn)) static void handle_sig(int sig) {
	if (sig == SIGSEGV) {
		siglongjmp(*jmpbuf_test, 2);
	} else if (sig == SIGABRT) {
		siglongjmp(*jmpbuf_test, 1);
	} else {
		bailout("Autotest signal handler encountered unknown signal: %s (%d)", strsignal(sig), sig);
	}
	__builtin_unreachable();
}

static void install_sighandlers(void) {
	sa.sa_handler = handle_sig;
	sigemptyset(&(sa.sa_mask));
	if (sigaction(SIGABRT, &sa, NULL) == -1) bailout("Could not install SIGABRT action: %s", strerror(errno));
	if (sigaction(SIGSEGV, &sa, NULL) == -1) bailout("Could not install SIGSEGV action: %s", strerror(errno));
}

int main(int argc, const char **argv) {
	test_case *test_cases;
	volatile int had_failure = 0;
	jmp_buf _jmpbuf_harness;

	(void) argc;

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

	jmpbuf_harness = &_jmpbuf_harness;
	if (setjmp(_jmpbuf_harness) == 1) {
		/* bailout() was called */
		return 1;
	}

	discover_tests(argv[0], &test_cases);

	fputs("TAP version 13\n", real_stdout);

	if (test_cases) {
		volatile size_t num_cases = 0;
		const test_case *tcase;
		const test_case *tcase_tofree;

		for (tcase = test_cases; tcase;) {
			volatile int failed = 0;
			const char * volatile message = NULL;
			++num_cases;

			tcase_tofree = NULL;

			install_sighandlers();

			if (tcase->skip) {
				fprintf(
					real_stdout,
					"ok %lu %s # SKIP Symbol prefixed with underscore (_TEST_%s)\n",
					num_cases, tcase->name, tcase->name);
			} else {
				sigjmp_buf _jmpbuf_test;
				jmpbuf_test = &_jmpbuf_test;
				switch (sigsetjmp(_jmpbuf_test, 1)) {
				case 0:
					tcase->fn();
					failed = 0;
					message = NULL;
					break;
				case 1: /* SIGABRT */
					failed = !tcase->allow_abrt;
					message = "Test case aborted";
					break;
				case 2: /* SIGSEGV */
					failed = !tcase->allow_segv;
					message = "Test case encountered a segfault";
					break;
				}

				fflush(stderr);

				if (tcase->must_fail == failed) {
					failed = 0;
					fprintf(
						real_stdout,
						"ok %lu %s\n",
						num_cases, tcase->name);
				} else if (tcase->must_fail && !failed) {
					failed = 1;
					fprintf(
						real_stdout,
						"not ok %lu %s\n  ---\n  message: Test case was expected to fail\n  severity: fail\n  ...\n",
						num_cases, tcase->name);
				} /* else case handled above */

				if (message != NULL) {
					fprintf(
						real_stdout,
						"  ---\n  message: %s\n  severity: %s\n  ...\n",
						message,
						failed ? "fail" : "comment");
				}

				had_failure = had_failure || failed;
			}

			tcase_tofree = tcase;
			tcase = tcase->next;
			free((void *) tcase_tofree);

			fflush(real_stdout);
		}

		fprintf(real_stdout, "1..%lu\n", num_cases);
	} else {
		fputs("0..0\n", real_stdout);
	}

	return had_failure;
}
