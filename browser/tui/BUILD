load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//engine",
        "//protocol",
        "//tui",
        "//uri",
        "@spdlog",
    ],
)
