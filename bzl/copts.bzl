# SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
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

HASTUR_COPTS = select({
    "//bzl:is_clang-cl": HASTUR_CLANG_CL_WARNING_FLAGS,
    "//bzl:is_msvc": HASTUR_MSVC_WARNING_FLAGS,
    "@platforms//os:linux": HASTUR_LINUX_WARNING_FLAGS,
})
