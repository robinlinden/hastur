load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

genrule(
    name = "default_css",
    srcs = ["default.css"],
    outs = ["default_css.h"],
    cmd = "xxd -i $< >$@",
)

cc_library(
    name = "css",
    srcs = [":default_css.h"] + glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:base_parser",
        "//util:from_chars",
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
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
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
