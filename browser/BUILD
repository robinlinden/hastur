load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "browser",
    srcs = ["main.cpp"],
    linkopts = select({
        "@bazel_tools//platforms:linux": ["-lpthread"],
        "@bazel_tools//platforms:windows": [],
    }),
    deps = [
        "//parser",
        "@asio",
    ],
)
