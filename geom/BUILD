load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "geom",
    hdrs = ["geom.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

cc_test(
    name = "geom_test",
    size = "small",
    srcs = ["geom_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":geom",
        "//etest",
    ],
)
