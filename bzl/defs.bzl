# SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Starlark rules for creating xfail tests."""

load("@rules_cc//cc:defs.bzl", "cc_binary")

def cc_xfail_test(
        name,
        size = None,
        **kwargs):
    cc_binary(
        name = name + "_bin",
        visibility = ["//visibility:private"],
        testonly = True,
        **kwargs
    )

    native.sh_test(
        name = name,
        size = size,
        srcs = ["//bzl:xfail_test_runner"],
        data = [":%s_bin" % name],
        args = ["$(location :%s_bin)" % name],
    )
