load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "geom",
    hdrs = ["geom.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "geom_test",
    size = "small",
    srcs = ["geom_test.cpp"],
    deps = [
        ":geom",
        "//etest",
    ],
)
