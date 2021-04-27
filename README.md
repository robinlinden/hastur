# Toy browser engine

## Building

Note that GCC 10, Clang 10, and MSVC are tested against. The project makes use
of C++20 features, so a reasonably recent compiler is required.

### With Bazel

#### Requirements

[Bazel][bazel] is used as the build system. I recommend using
[Bazelisk][bazelisk] as that will pick up the Bazel version to use from the
`.bazelversion` file in the repository root.

Per-developer configuration (e.g.  compiler used and build type) is managed in
a gitignored `.bazelrc.local` file. To set this up for your environment, copy
`.bazelrc.local.example` to `.bazelrc.local` and edit to suit your compiler of
choice.

#### Process

The following assumes that you either have Bazel or Bazelisk under the name
`bazel` on your `PATH` and that you have set up your `.bazelrc.local` file.

##### Listing build targets

`bazel query //...`

##### Building all targets

`bazel build //...`

##### Building a single target

`bazel build //html`

##### Running all tests

`bazel test //...`

##### Running the browser engine

`bazel run //browser`

### With CMake

Please note that the current CMakeLists does not support hastur's tests, and is
not configured to build the test gui.

#### Requirements

To build with CMake, ensure that version 3.20 or later of CMake is installed,
and that the following packages are installed on your system:

- [asio][asio]
- [fmt][fmt]
- [spdlog][spdlog]

### Process

```
cmake . && make
```

Static libraries will be output to the `lib/` directory.

[asio]: https://github.com/boostorg/asio/
[bazel]: https://bazel.build
[bazelisk]: https://github.com/bazelbuild/bazelisk
[fmt]: https://github.com/fmtlib/fmt
[spdlog]: https://github.com/gabime/spdlog
