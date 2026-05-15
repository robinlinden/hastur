# SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""cc_xfail_test rule definition."""

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:xfail_test.bzl", "xfail_test")

def cc_xfail_test(**kwargs):
    xfail_test(binary_rule = cc_binary, **kwargs)
