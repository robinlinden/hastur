load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "etest",
    srcs = ["etest.cpp"],
    hdrs = ["etest.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "etest_test",
    size = "small",
    srcs = ["etest_test.cpp"],
    deps = [":etest"],
)
