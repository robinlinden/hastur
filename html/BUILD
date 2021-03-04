load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "html",
    srcs = ["parse.cpp"],
    hdrs = [
        "parse.h",
        "parser.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//util",
    ],
)

cc_test(
    name = "html_test",
    size = "small",
    srcs = ["test_parser.cpp"],
    deps = [
        ":html",
        "//etest",
    ],
)
