# SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Starlark rules simplifying depending on GitHub repositories."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def github_archive(name, repo, version, **kwargs):
    owner, repo = repo.split("/")
    http_archive(
        name = name,
        strip_prefix = "%s-%s" % (repo, version.replace("v", "")),
        url = "https://github.com/%s/%s/archive/%s.tar.gz" % (owner, repo, version),
        **kwargs
    )
