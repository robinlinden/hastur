load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "type",
    hdrs = ["type.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "naive",
    hdrs = ["naive.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [":type"],
)

cc_test(
    name = "naive_test",
    size = "small",
    srcs = ["naive_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":naive",
        "//etest",
    ],
)
