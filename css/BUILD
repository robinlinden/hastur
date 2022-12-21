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
        "//util:string",
        "@fmt",
        "@spdlog",
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

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":css",
        "//etest",
        "@fmt",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]
