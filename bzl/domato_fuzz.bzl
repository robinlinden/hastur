# SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Macro for continuously running targets against Domato-generated HTML."""

load("@rules_python//python:defs.bzl", "py_binary")

def domato_fuzz(name, tested_target, tested_target_args = []):
    """Runs a target repeatedly on newly generated Domato HTML samples."""
    py_binary(
        name = name,
        srcs = ["//bzl:domato_fuzz_runner.py"],
        main = "//bzl:domato_fuzz_runner.py",
        args = [
            "$(rlocationpath {})".format(tested_target),
            "$(rlocationpath @domato//:domato)",
        ] + tested_target_args,
        data = [
            tested_target,
            "@domato//:domato",
        ],
        deps = ["@rules_python//python/runfiles"],
    )
