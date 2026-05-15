# SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Starlark rules for creating xfail tests."""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

def xfail_test(
        binary_rule,
        name,
        size = None,
        data = [],
        args = [],
        tags = [],
        **kwargs):
    binary_rule(
        name = name + "_bin",
        visibility = ["//visibility:private"],
        testonly = True,
        tags = tags,
        **kwargs
    )

    sh_test(
        name = name,
        size = size,
        srcs = ["//bzl:xfail_test_runner"],
        data = [":%s_bin" % name] + data,
        args = ["$(location :%s_bin)" % name] + args,
        tags = tags,
    )
