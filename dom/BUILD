load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "dom",
    srcs = ["dom.cpp"],
    hdrs = ["dom.h"],
    visibility = ["//visibility:public"],
    deps = ["@range-v3"],
)

cc_test(
    name = "dom_test",
    size = "small",
    srcs = ["dom_test.cpp"],
    deps = [
        ":dom",
        "//etest",
    ],
)
