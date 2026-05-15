# SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""sh_xfail_test rule definition."""

load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("//bzl:xfail_test.bzl", "xfail_test")

def sh_xfail_test(**kwargs):
    xfail_test(binary_rule = sh_binary, **kwargs)
