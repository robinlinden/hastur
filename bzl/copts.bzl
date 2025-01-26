# SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
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
    "-Wnrvo",
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
    # -Wall in clang-cl is an alias of -Weverything, and -W4 is an alias of clang's -Wall + -Wextra.
    "-W4",
    "-Werror",
    # Common idiom for zeroing members.
    "-Wno-missing-field-initializers",
]

HASTUR_COPTS = select({
    "@rules_cc//cc/compiler:clang": HASTUR_LINUX_WARNING_FLAGS,
    "@rules_cc//cc/compiler:clang-cl": HASTUR_CLANG_CL_WARNING_FLAGS,
    "@rules_cc//cc/compiler:gcc": HASTUR_LINUX_WARNING_FLAGS,
    "@rules_cc//cc/compiler:msvc-cl": HASTUR_MSVC_WARNING_FLAGS,
})

# C++ fuzzing requires a Clang compiler: https://github.com/bazelbuild/rules_fuzzing#prerequisites
HASTUR_FUZZ_PLATFORMS = select({
    "@rules_cc//cc/compiler:clang": ["//bzl:linux_or_macos"],
    "//conditions:default": ["@platforms//:incompatible"],
})
