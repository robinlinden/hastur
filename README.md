# Toy browser engine

[![codecov](https://codecov.io/gh/robinlinden/hastur/branch/master/graph/badge.svg?token=1H16FDJ3ML)][codecov]

## Building

### Requirements

#### Compiler

Right now GCC 10, Clang 12, and MSVC are tested against. The project makes use
of C++20 features, so a reasonably recent compiler is required.

#### Build system

[Bazel][bazel] is used as the build system. I recommend using
[Bazelisk][bazelisk] as that will pick up the Bazel version to use from the
`.bazelversion` file in the repository root.

Per-developer configuration (e.g.  compiler used and build type) is managed in
a gitignored `.bazelrc.local` file. To set this up for your environment, copy
`.bazelrc.local.example` to `.bazelrc.local` and edit to suit your compiler of
choice.

### Process

The following assumes that you either have Bazel or Bazelisk under the name
`bazel` on your `PATH` and that you have set up your `.bazelrc.local` file.

#### Listing build targets

`bazel query //...`

#### Building all targets

`bazel build //...`

#### Building a single target

`bazel build //html`

#### Running all tests

`bazel test //...`

#### Running the browser engine

`bazel run //browser`

#### Generate json compilation database

`bazel run refresh_compile_commands`

[bazel]: https://bazel.build
[bazelisk]: https://github.com/bazelbuild/bazelisk
[codecov]: https://codecov.io/gh/robinlinden/hastur
