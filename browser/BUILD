load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "browser",
    srcs = ["main.cpp"],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    deps = [
        "//html",
        "//http",
        "//tui",
        "@spdlog",
    ],
)
