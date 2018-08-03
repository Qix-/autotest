# Autotest

Autotest is a minimalistic testing framework written in ANSI C (C89/C90)
for ELF-based executables.

```c
/* test-foo.c */
#include <assert.h>

void TEST_foo(void) {
	assert(100 < 500);
}

void TEST_bar(void) {
	assert(15 == 10);
}
```

```console
$ cc -o test-foo test-foo.c -lautotest
$ ./test-foo
TAP version 13
ok 1 foo
not ok 2 bar
  ---
  message: "assertion failed: 15 == 10 (test-foo.c line 9)"
  severity: "fail"
  ...
1..2
```

## Usage

1. Write test functions with a void signature and `TEST_` identifier prefix (e.g. `void TEST_something(void) {...}`).
   Make sure they're [visible](https://gcc.gnu.org/wiki/Visibility) (this is the case by default).
2. Link test sources (or objects) against `-lautotest` into an executable. See `./executable --help` for options.

Assertions should use C's built-in `assert()` (from `assert.h`), or any `abort()`-based assertion library.

> **NOTE:** Do not specify a `main()` function - Autotest provides one for you.

### Ignoring `SIGABRT` or `SIGSEGV`

If you want to allow either a `SIGSEGV` (segfault) or `SIGABRT` (abort, e.g. when calling `assert(0)`), add
`SEGV_` and/or `ABRT_` after `TEST_`, respectively.

```c
#include <assert.h>

void TEST_SEGV_test_segault(void) {
	int *literally_death = (int *) 0x0;
	*literally_death = 0xDEADBEEF; /* segfault; ignored, test will pass */
}

void TEST_ABRT_test_abort(void) {
	assert(!"This will cause the test to fail"); /* abort; ignoreed, test will pass */
}
```

Note that this doesn't _expect_ the test to fail; see below for how to do that.

### Expecting failures

If you want to _expect_ that a test fails, add `FAIL_` after `TEST_`.

```c
#include <assert.h>

void TEST_FAIL_test_abort_should_pass(void) {
	assert(!"This should pass");
}

void TEST_FAIL_test_abort_should_pass(void) {
	/* doesn't segfault/abort - test will FAIL */
}
```

## Building

```console
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=On
```

## FAQ
Some (not-so-)frequently asked questions and their answers.

#### Does this work on MacOS?

No; MacOS uses Mach-O files, not ELF. If you want to submit a PR that enumerates
Mach-O symbols, feel free to submit a PR!

#### Does this work on Windows?

No; Windows has an entirely different API for symbol enumeration and requires a .pdb file.
If you want to submit a PR that handles all of this, we'd be very appreciative.

#### Can I prefix test cases with `static`?

No; `static` prevents the procedure (function) from emitting a publicly visible symbol, which
we use to filter out symbols.

#### Why `TEST_` and not `test_`?

The capitalization further prevents accidentally running non-test functions in dependencies
or third-party libraries (i.e. functions that are supposed to be in release-mode builds that
happen to start with `test_`).

We feel it is much less likely that exported release symbols start with `TEST_` than `test_`,
hence the choice.

#### Are there caveats to this method of testing?

Not really. Philosophically, enumerations of functions _should_ be explicitly listed somewhere -
a corollary being 'magic' (i.e. magic detection or enumeration of things) is generally an anti-pattern -
however, I feel that test cases can bypass this convention without suffering any maintainability setbacks.

To further complement this opinion, writing tests is generally considered tedious and boring, so
I feel the less time taken to set up testing and all of the surrounding harnesses, the better.

#### Why the default of TAP?

Output formats are subjective, and there are [quite a lot of things](https://github.com/sindresorhus/awesome-tap#reporters)
that work with TAP output.

Plus, it was easier to implement.

#### Why does Autotest route all stdout to stderr?

Because according to the [TAP Version 13 Specification](https://testanything.org/tap-version-13-specification.html),
all extraneous lines of output must be routed to stderr. Consumers of the TAP data must only use stdout.

If this messes with your tests, you're probably using an anti-pattern and thus we probably won't support
your use-case (but feel free to open an issue anyway).

# License

Autotest is copyright &copy; 2018 by Josh Junon and released under the MIT License. Unless
otherwise noted, all code included herein is licensed as such.

Certain functionality included is licensed (and compliant) under the
[APSL 2.0](https://opensource.apple.com/apsl) and is noted as such.
