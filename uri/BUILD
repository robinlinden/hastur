load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "uri",
    srcs = ["uri.cpp"],
    hdrs = ["uri.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:string",
        "@fmt",
    ],
)

cc_test(
    name = "uri_test",
    size = "small",
    srcs = ["uri_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":uri",
        "//etest",
    ],
)

cc_fuzz_test(
    name = "uri_fuzz_test",
    size = "small",
    testonly = True,
    srcs = ["uri_fuzz_test.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":uri"],
)
