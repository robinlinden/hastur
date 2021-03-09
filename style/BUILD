load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "style",
    srcs = ["style.cpp"],
    hdrs = ["style.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
    ],
)

cc_test(
    name = "style_test",
    size = "small",
    srcs = ["style_test.cpp"],
    deps = [
        ":style",
        "//etest",
    ],
)
