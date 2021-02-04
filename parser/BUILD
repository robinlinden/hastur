load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "parser",
    hdrs = ["parser.h"],
    deps = ["//dom"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "parser_test",
    srcs = ["test_parser.cpp"],
    size = "small",
    deps = [
        ":parser",
        "@catch2//:catch2_with_main",
    ],
)
