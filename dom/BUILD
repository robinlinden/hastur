load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "dom",
    srcs = ["dom.cpp"],
    hdrs = ["dom.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:overloaded",
    ],
)

cc_test(
    name = "dom_test",
    size = "small",
    srcs = ["dom_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":dom",
        "//etest",
    ],
)
