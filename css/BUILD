load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "css",
    srcs = [
        "default.cpp",
        "parse.cpp",
    ],
    hdrs = [
        "default.h",
        "parse.h",
        "parser.h",
        "rule.h",
    ],
    data = ["default.css"],
    visibility = ["//visibility:public"],
    deps = ["//util"],
)

cc_test(
    name = "css_test",
    size = "small",
    srcs = ["parser_test.cpp"],
    deps = [
        ":css",
        "//etest",
    ],
)
