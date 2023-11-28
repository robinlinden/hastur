# SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Common copts for Hastur targets."""

HASTUR_LINUX_WARNING_FLAGS = [
    "-Wall",
    "-Wextra",
    "-pedantic-errors",
    "-Werror",
    "-Wdouble-promotion",
    "-Wformat=2",
    "-Wmissing-declarations",
    "-Wnull-dereference",
    "-Wshadow",
    "-Wsign-compare",
    "-Wundef",
    "-fno-common",
    "-Wnon-virtual-dtor",
    "-Woverloaded-virtual",
    # Common idiom for zeroing members.
    "-Wno-missing-field-initializers",
]

HASTUR_MSVC_WARNING_FLAGS = [
    # More warnings.
    "/W4",
    # Treat warnings as errors.
    "/WX",
]

HASTUR_CLANG_CL_WARNING_FLAGS = [
    "-Wno-error",
    "-Wno-missing-field-initializers",
    "-Wno-unused-command-line-argument",
]

# Only a compiler named exactly "gcc" matches "@rules_cc//cc/compiler:gcc", so
# we still need a default-branch for gcc-11, gcc-12, etc.
# https://github.com/bazelbuild/bazel/issues/12707
# https://github.com/bazelbuild/bazel/issues/17794
HASTUR_COPTS = select({
    "@rules_cc//cc/compiler:clang": HASTUR_LINUX_WARNING_FLAGS,
    "@rules_cc//cc/compiler:clang-cl": HASTUR_CLANG_CL_WARNING_FLAGS,
    "@rules_cc//cc/compiler:msvc-cl": HASTUR_MSVC_WARNING_FLAGS,
    "//conditions:default": HASTUR_LINUX_WARNING_FLAGS,
})

# C++ fuzzing requires a Clang compiler: https://github.com/bazelbuild/rules_fuzzing#prerequisites
HASTUR_FUZZ_PLATFORMS = select({
    "@rules_cc//cc/compiler:clang": ["@platforms//os:linux"],
    "//conditions:default": ["@platforms//:incompatible"],
})
