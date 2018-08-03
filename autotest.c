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

static jmp_buf jmpbuf_harness;
static jmp_buf jmpbuf_test;
static FILE *real_stdout;
static int real_stdout_fd;

__attribute__((noreturn)) void bailout(const char *fmt, ...) {
	va_list ap;
	fputs("Bail out! ", real_stdout);
	va_start(ap, fmt);
	vfprintf(real_stdout, fmt, ap);
	va_end(ap);
	fputs("\n", real_stdout);
	longjmp(jmpbuf_harness, 1);
	__builtin_unreachable();
}

static void handle_sig(int sig) {
	if (sig == SIGSEGV) {
		longjmp(jmpbuf_test, 2);
	} else if (sig == SIGABRT) {
		longjmp(jmpbuf_test, 1);
	} else {
		bailout("Autotest signal handler encountered unknown signal: %s (%d)", strsignal(sig), sig);
	}
}

int main(int argc, const char **argv) {
	test_case *test_cases;
	volatile int had_failure = 0;

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

	if (setjmp(jmpbuf_harness) == 1) {
		/* bailout() was called */
		return 1;
	}

	discover_tests(argv[0], &test_cases);

	fputs("TAP version 13\n", real_stdout);

	if (test_cases) {
		volatile size_t num_cases = 0;
		const test_case *tcase;
		const test_case *tcase_tofree;
		sighandler_t old_handler_segv;
		sighandler_t old_handler_abrt;

		old_handler_segv = signal(SIGSEGV, &handle_sig);
		if (old_handler_segv == SIG_ERR) {
			bailout("Could not set SIGSEGV signal handler: %s", strerror(errno));
		}

		old_handler_abrt = signal(SIGABRT, &handle_sig);
		if (old_handler_abrt == SIG_ERR) {
			bailout("Could not set SIGABRT signal handler: %s", strerror(errno));
		}

		for (tcase = test_cases; tcase;) {
			++num_cases;

			tcase_tofree = NULL;

			if (tcase->skipped) {
				fprintf(real_stdout, "ok %lu %s # SKIP Symbol prefixed with underscore (_TEST_%s)\n", num_cases, tcase->name, tcase->name);
			} else {
				switch (setjmp(jmpbuf_test)) {
				case 0:
					tcase->fn();
					fprintf(real_stdout, "ok %lu %s\n", num_cases, tcase->name);
					break;
				case 1:
					had_failure = 1;
					fprintf(real_stdout, "not ok %lu %s\n  ---\n  message: Test case aborted\n  severity: fail\n  ...\n", num_cases, tcase->name);
					break;
				case 2:
					had_failure = 1;
					fprintf(real_stdout, "not ok %lu %s\n  ---\n  message: Test case segfaulted\n  severity: fail\n  ...\n", num_cases, tcase->name);
					break;
				}
			}

			tcase_tofree = tcase;
			tcase = tcase->next;
			free((void *) tcase_tofree);

			fflush(stderr);
			fflush(real_stdout);
		}

		if (signal(SIGSEGV, old_handler_segv) == SIG_ERR) {
			fprintf(stderr, "%s: warning: could not restore SIGSEGV signal handler: %s\n", argv[0], strerror(errno));
		}
		if (signal(SIGABRT, old_handler_abrt) == SIG_ERR) {
			fprintf(stderr, "%s: warning: could not restore SIGABRT signal handler: %s\n", argv[0], strerror(errno));
		}

		fprintf(real_stdout, "1..%lu\n", num_cases);
	} else {
		fputs("0..0\n", real_stdout);
	}

	return had_failure;
}
