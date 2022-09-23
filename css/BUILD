load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")

genrule(
    name = "default_css",
    srcs = ["default.css"],
    outs = ["default_css.h"],
    cmd = "xxd -i $< >$@",
)

cc_library(
    name = "css",
    srcs = [
        "default.cpp",
        "parse.cpp",
        "rule.cpp",
        ":default_css.h",
    ],
    hdrs = [
        "default.h",
        "parse.h",
        "parser.h",
        "rule.h",
    ],
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

cc_fuzz_test(
    name = "css_parser_fuzz_test",
    srcs = ["parser_fuzz_test.cpp"],
    tags = ["manual"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [":css"],
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
