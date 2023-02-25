load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "url",
    srcs = ["url.cpp"],
    hdrs = ["url.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:string",
        "//util:uuid",
        "@icu//:common",
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
    ],
)
