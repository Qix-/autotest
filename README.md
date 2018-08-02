# Autotest

Autotest is a minimalistic testing framework written in ANSI C (C89/C90)
for UNIX-based systems.

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

  ✔  PASS foo
  ✕  FAIL bar - assertion failed: 15 == 10 (test-foo.c line 9)

  1 passed
  1 failed

  TESTS FAIL
```

## Usage

1. Write test functions with a void signature and `TEST_` identifier prefix (e.g. `void TEST_something(void) {...}`).
   Make sure they're [visible](https://gcc.gnu.org/wiki/Visibility) (this is the case by default).
2. Link test sources (or objects) against `-lautotest` into an executable. See `./executable --help` for options.

Assertions should use C's built-in `assert()` (from `assert.h`), or any `abort()`-based assertion library.

> **NOTE:** 

> **NOTE:** Do not specify a `main()` function - Autotest provides one for you.

## Building

```console
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=On
```

## FAQ
Some (not-so-)frequently asked questions and their answers.

#### Will this work on non-UNIX platforms?

No - at least not without modification. If you want to add a new platform that doesn't
support enumerating symbols using `nm`'s method or doesn't allow referencing symbols
using `libdl` (`dlsym`, etc.) then you'll need to add a custom test case enumerator
(and submit a PR, if you would!).

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

#### Why not default to TAP?

Because there aren't enough obvious choices in TAP consumers, and none that we've found we've liked enough
to actively recommend or push.

Feel free to submit a pull request adding a list to this README with your favorite.

#### Is Autotest's use of `fork()` required?

No, but it allows Autotest to make the output a bit cleaner. If the use of `fork()` is causing
issues, pass `--verbose` as it's the only way to avoid `fork()`-ing.

# License

Autotest is copyright &copy; 2018 by Josh Junon and released under the MIT License. Unless
otherwise noted, all code included herein is licensed as such.

Certain functionality included is licensed (and compliant) under the
[APSL 2.0](https://opensource.apple.com/apsl) and is noted as such.
