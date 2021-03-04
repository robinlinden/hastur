load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "util",
    hdrs = ["base_parser.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "util_test",
    size = "small",
    srcs = ["base_parser_test.cpp"],
    deps = [
        ":util",
        "//etest",
    ],
)
