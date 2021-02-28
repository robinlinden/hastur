load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "css",
    srcs = ["parse.cpp"],
    hdrs = [
        "parse.h",
        "parser.h",
        "rule.h",
    ],
    visibility = ["//visibility:public"],
    deps = ["//util"],
)

cc_test(
    name = "css_test",
    size = "small",
    srcs = ["parser_test.cpp"],
    deps = [
        ":css",
        "@catch2//:catch2_with_main",
    ],
)
