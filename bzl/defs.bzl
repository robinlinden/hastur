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

# Code ripped straight from bazelbuild examples
def _var_providing_rule_impl(ctx):
    return [
        platform_common.TemplateVariableInfo({
            "FOO": ctx.attr.var_value,
        }),
    ]

var_providing_rule = rule(
    implementation = _var_providing_rule_impl,
    attrs = {"var_value": attr.string()},
)
