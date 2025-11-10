# SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Starlark rules for creating xfail tests."""

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

def cc_xfail_test(
        name,
        size = None,
        args = [],
        data = [],
        **kwargs):
    cc_binary(
        name = name + "_bin",
        visibility = ["//visibility:private"],
        testonly = True,
        **kwargs
    )

    sh_test(
        name = name,
        size = size,
        srcs = ["//bzl:xfail_test_runner"],
        data = [":%s_bin" % name] + data,
        args = ["$(location :%s_bin)" % name] + args,
    )

def sh_xfail_test(
        name,
        size = None,
        args = [],
        data = [],
        **kwargs):
    sh_binary(
        name = name + "_bin",
        visibility = ["//visibility:private"],
        testonly = True,
        **kwargs
    )

    sh_test(
        name = name,
        size = size,
        srcs = ["//bzl:xfail_test_runner"],
        data = [":%s_bin" % name] + data,
        args = ["$(location :%s_bin)" % name] + args,
    )
