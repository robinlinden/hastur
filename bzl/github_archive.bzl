# SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

"""Starlark rules simplifying depending on GitHub repositories."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def github_archive(name, repo, version, **kwargs):
    owner, repo = repo.split("/")

    # TODO(robinlinden): Use build file if it exists instead of requiring the user to provide
    #   `build_file = None` when they don't want a build file.
    if "build_file" not in kwargs:
        kwargs["build_file"] = "//third_party:%s.BUILD" % name

    http_archive(
        name = name,
        strip_prefix = "%s-%s" % (repo, version.replace("v", "")),
        url = "https://github.com/%s/%s/archive/%s.tar.gz" % (owner, repo, version),
        **kwargs
    )
