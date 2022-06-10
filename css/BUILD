load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "css",
    srcs = [
        "default.cpp",
        "parse.cpp",
        "rule.cpp",
    ],
    hdrs = [
        "default.h",
        "parse.h",
        "parser.h",
        "rule.h",
    ],
    data = ["default.css"],
    visibility = ["//visibility:public"],
    deps = [
        "//util:base_parser",
        "@fmt",
    ],
)

cc_test(
    name = "css_parser_test",
    size = "small",
    srcs = ["parser_test.cpp"],
    deps = [
        ":css",
        "//etest",
        "@fmt",
    ],
)

cc_test(
    name = "css_rule_test",
    size = "small",
    srcs = ["rule_test.cpp"],
    deps = [
        ":css",
        "//etest",
    ],
)
