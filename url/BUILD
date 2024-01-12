load("@rules_cc//cc:defs.bzl", "cc_import", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_import(
    name = "icudata",
    static_library = select({
        "@platforms//os:windows": "@icu//:sicudt74l.lib",
        "//conditions:default": "@icu//:libicudt74l.a",
    }),
    tags = ["no-cross"],
    visibility = ["//visibility:private"],
    alwayslink = True,
)

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
    srcs = [
        "url.cpp",
    ],
    hdrs = ["url.h"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
        ":icudata",
        ":rtti_hack",
        "//util:base_parser",
        "//util:string",
        "//util:unicode",
        "//util:uuid",
        "@icu//:common",
    ],
)

cc_test(
    name = "url_test",
    size = "small",
    srcs = ["url_test.cpp"],
    copts = HASTUR_COPTS + select({
        "@platforms//os:windows": [
            "/wd4100",
            "/wd4706",
        ],
        "//conditions:default": [],
    }),
    data = ["@wpt//:url/resources/urltestdata.json"],
    tags = ["no-cross"],
    deps = [
        ":url",
        "//etest",
        "@icu//:common",
        "@simdjson",
    ],
)

cc_fuzz_test(
    name = "url_fuzz_test",
    size = "small",
    srcs = ["url_fuzz_test.cpp"],
    copts = HASTUR_COPTS,
    corpus = glob(["url_fuzz_test_corpus/**"]),
    tags = ["no-cross"],
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":url"],
)
