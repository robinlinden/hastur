load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "rtti_hack",
    srcs = ["rtti_hack.cpp"],
    hdrs = ["rtti_hack.h"],
    copts = HASTUR_COPTS + select({
        "@platforms//os:windows": ["/GR"],
        "//conditions:default": ["-frtti"],
    }),
    deps = ["@icu//:common"],
)

cc_library(
    name = "url",
    srcs = ["url.cpp"],
    hdrs = ["url.h"],
    copts = HASTUR_COPTS,
    data = ["@icu-data//:icudt72l.dat"],
    visibility = ["//visibility:public"],
    deps = [
        ":rtti_hack",
        "//util:base_parser",
        "//util:string",
        "//util:unicode",
        "//util:uuid",
        "@icu//:common",
        "@spdlog",
    ],
)

cc_test(
    name = "url_test",
    size = "small",
    srcs = ["url_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":url",
        "//etest",
        "@icu//:common",
    ],
)

cc_fuzz_test(
    name = "url_fuzz_test",
    size = "small",
    srcs = ["url_fuzz_test.cpp"],
    copts = HASTUR_COPTS,
    corpus = glob(["url_fuzz_test_corpus/**"]),
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":url"],
)
