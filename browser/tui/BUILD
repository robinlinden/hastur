load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//engine",
        "//protocol",
        "//tui",
        "//uri",
        "@fmt",
        "@spdlog",
    ],
)
