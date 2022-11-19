load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

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
        "property_id.cpp",
        "rule.cpp",
        ":default_css.h",
    ],
    hdrs = [
        "default.h",
        "parse.h",
        "parser.h",
        "property_id.h",
        "rule.h",
    ],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:base_parser",
        "@fmt",
        "@spdlog",
    ],
)

cc_test(
    name = "css_parser_test",
    size = "small",
    srcs = ["parser_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":css",
        "//etest",
        "@fmt",
    ],
)

cc_fuzz_test(
    name = "css_parser_fuzz_test",
    srcs = ["parser_fuzz_test.cpp"],
    copts = HASTUR_COPTS,
    tags = ["manual"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [":css"],
)

cc_test(
    name = "css_rule_test",
    size = "small",
    srcs = ["rule_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":css",
        "//etest",
    ],
)

cc_test(
    name = "property_id_test",
    size = "small",
    srcs = ["property_id_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":css",
        "//etest",
        "@fmt",
    ],
)
