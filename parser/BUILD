load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "parser",
    hdrs = ["parser.h"],
    visibility = ["//visibility:public"],
    deps = ["//dom"],
)

cc_test(
    name = "parser_test",
    size = "small",
    srcs = ["test_parser.cpp"],
    deps = [
        ":parser",
        "@catch2//:catch2_with_main",
    ],
)
