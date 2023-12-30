load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "style",
    srcs = [
        "style.cpp",
        "styled_node.cpp",
    ],
    hdrs = [
        "style.h",
        "styled_node.h",
    ],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//gfx",
        "//util:from_chars",
        "//util:string",
        "@spdlog",
    ],
)

cc_test(
    name = "style_test",
    size = "small",
    srcs = ["style_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":style",
        "//css",
        "//dom",
        "//etest",
        "@fmt",
    ],
)

cc_test(
    name = "styled_node_test",
    size = "small",
    srcs = ["styled_node_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":style",
        "//css",
        "//dom",
        "//etest",
        "//gfx",
    ],
)
