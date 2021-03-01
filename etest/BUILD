load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "etest",
    srcs = ["etest.cpp"],
    hdrs = ["etest.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "etest_with_main",
    srcs = ["etest_main.cpp"],
    visibility = ["//visibility:public"],
    deps = [":etest"],
)

cc_test(
    name = "etest_test",
    size = "small",
    srcs = ["etest_test.cpp"],
    deps = [":etest_with_main"],
)
